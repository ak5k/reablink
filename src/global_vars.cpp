// global_vars.cpp
#include "global_vars.hpp"

std::atomic<double> g_timeline_offset_reablink{};
std::atomic<double> g_launch_offset_reablink{};
std::atomic_int g_abuf_len{};
std::atomic<double> g_abuf_srate{};
std::atomic<double> g_abuf_time{};