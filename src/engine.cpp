// Make sure to define this before <cmath> is included for Windows
#ifdef LINK_PLATFORM_WINDOWS
#define _USE_MATH_DEFINES // NOLINT
#endif

#include "engine.hpp"
#include "global_vars.hpp"
#include <algorithm>
#include <deque>
#include <numeric>
#include <vector>

#include <reaper_plugin_functions.h>

namespace reablink
{
using namespace ableton;

constexpr int TIMER_INTERVALS_SIZE = 512;

AudioEngine::AudioEngine(Link& link)
  : mLink(link), mSharedEngineData({0., false, false, 4., false}),
    mLockfreeEngineData(mSharedEngineData), mIsPlaying(false)
{
}

void AudioEngine::setMaster(bool isMaster)
{
  this->isMaster;
  this->isMaster = isMaster;
}

void AudioEngine::setPuppet(bool isPuppet)
{
  this->isPuppet = isPuppet;
}

bool AudioEngine::getMaster()
{
  return this->isMaster;
}

bool AudioEngine::getPuppet()
{
  return this->isPuppet;
}

void AudioEngine::startPlaying()
{
  std::lock_guard<std::mutex> lock(mEngineDataGuard);
  mSharedEngineData.requestStart = true;
}

void AudioEngine::stopPlaying()
{
  std::lock_guard<std::mutex> lock(mEngineDataGuard);
  mSharedEngineData.requestStop = true;
}

bool AudioEngine::isPlaying() const
{
  return mLink.captureAppSessionState().isPlaying();
}

double AudioEngine::beatTime() const
{
  const auto sessionState = mLink.captureAppSessionState();
  return sessionState.beatAtTime(mLink.clock().micros(),
                                 mSharedEngineData.quantum);
}

void AudioEngine::setTempo(double tempo)
{
  std::lock_guard<std::mutex> lock(mEngineDataGuard);
  mSharedEngineData.requestedTempo = tempo;
}

double AudioEngine::quantum() const
{
  return mSharedEngineData.quantum;
}

void AudioEngine::setQuantum(double quantum)
{
  std::lock_guard<std::mutex> lock(mEngineDataGuard);
  mSharedEngineData.quantum = quantum;
}

bool AudioEngine::isStartStopSyncEnabled() const
{
  return mLink.isStartStopSyncEnabled();
}

void AudioEngine::setStartStopSyncEnabled(const bool enabled)
{
  mLink.enableStartStopSync(enabled);
}

AudioEngine::EngineData AudioEngine::pullEngineData()
{
  auto engineData = EngineData{};
  if (mEngineDataGuard.try_lock())
  {
    engineData.requestedTempo = mSharedEngineData.requestedTempo;
    mSharedEngineData.requestedTempo = 0;
    engineData.requestStart = mSharedEngineData.requestStart;
    mSharedEngineData.requestStart = false;
    engineData.requestStop = mSharedEngineData.requestStop;
    mSharedEngineData.requestStop = false;

    mLockfreeEngineData.quantum = mSharedEngineData.quantum;
    mLockfreeEngineData.startStopSyncOn = mSharedEngineData.startStopSyncOn;

    mEngineDataGuard.unlock();
  }
  engineData.quantum = mLockfreeEngineData.quantum;

  return engineData;
}

// NOLINTBEGIN(*complexity)
#ifdef VEKTORBLOERG
void AudioEngine::audioCallback2(const std::chrono::microseconds hostTime,
                                 const std::size_t numSamples)
{
  (void)numSamples; // unused

  static bool sync_start{false};
  static bool syncCorrection = false;

  const auto engineData = pullEngineData();
  auto sessionState = mLink.captureAudioSessionState();
  // auto sessionState = mLink.captureAppSessionState();

  // calculate timer interval average
  static std::vector<double> timer_intervals(TIMER_INTERVALS_SIZE, 0.);

  static double time0 = 0.;
  auto frame_time = 0.;
  auto now = std::chrono::high_resolution_clock::now();
  auto now_double =
    std::chrono::duration<double>(now.time_since_epoch()).count();
  if (time0 > 0)
  {
    timer_intervals.push_back(now_double - time0);
    double sum = 0.0;
    int count = TIMER_INTERVALS_SIZE / 2;
    int num = 0;
    std::sort(timer_intervals.begin(), timer_intervals.end(),
              std::greater<double>());
    while (count > 0)
    {
      auto temp = timer_intervals.at(count + (TIMER_INTERVALS_SIZE / 4) - 1);
      if (temp > 0.0)
      {
        sum += temp;
        num++;
      }
      count--;
    }

    timer_intervals.pop_back();

    frame_time = sum / num;
  }
  time0 = now_double;

  // reaper timeline positions
  auto cpos = GetPlayPosition();
  // auto pos = GetPlayPosition(); // + frameTime.count() / 1.0e6; // =
  // hosttime
  // pos2
  auto pos2 = GetPlayPosition2(); // + frameTime.count() / 1.0e6; // = hosttime
  static int frame_count{0};
  if (!(GetPlayState() & 1) || (GetPlayState() & 2) ||
      (frame_count < 4 && (GetPlayState() & 1)))
  {
    pos2 = GetCursorPosition();
    cpos = GetCursorPosition();
  }

  // find current tempo and time sig
  // and active time sig marker
  bool lineartempo{false};
  double beatpos{0};
  double timepos{0};
  double hostBpm{0};
  int measurepos{0};
  int timesig_num{0};
  int timesig_denom{0};
  int ptidx{0};
  // TimeMap_GetTimeSigAtTime(0, pos2, &timesig_num, &timesig_denom, &hostBpm);
  // ptidx = FindTempoTimeSigMarker(0, pos2);
  TimeMap_GetTimeSigAtTime(0, cpos, &timesig_num, &timesig_denom, &hostBpm);
  ptidx = FindTempoTimeSigMarker(0, pos2);
  GetTempoTimeSigMarker(0, ptidx, &timepos, 0, 0, 0, 0, 0, 0);

  if (engineData.requestStart)
  {
    sessionState.setIsPlaying(true, hostTime);
  }

  if (engineData.requestStop)
  {
    sessionState.setIsPlaying(false, hostTime);
  }

  // static int frameCountDown {0};
  double wait_time{0};
  static double output_latency{0};

  if (!mIsPlaying && sessionState.isPlaying())
  {
    Undo_BeginBlock();
    // Reset the timeline so that beat 0 corresponds to the time when
    // transport starts
    // 'quantized launch' madness
    output_latency = GetOutputLatency();
    int ms{0};
    int ms_len{0};
    // (void)TimeMap2_timeToBeats(0, cpos, &ms, &ms_len, 0, 0);
    (void)TimeMap2_timeToBeats(0, GetCursorPosition(), &ms, &ms_len, 0, 0);
    timepos = TimeMap2_beatsToTime(0, ms_len, &ms);
    TimeMap_GetTimeSigAtTime(0, timepos, &timesig_num, &timesig_denom,
                             &hostBpm);
    sessionState.setTempo(hostBpm, hostTime);
    ptidx = FindTempoTimeSigMarker(0, timepos);
    double tempBpm{0};
    if (GetTempoTimeSigMarker(0, ptidx - 1, &timepos, &measurepos, &beatpos,
                              &tempBpm, &timesig_num, &timesig_denom,
                              &lineartempo))
    {
      SetTempoTimeSigMarker(0, ptidx - 1, timepos, measurepos, beatpos, hostBpm,
                            timesig_num, timesig_denom, lineartempo);
      GetTempoTimeSigMarker(0, ptidx, &timepos, &measurepos, &beatpos, 0,
                            &timesig_num, &timesig_denom, &lineartempo);
      // timepos = timepos;
    }

    sessionState.requestBeatAtStartPlayingTime(0, engineData.quantum);
    wait_time =
      ( //
        (double)sessionState.timeAtBeat(0.0, engineData.quantum).count() -
        (double)hostTime.count()) /
      1.0e6;
    frame_count = wait_time / frame_time;
    auto frame_wait_time = frame_count * frame_time;
    auto offset = frame_wait_time - wait_time;
    timepos = timepos + offset; // probably negative offset
    auto buffer_count = (int)(frame_wait_time / (g_abuf_len / g_abuf_srate));
    offset = buffer_count * (g_abuf_len / g_abuf_srate) - frame_wait_time;
    timepos = timepos + offset; // probably negative offset
    frame_count = frame_count > 0 ? frame_count : 1;
    frame_count++;

    if (mLink.numPeers() > 0)
    {
      SetEditCurPos(timepos, false, false);
    }
    OnPauseButton();
    sync_start = true;
    mIsPlaying = true;
  }
  else if (mIsPlaying && !sessionState.isPlaying())
  {
    OnStopButton();
    mIsPlaying = false;

    if (isPuppet)
    {
      // playbackFrameCount = 0;
      qnAbs = 0.;
      qnJumpOffset = 0.;
      qnLandOffset = 0.;
      syncCorrection = false;
      frame_count = 0;
      Main_OnCommand(40521, 0);
    }
    Undo_EndBlock("ReaBlink", -1);
  }

  // playback countdown and launch
  // if (mIsPlaying && sessionState.isPlaying() && frame_count > 0 &&
  //     (GetPlayState() & 2) //
  // )
  // {
  //   frame_count--;
  //   if (frame_count == 0)
  //   {
  //     OnPlayButton();
  //   }
  // }
  if (mIsPlaying && sync_start && (GetPlayState() & 2) &&
      sessionState.isPlaying() &&
      mLink.clock().micros().count() / 1.0e6 + frame_time + GetOutputLatency() +
          (2 * g_abuf_len / g_abuf_srate) >
        sessionState.timeAtBeat(0, engineData.quantum).count() / 1.0e6)

  {
    sync_start = false;
    frame_count = 0;
    OnPlayButton();
  }

  if (mIsPlaying && sessionState.isPlaying() //
        && (GetPlayState() & 1 || GetPlayState() & 4) ||
      (frame_count > 4 && !sync_start && (engineData.requestedTempo > 0)) //
  )
  {
    // get current qn position
    // set offsets if loop or jump

    auto currentQN = TimeMap_timeToQN_abs(0, pos2);
    auto host_pos = TimeMap_timeToQN_abs(0, cpos);
    host_pos = fmod(host_pos, 1.0);
    host_pos = host_pos * 60. / hostBpm;
    host_pos = host_pos * g_abuf_srate;

    auto ses_pos = sessionState.beatAtTime(mLink.clock().micros(), 1.);
    ses_pos = fmod(ses_pos, 1.0);
    ses_pos = ses_pos * 60. / sessionState.tempo();
    ses_pos = ses_pos * g_abuf_srate;

    if (mIsPlaying &&                                           // frame_count >
        4 && (currentQN < qnAbs || abs(currentQN - qnAbs) > 1.) // > 1. //
    )
    {
      // qnJumpOffset = sessionState.beatAtTime(hostTime, 1.);
      qnJumpOffset = sessionState.beatAtTime(hostTime, 1.);
      qnLandOffset = fmod(currentQN, 1.0);
    }
    qnAbs = currentQN;
    double currentHostBpm{0};
    // TimeMap_GetTimeSigAtTime(0, pos2 - frame_time, 0, 0, &currentHostBpm);
    TimeMap_GetTimeSigAtTime(0, pos2, 0, 0, &currentHostBpm);

    auto hostBeat =
      fmod(abs(fmod(qnAbs, 1.0) - qnLandOffset + qnJumpOffset), 1.0);

    auto sessionBeat = sessionState.phaseAtTime(hostTime, 1.);

    auto hostBeatDiff =
      abs(0.5 - fmod(abs(fmod(qnAbs, 1.0) - qnLandOffset + qnJumpOffset), 1.0));

    auto sessionBeatDiff = abs(0.5 - sessionState.phaseAtTime(hostTime, 1.));

    auto sessionBpm =
      sessionState.tempo(); // maybe Master has modified sessionState

    auto hostBeatTime = hostBeatDiff * 60. / (hostBpm);
    auto sessionBeatTime = sessionBeatDiff * 60. / (sessionBpm);
    auto qLen = floor((60. / sessionBpm) * 1.0e3);

    auto diff = abs(hostBeatTime - sessionBeatTime) * 1.0e3;
    // REAPER is master, unless user has requested tempo change
    if ((isMaster || mLink.numPeers() == 0) && !(engineData.requestedTempo > 0))
    {
      // try to improve sync if difference greater than 3 ms by forcing
      // local timeline upon peers
      if (sessionState
            .isPlaying() && // playbackFrameCount > playbackFrameSafe &&
          diff > syncTolerance &&
          diff < qLen - ceil((frame_time / 1.0e3) * 2))
      {
        double pushBeat{0};
        pushBeat = fmod(abs(fmod(TimeMap_timeToQN_abs(0, pos2), 1.) -
                            qnLandOffset + qnJumpOffset),
                        1.);
        if (abs(pushBeat - 1.) < beatTolerance)
        {
          pushBeat = 0.;
        }
        pushBeat = pushBeat + floor(sessionState.beatAtTime(hostTime, 1.));
        sessionState.forceBeatAtTime(pushBeat, hostTime, 1.);
      }
    }

    // REAPER is puppet, unless user has requested tempo change
    if (isPuppet && !(engineData.requestedTempo > 0))
    {
      if (!syncCorrection &&
          abs(currentHostBpm - sessionBpm) >
            sessionBpm * tempoTolerance * 1.5 &&
          (mIsPlaying || mLink.numPeers() == 0))
      {
        if (GetTempoTimeSigMarker(0, ptidx, &timepos, 0, 0, 0, 0, 0, 0))
        {
          double pushBeat{0};
          pushBeat = fmod(abs(fmod(TimeMap_timeToQN_abs(0, timepos), 1.) -
                              qnLandOffset + qnJumpOffset),
                          1.);
          if (abs(pushBeat - 1.) < beatTolerance)
          {
            pushBeat = 0.;
          }
          pushBeat += floor(sessionState.beatAtTime(hostTime, 1.));
          sessionState.setTempo(currentHostBpm,
                                sessionState.timeAtBeat(pushBeat, 1.));
        }
      }
      // try to improve sync with playrate change as long as difference is
      // greater than 3 ms
      else if (!isMaster && sessionState.isPlaying() &&
               // playbackFrameCount > playbackFrameSafe &&
               diff > syncTolerance && abs(hostBeat - sessionBeat) < 0.5
               // && diff < qLen - ceil((frameTime.count() / 1.0e3) * 2)
      )
      {
        if (!syncCorrection && frame_count > 12)
        {
          syncTolerance--;
          syncCorrection = true;
          if (hostBeat - sessionBeat > 0)
          {
            // slow_down
            Main_OnCommand(40525, 0);
          }
          else
          {
            Main_OnCommand(40524, 0);
          }
        }
      }
      // reset playback rate if sync corrected
      else if (syncCorrection && diff < syncTolerance)
      {
        Main_OnCommand(40521, 0);
        syncTolerance++;
        syncCorrection = false;
      }
    }
  }

  // set tempo
  if (isPuppet && engineData.requestedTempo > 0)
  {
    if (!SetTempoTimeSigMarker(0, ptidx, timepos, measurepos, beatpos,
                               engineData.requestedTempo, timesig_num,
                               timesig_denom, lineartempo))
    {
      sessionState.setTempo(engineData.requestedTempo, hostTime);
    }
  }

  frame_count++;
  if (isPuppet && frame_count % 12 == 0) // NOLINT
  {
    UpdateTimeline();
  }
  // Timeline modifications are complete, commit the results
  mLink.commitAudioSessionState(sessionState);
}
#endif

double GetNextFullMeasureTimePosition()
{
  double currentPosition = GetCursorPosition(); // Get the current position
  int timesig_num, timesig_denom;

  int measurePosition =
    TimeMap2_timeToBeats(0, currentPosition, &timesig_num, &timesig_denom, NULL,
                         NULL); // Convert the current position to beats

  return TimeMap2_beatsToTime(
    0, timesig_denom,
    &timesig_num); // Convert the next full measure position to time
}

double GetFrameTime()
{
  // calculate timer interval average
  static std::vector<double> timer_intervals(TIMER_INTERVALS_SIZE, 0.);

  static double time0 = 0.;
  auto frame_time = 0.;
  auto now = std::chrono::high_resolution_clock::now();
  auto now_double =
    std::chrono::duration<double>(now.time_since_epoch()).count();
  if (time0 > 0)
  {
    timer_intervals.push_back(now_double - time0);
    double sum = 0.0;
    int count = TIMER_INTERVALS_SIZE / 2;
    int num = 0;
    std::sort(timer_intervals.begin(), timer_intervals.end(),
              std::greater<double>());
    while (count > 0)
    {
      auto temp = timer_intervals.at(count + (TIMER_INTERVALS_SIZE / 4) - 1);
      if (temp > 0.0)
      {
        sum += temp;
        num++;
      }
      count--;
    }

    timer_intervals.pop_back();

    frame_time = sum / num;
  }
  time0 = now_double;
  return frame_time;
}

void AudioEngine::audioCallback(const std::chrono::microseconds hostTime,
                                const std::size_t numSamples)
{
  static double link_phase{0};
  static double link_phase0{0};
  static double reaper_phase{0};
  static double reaper_phase0{0};

  static bool quantized_launch{false};
  auto frame_time = GetFrameTime();
  static int frame_count{0};

  const auto engineData = pullEngineData();

  auto sessionState = mLink.captureAudioSessionState();

  if (engineData.requestStart)
  {
    sessionState.setIsPlaying(true, hostTime);
  }

  if (engineData.requestStop)
  {
    sessionState.setIsPlaying(false, hostTime);
  }

  bool lineartempo{false};
  double beatpos{0};
  double timepos{0};
  double hostBpm{0};
  int measurepos{0};
  int timesig_num{0};
  int timesig_denom{0};
  int ptidx{0};
  auto hear_pos = GetPlayState() & 1 ? GetPlayPosition() : GetCursorPosition();
  TimeMap_GetTimeSigAtTime(0, hear_pos, &timesig_num, &timesig_denom, &hostBpm);
  ptidx = FindTempoTimeSigMarker(0, hear_pos);
  GetTempoTimeSigMarker(0, ptidx, &timepos, 0, 0, 0, 0, 0, 0);

  if (!mIsPlaying && sessionState.isPlaying())
  {
    // Reset the timeline so that beat 0 corresponds to the time when transport
    // starts
    PreventUIRefresh(3);
    Undo_BeginBlock();
    if (mLink.numPeers() > 0)
    {
      sessionState.requestBeatAtStartPlayingTime(0, engineData.quantum);
      SetEditCurPos(GetNextFullMeasureTimePosition() - 8 * frame_time, false,
                    false);
    }
    OnPauseButton();
    mIsPlaying = true;
    quantized_launch = true;
    PreventUIRefresh(-3);
  }
  else if (mIsPlaying && !sessionState.isPlaying())
  {
    OnStopButton();
    Main_OnCommand(40521, 0);
    mIsPlaying = false;
    Undo_EndBlock("ReaBlink", -1);
  }

  if (engineData.requestedTempo > 0)
  {
    // Set the newly requested tempo from the beginning of this buffer
    sessionState.setTempo(engineData.requestedTempo, hostTime);
  }

  if (mIsPlaying)
  {
    // // As long as the engine is playing, generate metronome clicks in
    // // the buffer at the appropriate beats.
    // renderMetronomeIntoBuffer(sessionState, engineData.quantum, hostTime,
    //                           numSamples);

    auto launch_buffer = 4 * frame_time;
    auto link_start_time =
      sessionState.timeAtBeat(0, engineData.quantum).count() / 1.0e6;
    auto reaper_start_time =
      mLink.clock().micros().count() / 1.0e6 + launch_buffer;

    if (quantized_launch && mIsPlaying && sessionState.isPlaying() &&
        reaper_start_time > link_start_time)

    {
      PreventUIRefresh(1);
      if (mLink.numPeers() > 0)
      {
        auto launch_offset = reaper_start_time - link_start_time;
        auto driver_launch_time =
          g_abuf_time + 2 * g_abuf_len / g_abuf_srate + GetOutputLatency();
        auto driver_offset =
          driver_launch_time - mLink.clock().micros().count() / 1.0e6;
        SetEditCurPos(GetNextFullMeasureTimePosition() - launch_buffer +
                        launch_offset + driver_offset +
                        g_launch_offset_reablink,
                      false, false);
      }
      OnPlayButton();
      quantized_launch = false;
      PreventUIRefresh(-1);
    }

    auto pos = GetPlayPosition2();
    auto reaper_phase_current = TimeMap2_timeToQN(0, pos);
    reaper_phase_current = fmod(reaper_phase_current, 1.0);
    auto link_phase_current = sessionState.phaseAtTime(hostTime, 1.);
    auto reaper_phase_time = reaper_phase_current * 60. / sessionState.tempo();
    auto link_phase_time = link_phase_current * 60. / sessionState.tempo();

    auto diff = (reaper_phase_time - link_phase_time);
    g_timeline_offset_reablink =
      sessionState.beatAtTime(hostTime, engineData.quantum) < 0 ? 0. : diff;

    ShowConsoleMsg(
      std::string( //
                   //  "r: " + std::to_string(reaper_phase) +
                   //  " l: " + std::to_string(link_phase) +
        "diff: " + std::to_string(g_timeline_offset_reablink) + "\n")
        .c_str());

    auto limit = 0.0015; // seconds

    if (mLink.numPeers() > 0 && !quantized_launch &&
        sessionState.beatAtTime(hostTime, engineData.quantum) > 1.666 &&
        abs(diff) > limit &&
        abs(reaper_phase_current - link_phase_current) < 0.5)
    {
      if (reaper_phase_time > link_phase_time && Master_GetPlayRate(0) >= 1)
      {
        Main_OnCommand(40525, 0);
      }
      else if (reaper_phase_time < link_phase_time &&
               Master_GetPlayRate(0) <= 1)
      {
        Main_OnCommand(40524, 0);
      }
    }
    else if (mLink.numPeers() > 0 && abs(diff) < limit &&
             Master_GetPlayRate(0) != 1)
    {
      Main_OnCommand(40521, 0);
    }
    else if ((mLink.numPeers() == 0 || isMaster) && abs(diff) > limit)
    {
      double beats = TimeMap2_timeToQN(0, pos);
      sessionState.forceBeatAtTime(beats, hostTime, engineData.quantum);
    }
  }

  // set tempo
  if (isPuppet && engineData.requestedTempo > 0)
  {
    if (!SetTempoTimeSigMarker(0, ptidx, timepos, measurepos, beatpos,
                               engineData.requestedTempo, timesig_num,
                               timesig_denom, lineartempo))
    {
      sessionState.setTempo(engineData.requestedTempo, hostTime);
    }
  }

  frame_count++;
  if (isPuppet && frame_count % 12 == 0) // NOLINT
  {
    UpdateTimeline();
  }

  // Timeline modifications are complete, commit the results
  mLink.commitAudioSessionState(sessionState);
}

// NOLINTEND(*complexity)
} // namespace reablink