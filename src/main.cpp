#include "ReaBlink.hpp"

#define REAPERAPI_IMPLEMENT
#include <reaper_plugin_functions.h>

// from cfillion reapack
#define REQUIRED_API(name)                                                     \
    {                                                                          \
        reinterpret_cast<void**>(&name), #name, true                           \
    }

static bool loadAPI(void* (*getFunc)(const char*))
{
    struct ApiFunc
    {
        void** ptr;
        const char* name;
        bool required;
    };

    const ApiFunc funcs[] {REQUIRED_API(Audio_RegHardwareHook),
                           REQUIRED_API(FindTempoTimeSigMarker),
                           REQUIRED_API(GetCursorPosition),
                           REQUIRED_API(GetOutputLatency),
                           REQUIRED_API(GetPlayPosition),
                           REQUIRED_API(GetPlayPosition2),
                           REQUIRED_API(GetPlayState),
                           REQUIRED_API(GetTempoTimeSigMarker),
                           REQUIRED_API(Main_OnCommand),
                           REQUIRED_API(Master_GetTempo),
                           REQUIRED_API(OnPlayButton),
                           REQUIRED_API(OnStopButton),
                           REQUIRED_API(SetEditCurPos),
                           REQUIRED_API(SetTempoTimeSigMarker),
                           REQUIRED_API(TimeMap2_beatsToTime),
                           REQUIRED_API(TimeMap2_timeToBeats),
                           REQUIRED_API(TimeMap_GetTimeSigAtTime),
                           REQUIRED_API(TimeMap_timeToQN_abs),
                           REQUIRED_API(Undo_BeginBlock),
                           REQUIRED_API(Undo_EndBlock),
                           REQUIRED_API(UpdateTimeline),
                           REQUIRED_API(plugin_register)};

    for (const ApiFunc& func : funcs)
    {
        *func.ptr = getFunc(func.name);
        if (func.required && *func.ptr == nullptr)
        {
            return false;
        }
    }

    return true;
}

extern "C"
{
REAPER_PLUGIN_DLL_EXPORT int ReaperPluginEntry(
    REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t* rec)
{
    (void)hInstance;
    if (!rec)
    {
        blink::unregisterReaBlink();
        return 0;
    }
    else if (rec->caller_version != REAPER_PLUGIN_VERSION ||
             !loadAPI(rec->GetFunc))
    {
        return 0;
    }
    blink::registerReaBlink();
    return 1;
}
}
