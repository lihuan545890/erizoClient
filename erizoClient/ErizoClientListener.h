#pragma once

#include "sio/include/sio_client.h"

class ErizoClientListener
{
private:
	std::function<void(unsigned int)> callback;

public:
	ErizoClientListener();

	void onConnected();

	void onClose(const sio::client::close_reason& reason);

	void onFail();

	void onReconnect(unsigned reconnMade, unsigned delay);

	void onReconnecting();

	void setCallback(std::function<void(unsigned int)> callback);
};
