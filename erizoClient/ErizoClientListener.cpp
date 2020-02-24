#include "ErizoClientListener.h"

#include "ErizoLog.h"
#include "Synchronization.h"

using namespace std;

ErizoClientListener::ErizoClientListener()
{
}

void ErizoClientListener::onConnected()
{
	lock_guard<mutex> guard(g_mutex);
	g_sio_connect_finish = true;
	g_sio_connect_retval = 0;
	g_cond.notify_one();
}

void ErizoClientListener::onClose(const sio::client::close_reason& reason)
{
	ERIZOLOG_INFO << "sio closed: " << reason << endl;
	if (callback) {
		ERIZOLOG_INFO << "trigger callback for sio close" << endl;
		// close: 1
		callback(1);
	}
}

void ErizoClientListener::onFail()
{
	ERIZOLOG_ERROR << "sio failed " << endl;
	lock_guard<mutex> guard(g_mutex);
	g_sio_connect_finish = true;
	// keep g_sio_connect_retval unchanged with val "-1"
	g_cond.notify_one();
}

void ErizoClientListener::onReconnect(unsigned reconnMade, unsigned delay)
{
	ERIZOLOG_ERROR << "sio reconnect: reconnect made num is " << reconnMade << ", delay is " << delay << endl;
	if (g_mcu_connect_finish == true && g_mcu_connect_retval == 0)
	{
		ERIZOLOG_INFO << "invoke callee's callback to notify exception" << endl;

		g_mcu_connect_retval = -1;
		// invoke callee's callback to let it know that exception happened
		if (callback) {
			// close: 1
			callback(1);
		}
	}
}

void ErizoClientListener::onReconnecting()
{
	ERIZOLOG_ERROR << "sio reconnecting" << endl;
}

void ErizoClientListener::setCallback(std::function<void(unsigned int)> callback)
{
	this->callback = callback;
}
