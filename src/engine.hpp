#ifndef REABLINK_ENGINE_HPP
#define REABLINK_ENGINE_HPP

#include <ableton/Link.hpp>
#include <mutex>

namespace ableton::linkaudio
{

class AudioEngine
{
public:
  AudioEngine(Link &link);
  static void TempoCallback(double bpm);
  void setMaster(bool isMaster);
  void setPuppet(bool isPuppet);
  bool getMaster();
  bool getPuppet();
  void startPlaying();
  void stopPlaying();
  bool isPlaying() const;
  double beatTime() const;
  void setTempo(double tempo);
  double quantum() const;
  void setQuantum(double quantum);
  bool isStartStopSyncEnabled() const;
  void setStartStopSyncEnabled(bool enabled);
  void audioCallback(std::chrono::microseconds hostTime,
                     std::size_t numSamples);
  void audioCallback2(std::chrono::microseconds hostTime,
                      std::size_t numSamples);

private:
  struct EngineData
  {
    double requestedTempo;
    bool requestStart;
    bool requestStop;
    double quantum;
    bool startStopSyncOn;
  };

  EngineData pullEngineData();
  Link &mLink; // NOLINT
  EngineData mSharedEngineData;
  EngineData mLockfreeEngineData;
  bool mIsPlaying; // NOLINT
  std::mutex mEngineDataGuard;

  std::atomic_bool isPuppet{false};
  std::atomic_bool isMaster{false};

  friend class AudioPlatform;

  // int playbackFrameCount = 0;
  double qnAbs = 0.;
  double qnJumpOffset = 0.;
  double qnLandOffset = 0.;
  double diff = 0;
  double qLen = 0;

  static constexpr auto beatTolerance = 0.02;
  // static constexpr auto playbackFrameSafe = 16;
  int syncTolerance = 6;
  static constexpr auto tempoTolerance = 0.001;
};

class AudioPlatform
{
public:
  AudioPlatform(Link &link) : mEngine(link)
  {
  }

  AudioEngine mEngine;
};
} // namespace ableton::linkaudio

#endif // REABLINK_ENGINE_HPP