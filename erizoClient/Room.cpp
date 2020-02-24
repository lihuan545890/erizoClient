#include "Room.h"

#include "ErizoLog.h"
#include "Synchronization.h"

using namespace sio;
using namespace std;

string RoomEvent::ROOM_CONNECTED = "room-connected";
string RoomEvent::ROOM_DISCONNECTED = "room-disconnected";

void RoomSpec::setMaxVideoBW(const int64_t maxVideoBW)
{
	this->maxVideoBW = maxVideoBW;
}

void RoomSpec::setDefaultVideoBW(const int64_t defaultVideoBW)
{
	this->defaultVideoBW = defaultVideoBW;
}

int64_t RoomSpec::getMaxVideoBW()
{
	return maxVideoBW;
}

int64_t RoomSpec::getDefaultVideoBW()
{
	return defaultVideoBW;
}

Room::Room()
{
	p2p = false;
	state = DISCONNECTED;
	localStream = new Stream(true, true, false, true, true);

	addEventHandler(RoomEvent::ROOM_CONNECTED, bind(&Room::handleConnectEvent, this));
	addEventHandler(RoomEvent::ROOM_DISCONNECTED, bind(&Room::handleDisconnectEvent, this));

	addEventHandler(StreamEvent::STREAM_FAILED, bind(&Room::handleStreamFailedEvent, this));
}

Room::~Room()
{
	dispatchEvent(RoomEvent::ROOM_DISCONNECTED);
	delete localStream;
}

#ifdef MOCK
int Room::connect(const string& sioServer, const TokenMock& tokenMock)
#else
int Room::connect(const string& sioServer, const XtToken& xtToken)
#endif
{
	state = CONNECTING;

	erizoClient.set_open_listener(bind(&ErizoClientListener::onConnected, &erizoClientListener));
	erizoClient.set_close_listener(bind(&ErizoClientListener::onClose, &erizoClientListener, placeholders::_1));
	erizoClient.set_fail_listener(bind(&ErizoClientListener::onFail, &erizoClientListener));
	erizoClient.set_reconnect_listener(bind(&ErizoClientListener::onReconnect, &erizoClientListener, placeholders::_1, placeholders::_2));
	erizoClient.set_reconnecting_listener(bind(&ErizoClientListener::onReconnecting, &erizoClientListener));

	// Set max reconnect attempts to 3
	erizoClient.set_reconnect_attempts(3);

	// Set minimum delay for reconnecting: 2000ms
	// this is the delay for 1st reconnecting attempt
	// then the delay duration grows by attempts made
	erizoClient.set_reconnect_delay(2000);

	// Set maximum delay for reconnecting: 2000ms
	erizoClient.set_reconnect_delay_max(2000);

	erizoClient.connect(sioServer);

	unique_lock<mutex> lock(g_mutex);
	while (!g_sio_connect_finish)
	{
		g_cond.wait(lock);
	}
	lock.unlock();

	if (g_sio_connect_retval)
	{
		return g_mcu_connect_retval;
	}

	erizoSocket = erizoClient.socket();

	bindCallbacksOnErizoSocket();

#ifdef MOCK
	erizoSocket->emit("tokenMock", tokenMock.getObjectMessage(),
#else
	erizoSocket->emit("xtToken", xtToken.getObjectMessage(),
#endif
		[&](const message::list& ack_resp)
		{
			lock_guard<mutex> guard(g_mutex);

			if (ack_resp.at(0) != nullptr)
			{
				if (ack_resp.at(0)->get_string().compare(SUCCESS_CODE) == 0)
				{
					if (ack_resp.at(1) != nullptr)
					{
						TokenResp tokenResp(ack_resp.at(1));

						roomSpec.setDefaultVideoBW(tokenResp.getDefaultVideoBW());
						roomSpec.setMaxVideoBW(tokenResp.getMaxVideoBW());
						iceServers = tokenResp.getIceServers();

						if (!tokenResp.getStreams().empty())
						{
							// TODO
							// remoteStreams initialization in this room
							// 2- Retrieve list of streams
							//for (index in streams) {
							//	if (streams.hasOwnProperty(index)) {
							//		arg = streams[index];
							//		stream = Erizo.Stream({ streamID: arg.id, local : false, audio : arg.audio, video : arg.video, data : arg.data, screen : arg.screen, attributes : arg.attributes });
							//		streamList.push(stream);
							//		that.remoteStreams[arg.id] = stream;
							//	}
							//}
						}
						state = CONNECTED;
						roomId = tokenResp.getId();

						ERIZOLOG_INFO << "Connected to room " << roomId << endl;
						dispatchEvent(RoomEvent::ROOM_CONNECTED);

						g_mcu_connect_retval = 0;
					}
					else
					{
						ERIZOLOG_ERROR << "response contains only code without message body for token request!" << endl;
					}
				}
				else if (ack_resp.at(0)->get_string().compare(ERROR_CODE) == 0)
				{
					if (ack_resp.at(1) != nullptr)
					{
						ERIZOLOG_ERROR << "Not Connected! Error: " << ack_resp.at(1)->get_string() << endl;
					}
					else
					{
						ERIZOLOG_ERROR << "Not Connected! Error!" << endl;
					}
				}
			}
			else
			{
				ERIZOLOG_ERROR << "Exception: Receive null message response for token request!" << endl;
			}

			g_mcu_connect_finish = true;
			g_cond.notify_one();
		}
	);

	lock.lock();
	while (!g_mcu_connect_finish)
	{
		g_cond.wait(lock);
	}
	lock.unlock();

	return g_mcu_connect_retval;
}

void Room::disconnect()
{
	ERIZOLOG_INFO << "Disconnection requested" << endl;
	dispatchEvent(RoomEvent::ROOM_DISCONNECTED);
}

void Room::publish()
{
	ASSERT(localStream->isLocal() == true);

	if (localStream->hasAudio() || localStream->hasVideo())
	{
		PublishReq publishReq(localStream->hasVideo(), localStream->hasAudio(), localStream->hasData(), localStream->hasScreen(), roomSpec.getDefaultVideoBW(), "erizo");

		message::list req;
		req.push(publishReq.getObjectMessage());
		req.push("");

		erizoSocket->emit("publish", req,
			[&](const message::list& ack_resp)
			{
				if (ack_resp.at(0) != nullptr)
				{
					localStream->setStreamID(ack_resp.at(0)->get_int());
					ERIZOLOG_INFO << "Stream assigned an Id: " << ack_resp.at(0)->get_int() << ", starting the publish process" << endl;

					localStream->init();
					localStream->getConnection()->setSdpListener(bind(&Room::sdpListener, this, placeholders::_1, placeholders::_2));
					localStream->getConnection()->setCandidateListener(bind(&Room::candidateListener, this, placeholders::_1, placeholders::_2, placeholders::_3));

					if (localStream->getConnection()->addStream(localStream->getMediaStream()))
					{
						ERIZOLOG_INFO << "Finish adding mediaStream to peerConnection" << endl;
						localStream->getConnection()->createOffer();
					}
					else
					{
						ERIZOLOG_ERROR << "Adding stream to PeerConnection failed" << endl;
					}
				}
				else
				{
					ERIZOLOG_ERROR << "Receive null message response for publish request" << endl;
				}
			}
		);
	}
	else if (localStream->hasData())
	{
		// do nothing...
		// in future we should support data channel in peer connection
	}
}

void Room::handleConnectEvent()
{
	publish();
}

void Room::handleDisconnectEvent()
{
	if (state != DISCONNECTED)
	{
		ERIZOLOG_INFO << "begin disconnecting mcu" << endl;
		state = DISCONNECTED;

		// Remove all streams
		//for (index in that.remoteStreams) {
		//	if (that.remoteStreams.hasOwnProperty(index)) {
		//		stream = that.remoteStreams[index];
		//		removeStream(stream);
		//		delete that.remoteStreams[index];
		//		evt2 = Erizo.StreamEvent({ type: 'stream-removed', stream : stream });
		//		that.dispatchEvent(evt2);
		//	}
		//}
		//that.remoteStreams = {};
		ERIZOLOG_DEBUG << "disconnect step 1" << endl;
		if (erizoSocket)
		{
			// Clear all event bindings
			erizoSocket->off_all();
		}

		ERIZOLOG_DEBUG << "disconnect step 2" << endl;
		// close local stream's peer connection
		localStream->close();

		ERIZOLOG_DEBUG << "disconnect step 3" << endl;
		// close socket io connection
		erizoClient.close();

		ERIZOLOG_DEBUG << "disconnect step 4" << endl;
		erizoClient.clear_con_listeners();

		ERIZOLOG_INFO << "finish disconnecting mcu" << endl;
	}
}

void Room::handleStreamFailedEvent()
{
	ERIZOLOG_ERROR << "STREAM FAILED, DISCONNECTION" << endl;
	dispatchEvent(RoomEvent::ROOM_DISCONNECTED);
}

/*
** implementation of ConnectionListener
*/
void Room::sdpListener(string type, string sdp)
{
	message::list req;

	SdpMsg sdpMsg(type, sdp);
	SignalingMsgReq signalingMsgReq(localStream->getStreamID(), sdpMsg.getObjectMessage());

	req.push(signalingMsgReq.getObjectMessage());
	req.push("");
	erizoSocket->emit("signaling_message", req, [](const message::list& ack_resp){});
}

void Room::candidateListener(int sdpMLineIndex, string sdpMid, string candidate)
{
	message::list req;
	
	CandidateMsg candidateMsg(sdpMLineIndex, sdpMid, candidate);
	SignalingMsgReq signalingMsgReq(localStream->getStreamID(), candidateMsg.getObjectMessage());
	
	req.push(signalingMsgReq.getObjectMessage());
	req.push("");
	erizoSocket->emit("signaling_message", req, [](const message::list& ack_resp){});
}

void Room::setErrorCallback(std::function<void(unsigned int)> errorCallback)
{
	this->errorCallback = errorCallback;
	erizoClientListener.setCallback(errorCallback);
}

void Room::refreshDelayTimeForCapturer(uint64_t delay, uint64_t clientType)
{
	if (localStream && localStream->getConnection() && localStream->getConnection()->getDesktopCapturer())
	{
		localStream->getConnection()->getDesktopCapturer()->setDelay(delay, clientType);
	}
}

void Room::activateSession(bool active)
{
	if (localStream && localStream->getConnection() && localStream->getConnection()->getDesktopCapturer())
	{
		localStream->getConnection()->getDesktopCapturer()->activateSession(active);
	}
}
/*
** register all callbacks on ErizoSocket
*/
void Room::bindCallbacksOnErizoSocket()
{
	erizoSocket->on("signaling_message_erizo", socket::event_listener_aux(
		[&](std::string const& name, message::ptr const& data, bool isAck, message::list &ack_resp)
		{
			int64_t streamId = 0;
			int64_t peerId = 0;

			if (data->get_map()["streamId"] != nullptr)
			{
				streamId = data->get_map()["streamId"]->get_int();
			}
			else
			{
				// do nothing since peerId is used for p2p only
				peerId = data->get_map()["peerId"]->get_int();
			}

			if (streamId == localStream->getStreamID())
			{
				message::ptr message = data->get_map()["mess"];
				localStream->getConnection()->processSignalingMessage(message);
			}
		}
	));

	// We receive an event with a new stream in the room
	erizoSocket->on("onAddStream", socket::event_listener_aux(
		[&](std::string const& name, message::ptr const& data, bool isAck, message::list &ack_resp)
		{
			// do nothing since daemon side does not want to subscribe any stream
			ERIZOLOG_TRACE << "received signaling event: onAddStream" << endl;
		}
	));

	// We receive an event of a stream removed from the room
	erizoSocket->on("onRemoveStream", socket::event_listener_aux(
		[&](std::string const& name, message::ptr const& data, bool isAck, message::list &ack_resp)
		{
			// do nothing since daemon side does not want to subscribe any stream
			ERIZOLOG_TRACE << "received signaling event: onRemoveStream" << endl;
		}
	));

	erizoSocket->on("signaling_message_peer", socket::event_listener_aux(
		[&](std::string const& name, message::ptr const& data, bool isAck, message::list &ack_resp)
		{
			// do nothing since this signaling message is used for p2p only
			// note: erizoController get "signaling_message" and then judge whether p2p
			//       if so, then send "signaling_message_peer"
			ERIZOLOG_TRACE << "received signaling event: signaling_message_peer" << endl;
		}
	));

	erizoSocket->on("publish_me", socket::event_listener_aux(
		[&](std::string const& name, message::ptr const& data, bool isAck, message::list &ack_resp)
		{
			// do nothing since this signaling message is used for p2p only
			// note: erizoController get "subscribe" and then judge whether p2p
			//       if so, then send "publish_me"
			ERIZOLOG_TRACE << "received signaling event: publish_me" << endl;
		}
	));

	erizoSocket->on("onBandwidthAlert", socket::event_listener_aux(
		[&](std::string const& name, message::ptr const& data, bool isAck, message::list &ack_resp)
		{
			// TODO
			ERIZOLOG_WARN << "received signaling event: onBandwidthAlert" << endl;
		}
	));

	// We receive an event of new data in one of the streams
	erizoSocket->on("onDataStream", socket::event_listener_aux(
		[&](std::string const& name, message::ptr const& data, bool isAck, message::list &ack_resp)
		{
			// DataStream is not used in our sceanario
			// in future we will enable data channel in peer connection
			ERIZOLOG_TRACE << "received signaling event: onDataStream" << endl;
		}
	));

	erizoSocket->on("onUpdateAttributeStream", socket::event_listener_aux(
		[&](std::string const& name, message::ptr const& data, bool isAck, message::list &ack_resp)
		{
			// Temporarily do not support attribute update
			ERIZOLOG_TRACE << "received signaling event: onUpdateAttributeStream" << endl;
		}
	));

	// The socket has disconnected
	erizoSocket->on("disconnect", socket::event_listener_aux(
		[&](std::string const& name, message::ptr const& data, bool isAck, message::list &ack_resp)
		{
			ERIZOLOG_INFO << "Socket disconnected, lost connection to ErizoController" << endl;

			if (state != DISCONNECTED)
			{
				ERIZOLOG_WARN << "Unexpected disconnection from ErizoController" << endl;
				dispatchEvent(RoomEvent::ROOM_DISCONNECTED);
			}
		}
	));

	erizoSocket->on("error", socket::event_listener_aux(
		[&](std::string const& name, message::ptr const& data, bool isAck, message::list &ack_resp)
		{
			ERIZOLOG_ERROR << "Cannot connect to ErizoController (socket.io error)" << data->get_string() << endl;
		}
	));

	erizoSocket->on("connection_failed", socket::event_listener_aux(
		[&](std::string const& name, message::ptr const& data, bool isAck, message::list &ack_resp)
		{
			if (data->get_map()["type"] != nullptr)
			{
				if (data->get_map()["type"]->get_string().compare("publish") == 0)
				{
					ERIZOLOG_ERROR << "ICE Connection Failed on publishing, disconnecting" << endl;
					if (state != DISCONNECTED)
					{
						dispatchEvent(StreamEvent::STREAM_FAILED);
					}
				}
				else
				{
					ERIZOLOG_ERROR << "ICE Connection Failed on subscribe, alerting" << endl;
					if (state != DISCONNECTED)
					{
						dispatchEvent(StreamEvent::STREAM_FAILED);
					}
				}
			}
			else
			{
				ERIZOLOG_ERROR << "connection_failed signaling message's body has no type field!" << endl;
			}
		}
	));

}
