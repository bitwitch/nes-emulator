typedef struct {
    uint8_t duty            : 2;
    uint8_t loop            : 1;
    uint8_t constant_volume : 1;
    uint8_t envelope        : 4;
} channel_register_t;

typedef struct {
    uint8_t enabled     : 1;
    uint8_t divider     : 3;
    uint8_t negate      : 1;
    uint8_t shift_count : 3;
} sweep_register_t;

typedef struct {
    channel_register_t reg;
    sweep_register_t sweep;
    uint8_t length_counter;
    bool envelope_start; 
    uint8_t envelope_counter; 
    uint8_t envelope_current_volume; 
    bool sweep_reload; 
    uint8_t sweep_counter; 
    uint8_t duty_counter; 
    uint16_t freq_timer;
    uint16_t freq_counter;
    bool enabled;
} pulse_channel_t;

typedef struct {
    bool linear_control;
    bool enabled;
    bool linear_reload_flag;
    bool ultrasonic;
    uint8_t linear_load;
    uint16_t freq_timer;
    uint16_t freq_counter;
    uint8_t length_counter;
    uint8_t linear_counter;
    uint8_t step;
} tri_channel_t;

typedef struct {
    channel_register_t reg;
    uint8_t length_counter;
    uint8_t envelope_counter; 
    uint8_t envelope_current_volume; 
    uint16_t freq_timer;
    uint16_t freq_counter;
    uint16_t shift_reg;
    bool enabled;
    bool shift_mode;
    bool envelope_start; 
} noise_channel_t;

typedef struct {
    bool enabled;
    bool irq_enabled;
    bool irq_pending;
    bool loop;
    bool output_silent;
    bool sample_buffer_empty;
    uint8_t sample_buffer;
    uint8_t output;
    uint8_t output_shift;
    uint8_t output_bits;
    uint16_t sample_addr;
    uint16_t sample_length;
    uint16_t sample_addr_load;
    uint16_t sample_length_load;
    uint16_t freq_timer;
    uint16_t freq_counter;
    
} dmc_channel_t;  // delta modulation channel


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
    tri_channel_t triangle;
    noise_channel_t noise;
    dmc_channel_t dmc;
    frame_sequencer_t frame_sequencer;
} apu_t;



// audio device buffer
// NOTE(shaw): this is a large circular buffer that the apu will write to and
// the sound card will read from
static float *ad_buffer;
static uint32_t ad_buffer_size;
static uint32_t ad_buffer_read;
static uint32_t ad_buffer_write;
static uint32_t ad_buffer_in;

// apu sound buffer
#define WAVE_BUFFER_SIZE 2048
static float wave[WAVE_BUFFER_SIZE];
static int wave_index = 0;


static bool should_tick_quarter_frame(void);
static bool should_tick_half_frame(void);
static void tick_quarter_frame(void);
static void tick_half_frame(void);
static void tick_pulse_channel(pulse_channel_t *pulse);
static void tick_pulse_envelope(pulse_channel_t *pulse);
static void tick_pulse_sweep(pulse_channel_t *pulse, bool is_pulse_2);
static void tick_triangle_channel(void);
static void tick_noise_channel(void);
static void tick_noise_envelope(noise_channel_t *noise);
static void tick_dmc_channel(void);
static void tick_frame_sequencer(void);
static bool is_sweep_forcing_silence(pulse_channel_t *pulse);
static uint8_t pulse_output(pulse_channel_t *pulse);
static float triangle_output(void);
static uint8_t noise_output(void);

static apu_t apu;
/*static uint64_t samples_played;*/


/*
 * 01000000 (12.5%)
 * 01100000 (25%)
 * 01111000 (50%)
 * 10011111 (25% negated)
*/
/*static const uint8_t duty_table[4] = { 0x40, 0x60, 0x71, 0x9F };*/


/*
 * 00000010 (12.5%)
 * 00000110 (25%)
 * 00011110 (50%)
 * 11111001 (25% negated)
 *
 * NOTE(shaw): this is flipped from the nesdev wiki, because they effectively
 * read left to right but I am using the duty counter as a bit shift and
 * effectively ready right to left
*/
static const uint8_t duty_table[4] = { 0x02, 0x06, 0x1D, 0xF9 };


/*double pulse_lookup_table[31] = { 0, 0.011609139523578026, 0.022939481268011527, 0.034000949216896059, 0.044803001876172609, 0.055354659248956883, 0.065664527956003665, 0.075740824648844587, 0.085591397849462361, 0.095223748338502431, 0.10464504820333041, 0.11386215864759427, 0.12288164665523155, 0.13170980059397538, 0.14035264483627205, 0.14881595346904861, 0.15710526315789472, 0.16522588522588522, 0.17318291700241739, 0.18098125249301955, 0.18862559241706162, 0.19612045365662886, 0.20347017815646784, 0.21067894131185272, 0.21775075987841944, 0.22468949943545349, 0.23149888143176731, 0.23818248984115256, 0.24474377745241579, 0.2511860718171926, 0.25751258087706685 };*/

/*double tnd_lookup_table[203] = {0};*/

static uint8_t length_table[32] = {10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14, 12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30 };

static uint16_t noise_freq_table[16] = {4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068};

static uint16_t dmc_freq_table[16] = {428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106,  84,  72,  54};


/*
 * The callback must completely initialize the buffer; as of SDL 2.0, this
 * buffer is not initialized before the callback is called. If there is nothing
 * to play, the callback should fill the buffer with silence.
 *
 * testing shows this gets called around every 12ms
 */

/*void apu_sound_output(void* userdata, uint8_t *byte_buffer, int buffer_size) {*/
    /*apu_t *apu = (apu_t *)userdata;*/

    /*float *buffer = (float *)byte_buffer;*/
    /*int sample_count = buffer_size / sizeof(buffer[0]);*/

    /*static const float volume = 0.2;*/
    /*static const float frequency = 82.0;*/

    /*for (int i=0; i<sample_count; ++i, ++samples_played) {*/
        /*double time = samples_played / (double)apu.spec_desired.freq;*/
        /*double x = time * frequency * 2.0f * M_PI;*/
        /*float val = sin(x);*/
        /*buffer[i] = (float)(val * volume);*/
        /*printf("x=%f val=%f buffer[%d]=%d\n", x, val, i, buffer[i]);*/
    /*}*/
/*}*/

/*
 * The callback must completely initialize the buffer; as of SDL 2.0, this
 * buffer is not initialized before the callback is called. If there is nothing
 * to play, the callback should fill the buffer with silence.
 *
 * testing shows this gets called around every 12ms
 */
void apu_sound_output(void *userdata, uint8_t *byte_buffer, int buffer_size) {
    (void)userdata;
    static float sample = 0;
    float *sound_buf = (float*)byte_buffer;

    buffer_size >>= 2;

    while (buffer_size) {
        if (ad_buffer_in) {
            sample = ad_buffer[ad_buffer_read];
            ad_buffer_read = (ad_buffer_read + 1) % ad_buffer_size;
            ad_buffer_in--;
        } else {
            // Retain last known sample value, helps avoid clicking
            // noise when sound system is starved of audio data.
            /*sample = 0;*/
            //bufStarveDetected = 1;
            /*printf("sound buffer starved\n");*/
        }

        *sound_buf = sample;
        sound_buf++;
        buffer_size--;
    }
}

void apu_init(void) {
    apu.spec_desired.freq = 44100;
    apu.spec_desired.format = AUDIO_F32;
    apu.spec_desired.channels = 1;
    apu.spec_desired.samples = 512;
    apu.spec_desired.callback = apu_sound_output;
    apu.spec_desired.userdata = &apu;

    int bufsize_in_ms = 128; // I copied this default value from fceux
    ad_buffer_size = apu.spec_desired.freq * bufsize_in_ms / 1000;

    // enforce minimium size
	uint32_t min_size = apu.spec_desired.samples * 2;
    if (ad_buffer_size < min_size) ad_buffer_size = min_size;

    ad_buffer = malloc(ad_buffer_size * sizeof(float));
    if (!ad_buffer) {
        fprintf(stderr, "[AUDIO] Failed to allocate %zu bytes for the audio device buffer\n", 
                ad_buffer_size * sizeof(float));
    }

    ad_buffer_read = ad_buffer_write = ad_buffer_in = 0;

    apu.audio_device = SDL_OpenAudioDevice(NULL, 0, 
        &apu.spec_desired, &apu.spec_obtained, 
        0);

    if (!apu.audio_device)
        fprintf(stderr, "[AUDIO] Failed to open audio device: %s\n", SDL_GetError());
    else if(apu.spec_desired.format != apu.spec_obtained.format)
        fprintf(stderr, "[AUDIO] Failed to obtain the desired AudioSpec data format\n");
    else {
        const char *driverName = SDL_GetCurrentAudioDriver();
        if (driverName) {
            fprintf(stderr, "Loading SDL sound with %s driver...\n", driverName);
        }
        SDL_PauseAudioDevice(apu.audio_device, 0);
    }

    // noise shift register must never be zero or the noise channel will never produce any output
    apu.noise.shift_reg = 1;

}

uint8_t apu_read(uint16_t addr) {
	uint8_t data = 0;

	// NOTE(shaw): allowing reading addrs besides 0x4015 here for the memory viewer
	switch(addr) {

	// triangle $4008-$400B
	case 0x4008:
		data = (apu.triangle.linear_control << 7) | apu.triangle.linear_load;
		break;
	case 0x400A:
		data = apu.triangle.freq_timer & 0xFF;
		break;
	case 0x400B:
		data = (apu.triangle.length_counter << 3) | ((apu.triangle.freq_timer >> 8) & 0x7);
		break;

	// status register
    case 0x4015: { 
        frame_sequencer_t *seq = &apu.frame_sequencer;

        if (apu.pulse1.length_counter > 0)
            data |= 0x01;
        if (apu.pulse2.length_counter > 0)
            data |= 0x02;
        if (apu.triangle.length_counter > 0)
            data |= 0x04;
        if (apu.noise.length_counter > 0)
            data |= 0x08;
        if (apu.dmc.sample_length > 0)
            data |= 0x10;
        if (apu.dmc.irq_pending > 0)
            data |= 0x80;

		// NOTE(shaw): If an interrupt flag was set at the same moment of the
		// read, it will read back as 1 but it will not be cleared.
		// https://www.nesdev.org/wiki/APU#Frame_Counter_($4017)
		// 
		// I am not sure what this means, what do they mean "at the same
		// moment" and why do they just say "an" interrupt flag, suggesting not
		// just the sequencer irq flag but any interrupt?
        if (seq->irq_pending) {
            data |= 0x40;
		} 

        seq->irq_pending = false;

		break;
    }
	default: break;
	}
	return data;
}

void apu_write(uint16_t addr, uint8_t data) {
    switch(addr) {
        /* PULSE 1 */
        case 0x4000:
        /* pulse1 volume */
        {
            channel_register_t *reg = &apu.pulse1.reg;
            reg->duty            = (data >> 6) & 0x3;
            reg->loop            = (data >> 5) & 0x1;
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
            apu.pulse1.freq_timer = (apu.pulse1.freq_timer & 0x00FF) | ((data & 0x7) << 8);
            if (apu.pulse1.enabled)
                apu.pulse1.length_counter = length_table[(data >> 3) & 0x1F];
            /* reset phase */
            apu.pulse1.freq_counter = apu.pulse1.freq_timer;
            apu.pulse1.duty_counter = 0;

            apu.pulse1.envelope_start = true;
            break;
        }

        /* PULSE 2 */
        case 0x4004:
        /* pulse2 volume */
        {
            channel_register_t *reg = &apu.pulse2.reg;
            reg->duty            = (data >> 6) & 0x3;
            reg->loop            = (data >> 5) & 0x1;
            reg->constant_volume = (data >> 4) & 0x1;
            reg->envelope        = (data >> 0) & 0xF;
            break;
        }
        case 0x4005:
        /* pulse2 sweep */
        {
            sweep_register_t *sweep = &apu.pulse2.sweep;
            sweep->enabled     = (data >> 7) & 0x1;
            sweep->divider     = (data >> 4) & 0x7;
            sweep->negate      = (data >> 3) & 0x1;
            sweep->shift_count = (data >> 0) & 0x7;
            break;
        }
        case 0x4006:
        /* pulse2 freq_timer low */
        {
            apu.pulse2.freq_timer = (apu.pulse2.freq_timer & 0xFF00) | data;
            break;
        }
        case 0x4007:
        /* pulse2 freq_timer high and length counter load*/
        {
            apu.pulse2.freq_timer = (apu.pulse2.freq_timer & 0x00FF) | ((data & 0x7) << 8);
            if (apu.pulse2.enabled)
                apu.pulse2.length_counter = length_table[(data >> 3) & 0x1F];
            /* reset phase */
            apu.pulse2.freq_counter = apu.pulse2.freq_timer;
            apu.pulse2.duty_counter = 0;

            apu.pulse2.envelope_start = true;
            break;
        }

        /* TRIANGLE */
        case 0x4008:
        {
            apu.triangle.linear_control = (data >> 7) & 1;
            apu.triangle.linear_load = data & 0x7F;
            break;
        }

        case 0x4009: break;

        case 0x400A:
        /* triangle freq_timer low */
        {
            apu.triangle.freq_timer = (apu.triangle.freq_timer & 0xFF00) | data;
            break;
        }
        case 0x400B:
        /* triangle freq_timer high and length counter load*/
        {
            apu.triangle.freq_timer = (apu.triangle.freq_timer & 0x00FF) | (((uint16_t)data & 0x7) << 8);
            if (apu.triangle.enabled)
                apu.triangle.length_counter = length_table[(data >> 3) & 0x1F];
            apu.triangle.linear_reload_flag = true;
            break;
        }
 
        /* NOISE */
        case 0x400C:
        {
            channel_register_t *reg = &apu.noise.reg;
            reg->loop            = (data >> 5) & 0x1;
            reg->constant_volume = (data >> 4) & 0x1;
            reg->envelope        = (data >> 0) & 0xF;
            break;
        }
        case 0x400D: break;
        case 0x400E:
        {

            apu.noise.freq_timer = noise_freq_table[data & 0xF];
            apu.noise.shift_mode = (data >> 7) & 1;
            break;
        }
        case 0x400F:
        {
            if (apu.noise.enabled)
                apu.noise.length_counter = length_table[(data >> 3) & 0x1F];
            apu.noise.envelope_start = true;
            break;
        }

        /* DMC */
        case 0x4010:
        {
            apu.dmc.irq_enabled = (data >> 7) & 1;
            apu.dmc.loop = (data >> 6) & 1;
            apu.dmc.freq_timer = dmc_freq_table[data & 0xF];
            if (!apu.dmc.irq_enabled)
                apu.dmc.irq_pending = false;
            break;
        }
        case 0x4011:
        {
            apu.dmc.output = data & 0x7F;
            break;
        }
        case 0x4012:
        {
            apu.dmc.sample_addr_load = 0xC000 + (data << 6);
            break;
        }
        case 0x4013:
        {
            apu.dmc.sample_length_load = (data << 4) + 1;
            break;
        }

        /* status register */
        case 0x4015:
        {
            bool p1  = (data >> 0) & 1;
            bool p2  = (data >> 1) & 1;
            bool t   = (data >> 2) & 1;
            bool n   = (data >> 3) & 1;
            bool dmc = (data >> 4) & 1;

            apu.pulse1.enabled   = p1;
            apu.pulse2.enabled   = p2;
            apu.triangle.enabled = t;
            apu.noise.enabled = n;

            if (!p1) apu.pulse1.length_counter   = 0;
            if (!p2) apu.pulse2.length_counter   = 0;
            if (!t)  apu.triangle.length_counter = 0;
            if (!n)  apu.noise.length_counter    = 0;

            if (dmc) {
                if (apu.dmc.sample_length == 0) {
                    apu.dmc.sample_length = apu.dmc.sample_length_load;
                    apu.dmc.sample_addr = apu.dmc.sample_addr_load;
                }
            } 
            else
                apu.dmc.sample_length = 0;

            apu.dmc.irq_pending = false; // acknowledge DMC IRQ on write
            break;
        }

        case 0x4017:
        /* frame sequencer */
        {
            frame_sequencer_t *seq = &apu.frame_sequencer;
            seq->mode        = (data >> 7) & 1;
            seq->irq_inhibit = (data >> 6) & 1; 

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
    tick_pulse_envelope(&apu.pulse1);
    tick_pulse_envelope(&apu.pulse2);

    tick_noise_envelope(&apu.noise);

    /* tick triangle linear counter */
    tri_channel_t *tri = &apu.triangle;
    if (tri->linear_reload_flag)
        tri->linear_counter = tri->linear_load;
    else if (tri->linear_counter > 0)
        --tri->linear_counter;

    if (!tri->linear_control)
        tri->linear_reload_flag = false;
}

static void tick_half_frame(void) {
    tick_pulse_sweep(&apu.pulse1, true);
    tick_pulse_sweep(&apu.pulse2, false);

    if (!apu.pulse1.reg.loop && apu.pulse1.length_counter > 0)
        --apu.pulse1.length_counter;
    if (!apu.pulse2.reg.loop && apu.pulse2.length_counter > 0)
        --apu.pulse2.length_counter;
    if (!apu.triangle.linear_control && apu.triangle.length_counter > 0)
        --apu.triangle.length_counter;
    if (!apu.noise.reg.loop && apu.noise.length_counter > 0)
        --apu.noise.length_counter;
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

static void tick_pulse_channel(pulse_channel_t *pulse) {
    if (pulse->freq_counter > 0) 
        --pulse->freq_counter;
    else {
        pulse->freq_counter = pulse->freq_timer;
        pulse->duty_counter = (pulse->duty_counter + 1) & 7;
    }
}

static void tick_pulse_envelope(pulse_channel_t *pulse) {
    if (pulse->envelope_start) {
        pulse->envelope_start = false;
        pulse->envelope_current_volume = 0xF;
        pulse->envelope_counter = pulse->reg.envelope;
    } else {
        if (pulse->envelope_counter > 0)
            --pulse->envelope_counter;
        else {
            pulse->envelope_counter = pulse->reg.envelope;
            if (pulse->envelope_current_volume > 0)
                --pulse->envelope_current_volume;
            else if (pulse->reg.loop)
                pulse->envelope_current_volume = 0xF;
        }
    }
}

static void tick_noise_envelope(noise_channel_t *noise) {
    if (noise->envelope_start) {
        noise->envelope_start = false;
        noise->envelope_current_volume = 0xF;
        noise->envelope_counter = noise->reg.envelope;
    } else {
        if (noise->envelope_counter > 0)
            --noise->envelope_counter;
        else {
            noise->envelope_counter = noise->reg.envelope;
            if (noise->envelope_current_volume > 0)
                --noise->envelope_current_volume;
            else if (noise->reg.loop)
                noise->envelope_current_volume = 0xF;
        }
    }
}


static void tick_pulse_sweep(pulse_channel_t *pulse, bool is_pulse_1) {
    if (pulse->sweep_reload) {
        pulse->sweep_counter = pulse->sweep.divider;
        pulse->sweep_reload = false;
        // note there's an edge case here -- see http://wiki.nesdev.com/w/index.php/APU_Sweep
        //   for details.  You can probably ignore it for now
    } else if (pulse->sweep_counter > 0) {
        --pulse->sweep_counter;
    } else {
        pulse->sweep_counter = pulse->sweep.divider;
        if (pulse->sweep.enabled && !is_sweep_forcing_silence(pulse)) {
            if(pulse->sweep.negate) {
                pulse->freq_timer -= pulse->freq_timer >> pulse->sweep.shift_count;
                if (is_pulse_1)
                    pulse->freq_timer -= 1;
            }
            else
                pulse->freq_timer += (pulse->freq_timer >> pulse->sweep.shift_count);
        }
    }
}



static bool is_sweep_forcing_silence(pulse_channel_t *pulse) {
    sweep_register_t *sweep = &apu.pulse1.sweep;
    if (pulse->freq_timer < 8)
        return true;
    else if (!sweep->negate && (pulse->freq_timer + (pulse->freq_timer >> sweep->shift_count)) > 0x7FF)
        return true;
    else
        return false;
}

static uint8_t pulse_output(pulse_channel_t *pulse) {
    uint8_t output = 0;
    bool duty_high = (duty_table[pulse->reg.duty] >> pulse->duty_counter) & 1;

    if (duty_high && 
        pulse->length_counter > 0 && 
        !is_sweep_forcing_silence(pulse))
    {
        if(pulse->reg.constant_volume) 
            output = pulse->reg.envelope;
        else
            output = pulse->envelope_current_volume;
    }

    return output;
}


static void tick_triangle_channel(void) {
    tri_channel_t *tri = &apu.triangle;

    tri->ultrasonic = false;
    if (tri->freq_timer < 2 && tri->freq_counter == 0)
        tri->ultrasonic = true;
    
    if (tri->length_counter != 0 &&
        tri->linear_counter != 0 &&
        !tri->ultrasonic)
    {
        if (tri->freq_counter > 0)
            --tri->freq_counter;
        else {
            tri->freq_counter = tri->freq_timer;
            tri->step = (tri->step + 1) & 0x1F;    // tri->step bound to 00..1F range
        }
    }
}

static float triangle_output(void) {
    tri_channel_t *tri = &apu.triangle;

	// Due to the averaging effect of the lowpass filter, the resulting value
	// is halfway between 7 and 8
	if (tri->ultrasonic)       return 7.5; 

	// output either 15 - 0 or 0 - 15
    else if (tri->step & 0x10) return (float)(tri->step ^ 0x1F);
    else                       return tri->step;
}

static void tick_noise_channel(void) {
    noise_channel_t *n = &apu.noise;
    if (n->freq_counter > 0)
        --n->freq_counter;
    else {
        n->freq_counter = n->freq_timer;
        uint8_t xor_bit = n->shift_mode ? 6 : 1;
        uint8_t feedback_bit = ((n->shift_reg >> xor_bit) & 1) ^ (n->shift_reg & 1);

        // set bit 15 to feedback bit
        n->shift_reg = (n->shift_reg & ~0x8000) | (feedback_bit << 15);

        n->shift_reg >>= 1;
    }
}

static uint8_t noise_output(void) {
    uint8_t output = 0;
    noise_channel_t *noise = &apu.noise;
    if ((noise->shift_reg & 1) == 0 && noise->length_counter != 0) {
        if(noise->reg.constant_volume) 
            output = noise->reg.envelope;
        else
            output = noise->envelope_current_volume;
    }
    return output;
}

static void tick_dmc_channel(void) {
    dmc_channel_t *dmc = &apu.dmc;
    if (dmc->freq_counter > 0)
        --dmc->freq_counter;
    else {
        dmc->freq_counter = dmc->freq_timer;

        if (!dmc->output_silent) {
            if ((dmc->output_shift & 1) && dmc->output < 0x7E)
                dmc->output += 2;
            if (!(dmc->output_shift & 1) && dmc->output > 0x01)
                dmc->output -= 2;
        }

        --dmc->output_bits;
        dmc->output_shift >>= 1;

        if (dmc->output_bits == 0) {
            dmc->output_bits = 8;
            dmc->output_shift = dmc->sample_buffer;
            dmc->output_silent = dmc->sample_buffer_empty;
            dmc->sample_buffer_empty = true;
        }
    }



    // perform DMA if necessary
    if (dmc->sample_length > 0 && dmc->sample_buffer_empty) {

        // NOTE(shaw): this causes some cpu cycle delays, but I am ignoring them here
        // see: https://www.nesdev.org/wiki/APU_DMC
        dmc->sample_buffer = bus_read(dmc->sample_addr);

        dmc->sample_buffer_empty = false;
        dmc->sample_addr = (dmc->sample_addr + 1) | 0x8000;     // wrap $FFFF to $8000
        --dmc->sample_length;

        if (dmc->sample_length == 0) {
            if (dmc->loop) {
                dmc->sample_length = dmc->sample_length_load;
                dmc->sample_addr = dmc->sample_addr_load;
            }
            else if (dmc->irq_enabled)
                dmc->irq_pending = true;
        }
    }

}

static void write_sound(float *buffer, int count) {
    int wait_count = 0;
    while (count) {
        while (ad_buffer_in == ad_buffer_size) {
            SDL_Delay(1); wait_count++;

            if ( wait_count > 1000 ) {
                fprintf(stderr, "[AUDIO] Error: Sound sink is not draining... Breaking out of audio loop to prevent lockup.\n");
                return;
            }
        }

        ad_buffer[ad_buffer_write] = *buffer;
        count--;
        ad_buffer_write = (ad_buffer_write + 1) % ad_buffer_size;

        SDL_LockAudio();
        ad_buffer_in++;
        SDL_UnlockAudio();

        buffer++;
    }
}

bool apu_request_frame(void) {
    return ad_buffer_in <= apu.spec_obtained.samples;
}


void apu_flush_sound_buffer(void) {
    write_sound(wave, wave_index);
    wave_index = 0;
}

bool apu_irq_pending(void) {
	return apu.frame_sequencer.irq_pending;
}

void apu_irq_clear(void) {
	apu.frame_sequencer.irq_pending = false;
}

void apu_tick(void) {
    static int downsample_counter = -1;
    static bool downsample_even = 0;
    static bool even_cycle = true;

    uint8_t pulse1   = 0;
    uint8_t pulse2   = 0;
    float triangle   = 0;
    uint8_t noise    = 0;
    uint8_t dmc      = 0;

    if (even_cycle) {
        tick_pulse_channel(&apu.pulse1);
        tick_pulse_channel(&apu.pulse2);

        tick_noise_channel();

        tick_dmc_channel();
    }

    tick_triangle_channel();

    tick_frame_sequencer();

    pulse1   = pulse_output(&apu.pulse1);
    pulse2   = pulse_output(&apu.pulse2);
    triangle = triangle_output();
    noise    = noise_output();
    dmc      = apu.dmc.output;

    float pulse_out = 0.00752f * (pulse1 + pulse2);
    float tnd_out = 0.00851f * triangle + 0.00494f * noise + 0.00335f * dmc;
    float output = pulse_out + tnd_out;

    /*float output = pulse_lookup_table[pulse1 + pulse2];*/
    /*float output = pulse_lookup_table[pulse1 + pulse2] + tnd_lookup_table[3*triangle + 2*noise + dmc];*/

    // HACK: this is simulating downsampling the tick rate of the apu to approximate a 44100Hz sample rate 
    ++downsample_counter;
    int s = downsample_even ? 39 : 40;
    if (downsample_counter > s) {
        assert(wave_index < WAVE_BUFFER_SIZE);
        wave[wave_index++] = output;
        downsample_counter = 0;
        downsample_even = !downsample_even;
    }

    even_cycle = !even_cycle;
}

