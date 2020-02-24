#pragma once

#include <deque>
#include <functional>
#include <mutex>
#include "api/peerconnectioninterface.h"
#include "screen_video_capturer.h"
#include "sio/include/sio_message.h"

class CandidateElement
{
private:
	int sdpMLineIndex;
	std::string sdpMid;
	std::string candidate;
public:
	CandidateElement(int, std::string, std::string);
	CandidateElement(const CandidateElement&);
	int getSdpMLineIndex();
	std::string getSdpMid();
	std::string getCandidate();
};

class Connection : public webrtc::PeerConnectionObserver, public webrtc::CreateSessionDescriptionObserver
{
private:
	bool remoteDescriptionSet;
	rtc::Thread* rtcWorkerThread;
	rtc::Thread* rtcSignalingThread;
	rtc::scoped_refptr<webrtc::PeerConnectionInterface> peerConnection;
	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peerConnectionFactory;
	std::function<void(std::string, std::string)> sdpListener;
	std::function<void(int, std::string, std::string)> candidateListener;
	std::mutex mtx;
	std::deque<CandidateElement> candidatesDeque;
	_ScreenVideoCapturer* desktopCapturer;

	cricket::VideoCapturer* OpenVideoCaptureDevice();
	bool initializePeerConnection();
	bool createPeerConnection(bool dtls);

public:
	Connection();
	virtual ~Connection();
	rtc::scoped_refptr<webrtc::MediaStreamInterface> getUserMedia(bool video, bool audio, bool screen);
	bool addStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream);
	void createOffer();
	void processSignalingMessage(const sio::message::ptr& message);
	void deletePeerConnection();
	_ScreenVideoCapturer* getDesktopCapturer();

	void setSdpListener(std::function<void(std::string, std::string)> sdpListener);
	void setCandidateListener(std::function<void(int, std::string, std::string)> candidateListener);

	//-----------------------------------------
	// PeerConnectionObserver implementation.
	//-----------------------------------------
	// Triggered when media is received on a new stream from remote peer.
	virtual void OnAddStream(webrtc::MediaStreamInterface* stream) {}

	// Triggered when a remote peer close a stream.
	virtual void OnRemoveStream(webrtc::MediaStreamInterface* stream) {}

	// Triggered when a remote peer open a data channel.
	virtual void OnDataChannel(webrtc::DataChannelInterface* data_channel) {}

	// Triggered when renegotiation is needed, for example the ICE has restarted.
	virtual void OnRenegotiationNeeded() {}

	// New Ice candidate have been found.
	virtual void OnIceCandidate(const webrtc::IceCandidateInterface* candidate);

	// Triggered when the SignalingState changed.
	virtual void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) {}

	// Called any time the IceConnectionState changes
	virtual void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) {}

	// Called any time the IceGatheringState changes
	virtual void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) {}

	//-----------------------------------------
	// CreateSessionDescriptionObserver implementation.
	//-----------------------------------------
	virtual void OnSuccess(webrtc::SessionDescriptionInterface* desc);
	virtual void OnFailure(const std::string& error);
};
