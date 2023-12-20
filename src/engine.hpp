#pragma once

// Make sure to define this before <cmath> is included for Windows
#define _USE_MATH_DEFINES // NOLINT
#include <ableton/Link.hpp>
#include <ableton/link/HostTimeFilter.hpp>
#include <atomic>
#include <mutex>

namespace ableton::linkaudio
{

class AudioEngine
{
  public:
    AudioEngine(Link& link);
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

    Link& mLink; // NOLINT
    EngineData mSharedEngineData;
    EngineData mLockfreeEngineData;
    bool mIsPlaying; // NOLINT
    std::mutex mEngineDataGuard;

    friend class AudioPlatform;
};

class AudioPlatform
{
  public:
    AudioPlatform(Link& link)
        : mEngine(link)
    {
    }

    AudioEngine mEngine;
};

} // namespace ableton::linkaudio
