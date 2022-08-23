#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL_audio.h>

#define PI 3.1415926535


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
    uint8_t envelope_counter; 
    uint16_t timer;


    /* TODO(shaw): maybe it would be easier to store sweep variables for each
     * data point rather than packing them in a "register" byte here */
    uint8_t sweep_register;
} pulse_channel_t;

typedef struct {
    SDL_AudioSpec spec_desired, spec_obtained;
    SDL_AudioDeviceID audio_device;
    pulse_channel_t pulse1, pulse2;
} apu_t;

static apu_t apu;
static uint64_t samples_played;

/*
 * The callback must completely initialize the buffer; as of SDL 2.0, this
 * buffer is not initialized before the callback is called. If there is nothing
 * to play, the callback should fill the buffer with silence.
 */
void apu_sound_output(void* userdata, uint8_t *byte_buffer, int buffer_size) {
    /*apu_t *apu = (apu_t *)userdata;*/
    (void)userdata;

    float *buffer = (float *)byte_buffer;
    int sample_count = buffer_size / sizeof(buffer[0]);

    static const float volume = 0.2;
    static const float frequency = 82.0;


    /*
    69

    512

    (69+512)/44100 = 0.0137 
    69+(512/44100) = 69.0226

    sample_num
    -----------------------------------------------   => seconds
    samples_per_sec



    [____________________________________] 0.0116 secs

           512 samples
          44100 samples/sec
  */




    for (int i=0; i<sample_count; ++i, ++samples_played) {
        double time = i + (samples_played / (double)apu.spec_desired.freq);
        double x = time * frequency * 2.0f * PI;
        float val = sin(x);
        buffer[i] = (float)(val * volume);
        /*printf("x=%f val=%f buffer[%d]=%d\n", x, val, i, buffer[i]);*/
    }
}

void apu_init(void) {
    apu.spec_desired.freq = 44100;
    apu.spec_desired.format = AUDIO_F32;
    apu.spec_desired.channels = 1;
    apu.spec_desired.samples = 512;
    apu.spec_desired.callback = apu_sound_output;
    apu.spec_desired.userdata = &apu;
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
            sweep_register_t *reg = &apu.pulse1.sweep;
            sweep->enabled     = (data >> 7) & 0x1;
            sweep->divider     = (data >> 4) & 0x7;
            sweep->negate      = (data >> 3) & 0x1;
            sweep->shift_count = (data >> 0) & 0x7;
            break;
        }
        case 0x4002:
        /* pulse1 timer low */
        {
            apu.pulse1.timer = (apu.pulse1.timer & 0xFF00) | data;
            break;
        }
        case 0x4003:
        /* pulse1 timer high and length counter load*/
        {
            apu.pulse1.timer = (apu.pulse1.timer & 0x07FF) | ((data & 0x7) << 8);
            if (pulse1 is enabled)
                apu.pulse1.length_counter = length_table[(data >> 3) & 0x1F];
            break;
        }
    }
}



