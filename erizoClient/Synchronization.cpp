#include "Synchronization.h"

bool g_sio_connect_finish = false;
bool g_mcu_connect_finish = false;
int g_sio_connect_retval = -1;
int g_mcu_connect_retval = -1;
std::mutex g_mutex;
std::condition_variable g_cond;
