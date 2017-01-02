#include "main.h"

#include "../flist.h"

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

        for(int i = 0; i < count; i++) {
            debug_notice("WINDND:\tSending file number %i\n", i);
            UTOX_MSG_FT *msg = calloc(count, sizeof(UTOX_MSG_FT));
            if (!msg) {
                debug_error("WINDND:\tUnable to alloc for UTOX_MSG_FT\n");
                return 0;
            }

            uint8_t *path = calloc(UTOX_FILE_NAME_LENGTH, sizeof(uint8_t));
            if (!path) {
                debug_error("WINDND:\tUnable to alloc for UTOX_MSG_FT\n");
                return 0;
            }

            DragQueryFile(h, i, path, UTOX_FILE_NAME_LENGTH);

            msg->file = fopen(path, "rb");
            if (!msg->file) {
                debug_error("WINDND:\tUnable to read file %s\n", path);
                return 0;
            }

            msg->name = path;
            postmessage_toxcore(TOX_FILE_SEND_NEW, ((FRIEND*)flist_get_selected()->data)->number, 0, msg);
            debug_info("WINDND:\tFile number %i sent!\n", i);
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
