#include "Stream.h"

std::string StreamEvent::STREAM_ADDED = "stream-added";
std::string StreamEvent::STREAM_REMOVED = "stream-removed";
std::string StreamEvent::STREAM_SUBSCRIBED = "stream-subscribed";
std::string StreamEvent::STREAM_FAILED = "stream-failed";

Stream::Stream(bool video, bool audio, bool data, bool screen, bool local, int64_t streamID)
{
	this->video = video;
	this->audio = audio;
	this->data = data;
	this->screen = screen;
	this->local = local;
	if (this->local)
	{
		this->streamID = 0;
	}
	else
	{
		this->streamID = streamID;
	}
	connection = nullptr;
	mediaStream = nullptr;
}

Stream::~Stream()
{
}

void Stream::init()
{
	// no need to delete connection in destructor
	// it is managed by reference counter automatically
	connection = new rtc::RefCountedObject<Connection>();

	if (audio || video)
	{
		mediaStream = connection->getUserMedia(video, audio, screen);
	}
}

void Stream::close()
{
	mediaStream = nullptr;
	if (connection != nullptr)
	{
		connection->deletePeerConnection();
	}
}

bool Stream::isLocal()
{
	return local;
}

bool Stream::hasVideo()
{
	return video;
}

bool Stream::hasAudio()
{
	return audio;
}

bool Stream::hasData()
{
	return data;
}

bool Stream::hasScreen()
{
	return screen;
}

void Stream::setStreamID(int64_t streamID)
{
	this->streamID = streamID;
}

int64_t Stream::getStreamID()
{
	return streamID;
}

rtc::scoped_refptr<Connection> Stream::getConnection()
{
	return connection;
}

rtc::scoped_refptr<webrtc::MediaStreamInterface> Stream::getMediaStream()
{
	return mediaStream;
}
