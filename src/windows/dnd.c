#include "main.h"

#include "../filesys.h"
#include "../file_transfers.h"
#include "../flist.h"
#include "../friend.h"
#include "../debug.h"
#include "../tox.h"
#include "../macros.h"

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
    LOG_NOTE("DnD", "Dropped!" );

    if (!flist_get_friend()) {
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
        LOG_INFO("WINDND", "%u files dropped\n", count);

        for (int i = 0; i < count; i++) {
            LOG_NOTE("WINDND", "Sending file number %i", i);
            UTOX_MSG_FT *msg = calloc(1, sizeof(UTOX_MSG_FT));
            if (!msg) {
                LOG_ERR("WINDND", "Unable to alloc for UTOX_MSG_FT");
                return 0;
            }

            char *path = calloc(1, UTOX_FILE_NAME_LENGTH);
            if (!path) {
                LOG_ERR("WINDND", "Unable to alloc for UTOX_MSG_FT");
                free(msg);
                return 0;
            }

            DragQueryFile(h, i, path, UTOX_FILE_NAME_LENGTH);

            msg->file = fopen(path, "rb");
            if (!msg->file) {
                LOG_ERR("WINDND", "Unable to read file %s" , path);
                free(msg);
                free(path);
                return 0;
            }

            msg->name = (uint8_t *)path;
            postmessage_toxcore(TOX_FILE_SEND_NEW, flist_get_friend()->number, 0, msg);
            LOG_INFO("WINDND", "File number %i sent!" , i);
        }

        ReleaseStgMedium(&medium);
    } else {
        LOG_ERR("WINDND", "itz failed! %lX", r);
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
