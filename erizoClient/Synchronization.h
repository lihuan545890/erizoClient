#pragma once

#include <mutex>
#include <condition_variable>

extern bool g_sio_connect_finish;
extern bool g_mcu_connect_finish;
extern int g_sio_connect_retval;
extern int g_mcu_connect_retval;
extern std::mutex g_mutex;
extern std::condition_variable g_cond;
