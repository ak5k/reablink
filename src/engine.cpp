// Make sure to define this before <cmath> is included for Windows
#ifdef LINK_PLATFORM_WINDOWS
#define _USE_MATH_DEFINES // NOLINT
#endif

#include "engine.hpp"
#include "RollingAverage.hpp"
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
  return sessionState.beatAtTime(
    mLink.clock().micros(), mSharedEngineData.quantum
  );
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

// Function to find a tempo/time signature marker by position
int FindTempoTimeSigMarkerByPosition(ReaProject* proj, double targetPos)
{
  int idx = 0;
  double pos;
  int measure;
  double beat;
  double bpm;
  int timesig_num, timesig_denom;
  bool lineartempo;

  while (true)
  {
    if (!GetTempoTimeSigMarker(
          proj, idx, &pos, &measure, &beat, &bpm, &timesig_num, &timesig_denom,
          &lineartempo
        ))
      break; // No more tempo/time signature markers

    if (pos == targetPos)
    {
      return idx; // Found the tempo/time signature marker
    }

    ++idx;
  }

  return -1; // Not found
}

// Function to find a region or marker by name containing a specific substring
int FindRegionOrMarkerByNameContaining(
  ReaProject* proj, const std::string& substring
)
{
  int idx = 0;
  bool isRegion;
  double pos, regEnd;
  int markerId;
  std::string regionName;

  while (true)
  {
    const char* namebuf;
    if (!EnumProjectMarkers2(
          proj, idx, &isRegion, &pos, &regEnd, &namebuf, &markerId
        ))
    {
      break; // No more markers or regions
    }

    regionName = namebuf;
    if (isRegion && regionName.find(substring) != std::string::npos)
    {
      return idx; // Found the region or marker
    }

    ++idx;
  }

  return -1; // Not found
}

double GetFrameTime()
{
  // calculate timer interval average
  static std::vector<double> timer_intervals(TIMER_INTERVALS_SIZE, 0.);

  static double time0 = 0.;
  // auto frame_time = 0.;
  auto now = std::chrono::high_resolution_clock::now();
  auto now_double =
    std::chrono::duration<double>(now.time_since_epoch()).count();
  static RollingAverage frame_time_avg(512);
  frame_time_avg.add(now_double - time0);
  time0 = now_double;
  return frame_time_avg.average();
}

double GetNextFullMeasureTimePosition()
{
  double currentPosition = GetCursorPosition(); // Get the current position
  int measures = 0;
  auto beats = TimeMap2_timeToBeats(0, currentPosition, &measures, 0, 0, 0);
  (void)beats;

  auto firstBeatPosition = TimeMap2_beatsToTime(0, 0, &measures);

  if (currentPosition == firstBeatPosition)
  {
    return currentPosition;
  }

  int measures3 = 0;
  int beats3 = 0;

  int measurePosition = (int)TimeMap2_timeToBeats(
    0, currentPosition, &measures3, &beats3, NULL,
    NULL
  ); // Convert the current position to beats
  (void)measurePosition;
  return TimeMap2_beatsToTime(
    0, beats3,
    &measures3
  ); // Convert the next full measure position to time
}

int SetLaunchPrerollRegion()
{
  double projectLength = GetProjectLength(0); // Get the length of the project
  int measures3 = 0;
  int beats3 = 0;
  double num_measures = TimeMap2_timeToBeats(
    0, projectLength, &measures3, &beats3, NULL,
    NULL
  ); // Convert the project length to measures/beats
  auto region_start = TimeMap2_beatsToTime(0, beats3, &measures3);
  auto pos = GetNextFullMeasureTimePosition();

  num_measures = TimeMap2_timeToBeats(
    0, region_start, &measures3, &beats3, NULL,
    NULL
  ); // Convert the project length to measures/beats
  auto region_end = TimeMap2_beatsToTime(0, beats3, &measures3);

  (void)pos;
  (void)num_measures;
  // Add the region to the project
  int isRegion = true; // We want to create a region, not a marker
  int color = 0;       // The color of the region (0 = default color)
  return AddProjectMarker2(
    0, isRegion, region_start, region_end, "reablink pre-roll", -1, color
  );
}

void ClearReablinkDummyObjects()
{
  int n = CountProjectMarkers(0, 0, 0);
  while (n-- > 0)
  {
    int idx = FindRegionOrMarkerByNameContaining(0, "reablink");
    if (idx == -1)
    {
      break;
    }
    const char* namebuf;
    double pos{0};
    EnumProjectMarkers(idx, NULL, &pos, NULL, &namebuf, NULL);
    if (namebuf != nullptr && strstr(namebuf, "reablink pre-roll") != nullptr)
    {
      DeleteTempoTimeSigMarker(0, FindTempoTimeSigMarkerByPosition(0, pos));
    }
    for (int i = CountMediaItems(0); i > -1; i--)
    {
      auto item = GetMediaItem(0, i);
      if (GetMediaItemInfo_Value(item, "D_POSITION") == pos)
      {
        DeleteTrackMediaItem(GetMediaItem_Track(item), item);
      }
    }
    DeleteProjectMarkerByIndex(0, idx);
  }
}

void AudioEngine::audioCallback(
  const std::chrono::microseconds hostTime, const std::size_t numSamples
)
{
  static bool quantized_launch{false};
  auto frame_time = GetFrameTime();
  static int frame_count{0};
  static double qn_prev{0};
  static int preroll_region_idx{0};
  static int target_region_idx{0};
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
      TimeMap_GetTimeSigAtTime(
        0, pos_target, &timesig_num, &timesig_denom, &tempo
      );
      target_region_idx = AddProjectMarker2(
        0, true, pos_target, pos_target + (60. / tempo * engineData.quantum),
        "reablink target", -1, 0
      );
      auto target_tempo_idx = FindTempoTimeSigMarker(0, pos_target);
      SetTempoTimeSigMarker(
        0, target_tempo_idx, pos_target, 0, 0, sessionState.tempo(),
        timesig_num, timesig_denom, false
      );
      preroll_region_idx = SetLaunchPrerollRegion();
      sessionState.requestBeatAtStartPlayingTime(0, engineData.quantum);
      auto beat_now = sessionState.beatAtTime(hostTime, engineData.quantum);
      auto beat_offset = engineData.quantum - abs(beat_now);
      double pos_preroll{0};
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

      SetTempoTimeSigMarker(
        0, -1, pos_preroll, 0, 0, sessionState.tempo(), timesig_num,
        timesig_denom, false
      );
      FindTempoTimeSigMarker(0, pos_preroll);

      MediaTrack* track = GetTrack(0, 0); // Get the first track

      // Create the new MIDI item

      CreateNewMIDIItemInProj(
        track, pos_preroll,
        pos_preroll + (60. / sessionState.tempo() * engineData.quantum), 0
      );

      int measures{0};
      TimeMap2_timeToBeats(0, pos_preroll, &measures, 0, 0, 0);
      auto pos_preroll_start = TimeMap2_beatsToTime(0, beat_offset, &measures);
      SetEditCurPos(
        pos_preroll_start + (hostTime - mLink.clock().micros()).count() / 1.0e6,
        false, false
      );

      quantized_launch = true;
    }
    frame_count = 0;
    OnPlayButton();
    jump_offset = 0;
    land_offset = 0;
    mIsPlaying = true;
    launch_cleared = false;
    PreventUIRefresh(-3);
  }
  else if (isPuppet && mIsPlaying && !sessionState.isPlaying())
  {
    // stop stuff
    OnStopButton();
    Main_OnCommand(40521, 0);
    mIsPlaying = false;
    qn_prev = 0;
    launch_cleared = false;
    quantized_launch = false;
    ClearReablinkDummyObjects();
    Undo_EndBlock("ReaBlink", -1);
  }
  else if (isPuppet && !mIsPlaying && !sessionState.isPlaying() && GetPlayState() & 1)
  {
    ClearReablinkDummyObjects();
    OnStopButton();
  }

  // bool lineartempo{false};
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
        ClearReablinkDummyObjects();
      }
    }

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
        sessionState.requestBeatAtTime(
          fmod(beat, 1.), hostTime, 4. / timesig_denom
        );
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
    static RollingAverage diff_avg(8);
    diff_avg.add(diff);
    diff = diff_avg.average();
    g_timeline_offset_reablink = diff;

    auto buf_len_time = GetOutputLatency();
    double limit_denom = 8.;
    if (buf_len_time / (numSamples / g_abuf_srate) > 3)
    {
      limit_denom = 1.0;
    }
    static auto limit = std::max(
      frame_time / limit_denom,
      buf_len_time / limit_denom
    ); // seconds

    if (!isMaster && isPuppet && mLink.numPeers() > 0 && !quantized_launch &&
        (sessionState.beatAtTime(hostTime, engineData.quantum) < 0 ||
         sessionState.beatAtTime(hostTime, engineData.quantum) > 1.666) &&
        abs(diff) > limit &&
        abs(reaper_phase_current - link_phase_current) < 0.5 &&
        GetToggleCommandState(40620) == 0)
    {
      limit = std::max(
        frame_time / limit_denom / 2,
        buf_len_time / limit_denom / 2
      ); // seconds
      if (reaper_phase_time > link_phase_time && Master_GetPlayRate(0) >= 1)
      {
        Main_OnCommand(40525, 0);
        Main_OnCommand(40525, 0);
      }
      else if (reaper_phase_time < link_phase_time && Master_GetPlayRate(0) <= 1)
      {
        Main_OnCommand(40524, 0);
        Main_OnCommand(40524, 0);
      }
    }
    else if (!isMaster && isPuppet && mLink.numPeers() > 0 && abs(diff) < limit && Master_GetPlayRate(0) != 1)
    {
      limit = std::max(
        frame_time / limit_denom,
        buf_len_time / limit_denom
      ); // seconds
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
      if ((r_pos > loop_start && r_pos < loop_end) && abs(beat - loop_end_beat) < 1)
      {
        new_tempo = hostBpm;
      }
      (void)measures;
    }
    if (!SetTempoTimeSigMarker(
          0, ptidx, timepos, measurepos, beatpos, new_tempo, timesig_num,
          timesig_denom, 0
        ))
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