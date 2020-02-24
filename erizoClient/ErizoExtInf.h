#pragma once

#include <functional>
#include <string>
#include <windows.h>

class Room;

class MCURoom
{
private:
	Room* room;
	HANDLE hFile;

public:
	MCURoom();
	~MCURoom();

	int connect(const std::string& user, const std::string& token,
		const std::string& sessionId, const std::string& sessionMgrUrl,
		const std::string& socketIoServer, int64_t clientType, int64_t clientVer);

	void disconnect();

	void setErrorCallback(std::function<void(unsigned int)> errorCallback);

	void refreshDelayTimeForCapturer(uint64_t delay, uint64_t clientType);
	
	void activateSession(bool active);
};
