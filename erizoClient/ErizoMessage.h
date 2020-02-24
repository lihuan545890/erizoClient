#pragma once

#include "sio/include/sio_client.h"

extern std::string SUCCESS_CODE;
extern std::string ERROR_CODE;

/*---------------------------------------------------------------------------*/

class TokenMock
{
private:
	bool secure;
	std::string tokenId;
	std::string host;

	static std::string TOKENID;
	static std::string HOST;
	static std::string SECURE;

public:
	TokenMock(std::string tokenId, std::string host, bool secure);

	sio::message::ptr getObjectMessage() const;
};

/*---------------------------------------------------------------------------*/

class XtToken
{
private:
	std::string user;
	std::string token;
	std::string sessionId;
	std::string sessionMgrUrl;
	int64_t clientType;
	int64_t clientVer;

	static std::string USER;
	static std::string TOKEN;
	static std::string SESSIONID;
	static std::string SESSIONMGRURL;
	static std::string CLIENTTYPE;
	static std::string CLIENTVER;

public:
	XtToken(std::string user, std::string token, std::string sessionId, std::string sessionMgrUrl, int64_t clientType, int64_t clientVer);

	sio::message::ptr getObjectMessage() const;
};

/*---------------------------------------------------------------------------*/

class TokenResp
{
private:
	// bool p2p;
	int64_t maxVideoBW;
	int64_t defaultVideoBW;
	std::string id; // room id
	std::vector<sio::message::ptr> streams;
	std::vector<sio::message::ptr> iceServers;

	static std::string P2P;
	static std::string MAXVIDEOBW;
	static std::string DEFAULTVIDEOBW;
	static std::string ID;
	static std::string STREAMS;
	static std::string ICESERVERS;

public:
	TokenResp(const sio::message::ptr& msg);
	int64_t getMaxVideoBW();
	int64_t getDefaultVideoBW();
	std::string getId();
	std::vector<sio::message::ptr> getStreams();
	std::vector<sio::message::ptr> getIceServers();
};

/*---------------------------------------------------------------------------*/

class PublishReq
{
private:
	bool video;
	bool audio;
	bool data;
	bool screen;
	int64_t minVideoBW;
	std::string state;

	static std::string VIDEO;
	static std::string AUDIO;
	static std::string DATA;
	static std::string SCREEN;
	static std::string MINVIDEOBW;
	static std::string STATE;

public:
	PublishReq(bool video, bool audio, bool data, bool screen, int64_t minVideoBW, std::string state);

	sio::message::ptr getObjectMessage() const;
};

/*---------------------------------------------------------------------------*/

class SdpMsg
{
private:
	std::string type;
	std::string sdp;
	
	static std::string TYPE;
	static std::string SDP;

public:
	SdpMsg(std::string type, std::string sdp);

	sio::message::ptr getObjectMessage() const;
};

/*---------------------------------------------------------------------------*/

class CandidateMsg
{
private:
	int64_t sdpMLineIndex;
	std::string sdpMid;
	std::string candidate;

	static std::string SDPMID;
	static std::string SDPMLINEINDEX;
	static std::string CANDIDATE;
	static std::string TYPE;

public:
	CandidateMsg(int sdpMLineIndex, std::string sdpMid, std::string candidate);
	sio::message::ptr getObjectMessage() const;
};

/*---------------------------------------------------------------------------*/

class SignalingMsgReq
{
private:
	int64_t streamId;
	sio::message::ptr msg;

	static std::string STREAMID;
	static std::string MSG;

public:
	SignalingMsgReq(int64_t streamId, sio::message::ptr msg);
	sio::message::ptr getObjectMessage() const;
};
