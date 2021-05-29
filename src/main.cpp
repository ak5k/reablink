// #if defined(_WIN32) && !defined(NDEBUG)
// // debug_new.cpp
// // compile by using: cl /EHsc /W4 /DNDEBUG /MDd debug_new.cpp
// #define _CRTDBG_MAP_ALLOC
// #include <crtdbg.h>
// #include <cstdlib>
// #ifndef NDEBUG
// #define DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
// // Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// // allocations to be of _CLIENT_BLOCK type
// #else
// #define DBG_NEW new
// #endif
// #endif

#include "reablink/ReaBlink.hpp"

#define REAPERAPI_IMPLEMENT
#include <reaper_plugin_functions.h>

REAPER_PLUGIN_HINSTANCE g_hInst; // to api extern

extern "C" {
REAPER_PLUGIN_DLL_EXPORT int ReaperPluginEntry(
    REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t* rec)
{
    g_hInst = hInstance;
    if (rec && REAPERAPI_LoadAPI(rec->GetFunc) == 0) {
        registerReaBlink();
        return 1;
    }
    else {
// #if defined(_WIN32) && !defined(NDEBUG)
//         _CrtDumpMemoryLeaks();
// #endif
        unregisterReaBlink();
        return 0;
    }
}
}