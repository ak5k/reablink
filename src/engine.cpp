#include "engine.hpp"

// Make sure to define this before <cmath> is included for Windows
#ifdef LINK_PLATFORM_WINDOWS
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#include <iostream>

#include <reaper_plugin_functions.h>

namespace ableton::linkaudio
{
    AudioEngine::AudioEngine(Link &link)
        : mLink(link), mOutputLatency(std::chrono::microseconds{0}), mSharedEngineData({0., false, false, 4., false}), mLockfreeEngineData(mSharedEngineData), mTimeAtLastClick{}, mIsPlaying(false)
    {
        if (!mOutputLatency.is_lock_free())
        {
            std::cout << "WARNING: AudioEngine::mOutputLatency is not lock free!"
                      << std::endl;
        }
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

    void AudioEngine::audioCallback(const std::chrono::microseconds hostTime,
                                    const std::size_t numSamples)
    {
        (void)numSamples; // unused
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

        if (!mIsPlaying && sessionState.isPlaying())
        {
            // Reset the timeline so that beat 0 corresponds to the time when
            // transport starts
            sessionState.requestBeatAtStartPlayingTime(0, engineData.quantum);
            mIsPlaying = true;
        }
        else if (mIsPlaying && !sessionState.isPlaying())
        {
            mIsPlaying = false;
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
