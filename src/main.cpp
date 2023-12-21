#include "api.hpp"

#define REAPERAPI_IMPLEMENT
#include <reaper_plugin_functions.h>

extern "C"
{
REAPER_PLUGIN_DLL_EXPORT auto REAPER_PLUGIN_ENTRYPOINT(
    REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t* rec) -> int
{
    (void)hInstance;
    if (rec != nullptr && REAPERAPI_LoadAPI(rec->GetFunc) == 0)
    {
        reablink::Init();
        return 1;
    }
    // reablink::Unregister();
    return 0;
}
}