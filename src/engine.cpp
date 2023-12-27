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

double GetFrameTime()
{
  // calculate timer interval average
  static std::vector<double> timer_intervals(TIMER_INTERVALS_SIZE, 0.);

  static double time0 = 0.;
  auto frame_time = 0.;
  auto now = std::chrono::high_resolution_clock::now();
  auto now_double =
    std::chrono::duration<double>(now.time_since_epoch()).count();
  if (time0 > 0.)
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

double GetNextFullMeasureTimePosition()
{
  double currentPosition = GetCursorPosition(); // Get the current position
  int measures = 0;
  auto beats = TimeMap2_timeToBeats(0, currentPosition, &measures, 0, 0, 0);

  auto firstBeatPosition = TimeMap2_beatsToTime(0, 0, &measures);

  if (currentPosition == firstBeatPosition)
  {
    return currentPosition;
  }

  int measures3 = 0;
  int beats3 = 0;

  int measurePosition =
    TimeMap2_timeToBeats(0, currentPosition, &measures3, &beats3, NULL,
                         NULL); // Convert the current position to beats

  return TimeMap2_beatsToTime(
    0, beats3,
    &measures3); // Convert the next full measure position to time
}

int SetLaunchPrerollRegion()
{
  double projectLength = GetProjectLength(0); // Get the length of the project
  int measures3 = 0;
  int beats3 = 0;
  double num_measures =
    TimeMap2_timeToBeats(0, projectLength, &measures3, &beats3, NULL,
                         NULL); // Convert the project length to measures/beats
  auto region_start = TimeMap2_beatsToTime(0, beats3, &measures3);
  auto pos = GetNextFullMeasureTimePosition();

  num_measures =
    TimeMap2_timeToBeats(0, region_start, &measures3, &beats3, NULL,
                         NULL); // Convert the project length to measures/beats
  auto region_end = TimeMap2_beatsToTime(0, beats3, &measures3);

  // Add the region to the project
  int isRegion = true; // We want to create a region, not a marker
  int color = 0;       // The color of the region (0 = default color)
  bool isSet = true;   // We want to set the region
  return AddProjectMarker2(0, isRegion, region_start, region_end,
                           "reablink pre-roll", -1, color);
}

void AudioEngine::audioCallback(const std::chrono::microseconds hostTime,
                                const std::size_t numSamples)
{
  static bool quantized_launch{false};
  auto frame_time = GetFrameTime();
  static int frame_count{0};
  static double qn_prev{0};
  static MediaItem* preroll_midi_item{nullptr};
  static int preroll_region_idx{0};
  static int target_region_idx{0};
  static int preroll_tempo_idx{0};
  static double jump_offset{0};
  static double land_offset{0};
  static bool launch_cleared{false};

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

  if (isPuppet && !mIsPlaying && sessionState.isPlaying())
  {
    // Reset the timeline so that beat 0 corresponds to the time when transport
    // starts
    auto time0 = time_precise();
    PreventUIRefresh(3);
    qn_prev = 0;
    Undo_BeginBlock();
    if (mLink.numPeers() > 0 && GetToggleCommandState(40620) == 0)
    {
      sessionState.setTempo(sessionState.tempo(), hostTime);
      auto pos_target = GetNextFullMeasureTimePosition();
      int timesig_num = 0;
      int timesig_denom = 0;
      double tempo = 0;
      TimeMap_GetTimeSigAtTime(0, pos_target, &timesig_num, &timesig_denom,
                               &tempo);
      target_region_idx = AddProjectMarker2(
        0, true, pos_target, pos_target + (60. / tempo * engineData.quantum),
        "reablink target", -1, 0);
      auto target_tempo_idx = FindTempoTimeSigMarker(0, pos_target);
      SetTempoTimeSigMarker(0, target_tempo_idx, pos_target, 0, 0,
                            sessionState.tempo(), timesig_num, timesig_denom,
                            false);
      preroll_region_idx = SetLaunchPrerollRegion();
      sessionState.requestBeatAtStartPlayingTime(0, engineData.quantum);
      auto beat_now = sessionState.beatAtTime(hostTime, engineData.quantum);
      auto beat_offset = engineData.quantum - abs(beat_now);
      double pos_preroll{0};
      int markers{0};
      int regions{0};
      // CountProjectMarkers(0, &markers, &regions);
      for (int i = 0; i < CountProjectMarkers(0, 0, 0); i++)
      {
        int region_idx{0};
        bool isRegion{false};
        EnumProjectMarkers(i, &isRegion, &pos_preroll, NULL, NULL, &region_idx);
        if (isRegion && region_idx == preroll_region_idx)
        {
          break;
        }
      }

      SetTempoTimeSigMarker(0, -1, pos_preroll, 0, 0, sessionState.tempo(),
                            timesig_num, timesig_denom, false);
      preroll_tempo_idx = FindTempoTimeSigMarker(0, pos_preroll);

      MediaTrack* track = GetTrack(0, 0); // Get the first track

      // Create the new MIDI item
      preroll_midi_item = CreateNewMIDIItemInProj(
        track, pos_preroll,
        pos_preroll + (60. / sessionState.tempo() * engineData.quantum), 0);

      int measures{0};
      TimeMap2_timeToBeats(0, pos_preroll, &measures, 0, 0, 0);
      auto pos_preroll_start = TimeMap2_beatsToTime(0, beat_offset, &measures);
      SetEditCurPos(pos_preroll_start +
                      (hostTime - mLink.clock().micros()).count() / 1.0e6,
                    false, false);

      // auto start_tempo_marker_idx =
      //   FindTempoTimeSigMarker(0, GetNextFullMeasureTimePosition());

      // int timesig_denom2 = 0;
      // int timesig_num2 = 0;
      // GetTempoTimeSigMarker(0, start_tempo_marker_idx, 0, 0, 0, 0,
      //                       &timesig_num2, &timesig_denom2, 0);
      // SetTempoTimeSigMarker(
      //   0, start_tempo_marker_idx,
      //   start_tempo_marker_idx == -1 ? 0 : GetNextFullMeasureTimePosition(),
      //   0, 0, sessionState.tempo(), timesig_num2, timesig_denom2, false);
      // SetEditCurPos(GetNextFullMeasureTimePosition() - 8 * frame_time, false,
      //               false);
      quantized_launch = true;
    }
    frame_count = 0;
    OnPlayButton();
    jump_offset = 0;
    land_offset = 0;
    auto time1 = time_precise();
    mIsPlaying = true;
    launch_cleared = false;
    PreventUIRefresh(-3);
  }
  else if (isPuppet && mIsPlaying && !sessionState.isPlaying())
  {
    // stop stuff
    PreventUIRefresh(3);
    OnStopButton();
    Main_OnCommand(40521, 0);
    mIsPlaying = false;
    qn_prev = 0;
    launch_cleared = false;
    quantized_launch = false;
    // Undo_DoUndo2(0);
    Undo_EndBlock("ReaBlink", -1);
    PreventUIRefresh(-3);
  }
  else if (isPuppet && !mIsPlaying && !sessionState.isPlaying() &&
           GetPlayState() & 1)
  {
    OnStopButton();
  }

  bool lineartempo{false};
  double beatpos{0};
  double timepos{0};
  double hostBpm{0};
  int measurepos{0};
  int timesig_num{0};
  int timesig_denom{0};
  int ptidx{0};
  auto r_pos = GetPlayState() & 1 ? GetPlayPosition2() : GetCursorPosition();
  TimeMap_GetTimeSigAtTime(0, r_pos, &timesig_num, &timesig_denom, &hostBpm);
  ptidx = FindTempoTimeSigMarker(0, r_pos);
  GetTempoTimeSigMarker(0, ptidx, &timepos, 0, 0, 0, 0, 0, 0);

  // update local quantum
  if (quantum() != (double)timesig_num / timesig_denom * 4.)
  {
    setQuantum((double)timesig_num / timesig_denom * 4.);
  }

  if (mIsPlaying)
  {
    if (isPuppet && frame_count == 1 && mLink.numPeers() > 0)
    {
      GoToRegion(0, target_region_idx, false);
      quantized_launch = false;
    }

    if (!launch_cleared)
    {
      int current_idx{0};
      int region_idx{0};
      bool isRegion{false};
      GetLastMarkerAndCurRegion(0, GetPlayPosition(), 0, &current_idx);
      EnumProjectMarkers(current_idx, &isRegion, 0, 0, 0, &region_idx);
      if (isRegion && region_idx == target_region_idx)
      {
        launch_cleared = true;
        PreventUIRefresh(3);
        for (int i = CountProjectMarkers(0, 0, 0) - 1; i > -1; i--)
        {
          int region_idx{0};
          bool isRegion{false};
          double pos_preroll{0};
          EnumProjectMarkers(i, &isRegion, &pos_preroll, NULL, NULL,
                             &region_idx);
          if (isRegion && (region_idx == preroll_region_idx ||
                           region_idx == target_region_idx))
          {
            DeleteProjectMarkerByIndex(0, i);
          }
        }
        DeleteTempoTimeSigMarker(0, preroll_tempo_idx);
        DeleteTrackMediaItem(GetMediaItem_Track(preroll_midi_item),
                             preroll_midi_item);
        PreventUIRefresh(-3);
      }
    }
    // // As long as the engine is playing, generate metronome clicks in
    // // the buffer at the appropriate beats.
    // renderMetronomeIntoBuffer(sessionState, engineData.quantum, hostTime,
    //                           numSamples);

    // // quantized launch
    // auto launch_buffer = 4 * frame_time;
    // auto link_start_time =
    //   sessionState.timeAtBeat(0, engineData.quantum).count() / 1.0e6;
    // auto reaper_start_time =
    //   mLink.clock().micros().count() / 1.0e6 + launch_buffer;

    // if (quantized_launch && mIsPlaying && sessionState.isPlaying() &&
    //     reaper_start_time > link_start_time)

    // {
    //   PreventUIRefresh(1);
    //   if (mLink.numPeers() > 0)
    //   {
    //     auto launch_offset = reaper_start_time - link_start_time;
    //     auto driver_launch_time =
    //       g_abuf_time + 2 * g_abuf_len / g_abuf_srate + GetOutputLatency();
    //     auto driver_offset =
    //       driver_launch_time - mLink.clock().micros().count() / 1.0e6;
    //     SetEditCurPos(GetNextFullMeasureTimePosition() - launch_buffer +
    //                     launch_offset + driver_offset +
    //                     g_launch_offset_reablink,
    //                   false, false);
    //   }
    //   jump_offset = 0;
    //   land_offset = 0;
    //   OnPlayButton();
    //   quantized_launch = false;
    //   PreventUIRefresh(-1);
    // }

    // set tempo if host /
    //   timeline has changed it
    if (sessionState.beatAtTime(hostTime, engineData.quantum) > 0. &&
        hostBpm != sessionState.tempo() && !(engineData.requestedTempo > 0.))
    {
      sessionState.setTempo(hostBpm, hostTime);
    }

    // get current qn/beat position
    auto pos = GetPlayPosition2();
    int measures{0};
    auto beat =
      TimeMap2_timeToBeats(0, pos, &measures, &timesig_num, 0, &timesig_denom);

    auto qn_abs = TimeMap2_timeToQN(0, pos);

    // handle looping/jumps
    if (abs(qn_abs - qn_prev) > 0.5 &&
        sessionState.beatAtTime(hostTime, engineData.quantum) > 1.)
    {
      if (GetSetRepeat(-1) == 1)
      {
        double start_pos{0};
        double end_pos{0};
        GetSet_LoopTimeRange(false, false, &start_pos, &end_pos, false);
        if (pos > start_pos && pos < end_pos)
        {
          auto start_beat =
            TimeMap2_timeToBeats(0, start_pos, &measures, 0, 0, 0);
          auto end_beat = TimeMap2_timeToBeats(0, end_pos, &measures, 0, 0, 0);
          jump_offset = fmod(jump_offset + end_beat, 1.0);
          land_offset = fmod(land_offset + start_beat, 1.0);
        }
      }
      else
      {
        sessionState.requestBeatAtTime(fmod(beat, 1.), hostTime,
                                       4. / timesig_denom);
      }
    }
    qn_prev = qn_abs;
    auto reaper_phase_current = fmod(beat - land_offset + jump_offset, 1.0);

    // sync
    auto link_phase_current =
      sessionState.phaseAtTime(hostTime, 4. / timesig_denom) *
      (timesig_denom / 4);
    auto reaper_phase_time = reaper_phase_current * 60. / sessionState.tempo();
    auto link_phase_time =
      fmod(link_phase_current, 1.0) * 60. / sessionState.tempo();

    auto diff = (reaper_phase_time - link_phase_time);
    g_timeline_offset_reablink = diff;
    // sessionState.beatAtTime(hostTime, engineData.quantum) < 0 ? 0. : diff;

    // ShowConsoleMsg(
    //   std::string( //
    //                //  "r: " + std::to_string(reaper_phase) +
    //                //  " l: " + std::to_string(link_phase) +
    //     "diff: " + std::to_string(g_timeline_offset_reablink) + "\n")
    //     .c_str());

    static auto limit = frame_time / 8; // seconds

    if (!isMaster && isPuppet && mLink.numPeers() > 0 && !quantized_launch &&
        (sessionState.beatAtTime(hostTime, engineData.quantum) < 0 ||
         sessionState.beatAtTime(hostTime, engineData.quantum) > 1.666) &&
        abs(diff) > limit &&
        abs(reaper_phase_current - link_phase_current) < 0.5 &&
        GetToggleCommandState(40620) == 0)
    {
      limit = frame_time / 16;
      if (reaper_phase_time > link_phase_time && Master_GetPlayRate(0) >= 1)
      {
        Main_OnCommand(40525, 0);
        Main_OnCommand(40525, 0);
      }
      else if (reaper_phase_time < link_phase_time &&
               Master_GetPlayRate(0) <= 1)
      {
        Main_OnCommand(40524, 0);
        Main_OnCommand(40524, 0);
      }
    }
    else if (!isMaster && isPuppet && mLink.numPeers() > 0 &&
             abs(diff) < limit && Master_GetPlayRate(0) != 1)
    {
      limit = frame_time / 8;
      Main_OnCommand(40521, 0);
    }
    else if ((mLink.numPeers() == 0 || isMaster) && abs(diff) > limit)
    {
      double beats = TimeMap2_timeToQN(0, pos);
      sessionState.forceBeatAtTime(beats, hostTime, engineData.quantum);
    }
  }

  if (engineData.requestedTempo > 0)
  {
    // Set the newly requested tempo from the beginning of this buffer
    sessionState.setTempo(engineData.requestedTempo, hostTime);
  }

  // set tempo
  if (isPuppet && engineData.requestedTempo > 0)
  {
    auto new_tempo = engineData.requestedTempo;
    if (GetSetRepeat(-1) == 1)
    {
      double loop_start{0};
      double loop_end{0};
      int measures{0};
      GetSet_LoopTimeRange(false, false, &loop_start, &loop_end, false);
      auto loop_end_beat = TimeMap2_timeToBeats(0, loop_end, 0, 0, 0, 0);
      auto beat = TimeMap2_timeToBeats(0, r_pos, 0, 0, 0, 0);
      if ((r_pos > loop_start && r_pos < loop_end) &&
          abs(beat - loop_end_beat) < 1)
      {
        new_tempo = hostBpm;
      }
    }
    if (!SetTempoTimeSigMarker(0, ptidx, timepos, measurepos, beatpos,
                               new_tempo, timesig_num, timesig_denom, 0))
    {
      sessionState.setTempo(new_tempo, hostTime);
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