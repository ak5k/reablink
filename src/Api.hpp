#include "AudioEngine.hpp"
#include "reascript_vararg.hpp"
#include <reaper_plugin_functions.h>

struct LinkSession
{
    std::atomic<bool> running = true;
    ableton::Link link = ableton::Link(Master_GetTempo());
    ableton::linkaudio::AudioPlatform audioPlatform =
        ableton::linkaudio::AudioPlatform(link);

    LinkSession() = default;
};
