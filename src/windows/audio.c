#include "main.h"

#include "../debug.h"
#include "../macros.h"
#include "../main.h"

#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>
#include <math.h>

#include <audioclient.h>
#include <mmdeviceapi.h>

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC 10000000
#define REFTIMES_PER_MILLISEC 10000

#define EXIT_ON_ERROR(hres) \
    if (FAILED(hres)) {     \
        goto Exit;          \
    }

#define SAFE_RELEASE(punk)             \
    if ((punk) != NULL) {              \
        (punk)->lpVtbl->Release(punk); \
        (punk) = NULL;                 \
    }

IAudioClient *       pAudioClient   = NULL;
IAudioCaptureClient *pCaptureClient = NULL;
WAVEFORMATEX *       pwfx           = NULL;


// const GUID IID_IMMDeviceEnumerator = {0xa95664d2, 0x9614, 0x4f35, {0xa7,0x46, 0xde,0x8d,0xb6,0x36,0x17,0xe6}};
// const CLSID CLSID_MMDeviceEnumerator = {0xbcde0395, 0xe52f, 0x467c, {0x8e,0x3d, 0xc4,0x57,0x92,0x91,0x69,0x2e}};
// const GUID IID_IAudioClient = {0x1cb9ad4c, 0xdbfa, 0x4c32, {0xb1,0x78, 0xc2,0xf5,0x68,0xa7,0x03,0xb2}};


// const GUID KSDATAFORMAT_SUBTYPE_IEEE_FLOAT = {STATIC_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT};
// const GUID KSDATAFORMAT_SUBTYPE_PCM = {STATIC_KSDATAFORMAT_SUBTYPE_PCM};

const GUID IID_IAudioCaptureClient_utox = { 0xc8adbd64, 0xe71e, 0x48a0, { 0xa4, 0xde, 0x18, 0x5c, 0x39, 0x5c, 0xd3, 0x17 } };


/* note: only works when loopback is 48khz 2 channel floating*/
void audio_detect(void) {
    HRESULT        hr;
    REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
    // REFERENCE_TIME hnsActualDuration;
    UINT32               bufferFrameCount;
    IMMDeviceEnumerator *pEnumerator       = NULL;
    IMMDevice *          pDevice           = NULL;
    IMMDeviceCollection *pDeviceCollection = NULL;
    // BOOL bDone = FALSE;
    UINT count;
    // HANDLE hEvent = NULL;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void **)&pEnumerator);
    EXIT_ON_ERROR(hr)

    hr = pEnumerator->lpVtbl->EnumAudioEndpoints(pEnumerator, eAll, DEVICE_STATE_ACTIVE, &pDeviceCollection);
    EXIT_ON_ERROR(hr)

    hr = pDeviceCollection->lpVtbl->GetCount(pDeviceCollection, &count);
    EXIT_ON_ERROR(hr)

    LOG_TRACE("Windows", "Audio out devices %u" , count);

    hr = pEnumerator->lpVtbl->GetDefaultAudioEndpoint(pEnumerator, eRender, eConsole, &pDevice);
    EXIT_ON_ERROR(hr)

    hr = pDevice->lpVtbl->Activate(pDevice, &IID_IAudioClient, CLSCTX_ALL, NULL, (void **)&pAudioClient);
    EXIT_ON_ERROR(hr)

    hr = pAudioClient->lpVtbl->GetMixFormat(pAudioClient, &pwfx);
    EXIT_ON_ERROR(hr)

    LOG_INFO("Windows Audio", "default audio format: %u %u %u %lu %lu %u %u\n", WAVE_FORMAT_PCM, pwfx->wFormatTag,
          pwfx->nChannels, pwfx->nSamplesPerSec, pwfx->nAvgBytesPerSec, pwfx->wBitsPerSample, pwfx->nBlockAlign);

    if (pwfx->nSamplesPerSec != 48000 || pwfx->nChannels != 2 || pwfx->wFormatTag != WAVE_FORMAT_EXTENSIBLE) {
        LOG_TRACE("Windows", "Audio - unsupported format for loopback" );
        goto Exit;
    }

    WAVEFORMATEXTENSIBLE *wfx = (void *)pwfx;
    if (memcmp(&KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, &wfx->SubFormat, sizeof(wfx->SubFormat)) != 0) {
        goto Exit;
    }

    /* if(memcmp(&KSDATAFORMAT_SUBTYPE_PCM, &wfx->SubFormat, sizeof(wfx->SubFormat)) == 0) {
        printf("pcm\n");
    } else {
        printf("unknown\n");
    }*/

    hr = pAudioClient->lpVtbl->Initialize(pAudioClient, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK,
                                          hnsRequestedDuration, 0, pwfx, NULL);
    EXIT_ON_ERROR(hr)

    /*AUDCLNT_STREAMFLAGS_EVENTCALLBACK
    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hEvent == NULL)
    {
        hr = E_FAIL;
        goto Exit;
    }

    hr = pAudioClient->lpVtbl->
    EXIT_ON_ERROR(hr)*/

    // Get the size of the allocated buffer.
    hr = pAudioClient->lpVtbl->GetBufferSize(pAudioClient, &bufferFrameCount);
    EXIT_ON_ERROR(hr)

    hr = pAudioClient->lpVtbl->GetService(pAudioClient, &IID_IAudioCaptureClient_utox, (void **)&pCaptureClient);
    EXIT_ON_ERROR(hr)

    LOG_TRACE("Windows", "Audio frame count %u && Samples/s %lu" , bufferFrameCount, pwfx->nSamplesPerSec);

    // postmessage_utox(AUDIO_IN_DEVICE, STR_AUDIO_IN_DEFAULT_LOOPBACK, 0, (void*)(size_t)1);
    // this has no effect on my system, so I'm commenting it out, if you can't get audio, try enabling this again!
    return;

Exit:
    CoTaskMemFree(pwfx);
    SAFE_RELEASE(pEnumerator)
    SAFE_RELEASE(pDevice)
    SAFE_RELEASE(pAudioClient)
    SAFE_RELEASE(pCaptureClient)

    LOG_ERR("Windows", "Audio_init fail: %lu" , hr);
}

bool audio_init(void *UNUSED(handle)) {
    return SUCCEEDED(pAudioClient->lpVtbl->Start(pAudioClient));
}

bool audio_close(void *UNUSED(handle)) {
    return SUCCEEDED(pAudioClient->lpVtbl->Stop(pAudioClient));
}

static void *convertsamples(int16_t *dest, float *src, uint16_t samples) {
    if (!src) {
        memset(dest, 0, samples * 2);
        return NULL;
    }

    for (uint16_t i = 0; i != samples; i++) {
        float x = *src++;
        const float y = *src++;

        x = (x + y) * INT16_MAX / 2.0;

        if (x > INT16_MAX) {
            x = INT16_MAX;
        } else if (x < INT16_MIN) {
            x = INT16_MIN;
        }
        int16_t v = lrintf(x);
        *dest++   = v; // x;
    }

    return src;
}

bool audio_frame(int16_t *buffer) {
    // HRESULT hr;
    UINT32 numFramesAvailable;
    UINT32 packetLength = 0;
    BYTE * pData;
    DWORD  flags;

    pCaptureClient->lpVtbl->GetNextPacketSize(pCaptureClient, &packetLength);
    // hr = pCaptureClient->lpVtbl->GetNextPacketSize(pCaptureClient, &packetLength);
    // EXIT_ON_ERROR(hr)

    while (packetLength != 0) {
        // Get the available data in the shared buffer.
        pCaptureClient->lpVtbl->GetBuffer(pCaptureClient, &pData, &numFramesAvailable, &flags, NULL, NULL);
        // hr = pCaptureClient->lpVtbl->GetBuffer(pCaptureClient, &pData, &numFramesAvailable, &flags, NULL, NULL);
        // EXIT_ON_ERROR(hr)

        if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
            pData = NULL; // Tell CopyData to write silence.
        }

        if (numFramesAvailable != 480) {
            printf("ERROR\n");
        }

        static bool frame = true;

        convertsamples(&buffer[frame ? 0 : 480], (void *)pData, 480);

        frame = !frame;


        // Copy the available capture data to the audio sink.
        // printf("%u\n", numFramesAvailable);
        // hr = pMySink->CopyData(pData, numFramesAvailable, &bDone);
        // EXIT_ON_ERROR(hr)

        pCaptureClient->lpVtbl->ReleaseBuffer(pCaptureClient, numFramesAvailable);
        // hr = pCaptureClient->lpVtbl->ReleaseBuffer(pCaptureClient, numFramesAvailable);
        // EXIT_ON_ERROR(hr)

        if (frame) {
            return true;
        }

        pCaptureClient->lpVtbl->GetNextPacketSize(pCaptureClient, &packetLength);
        // hr = pCaptureClient->lpVtbl->GetNextPacketSize(pCaptureClient, &packetLength);
        // EXIT_ON_ERROR(hr)
    }

    return false;
}
