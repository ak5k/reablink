if reaper.Blink_GetEnabled() then
    local pos, retval, _
    if reaper.GetPlayState() & 1 == 1 or reaper.GetPlayState() & 4 == 4 then
        pos = reaper.GetPlayPosition2()
    else
        pos = reaper.GetCursorPosition()
    end
    ptidx = reaper.FindTempoTimeSigMarker(0, pos)
    retval, _, _, _, _, _, timesig_denom, _ =
        reaper.GetTempoTimeSigMarker(0, ptidx)
    if retval then
        quantum_size = 4 / timesig_denom
        reaper.Blink_SetQuantum(reaper.Blink_GetQuantum() + quantum_size)
    end
end
