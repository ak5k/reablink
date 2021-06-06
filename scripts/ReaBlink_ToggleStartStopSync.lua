if reaper.Blink_GetStartStopSyncEnabled() then
  state = 0
  reaper.Blink_SetStartStopSyncEnabled(false)
else
  state = 1
  reaper.Blink_SetStartStopSyncEnabled(true)
end
is_new_value, filename, sectionID, cmdID, mode, resolution, val = reaper.get_action_context()
reaper.SetToggleCommandState(sectionID, cmdID, state)
reaper.RefreshToolbar2(sectionID, cmdID)
