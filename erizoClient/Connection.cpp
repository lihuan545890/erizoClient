#include "Connection.h"

#include "webrtc/api/test/fakeconstraints.h"
#include "webrtc/base/ssladapter.h"
#include "webrtc/base/win32socketinit.h"
#include "ErizoLog.h"

using namespace std;

#define DTLS_ON  true
#define DTLS_OFF false

const string kAudioLabel = "audio_label";
const string kVideoLabel = "video_label";
const string kStreamLabel = "stream_label";

CandidateElement::CandidateElement(int sdpMLineIndex, std::string sdpMid, std::string candidate)
{
	this->sdpMLineIndex = sdpMLineIndex;
	this->sdpMid = sdpMid;
	this->candidate = candidate;
}

CandidateElement::CandidateElement(const CandidateElement& candidateElement)
{
	this->sdpMLineIndex = candidateElement.sdpMLineIndex;
	this->sdpMid = candidateElement.sdpMid;
	this->candidate = candidateElement.candidate;
}

int CandidateElement::getSdpMLineIndex()
{
	return sdpMLineIndex;
}

std::string CandidateElement::getSdpMid()
{
	return sdpMid;
}

std::string CandidateElement::getCandidate()
{
	return candidate;
}

Connection::Connection()
{
	remoteDescriptionSet = false;

	// TODO: how to handle return value?
	if (!initializePeerConnection())
	{
		ERIZOLOG_ERROR << "Failed to initialize our PeerConnection instance" << endl;
	}
}

Connection::~Connection()
{
	deletePeerConnection();
}

cricket::VideoCapturer* Connection::OpenVideoCaptureDevice()
{
	_ScreenVideoCapturerFactory* screen_capturer_factory = new _ScreenVideoCapturerFactory();
	rtc::WindowId::WindowT windowsId_ = GetDesktopWindow();
	cricket::VideoCapturer* capturer = NULL;
	capturer = screen_capturer_factory->Create(cricket::ScreencastId(windowsId_));
	desktopCapturer = screen_capturer_factory->screen_capturer();
	return capturer;
}

bool Connection::initializePeerConnection()
{
	rtc::EnsureWinsockInit();
	rtc::InitializeSSL();

	// Below codes are used to create signaling thread for webrtc
	// we have to create this thread explicitly
	// due to https://bugs.chromium.org/p/webrtc/issues/detail?id=4196
	// otherwise onSuccess() will not be triggered after createOffer()
	//
	// How to handle
	// https://groups.google.com/forum/?fromgroups#!topic/discuss-webrtc/rBj5VD9pNTA
	//
	rtcWorkerThread = new rtc::Thread();
	rtcWorkerThread->Start();
	rtcSignalingThread = new rtc::Thread();
	rtcSignalingThread->Start();

	ASSERT(peerConnectionFactory.get() == NULL);
	ASSERT(peerConnection.get() == NULL);
	ASSERT(desktopCapturer == NULL);

	peerConnectionFactory = webrtc::CreatePeerConnectionFactory(rtcWorkerThread, rtcSignalingThread, NULL, NULL, NULL);
	if (!peerConnectionFactory.get())
	{
		ERIZOLOG_ERROR << "Failed to initialize PeerConnectionFactory" << endl;
		deletePeerConnection();
		return false;
	}

	ERIZOLOG_INFO << "Finish PeerConnectionFactory initialization" << endl;

	if (!createPeerConnection(DTLS_ON))
	{
		ERIZOLOG_ERROR << "CreatePeerConnection failed" << endl;
		deletePeerConnection();
	}

	ERIZOLOG_INFO << "Finish PeerConnection initialization" << endl;

	return peerConnection.get() != NULL;
}

bool Connection::createPeerConnection(bool dtls)
{
	ASSERT(peerConnectionFactory.get() != NULL);
	ASSERT(peerConnection.get() == NULL);

	webrtc::PeerConnectionInterface::RTCConfiguration config;
//	webrtc::PeerConnectionInterface::IceServer server;
//	server.uri = "stun:stun.l.google.com:19302";
//	config.servers.push_back(server);

	webrtc::FakeConstraints constraints;
	if (dtls)
	{
		constraints.AddOptional(
			webrtc::MediaConstraintsInterface::kEnableDtlsSrtp,
			"true");
	}
	else
	{
		constraints.AddOptional(
			webrtc::MediaConstraintsInterface::kEnableDtlsSrtp,
			"false");
	}
	peerConnection = peerConnectionFactory->CreatePeerConnection(config,
		&constraints, NULL, NULL, this);
	return peerConnection.get() != NULL;
}

void Connection::deletePeerConnection()
{
	peerConnection = NULL;
	peerConnectionFactory = NULL;

	if (rtcWorkerThread)
	{
		rtcWorkerThread->Stop();
		delete rtcWorkerThread;
		rtcWorkerThread = NULL;
	}

	if (rtcSignalingThread)
	{
		rtcSignalingThread->Stop();
		delete rtcSignalingThread;
		rtcSignalingThread = NULL;
	}
}

rtc::scoped_refptr<webrtc::MediaStreamInterface> Connection::getUserMedia(bool video, bool audio, bool screen)
{
	rtc::scoped_refptr<webrtc::MediaStreamInterface> stream =
		peerConnectionFactory->CreateLocalMediaStream(kStreamLabel);

	if (audio)
	{
		rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
			peerConnectionFactory->CreateAudioTrack(
			kAudioLabel, peerConnectionFactory->CreateAudioSource(NULL)));

		stream->AddTrack(audio_track);
	}

	if (video && screen)
	{
		rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track(
			peerConnectionFactory->CreateVideoTrack(
			kVideoLabel,
			peerConnectionFactory->CreateVideoSource(OpenVideoCaptureDevice(),
			NULL)));

		stream->AddTrack(video_track);
	}

	ERIZOLOG_INFO << "Finish adding audio/video track to mediaStream during getUserMedia" << endl;

	return stream;
}

bool Connection::addStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
	ASSERT(peerConnection.get() != NULL);
	return peerConnection->AddStream(stream);
}

void Connection::createOffer()
{
	ASSERT(peerConnection.get() != NULL);
	peerConnection->CreateOffer(this, NULL);
}

_ScreenVideoCapturer* Connection::getDesktopCapturer()
{
	return desktopCapturer;
}

//-----------------------------------------
// DummySetSessionDescriptionObserver implementation.
//-----------------------------------------
class DummySetSessionDescriptionObserver
	: public webrtc::SetSessionDescriptionObserver
{
public:
	static DummySetSessionDescriptionObserver* Create()
	{
		return
			new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
	}

	virtual void OnSuccess() {}

	virtual void OnFailure(const std::string& error)
	{
		ERIZOLOG_ERROR << error << endl;
	}

protected:
	DummySetSessionDescriptionObserver() {}
	~DummySetSessionDescriptionObserver() {}
};

void Connection::setSdpListener(function<void(string, string)> sdpListener)
{
	this->sdpListener = sdpListener;
}

void Connection::setCandidateListener(std::function<void(int, std::string, std::string)> candidateListener)
{
	this->candidateListener = candidateListener;
}

void Connection::processSignalingMessage(const sio::message::ptr& message)
{
	std::string type = message->get_map()["type"]->get_string();
	if (type == "answer")
	{
		string remoteSdp = message->get_map()["sdp"]->get_string();

		webrtc::SdpParseError error;
		webrtc::SessionDescriptionInterface* sessionDescription(
			webrtc::CreateSessionDescription(type, remoteSdp, &error));

		if (!sessionDescription)
		{
			ERIZOLOG_ERROR << "Can't parse received session description message. " << "SdpParseError was: " << error.description << endl;
			return;
		}

		ERIZOLOG_INFO << " Received remote session description" << endl;

		peerConnection->SetRemoteDescription(
			DummySetSessionDescriptionObserver::Create(), sessionDescription);

		/*
		 * processSignalingMessage() and OnIceCandidate() are executed in different threads
		 * but candidatesDeque is accessed in both threads
		 * also IMPORTANT: preserve ordering of candidates
		 * therefore we have to add synchronization
		*/
		unique_lock<mutex> lock(mtx);

		remoteDescriptionSet = true;

		for (deque<CandidateElement>::iterator it = candidatesDeque.begin(); it != candidatesDeque.end(); it++)
		{
			if (candidateListener)
			{
				candidateListener(it->getSdpMLineIndex(), it->getSdpMid(), it->getCandidate());
				ERIZOLOG_DEBUG << "Local candidate sent: " << it->getSdpMLineIndex() << " " << it->getSdpMid() << " " << it->getCandidate() << endl;
			}
		}

		candidatesDeque.clear();

		lock.unlock();
	}
	else if (type == "candidate")
	{
		ERIZOLOG_INFO << "candidate-------------------------------------------------------------------------------" << endl;
	}
	else
	{
		ERIZOLOG_INFO << type << "--------------------------------------------------------------------------------" << endl;
	}
}

//-----------------------------------------
// CreateSessionDescriptionObserver implementation.
//-----------------------------------------
void Connection::OnSuccess(webrtc::SessionDescriptionInterface* desc)
{
	ERIZOLOG_INFO << "Offer has been created successfully" << endl;

	peerConnection->SetLocalDescription(
		DummySetSessionDescriptionObserver::Create(), desc);

	string sdp;
	desc->ToString(&sdp);

	if (sdpListener)
	{
		sdpListener(desc->type(), sdp);
	}
}

void Connection::OnFailure(const string& error)
{
	ERIZOLOG_ERROR << "Failed in sdp offer creation. " << error << endl;
}

//-----------------------------------------
// PeerConnectionObserver implementation.
//-----------------------------------------
// New Ice candidate have been found.
void Connection::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{
	string candidateStr;

	if (!candidate->ToString(&candidateStr))
	{
		ERIZOLOG_ERROR << "Failed to serialize candidate" << endl;
		return;
	}

	unique_lock<mutex> lock(mtx);

	if (remoteDescriptionSet)
	{
		if (candidateListener)
		{
			candidateListener(candidate->sdp_mline_index(), candidate->sdp_mid(), "a=" + candidateStr);
		}

		ERIZOLOG_DEBUG << "Local candidate sent: " << candidate->sdp_mline_index() << " " << candidate->sdp_mid() << " " << "a=" + candidateStr << endl;
	}
	else
	{
		CandidateElement candidateElement(candidate->sdp_mline_index(),
			candidate->sdp_mid(), "a=" + candidateStr);

		// copy constructor!
		candidatesDeque.push_back(candidateElement);

		ERIZOLOG_DEBUG << "Local candidate stored: " << candidate->sdp_mline_index() << " " << candidate->sdp_mid() << " " << "a=" + candidateStr << endl;
	}

	lock.unlock();
}
