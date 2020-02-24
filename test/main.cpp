#include "../erizoClient/ErizoExtInf.h"

#include <iostream>
#include <windows.h>

using namespace std;

void tempFunc(unsigned int temp)
{
	std::cout << "hello" << std::endl;
}

int main(int argc, const char* args[])
{
	MCURoom room;
	std::string user = "823745234750";
	std::string token = "a0fa488a-fc44-4087-9380-3f12bb0c8dff";
	std::string sessionId = "24b12846-26a1-41b7-a0d3-136ebdaf1e7c";
	std::string sessionMgrUrl = "https://192.168.11.7/uz-sessionmgrv2/";
	std::string socketIoServer = "https://mcu.uzer.me:8080";

	room.setErrorCallback(tempFunc);

	int ret = room.connect(user, token, sessionId,
		sessionMgrUrl, socketIoServer, 1, 1);

	if (ret == 0)
	{
		Sleep(3000);
	}

	room.disconnect();

	room.setErrorCallback(tempFunc);

	ret = room.connect(user, token, sessionId,
		sessionMgrUrl, socketIoServer, 1, 1);

	if (ret == 0)
	{
		Sleep(3000);
	}

	room.disconnect();
	Sleep(1000);

	return 0;
}
