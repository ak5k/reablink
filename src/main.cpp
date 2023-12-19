#include "engine.hpp" // must include before reaper_plugin_functions.h

#define REAPERAPI_IMPLEMENT
#include <reaper_plugin_functions.h>

#include "api.hpp"

extern "C"
{
REAPER_PLUGIN_DLL_EXPORT auto REAPER_PLUGIN_ENTRYPOINT(
    REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t* rec) -> int
{
    (void)hInstance;
    if (rec != nullptr && REAPERAPI_LoadAPI(rec->GetFunc) == 0)
    {
        reablink::Register();
        return 1;
    }
    reablink::Unregister();
    return 0;
}
}