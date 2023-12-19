if reaper.Blink_GetEnabled() then
    if reaper.Blink_GetPuppet() then
        reaper.Blink_SetTempo(reaper.Blink_GetTempo() - 1)
    else
        reaper.Blink_SetTempoAtTime(reaper.Blink_GetTempo() - 1,
                                    reaper.Blink_GetClockNow())
    end
end
