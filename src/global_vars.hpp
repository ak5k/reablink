// global_vars.hpp
#ifndef GLOBAL_VARS_HPP
#define GLOBAL_VARS_HPP

#include <atomic>

extern std::atomic_int g_abuf_len;
extern std::atomic<double> g_abuf_srate;
extern std::atomic<double> g_abuf_time;

#endif // GLOBAL_VARS_HPP