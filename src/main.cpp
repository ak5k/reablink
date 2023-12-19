#include "AudioEngine.hpp"

#define REAPERAPI_IMPLEMENT
#include <reaper_plugin_functions.h>

#include "Api.hpp"

extern "C"
{
REAPER_PLUGIN_DLL_EXPORT auto REAPER_PLUGIN_ENTRYPOINT(
    REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t* rec) -> int
{
    (void)hInstance;
    if (rec != nullptr && REAPERAPI_LoadAPI(rec->GetFunc) == 0)
    {
        return 1;
    }
    return 0;
}
}