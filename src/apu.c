#include "apu.h"
#include "hal.h"
#include <string.h>

#define M_CYCLES_PER_SEC 1048576u
#define OUT_BUF_FRAMES   1024

typedef struct {
    bool     enabled;      
    bool     len_enable;    
    uint16_t length;        
    uint16_t freq;         
    int      timer;        
    uint8_t  vol;         
    uint8_t  env_timer;
    bool     env_active;
    uint8_t  duty_pos;
} Chan;

static struct {
    bool     power;               
    uint8_t  reg[0x17];           
    Chan     ch[4];
    uint8_t  fs;                  

    uint8_t  sweep_timer;
    uint16_t sweep_shadow;
    bool     sweep_enabled;
    bool     sweep_neg_used;

    uint8_t  wave_ram[16];
    uint8_t  wave_pos;            
    uint8_t  wave_buf;             
    bool     wave_read_now;       

    uint16_t lfsr;

    // saída
    uint32_t sample_acc;
    int32_t  acc_l, acc_r;
    int      acc_n;
    float    cap_l, cap_r;          // capacitores do filtro passa-alta
    int16_t  out[OUT_BUF_FRAMES * 2];
    int      out_n;
} apu;

#define NR(addr) (apu.reg[(addr) - 0xFF10])

static const uint8_t duty_table[4][8] = {
    {0,0,0,0,0,0,0,1}, {1,0,0,0,0,0,0,1}, {1,0,0,0,0,1,1,1}, {0,1,1,1,1,1,1,0},
};

static const uint8_t read_mask[0x17] = {
    0x80, 0x3F, 0x00, 0xFF, 0xBF,         
    0xFF, 0x3F, 0x00, 0xFF, 0xBF,         
    0x7F, 0xFF, 0x9F, 0xFF, 0xBF,
    0xFF, 0xFF, 0x00, 0x00, 0xBF,   
    0x00, 0x00, 0x70,                     
};

static bool dac_on(int i) {
    switch (i) {
    case 0: return NR(0xFF12) & 0xF8;
    case 1: return NR(0xFF17) & 0xF8;
    case 2: return NR(0xFF1A) & 0x80;
    default: return NR(0xFF21) & 0xF8;
    }
}

static int noise_period(void) {                 // em ciclos-M
    uint8_t nr43 = NR(0xFF22);
    int r = nr43 & 7, s = nr43 >> 4;
    return (r ? r * 4 : 2) << s;
}

static int sweep_calc(void) {
    uint8_t nr10 = NR(0xFF10);
    int delta = apu.sweep_shadow >> (nr10 & 7);
    int n;
    if (nr10 & 8) { n = apu.sweep_shadow - delta; apu.sweep_neg_used = true; }
    else          { n = apu.sweep_shadow + delta; }
    return n;
}

static void sweep_tick(void) {
    uint8_t nr10 = NR(0xFF10);
    uint8_t pace = (nr10 >> 4) & 7;
    if (apu.sweep_timer > 0 && --apu.sweep_timer == 0) {
        apu.sweep_timer = pace ? pace : 8;
        if (apu.sweep_enabled && pace) {
            int n = sweep_calc();
            if (n > 2047) { apu.ch[0].enabled = false; return; }
            if (nr10 & 7) {
                apu.sweep_shadow = (uint16_t)n;
                apu.ch[0].freq = (uint16_t)n;
                NR(0xFF13) = n & 0xFF;
                NR(0xFF14) = (NR(0xFF14) & ~7) | ((n >> 8) & 7);
                if (sweep_calc() > 2047) apu.ch[0].enabled = false;
            }
        }
    }
}

static uint8_t env_reg(int i) {
    return i == 0 ? NR(0xFF12) : i == 1 ? NR(0xFF17) : NR(0xFF21);
}

static void env_tick(void) {
    static const int idx[3] = {0, 1, 3};
    for (int k = 0; k < 3; k++) {
        Chan *c = &apu.ch[idx[k]];
        uint8_t r = env_reg(idx[k] == 3 ? 3 : idx[k]);
        uint8_t period = r & 7;
        if (c->env_timer > 0 && --c->env_timer == 0) {
            c->env_timer = period ? period : 8;
            if (c->enabled && c->env_active && period) {
                if (r & 8) { if (c->vol < 15) c->vol++; else c->env_active = false; }
                else       { if (c->vol > 0)  c->vol--; else c->env_active = false; }
            }
        }
    }
}

static void length_tick(void) {
    for (int i = 0; i < 4; i++) {
        Chan *c = &apu.ch[i];
        if (c->len_enable && c->length > 0) {
            if (--c->length == 0) c->enabled = false;
        }
    }
}

void apu_div_tick(void) {
    if (!apu.power) return;
    switch (apu.fs) {
    case 0: case 4: length_tick(); break;
    case 2: case 6: length_tick(); sweep_tick(); break;
    case 7: env_tick(); break;
    default: break;
    }
    apu.fs = (apu.fs + 1) & 7;
}

static void trigger(int i) {
    Chan *c = &apu.ch[i];
    uint16_t maxlen = (i == 2) ? 256 : 64;

    if (dac_on(i)) c->enabled = true;

    if (c->length == 0) {
        c->length = maxlen;
        if (c->len_enable && (apu.fs & 1)) c->length--;   
    }

    if (i == 0 || i == 1) {
        c->timer = 2048 - c->freq;
    }
    if (i == 2) {
        c->timer = (c->freq ^ 0x7FF) + 3;       
        apu.wave_pos = 0;
    }
    if (i == 3) {
        c->timer = noise_period();
        apu.lfsr = 0;
    }
    if (i != 2) {                 
        uint8_t r = env_reg(i);
        c->vol = r >> 4;
        c->env_timer = (r & 7) ? (r & 7) : 8;
        c->env_active = true;
    }
    if (i == 0) {                       
        uint8_t nr10 = NR(0xFF10);
        uint8_t pace = (nr10 >> 4) & 7, shift = nr10 & 7;
        apu.sweep_shadow = c->freq;
        apu.sweep_timer = pace ? pace : 8;
        apu.sweep_enabled = pace || shift;
        apu.sweep_neg_used = false;
        if (shift && sweep_calc() > 2047) c->enabled = false;
    }
    if (!dac_on(i)) c->enabled = false;
}

static uint8_t chan_digital(int i) {
    Chan *c = &apu.ch[i];
    if (!c->enabled) return 0;
    switch (i) {
    case 0: case 1: return duty_table[NR(i == 0 ? 0xFF11 : 0xFF16) >> 6][c->duty_pos] ? c->vol : 0;
    case 2: {
        static const uint8_t sh[4] = {4, 0, 1, 2};
        return apu.wave_buf >> sh[(NR(0xFF1C) >> 5) & 3];
    }
    default: return (apu.lfsr & 1) ? c->vol : 0;
    }
}

static void generators_tick(void) {
    apu.wave_read_now = false;

    for (int i = 0; i < 2; i++) {                  
        Chan *c = &apu.ch[i];
        if (--c->timer <= 0) {
            c->timer += 2048 - c->freq;
            c->duty_pos = (c->duty_pos + 1) & 7;
        }
    }

    for (int k = 0; k < 2; k++) {                
        Chan *c = &apu.ch[2];
        if (--c->timer <= 0) {
            c->timer += 2048 - c->freq;
            if (c->enabled) {
                apu.wave_pos = (apu.wave_pos + 1) & 31;
                uint8_t b = apu.wave_ram[apu.wave_pos >> 1];
                apu.wave_buf = (apu.wave_pos & 1) ? (b & 0x0F) : (b >> 4);
                apu.wave_read_now = true;
            }
        }
    }

    {                                            
        Chan *c = &apu.ch[3];
        int shift = NR(0xFF22) >> 4;
        if (shift < 14 && --c->timer <= 0) {
            c->timer += noise_period();
            int bit = ~(apu.lfsr ^ (apu.lfsr >> 1)) & 1;
            apu.lfsr = (uint16_t)((apu.lfsr >> 1) | (bit << 14));
            if (NR(0xFF22) & 8) apu.lfsr = (uint16_t)((apu.lfsr & ~0x40) | (bit << 6));
        } else if (shift >= 14 && c->timer < 1) {
            c->timer = 1;
        }
    }
}

static void emit_sample(void) {
    float l = 0, r = 0;
    bool any_dac = dac_on(0) || dac_on(1) || dac_on(2) || dac_on(3);

    if (apu.power && any_dac && apu.acc_n) {
        float inl = (float)apu.acc_l / (float)apu.acc_n / 480.0f;   
        float inr = (float)apu.acc_r / (float)apu.acc_n / 480.0f;
        const float charge = 0.996f;                              
        l = inl - apu.cap_l; apu.cap_l = inl - l * charge;
        r = inr - apu.cap_r; apu.cap_r = inr - r * charge;
    }

    int32_t sl = (int32_t)(l * 0.5f * 32767.0f);
    int32_t sr = (int32_t)(r * 0.5f * 32767.0f);
    if (sl > 32767) sl = 32767; if (sl < -32768) sl = -32768;
    if (sr > 32767) sr = 32767; if (sr < -32768) sr = -32768;
    apu.out[apu.out_n * 2]     = (int16_t)sl;
    apu.out[apu.out_n * 2 + 1] = (int16_t)sr;
    if (++apu.out_n == OUT_BUF_FRAMES) apu_flush();

    apu.acc_l = apu.acc_r = 0;
    apu.acc_n = 0;
}

void apu_flush(void) {
    if (apu.out_n) {
        hal_audio_push(apu.out, (unsigned)apu.out_n);
        apu.out_n = 0;
    }
}

void apu_tick(void) {
    generators_tick();

    if (apu.power) {
        uint8_t nr50 = NR(0xFF24), nr51 = NR(0xFF25);
        int lv = ((nr50 >> 4) & 7) + 1, rv = (nr50 & 7) + 1;
        int sl = 0, sr = 0;
        for (int i = 0; i < 4; i++) {
            if (!dac_on(i)) continue;
            int a = 15 - 2 * chan_digital(i);          
            if (nr51 & (0x10 << i)) sl += a;
            if (nr51 & (0x01 << i)) sr += a;
        }
        apu.acc_l += sl * lv;
        apu.acc_r += sr * rv;
    }
    apu.acc_n++;

    apu.sample_acc += HAL_AUDIO_RATE;
    if (apu.sample_acc >= M_CYCLES_PER_SEC) {
        apu.sample_acc -= M_CYCLES_PER_SEC;
        emit_sample();
    }
}

// ---- Liga / desliga -------------------------------------------------------
static void power_off(void) {
    uint16_t keep[4] = { apu.ch[0].length, apu.ch[1].length, apu.ch[2].length, apu.ch[3].length };
    memset(apu.reg, 0, sizeof apu.reg);
    for (int i = 0; i < 4; i++) {
        memset(&apu.ch[i], 0, sizeof(Chan));
        apu.ch[i].length = keep[i];       // no DMG os contadores de comprimento sobrevivem
    }
    apu.sweep_enabled = false;
    apu.sweep_shadow = 0;
    apu.sweep_timer = 0;
    apu.lfsr = 0;
    apu.power = false;
}

static void power_on(void) {
    apu.power = true;
    apu.fs = 0;
    apu.wave_buf = 0;
    apu.wave_pos = 0;
    for (int i = 0; i < 2; i++) apu.ch[i].duty_pos = 0;
}

void apu_init(void) {
    memset(&apu, 0, sizeof apu);
    apu.power = true;
    NR(0xFF10) = 0x80; NR(0xFF11) = 0xBF; NR(0xFF12) = 0xF3; NR(0xFF14) = 0xBF;
    NR(0xFF16) = 0x3F; NR(0xFF19) = 0xBF; NR(0xFF1A) = 0x7F; NR(0xFF1C) = 0x9F;
    NR(0xFF1E) = 0xBF; NR(0xFF20) = 0xFF; NR(0xFF23) = 0xBF;
    NR(0xFF24) = 0x77; NR(0xFF25) = 0xF3;
    apu.ch[0].enabled = true;          
    apu.ch[0].vol = 0;
    apu.ch[0].freq = 0x7C1 & 0x7FF;
    apu.ch[0].length = 0;
}

uint8_t apu_read(uint16_t a) {
    if (a >= 0xFF30 && a <= 0xFF3F) {
        if (apu.ch[2].enabled) return apu.wave_read_now ? apu.wave_ram[apu.wave_pos >> 1] : 0xFF;
        return apu.wave_ram[a - 0xFF30];
    }
    if (a == 0xFF26) {
        uint8_t v = 0x70 | (apu.power ? 0x80 : 0);
        for (int i = 0; i < 4; i++) if (apu.ch[i].enabled) v |= 1 << i;
        return v;
    }
    if (a < 0xFF10 || a > 0xFF25) return 0xFF;
    return NR(a) | read_mask[a - 0xFF10];
}

void apu_write(uint16_t a, uint8_t v) {
    if (a >= 0xFF30 && a <= 0xFF3F) {
        if (apu.ch[2].enabled) {
            if (apu.wave_read_now) apu.wave_ram[apu.wave_pos >> 1] = v;
        } else {
            apu.wave_ram[a - 0xFF30] = v;
        }
        return;
    }
    if (a == 0xFF26) {
        bool on = v & 0x80;
        if (on && !apu.power) power_on();
        else if (!on && apu.power) power_off();
        return;
    }
    if (a < 0xFF10 || a > 0xFF25) return;

    if (!apu.power) {
        if (a == 0xFF11 || a == 0xFF16) apu.ch[a == 0xFF11 ? 0 : 1].length = 64 - (v & 0x3F);
        else if (a == 0xFF1B)           apu.ch[2].length = 256 - v;
        else if (a == 0xFF20)           apu.ch[3].length = 64 - (v & 0x3F);
        return;
    }

    switch (a) {
    case 0xFF10:
        if (apu.sweep_neg_used && !(v & 8)) apu.ch[0].enabled = false;   // trocou de - para + depois de usar -
        break;
    case 0xFF11: apu.ch[0].length = 64 - (v & 0x3F); break;
    case 0xFF16: apu.ch[1].length = 64 - (v & 0x3F); break;
    case 0xFF1B: apu.ch[2].length = 256 - v; break;
    case 0xFF20: apu.ch[3].length = 64 - (v & 0x3F); break;
    default: break;
    }
    NR(a) = v;

    switch (a) {
    case 0xFF12: case 0xFF17: case 0xFF21:
        if (!(v & 0xF8)) apu.ch[a == 0xFF12 ? 0 : a == 0xFF17 ? 1 : 3].enabled = false;   // DAC desligou
        break;
    case 0xFF1A:
        if (!(v & 0x80)) apu.ch[2].enabled = false;
        break;
    case 0xFF13: apu.ch[0].freq = (apu.ch[0].freq & 0x700) | v; break;
    case 0xFF18: apu.ch[1].freq = (apu.ch[1].freq & 0x700) | v; break;
    case 0xFF1D: apu.ch[2].freq = (apu.ch[2].freq & 0x700) | v; break;
    case 0xFF14: case 0xFF19: case 0xFF1E: case 0xFF23: {
        int i = a == 0xFF14 ? 0 : a == 0xFF19 ? 1 : a == 0xFF1E ? 2 : 3;
        Chan *c = &apu.ch[i];
        if (i != 3) c->freq = (c->freq & 0xFF) | ((v & 7) << 8);
        bool was = c->len_enable;
        c->len_enable = v & 0x40;
        // Clock extra: ligar o comprimento quando o PRÓXIMO passo não o conta
        if (!was && c->len_enable && c->length > 0 && (apu.fs & 1)) {
            if (--c->length == 0 && !(v & 0x80)) c->enabled = false;
        }
        if (v & 0x80) trigger(i);
        break;
    }
    default: break;
    }
}
