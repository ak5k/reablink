#include "ReaBlink.hpp"
#include "BlinkEngine.hpp"
#include <reaper_plugin_functions.h>
#include <reascript_vararg.hpp>

BlinkEngine& blinkEngine = BlinkEngine::GetInstance();

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

/*! @brief Is Link currently enabled?
 *  Thread-safe: yes
 *  Realtime-safe: yes
 */
bool GetEnabled()
{
    return blinkEngine.GetLink().isEnabled();
}
const char* defstring_GetEnabled =
    "bool\0\0\0"
    "Is Blink currently enabled?";

/*! @brief Enable/disable Link.
 *  Thread-safe: yes
 *  Realtime-safe: no
 */
void SetEnabled(bool enable)
{
    if (enable && !blinkEngine.GetLink().isEnabled()) {
        blinkEngine.GetLink().enable(true);
    }
    else if (!enable && blinkEngine.GetLink().isEnabled()) {
        blinkEngine.GetLink().enable(false);
    }
    else {
        blinkEngine.GetLink().enable(false);
    }
    return;
}
const char* defstring_SetEnabled =
    "void\0bool\0enable\0"
    "Enable/disable Blink. In Blink methods transport, tempo and timeline "
    "refer to Link session, not local REAPER instance.";

/*! @brief: Is start/stop synchronization enabled?
 *  Thread-safe: yes
 *  Realtime-safe: no
 */
bool GetStartStopSyncEnabled()
{
    return blinkEngine.GetStartStopSyncEnabled();
}
const char* defstring_GetStartStopSyncEnabled =
    "bool\0\0\0"
    "Is start/stop synchronization enabled?";

/*! @brief: Enable start/stop synchronization.
 *  Thread-safe: yes
 *  Realtime-safe: no
 */
void SetStartStopSyncEnabled(bool enable)
{
    blinkEngine.SetStartStopSyncEnabled(enable);
    return;
}
const char* defstring_SetStartStopSyncEnabled =
    "void\0bool\0enable\0"
    "Enable start/stop synchronization.";

/*! @brief How many peers are currently connected in a Link session?
 *  Thread-safe: yes
 *  Realtime-safe: yes
 */
int GetNumPeers()
{
    return (int)blinkEngine.GetLink().numPeers();
}
const char* defstring_GetNumPeers =
    "int\0\0\0"
    "How many peers are currently connected in Link session?";

/*! @brief The clock used by Link.
 *  Thread-safe: yes
 *  Realtime-safe: yes
 *
 *  @discussion The Clock type is a platform-dependent representation
 *  of the system clock.
 */
double GetClockNow()
{
    return blinkEngine.GetLink().clock().micros().count() / 1.0e6;
}
const char* defstring_GetClockNow =
    "double\0\0\0"
    "Clock used by Blink.";

/*! @brief: The tempo of the timeline, in Beats Per Minute.
 *
 *  @discussion This is a stable value that is appropriate for display
 *  to the user. Beat time progress will not necessarily match this tempo
 *  exactly because of clock drift compensation.
 */
double GetTempo()
{
    return blinkEngine.GetLink().captureAppSessionState().tempo();
}
const char* defstring_GetTempo =
    "double\0\0\0"
    "Tempo of timeline, in quarter note Beats Per Minute.";

/*! @brief: Set the timeline tempo to the given bpm value.
 */
void SetTempo(double bpm)
{
    blinkEngine.SetTempo(bpm);
    return;
}
const char* defstring_SetTempo =
    "void\0double\0bpm\0"
    "Set timeline tempo to given bpm value.";

/*! @brief: Set the timeline tempo to the given bpm value, taking
 *  effect at the given time.
 */
void SetTempoAtTime(double bpm, double time)
{
    auto sessionState = blinkEngine.GetLink().captureAppSessionState();
    sessionState.setTempo(bpm, doubleToMicros(time));
    blinkEngine.GetLink().commitAppSessionState(sessionState);
    return;
}
const char* defstring_SetTempoAtTime =
    "void\0double,double\0bpm,time\0"
    "Set tempo to given bpm value, taking effect at given time.";

/*! @brief: Get the beat value corresponding to the given time
 *  for the given quantum.
 *
 *  @discussion: The magnitude of the resulting beat value is
 *  unique to this Link instance, but its phase with respect to
 *  the provided quantum is shared among all session
 *  peers. For non-negative beat values, the following
 *  property holds: fmod(beatAtTime(t, q), q) == phaseAtTime(t, q)
 */
double GetBeatAtTime(double time, double quantum)
{
    return blinkEngine.GetLink().captureAppSessionState().beatAtTime(
        doubleToMicros(time), quantum);
}
const char* defstring_GetBeatAtTime =
    "double\0double,double\0time,quantum\0"
    "Get session beat value corresponding to given time for given quantum.";

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
double GetPhaseAtTime(double time, double quantum)
{
    return blinkEngine.GetLink().captureAppSessionState().phaseAtTime(
        doubleToMicros(time), quantum);
}
const char* defstring_GetPhaseAtTime =
    "double\0double,double\0time,quantum\0"
    "Get session phase at given time for given quantum.";

/*! @brief: Get the time at which the given beat occurs for the
 *  given quantum.
 *
 *  @discussion: The inverse of beatAtTime, assuming a constant
 *  tempo. beatAtTime(timeAtBeat(b, q), q) === b.
 */
double GetTimeAtBeat(double beat, double quantum)
{
    return microsToDouble(
        blinkEngine.GetLink().captureAppSessionState().timeAtBeat(
            beat, quantum));
}
const char* defstring_GetTimeAtBeat =
    "double\0double,double\0beat,quantum\0"
    "Get time at which given beat occurs for given quantum.";

/*! @brief: Attempt to map the given beat to the given time in the
 *  context of the given quantum.
 *
 *  @discussion: This method behaves differently depending on the
 *  state of the session. If no other peers are connected,
 *  then this instance is in a session by itself and is free to
 *  re-map the beat/time relationship whenever it pleases. In this
 *  case, beatAtTime(time, quantum) == beat after this method has
 *  been called.
 *
 *  If there are other peers in the session, this instance
 *  should not abruptly re-map the beat/time relationship in the
 *  session because that would lead to beat discontinuities among
 *  the other peers. In this case, the given beat will be mapped
 *  to the next time value greater than the given time with the
 *  same phase as the given beat.
 *
 *  This method is specifically designed to enable the concept of
 *  "quantized launch" in client applications. If there are no other
 *  peers in the session, then an event (such as starting
 *  transport) happens immediately when it is requested. If there
 *  are other peers, however, we wait until the next time at which
 *  the session phase matches the phase of the event, thereby
 *  executing the event in-phase with the other peers in the
 *  session. The client only needs to invoke this method to
 *  achieve this behavior and should not need to explicitly check
 *  the number of peers.
 */
void SetBeatAtTimeRequest(double beat, double time, double quantum)
{
    auto sessionState = blinkEngine.GetLink().captureAppSessionState();
    sessionState.requestBeatAtTime(beat, doubleToMicros(time), quantum);
    blinkEngine.GetLink().commitAppSessionState(sessionState);
    return;
}
const char* defstring_SetBeatAtTimeRequest =
    "void\0double,double,double\0bpm,time,quantum\0"
    "Attempt to map given beat to given time in context of given quantum.";

/*! @brief: Rudely re-map the beat/time relationship for all peers
 *  in a session.
 *
 *  @discussion: DANGER: This method should only be needed in
 *  certain special circumstances. Most applications should not
 *  use it. It is very similar to requestBeatAtTime except that it
 *  does not fall back to the quantizing behavior when it is in a
 *  session with other peers. Calling this method will
 *  unconditionally map the given beat to the given time and
 *  broadcast the result to the session. This is very anti-social
 *  behavior and should be avoided.
 *
 *  One of the few legitimate uses of this method is to
 *  synchronize a Link session with an external clock source. By
 *  periodically forcing the beat/time mapping according to an
 *  external clock source, a peer can effectively bridge that
 *  clock into a Link session. Much care must be taken at the
 *  application layer when implementing such a feature so that
 *  users do not accidentally disrupt Link sessions that they may
 *  join.
 */
void SetBeatAtTimeForce(double beat, double time, double quantum)
{
    auto sessionState = blinkEngine.GetLink().captureAppSessionState();
    sessionState.forceBeatAtTime(beat, doubleToMicros(time), quantum);
    blinkEngine.GetLink().commitAppSessionState(sessionState);
    return;
}
const char* defstring_SetBeatAtTimeForce =
    "void\0double,double,double\0bpm,time,quantum\0"
    "Rudely re-map beat/time relationship for all peers in Link session.";

/*! @brief: Set if transport should be playing or stopped, taking effect
 *  at the given time.
 */
void SetPlaying(bool playing, double time)
{
    auto sessionState = blinkEngine.GetLink().captureAppSessionState();
    sessionState.setIsPlaying(playing, doubleToMicros(time));
    blinkEngine.GetLink().commitAppSessionState(sessionState);
    return;
}
const char* defstring_SetPlaying =
    "void\0bool,double\0playing,time\0"
    "Set if transport should be playing or stopped, taking effect at given "
    "time.";

/*! @brief: Is transport playing? */
bool GetPlaying()
{
    return blinkEngine.GetLink().captureAppSessionState().isPlaying();
}
const char* defstring_GetPlaying =
    "bool\0\0\0"
    "Is transport playing?";

/*! @brief: Get the time at which a transport start/stop occurs */
double GetTimeForPlaying()
{
    return microsToDouble(
        blinkEngine.GetLink().captureAppSessionState().timeForIsPlaying());
}
const char* defstring_GetTimeForPlaying =
    "double\0\0\0"
    "Get time at which transport start/stop occurs.";

/*! @brief: Convenience function to attempt to map the given beat to the time
 *  when transport is starting to play in context of the given quantum.
 *  This function evaluates to a no-op if GetPlaying() equals false.
 */
void SetBeatAtStartPlayingTimeRequest(double beat, double quantum)
{
    auto sessionState = blinkEngine.GetLink().captureAppSessionState();
    sessionState.requestBeatAtStartPlayingTime(beat, quantum);
    blinkEngine.GetLink().commitAppSessionState(sessionState);
    return;
}
const char* defstring_SetBeatAtStartPlayingTimeRequest =
    "void\0double,double\0beat,quantum\0"
    "Convenience function to attempt to map given beat to time when "
    "transport is starting to play in context of given quantum. This "
    "function evaluates to a no-op if GetPlaying() equals false.";

/*! @brief: Convenience function to start or stop transport at a given time
 * and attempt to map the given beat to this time in context of the given
 * quantum.
 */
void SetPlayingAndBeatAtTimeRequest(
    bool playing, double time, double beat, double quantum)
{
    auto sessionState = blinkEngine.GetLink().captureAppSessionState();
    sessionState.setIsPlayingAndRequestBeatAtTime(
        playing, doubleToMicros(time), beat, quantum);
    blinkEngine.GetLink().commitAppSessionState(sessionState);
    return;
}
const char* defstring_SetPlayingAndBeatAtTimeRequest =
    "void\0bool,double,double,double\0playing,time,beat,quantum\0"
    "Convenience function to start or stop transport at given time and attempt "
    "to map given beat to this time in context of given quantum.";

void startStop()
{
    auto sessionState = blinkEngine.GetLink().captureAppSessionState();
    if (sessionState.isPlaying()) {
        sessionState.setIsPlaying(
            false, blinkEngine.GetLink().clock().micros());
    }
    else {
        sessionState.setIsPlaying(true, blinkEngine.GetLink().clock().micros());
    }
    blinkEngine.GetLink().commitAppSessionState(sessionState);
    return;
}
const char* defstring_startStop =
    "void\0\0\0"
    "Transport start/stop.";

void SetQuantum(double quantum)
{
    blinkEngine.SetQuantum(quantum);
    return;
}
const char* defstring_SetQuantum =
    "void\0double\0quantum\0"
    "Set quantum. Usually this is set to length of one measure/bar in "
    "quarter notes.";

double GetQuantum()
{
    return blinkEngine.GetQuantum();
}
const char* defstring_GetQuantum =
    "double\0\0\0"
    "Get quantum.";

void SetMaster(bool enable)
{
    return blinkEngine.SetMaster(enable);
}
const char* defstring_SetMaster =
    "void\0bool\0enable\0"
    "Set Blink as Master. Puppet needs to be enabled first. Same as Puppet, "
    "but possible beat offset is broadcast to Link session, effectively "
    "forcing local REAPER timeline on peers. Only one, if any, Blink should be "
    "Master in Link session.";

bool GetMaster()
{
    return blinkEngine.GetMaster();
}
const char* defstring_GetMaster =
    "bool\0\0\0"
    "Is Blink Master?";

void SetPuppet(bool enable)
{
    blinkEngine.Initialize(enable);
    return blinkEngine.SetPuppet(enable);
}
const char* defstring_SetPuppet =
    "void\0bool\0enable\0"
    "Set Blink as Puppet. When enabled, Blink attempts to synchronize local "
    "REAPER tempo to Link session tempo by adjusting current active tempo/time "
    "signature marker, or broadcasts local REAPER tempo changes into Link "
    "session, and attempts to correct possible offset by adjusting REAPER "
    "playrate. Based on cumulative single beat phase since Link session "
    "transport start, regardless of quantum.";

bool GetPuppet()
{
    return blinkEngine.GetPuppet();
}
const char* defstring_GetPuppet =
    "bool\0\0\0"
    "Is Blink Puppet?";

bool runCommand(int command, int flag)
{
    (void)flag;
    auto res = false;
    if (GetEnabled()) {
        if (command == 40044) {
            res = true;
            if (GetPuppet()) {
                if (blinkEngine.GetPlaying()) {
                    blinkEngine.StopPlaying();
                }
                else {
                    blinkEngine.StartPlaying();
                }
            }
            else {
                if (GetPlaying()) {
                    SetPlaying(false, GetClockNow());
                }
                else {
                    SetPlayingAndBeatAtTimeRequest(
                        true, GetClockNow(), 0, GetQuantum());
                }
            }
        }
        if (command == 41130) {
            res = true;
            const auto tempo = GetTempo();
            if (GetPuppet()) {
                blinkEngine.SetTempo(tempo - 1);
            }
            else {
                SetTempo(tempo - 1);
            }
        }
        if (command == 41129) {
            res = true;
            const auto tempo = GetTempo();
            if (GetPuppet()) {
                blinkEngine.SetTempo(tempo + 1);
            }
            else {
                SetTempo(tempo + 1);
            }
        }
    }
    return res;
}

void SetCaptureTransportCommands(bool enable)
{
    if (enable) {
        plugin_register("hookcommand", (void*)runCommand);
    }
    else {
        plugin_register("-hookcommand", (void*)runCommand);
    }
    return;
}
const char* defstring_SetCaptureTransportCommands =
    "void\0bool\0enable\0"
    "Captures REAPER 'Transport: Play/stop' and 'Tempo: Increase/Decrease "
    "current project tempo by 01 BPM' commands and broadcasts them into Link "
    "session. When used with Master or Puppet mode enabled, provides better "
    "integration between REAPER and Link session transport and tempos.";

void registerReaBlink()
{
    plugin_register("API_Blink_SetEnabled", (void*)SetEnabled);
    plugin_register("APIdef_Blink_SetEnabled", (void*)defstring_SetEnabled);
    plugin_register(
        "APIvararg_Blink_SetEnabled",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetEnabled>));

    plugin_register("API_Blink_GetEnabled", (void*)GetEnabled);
    plugin_register("APIdef_Blink_GetEnabled", (void*)defstring_GetEnabled);
    plugin_register(
        "APIvararg_Blink_GetEnabled",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetEnabled>));

    plugin_register("API_Blink_GetMaster", (void*)GetMaster);
    plugin_register("APIdef_Blink_GetMaster", (void*)defstring_GetMaster);
    plugin_register(
        "APIvararg_Blink_GetMaster",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetMaster>));

    plugin_register("API_Blink_SetMaster", (void*)SetMaster);
    plugin_register("APIdef_Blink_SetMaster", (void*)defstring_SetMaster);
    plugin_register(
        "APIvararg_Blink_SetMaster",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetMaster>));

    plugin_register("API_Blink_GetPuppet", (void*)GetPuppet);
    plugin_register("APIdef_Blink_GetPuppet", (void*)defstring_GetPuppet);
    plugin_register(
        "APIvararg_Blink_GetPuppet",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetPuppet>));

    plugin_register("API_Blink_SetPuppet", (void*)SetPuppet);
    plugin_register("APIdef_Blink_SetPuppet", (void*)defstring_SetPuppet);
    plugin_register(
        "APIvararg_Blink_SetPuppet",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetPuppet>));

    plugin_register(
        "API_Blink_GetStartStopSyncEnabled", (void*)GetStartStopSyncEnabled);
    plugin_register(
        "APIdef_Blink_GetStartStopSyncEnabled",
        (void*)defstring_GetStartStopSyncEnabled);
    plugin_register(
        "APIvararg_Blink_GetStartStopSyncEnabled",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetStartStopSyncEnabled>));

    plugin_register(
        "API_Blink_SetStartStopSyncEnabled", (void*)SetStartStopSyncEnabled);
    plugin_register(
        "APIdef_Blink_SetStartStopSyncEnabled",
        (void*)defstring_SetStartStopSyncEnabled);
    plugin_register(
        "APIvararg_Blink_SetStartStopSyncEnabled",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetStartStopSyncEnabled>));

    plugin_register("API_Blink_GetNumPeers", (void*)GetNumPeers);
    plugin_register("APIdef_Blink_GetNumPeers", (void*)defstring_GetNumPeers);
    plugin_register(
        "APIvararg_Blink_GetNumPeers",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetNumPeers>));

    plugin_register("API_Blink_GetClockNow", (void*)GetClockNow);
    plugin_register("APIdef_Blink_GetClockNow", (void*)defstring_GetClockNow);
    plugin_register(
        "APIvararg_Blink_GetClockNow",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetClockNow>));

    plugin_register("API_Blink_GetTempo", (void*)GetTempo);
    plugin_register("APIdef_Blink_GetTempo", (void*)defstring_GetTempo);
    plugin_register(
        "APIvararg_Blink_GetTempo",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetTempo>));

    plugin_register("API_Blink_GetBeatAtTime", (void*)GetBeatAtTime);
    plugin_register(
        "APIdef_Blink_GetBeatAtTime", (void*)defstring_GetBeatAtTime);
    plugin_register(
        "APIvararg_Blink_GetBeatAtTime",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetBeatAtTime>));

    plugin_register("API_Blink_GetPhaseAtTime", (void*)GetPhaseAtTime);
    plugin_register(
        "APIdef_Blink_GetPhaseAtTime", (void*)defstring_GetPhaseAtTime);
    plugin_register(
        "APIvararg_Blink_GetPhaseAtTime",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetPhaseAtTime>));

    plugin_register("API_Blink_GetTimeAtBeat", (void*)GetTimeAtBeat);
    plugin_register(
        "APIdef_Blink_GetTimeAtBeat", (void*)defstring_GetTimeAtBeat);
    plugin_register(
        "APIvararg_Blink_GetTimeAtBeat",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetTimeAtBeat>));

    plugin_register("API_Blink_GetTimeForPlaying", (void*)GetTimeForPlaying);
    plugin_register(
        "APIdef_Blink_GetTimeForPlaying", (void*)defstring_GetTimeForPlaying);
    plugin_register(
        "APIvararg_Blink_GetTimeForPlaying",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetTimeForPlaying>));

    plugin_register("API_Blink_GetPlaying", (void*)GetPlaying);
    plugin_register("APIdef_Blink_GetPlaying", (void*)defstring_GetPlaying);
    plugin_register(
        "APIvararg_Blink_GetPlaying",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetPlaying>));

    plugin_register("API_Blink_SetPlaying", (void*)SetPlaying);
    plugin_register("APIdef_Blink_SetPlaying", (void*)defstring_SetPlaying);
    plugin_register(
        "APIvararg_Blink_SetPlaying",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetPlaying>));

    plugin_register("API_Blink_StartStop", (void*)startStop);
    plugin_register("APIdef_Blink_StartStop", (void*)defstring_startStop);
    plugin_register(
        "APIvararg_Blink_StartStop",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&startStop>));

    plugin_register("API_Blink_SetTempo", (void*)SetTempo);
    plugin_register("APIdef_Blink_SetTempo", (void*)defstring_SetTempo);
    plugin_register(
        "APIvararg_Blink_SetTempo",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetTempo>));

    plugin_register("API_Blink_SetTempoAtTime", (void*)SetTempoAtTime);
    plugin_register(
        "APIdef_Blink_SetTempoAtTime", (void*)defstring_SetTempoAtTime);
    plugin_register(
        "APIvararg_Blink_SetTempoAtTime",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetTempoAtTime>));

    plugin_register(
        "API_Blink_SetBeatAtTimeRequest", (void*)SetBeatAtTimeRequest);
    plugin_register(
        "APIdef_Blink_SetBeatAtTimeRequest",
        (void*)defstring_SetBeatAtTimeRequest);
    plugin_register(
        "APIvararg_Blink_SetBeatAtTimeRequest",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetBeatAtTimeRequest>));

    plugin_register("API_Blink_SetQuantum", (void*)SetQuantum);
    plugin_register("APIdef_Blink_SetQuantum", (void*)defstring_SetQuantum);
    plugin_register(
        "APIvararg_Blink_SetQuantum",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetQuantum>));

    plugin_register("API_Blink_GetQuantum", (void*)GetQuantum);
    plugin_register("APIdef_Blink_GetQuantum", (void*)defstring_GetQuantum);
    plugin_register(
        "APIvararg_Blink_GetQuantum",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetQuantum>));

    plugin_register("API_Blink_SetBeatAtTimeForce", (void*)SetBeatAtTimeForce);
    plugin_register(
        "APIdef_Blink_SetBeatAtTimeForce", (void*)defstring_SetBeatAtTimeForce);
    plugin_register(
        "APIvararg_Blink_SetBeatAtTimeForce",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetBeatAtTimeForce>));

    plugin_register(
        "API_Blink_SetPlayingAndBeatAtTimeRequest",
        (void*)SetPlayingAndBeatAtTimeRequest);
    plugin_register(
        "APIdef_Blink_SetPlayingAndBeatAtTimeRequest",
        (void*)defstring_SetPlayingAndBeatAtTimeRequest);
    plugin_register(
        "APIvararg_Blink_SetPlayingAndBeatAtTimeRequest",
        reinterpret_cast<void*>(
            &InvokeReaScriptAPI<&SetPlayingAndBeatAtTimeRequest>));

    plugin_register(
        "API_Blink_SetBeatAtStartPlayingTimeRequest",
        (void*)SetBeatAtStartPlayingTimeRequest);
    plugin_register(
        "APIdef_Blink_SetBeatAtStartPlayingTimeRequest",
        (void*)defstring_SetBeatAtStartPlayingTimeRequest);
    plugin_register(
        "APIvararg_Blink_SetBeatAtStartPlayingTimeRequest",
        reinterpret_cast<void*>(
            &InvokeReaScriptAPI<&SetBeatAtStartPlayingTimeRequest>));

    plugin_register(
        "API_Blink_SetCaptureTransportCommands",
        (void*)SetCaptureTransportCommands);
    plugin_register(
        "APIdef_Blink_SetCaptureTransportCommands",
        (void*)defstring_SetCaptureTransportCommands);
    plugin_register(
        "APIvararg_Blink_SetCaptureTransportCommands",
        reinterpret_cast<void*>(
            &InvokeReaScriptAPI<&SetCaptureTransportCommands>));
}

void unregisterReaBlink()
{
    plugin_register(
        "-API_Blink_SetPlayingAndBeatAtTimeRequest",
        (void*)SetPlayingAndBeatAtTimeRequest);
    plugin_register(
        "-APIdef_Blink_SetPlayingAndBeatAtTimeRequest",
        (void*)defstring_SetPlayingAndBeatAtTimeRequest);
    plugin_register(
        "-APIvararg_Blink_SetPlayingAndBeatAtTimeRequest",
        reinterpret_cast<void*>(
            &InvokeReaScriptAPI<&SetPlayingAndBeatAtTimeRequest>));

    plugin_register("-API_Blink_SetPlaying", (void*)SetPlaying);
    plugin_register("-APIdef_Blink_SetPlaying", (void*)defstring_SetPlaying);
    plugin_register(
        "-APIvararg_Blink_SetPlaying",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetPlaying>));

    plugin_register(
        "-API_Blink_SetBeatAtTimeRequest", (void*)SetBeatAtTimeRequest);
    plugin_register(
        "-APIdef_Blink_SetBeatAtTimeRequest",
        (void*)defstring_SetBeatAtTimeRequest);
    plugin_register(
        "-APIvararg_Blink_SetBeatAtTimeRequest",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetBeatAtTimeRequest>));

    plugin_register("-API_Blink_SetBeatAtTimeForce", (void*)SetBeatAtTimeForce);
    plugin_register(
        "-APIdef_Blink_SetBeatAtTimeForce",
        (void*)defstring_SetBeatAtTimeForce);
    plugin_register(
        "-APIvararg_Blink_SetBeatAtTimeForce",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetBeatAtTimeForce>));

    plugin_register("-API_Blink_SetEnabled", (void*)SetEnabled);
    plugin_register("-APIdef_Blink_SetEnabled", (void*)defstring_SetEnabled);
    plugin_register(
        "-APIvararg_Blink_SetEnabled",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetEnabled>));

    plugin_register("-API_Blink_GetEnabled", (void*)GetEnabled);
    plugin_register("-APIdef_Blink_GetEnabled", (void*)defstring_GetEnabled);
    plugin_register(
        "-APIvararg_Blink_GetEnabled",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetEnabled>));

    plugin_register(
        "-API_Blink_GetStartStopSyncEnabled", (void*)GetStartStopSyncEnabled);
    plugin_register(
        "-APIdef_Blink_GetStartStopSyncEnabled",
        (void*)defstring_GetStartStopSyncEnabled);
    plugin_register(
        "-APIvararg_Blink_GetStartStopSyncEnabled",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetStartStopSyncEnabled>));

    plugin_register(
        "-API_Blink_SetStartStopSyncEnabled", (void*)SetStartStopSyncEnabled);
    plugin_register(
        "-APIdef_Blink_SetStartStopSyncEnabled",
        (void*)defstring_SetStartStopSyncEnabled);
    plugin_register(
        "-APIvararg_Blink_SetStartStopSyncEnabled",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetStartStopSyncEnabled>));

    plugin_register("-API_Blink_GetNumPeers", (void*)GetNumPeers);
    plugin_register("-APIdef_Blink_GetNumPeers", (void*)defstring_GetNumPeers);
    plugin_register(
        "-APIvararg_Blink_GetNumPeers",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetNumPeers>));

    plugin_register("-API_Blink_GetClockNow", (void*)GetClockNow);
    plugin_register("-APIdef_Blink_GetClockNow", (void*)defstring_GetClockNow);
    plugin_register(
        "-APIvararg_Blink_GetClockNow",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetClockNow>));

    plugin_register("-API_Blink_GetTempo", (void*)GetTempo);
    plugin_register("-APIdef_Blink_GetTempo", (void*)defstring_GetTempo);
    plugin_register(
        "-APIvararg_Blink_GetTempo",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetTempo>));

    plugin_register("-API_Blink_GetBeatAtTime", (void*)GetBeatAtTime);
    plugin_register(
        "-APIdef_Blink_GetBeatAtTime", (void*)defstring_GetBeatAtTime);
    plugin_register(
        "-APIvararg_Blink_GetBeatAtTime",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetBeatAtTime>));

    plugin_register("-API_Blink_GetPhaseAtTime", (void*)GetPhaseAtTime);
    plugin_register(
        "-APIdef_Blink_GetPhaseAtTime", (void*)defstring_GetPhaseAtTime);
    plugin_register(
        "-APIvararg_Blink_GetPhaseAtTime",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetPhaseAtTime>));

    plugin_register("-API_Blink_GetTimeAtBeat", (void*)GetTimeAtBeat);
    plugin_register(
        "-APIdef_Blink_GetTimeAtBeat", (void*)defstring_GetTimeAtBeat);
    plugin_register(
        "-APIvararg_Blink_GetTimeAtBeat",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetTimeAtBeat>));

    plugin_register("-API_Blink_GetTimeForPlaying", (void*)GetTimeForPlaying);
    plugin_register(
        "-APIdef_Blink_GetTimeForPlaying", (void*)defstring_GetTimeForPlaying);
    plugin_register(
        "-APIvararg_Blink_GetTimeForPlaying",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetTimeForPlaying>));

    plugin_register("-API_Blink_GetPlaying", (void*)GetPlaying);
    plugin_register("-APIdef_Blink_GetPlaying", (void*)defstring_GetPlaying);
    plugin_register(
        "-APIvararg_Blink_GetPlaying",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetPlaying>));

    plugin_register("-API_Blink_StartStop", (void*)startStop);
    plugin_register("-APIdef_Blink_StartStop", (void*)defstring_startStop);
    plugin_register(
        "-APIvararg_Blink_StartStop",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&startStop>));

    plugin_register("-API_Blink_SetTempo", (void*)SetTempo);
    plugin_register("-APIdef_Blink_SetTempo", (void*)defstring_SetTempo);
    plugin_register(
        "-APIvararg_Blink_SetTempo",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetTempo>));

    plugin_register("-API_Blink_SetQuantum", (void*)SetQuantum);
    plugin_register("-APIdef_Blink_SetQuantum", (void*)defstring_SetQuantum);
    plugin_register(
        "-APIvararg_Blink_SetQuantum",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetQuantum>));

    plugin_register("-API_Blink_GetQuantum", (void*)GetQuantum);
    plugin_register("-APIdef_Blink_GetQuantum", (void*)defstring_GetQuantum);
    plugin_register(
        "-APIvararg_Blink_GetQuantum",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetQuantum>));

    plugin_register("-API_Blink_GetMaster", (void*)GetMaster);
    plugin_register("-APIdef_Blink_GetMaster", (void*)defstring_GetMaster);
    plugin_register(
        "-APIvararg_Blink_GetMaster",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetMaster>));

    plugin_register("-API_Blink_SetMaster", (void*)SetMaster);
    plugin_register("-APIdef_Blink_SetMaster", (void*)defstring_SetMaster);
    plugin_register(
        "-APIvararg_Blink_SetMaster",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetMaster>));

    plugin_register("-API_Blink_GetPuppet", (void*)GetPuppet);
    plugin_register("-APIdef_Blink_GetPuppet", (void*)defstring_GetPuppet);
    plugin_register(
        "-APIvararg_Blink_GetPuppet",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&GetPuppet>));

    plugin_register("-API_Blink_SetPuppet", (void*)SetPuppet);
    plugin_register("-APIdef_Blink_SetPuppet", (void*)defstring_SetPuppet);
    plugin_register(
        "-APIvararg_Blink_SetPuppet",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetPuppet>));

    plugin_register("-API_Blink_SetTempoAtTime", (void*)SetTempoAtTime);
    plugin_register(
        "-APIdef_Blink_SetTempoAtTime", (void*)defstring_SetTempoAtTime);
    plugin_register(
        "-APIvararg_Blink_SetTempoAtTime",
        reinterpret_cast<void*>(&InvokeReaScriptAPI<&SetTempoAtTime>));

    plugin_register(
        "-API_Blink_SetBeatAtStartPlayingTimeRequest",
        (void*)SetBeatAtStartPlayingTimeRequest);
    plugin_register(
        "-APIdef_Blink_SetBeatAtStartPlayingTimeRequest",
        (void*)defstring_SetBeatAtStartPlayingTimeRequest);
    plugin_register(
        "-APIvararg_Blink_SetBeatAtStartPlayingTimeRequest",
        reinterpret_cast<void*>(
            &InvokeReaScriptAPI<&SetBeatAtStartPlayingTimeRequest>));

    plugin_register("-hookcommand", (void*)runCommand);

    blinkEngine.GetLink().enable(false); // !!!
}