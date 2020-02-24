#pragma once

#include <string>

class ConnectionListener
{
protected:
	virtual void sdpListener(std::string type, std::string value) = 0;
	virtual void candidateListener(int sdpMLineIndex, std::string sdpMid, std::string candidate) = 0;
};
