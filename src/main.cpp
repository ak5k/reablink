#include "api.hpp"
#include <stdio.h>

#define REAPERAPI_IMPLEMENT
#include <reaper_plugin_functions.h>

#define REQUIRED_API(name)                                                     \
  {                                                                            \
    (void**)&name, #name, true                                                 \
  }
#define OPTIONAL_API(name)                                                     \
  {                                                                            \
    (void**)&name, #name, false                                                \
  }

static bool loadAPI(void* (*getFunc)(const char*))
{
  if (!getFunc)
    return false;

  struct ApiFunc
  {
    void** ptr;
    const char* name;
    bool required;
  };

  const ApiFunc funcs[]{REQUIRED_API(AddProjectMarker),
                        REQUIRED_API(AddProjectMarker2),
                        OPTIONAL_API(AddRemoveReaScript),
                        REQUIRED_API(Audio_RegHardwareHook),
                        REQUIRED_API(CountMediaItems),
                        REQUIRED_API(CountProjectMarkers),
                        REQUIRED_API(CreateNewMIDIItemInProj),
                        REQUIRED_API(DeleteProjectMarker),
                        REQUIRED_API(DeleteProjectMarkerByIndex),
                        REQUIRED_API(DeleteTempoTimeSigMarker),
                        REQUIRED_API(DeleteTrack),
                        REQUIRED_API(DeleteTrackMediaItem),
                        REQUIRED_API(EnumProjectMarkers),
                        REQUIRED_API(EnumProjectMarkers2),
                        REQUIRED_API(FindTempoTimeSigMarker),
                        REQUIRED_API(GetAppVersion),
                        REQUIRED_API(GetCursorPosition),
                        REQUIRED_API(GetExtState),
                        REQUIRED_API(GetLastMarkerAndCurRegion),
                        REQUIRED_API(GetMediaItem),
                        REQUIRED_API(GetMediaItemInfo_Value),
                        REQUIRED_API(GetMediaItem_Track),
                        REQUIRED_API(GetOutputLatency),
                        REQUIRED_API(GetPlayPosition),
                        REQUIRED_API(GetPlayPosition2),
                        REQUIRED_API(GetPlayState),
                        REQUIRED_API(GetProjectLength),
                        REQUIRED_API(GetResourcePath),
                        REQUIRED_API(GetSetRepeat),
                        REQUIRED_API(GetSet_LoopTimeRange),
                        REQUIRED_API(GetTempoTimeSigMarker),
                        REQUIRED_API(GetToggleCommandState),
                        REQUIRED_API(GetTrack),
                        REQUIRED_API(GoToRegion),
                        REQUIRED_API(Main_OnCommand),
                        REQUIRED_API(Master_GetPlayRate),
                        REQUIRED_API(Master_GetTempo),
                        REQUIRED_API(OnPlayButton),
                        REQUIRED_API(OnStopButton),
                        REQUIRED_API(PreventUIRefresh),
                        REQUIRED_API(SetEditCurPos),
                        REQUIRED_API(SetExtState),
                        REQUIRED_API(SetTempoTimeSigMarker),
                        REQUIRED_API(ShowConsoleMsg),
                        REQUIRED_API(TimeMap2_beatsToTime),
                        REQUIRED_API(TimeMap2_timeToBeats),
                        REQUIRED_API(TimeMap2_timeToQN),
                        REQUIRED_API(TimeMap_GetTimeSigAtTime),
                        REQUIRED_API(Undo_BeginBlock),
                        REQUIRED_API(Undo_EndBlock),
                        REQUIRED_API(UpdateTimeline),
                        REQUIRED_API(plugin_register),
                        REQUIRED_API(time_precise)};

  for (const ApiFunc& func : funcs)
  {
    *func.ptr = getFunc(func.name);

    if (func.required && !*func.ptr)
    {
      fprintf(stderr,
              "[ReaBlink] Unable to import the following API function: %s\n",
              func.name);
      return false;
    }
  }

  return true;
}

extern "C"
{
REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(
  REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t* rec)
{
  (void)hInstance;
  // if (rec != nullptr && REAPERAPI_LoadAPI(rec->GetFunc) == 0)
  if (rec != nullptr && loadAPI(rec->GetFunc))
  {
    reablink::Init(rec);
    return 1;
  }

  return 0;
}
}