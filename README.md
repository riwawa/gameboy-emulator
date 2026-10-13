# Game Boy Emulator

A Game Boy (DMG-01) emulator written in C, developed from scratch as a learning project in computer architecture, low-level programming, and embedded systems.

> **Work in progress:** This project is in early development and is not yet capable of running Game Boy games.

## Overview

The goal of this project is to understand how the original Nintendo Game Boy works by implementing its core components from scratch.

Rather than focusing on building a highly optimized emulator, the emphasis is on learning how the hardware operates, including CPU instruction decoding, memory organization, communication between components, and eventually graphics and input handling.

The emulator is the first stage of a larger project: **building a standalone, physical Game Boy-inspired handheld device using an embedded platform**, with its own display, buttons, and supporting electronics.

## Development Status

The project is currently in its early stages.

| Component | Status |
|---|---|
| Cartridge loading and ROM header parsing | Initial implementation |
| Memory bus | In development |
| SM83 CPU registers and context | Initial implementation |
| Instruction decoding | In progress |
| CPU execution | Not implemented |
| Interrupt handling | Planned |
| PPU (graphics) | Planned |
| APU (audio) | Planned |
| Timer and input | Planned |
| Embedded hardware integration | Long-term goal |

## Target Hardware & Architecture

The original Nintendo Game Boy (DMG-01), released in 1989, is an 8-bit handheld console built around the **Sharp SM83 CPU**, a processor often described as Z80-like, although it features its own instruction set and several architectural differences.

### Original Game Boy (DMG-01) Specifications

| Component | Specification |
|---|---|
| CPU | Sharp SM83 (8-bit, Z80-like architecture) |
| Clock Speed | ~4.19 MHz |
| Address Space | 64 KB (16-bit addressing) |
| Work RAM | 8 KB |
| Video RAM | 8 KB |
| Display | 160 × 144 pixels |
| Graphics | 4 shades of gray, displayed as green tones on the original LCD |
| Audio | 4 sound channels |
| Cartridge | ROM cartridges with optional memory bank controllers (MBCs) |

### Emulator Implementation

The emulator is being developed from scratch in **C**, with an emphasis on understanding the original hardware rather than relying on existing emulation frameworks.

The implementation is organized around the console's main components, including the cartridge interface, memory bus, CPU instruction decoding and execution, and eventually the PPU, APU, timers, and input system.

The long-term goal is to bring the emulator to a dedicated embedded platform and build a standalone handheld device with a physical display, buttons, and custom electronics.
## References

- [Pan Docs](https://gbdev.io/pandocs/) — Game Boy technical documentation
- [GBDev Opcode Tables](https://gbdev.io/gb-opcodes/optables/) — CPU instruction reference
- [Game Boy CPU Manual](https://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf) — CPU reference
- [Code — Charles Petzold](https://www.charlespetzold.com/books/code/) — Computer hardware and software fundamentals
- [Low Level Devel](https://www.youtube.com/playlist?list=PLVxiWMqQvhg_yk4qy2cSC3457wZJga_e5) - Youtube Emulator coURSE

## Disclaimer
This is an independent educational project and is not affiliated with or endorsed by Nintendo.
Game Boy is a trademark of Nintendo. No copyrighted game ROMs or proprietary boot ROMs are distributed with this project.