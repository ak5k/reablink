#pragma once

#ifdef _WIN32
// Make sure to define this before <cmath> is included for Windows
#define _USE_MATH_DEFINES
#define WIN32_MEAN_AND_LEAN
#define ASIO_NO_EXCEPTIONS
#endif

#include <ableton/Link.hpp>
#include <ableton/link/HostTimeFilter.hpp>
#include <cmath>
#include <mutex>

#ifndef NDEBUG
#define REAPERAPI_MINIMAL
#define REAPERAPI_WANT_Audio_RegHardwareHook
#define REAPERAPI_WANT_FindTempoTimeSigMarker
#define REAPERAPI_WANT_GetCursorPosition
#define REAPERAPI_WANT_GetOutputLatency
#define REAPERAPI_WANT_GetPlayPosition
#define REAPERAPI_WANT_GetPlayPosition2
#define REAPERAPI_WANT_GetPlayState
#define REAPERAPI_WANT_GetTempoTimeSigMarker
#define REAPERAPI_WANT_Main_OnCommand
#define REAPERAPI_WANT_Master_GetTempo
#define REAPERAPI_WANT_OnPlayButton
#define REAPERAPI_WANT_OnStopButton
#define REAPERAPI_WANT_SetEditCurPos
#define REAPERAPI_WANT_SetTempoTimeSigMarker
#define REAPERAPI_WANT_TimeMap2_beatsToTime
#define REAPERAPI_WANT_TimeMap2_timeToBeats
#define REAPERAPI_WANT_TimeMap_GetTimeSigAtTime
#define REAPERAPI_WANT_TimeMap_timeToQN_abs
#define REAPERAPI_WANT_Undo_BeginBlock
#define REAPERAPI_WANT_Undo_EndBlock
#define REAPERAPI_WANT_UpdateTimeline
#define REAPERAPI_WANT_plugin_register
#endif

#include <reaper_plugin_functions.h>

#ifdef ASIO_NO_EXCEPTIONS
template <typename Exception>
void asio::detail::throw_exception(const Exception& e)
{
    try {
        throw e;
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
    // (void)e;
    return;
}
#endif

class BlinkEngine {
  public:
    static BlinkEngine& GetInstance();

    BlinkEngine(BlinkEngine const&) = delete;
    BlinkEngine(BlinkEngine&&) = delete;
    BlinkEngine& operator=(BlinkEngine const&) = delete;
    BlinkEngine& operator=(BlinkEngine&&) = delete;

    ableton::Link& GetLink() const;
    bool GetMaster() const;
    bool GetPlaying() const;
    bool GetPuppet() const;
    bool GetStartStopSyncEnabled() const;
    double GetQuantum() const;
    void AudioCallback(const std::chrono::microseconds& hostTime);
    void Initialize(bool enable);
    void SetMaster(bool enable);
    void SetPuppet(bool enable);
    void SetQuantum(double quantum);
    void SetStartStopSyncEnabled(bool enabled);
    void SetTempo(double tempo);
    void StartPlaying();
    void StopPlaying();

  private:
    BlinkEngine();
    struct EngineData {
        double requestedTempo;
        bool requestStart;
        bool requestStop;
        double quantum;
        bool startStopSyncOn;
        bool isPuppet;
        bool isMaster;
    };

    EngineData PullEngineData();
    static void OnAudioBuffer(
        bool isPost, int len, double srate, audio_hook_register_t* reg);
    static void TempoCallback(double bpm);

    EngineData lockfreeEngineData;
    EngineData sharedEngineData;
    audio_hook_register_t audioHook;
    bool isFrameCountDown;
    bool isPlaying;
    bool syncCorrection;
    double beatOffset;
    double qnAbs;
    double qnJumpOffset;
    double qnLandOffset;
    double quantum;
    double samplePosition;
    double sampleRate;
    int64_t frameCountDown;
    size_t frameSize;
    size_t playbackFrameCount;
    static constexpr auto beatTolerance = 0.01;
    static constexpr auto playbackFrameSafe = 16;
    static constexpr auto tempoTolerance = 0.005;
    std::chrono::microseconds frameTime;
    std::chrono::microseconds outputLatency;
    std::mutex m;

#ifdef _WIN32
    ableton::link::HostTimeFilter<ableton::platforms::windows::Clock>
        mHostTimeFilter;
#else
    ableton::link::HostTimeFilter<ableton::link::platform::Clock>
        mHostTimeFilter;
#endif
};
