#pragma once

#include "Connection.h"

class Stream
{
private:
	bool video;
	bool audio;
	bool data;

	// Indicates if the stream has screen activated
	bool screen;
	bool local;

	//	bool recording;

	rtc::scoped_refptr<Connection> connection;

	int64_t streamID;

	rtc::scoped_refptr<webrtc::MediaStreamInterface> mediaStream;

public:
	Stream(bool video, bool audio, bool data, bool screen, bool local, int64_t streamID = 0);
	~Stream();
	void init();
	void close();
	bool isLocal();
	bool hasVideo();
	bool hasAudio();
	bool hasData();
	bool hasScreen();
	void setStreamID(int64_t streamID);
	int64_t getStreamID();
	rtc::scoped_refptr<Connection> getConnection();
	rtc::scoped_refptr<webrtc::MediaStreamInterface> getMediaStream();
};

class StreamEvent
{
public:
	static std::string STREAM_ADDED;
	static std::string STREAM_REMOVED;
	static std::string STREAM_SUBSCRIBED;
	static std::string STREAM_FAILED;
};
