# Game Boy Emulator (DMG-01)

A Game Boy (DMG-01) emulator written in C from scratch, built as a learning project in computer architecture and low-level programming. It is the first stage of a larger goal: running the same emulator core on a **standalone, physical handheld** built on an embedded platform, with its own display, buttons, speaker and electronics.

> **Status:** plays real games (Tetris, Super Mario Land, Pokémon, Link's Awakening, Final Fantasy Adventure and others) with sound and battery saves. CPU, interrupts, timer, graphics, audio and the MBC1, MBC2, MBC3 and MBC5 cartridge chips are implemented and checked against reference test ROMs. Game Boy Color-only games are not supported.

## Features

- Sharp SM83 CPU: full instruction set, interrupts, HALT, EI delay
- Memory bus with VRAM, WRAM (plus echo), OAM, HRAM and I/O registers
- Timer (DIV / TIMA / TMA / TAC) driven by the 16-bit system counter, including falling-edge quirks and delayed reload
- PPU: modes 2/3/0/1 timing, LY/LYC and STAT interrupts, background, window, sprites (8x8 and 8x16), palettes, OAM DMA
- APU: four channels (two pulse with sweep and envelope, wave, noise), frame sequencer, length counters, stereo mixing at 44.1 kHz
- Joypad matrix with interrupt
- Cartridges: ROM-only, MBC1, MBC2, MBC3 (with real-time clock) and MBC5 (rumble is ignored)
- Battery saves: a `.sav` file next to the ROM, compatible with other emulators (see [Saves](#saves))
- SDL2 front end: scaled window, ~59.7 fps pacing, keyboard input, sound, BMP screenshots

## Validation

The emulator is checked against independent reference suites, not just against itself. All numbers below come from the current code.

| Suite | Result |
|---|---|
| [SingleStepTests/sm83](https://github.com/SingleStepTests/sm83) (per-instruction CPU states) | 498 / 500 opcodes (HALT and STOP differ by convention) |
| Blargg `cpu_instrs` (all 11 + combined 64 KB ROM) | Passed |
| Blargg `instr_timing`, `mem_timing` | Passed |
| Blargg `dmg_sound` | 9 / 12 (see known gaps) |
| Mooneye `acceptance/timer/*` | 13 / 13 |
| Mooneye `div_timing`, `ei_sequence`, `ei_timing`, `intr_timing`, `di_timing`, `if_ie_registers`, `halt_*` | Passed |
| Mooneye `emulator-only/mbc1/*` | 12 / 13 (multicart wiring not implemented) |
| Mooneye `emulator-only/mbc2/*` | 7 / 7 |
| Mooneye `emulator-only/mbc5/*` | 8 / 8 |
| `mbc3-tester` and `rtc3test` (MBC3 banking and clock) | Screens match the reference |
| [dmg-acid2](https://github.com/mattcurrie/dmg-acid2) | Pixel-exact |

### Known gaps

- Blargg `dmg_sound` 09, 10, 12: wave channel timing when RAM is read or written while the channel is playing
- Mooneye `add_sp_e_timing`, `call_timing`, `jp_timing`, `push_timing`, `rst_timing`, `ret_timing`, `reti_timing`: the CPU accounts for time per instruction, not per memory access
- Mooneye `ie_push`, `boot_hwio`: boot state and an interrupt-dispatch corner case
- Blargg `oam_bug`: the OAM corruption bug is not emulated
- Game Boy Color: CGB-only games do not run (they need the colour PPU, extra banks and double speed)
- MBC5 rumble is accepted but does nothing

## Build and run

Requirements: a C compiler (clang or gcc), `make` and SDL2 (`brew install sdl2` on macOS).

```bash
make
make run ROM="path/to/game.gb"
```

Controls:

| Key | Button |
|---|---|
| Arrow keys | D-pad |
| X | A |
| Z | B |
| Enter | Start |
| Backspace | Select |
| F12 | Screenshot (`tela.bmp`) |
| Esc | Quit |

Game ROMs and the Blargg/Mooneye test ROMs are not in the repository (they are ignored by git). Put them under `tests/` after downloading them. The hand-written test ROMs and their runner are versioned in `tests/`.

## Saves

For cartridges with a battery, the emulator loads `game.sav` (next to `game.gb`) when it starts and writes it when you quit and a couple of seconds after the cartridge RAM changes, so a crash loses little. The file is the cartridge RAM followed by, on MBC3 cartridges with a clock, 48 bytes of real-time clock state in the layout used by VBA, BGB and SameBoy. When a game is reopened, the clock advances by the real time that passed.

Writes go to a temporary file that replaces the old save only when complete.

## Architecture

The code is split into a portable **core** and a thin **platform** layer, so the same core can later be linked into firmware.

```
include/, src/        core: CPU, bus, PPU, APU, timer, joypad, cartridge, emu
platform/sdl/         desktop front end (window, input, sound, timing, save files)
tests/                test ROMs and runners
```

The platform owns the main loop and the core advances one frame at a time:

```c
emu_init(rom, size);            // ROM already in memory (RAM or flash), not copied
while (running) {
    emu_run_frame();            // runs the CPU until the PPU finishes one frame
    // the platform shows the frame, reads buttons and waits for the next one
}
emu_shutdown();                 // also writes the battery save
```

The core never touches the screen, keyboard, speaker, files or clock. It talks to the outside world through a small hardware abstraction layer (`include/hal.h`):

| Function | Purpose |
|---|---|
| `hal_draw_line(y, shades)` | receives each finished scanline of 160 pixels (shades 0 to 3) |
| `hal_audio_push(samples, frames)` | receives stereo 16-bit samples at 44.1 kHz |
| `hal_save_load(buf, cap)` / `hal_save_store(buf, len)` | read and write the battery save as a block of bytes |
| `hal_time_unix()` | wall-clock seconds for the cartridge clock (0 if the board has none) |
| `hal_debug_putc(c)` | receives serial output, useful for test ROMs |

Porting to a microcontroller means implementing those functions for a real display, audio output and flash or SD card, and feeding buttons into the joypad module.

### Design notes

- Per-line rendering: the PPU draws a whole scanline when mode 3 ends, which keeps it fast enough for small targets
- The PPU runs 4 dots per M-cycle and a frame is 70224 dots (17556 M-cycles)
- The cartridge ROM is a `const` pointer read through one function, so it can live in flash on an embedded target
- The bus and cartridge are static, with no heap use except the cartridge RAM
- The cartridge clock counts emulated time, not the host clock, and catches up with real time only when a save is loaded

## References

- [Pan Docs](https://gbdev.io/pandocs/): Game Boy technical documentation
- [GBDev Opcode Tables](https://gbdev.io/gb-opcodes/optables/): CPU instruction reference
- [Game Boy CPU Manual](https://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf)
- [Gekkio's Mooneye Test Suite](https://github.com/Gekkio/mooneye-test-suite) and [Blargg's test ROMs](https://github.com/retrio/gb-test-roms)
- [Low Level Devel](https://www.youtube.com/playlist?list=PLVxiWMqQvhg_yk4qy2cSC3457wZJga_e5): emulator video course
- [Code](https://www.charlespetzold.com/books/code/) by Charles Petzold

