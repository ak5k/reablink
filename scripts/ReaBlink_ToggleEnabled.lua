if reaper.Blink_GetEnabled() then
  state = 0
  reaper.Blink_SetEnabled(false)
else
  state = 1
  reaper.Blink_SetEnabled(true)
end
is_new_value, filename, sectionID, cmdID, mode, resolution, val = reaper.get_action_context()
reaper.SetToggleCommandState(sectionID, cmdID,state )
reaper.RefreshToolbar2( sectionID, cmdID )
