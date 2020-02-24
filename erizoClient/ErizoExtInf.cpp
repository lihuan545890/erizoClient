#include "ErizoExtInf.h"

#include "ErizoLog.h"
#include "Room.h"

#define LOCKFILE "erizoClient.lock"

MCURoom::MCURoom()
{
	room = new Room();
	hFile = nullptr;
}

MCURoom::~MCURoom()
{
	disconnect();
}

int MCURoom::connect(const std::string& user, const std::string& token,
	const std::string& sessionId, const std::string& sessionMgrUrl, const std::string& socketIoServer,
	int64_t clientType, int64_t clientVer)
{
	if (room == nullptr)
	{
		room = new Room();
	}
	int retval = 0;
	hFile = CreateFile(LOCKFILE, GENERIC_READ, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_SHARING_VIOLATION)
	{
		ERIZOLOG_ERROR << "connect() has already been called once" << std::endl;
		return -1;
	}

#ifdef MOCK
	TokenMock tokenMock(sessionId, socketIoServer, false);
	retval = room->connect(socketIoServer, tokenMock);
#else
	XtToken xtToken(user, token, sessionId, sessionMgrUrl, clientType, clientVer);
	retval = room->connect(socketIoServer, xtToken);
#endif

	return retval;
}

void MCURoom::disconnect()
{
	if (hFile)
	{
		CloseHandle(hFile);
		hFile = nullptr;
		DeleteFile(LOCKFILE);
	}
	if (room)
	{
		room->disconnect();
		delete room;
		room = nullptr;
	}
}

void MCURoom::setErrorCallback(std::function<void(unsigned int)> errorCallback)
{
	if (room == nullptr)
	{
		room = new Room();
	}
	room->setErrorCallback(errorCallback);
}

void MCURoom::refreshDelayTimeForCapturer(uint64_t delay, uint64_t clientType)
{
	if (delay)
	{
		room->refreshDelayTimeForCapturer(delay, clientType);
	}
}

void  MCURoom::activateSession(bool active)
{
	room->activateSession(active);
}

/*
int MCURoom::startMonitorForReconnection()
{
	hThread = CreateThread(NULL, 0, reconnectionMonitor, (void*)this, 0, NULL);
	if (hThread)
	{
		return 0;
	}
	ERIZOLOG_ERROR << "Failed in creating thread" << std::endl;
	return -1;
}

DWORD WINAPI MCURoom::reconnectionMonitor(void* param)
{
	MCURoom* mcuRoom = (MCURoom*)param;

	// triggered by condition variable
	// wait()

	return 0;
}
*/
