#include "ReaBlink.hpp"
#include "BlinkEngine.hpp"
#include <reaper_plugin_functions.h>
#include <reascript_vararg.hpp>

BlinkEngine* blinkEngine;

typedef std::chrono::microseconds usec;
typedef std::chrono::duration<double> dsec;

std::chrono::microseconds doubleToMicros(double time)
{
    return std::chrono::microseconds(llround(time * 1.0e6));
}

double microsToDouble(std::chrono::microseconds time)
{
    return std::chrono::duration<double>(time).count();
}

/*! @brief Enable/disable Link.
 *  Thread-safe: yes
 *  Realtime-safe: no
 */
const char* defstring_SetEnabled =
    "void\0bool\0enable\0"
    "Enable/disable Blink.";
void SetEnabled(bool enable)
{
    if (enable && !blinkEngine->GetLink().isEnabled())
    {
        blinkEngine->GetLink().enable(true);
    }
    else if (!enable && blinkEngine->GetLink().isEnabled())
    {
        blinkEngine->GetLink().enable(false);
    }
    else
    {
        blinkEngine->GetLink().enable(false);
    }
    return;
}

/*! @brief Is Link currently enabled?
 *  Thread-safe: yes
 *  Realtime-safe: yes
 */
const char* defstring_GetEnabled =
    "bool\0\0\0"
    "Is Blink currently enabled?";
bool GetEnabled()
{
    return blinkEngine->GetLink().isEnabled();
}

/*! @brief: Is start/stop synchronization enabled?
 *  Thread-safe: yes
 *  Realtime-safe: no
 */
const char* defstring_GetStartStopSyncEnabled =
    "bool\0\0\0"
    "Is start/stop synchronization enabled?";
bool GetStartStopSyncEnabled()
{
    return blinkEngine->GetStartStopSyncEnabled();
}

/*! @brief: Enable start/stop synchronization.
 *  Thread-safe: yes
 *  Realtime-safe: no
 */
const char* defstring_SetStartStopSyncEnabled =
    "void\0bool\0enable\0"
    "Enable start/stop synchronization.";
void SetStartStopSyncEnabled(bool enable)
{
    blinkEngine->SetStartStopSyncEnabled(enable);
    return;
}

/*! @brief How many peers are currently connected in a Link session?
 *  Thread-safe: yes
 *  Realtime-safe: yes
 */
const char* defstring_GetNumPeers =
    "int\0\0\0"
    "How many peers are currently connected in Link session?";
int GetNumPeers()
{
    return (int)blinkEngine->GetLink().numPeers();
}

/*! @brief The clock used by Link.
 *  Thread-safe: yes
 *  Realtime-safe: yes
 *
 *  @discussion The Clock type is a platform-dependent representation
 *  of the system clock. It exposes a micros() method, which is a
 *  normalized representation of the current system time in
 *  std::chrono::microseconds.
 */

const char* defstring_GetClockNow =
    "double\0\0\0"
    "Clock used by Blink.";
double GetClockNow()
{
    return blinkEngine->GetLink().clock().micros().count() / 1.0e6;
}

/*! @brief: The tempo of the timeline, in Beats Per Minute.
 *
 *  @discussion This is a stable value that is appropriate for display
 *  to the user. Beat time progress will not necessarily match this tempo
 *  exactly because of clock drift compensation.
 */
const char* defstring_GetTempo =
    "double\0\0\0"
    "Tempo of session timeline, in Beats Per Minute.";
double GetTempo()
{
    return blinkEngine->GetLink().captureAppSessionState().tempo();
}

/*! @brief: Get the beat value corresponding to the given time
 *  for the given quantum.
 *
 *  @discussion: The magnitude of the resulting beat value is
 *  unique to this Link instance, but its phase with respect to
 *  the provided quantum is shared among all session
 *  peers. For non-negative beat values, the following
 *  property holds: fmod(beatAtTime(t, q), q) == phaseAtTime(t, q)
 */
const char* defstring_GetBeatAtTime =
    "double\0double,double\0time,quantum\0"
    "Get session beat value corresponding to given time for given quantum.";
double GetBeatAtTime(double time, double quantum)
{
    return blinkEngine->GetLink().captureAppSessionState().beatAtTime(
        doubleToMicros(time), quantum);
}

/*! @brief: Get the session phase at the given time for the given
 *  quantum.
 *
 *  @discussion: The result is in the interval [0, quantum). The
 *  result is equivalent to fmod(beatAtTime(t, q), q) for
 *  non-negative beat values. This method is convenient if the
 *  client is only interested in the phase and not the beat
 *  magnitude. Also, unlike fmod, it handles negative beat values
 *  correctly.
 */
const char* defstring_GetPhaseAtTime =
    "double\0double,double\0time,quantum\0"
    "Get session phase at given time for given quantum since beat 0.";
double GetPhaseAtTime(double time, double quantum)
{
    return blinkEngine->GetLink().captureAppSessionState().phaseAtTime(
        doubleToMicros(time), quantum);
}

/*! @brief: Get the time at which the given beat occurs for the
 *  given quantum.
 *
 *  @discussion: The inverse of beatAtTime, assuming a constant
 *  tempo. beatAtTime(timeAtBeat(b, q), q) === b.
 */
const char* defstring_GetTimeAtBeat =
    "double\0double,double\0beat,quantum\0"
    "Get time at which given beat occurs for given quantum.";
double GetTimeAtBeat(double beat, double quantum)
{
    return microsToDouble(
        blinkEngine->GetLink().captureAppSessionState().timeAtBeat(
            beat, quantum));
}

/*! @brief: Is transport playing? */
const char* defstring_GetPlaying =
    "bool\0\0\0"
    "Is transport playing?";
bool GetPlaying()
{
    return blinkEngine->GetLink().captureAppSessionState().isPlaying();
}

const char* defstring_SetPlaying =
    "void\0bool,double\0playing,time\0"
    "Set if transport should be playing or stopped, taking effect at "
    "given time.";
void SetPlaying(bool playing, double time)
{
    auto sessionState = blinkEngine->GetLink().captureAppSessionState();
    sessionState.setIsPlaying(playing, doubleToMicros(time));
    blinkEngine->GetLink().commitAppSessionState(sessionState);
    return;
}

const char* defstring_SetPlayingAndBeatAtTimeRequest =
    "void\0bool,double,double,double\0playing,time,beat,quantum\0"
    "Convenience function to start or stop transport at given time and attempt "
    "to map given beat to this time in context of given quantum.";
void SetPlayingAndBeatAtTimeRequest(
    bool playing, double time, double beat, double quantum)
{
    auto sessionState = blinkEngine->GetLink().captureAppSessionState();
    sessionState.setIsPlayingAndRequestBeatAtTime(
        playing, doubleToMicros(time), beat, quantum);
    blinkEngine->GetLink().commitAppSessionState(sessionState);
    return;
}

/*! @brief: Get the time at which a transport start/stop occurs */
const char* defstring_GetTimeForPlaying =
    "double\0\0\0"
    "Get time at which transport start/stop occurs.";
double GetTimeForPlaying()
{
    return microsToDouble(
        blinkEngine->GetLink().captureAppSessionState().timeForIsPlaying());
}

const char* defstring_startStop =
    "void\0\0\0"
    "Transport start/stop.";
void startStop()
{
    auto sessionState = blinkEngine->GetLink().captureAppSessionState();
    if (sessionState.isPlaying())
    {
        sessionState.setIsPlaying(
            false, blinkEngine->GetLink().clock().micros());
    }
    else
    {
        sessionState.setIsPlaying(
            true, blinkEngine->GetLink().clock().micros());
    }
    blinkEngine->GetLink().commitAppSessionState(sessionState);
    return;
}

const char* defstring_SetTempo =
    "void\0double\0bpm\0"
    "Set tempo to given bpm value.";
void SetTempo(double bpm)
{
    blinkEngine->SetTempo(bpm);
    return;
}

const char* defstring_SetBeatAtTimeRequest =
    "void\0double,double,double\0bpm,time,quantum\0"
    "Attempt to map given beat to given time in context of given quantum.";
void SetBeatAtTimeRequest(double beat, double time, double quantum)
{
    auto sessionState = blinkEngine->GetLink().captureAppSessionState();
    sessionState.requestBeatAtTime(beat, doubleToMicros(time), quantum);
    blinkEngine->GetLink().commitAppSessionState(sessionState);
    return;
}

const char* defstring_SetBeatAtTimeForce =
    "void\0double,double,double\0bpm,time,quantum\0"
    "Rudely re-map the beat/time relationship for all peers in Link session.";
void SetBeatAtTimeForce(double beat, double time, double quantum)
{
    auto sessionState = blinkEngine->GetLink().captureAppSessionState();
    sessionState.forceBeatAtTime(beat, doubleToMicros(time), quantum);
    blinkEngine->GetLink().commitAppSessionState(sessionState);
    return;
}

const char* defstring_SetTempoAtTime =
    "void\0double,double\0bpm,time\0"
    "Set tempo to given bpm value, taking effect at given time.";
void SetTempoAtTime(double bpm, double time)
{
    auto sessionState = blinkEngine->GetLink().captureAppSessionState();
    sessionState.setTempo(bpm, doubleToMicros(time));
    blinkEngine->GetLink().commitAppSessionState(sessionState);
    return;
}

const char* defstring_SetQuantum =
    "void\0double\0quantum\0"
    "Set quantum. Usually this is set to length of one measure/bar in "
    "quarter notes.";
void SetQuantum(double quantum)
{
    blinkEngine->SetQuantum(quantum);
    return;
}

const char* defstring_GetQuantum =
    "double\0\0\0"
    "Get quantum.";
double GetQuantum()
{
    return blinkEngine->GetQuantum();
}

const char* defstring_SetMaster =
    "void\0bool\0enable\0"
    "Set Blink as Master. Puppet needs to be enabled first. Same as Puppet, "
    "but possible beat offset is broadcast to Link session, effectively "
    "forcing local REAPER timeline on peers. Only one, if any, Blink should be "
    "Master in Link session.";
void SetMaster(bool enable)
{
    return blinkEngine->SetMaster(enable);
}

const char* defstring_GetMaster =
    "bool\0\0\0"
    "Is Blink Master?";
bool GetMaster()
{
    return blinkEngine->GetMaster();
}

const char* defstring_SetPuppet =
    "void\0bool\0enable\0"
    "Set Blink as Puppet. When enabled, Blink attempts to synchronize local "
    "REAPER tempo to Link session tempo by adjusting current active tempo time "
    "signature marker and to correct possible offset by adjusting playrate. "
    "Based on cumulative single beat phase since Link session playback beat 0, "
    "regardless of quantum.";
void SetPuppet(bool enable)
{
    blinkEngine->Initialize(enable);
    return blinkEngine->SetPuppet(enable);
}

const char* defstring_GetPuppet =
    "bool\0\0\0"
    "Is Blink Puppet?";
bool GetPuppet()
{
    return blinkEngine->GetPuppet();
}

void registerReaBlink()
{
    blinkEngine = blinkEngine->GetInstance();

    plugin_register("API_Blink_SetEnabled", (void*)SetEnabled);
    plugin_register("APIdef_Blink_SetEnabled", (void*)defstring_SetEnabled);
    plugin_register("APIvararg_Blink_SetEnabled",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetEnabled>));

    plugin_register("API_Blink_GetEnabled", (void*)GetEnabled);
    plugin_register("APIdef_Blink_GetEnabled", (void*)defstring_GetEnabled);
    plugin_register("APIvararg_Blink_GetEnabled",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetEnabled>));

    plugin_register("API_Blink_GetMaster", (void*)GetMaster);
    plugin_register("APIdef_Blink_GetMaster", (void*)defstring_GetMaster);
    plugin_register("APIvararg_Blink_GetMaster",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetMaster>));

    plugin_register("API_Blink_SetMaster", (void*)SetMaster);
    plugin_register("APIdef_Blink_SetMaster", (void*)defstring_SetMaster);
    plugin_register("APIvararg_Blink_SetMaster",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetMaster>));

    plugin_register("API_Blink_GetPuppet", (void*)GetPuppet);
    plugin_register("APIdef_Blink_GetPuppet", (void*)defstring_GetPuppet);
    plugin_register("APIvararg_Blink_GetPuppet",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetPuppet>));

    plugin_register("API_Blink_SetPuppet", (void*)SetPuppet);
    plugin_register("APIdef_Blink_SetPuppet", (void*)defstring_SetPuppet);
    plugin_register("APIvararg_Blink_SetPuppet",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetPuppet>));

    plugin_register(
        "API_Blink_GetStartStopSyncEnabled", (void*)GetStartStopSyncEnabled);
    plugin_register("APIdef_Blink_GetStartStopSyncEnabled",
        (void*)defstring_GetStartStopSyncEnabled);
    plugin_register("APIvararg_Blink_GetStartStopSyncEnabled",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetStartStopSyncEnabled>));

    plugin_register(
        "API_Blink_SetStartStopSyncEnabled", (void*)SetStartStopSyncEnabled);
    plugin_register("APIdef_Blink_SetStartStopSyncEnabled",
        (void*)defstring_SetStartStopSyncEnabled);
    plugin_register("APIvararg_Blink_SetStartStopSyncEnabled",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetStartStopSyncEnabled>));

    plugin_register("API_Blink_GetNumPeers", (void*)GetNumPeers);
    plugin_register("APIdef_Blink_GetNumPeers", (void*)defstring_GetNumPeers);
    plugin_register("APIvararg_Blink_GetNumPeers",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetNumPeers>));

    plugin_register("API_Blink_GetClockNow", (void*)GetClockNow);
    plugin_register("APIdef_Blink_GetClockNow", (void*)defstring_GetClockNow);
    plugin_register("APIvararg_Blink_GetClockNow",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetClockNow>));

    plugin_register("API_Blink_GetTempo", (void*)GetTempo);
    plugin_register("APIdef_Blink_GetTempo", (void*)defstring_GetTempo);
    plugin_register("APIvararg_Blink_GetTempo",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetTempo>));

    plugin_register("API_Blink_GetBeatAtTime", (void*)GetBeatAtTime);
    plugin_register(
        "APIdef_Blink_GetBeatAtTime", (void*)defstring_GetBeatAtTime);
    plugin_register("APIvararg_Blink_GetBeatAtTime",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetBeatAtTime>));

    plugin_register("API_Blink_GetPhaseAtTime", (void*)GetPhaseAtTime);
    plugin_register(
        "APIdef_Blink_GetPhaseAtTime", (void*)defstring_GetPhaseAtTime);
    plugin_register("APIvararg_Blink_GetPhaseAtTime",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetPhaseAtTime>));

    plugin_register("API_Blink_GetTimeAtBeat", (void*)GetTimeAtBeat);
    plugin_register(
        "APIdef_Blink_GetTimeAtBeat", (void*)defstring_GetTimeAtBeat);
    plugin_register("APIvararg_Blink_GetTimeAtBeat",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetTimeAtBeat>));

    plugin_register("API_Blink_GetTimeForPlaying", (void*)GetTimeForPlaying);
    plugin_register(
        "APIdef_Blink_GetTimeForPlaying", (void*)defstring_GetTimeForPlaying);
    plugin_register("APIvararg_Blink_GetTimeForPlaying",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetTimeForPlaying>));

    plugin_register("API_Blink_GetPlaying", (void*)GetPlaying);
    plugin_register("APIdef_Blink_GetPlaying", (void*)defstring_GetPlaying);
    plugin_register("APIvararg_Blink_GetPlaying",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetPlaying>));

    plugin_register("API_Blink_SetPlaying", (void*)SetPlaying);
    plugin_register("APIdef_Blink_SetPlaying", (void*)defstring_SetPlaying);
    plugin_register("APIvararg_Blink_SetPlaying",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetPlaying>));

    plugin_register("API_Blink_StartStop", (void*)startStop);
    plugin_register("APIdef_Blink_StartStop", (void*)defstring_startStop);
    plugin_register("APIvararg_Blink_StartStop",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&startStop>));

    plugin_register("API_Blink_SetTempo", (void*)SetTempo);
    plugin_register("APIdef_Blink_SetTempo", (void*)defstring_SetTempo);
    plugin_register("APIvararg_Blink_SetTempo",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetTempo>));

    plugin_register("API_Blink_SetTempoAtTime", (void*)SetTempoAtTime);
    plugin_register(
        "APIdef_Blink_SetTempoAtTime", (void*)defstring_SetTempoAtTime);
    plugin_register("APIvararg_Blink_SetTempoAtTime",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetTempoAtTime>));

    plugin_register(
        "API_Blink_SetBeatAtTimeRequest", (void*)SetBeatAtTimeRequest);
    plugin_register("APIdef_Blink_SetBeatAtTimeRequest",
        (void*)defstring_SetBeatAtTimeRequest);
    plugin_register("APIvararg_Blink_SetBeatAtTimeRequest",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetBeatAtTimeRequest>));

    plugin_register("API_Blink_SetQuantum", (void*)SetQuantum);
    plugin_register("APIdef_Blink_SetQuantum", (void*)defstring_SetQuantum);
    plugin_register("APIvararg_Blink_SetQuantum",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetQuantum>));

    plugin_register("API_Blink_GetQuantum", (void*)GetQuantum);
    plugin_register("APIdef_Blink_GetQuantum", (void*)defstring_GetQuantum);
    plugin_register("APIvararg_Blink_GetQuantum",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetQuantum>));

    plugin_register("API_Blink_SetBeatAtTimeForce", (void*)SetBeatAtTimeForce);
    plugin_register(
        "APIdef_Blink_SetBeatAtTimeForce", (void*)defstring_SetBeatAtTimeForce);
    plugin_register("APIvararg_Blink_SetBeatAtTimeForce",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetBeatAtTimeForce>));

    plugin_register("API_Blink_SetPlayingAndBeatAtTimeRequest",
        (void*)SetPlayingAndBeatAtTimeRequest);
    plugin_register("APIdef_Blink_SetPlayingAndBeatAtTimeRequest",
        (void*)defstring_SetPlayingAndBeatAtTimeRequest);
    plugin_register("APIvararg_Blink_SetPlayingAndBeatAtTimeRequest",
        reinterpret_cast<void*>(
            &InvokeReaScriptAPI<&SetPlayingAndBeatAtTimeRequest>));
}

void unregisterReaBlink()
{
    plugin_register("-API_Blink_SetPlayingAndBeatAtTimeRequest",
        (void*)SetPlayingAndBeatAtTimeRequest);
    plugin_register("-APIdef_Blink_SetPlayingAndBeatAtTimeRequest",
        (void*)defstring_SetPlayingAndBeatAtTimeRequest);
    plugin_register("-APIvararg_Blink_SetPlayingAndBeatAtTimeRequest",
        reinterpret_cast<void*>(
            &InvokeReaScriptAPI<&SetPlayingAndBeatAtTimeRequest>));

    plugin_register("-API_Blink_SetPlaying", (void*)SetPlaying);
    plugin_register("-APIdef_Blink_SetPlaying", (void*)defstring_SetPlaying);
    plugin_register("-APIvararg_Blink_SetPlaying",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetPlaying>));

    plugin_register(
        "-API_Blink_SetBeatAtTimeRequest", (void*)SetBeatAtTimeRequest);
    plugin_register("-APIdef_Blink_SetBeatAtTimeRequest",
        (void*)defstring_SetBeatAtTimeRequest);
    plugin_register("-APIvararg_Blink_SetBeatAtTimeRequest",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetBeatAtTimeRequest>));

    plugin_register("-API_Blink_SetBeatAtTimeForce", (void*)SetBeatAtTimeForce);
    plugin_register("-APIdef_Blink_SetBeatAtTimeForce",
        (void*)defstring_SetBeatAtTimeForce);
    plugin_register("-APIvararg_Blink_SetBeatAtTimeForce",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetBeatAtTimeForce>));

    plugin_register("-API_Blink_SetEnabled", (void*)SetEnabled);
    plugin_register("-APIdef_Blink_SetEnabled", (void*)defstring_SetEnabled);
    plugin_register("-APIvararg_Blink_SetEnabled",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetEnabled>));

    plugin_register("-API_Blink_GetEnabled", (void*)GetEnabled);
    plugin_register("-APIdef_Blink_GetEnabled", (void*)defstring_GetEnabled);
    plugin_register("-APIvararg_Blink_GetEnabled",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetEnabled>));

    plugin_register(
        "-API_Blink_GetStartStopSyncEnabled", (void*)GetStartStopSyncEnabled);
    plugin_register("-APIdef_Blink_GetStartStopSyncEnabled",
        (void*)defstring_GetStartStopSyncEnabled);
    plugin_register("-APIvararg_Blink_GetStartStopSyncEnabled",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetStartStopSyncEnabled>));

    plugin_register(
        "-API_Blink_SetStartStopSyncEnabled", (void*)SetStartStopSyncEnabled);
    plugin_register("-APIdef_Blink_SetStartStopSyncEnabled",
        (void*)defstring_SetStartStopSyncEnabled);
    plugin_register("-APIvararg_Blink_SetStartStopSyncEnabled",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetStartStopSyncEnabled>));

    plugin_register("-API_Blink_GetNumPeers", (void*)GetNumPeers);
    plugin_register("-APIdef_Blink_GetNumPeers", (void*)defstring_GetNumPeers);
    plugin_register("-APIvararg_Blink_GetNumPeers",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetNumPeers>));

    plugin_register("-API_Blink_GetClockNow", (void*)GetClockNow);
    plugin_register("-APIdef_Blink_GetClockNow", (void*)defstring_GetClockNow);
    plugin_register("-APIvararg_Blink_GetClockNow",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetClockNow>));

    plugin_register("-API_Blink_GetTempo", (void*)GetTempo);
    plugin_register("-APIdef_Blink_GetTempo", (void*)defstring_GetTempo);
    plugin_register("-APIvararg_Blink_GetTempo",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetTempo>));

    plugin_register("-API_Blink_GetBeatAtTime", (void*)GetBeatAtTime);
    plugin_register(
        "-APIdef_Blink_GetBeatAtTime", (void*)defstring_GetBeatAtTime);
    plugin_register("-APIvararg_Blink_GetBeatAtTime",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetBeatAtTime>));

    plugin_register("-API_Blink_GetPhaseAtTime", (void*)GetPhaseAtTime);
    plugin_register(
        "-APIdef_Blink_GetPhaseAtTime", (void*)defstring_GetPhaseAtTime);
    plugin_register("-APIvararg_Blink_GetPhaseAtTime",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetPhaseAtTime>));

    plugin_register("-API_Blink_GetTimeAtBeat", (void*)GetTimeAtBeat);
    plugin_register(
        "-APIdef_Blink_GetTimeAtBeat", (void*)defstring_GetTimeAtBeat);
    plugin_register("-APIvararg_Blink_GetTimeAtBeat",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetTimeAtBeat>));

    plugin_register("-API_Blink_GetTimeForPlaying", (void*)GetTimeForPlaying);
    plugin_register(
        "-APIdef_Blink_GetTimeForPlaying", (void*)defstring_GetTimeForPlaying);
    plugin_register("-APIvararg_Blink_GetTimeForPlaying",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetTimeForPlaying>));

    plugin_register("-API_Blink_GetPlaying", (void*)GetPlaying);
    plugin_register("-APIdef_Blink_GetPlaying", (void*)defstring_GetPlaying);
    plugin_register("-APIvararg_Blink_GetPlaying",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetPlaying>));

    plugin_register("-API_Blink_StartStop", (void*)startStop);
    plugin_register("-APIdef_Blink_StartStop", (void*)defstring_startStop);
    plugin_register("-APIvararg_Blink_StartStop",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&startStop>));

    plugin_register("-API_Blink_SetTempo", (void*)SetTempo);
    plugin_register("-APIdef_Blink_SetTempo", (void*)defstring_SetTempo);
    plugin_register("-APIvararg_Blink_SetTempo",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetTempo>));

    plugin_register("-API_Blink_SetQuantum", (void*)SetQuantum);
    plugin_register("-APIdef_Blink_SetQuantum", (void*)defstring_SetQuantum);
    plugin_register("-APIvararg_Blink_SetQuantum",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetQuantum>));

    plugin_register("-API_Blink_GetQuantum", (void*)GetQuantum);
    plugin_register("-APIdef_Blink_GetQuantum", (void*)defstring_GetQuantum);
    plugin_register("-APIvararg_Blink_GetQuantum",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetQuantum>));

    plugin_register("-API_Blink_GetMaster", (void*)GetMaster);
    plugin_register("-APIdef_Blink_GetMaster", (void*)defstring_GetMaster);
    plugin_register("-APIvararg_Blink_GetMaster",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetMaster>));

    plugin_register("-API_Blink_SetMaster", (void*)SetMaster);
    plugin_register("-APIdef_Blink_SetMaster", (void*)defstring_SetMaster);
    plugin_register("-APIvararg_Blink_SetMaster",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetMaster>));

    plugin_register("-API_Blink_GetPuppet", (void*)GetPuppet);
    plugin_register("-APIdef_Blink_GetPuppet", (void*)defstring_GetPuppet);
    plugin_register("-APIvararg_Blink_GetPuppet",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetPuppet>));

    plugin_register("-API_Blink_SetPuppet", (void*)SetPuppet);
    plugin_register("-APIdef_Blink_SetPuppet", (void*)defstring_SetPuppet);
    plugin_register("-APIvararg_Blink_SetPuppet",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetPuppet>));

    plugin_register("-API_Blink_SetTempoAtTime", (void*)SetTempoAtTime);
    plugin_register(
        "-APIdef_Blink_SetTempoAtTime", (void*)defstring_SetTempoAtTime);
    plugin_register("-APIvararg_Blink_SetTempoAtTime",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetTempoAtTime>));

    delete blinkEngine;
}