// global_vars.hpp
#ifndef GLOBAL_VARS_HPP
#define GLOBAL_VARS_HPP

#include <atomic>

namespace reablink
{
extern std::atomic<double> g_timeline_offset_reablink;
extern std::atomic<double> g_launch_offset_reablink;
extern std::atomic_int g_abuf_len;
extern std::atomic<double> g_abuf_srate;
extern std::atomic<double> g_abuf_time;
} // namespace reablink

#endif // GLOBAL_VARS_HPP