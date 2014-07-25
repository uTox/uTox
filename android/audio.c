/* uTox audio using OpenSL
 *  todo: error checking, only record when needed, audio sources only in "playing" state when they have something to play(does it make a difference?)
 */

static SLObjectItf engineObject = NULL;
static SLEngineItf engineEngine;

static SLObjectItf outputMixObject = NULL;

static SLObjectItf recorderObject = NULL;
static SLRecordItf recorderRecord;
static SLAndroidSimpleBufferQueueItf recorderBufferQueue;

//dont change this
#define FRAMES (960 * 3)

static short recbuf[960 * 2];

typedef struct
{
    SLObjectItf player;
    SLAndroidSimpleBufferQueueItf queue;
    uint8_t value;
    volatile _Bool queued[8];
    uint8_t unqueue;
    short buf[960 * 8];
} AUDIO_PLAYER;

AUDIO_PLAYER loopback, call_player[MAX_CALLS];

static SLDataFormat_PCM format_pcm = {
    .formatType = SL_DATAFORMAT_PCM,
    .numChannels = 1,
    .samplesPerSec = SL_SAMPLINGRATE_48,
    .bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16,
    .containerSize = SL_PCMSAMPLEFORMAT_FIXED_16,
    .channelMask = SL_SPEAKER_FRONT_CENTER,
    .endianness = SL_BYTEORDER_LITTLEENDIAN
};

volatile _Bool call[MAX_CALLS];

pthread_mutex_t callback_lock;

void* frames[128];
uint8_t frame_count;

static void player_queue(AUDIO_PLAYER *p, const int16_t *data)
{
    SLresult result;

    if(!p->queued[p->value]) {
        p->queued[p->value] = 1;

        memcpy(&p->buf[p->value * 960], data, 960 * 2);
        result = (*p->queue)->Enqueue(p->queue, &p->buf[p->value * 960], 960 * 2);

        p->value++;
        if(p->value == 8) {
            p->value = 0;
        }
    } else {
        debug("dropped\n");
    }
}

/* thread dedicated to encoding audio frames */
/* todo: exit */
void encoder_thread(void *arg)
{
    while(1) {
        void *frame;
        uint8_t c;

        pthread_mutex_lock(&callback_lock);

        c = volatile(frame_count);
        if(c) {
            frame = volatile(frames[0]);
            memmove(&frames[0], &frames[1], (c - 1) * sizeof(void*));
            frame_count--;
        }

        pthread_mutex_unlock(&callback_lock);

        if(c) {
            if(volatile(audio_preview)) {
                player_queue(&loopback, frame);
            }

            int i;
            for(i = 0; i < MAX_CALLS; i++) {
                if(call[i]) {
                    int r;
                    uint8_t dest[960 * 2];

                    if((r = toxav_prepare_audio_frame(arg, i, dest, sizeof(dest), frame, 960)) < 0) {
                        debug("toxav_prepare_audio_frame error %i\n", r);
                    }

                    if((r = toxav_send_audio(arg, i, dest, r)) < 0) {
                        debug("toxav_send_audio error %i %s\n", r, strerror(errno));
                    }
                }
            }

            free(frame);
        }

        if(c <= 1) {
            yieldcpu(1);
        }
    }
}

/* these two callbacks assume they will be called from the same thread (not at the same time from different threads) */
void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
    SLresult result;
    static _Bool b;
    short *buf = &recbuf[b ? 960 : 0];

    pthread_mutex_lock(&callback_lock);

    if(frame_count == 128) {
        debug("problem~!~\n");
    } else {
        void *frame = malloc(960 * 2);
        memcpy(frame, buf, 960 * 2);
        frames[frame_count++] = frame;
    }

    result = (*bq)->Enqueue(bq, buf, 960 * 2);
    b = !b;

    pthread_mutex_unlock(&callback_lock);
}

void playCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
    AUDIO_PLAYER *p = context;
    p->queued[p->unqueue++] = 0;
    if(p->unqueue == 8) {
        p->unqueue = 0;
    }
}

_Bool createAudioRecorder(ToxAv *av)
{
    SLresult result;

    // configure audio source
    SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
    SLDataSource audioSrc = {&loc_dev, NULL};

    // configure audio sink
    SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataSink audioSnk = {&loc_bq, &format_pcm};

    // create audio recorder
    // (requires the RECORD_AUDIO permission)
    const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    result = (*engineEngine)->CreateAudioRecorder(engineEngine, &recorderObject, &audioSrc, &audioSnk, 1, id, req);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }

    // realize the audio recorder
    result = (*recorderObject)->Realize(recorderObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }

    // get the record interface
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_RECORD, &recorderRecord);
    // get the buffer queue interface
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &recorderBufferQueue);

    // register callback on the buffer queue
    result = (*recorderBufferQueue)->RegisterCallback(recorderBufferQueue, bqRecorderCallback, NULL);

    pthread_mutex_init(&callback_lock, NULL);
    thread(encoder_thread, av);

    return 1;
}

void startRecording(void)
{
    SLresult result;

    result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_STOPPED);
    result = (*recorderBufferQueue)->Clear(recorderBufferQueue);

    result = (*recorderBufferQueue)->Enqueue(recorderBufferQueue, &recbuf[0], 960 * 2);
    result = (*recorderBufferQueue)->Enqueue(recorderBufferQueue, &recbuf[960], 960 * 2);

    result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_RECORDING);
}

void init_player(AUDIO_PLAYER *p)
{
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 8};
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};
    SLPlayItf bqPlayerPlay;

    const SLInterfaceID ids[] = {SL_IID_BUFFERQUEUE};
    const SLboolean reqs[] = {SL_BOOLEAN_TRUE};

    (*engineEngine)->CreateAudioPlayer(engineEngine, &p->player, &audioSrc, &audioSnk, 1, ids, reqs);
    (*p->player)->Realize(p->player, SL_BOOLEAN_FALSE);
    (*p->player)->GetInterface(p->player, SL_IID_PLAY, &bqPlayerPlay);
    (*p->player)->GetInterface(p->player, SL_IID_BUFFERQUEUE, &p->queue);
    (*p->queue)->RegisterCallback(p->queue, playCallback, p);
    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
}

void createEngine(void)
{
    SLresult result;

    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);

    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);

    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);

    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, NULL, NULL);

    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);


    init_player(&loopback);

    int i = 0;
    for(i = 0; i != MAX_CALLS; i++) {
        init_player(&call_player[i]);
    }
}

/* ASSUMES LENGTH == 960 */
void audio_play(int32_t call_index, const int16_t *data, int length)
{
    player_queue(&call_player[call_index], data);
}

void audio_begin(int32_t call_index)
{
    call[call_index] = 1;
}

void audio_end(int32_t call_index)
{
    call[call_index] = 0;
}
