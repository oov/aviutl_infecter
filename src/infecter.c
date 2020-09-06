#include "infecter.h"

#include "ver.h"

HANDLE lwinput = NULL;
INPUT_PLUGIN_TABLE *target = NULL;
INPUT_PLUGIN_TABLE orig = {0};

struct input_handle {
    INPUT_HANDLE ih;
    LPSTR file;
    int shared;
    ULONGLONG closed_at;
};

#define NUM_HANDLES 8
struct input_handle handles[NUM_HANDLES] = {0};

static struct input_handle *input_handle_find_exists(LPSTR file)
{
    for (int i = 0; i < NUM_HANDLES; ++i) {
        struct input_handle *h = &handles[i];
        if (!h->file) {
            continue;
        }
        if (strcmp(file, h->file) != 0) {
            continue;
        }
        return h;
    }
    return NULL;
}

static void input_handle_soft_close(struct input_handle *h) {
    if (--h->shared == 0) {
        h->closed_at = GetTickCount64();
        return;
    }
}

static void input_handle_soft_open(struct input_handle *h) {
    if (++h->shared == 1) {
        h->closed_at = 0;
        return;
    }
}

static BOOL input_handle_remove(struct input_handle *h) {
    BOOL r = TRUE;
    if (h->ih) {
        r = orig.func_close(h->ih);
        h->ih = NULL;
    }
    if (h->file) {
        free(h->file);
        h->file = NULL;
    }
    h->closed_at = 0;
    h->shared = 0;
    return r;
}

static BOOL input_handle_put(struct input_handle *h, LPSTR file) {
    INPUT_HANDLE ih = orig.func_open(file);
    if (!ih) {
        return FALSE;
    }
    h->ih = ih;
    size_t len = strlen(file)+1;
    h->file = malloc(len);
    memcpy(h->file, file, len);
    h->closed_at = 0;
    h->shared = 1;
    return TRUE;
}

static struct input_handle *input_handle_insert(LPSTR file)
{
    struct input_handle *closed_handle = NULL;
    for (int i = 0; i < NUM_HANDLES; ++i) {
        struct input_handle *h = &handles[i];
        if (!h->ih) { // found unused
            if (!input_handle_put(h, file)) {
                return NULL;
            }
            return h;
        }
        if (h->closed_at != 0) {
            // find oldest closed handle
            if (!closed_handle || closed_handle->closed_at > h->closed_at) {
                closed_handle = h;
            }
        }
    }
    // no space left but found soft closed handle
    if (closed_handle) {
        if (!input_handle_remove(closed_handle)) {
            return NULL;
        }
        if (!input_handle_put(closed_handle, file)) {
            return NULL;
        }
        return closed_handle;
    }
    return NULL;
}

static INPUT_HANDLE func_open(LPSTR file)
{
    struct input_handle *h = input_handle_find_exists(file);
    if (h) {
        input_handle_soft_open(h);
        return h;
    }
    return input_handle_insert(file);
}

static BOOL func_close(INPUT_HANDLE ih)
{
    struct input_handle *h = ih;
    input_handle_soft_close(h);
    return TRUE;
}

static BOOL func_info_get(INPUT_HANDLE ih, INPUT_INFO *iip)
{
    struct input_handle *h = ih;
    return orig.func_info_get(h->ih, iip);
}

static int func_read_video(INPUT_HANDLE ih, int frame, void *buf)
{
    struct input_handle *h = ih;
    int r = orig.func_read_video(h->ih, frame, buf);
    if (!r && frame > 0) {
        orig.func_read_video(h->ih, frame-1, buf);
        r = orig.func_read_video(h->ih, frame, buf);
    }
    return r;
}

static int func_read_audio(INPUT_HANDLE ih, int start, int length, void *buf)
{
    struct input_handle *h = ih;
    return orig.func_read_audio(h->ih, start, length, buf);
}

static BOOL func_is_keyframe(INPUT_HANDLE ih, int frame)
{
    struct input_handle *h = ih;
    return orig.func_is_keyframe(h->ih, frame);
}

static BOOL infecter_init( FILTER *fp )
{
    HANDLE h = LoadLibraryA("lwinput.aui");
    if (!h) {
        h = LoadLibraryA("plugins\\lwinput.aui");
    }
    if (!h) {
        return FALSE;
    }
    lwinput = h;

    typedef INPUT_PLUGIN_TABLE* (__stdcall *get_input_plugin_table_func)(void);
    get_input_plugin_table_func fn = (void*)GetProcAddress(h, "GetInputPluginTable");
    if (!fn) {
        return FALSE;
    }

    target = fn();
    if (!target) {
        return FALSE;
    }

    orig.func_open = target->func_open;
    target->func_open = func_open;
    orig.func_close = target->func_close;
    target->func_close = func_close;
    orig.func_info_get = target->func_info_get;
    target->func_info_get = func_info_get;
    orig.func_read_video = target->func_read_video;
    target->func_read_video = func_read_video;
    orig.func_read_audio = target->func_read_audio;
    target->func_read_audio = func_read_audio;
    orig.func_is_keyframe = target->func_is_keyframe;
    target->func_is_keyframe = func_is_keyframe;
    return TRUE;
}

static BOOL infecter_exit( FILTER *fp )
{
    if (target) {
        target->func_open = orig.func_open;
        target->func_close = orig.func_close;
        target->func_info_get = orig.func_info_get;
        target->func_read_video = orig.func_read_video;
        target->func_read_audio = orig.func_read_audio;
        target->func_is_keyframe = orig.func_is_keyframe;
    }
    if (lwinput) {
        FreeLibrary(lwinput);
    }
    return TRUE;
}

#define INFECTER_NAME "lwinput infecter"
FILTER_DLL infecter = {
    FILTER_FLAG_ALWAYS_ACTIVE | FILTER_FLAG_NO_CONFIG | FILTER_FLAG_EX_INFORMATION,
    200,
    200,
    INFECTER_NAME,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    NULL,
    NULL,
    NULL,
    infecter_init,
    infecter_exit,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    INFECTER_NAME " " VERSION,
    NULL,
    NULL,
};