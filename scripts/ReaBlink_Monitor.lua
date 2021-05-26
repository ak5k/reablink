header = "enabled | num peers | quantum | start stop sync | tempo   | beats(QN) | metro\n"
gfx.init("ReaBlink Monitor", 640, 32)

if not reaper.Blink_GetEnabled() then
  reaper.Blink_SetEnabled(true)
end

--reaper.Blink_SetStartStopSyncEnabled(true)
--reaper.Blink_SetPuppet(true)

local function main()    
    if reaper.GetPlayState() & 1 == 1 or reaper.GetPlayState() & 4 == 4 then
      pos = reaper.GetPlayPosition2()
    else
      pos = reaper.GetCursorPosition()
    end
  bpmDiv = reaper.TimeMap_GetDividedBpmAtTime(pos)
  _, _, bpm = reaper.TimeMap_GetTimeSigAtTime(0, pos)
  bpmRate = bpmDiv / bpm
  str = ""
  phaseStr = ""
  time = reaper.Blink_GetClockNow()
  time2 = reaper.time_precise()
  time_diff = math.abs(time2-time)
  linkEnabled = reaper.Blink_GetEnabled()
  numPeers = reaper.Blink_GetNumPeers()
  quantum  = reaper.Blink_GetQuantum()
  startStopSyncOn = reaper.Blink_GetStartStopSyncEnabled()
  if linkEnabled then
    enable = "yes"
  else
    enable = "no"
  end
  beats = reaper.Blink_GetBeatAtTime(time, quantum)
  phase = reaper.Blink_GetPhaseAtTime(time, quantum)*bpmRate
  if startStopSyncOn then
    startStop = "yes"
  else
    startStop = "no"
  end
  if reaper.Blink_GetPlaying() then
    isPlaying = "[playing]"
  else
    isPlaying = "[stopped]"
  end
  phaseStr = ""
  for i = 0, math.ceil(quantum*bpmRate-1) do
    if i < phase then
      phaseStr = phaseStr .. "X"
    else
      phaseStr = phaseStr .. "0"
    end
  end
  if math.floor(quantum*bpmRate) == quantum*bpmRate then
  str = string.format("%7s | ", enable)
  str = str .. string.format("%9d | ", numPeers)
  str = str .. string.format("%7d | ", quantum*bpmRate or 1)
  str = str .. string.format("%3s ", startStop)
  str = str .. string.format("%11s | ", isPlaying)
  str = str .. string.format("%7.2f | ", reaper.Blink_GetTempo())
  str = str .. string.format("%9.2f | ", beats)
  str = str .. phaseStr .. "\n"
  end
  gfx.x, gfx.y = 0, 0
  gfx.drawstr(header .. str)
  gfx.update()
  reaper.defer(main)
end

main()

reaper.atexit()