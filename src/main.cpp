#include "api.hpp"

#define REAPERAPI_WANT_AddProjectMarker
#define REAPERAPI_WANT_AddProjectMarker2
#define REAPERAPI_WANT_AddRemoveReaScript
#define REAPERAPI_WANT_Audio_RegHardwareHook
#define REAPERAPI_WANT_CountMediaItems
#define REAPERAPI_WANT_CountProjectMarkers
#define REAPERAPI_WANT_CreateNewMIDIItemInProj
#define REAPERAPI_WANT_DeleteProjectMarker
#define REAPERAPI_WANT_DeleteProjectMarkerByIndex
#define REAPERAPI_WANT_DeleteTempoTimeSigMarker
#define REAPERAPI_WANT_DeleteTrack
#define REAPERAPI_WANT_DeleteTrackMediaItem
#define REAPERAPI_WANT_EnumProjectMarkers
#define REAPERAPI_WANT_EnumProjectMarkers2
#define REAPERAPI_WANT_FindTempoTimeSigMarker
#define REAPERAPI_WANT_GetAppVersion
#define REAPERAPI_WANT_GetAudioDeviceInfo
#define REAPERAPI_WANT_GetCursorPosition
#define REAPERAPI_WANT_GetExtState
#define REAPERAPI_WANT_GetLastMarkerAndCurRegion
#define REAPERAPI_WANT_GetMediaItem
#define REAPERAPI_WANT_GetMediaItem_Track
#define REAPERAPI_WANT_GetMediaItemInfo_Value
#define REAPERAPI_WANT_GetOutputLatency
#define REAPERAPI_WANT_GetPlayPosition
#define REAPERAPI_WANT_GetPlayPosition2
#define REAPERAPI_WANT_GetPlayState
#define REAPERAPI_WANT_GetProjectLength
#define REAPERAPI_WANT_GetResourcePath
#define REAPERAPI_WANT_GetSet_LoopTimeRange
#define REAPERAPI_WANT_GetSetRepeat
#define REAPERAPI_WANT_GetTempoTimeSigMarker
#define REAPERAPI_WANT_GetToggleCommandState
#define REAPERAPI_WANT_GetTrack
#define REAPERAPI_WANT_GoToRegion
#define REAPERAPI_WANT_Main_OnCommand
#define REAPERAPI_WANT_Master_GetPlayRate
#define REAPERAPI_WANT_Master_GetTempo
#define REAPERAPI_WANT_OnPlayButton
#define REAPERAPI_WANT_OnStopButton
#define REAPERAPI_WANT_plugin_register
#define REAPERAPI_WANT_PreventUIRefresh
#define REAPERAPI_WANT_SetEditCurPos
#define REAPERAPI_WANT_SetExtState
#define REAPERAPI_WANT_SetTempoTimeSigMarker
#define REAPERAPI_WANT_time_precise
#define REAPERAPI_WANT_TimeMap2_beatsToTime
#define REAPERAPI_WANT_TimeMap2_timeToBeats
#define REAPERAPI_WANT_TimeMap2_timeToQN
#define REAPERAPI_WANT_TimeMap_GetTimeSigAtTime
#define REAPERAPI_WANT_Undo_BeginBlock
#define REAPERAPI_WANT_Undo_EndBlock
#define REAPERAPI_WANT_UpdateTimeline
#define REAPERAPI_WANT_AddProjectMarker
#define REAPERAPI_WANT_AddProjectMarker2
#define REAPERAPI_WANT_AddRemoveReaScript
#define REAPERAPI_WANT_Audio_RegHardwareHook
#define REAPERAPI_WANT_CountMediaItems
#define REAPERAPI_WANT_CountProjectMarkers
#define REAPERAPI_WANT_CreateNewMIDIItemInProj
#define REAPERAPI_WANT_DeleteProjectMarker
#define REAPERAPI_WANT_DeleteProjectMarkerByIndex
#define REAPERAPI_WANT_DeleteTempoTimeSigMarker
#define REAPERAPI_WANT_DeleteTrack
#define REAPERAPI_WANT_DeleteTrackMediaItem
#define REAPERAPI_WANT_EnumProjectMarkers
#define REAPERAPI_WANT_EnumProjectMarkers2
#define REAPERAPI_WANT_FindTempoTimeSigMarker
#define REAPERAPI_WANT_GetAppVersion
#define REAPERAPI_WANT_GetAudioDeviceInfo
#define REAPERAPI_WANT_GetCursorPosition
#define REAPERAPI_WANT_GetExtState
#define REAPERAPI_WANT_GetLastMarkerAndCurRegion
#define REAPERAPI_WANT_GetMediaItem
#define REAPERAPI_WANT_GetMediaItem_Track
#define REAPERAPI_WANT_GetMediaItemInfo_Value
#define REAPERAPI_WANT_GetOutputLatency
#define REAPERAPI_WANT_GetPlayPosition
#define REAPERAPI_WANT_GetPlayPosition2
#define REAPERAPI_WANT_GetPlayState
#define REAPERAPI_WANT_GetProjectLength
#define REAPERAPI_WANT_GetResourcePath
#define REAPERAPI_WANT_GetSet_LoopTimeRange
#define REAPERAPI_WANT_GetSetRepeat
#define REAPERAPI_WANT_GetTempoTimeSigMarker
#define REAPERAPI_WANT_GetToggleCommandState
#define REAPERAPI_WANT_GetTrack
#define REAPERAPI_WANT_GoToRegion
#define REAPERAPI_WANT_Main_OnCommand
#define REAPERAPI_WANT_Master_GetPlayRate
#define REAPERAPI_WANT_Master_GetTempo
#define REAPERAPI_WANT_OnPlayButton
#define REAPERAPI_WANT_OnStopButton
#define REAPERAPI_WANT_plugin_register
#define REAPERAPI_WANT_PreventUIRefresh
#define REAPERAPI_WANT_SetEditCurPos
#define REAPERAPI_WANT_SetExtState
#define REAPERAPI_WANT_SetTempoTimeSigMarker
#define REAPERAPI_WANT_time_precise
#define REAPERAPI_WANT_TimeMap2_beatsToTime
#define REAPERAPI_WANT_TimeMap2_timeToBeats
#define REAPERAPI_WANT_TimeMap2_timeToQN
#define REAPERAPI_WANT_TimeMap_GetTimeSigAtTime
#define REAPERAPI_WANT_Undo_BeginBlock
#define REAPERAPI_WANT_Undo_EndBlock
#define REAPERAPI_WANT_UpdateTimeline

#define REAPERAPI_MINIMAL
#define REAPERAPI_IMPLEMENT
#include <reaper_plugin_functions.h>

extern "C"
{
REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(
  REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t* rec)
{
  (void)hInstance;
  if (rec != nullptr && REAPERAPI_LoadAPI(rec->GetFunc) == 0)
  {
    return 1;
    reablink::Init();
  }
  // reablink::Unregister();
  return 0;
}
}