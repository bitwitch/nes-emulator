#include "apu.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <SDL2/SDL_audio.h>
#include "io.h"


typedef struct {
    uint8_t duty            : 2;
    uint8_t halt            : 1;
    uint8_t constant_volume : 1;
    uint8_t envelope        : 4;
} pulse_register_t;

typedef struct {
    uint8_t enabled     : 1;
    uint8_t divider     : 3;
    uint8_t negate      : 1;
    uint8_t shift_count : 3;
} sweep_register_t;

typedef struct {
    pulse_register_t reg;
    sweep_register_t sweep;
    uint8_t length_counter;
    uint8_t freq_counter;
    uint8_t envelope_counter; 
    uint8_t duty_counter; 
    uint16_t freq_timer;
    bool enabled;
} pulse_channel_t;

typedef struct {
    /*divider;*/
    /*sequencer;*/

    bool mode;
    bool irq_pending;
    bool irq_inhibit;
    uint8_t step;
    uint16_t counter;


} frame_sequencer_t;

typedef struct {
    SDL_AudioSpec spec_desired, spec_obtained;
    SDL_AudioDeviceID audio_device;
    pulse_channel_t pulse1, pulse2;
    frame_sequencer_t frame_sequencer;
} apu_t;


static bool should_tick_quarter_frame(void);
static bool should_tick_half_frame(void);
static void tick_quarter_frame(void);
static void tick_half_frame(void);
static void tick_frame_sequencer(void);
static bool is_sweep_forcing_silence(void);
static uint8_t pulse_output(pulse_channel_t *pulse);

static apu_t apu;
static uint64_t samples_played;


/*
 * 01000000 (12.5%)
 * 01100000 (25%)
 * 01111000 (50%)
 * 10011111 (25% negated)
*/
static const uint8_t duty_table[8] = { 0x40, 0x60, 0x71, 0x9F };

double pulse_lookup_table[31] = { 0, 0.011609139523578026, 0.022939481268011527, 0.034000949216896059, 0.044803001876172609, 0.055354659248956883, 0.065664527956003665, 0.075740824648844587, 0.085591397849462361, 0.095223748338502431, 0.10464504820333041, 0.11386215864759427, 0.12288164665523155, 0.13170980059397538, 0.14035264483627205, 0.14881595346904861, 0.15710526315789472, 0.16522588522588522, 0.17318291700241739, 0.18098125249301955, 0.18862559241706162, 0.19612045365662886, 0.20347017815646784, 0.21067894131185272, 0.21775075987841944, 0.22468949943545349, 0.23149888143176731, 0.23818248984115256, 0.24474377745241579, 0.2511860718171926, 0.25751258087706685 };

double tnd_lookup_table[203] = {0};

static void tick_pulse_channel(pulse_channel_t *pulse) {
    if (pulse->freq_counter > 0) 
        --pulse->freq_counter;
    else {
        pulse->freq_counter = pulse->freq_timer;
        pulse->duty_counter = (pulse->duty_counter + 1) & 7;
    }
}

/*
 * The callback must completely initialize the buffer; as of SDL 2.0, this
 * buffer is not initialized before the callback is called. If there is nothing
 * to play, the callback should fill the buffer with silence.
 *
 * testing shows this gets called around every 12ms
 */

/* NOTE(shaw): This is currently unused, as I switched to SDL_QueueAudio */

/*void apu_sound_output(void* userdata, uint8_t *byte_buffer, int buffer_size) {*/
    /*[>apu_t *apu = (apu_t *)userdata;<]*/
    /*(void)userdata;*/

    /*float *buffer = (float *)byte_buffer;*/
    /*int sample_count = buffer_size / sizeof(buffer[0]);*/

    /*static const float volume = 0.2;*/
    /*static const float frequency = 82.0;*/

    /*for (int i=0; i<sample_count; ++i, ++samples_played) {*/
        /*double time = samples_played / (double)apu.spec_desired.freq;*/
        /*double x = time * frequency * 2.0f * M_PI;*/
        /*float val = sin(x);*/
        /*buffer[i] = (float)(val * volume);*/
        /*[>printf("x=%f val=%f buffer[%d]=%d\n", x, val, i, buffer[i]);<]*/
    /*}*/
/*}*/

void apu_init(void) {
    apu.spec_desired.freq = 44100;
    apu.spec_desired.format = AUDIO_F32;
    apu.spec_desired.channels = 1;
    apu.spec_desired.samples = 512;
    /*apu.spec_desired.callback = apu_sound_output;*/
    /*apu.spec_desired.userdata = &apu;*/
    apu.audio_device = SDL_OpenAudioDevice(NULL, 0, 
        &apu.spec_desired, &apu.spec_obtained, 
        0);

    if (!apu.audio_device)
        fprintf(stderr, "[AUDIO] Failed to open audio device: %s\n", SDL_GetError());
    else if(apu.spec_desired.format != apu.spec_obtained.format)
        fprintf(stderr, "[AUDIO] Failed to obtain the desired AudioSpec data format\n");
    else 
        SDL_PauseAudioDevice(apu.audio_device, 0);
}

uint8_t apu_read(uint16_t addr) {
    if (addr == 0x4015) { /* status register */
        /* see https://www.nesdev.org/wiki/APU#Status_($4015) */
        assert(0 && "not implemented");
    }

    return 0;
}

void apu_write(uint16_t addr, uint8_t data) {
    switch(addr) {
        case 0x4000:
        /* pulse1 volume */
        {
            pulse_register_t *reg = &apu.pulse1.reg;
            reg->duty            = (data >> 6) & 0x3;
            reg->halt            = (data >> 5) & 0x1;
            reg->constant_volume = (data >> 4) & 0x1;
            reg->envelope        = (data >> 0) & 0xF;
            break;
        }
        case 0x4001:
        /* pulse1 sweep */
        {
            sweep_register_t *sweep = &apu.pulse1.sweep;
            sweep->enabled     = (data >> 7) & 0x1;
            sweep->divider     = (data >> 4) & 0x7;
            sweep->negate      = (data >> 3) & 0x1;
            sweep->shift_count = (data >> 0) & 0x7;
            break;
        }
        case 0x4002:
        /* pulse1 freq_timer low */
        {
            apu.pulse1.freq_timer = (apu.pulse1.freq_timer & 0xFF00) | data;
            break;
        }
        case 0x4003:
        /* pulse1 freq_timer high and length counter load*/
        {
            apu.pulse1.freq_timer = (apu.pulse1.freq_timer & 0x07FF) | ((data & 0x7) << 8);
            /*if (pulse1 is enabled)*/
                /*apu.pulse1.length_counter = length_table[(data >> 3) & 0x1F];*/
            /* reset phase */
            apu.pulse1.freq_counter = apu.pulse1.freq_timer;
            apu.pulse1.duty_counter = 0;

            break;
        }



        case 0x4015:
        /* status register */
        {
            /* TODO(shaw): other channel enable flags */
            bool p1 = (data >> 0) & 1;
            bool p2 = (data >> 1) & 1;

            apu.pulse1.enabled = p1;
            apu.pulse2.enabled = p2;

            if (!p1)
                apu.pulse1.length_counter = 0;
            if (!p2)
                apu.pulse2.length_counter = 0;
            break;
        }

        case 0x4017:
        /* frame sequencer */
        {
            frame_sequencer_t *seq = &apu.frame_sequencer;
            seq->mode        = (data >> 7) & 1;
            seq->irq_inhibit = (data >> 7) & 2; 

            seq->counter = 7457; /* cpu cycles to next sequence step */

            if (seq->mode) {
                tick_quarter_frame();
                tick_half_frame();
            }

            if (seq->irq_inhibit)
                seq->irq_pending = 0;

            break;
        }
    }
}

static bool should_tick_quarter_frame(void) {
    uint8_t step = apu.frame_sequencer.step;
    uint8_t mode = apu.frame_sequencer.mode;
    if (step < 3) return true;
    if (!mode && step == 3) return true;
    if  (mode && step == 4) return true;
    return false;
}

static bool should_tick_half_frame(void) {
    uint8_t step = apu.frame_sequencer.step;
    uint8_t mode = apu.frame_sequencer.mode;
    if (step == 1) return true;
    if (!mode && step == 3) return true;
    if  (mode && step == 4) return true;
    return false;
}

static void tick_quarter_frame(void) {
    /* TODO(shaw): tick envelope and triangle linear counter */
}

static void tick_half_frame(void) {
    /* TODO(shaw): tick sweep units */


    /* TODO(shaw): tick other channel length counters */

    if (apu.pulse1.enabled && apu.pulse1.length_counter > 0)
        --apu.pulse1.length_counter;

}

static void tick_frame_sequencer(void) {
    frame_sequencer_t *seq = &apu.frame_sequencer;
    if (seq->counter > 0) {
        --seq->counter;
    } else {
        if (should_tick_quarter_frame())
            tick_quarter_frame();
        if (should_tick_half_frame())
            tick_half_frame();
        if (!seq->irq_inhibit && !seq->mode && seq->step == 3)
            seq->irq_pending = true;

        ++seq->step;
        seq->step %= seq->mode ? 5 : 4;
            
        seq->counter = 7457; /* cpu cycles to next sequence step */
    }
}


static bool is_sweep_forcing_silence(void) {
    /* TODO(shaw): implement this */
    return 0;
}

static uint8_t pulse_output(pulse_channel_t *pulse) {
    uint8_t output = 0;

    bool duty_high = (duty_table[pulse->reg.duty] >> pulse->duty_counter) & 1;
    if (duty_high && 
        pulse->length_counter > 0 && 
        !is_sweep_forcing_silence())
    {
        if(pulse->reg.constant_volume) 
            output = pulse->reg.envelope;
        else
            /*
             * TODO(shaw): this should be the current envelope volume
             * output = decay_hidden_vol;
             * */
            output = pulse->reg.envelope;
    }

    return output;
}


 
#define AUDIO_BUFFER_LEN 706
static float audio_buffer[AUDIO_BUFFER_LEN] = {0}; // ~16ms of audio at 44100 samples / sec
static int sample_index = 0;
static bool audio_buffer_full = false;

void apu_tick(void) {
    static bool even_cycle = true;

    uint8_t pulse1   = 0;
    uint8_t pulse2   = 0;
    /*uint8_t triangle = 0;*/
    /*uint8_t noise    = 0;*/
    /*uint8_t dmc      = 0;*/

    if (even_cycle) {
        tick_pulse_channel(&apu.pulse1);
        /*tick_pulse_channel(&apu.pulse2);*/

        /* tick noise channel */

        /* tick DMC */
    }

    /* tick triangle channel */

    tick_frame_sequencer();

    pulse1 = pulse_output(&apu.pulse1);

    float output = pulse_lookup_table[pulse1 + pulse2];
    /*float output = pulse_lookup_table[pulse1 + pulse2] + tnd_lookup_table[3*triangle + 2*noise + dmc];*/

    /*static const float volume = 0.2;*/
    /*static const float frequency = 82.0;*/
    /*double time = samples_played++ / (double)apu.spec_desired.freq;*/
    /*double x = time * frequency * 2.0f * M_PI;*/
    /*float output = sin(x) * volume;*/
 
    audio_buffer[sample_index++] = output;

    if (sample_index >= AUDIO_BUFFER_LEN) {
        SDL_QueueAudio(apu.audio_device, audio_buffer, sizeof(audio_buffer[0]) * AUDIO_BUFFER_LEN);
        audio_buffer_full = true;
        sample_index = 0;
        if (platform_state.w)
            render_sound_wave(audio_buffer, AUDIO_BUFFER_LEN);
    }

    even_cycle = !even_cycle;
}


/*static float visual_audio_buffer[AUDIO_BUFFER_LEN*2];*/
/*static bool visual_audio_right = true;*/
/*void apu_render_sound_wave(void) {*/
    /*if (audio_buffer_full) {*/
        /*int offset = visual_audio_right ? AUDIO_BUFFER_LEN : 0;*/
        /*visual_audio_right = !visual_audio_right;*/
        /*memcpy(visual_audio_buffer + offset, audio_buffer, AUDIO_BUFFER_LEN * sizeof(audio_buffer[0]));*/
        /*audio_buffer_full = false;*/
    /*}*/

    /*render_sound_wave(visual_audio_buffer, AUDIO_BUFFER_LEN*2);*/
/*}*/





