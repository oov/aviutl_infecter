#include <windows.h>
#include "aviutl.h"
#include "infecter.h"

FILTER_DLL *filter_list[] = {&infecter, NULL};

EXTERN_C FILTER_DLL __declspec(dllexport) * *__stdcall GetFilterTableList(void)
{
    return (FILTER_DLL **)&filter_list;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        break;

    case DLL_PROCESS_DETACH:
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}