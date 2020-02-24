#pragma once

#include "sio/include/sio_client.h"
#include "ConnectionListener.h"
#include "ErizoClientListener.h"
#include "ErizoMessage.h"
#include "Event.h"
#include "Stream.h"

//#define MOCK

enum RoomState
{
	DISCONNECTED = 0,
	CONNECTING,
	CONNECTED
};

class RoomSpec
{
private:
	int64_t maxVideoBW;
	int64_t defaultVideoBW;

public:
	void setMaxVideoBW(const int64_t maxVideoBW);
	void setDefaultVideoBW(const int64_t defaultVideoBW);
	int64_t getMaxVideoBW();
	int64_t getDefaultVideoBW();
};

class Room: public ConnectionListener, public Event
{
private:
	bool p2p;
	RoomState state;
	Stream* localStream;
	sio::socket::ptr erizoSocket;
	std::string roomId;
	sio::client erizoClient;
	RoomSpec roomSpec;
	std::vector<sio::message::ptr> iceServers;
	std::function<void(unsigned int)> errorCallback;
	ErizoClientListener erizoClientListener;

	// TODO: remoteStreams
	//std::vector<sio::message::ptr> remoteStreams;

	void publish();

	// RoomEvent handlers
	void handleConnectEvent();
	void handleDisconnectEvent();

	// StreamEvent handlers
	void handleStreamFailedEvent();

	// register all callbacks on ErizoSocket
	void bindCallbacksOnErizoSocket();

	// implementation of ConnectionListener
	void sdpListener(std::string type, std::string value);
	void candidateListener(int sdpMLineIndex, std::string sdpMid, std::string candidate);

public:
	Room();
	virtual ~Room();

#ifdef MOCK
	int connect(const std::string& sioServer, const TokenMock& tokenMock);
#else
	int connect(const std::string& sioServer, const XtToken& xtToken);
#endif

	void disconnect();

	void setErrorCallback(std::function<void(unsigned int)> errorCallback);

	void refreshDelayTimeForCapturer(uint64_t delay, uint64_t clientType);

	void activateSession(bool active);
};

class RoomEvent
{
	public:
		static std::string ROOM_CONNECTED;
		static std::string ROOM_DISCONNECTED;
};
