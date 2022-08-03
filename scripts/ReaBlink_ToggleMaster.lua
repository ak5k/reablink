if reaper.Blink_GetPuppet() and not reaper.Blink_GetMaster() then
    state = 1
    reaper.Blink_SetMaster(true)
else
    state = 0
    reaper.Blink_SetMaster(false)
end
is_new_value, filename, sectionID, cmdID, mode, resolution, val =
    reaper.get_action_context()
reaper.SetToggleCommandState(sectionID, cmdID, state)
reaper.RefreshToolbar2(sectionID, cmdID)
