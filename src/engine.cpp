// Make sure to define this before <cmath> is included for Windows
#ifdef LINK_PLATFORM_WINDOWS
#define _USE_MATH_DEFINES // NOLINT
#endif

#include "engine.hpp"
#include "global_vars.hpp"
#include <algorithm>
#include <vector>

#include <reaper_plugin_functions.h>

namespace ableton::linkaudio
{

constexpr int TIMER_INTERVALS_SIZE = 512;

AudioEngine::AudioEngine(Link& link)
    : mLink(link)
    , mSharedEngineData({0., false, false, 4., false})
    , mLockfreeEngineData(mSharedEngineData)
    , mIsPlaying(false)
{
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
    auto engineData = EngineData {};
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

void AudioEngine::audioCallback(const std::chrono::microseconds hostTime,
                                const std::size_t numSamples)
{
    (void)numSamples; // unused

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
            auto temp =
                timer_intervals.at(count + (TIMER_INTERVALS_SIZE / 4) - 1);
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
    auto cpos = GetCursorPosition();
    // auto pos = GetPlayPosition(); // + frameTime.count() / 1.0e6; // =
    // hosttime
    auto pos2 =
        GetPlayPosition2(); // + frameTime.count() / 1.0e6; // = hosttime

    // find current tempo and time sig
    // and active time sig marker
    bool lineartempo {false};
    double beatpos {0};
    double timepos {0};
    double hostBpm {0};
    int measurepos {0};
    int timesig_num {0};
    int timesig_denom {0};
    int ptidx {0};
    TimeMap_GetTimeSigAtTime(0, pos2, &timesig_num, &timesig_denom, &hostBpm);
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
    static int frame_count {0};
    double wait_time {0};
    if (!mIsPlaying && sessionState.isPlaying())
    {
        Undo_BeginBlock();
        // Reset the timeline so that beat 0 corresponds to the time when
        // transport starts
        // 'quantized launch' madness
        int ms {0};
        int ms_len {0};
        (void)TimeMap2_timeToBeats(0, cpos, &ms, &ms_len, 0, 0);
        timepos = TimeMap2_beatsToTime(0, ms_len, &ms);
        TimeMap_GetTimeSigAtTime(0, timepos, &timesig_num, &timesig_denom,
                                 &hostBpm);
        sessionState.setTempo(hostBpm, hostTime);
        ptidx = FindTempoTimeSigMarker(0, timepos);
        double tempBpm {0};
        if (GetTempoTimeSigMarker(0, ptidx - 1, &timepos, &measurepos, &beatpos,
                                  &tempBpm, &timesig_num, &timesig_denom,
                                  &lineartempo))
        {
            SetTempoTimeSigMarker(0, ptidx - 1, timepos, measurepos, beatpos,
                                  hostBpm, timesig_num, timesig_denom,
                                  lineartempo);
            GetTempoTimeSigMarker(0, ptidx, &timepos, &measurepos, &beatpos, 0,
                                  &timesig_num, &timesig_denom, &lineartempo);
            // timepos = timepos;
        }

        sessionState.requestBeatAtStartPlayingTime(0, engineData.quantum);
        wait_time = ( //
                        sessionState.timeAtBeat(0.00000001, engineData.quantum)
                            .count() -
                        hostTime.count()) /
                    1.0e6;
        frame_count = wait_time / frame_time;
        auto frame_wait_time = frame_count * frame_time;
        auto offset = frame_wait_time - wait_time;
        timepos = timepos + offset; // probably negative offset
        auto buffer_count =
            (int)(frame_wait_time / (g_abuf_len / g_abuf_srate));
        offset = buffer_count * (g_abuf_len / g_abuf_srate) - frame_wait_time;
        timepos = timepos + offset; // probably negative offset
        frame_count = frame_count > 0 ? frame_count : 1;

        OnStopButton();
        SetEditCurPos(timepos, false, false);
        mIsPlaying = true;
    }
    else if (mIsPlaying && !sessionState.isPlaying())
    {
        OnStopButton();
        mIsPlaying = false;
        Undo_EndBlock("ReaBlink", -1);
    }

    // playback countdown and launch
    if (mIsPlaying && sessionState.isPlaying() && frame_count > 0)
    {
        frame_count--;
        if (frame_count == 0)
        {
            OnPlayButton();
        }
    }

    if (engineData.requestedTempo > 0)
    {
        // Set the newly requested tempo from the beginning of this buffer
        sessionState.setTempo(engineData.requestedTempo, hostTime);
    }

    // Timeline modifications are complete, commit the results
    mLink.commitAudioSessionState(sessionState);

    if (mIsPlaying)
    {
        // As long as the engine is playing, generate metronome clicks in
        // the buffer at the appropriate beats.
    }
}

} // namespace ableton::linkaudio