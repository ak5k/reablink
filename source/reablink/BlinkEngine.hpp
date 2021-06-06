#ifndef BLINKENGINE_HPP
#define BLINKENGINE_HPP

#ifdef WIN32
// Make sure to define this before <cmath> is included for Windows
#define _USE_MATH_DEFINES
/* #define htonll(x) \
    ((1 == htonl(1))                                                           \
         ? (x)                                                                 \
         : ((uint64_t)htonl((x)&0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x)                                                              \
    ((1 == ntohl(1))                                                           \
         ? (x)                                                                 \
         : ((uint64_t)ntohl((x)&0xFFFFFFFF) << 32) | ntohl((x) >> 32)) */

#define ASIO_NO_EXCEPTIONS
#endif

#include <ableton/Link.hpp>
// #include <ableton/link/HostTimeFilter.hpp>
#include <cmath>
#include <iostream>
#include <mutex>

#ifdef WIN32
#ifdef NDEBUG
template <typename Exception>
void asio::detail::throw_exception(const Exception& e)
{
    (void)e;
    return;
}
#else
template <typename Exception>
void asio::detail::throw_exception(const Exception& e)
{
    try {
        throw e;
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}
#endif
#endif

#ifdef NDEBUG
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
    static void OnAudioBuffer(
        bool isPost,
        int len,
        double srate,
        audio_hook_register_t* reg);
    void SetMaster(bool enable);
    void SetPuppet(bool enable);
    void SetQuantum(double setQuantum);
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
    static void TempoCallback(double bpm);

    audio_hook_register_t audioHook;
    int64_t frameCountDown;
    int frameSize;
    std::chrono::microseconds frameTime;
    bool isPlaying;
    std::chrono::microseconds outputLatency;
    size_t playbackFrameCount;
    double qnAbs;
    double qnJumpOffset;
    double qnLandOffset;
    double samplePosition;
    double sampleRate;
    bool syncCorrection;
    EngineData sharedEngineData;
    EngineData lockfreeEngineData;

    static constexpr auto beatTolerance = 0.02;
    static constexpr auto playbackFrameSafe = 16;
    static constexpr auto syncTolerance = 3;
    static constexpr auto tempoTolerance = 0.005;

    std::mutex m;

    // #ifdef WIN32
    //     ableton::link::HostTimeFilter<ableton::platforms::windows::Clock>
    //         hostTimeFilter;
    // #else
    //     ableton::link::HostTimeFilter<ableton::link::platform::Clock>
    //         hostTimeFilter;
    // #endif
};

#endif
