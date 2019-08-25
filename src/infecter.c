#include "infecter.h"

#include "stb_ds.h"
#include "ver.h"

HANDLE lwinput = NULL;
INPUT_PLUGIN_TABLE *target = NULL;
INPUT_PLUGIN_TABLE orig = {0};

struct { char *key; INPUT_HANDLE value; } *handle_map = NULL;

static INPUT_HANDLE func_open(LPSTR file)
{
    INPUT_HANDLE h = shget(handle_map, file);
    if (!h) {
        h = orig.func_open(file);
        shput(handle_map, file, h);
    }
    return h;
}

static BOOL func_close(INPUT_HANDLE ih)
{
    // DO NOT CLOSE!
    return TRUE;
}

static int func_read_video(INPUT_HANDLE ih, int frame, void *buf)
{
    int r = orig.func_read_video(ih, frame, buf);
    if (!r && frame > 0) {
        orig.func_read_video(ih, frame-1, buf);
        r = orig.func_read_video(ih, frame, buf);
    }
    return r;
}

static BOOL infecter_init( FILTER *fp )
{
    sh_new_strdup(handle_map);
    shdefault(handle_map, NULL);

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
    orig.func_read_video = target->func_read_video;
    target->func_read_video = func_read_video;
    return TRUE;
}

static BOOL infecter_exit( FILTER *fp )
{
    if (target) {
        target->func_open = orig.func_open;
        target->func_close = orig.func_close;
        target->func_read_video = orig.func_read_video;
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