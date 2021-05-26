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

#include "BlinkEngine.hpp"

BlinkEngine::BlinkEngine()
    : audioHook {OnAudioBuffer, 0, 0, 0, 0}
    , beatOffset {0}
    , frameCountDown {0}
    , frameSize {0}
    , frameTime(std::chrono::microseconds {0})
    , isFrameCountDown {false}
    , isPlaying {false}
    , link {Master_GetTempo()}
    , outputLatency {std::chrono::microseconds {0}}
    , playbackFrameCount {0}
    , qnAbs {0}
    , qnJumpOffset {0}
    , qnLandOffset {0}
    , samplePosition {0}
    , sampleRate {0}
    , syncCorrection {false}
    , tempoPush {false}
    , sharedEngineData {0., false, false, 4., false, false, false}
    , lockfreeEngineData {sharedEngineData}
{
    link.setTempoCallback(TempoCallback);
}

BlinkEngine::~BlinkEngine()
{
    Audio_RegHardwareHook(false, &audioHook);
    link.enable(false);
    return;
}

BlinkEngine* BlinkEngine::GetInstance()
{
    static auto* blinkEngine = new BlinkEngine();
    return blinkEngine;
}

void BlinkEngine::Initialize(bool enable)
{
    Audio_RegHardwareHook(enable, &audioHook);
}

ableton::Link& BlinkEngine::GetLink()
{
    return link;
}

void BlinkEngine::StartPlaying()
{
    std::scoped_lock<std::mutex> lck(m);
    sharedEngineData.requestStart = true;
}

void BlinkEngine::StopPlaying()
{
    std::scoped_lock<std::mutex> lck(m);
    sharedEngineData.requestStop = true;
}

bool BlinkEngine::GetPlaying() const
{
    return link.captureAppSessionState().isPlaying();
}

void BlinkEngine::SetTempo(double tempo)
{
    std::scoped_lock<std::mutex> lck(m);
    sharedEngineData.requestedTempo = tempo;
}

double BlinkEngine::GetQuantum() const
{
    return sharedEngineData.quantum;
}

void BlinkEngine::SetQuantum(double quantum)
{
    std::scoped_lock<std::mutex> lck(m);
    sharedEngineData.quantum = quantum;
}

void BlinkEngine::SetMaster(bool enable)
{
    std::scoped_lock<std::mutex> lck(m);
    sharedEngineData.isMaster = enable;
}

void BlinkEngine::SetPuppet(bool enable)
{
    std::scoped_lock<std::mutex> lck(m);
    sharedEngineData.isPuppet = enable;
}

bool BlinkEngine::GetMaster() const
{
    return sharedEngineData.isMaster;
}

bool BlinkEngine::GetPuppet() const
{
    return sharedEngineData.isPuppet;
}

bool BlinkEngine::GetStartStopSyncEnabled() const
{
    return link.isStartStopSyncEnabled();
}

void BlinkEngine::SetStartStopSyncEnabled(const bool enabled)
{
    link.enableStartStopSync(enabled);
}

void BlinkEngine::TempoCallback(double bpm)
{
    BlinkEngine* blinkEngine = GetInstance();
    if (blinkEngine->sharedEngineData.isPuppet)
    {
        blinkEngine->SetTempo(bpm);
    }
    return;
}

BlinkEngine::EngineData BlinkEngine::PullEngineData()
{
    auto engineData = EngineData {};
    if (m.try_lock())
    {
        engineData.requestedTempo = sharedEngineData.requestedTempo;
        sharedEngineData.requestedTempo = 0;
        engineData.requestStart = sharedEngineData.requestStart;
        sharedEngineData.requestStart = false;
        engineData.requestStop = sharedEngineData.requestStop;
        sharedEngineData.requestStop = false;

        lockfreeEngineData.isMaster = sharedEngineData.isMaster;
        lockfreeEngineData.isPuppet = sharedEngineData.isPuppet;
        lockfreeEngineData.quantum = sharedEngineData.quantum;
        lockfreeEngineData.startStopSyncOn = sharedEngineData.startStopSyncOn;

        m.unlock();
    }
    engineData.quantum = lockfreeEngineData.quantum;
    engineData.isMaster = lockfreeEngineData.isMaster;
    engineData.isPuppet = lockfreeEngineData.isPuppet;
    if (!engineData.isPuppet)
    {
        engineData.isMaster = false;
        SetMaster(false);
    }

    return engineData;
}

void BlinkEngine::OnAudioBuffer(
    bool isPost, int len, double srate, audio_hook_register_t* reg)
{
    (void)reg;
    BlinkEngine* blinkEngine = GetInstance();
    if (len != blinkEngine->frameSize || srate != blinkEngine->sampleRate)
    {
        blinkEngine->frameSize = len;
        blinkEngine->frameTime =
            std::chrono::microseconds(llround((len / srate) * 1.0e6));
        blinkEngine->mHostTimeFilter.reset();
        blinkEngine->outputLatency =
            std::chrono::microseconds(llround(GetOutputLatency() * 1.0e6));
        blinkEngine->samplePosition = len;
        blinkEngine->sampleRate = srate;
    }
    if (!isPost)
    {
        // does linear regression to generate timestamp based on sample position
        // advancement and current system time
        const auto hostTime =
            blinkEngine->mHostTimeFilter.sampleTimeToHostTime(
                blinkEngine->samplePosition)
            + blinkEngine->outputLatency // includes one frame time
            + blinkEngine->frameTime;

        blinkEngine->samplePosition += len; // advance sample position
        if (blinkEngine->GetLink().isEnabled())
        {
            blinkEngine->AudioCallback(hostTime);
        }
    }
    return;
} // called twice per frame, isPost being false then true

void BlinkEngine::AudioCallback(const std::chrono::microseconds& hostTime)
{
    // current link state
    const auto engineData = PullEngineData();
    auto sessionState = link.captureAudioSessionState();

    // reaper timeline positions
    const auto cpos = GetCursorPosition();
    auto pos = GetPlayPosition();                               // = now
    auto pos2 = GetPlayPosition2() + frameTime.count() / 1.0e6; // = hosttime

    // set undo if playing
    // otherwise position to cursor
    if ((GetPlayState() & 1) == 1 || (GetPlayState() & 4) == 4)
    {
        if (playbackFrameCount == 0)
        {
            std::thread(Undo_BeginBlock).detach();
        }
        playbackFrameCount++;
    }
    else
    {
        if (playbackFrameCount > 0)
        {
            std::thread(Undo_EndBlock, "ReaBlink", -1).detach();
        }
        pos = cpos;
        pos2 = cpos;
    }

    // find current tempo and time sig
    // and active time sig marker
    bool lineartempo {0};
    double beatpos {0}, quantumNorm {0}, timepos {0}, hostBpm {0};
    int measurepos {0}, timesig_num {0}, timesig_denom {0};
    TimeMap_GetTimeSigAtTime(0, pos2, &timesig_num, &timesig_denom, &hostBpm);
    auto ptidx = FindTempoTimeSigMarker(0, pos2);
    GetTempoTimeSigMarker(0, ptidx, &timepos, 0, 0, 0, 0, 0, 0);

    // set quantum
    // if (engineData.isPuppet)
    // {
    //     quantumNorm = timesig_num / (timesig_denom / 4.0);
    //     SetQuantum(quantumNorm); // set local quantum
    // }
    // else
    // {
    //     quantumNorm = engineData.quantum;
    // }

    if (engineData.requestStart)
    {
        sessionState.setIsPlaying(true, hostTime); // start link play
    }

    if (engineData.requestStop)
    {
        sessionState.setIsPlaying(false, hostTime); // stop link play
    }

    if (!isPlaying && sessionState.isPlaying())
    {
        // Reset the timeline so that beat 0 corresponds to the time
        // when transport starts
        frameCountDown = 1;
        std::thread(OnStopButton).detach();
        if (link.numPeers() > 0 && engineData.isPuppet)
        {
            // 'quantized launch' madness
            int ms {0}, ms_len {0};
            (void)TimeMap2_timeToBeats(0, cpos, &ms, &ms_len, 0, 0);
            timepos = TimeMap2_beatsToTime(0, ms_len, &ms);
            SetEditCurPos(timepos, false, false);
            TimeMap_GetTimeSigAtTime(
                0, timepos, &timesig_num, &timesig_denom, &hostBpm);
            sessionState.setTempo(hostBpm, hostTime);
            ptidx = FindTempoTimeSigMarker(0, timepos);
            double tempBpm {0};
            if (GetTempoTimeSigMarker(0, ptidx - 1, &timepos, &measurepos,
                    &beatpos, &tempBpm, &timesig_num, &timesig_denom,
                    &lineartempo))
            {
                SetTempoTimeSigMarker(0, ptidx - 1, timepos, measurepos,
                    beatpos, hostBpm, timesig_num, timesig_denom, lineartempo);
                GetTempoTimeSigMarker(0, ptidx, &timepos, &measurepos, &beatpos,
                    0, &timesig_num, &timesig_denom, &lineartempo);
                timepos = timepos;
            }
            sessionState.requestBeatAtStartPlayingTime(0, engineData.quantum);
            frameCountDown =
                (sessionState.timeAtBeat(0, engineData.quantum) - hostTime)
                / frameTime;
            auto host_start_time =
                hostTime
                + std::chrono::duration_cast<std::chrono::microseconds>(
                    frameTime * frameCountDown);
            auto session_start_time =
                sessionState.timeAtBeat(0, engineData.quantum);
            double startOffset =
                (host_start_time.count() - session_start_time.count()) / 1.0e6;
            frameCountDown++;
            timepos += startOffset;
            std::thread(SetEditCurPos, timepos, false, true).detach();
        }
        else if (link.numPeers() == 0 && engineData.isPuppet)
        {
            const auto startBeat = fmod(TimeMap_timeToQN_abs(0, cpos), 1.);
            sessionState.forceBeatAtTime(startBeat, hostTime, 1.);
        }
        isPlaying = true;
    }
    else if (isPlaying && !sessionState.isPlaying())
    {
        isPlaying = false;
        // stop reaper
        if (engineData.isPuppet)
        {
            playbackFrameCount = 0;
            qnAbs = 0.;
            qnJumpOffset = 0.;
            qnLandOffset = 0.;
            syncCorrection = false;
            std::thread(OnStopButton).detach();
        }
    }

    // playback countdown and launch
    if (isPlaying && sessionState.isPlaying() && engineData.isPuppet
        && frameCountDown > 0)
    {
        frameCountDown--;
        if (frameCountDown == 0)
        {
            std::thread(OnPlayButton).detach();
        }
    }

    // set tempo
    if (engineData.requestedTempo > 0)
    {
        // set REAPER if puppet
        if (engineData.isPuppet)
        {
            tempoPush =
                true; // prevent responding to callback from link session
            // set tempo to current active tempo time sig marker
            std::thread([this, engineData, ptidx, timepos, measurepos, beatpos,
                            timesig_num, timesig_denom, lineartempo]() {
                if (!SetTempoTimeSigMarker(0, ptidx, timepos, measurepos,
                        beatpos, round(engineData.requestedTempo * 100) / 100,
                        timesig_num, timesig_denom, lineartempo))
                {
                    SetTempo(engineData.requestedTempo);
                }
                else
                {
                    UpdateTimeline();
                }
            }).detach();
        }
        // set tempo to link session
        sessionState.setTempo(engineData.requestedTempo, hostTime);
    }

    // get current qn position
    // set offsets if loop or jump
    const auto currentQN = TimeMap_timeToQN_abs(0, pos2);
    if (isPlaying && playbackFrameCount > playbackFrameSafe
        && (currentQN < qnAbs || abs(currentQN - qnAbs) > 1.))
    {
        qnJumpOffset = sessionState.beatAtTime(hostTime, 1.);
        qnLandOffset = fmod(currentQN, 1.0);
    }
    qnAbs = currentQN;
    double currentHostBpm {0};
    TimeMap_GetTimeSigAtTime(
        0, pos2 - frameTime.count() / 1.0e6, 0, 0, &currentHostBpm);

    const auto hostBeat =
        fmod(abs(fmod(qnAbs, 1.0) - qnLandOffset + qnJumpOffset), 1.0);

    const auto sessionBeat = sessionState.phaseAtTime(hostTime, 1.);

    const auto sessionBpm =
        sessionState.tempo(); // maybe Master has modified sessionState

    const auto hostBeatTime = hostBeat * 60. / (hostBpm);
    const auto sessionBeatTime = sessionBeat * 60. / (sessionBpm);
    const auto diff = round(abs(hostBeatTime - sessionBeatTime) * 1.0e3);
    const auto qLen = floor((60. / sessionBpm) * 1.0e3);

    // REAPER is master, unless user has requested tempo change
    if (engineData.isMaster && !(engineData.requestedTempo > 0))
    {
        // try to improve sync if difference greater than 3 ms by forcing
        // local timeline upon peers
        if (sessionState.isPlaying() && playbackFrameCount > playbackFrameSafe
            && diff > 3.0
            && diff < qLen - ceil((frameTime.count() / 1.0e3) * 2))
        {
            double pushBeat {0};
            pushBeat = fmod(abs(fmod(TimeMap_timeToQN_abs(0, pos2), 1.)
                                - qnLandOffset + qnJumpOffset),
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
    if (engineData.isPuppet && !(engineData.requestedTempo > 0))
    {
        if (abs(currentHostBpm - sessionBpm) > tempoTolerance
            && (isPlaying || link.numPeers() == 0))
        {
            if (GetTempoTimeSigMarker(0, ptidx, &timepos, 0, 0, 0, 0, 0, 0))
            {
                double pushBeat {0};
                pushBeat = fmod(abs(fmod(TimeMap_timeToQN_abs(0, timepos), 1.)
                                    - qnLandOffset + qnJumpOffset),
                    1.);
                if (abs(pushBeat - 1.) < beatTolerance)
                {
                    pushBeat = 0.;
                }
                pushBeat += floor(sessionState.beatAtTime(hostTime, 1.));
                sessionState.setTempo(
                    currentHostBpm, sessionState.timeAtBeat(pushBeat, 1.));
            }
        }
        // try to improve sync with playrate change as long as difference is
        // greater than 3 ms
        else if (!engineData.isMaster && sessionState.isPlaying()
                 && playbackFrameCount > playbackFrameSafe && diff > 3.0
                 && diff < qLen - ceil((frameTime.count() / 1.0e3) * 2))
        {
            syncCorrection = true;
            if (hostBeat - sessionBeat > 0)
            {
                // slow_down
                std::thread([]() { Main_OnCommand(40525, 0); }).detach();
            }
            else
            {
                std::thread([]() { Main_OnCommand(40524, 0); }).detach();
            }
        }
        // reset playback rate if sync corrected
        else if (syncCorrection)
        {
            std::thread(Main_OnCommand, 40521, 0).detach();
            syncCorrection = false;
        }
    }

    // Timeline modifications are complete, commit the results
    link.commitAudioSessionState(sessionState);
}