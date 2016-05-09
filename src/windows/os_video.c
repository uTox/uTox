#include "../main.h"

IGraphBuilder *pGraph;
IBaseFilter *pGrabberF;
IMediaControl *pControl;
ISampleGrabber *pGrabber;

// TODO: free resources correctly (on failure, etc)
IBaseFilter *pNullF = NULL;

IPin *pPin = NULL, *pIPin;
HWND desktopwnd;
HDC desktopdc, capturedc;
HBITMAP capturebitmap;
_Bool capturedesktop;
void *dibits;

static uint16_t video_x, video_y;

// TODO this is in main.c too, probably want to decide how to handle this. FIXME!
static int utf8tonative(char_t *str, wchar_t *out, int length){
    return MultiByteToWideChar(CP_UTF8, 0, (char*)str, length, out, length);
}


void video_begin(uint32_t id, char_t *name, uint16_t name_length, uint16_t width, uint16_t height) {
    if(video_hwnd[id]) {
        return;
    }

    HWND *h = &video_hwnd[id];
    wchar_t out[name_length + 1];
    int len = utf8tonative(name, out, name_length);
    out[len] = 0;

    RECT r = {
        .left = 0,
        .top = 0,
        .right = width,
        .bottom = height
    };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, 0);

    width = r.right - r.left;
    height = r.bottom - r.top;

    if(width > GetSystemMetrics(SM_CXSCREEN)) {
        width = GetSystemMetrics(SM_CXSCREEN);
    }

    if(height > GetSystemMetrics(SM_CYSCREEN)) {
        height = GetSystemMetrics(SM_CYSCREEN);
    }

    *h = CreateWindowExW(0, L"uTox", out, WS_OVERLAPPEDWINDOW, 0, 0, width, height, NULL, NULL, hinstance, NULL);

    ShowWindow(*h, SW_SHOW);
}

void video_end(uint32_t id) {
    if(!video_hwnd[id]) {
        return;
    }

    DestroyWindow(video_hwnd[id]);
    video_hwnd[id] = NULL;
}

volatile _Bool newframe = 0;
uint8_t *frame_data;

HRESULT STDMETHODCALLTYPE test_SampleCB(ISampleGrabberCB *lpMyObj, double SampleTime, IMediaSample *pSample) {
    // you can call functions like:
    //REFERENCE_TIME   tStart, tStop;
    void *sampleBuffer;
    int length;

    pSample->lpVtbl->GetPointer(pSample, (BYTE**)&sampleBuffer);
    length = pSample->lpVtbl->GetActualDataLength(pSample);

    /*pSample->GetTime(&tStart, &tStop);
    */
    if(length == (uint32_t)video_width * video_height * 3)
    {
        uint8_t *p = frame_data + video_width * video_height * 3;
        int y;
        for(y = 0; y != video_height; y++) {
            p -= video_width * 3;
            memcpy(p, sampleBuffer, video_width * 3);
            sampleBuffer += video_width * 3;
        }

        newframe = 1;
    }

    //debug("frame %u\n", length);
    return S_OK;
}

STDMETHODIMP test_QueryInterface(ISampleGrabberCB *lpMyObj, REFIID riid, LPVOID FAR * lppvObj) { return 0; }
STDMETHODIMP_(ULONG) test_AddRef(ISampleGrabberCB *lpMyObj) { return 1; }

STDMETHODIMP_(ULONG) test_Release(ISampleGrabberCB *lpMyObj) {
    free(lpMyObj->lpVtbl);
    free(lpMyObj);
    return 0;
}

#define SafeRelease(x) if(*(x)) {(*(x))->lpVtbl->Release(*(x));}

HRESULT IsPinConnected(IPin *local_pPin, BOOL *pResult) {
    IPin *pTmp = NULL;
    HRESULT hr = local_pPin->lpVtbl->ConnectedTo(local_pPin, &pTmp);
    if (SUCCEEDED(hr))
    {
        *pResult = TRUE;
    }
    else if (hr == VFW_E_NOT_CONNECTED)
    {
        // The pin is not connected. This is not an error for our purposes.
        *pResult = FALSE;
        hr = S_OK;
    }

    SafeRelease(&pTmp);
    return hr;
}

HRESULT IsPinDirection(IPin *local_pPin, PIN_DIRECTION dir, BOOL *pResult) {
    PIN_DIRECTION pinDir;
    HRESULT hr = local_pPin->lpVtbl->QueryDirection(local_pPin, &pinDir);
    if (SUCCEEDED(hr))
    {
        *pResult = (pinDir == dir);
    }
    return hr;
}

HRESULT MatchPin(IPin *local_pPin, PIN_DIRECTION direction, BOOL bShouldBeConnected, BOOL *pResult) {
    //assert(pResult != NULL);

    BOOL bMatch = FALSE;
    BOOL bIsConnected = FALSE;

    HRESULT hr = IsPinConnected(local_pPin, &bIsConnected);
    if (SUCCEEDED(hr))
    {
        if (bIsConnected == bShouldBeConnected)
        {
            hr = IsPinDirection(local_pPin, direction, &bMatch);
        }
    }

    if (SUCCEEDED(hr))
    {
        *pResult = bMatch;
    }
    return hr;
}

HRESULT FindUnconnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin) {
    IEnumPins *pEnum = NULL;
    IPin *local_pPin = NULL;
    BOOL bFound = FALSE;

    HRESULT hr = pFilter->lpVtbl->EnumPins(pFilter, &pEnum);
    if (FAILED(hr))
    {
        goto done;
    }

    while (S_OK == pEnum->lpVtbl->Next(pEnum, 1, &local_pPin, NULL))
    {
        hr = MatchPin(local_pPin, PinDir, FALSE, &bFound);
        if (FAILED(hr))
        {
            goto done;
        }
        if (bFound)
        {
            *ppPin = local_pPin;
            (*ppPin)->lpVtbl->AddRef(*ppPin);
            break;
        }
        SafeRelease(&local_pPin);
    }

    if (!bFound)
    {
        hr = VFW_E_NOT_FOUND;
    }

done:
    SafeRelease(&local_pPin);
    SafeRelease(&pEnum);
    return hr;
}

IPin* ConnectFilters2(IGraphBuilder *_pGraph, IPin *pOut, IBaseFilter *pDest) {
    IPin *pIn = NULL;

    // Find an input pin on the downstream filter.
    HRESULT hr = FindUnconnectedPin(pDest, PINDIR_INPUT, &pIn);
    if (SUCCEEDED(hr))
    {
        // Try to connect them.
        hr = pGraph->lpVtbl->Connect(_pGraph, pOut, pIn);
        pIn->lpVtbl->Release(pIn);
    }
    return SUCCEEDED(hr) ? pIn : NULL;
}

HRESULT ConnectFilters(IGraphBuilder *_pGraph, IBaseFilter *pSrc, IBaseFilter *pDest) {
    IPin *pOut = NULL;

    // Find an output pin on the first filter.
    HRESULT hr = FindUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut);
    if (SUCCEEDED(hr))
    {
        if(!ConnectFilters2(_pGraph, pOut, pDest)) {
            hr = 1;
        }
        pOut->lpVtbl->Release(pOut);
    }
    return hr;
}

uint16_t native_video_detect(void) {
    // Indicate that we support desktop capturing.
    utox_video_append_device((void*)1, 1, STR_VIDEO_IN_DESKTOP, 0);

    max_video_width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    max_video_height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    HRESULT hr;
    CoInitialize(NULL);

    IMediaEventEx *pEvent;

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IGraphBuilder, (void**)&pGraph);
    if(FAILED(hr)) {
        return 0;
    }

    hr = pGraph->lpVtbl->QueryInterface(pGraph, &IID_IMediaControl, (void**)&pControl);
    if(FAILED(hr)) {
        return 0;
    }

    hr = pGraph->lpVtbl->QueryInterface(pGraph, &IID_IMediaEventEx, (void**)&pEvent);
    if(FAILED(hr)) {
        return 0;
    }

    hr = CoCreateInstance(&CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (void**)&pGrabberF);
    if(FAILED(hr)) {
        return 0;
    }

    hr = pGraph->lpVtbl->AddFilter(pGraph, pGrabberF, L"Sample Grabber");
    if(FAILED(hr)) {
        return 0;
    }

    hr = pGrabberF->lpVtbl->QueryInterface(pGrabberF, &IID_ISampleGrabber, (void**)&pGrabber);
    if(FAILED(hr)) {
        return 0;
    }

    AM_MEDIA_TYPE mt = {
        .majortype = MEDIATYPE_Video,
        .subtype = MEDIASUBTYPE_RGB24,
    };

    hr = pGrabber->lpVtbl->SetMediaType(pGrabber, &mt);
    if(FAILED(hr)) {
        return 0;
    }

    ICreateDevEnum *pSysDevEnum = NULL;
    hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, &IID_ICreateDevEnum, (void**)&pSysDevEnum);
    if(FAILED(hr)) {
        debug("CoCreateInstance failed()\n");
        return 0;
    }
    // Obtain a class enumerator for the video compressor category.
    IEnumMoniker *pEnumCat = NULL;
    hr = pSysDevEnum->lpVtbl->CreateClassEnumerator(pSysDevEnum, &CLSID_VideoInputDeviceCategory, &pEnumCat, 0);
    if(hr != S_OK) {

        pSysDevEnum->lpVtbl->Release(pSysDevEnum);
        debug("CreateClassEnumerator failed()\n");
        return 0;
    }

    IBaseFilter *pFilter = NULL;
    IMoniker *pMoniker = NULL;


    uint16_t device_count = 1; /* start at 1 because we support desktop grabbing */
    debug("Windows Video Devices:\n");
    ULONG cFetched;
    while(pEnumCat->lpVtbl->Next(pEnumCat, 1, &pMoniker, &cFetched) == S_OK) {
        IPropertyBag *pPropBag;
        hr = pMoniker->lpVtbl->BindToStorage(pMoniker, 0, 0, &IID_IPropertyBag, (void **)&pPropBag);
        if(SUCCEEDED(hr)) {
            // To retrieve the filter's friendly name, do the following:
            VARIANT varName;
            VariantInit(&varName);
            hr = pPropBag->lpVtbl->Read(pPropBag, L"FriendlyName", &varName, 0);
            if (SUCCEEDED(hr)) {
                if(varName.vt == VT_BSTR) {
                    debug("\tFriendly name: %ls\n", varName.bstrVal);
                } else {
                    debug("\tEw, got an unfriendly name\n");
                }
                // To create an instance of the filter, do the following:
                hr = pMoniker->lpVtbl->BindToObject(pMoniker, NULL, NULL, &IID_IBaseFilter, (void**)&pFilter);
                if(SUCCEEDED(hr)) {
                    int len = wcslen(varName.bstrVal);
                    void *data = malloc(sizeof(*pFilter) + len * 2);
                    WideCharToMultiByte(CP_UTF8, 0, varName.bstrVal, -1, data + sizeof(*pFilter), len * 2, NULL, 0);
                    memcpy(data, &pFilter, sizeof(pFilter));
                    utox_video_append_device(data, 0, data + 8, 1);
                    device_count++;
                }
            } else {
                debug("Windows Video Code:\tcouldn't get a name for this device, this is a bug, please report!\n");
            }

            VariantClear(&varName);
            // Now add the filter to the graph.
            // Remember to release pFilter later.
            pPropBag->lpVtbl->Release(pPropBag);
        }
        pMoniker->lpVtbl->Release(pMoniker);
    }
    pEnumCat->lpVtbl->Release(pEnumCat);
    pSysDevEnum->lpVtbl->Release(pSysDevEnum);

    hr = CoCreateInstance(&CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (void**)&pNullF);
    if(FAILED(hr)) {
        debug("CoCreateInstance failed\n");
        return 0;
    }

    hr = pGraph->lpVtbl->AddFilter(pGraph, pNullF, L"Null Filter");
    if(FAILED(hr)) {
        debug("AddFilter failed\n");
        return 0;
    }

    hr = ConnectFilters(pGraph, pGrabberF, pNullF);
    if(FAILED(hr)) {
        debug("ConnectFilters (2) failed\n");
        return 0;
    }

    /* I think this generates and formats the call back to copy each frame from the webcam */
    ISampleGrabberCB *test;
    test = malloc(sizeof(ISampleGrabberCB));
    test->lpVtbl = malloc(sizeof(*(test->lpVtbl)));
    // no idea what im doing here
    /* Yeah, me neither... */
    test->lpVtbl->QueryInterface = test_QueryInterface;
    test->lpVtbl->AddRef         = test_AddRef;
    test->lpVtbl->Release        = test_Release;
    test->lpVtbl->SampleCB       = test_SampleCB;
    test->lpVtbl->BufferCB       = 0;

    /* I think this sets the call back for each frame... */
    hr = pGrabber->lpVtbl->SetCallback(pGrabber, test, 0);
    if(FAILED(hr)) {
        return 0;
    }

    return device_count;
}

_Bool native_video_init(void *handle) {
    if((size_t)handle == 1) {
        video_x      = volatile(video_grab_x);
        video_y      = volatile(video_grab_y);
        video_width  = volatile(video_grab_w);
        video_height = volatile(video_grab_h);

        if(video_width & 1) {
            if(video_x & 1) {
                video_x--;
            }
            video_width++;
        }

        if(video_width & 2) {
            video_width -= 2;
        }

        if(video_height & 1) {
            if(video_y & 1) {
                video_y--;
            }
            video_height++;
        }

        debug("size: %u %u\n", video_width, video_height);

        desktopwnd = GetDesktopWindow();
        if(!desktopwnd) {
            debug("GetDesktopWindow() failed\n");
            return 0;
        }

        if(!(desktopdc = GetDC(desktopwnd))) {
            debug("GetDC(desktopwnd) failed\n");
           return 0;
        }

        if(!(capturedc = CreateCompatibleDC(desktopdc))) {
            debug("CreateCompatibleDC(desktopdc) failed\n");
            return 0;
        }

        if(!(capturebitmap = CreateCompatibleBitmap(desktopdc, video_width, video_height))) {
            debug("CreateCompatibleBitmap(desktopdc) failed\n");
            return 0;
        }

        SelectObject(capturedc, capturebitmap);
        dibits = malloc(video_width * video_height * 3);
        capturedesktop = 1;
        return 1;
    } else {
        capturedesktop = 0;
    }

    HRESULT hr;
    IBaseFilter *pFilter = handle;

    hr = pGraph->lpVtbl->AddFilter(pGraph, pFilter, L"Video Capture");
    if (FAILED(hr)) {
        debug("AddFilter failed\n");
        return 0;
    }

    IEnumPins *pEnum = NULL;

    /* build filter graph */
    hr = pFilter->lpVtbl->EnumPins(pFilter, &pEnum);
    if (FAILED(hr)) {
        debug("EnumPins failed\n");
        return 0;
    }

    while(S_OK == pEnum->lpVtbl->Next(pEnum, 1, &pPin, NULL)) {
        pIPin = ConnectFilters2(pGraph, pPin, pGrabberF);
        SafeRelease(&pPin);
        if(pIPin) {
            break;
        }
    }

    if (FAILED(hr)) {
        debug("failed to connect a filter\n");
        return 0;
    }

    IAMStreamConfig *pConfig = NULL;
    AM_MEDIA_TYPE *pmt = NULL;

    hr = pPin->lpVtbl->QueryInterface(pPin, &IID_IAMStreamConfig, (void**)&pConfig);
    if (FAILED(hr)) {
        debug("Windows Video device: QueryInterface failed\n");
        return 0;
    }

    hr = pConfig->lpVtbl->GetFormat(pConfig, &pmt);
    if (FAILED(hr)) {
        debug("Windows Video device: GetFormat failed\n");
        return 0;
    }

    BITMAPINFOHEADER *bmiHeader;
    if (IsEqualGUID(&pmt->formattype, &FORMAT_VideoInfo)) {
        VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER*)pmt->pbFormat;
        bmiHeader = &(pvi->bmiHeader);
        video_width = bmiHeader->biWidth;
        video_height = bmiHeader->biHeight;
    } else {
        debug("got bad format\n");
        video_width = 0;
        video_height = 0;
    }

    frame_data = malloc((size_t)video_width * video_height * 3);

    debug("Windows video init:\n\twidth height %u %u\n", video_width, video_height);

    return 1;
}

void native_video_close(void *handle) {
    if ((size_t)handle == 1) {
        ReleaseDC(desktopwnd, desktopdc);
        DeleteDC(capturedc);
        DeleteObject(capturebitmap);
        free(dibits);
        capturedesktop = 0;
        return;
    }

    debug("closing webcam... ");

    HRESULT hr;
    IBaseFilter *pFilter = handle;

    hr = pGraph->lpVtbl->RemoveFilter(pGraph, pFilter);
    if (FAILED(hr)) {
        return;
    }

    hr = pGraph->lpVtbl->Disconnect(pGraph, pPin);
    if (FAILED(hr)) {
        return;
    }

    hr = pGraph->lpVtbl->Disconnect(pGraph, pIPin);
    if (FAILED(hr)) {
        return;
    }

    debug("closed webcam\n");
}

int native_video_getframe(uint8_t *y, uint8_t *u, uint8_t *v, uint16_t width, uint16_t height) {
    if (width != video_width || height != video_height) {
        debug("width/height mismatch %u %u != %u %u\n", width, height, video_width, video_height);
        return 0;
    }

    if (capturedesktop) {
        static uint64_t lasttime;
        uint64_t t = get_time();
        if(t - lasttime >= (uint64_t)1000 * 1000 * 1000 / 24) {
            BITMAPINFO info = {
                .bmiHeader = {
                    .biSize = sizeof(BITMAPINFOHEADER),
                    .biWidth = video_width,
                    .biHeight = -(int)video_height,
                    .biPlanes = 1,
                    .biBitCount = 24,
                    .biCompression = BI_RGB,
                }
            };

            BitBlt(capturedc, 0, 0, video_width, video_height, desktopdc, video_x, video_y, SRCCOPY | CAPTUREBLT);
            GetDIBits(capturedc, capturebitmap, 0, video_height, dibits, &info, DIB_RGB_COLORS);
            bgrtoyuv420(y, u, v, dibits, video_width, video_height);
            lasttime = t;
            return 1;
        }
        return 0;
    }

    if(newframe) {
        newframe = 0;
        bgrtoyuv420(y, u, v, frame_data, video_width, video_height);
        return 1;
    }
    return 0;
}

_Bool native_video_startread(void) {
    if(capturedesktop) {
        return 1;
    }
    debug("start webcam\n");
    HRESULT hr;
    hr = pControl->lpVtbl->Run(pControl);
    if(FAILED(hr)) {
        debug("Run failed\n");
        return 0;
    }
    return 1;
}

_Bool native_video_endread(void) {
    if(capturedesktop) {
        return 1;
    }
    debug("stop webcam\n");
    HRESULT hr;
    hr = pControl->lpVtbl->StopWhenReady(pControl);
    if(FAILED(hr)) {
        debug("Stop failed\n");
        return 0;
    }
    return 1;
}
