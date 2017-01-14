#include "main.h"

#include "../flist.h"
#include "../logging_native.h"

typedef struct {
    IDropTarget dt;
    LONG ref;
} my_IDropTarget;

ULONG __stdcall dnd_AddRef(IDropTarget *lpMyObj) {
    my_IDropTarget *p = (void*)lpMyObj;
    return InterlockedIncrement(&p->ref);
}

ULONG __stdcall dnd_Release(IDropTarget *lpMyObj) {
    my_IDropTarget *p = (void*)lpMyObj;
    LONG count = InterlockedDecrement(&p->ref);

    if (!count) {
        free(lpMyObj->lpVtbl);
        free(lpMyObj);
    }

    return count;
}

HRESULT __stdcall dnd_QueryInterface(IDropTarget *lpMyObj, REFIID riid, LPVOID FAR *lppvObj) {
      *lppvObj = NULL;

//  PRINT_GUID (riid);
  if (IsEqualIID (riid, &IID_IUnknown) || IsEqualIID (riid, &IID_IDropTarget)) {
      dnd_AddRef (lpMyObj);
      *lppvObj = lpMyObj;
      return S_OK;
    }

    return E_NOINTERFACE;
}

HRESULT __stdcall dnd_DragEnter(IDropTarget *UNUSED(lpMyObj), IDataObject *UNUSED(pDataObject),
                                DWORD UNUSED(grfKeyState), POINTL UNUSED(pt), DWORD *pdwEffect) {
    *pdwEffect = DROPEFFECT_COPY;
    return S_OK;
}

HRESULT __stdcall dnd_DragOver(IDropTarget *UNUSED(lpMyObj), DWORD UNUSED(grfKeyState),
                               POINTL UNUSED(pt), DWORD *pdwEffect) {
    *pdwEffect = DROPEFFECT_COPY;
    return S_OK;
}

HRESULT __stdcall dnd_DragLeave(IDropTarget *UNUSED(lpMyObj)) {
    return S_OK;
}

HRESULT __stdcall dnd_Drop(IDropTarget *UNUSED(lpMyObj), IDataObject *pDataObject,
                           DWORD UNUSED(grfKeyState), POINTL UNUSED(pt), DWORD *pdwEffect) {
    *pdwEffect = DROPEFFECT_COPY;
    debug_notice("DnD:\tDroppped!\n");

    if (flist_get_selected()->item != ITEM_FRIEND) {
        return S_OK;
    }

    FORMATETC format = {
        .cfFormat = CF_HDROP,
        .dwAspect = DVASPECT_CONTENT,
        .lindex = -1,
        .tymed = TYMED_HGLOBAL,
    };
    STGMEDIUM medium;

    HRESULT r = pDataObject->lpVtbl->GetData(pDataObject, &format, &medium);
    if (r == S_OK) {
        HDROP h = medium.hGlobal;
        int count = DragQueryFile(h, ~0, NULL, 0);
        debug_info("%u files dropped\n", count);

        char *paths = calloc(count, sizeof(uint8_t) * UTOX_FILE_NAME_LENGTH);
        if (paths) {
            char *p = paths;
            for(int i = 0; i != count; i++) {
                p += DragQueryFile(h, i, p, UTOX_FILE_NAME_LENGTH);
                *p++ = '\n';
            }

            postmessage_toxcore(TOX_FILE_SEND_NEW, (FRIEND*)flist_get_selected()->data - friend, 0xFFFF, paths);
        } else {
            debug_error("DnD:\tUnable to get memory for drag and drop file names... this is bad!\n");
        }

        ReleaseStgMedium(&medium);
    } else {
        debug_error("itz failed! %lX\n", r);
    }

    return S_OK;
}

void dnd_init(HWND window) {
    my_IDropTarget *p;
    p = malloc(sizeof(my_IDropTarget));
    p->dt.lpVtbl = malloc(sizeof(*(p->dt.lpVtbl)));
    p->ref = 0;

    p->dt.lpVtbl->QueryInterface = dnd_QueryInterface;
    p->dt.lpVtbl->AddRef = dnd_AddRef;
    p->dt.lpVtbl->Release = dnd_Release;

    p->dt.lpVtbl->DragEnter = dnd_DragEnter;
    p->dt.lpVtbl->DragLeave = dnd_DragLeave;
    p->dt.lpVtbl->DragOver = dnd_DragOver;
    p->dt.lpVtbl->Drop = dnd_Drop;

    CoLockObjectExternal((struct IUnknown*)p, TRUE, FALSE);

    RegisterDragDrop(window, (IDropTarget*)p);
}
