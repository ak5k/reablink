if reaper.Blink_GetEnabled() and not reaper.Blink_GetPuppet() then
  state = 1
  reaper.Blink_SetPuppet(true)
else
  state = 0
  reaper.Blink_SetMaster(false)
  reaper.Blink_SetPuppet(false)
end
is_new_value, filename, sectionID, cmdID, mode, resolution, val = reaper.get_action_context()
reaper.SetToggleCommandState(sectionID, cmdID,state)
reaper.RefreshToolbar2(sectionID, cmdID)
