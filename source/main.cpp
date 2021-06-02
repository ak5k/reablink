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

#ifdef NDEBUG
#define REAPERAPI_MINIMAL
#define REAPERAPI_WANT_Audio_RegHardwareHook
#define REAPERAPI_WANT_FindTempoTimeSigMarker
#define REAPERAPI_WANT_GetAudioDeviceInfo
#define REAPERAPI_WANT_GetCursorPosition
#define REAPERAPI_WANT_GetOutputLatency
#define REAPERAPI_WANT_GetPlayPosition
#define REAPERAPI_WANT_GetPlayPosition2
#define REAPERAPI_WANT_GetPlayState
#define REAPERAPI_WANT_GetTempoTimeSigMarker
#define REAPERAPI_WANT_Main_OnCommand
#define REAPERAPI_WANT_Master_GetTempo
#define REAPERAPI_WANT_OnPlayButton
#define REAPERAPI_WANT_OnStopButton
#define REAPERAPI_WANT_SetEditCurPos
#define REAPERAPI_WANT_SetTempoTimeSigMarker
#define REAPERAPI_WANT_TimeMap2_beatsToTime
#define REAPERAPI_WANT_TimeMap2_timeToBeats
#define REAPERAPI_WANT_TimeMap_GetTimeSigAtTime
#define REAPERAPI_WANT_TimeMap_timeToQN_abs
#define REAPERAPI_WANT_Undo_BeginBlock
#define REAPERAPI_WANT_Undo_EndBlock
#define REAPERAPI_WANT_UpdateTimeline
#define REAPERAPI_WANT_plugin_register
#endif

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