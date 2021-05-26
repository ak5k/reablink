/* Copyright 2016, Ableton AG, Berlin. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  If you would like to incorporate Link into a proprietary software
 * application, please contact <link-devs@ableton.com>.
 */

#pragma once

// Make sure to define this before <cmath> is included for Windows
#ifdef _WIN32
#define WIN32_MEAN_AND_LEAN
#define _USE_MATH_DEFINES
#define ASIO_NO_EXCEPTIONS
#endif

#include <ableton/Link.hpp>
#include <ableton/link/HostTimeFilter.hpp>
#include <atomic>
#include <cmath>
#include <mutex>

#ifdef ASIO_NO_EXCEPTIONS
template <typename Exception>
void asio::detail::throw_exception(const Exception& e)
{
    // try
    // {
    //     throw e;
    // }
    // catch (const std::exception& e)
    // {
    //     std::cerr << e.what() << '\n';
    // }
    (void)e;
    return;
}
#endif

#include <reaper_plugin_functions.h>

class BlinkEngine
{
public:
    BlinkEngine(BlinkEngine const&) = delete;
    BlinkEngine& operator=(BlinkEngine&&) = delete;
    BlinkEngine& operator=(BlinkEngine const&) = delete;
    ~BlinkEngine();
    static BlinkEngine* GetInstance();

    ableton::Link& GetLink();
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
    struct EngineData
    {
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
    ableton::Link link;
    audio_hook_register_t audioHook;
    bool isFrameCountDown;
    bool isPlaying;
    bool syncCorrection;
    bool tempoPush;
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