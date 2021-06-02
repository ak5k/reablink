header = "enabled | num peers | quantum | start stop sync | tempo   | beats(QN) | metro\n"
gfx.init("ReaBlink Monitor", 640, 32)

if not reaper.Blink_GetEnabled() then 
  reaper.Blink_SetEnabled(true)
end

reaper.Blink_SetStartStopSyncEnabled(true)
reaper.Blink_SetPuppet(true)
reaper.Blink_SetCaptureTransportCommands(true)

--reaper.Blink_SetEngine()

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
  time = time2 or 0 
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
  
  --reaper.Blink_SetEngineState()
  
  gfx.x, gfx.y = 0, 0
  gfx.drawstr(header .. str)
  gfx.update()
  reaper.defer(main)
end

is_new_value, filename, sectionID, cmdID, mode, resolution, val = reaper.get_action_context()
reaper.SetToggleCommandState(sectionID, cmdID, 1)
reaper.RefreshToolbar2(sectionID, cmdID)

main()

local function shutdown()
  reaper.Blink_SetStartStopSyncEnabled(false)
  reaper.Blink_SetMaster(false)
  reaper.Blink_SetPuppet(false)
  reaper.Blink_SetCaptureTransportCommands(false)
  reaper.SetToggleCommandState(sectionID, cmdID, 0)
  reaper.RefreshToolbar2(sectionID, cmdID)
end

reaper.atexit(shutdown)