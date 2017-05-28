#include "audio.h"

#include "utox_av.h"

#include "../native/audio.h"
#include "../native/keyboard.h"
#include "../native/thread.h"
#include "../native/time.h"

#include "../debug.h"
#include "../friend.h"
#include "../groups.h"
#include "../macros.h"
#include "../main.h" // utox_audio_thread_init, self, USER_STATUS_*, UTOX_MAX_CALLS
#include "../self.h"
#include "../settings.h"
#include "../tox.h"
#include "../utox.h"

#include "../../langs/i18n_decls.h"

#include <math.h>
#include <string.h>
#include <tox/toxav.h>

#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>

#ifdef AUDIO_FILTERING
#include <AL/alext.h>
#endif
/* include for compatibility with older versions of OpenAL */
#ifndef ALC_ALL_DEVICES_SPECIFIER
#include <AL/alext.h>
#endif
#endif

#ifdef AUDIO_FILTERING
#include <filter_audio.h>
#endif

static void utox_filter_audio_kill(Filter_Audio *filter_audio_handle) {
#ifdef AUDIO_FILTERING
    kill_filter_audio(filter_audio_handle);
#else
    (void)filter_audio_handle;
#endif
}

static ALCdevice *audio_out_handle, *audio_in_handle;
static void *     audio_out_device, *audio_in_device;
static bool       speakers_on, microphone_on;
static int16_t    speakers_count, microphone_count;
/* TODO hacky fix. This source list should be a VLA with a way to link sources to friends.
 * NO SRSLY don't leave this like this! */
static ALuint ringtone, preview, notifytone;

static ALuint RingBuffer, ToneBuffer;

static bool audio_in_device_open(void) {
    if (!audio_in_device) {
        return false;
    }
    if (audio_in_device == (void *)1) {
        audio_in_handle = (void *)1;
        return true;
    }

    alGetError();
    audio_in_handle = alcCaptureOpenDevice(audio_in_device, UTOX_DEFAULT_SAMPLE_RATE_A, AL_FORMAT_MONO16,
                                           (UTOX_DEFAULT_FRAME_A * UTOX_DEFAULT_SAMPLE_RATE_A * 4) / 1000);
    if (alGetError() == AL_NO_ERROR) {
        return true;
    }
    return false;
}

static bool audio_in_device_close(void) {
    if (audio_in_handle) {
        if (audio_in_handle == (void *)1) {
            audio_in_handle = NULL;
            microphone_on = false;
            return false;
        }
        if (microphone_on) {
            alcCaptureStop(audio_in_handle);
        }
        alcCaptureCloseDevice(audio_in_handle);
    }
    audio_in_handle = NULL;
    microphone_on = false;
    return true;
}

static bool audio_in_listen(void) {
    if (microphone_on) {
        microphone_count++;
        return true;
    }

    if (audio_in_handle) {
        if (audio_in_device == (void *)1) {
            audio_init(audio_in_handle);
            return true;
        }
        alcCaptureStart(audio_in_handle);
    } else if (audio_in_device) {
        /* Unable to get handle, try to open it again. */
        audio_in_device_open();
        if (audio_in_handle) {
            alcCaptureStart(audio_in_handle);
        } else {
            LOG_TRACE("uTox Audio", "Unable to listen to device!" );
        }
    }

    if (audio_in_handle) {
        microphone_on    = true;
        microphone_count = 1;
        return true;
    }

    microphone_on    = false;
    microphone_count = 0;
    return false;

}

static bool audio_in_ignore(void) {
    if (!microphone_on) {
        return false;
    }

    if (--microphone_count > 0) {
        return true;
    }

    if (audio_in_handle) {
        if (audio_in_handle == (void *)1) {
            audio_close(audio_in_handle);
            microphone_on    = false;
            microphone_count = 0;
            return false;
        }
        alcCaptureStop(audio_in_handle);
    }

    microphone_on = false;
    microphone_count = 0;
    return false;
}

bool utox_audio_in_device_set(ALCdevice *new_device) {
    if (microphone_on || microphone_count) {
        return false;
    }

    if (new_device) {
        audio_in_device = new_device;
        LOG_TRACE("uTox Audio", "Audio in device changed." );
        return true;
    }

    audio_in_device = NULL;
    audio_in_handle = NULL;
    LOG_ERR("uTox Audio", "Audio out device set to null." );
    return false;
}

ALCdevice *utox_audio_in_device_get(void) {
    if (audio_in_handle) {
        return audio_in_device;
    }
    return NULL;
}

static ALCcontext *context;

static bool audio_out_device_open(void) {
    if (speakers_on) {
        speakers_count++;
        return true;
    }

    audio_out_handle = alcOpenDevice(audio_out_device);
    if (!audio_out_handle) {
        LOG_TRACE("uTox Audio", "alcOpenDevice() failed" );
        speakers_on = false;
        return false;
    }

    context = alcCreateContext(audio_out_handle, NULL);
    if (!alcMakeContextCurrent(context)) {
        LOG_TRACE("uTox Audio", "alcMakeContextCurrent() failed" );
        alcCloseDevice(audio_out_handle);
        audio_out_handle = NULL;
        speakers_on = false;
        return false;
    }

    ALint error;
    alGetError(); /* clear errors */
    /* Create the buffers for the ringtone */
    alGenSources((ALuint)1, &preview);
    if ((error = alGetError()) != AL_NO_ERROR) {
        LOG_TRACE("uTox Audio", "Error generating source with err %x" , error);
        speakers_on = false;
        speakers_count = 0;
        return false;
    }
    /* Create the buffers for incoming audio */
    alGenSources((ALuint)1, &ringtone);
    if ((error = alGetError()) != AL_NO_ERROR) {
        LOG_TRACE("uTox Audio", "Error generating source with err %x" , error);
        speakers_on = false;
        speakers_count = 0;
        return false;
    }
    alGenSources((ALuint)1, &notifytone);
    if ((error = alGetError()) != AL_NO_ERROR) {
        LOG_TRACE("uTox Audio", "Error generating source with err %x" , error);
        speakers_on = false;
        speakers_count = 0;
        return false;
    }

    speakers_on = true;
    speakers_count = 1;
    return true;
}

static bool audio_out_device_close(void) {
    if (!audio_out_handle) {
        return false;
    }

    if (!speakers_on) {
        return false;
    }

    if (--speakers_count > 0) {
        return true;
    }

    alDeleteSources((ALuint)1, &preview);
    alDeleteSources((ALuint)1, &ringtone);
    alDeleteSources((ALuint)1, &notifytone);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(audio_out_handle);
    audio_out_handle = NULL;
    speakers_on = false;
    speakers_count = 0;
    return false;
}

bool utox_audio_out_device_set(ALCdevice *new_device) {
    if (new_device) {
        audio_out_device = new_device;
        LOG_TRACE("uTox Audio", "Audio out device changed." );
        return true;
    }

    audio_out_device = NULL;
    audio_out_handle = NULL;
    LOG_TRACE("uTox Audio", "Audio in device set to null." );
    return false;
}

ALCdevice *utox_audio_out_device_get(void) {
    if (audio_out_handle) {
        return audio_out_device;
    }
    return NULL;
}

void sourceplaybuffer(unsigned int f, const int16_t *data, int samples, uint8_t channels, unsigned int sample_rate) {
    if (!channels || channels > 2) {
        return;
    }

    ALuint source;
    if (f >= self.friend_list_size) {
        source = preview;
    } else {
        source = get_friend(f)->audio_dest;
    }

    ALuint bufid;
    ALint processed = 0, queued = 16;
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
    alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
    alSourcei(source, AL_LOOPING, AL_FALSE);

    if (processed) {
        ALuint bufids[processed];
        alSourceUnqueueBuffers(source, processed, bufids);
        alDeleteBuffers(processed - 1, bufids + 1);
        bufid = bufids[0];
    } else if (queued < 16) {
        alGenBuffers(1, &bufid);
    } else {
        LOG_TRACE("uTox Audio", "dropped audio frame" );
        return;
    }

    alBufferData(bufid, (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, data, samples * 2 * channels,
                 sample_rate);
    alSourceQueueBuffers(source, 1, &bufid);

    // LOG_TRACE("uTox Audio", "audio frame || samples == %i channels == %u rate == %u " , samples, channels, sample_rate);

    ALint state;
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) {
        alSourcePlay(source);
        // LOG_TRACE("uTox Audio", "Starting source %u" , i);
    }
}

static void audio_in_init(void) {
    const char *audio_in_device_list;
    audio_in_device_list = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
    if (audio_in_device_list) {
        audio_in_device = (void *)audio_in_device_list;
        LOG_TRACE("uTox Audio", "input device list:" );
        while (*audio_in_device_list) {
            LOG_TRACE("uTox Audio", "\t%s" , audio_in_device_list);
            postmessage_utox(AUDIO_IN_DEVICE, UI_STRING_ID_INVALID, 0, (void *)audio_in_device_list);
            audio_in_device_list += strlen(audio_in_device_list) + 1;
        }
    }
    postmessage_utox(AUDIO_IN_DEVICE, STR_AUDIO_IN_NONE, 0, NULL);
    audio_detect(); /* Get audio devices for windows */
}

static void audio_out_init(void) {
    const char *audio_out_device_list;
    if (alcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT")) {
        audio_out_device_list = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
    } else {
        audio_out_device_list = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
    }

    if (audio_out_device_list) {
        audio_out_device = (void *)audio_out_device_list;
        LOG_TRACE("uTox Audio", "output device list:" );
        while (*audio_out_device_list) {
            LOG_TRACE("uTox Audio", "\t%s" , audio_out_device_list);
            postmessage_utox(AUDIO_OUT_DEVICE, 0, 0, (void *)audio_out_device_list);
            audio_out_device_list += strlen(audio_out_device_list) + 1;
        }
    }

    audio_out_handle = alcOpenDevice(audio_out_device);
    if (!audio_out_handle) {
        LOG_TRACE("uTox Audio", "alcOpenDevice() failed" );
        return;
    }

    int attrlist[] = { ALC_FREQUENCY, UTOX_DEFAULT_SAMPLE_RATE_A, ALC_INVALID };

    context = alcCreateContext(audio_out_handle, attrlist);
    if (!alcMakeContextCurrent(context)) {
        LOG_TRACE("uTox Audio", "alcMakeContextCurrent() failed" );
        alcCloseDevice(audio_out_handle);
        return;
    }

    // ALint error;
    // alGetError(); /* clear errors */

    // alGenSources((ALuint)1, &ringtone);
    // if ((error = alGetError()) != AL_NO_ERROR) {
    //     LOG_TRACE("uTox Audio", "Error generating source with err %x" , error);
    //     return;
    // }

    // alGenSources((ALuint)1, &preview);
    // if ((error = alGetError()) != AL_NO_ERROR) {
    //     LOG_TRACE("uTox Audio", "Error generating source with err %x" , error);
    //     return;
    // }

    alcCloseDevice(audio_out_handle);
}

static bool audio_source_init(ALuint *source) {
    ALint error;
    alGetError();
    alGenSources((ALuint)1, source);
    if ((error = alGetError()) != AL_NO_ERROR) {
        LOG_TRACE("uTox Audio", "Error generating source with err %x" , error);
        return false;
    }
    return true;
}

static void audio_source_raze(ALuint *source) {
    LOG_INFO("Audio", "Deleting source");
    alDeleteSources((ALuint)1, source);
}

// clang-format off
enum {
    NOTE_none,
    NOTE_c3_sharp,
    NOTE_g3,
    NOTE_b3,
    NOTE_c4,
    NOTE_a4,
    NOTE_b4,
    NOTE_e4,
    NOTE_f4,
    NOTE_c5,
    NOTE_d5,
    NOTE_e5,
    NOTE_f5,
    NOTE_g5,
    NOTE_a5,
    NOTE_c6_sharp,
    NOTE_e6,
};

static struct {
    uint8_t note;
    double  freq;
} notes[] = {
    {NOTE_none,         1           }, /* Can't be 0 or openal will skip this note/time */
    {NOTE_c3_sharp,     138.59      },
    {NOTE_g3,           196.00      },
    {NOTE_b3,           246.94      },
    {NOTE_c4,           261.63      },
    {NOTE_a4,           440.f       },
    {NOTE_b4,           493.88      },
    {NOTE_e4,           329.63      },
    {NOTE_f4,           349.23      },
    {NOTE_c5,           523.25      },
    {NOTE_d5,           587.33      },
    {NOTE_e5,           659.25      },
    {NOTE_f5,           698.46      },
    {NOTE_g5,           783.99      },
    {NOTE_a5,           880.f       },
    {NOTE_c6_sharp,     1108.73     },
    {NOTE_e6,           1318.51     },
};

static struct melodies { /* C99 6.7.8/10 uninitialized arithmetic types are 0 this is what we want. */
    uint8_t count;
    uint8_t volume;
    uint8_t fade;
    uint8_t notes[8];
} normal_ring[16] = {
    {1, 14, 1, {NOTE_f5,        }},
    {1, 14, 1, {NOTE_f5,        }},
    {1, 14, 1, {NOTE_f5,        }},
    {1, 14, 1, {NOTE_c6_sharp,  }},
    {1, 14, 0, {NOTE_c5,        }},
    {1, 14, 1, {NOTE_c5,        }},
    {0, 0, 0,  {0,  }},
}, friend_offline[4] = {
    {1, 14, 1, {NOTE_c4, }},
    {1, 14, 1, {NOTE_g3, }},
    {1, 14, 1, {NOTE_g3, }},
    {0, 0, 0,  {0, }},
}, friend_online[4] = {
    {1, 14, 0, {NOTE_g3, }},
    {1, 14, 1, {NOTE_g3, }},
    {1, 14, 1, {NOTE_a4, }},
    {1, 14, 1, {NOTE_b4, }},
}, friend_new_msg[8] = {
    {1, 0, 0,  {0, }}, /* 3/8 sec of silence for spammy friends */
    {1, 0, 0,  {0, }},
    {1, 0, 0,  {0, }},
    {1, 9,  0, {NOTE_g5, }},
    {1, 9,  1, {NOTE_g5, }},
    {1, 12, 1, {NOTE_a4, }},
    {1, 10, 1, {NOTE_a4, }},
    {1, 0, 0,  {0, }},
}, friend_request[8] = {
    {1, 9,  0, {NOTE_g5, }},
    {1, 9,  1, {NOTE_g5, }},
    {1, 12, 1, {NOTE_b3, }},
    {1, 10, 1, {NOTE_b3, }},
    {1, 9,  0, {NOTE_g5, }},
    {1, 9,  1, {NOTE_g5, }},
    {1, 12, 1, {NOTE_b3, }},
    {1, 10, 0, {NOTE_b3, }},
};
// clang-format on

typedef struct melodies MELODY;

// TODO: These should be functions rather than macros that only work in a specific context.
#define FADE_STEP_OUT() (1 - ((double)(index % (sample_rate / notes_per_sec)) / (sample_rate / notes_per_sec)))
#define FADE_STEP_IN() (((double)(index % (sample_rate / notes_per_sec)) / (sample_rate / notes_per_sec)))

// GEN_NOTE_RAW is unused. Delete?
#define GEN_NOTE_RAW(x, a) ((a * base_amplitude) * (sin((tau * x) * index / sample_rate)))
#define GEN_NOTE_NUM(x, a) ((a * base_amplitude) * (sin((tau * notes[x].freq) * index / sample_rate)))

#define GEN_NOTE_NUM_FADE(x, a) \
    ((a * base_amplitude * FADE_STEP_OUT()) * (sin((tau * notes[x].freq) * index / sample_rate)))
// GEN_NOTE_NUM_FADE_IN is unused. Delete?
#define GEN_NOTE_NUM_FADE_IN(x, a) \
    ((a * base_amplitude * FADE_STEP_IN()) * (sin((tau * notes[x].freq) * index / sample_rate)))

static void generate_melody(MELODY melody[], uint32_t seconds, uint32_t notes_per_sec, ALuint *target) {
    ALint error;
    alGetError(); /* clear errors */

    alGenBuffers((ALuint)1, target);

    if ((error = alGetError()) != AL_NO_ERROR) {
        LOG_TRACE("uTox Audio", "Error generating buffer with err %i" , error);
        return;
    }

    const uint32_t sample_rate    = 22000;
    const uint32_t base_amplitude = 1000;
    const double tau = 6.283185307179586476925286766559;

    const size_t buf_size = seconds * sample_rate * 2; // 16 bit (2 bytes per sample)
    int16_t *samples  = calloc(buf_size, sizeof(int16_t));

    if (!samples) {
        LOG_TRACE("uTox Audio", "Unable to generate ringtone buffer!" );
        return;
    }

    for (uint64_t index = 0; index < buf_size; ++index) {
        /* index / sample rate `mod` seconds. will give you full second long notes
         * you can change the length each tone is played by changing notes_per_sec
         * but you'll need to add additional case to cover the entire span of time */
        const int position = ((index / (sample_rate / notes_per_sec)) % (seconds * notes_per_sec));

        for (int i = 0; i < melody[position].count; ++i) {
            if (melody[position].fade) {
                samples[index] += GEN_NOTE_NUM_FADE(melody[position].notes[i], melody[position].volume);
            } else {
                samples[index] += GEN_NOTE_NUM(melody[position].notes[i], melody[position].volume);
            }
        }
    }

    alBufferData(*target, AL_FORMAT_MONO16, samples, buf_size, sample_rate);
    free(samples);
}

static void generate_tone_call_ringtone() { generate_melody(normal_ring, 4, 4, &RingBuffer); }

static void generate_tone_friend_offline() { generate_melody(friend_offline, 1, 4, &ToneBuffer); }

static void generate_tone_friend_online() { generate_melody(friend_online, 1, 4, &ToneBuffer); }

static void generate_tone_friend_new_msg() { generate_melody(friend_new_msg, 1, 8, &ToneBuffer); }

static void generate_tone_friend_request() { generate_melody(friend_request, 1, 8, &ToneBuffer); }

void postmessage_audio(uint8_t msg, uint32_t param1, uint32_t param2, void *data) {
    while (audio_thread_msg && utox_audio_thread_init) {
        yieldcpu(1);
    }

    audio_msg.msg    = msg;
    audio_msg.param1 = param1;
    audio_msg.param2 = param2;
    audio_msg.data   = data;

    audio_thread_msg = 1;
}

// TODO: This function is 300 lines long. Cut it up.
void utox_audio_thread(void *args) {
    time_t close_device_time = 0;
    ToxAV *av = args;

    #ifdef AUDIO_FILTERING
    LOG_INFO("uTox Audio", "Audio Filtering"
    #ifdef ALC_LOOPBACK_CAPTURE_SAMPLES
        " and Echo cancellation"
    #endif
        " enabled in this build" );
    #endif
    // bool call[MAX_CALLS] = {0}, preview = 0;

    const int perframe = (UTOX_DEFAULT_FRAME_A * UTOX_DEFAULT_SAMPLE_RATE_A) / 1000;
    uint8_t buf[perframe * 2 * UTOX_DEFAULT_AUDIO_CHANNELS]; //, dest[perframe * 2 * UTOX_DEFAULT_AUDIO_CHANNELS];
    memset(buf, 0, sizeof(buf));

    LOG_TRACE("uTox Audio", "frame size: %u" , perframe);

    /* init Microphone */
    audio_in_init();
    // audio_in_device_open();
    // audio_in_listen();

    /* init Speakers */
    audio_out_init();
    // audio_out_device_open();
    // audio_out_device_close();

    Filter_Audio *f_a = NULL;

    #define PREVIEW_BUFFER_SIZE (UTOX_DEFAULT_SAMPLE_RATE_A / 2)
    int16_t *preview_buffer = calloc(PREVIEW_BUFFER_SIZE, 2);
    if (!preview_buffer) {
        LOG_ERR("uTox Audio", "Unable to allocate memory for preview buffer.");
        return;
    }
    unsigned int preview_buffer_index = 0;
    bool preview_on = false;

    utox_audio_thread_init = true;
    while (1) {
        if (audio_thread_msg) {
            const TOX_MSG *m = &audio_msg;
            if (!m->msg) {
                break;
            }

            int call_ringing = 0;
            switch (m->msg) {
                case UTOXAUDIO_CHANGE_MIC: {
                    while (audio_in_ignore());
                    while (audio_in_device_close());

                    break;
                }
                case UTOXAUDIO_CHANGE_SPEAKER: {
                    while (audio_out_device_close());

                    break;
                }
                case UTOXAUDIO_START_FRIEND: {
                    FRIEND *f = get_friend(m->param1);
                    if (!f->audio_dest) {
                        audio_source_init(&f->audio_dest);
                    }
                    audio_out_device_open();
                    audio_in_listen();
                    break;
                }
                case UTOXAUDIO_STOP_FRIEND: {
                    FRIEND *f = get_friend(m->param1);
                    if (f->audio_dest) {
                        audio_source_raze(&f->audio_dest);
                        f->audio_dest = 0;
                    }
                    audio_in_ignore();
                    audio_out_device_close();
                    break;
                }
                case UTOXAUDIO_GROUPCHAT_START: {
                    GROUPCHAT *g = get_group(m->param1);
                    if (!g) {
                        LOG_ERR("uTox Audio", "Could not get group %u", m->param1);
                        break;
                    }

                    if (!g->audio_dest) {
                        audio_source_init(&g->audio_dest);
                    }

                    audio_out_device_open();
                    audio_in_listen();
                    break;
                }
                case UTOXAUDIO_GROUPCHAT_STOP: {
                    GROUPCHAT *g = get_group(m->param1);
                    if (!g) {
                        LOG_ERR("uTox Audio", "Could not get group %u", m->param1);
                        break;
                    }

                    if (g->audio_dest) {
                        audio_source_raze(&g->audio_dest);
                        g->audio_dest = 0;
                    }

                    audio_in_ignore();
                    audio_out_device_close();
                    break;
                }
                case UTOXAUDIO_START_PREVIEW: {
                    preview_on = true;
                    audio_out_device_open();
                    audio_in_listen();
                    break;
                }
                case UTOXAUDIO_STOP_PREVIEW: {
                    preview_on = false;
                    audio_in_ignore();
                    audio_out_device_close();
                    break;
                }
                case UTOXAUDIO_PLAY_RINGTONE: {
                    if (settings.ringtone_enabled && self.status != USER_STATUS_DO_NOT_DISTURB) {
                        LOG_INFO("uTox Audio", "Going to start ringtone!" );

                        audio_out_device_open();

                        generate_tone_call_ringtone();

                        alSourcei(ringtone, AL_LOOPING, AL_TRUE);
                        alSourcei(ringtone, AL_BUFFER, RingBuffer);

                        alSourcePlay(ringtone);
                        call_ringing++;
                    }
                    break;
                }
                case UTOXAUDIO_STOP_RINGTONE: {
                    call_ringing--;
                    LOG_INFO("uTox Audio", "Going to stop ringtone!" );
                    alSourceStop(ringtone);
                    yieldcpu(5);
                    audio_out_device_close();
                    break;
                }
                case UTOXAUDIO_PLAY_NOTIFICATION: {
                    if (settings.ringtone_enabled && self.status == USER_STATUS_AVAILABLE) {
                        LOG_INFO("uTox Audio", "Going to start notification tone!" );

                        if (close_device_time <= time(NULL)) {
                            audio_out_device_open();
                        }

                        switch (m->param1) {
                            case NOTIFY_TONE_FRIEND_ONLINE: {
                                generate_tone_friend_online();
                                break;
                            }
                            case NOTIFY_TONE_FRIEND_OFFLINE: {
                                generate_tone_friend_offline();
                                break;
                            }
                            case NOTIFY_TONE_FRIEND_NEW_MSG: {
                                generate_tone_friend_new_msg();
                                break;
                            }
                            case NOTIFY_TONE_FRIEND_REQUEST: {
                                generate_tone_friend_request();
                                break;
                            }
                        }

                        alSourcei(notifytone, AL_LOOPING, AL_FALSE);
                        alSourcei(notifytone, AL_BUFFER, ToneBuffer);

                        alSourcePlay(notifytone);

                        time(&close_device_time);
                        close_device_time += 10;
                        LOG_INFO("uTox Audio", "close device set!" );
                    }
                    break;
                }
                case UTOXAUDIO_STOP_NOTIFICATION: {
                    break;
                }
            }
            audio_thread_msg = 0;

            if (close_device_time && time(NULL) >= close_device_time) {
                LOG_INFO("uTox Audio", "close device triggered!" );
                audio_out_device_close();
                close_device_time = 0;
            }
        }

        // TODO move this code to filter_audio.c
        #ifdef AUDIO_FILTERING
        if (!f_a && settings.audiofilter_enabled) {
            f_a = new_filter_audio(UTOX_DEFAULT_SAMPLE_RATE_A);
            if (!f_a) {
                settings.audiofilter_enabled = false;
                LOG_TRACE("uTox Audio", "filter audio failed" );
            } else {
                LOG_TRACE("uTox Audio", "filter audio on" );
            }
        } else if (f_a && !settings.audiofilter_enabled) {
            kill_filter_audio(f_a);
            f_a = NULL;
            LOG_TRACE("uTox Audio", "filter audio off" );
        }
        #else
        if (settings.audiofilter_enabled) {
            settings.audiofilter_enabled = false;
        }
        #endif

        bool sleep = true;

        if (microphone_on) {
            ALint samples;
            bool frame = 0;
            /* If we have a device_in we're on linux so we can just call OpenAL, otherwise we're on something else so
             * we'll need to call audio_frame() to add to the buffer for us. */
            if (audio_in_handle == (void *)1) {
                frame = audio_frame((void *)buf);
                if (frame) {
                    /* We have an audio frame to use, continue without sleeping. */
                    sleep = false;
                }
            } else {
                alcGetIntegerv(audio_in_handle, ALC_CAPTURE_SAMPLES, sizeof(samples), &samples);
                if (samples >= perframe) {
                    alcCaptureSamples(audio_in_handle, buf, perframe);
                    frame = true;
                    if (samples >= perframe * 2) {
                        sleep = false;
                    }
                }
            }

            #ifdef AUDIO_FILTERING
            #ifdef ALC_LOOPBACK_CAPTURE_SAMPLES
            if (f_a && settings.audiofilter_enabled) {
                alcGetIntegerv(audio_out_device, ALC_LOOPBACK_CAPTURE_SAMPLES, sizeof(samples), &samples);
                if (samples >= perframe) {
                    int16_t buffer[perframe];
                    alcCaptureSamplesLoopback(audio_out_handle, buffer, perframe);
                    pass_audio_output(f_a, buffer, perframe);
                    set_echo_delay_ms(f_a, UTOX_DEFAULT_FRAME_A);
                    if (samples >= perframe * 2) {
                        sleep = false;
                    }
                }
            }
            #endif
            #endif

            if (frame) {
                bool voice = true;
                #ifdef AUDIO_FILTERING
                if (f_a) {
                    const int ret = filter_audio(f_a, (int16_t *)buf, perframe);

                    if (ret == -1) {
                        LOG_TRACE("uTox Audio", "filter audio error" );
                    }

                    if (ret == 0) {
                        voice = false;
                    }
                }
                #endif

                /* If push to talk, we don't have to do anything */
                if (!check_ptt_key()) {
                    voice = false; // PTT is up, send nothing.
                }

                if (preview_on) {
                    if (preview_buffer_index + perframe > PREVIEW_BUFFER_SIZE) {
                        preview_buffer_index = 0;
                    }
                    sourceplaybuffer(self.friend_list_size, preview_buffer + preview_buffer_index, perframe,
                                     UTOX_DEFAULT_AUDIO_CHANNELS, UTOX_DEFAULT_SAMPLE_RATE_A);
                    if (voice) {
                        memcpy(preview_buffer + preview_buffer_index, buf, perframe * sizeof(int16_t));
                    } else {
                        memset(preview_buffer + preview_buffer_index, 0, perframe * sizeof(int16_t));
                    }
                    preview_buffer_index += perframe;
                }

                if (voice) {
                    size_t active_call_count = 0;
                    for (size_t i = 0; i < self.friend_list_count; i++) {
                        if (UTOX_SEND_AUDIO(i)) {
                            active_call_count++;
                            TOXAV_ERR_SEND_FRAME error = 0;
                            // LOG_TRACE("uTox Audio", "Sending audio frame!" );
                            toxav_audio_send_frame(av, get_friend(i)->number, (const int16_t *)buf, perframe,
                                                   UTOX_DEFAULT_AUDIO_CHANNELS, UTOX_DEFAULT_SAMPLE_RATE_A, &error);
                            if (error) {
                                LOG_TRACE("uTox Audio", "toxav_send_audio error friend == %lu, error ==  %i" , i, error);
                            } else {
                                // LOG_TRACE("uTox Audio", "Send a frame to friend %i" ,i);
                                if (active_call_count >= UTOX_MAX_CALLS) {
                                    LOG_TRACE("uTox Audio", "We're calling more peers than allowed by UTOX_MAX_CALLS, This is a bug" );
                                    break;
                                }
                            }
                        }
                    }

                    Tox *tox = toxav_get_tox(av);
                    uint32_t num_chats = tox_conference_get_chatlist_size(tox);

                    if (num_chats) {
                        for (size_t i = 0 ; i < num_chats; ++i) {
                            if (get_group(i) && get_group(i)->active_call) {
                                //LOG_INFO("uTox Audio", "Sending audio in groupchat %u", chats[i]);
                                toxav_group_send_audio(tox, i, (int16_t *)buf, perframe,
                                                       UTOX_DEFAULT_AUDIO_CHANNELS, UTOX_DEFAULT_SAMPLE_RATE_A);
                            }
                        }
                    }
                }
            }
        }

        if (sleep) {
            yieldcpu(50);
        }
    }

    utox_filter_audio_kill(f_a);

    // missing some cleanup ?
    alDeleteSources(1, &ringtone);
    alDeleteSources(1, &preview);
    alDeleteBuffers(1, &RingBuffer);

    while (audio_in_device_close());
    while (audio_out_device_close());

    audio_thread_msg       = 0;
    utox_audio_thread_init = false;
    free(preview_buffer);
    LOG_TRACE("uTox Audio", "Clean thread exit!");
}

void callback_av_group_audio(void *UNUSED(tox), int groupnumber, int peernumber, const int16_t *pcm, unsigned int samples,
                             uint8_t channels, unsigned int sample_rate, void *UNUSED(userdata))
{
    GROUPCHAT *g = get_group(groupnumber);
    if (!g) {
        LOG_ERR("uTox Audio", "Could not get group with number: %i", groupnumber);
        return;
    }

    LOG_INFO("uTox Audio", "Received audio in groupchat %i from peer %i", groupnumber, peernumber);

    uint64_t time = get_time();

    if (time - g->last_recv_audio[peernumber] > (uint64_t)1 * 1000 * 1000 * 1000) {
        postmessage_utox(GROUP_UPDATE, groupnumber, peernumber, NULL);
    }

    g->last_recv_audio[peernumber] = time;

    if (channels < 1 || channels > 2) {
        LOG_ERR("uTox Audio", "Can't continue, with channel > 2 or < 1.");
        return;
    }

    if (g->muted) {
        LOG_TRACE("uTox Audio", "Group %u audio muted.", groupnumber);
        return;
    }

    ALuint bufid;
    ALint processed = 0, queued = 16;
    alGetSourcei(g->source[peernumber], AL_BUFFERS_PROCESSED, &processed);
    alGetSourcei(g->source[peernumber], AL_BUFFERS_QUEUED, &queued);
    alSourcei(g->source[peernumber], AL_LOOPING, AL_FALSE);

    if (processed) {
        ALuint bufids[processed];
        alSourceUnqueueBuffers(g->source[peernumber], processed, bufids);
        alDeleteBuffers(processed - 1, bufids + 1);
        bufid = bufids[0];
    } else if(queued < 16) {
        alGenBuffers(1, &bufid);
    } else {
        LOG_TRACE("uTox Audio", "dropped audio frame %i %i" , groupnumber, peernumber);
        return;
    }

    alBufferData(bufid, (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, pcm, samples * 2 * channels,
                    sample_rate);
    alSourceQueueBuffers(g->source[peernumber], 1, &bufid);

    ALint state;
    alGetSourcei(g->source[peernumber], AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) {
        alSourcePlay(g->source[peernumber]);
        LOG_DEBUG("uTox Audio", "Starting source %i %i" , groupnumber, peernumber);
    }
}

void group_av_peer_add(GROUPCHAT *g, int peernumber) {
    if (!g || peernumber < 0) {
        LOG_ERR("uTox Audio", "Invalid groupchat or peer number");
        return;
    }

    LOG_INFO("uTox Audio", "Adding source for peer %u in group %u", peernumber, g->number);
    alGenSources(1, &g->source[peernumber]);
}

void group_av_peer_remove(GROUPCHAT *g, int peernumber) {
    if (!g || peernumber < 0) {
        LOG_ERR("uTox Audio", "Invalid groupchat or peer number");
        return;
    }

    LOG_INFO("uTox Audio", "Deleting source for peer %u in group %u", peernumber, g->number);
    alDeleteSources(1, &g->source[peernumber]);
}
