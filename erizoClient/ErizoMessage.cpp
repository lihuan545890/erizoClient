#include "ErizoMessage.h"

using namespace sio;
using namespace std;

string SUCCESS_CODE = "success";
string ERROR_CODE = "error";

/*---------------------------------------------------------------------------*/

string TokenMock::TOKENID = "tokenId";
string TokenMock::HOST = "host";
string TokenMock::SECURE = "secure";

TokenMock::TokenMock(string tokenId, string host, bool secure)
{
	this->tokenId = tokenId;
	this->host = host;
	this->secure = secure;
}

message::ptr TokenMock::getObjectMessage() const
{
	message::ptr tokenMockPtr = object_message::create();

	tokenMockPtr->get_map()[TOKENID] = string_message::create(tokenId);
	tokenMockPtr->get_map()[HOST] = string_message::create(host);
	tokenMockPtr->get_map()[SECURE] = bool_message::create(secure);

	return tokenMockPtr;
}

/*---------------------------------------------------------------------------*/

string XtToken::USER = "user";
string XtToken::TOKEN = "token";
string XtToken::SESSIONID = "sessionId";
string XtToken::SESSIONMGRURL = "sessionMgrUrl";
string XtToken::CLIENTTYPE = "clientType";
string XtToken::CLIENTVER = "clientVer";

XtToken::XtToken(string user, string token, string sessionId, string sessionMgrUrl, int64_t clientType, int64_t clientVer)
{
	this->user = user;
	this->token = token;
	this->sessionId = sessionId;
	this->sessionMgrUrl = sessionMgrUrl;
	this->clientType = clientType;
	this->clientVer = clientVer;
}

sio::message::ptr XtToken::getObjectMessage() const
{
	message::ptr xtTokenPtr = object_message::create();

	xtTokenPtr->get_map()[USER] = string_message::create(user);
	xtTokenPtr->get_map()[TOKEN] = string_message::create(token);
	xtTokenPtr->get_map()[SESSIONID] = string_message::create(sessionId);
	xtTokenPtr->get_map()[SESSIONMGRURL] = string_message::create(sessionMgrUrl);
	xtTokenPtr->get_map()[CLIENTTYPE] = int_message::create(clientType);
	xtTokenPtr->get_map()[CLIENTVER] = int_message::create(clientVer);

	return xtTokenPtr;
}

/*---------------------------------------------------------------------------*/

string TokenResp::P2P = "p2p";
string TokenResp::MAXVIDEOBW = "maxVideoBW";
string TokenResp::DEFAULTVIDEOBW = "defaultVideoBW";
string TokenResp::ID = "id";
string TokenResp::STREAMS = "streams";
string TokenResp::ICESERVERS = "iceServers";

TokenResp::TokenResp(const message::ptr& msg)
{
	if (msg != NULL)
	{
		// bool p2p = msg->get_map()[P2P]->get_bool();
		maxVideoBW = msg->get_map()[MAXVIDEOBW]->get_int();
		defaultVideoBW = msg->get_map()[DEFAULTVIDEOBW]->get_int();
		id = msg->get_map()[ID]->get_string();
		streams = msg->get_map()[STREAMS]->get_vector();
		iceServers = msg->get_map()[ICESERVERS]->get_vector();
	}
}

int64_t TokenResp::getMaxVideoBW()
{
	return maxVideoBW;
}

int64_t TokenResp::getDefaultVideoBW()
{
	return defaultVideoBW;
}

string TokenResp::getId()
{
	return id;
}

vector<message::ptr> TokenResp::getStreams()
{
	return streams;
}

vector<message::ptr> TokenResp::getIceServers()
{
	return iceServers;
}

/*---------------------------------------------------------------------------*/

string PublishReq::VIDEO = "video";
string PublishReq::AUDIO = "audio";
string PublishReq::DATA = "data";
string PublishReq::SCREEN = "screen";
string PublishReq::MINVIDEOBW = "minVideoBW";
string PublishReq::STATE = "state";

PublishReq::PublishReq(bool video, bool audio, bool data, bool screen, int64_t minVideoBW, string state)
{
	this->video = video;
	this->audio = audio;
	this->data = data;
	this->screen = screen;
	this->minVideoBW = minVideoBW;
	this->state = state;
}

message::ptr PublishReq::getObjectMessage() const
{
	message::ptr publishReqPtr = object_message::create();

	publishReqPtr->get_map()[VIDEO] = bool_message::create(video);
	publishReqPtr->get_map()[AUDIO] = bool_message::create(audio);
	publishReqPtr->get_map()[DATA] = bool_message::create(data);
	publishReqPtr->get_map()[SCREEN] = bool_message::create(screen);
	publishReqPtr->get_map()[MINVIDEOBW] = int_message::create(minVideoBW);
	publishReqPtr->get_map()[STATE] = string_message::create(state);

	return publishReqPtr;
}

/*---------------------------------------------------------------------------*/

string SdpMsg::TYPE = "type";
string SdpMsg::SDP = "sdp";

SdpMsg::SdpMsg(string type, string sdp)
{
	this->type = type;
	this->sdp = sdp;
}

message::ptr SdpMsg::getObjectMessage() const
{
	message::ptr sdpMsg = object_message::create();

	sdpMsg->get_map()[TYPE] = string_message::create(type);
	sdpMsg->get_map()[SDP] = string_message::create(sdp);

	return sdpMsg;
}

/*---------------------------------------------------------------------------*/

string CandidateMsg::SDPMID = "sdpMid";
string CandidateMsg::SDPMLINEINDEX = "sdpMLineIndex";
string CandidateMsg::CANDIDATE = "candidate";
string CandidateMsg::TYPE = "type";

CandidateMsg::CandidateMsg(int sdpMLineIndex, string sdpMid, string candidate)
{
	this->sdpMLineIndex = sdpMLineIndex;
	this->sdpMid = sdpMid;
	this->candidate = candidate;
}

message::ptr CandidateMsg::getObjectMessage() const
{
	message::ptr candidateMsg = object_message::create();
	message::ptr msg = object_message::create();

	msg->get_map()[SDPMLINEINDEX] = int_message::create(sdpMLineIndex);
	msg->get_map()[SDPMID] = string_message::create(sdpMid);
	msg->get_map()[CANDIDATE] = string_message::create(candidate);

	candidateMsg->get_map()[TYPE] = string_message::create(CANDIDATE);
	candidateMsg->get_map()[CANDIDATE] = msg;

	return candidateMsg;
}

/*---------------------------------------------------------------------------*/

string SignalingMsgReq::STREAMID = "streamId";
string SignalingMsgReq::MSG = "msg";

SignalingMsgReq::SignalingMsgReq(int64_t streamId, sio::message::ptr msg)
{
	this->streamId = streamId;
	this->msg = msg;
}

message::ptr SignalingMsgReq::getObjectMessage() const
{
	message::ptr signalingMsgReq = object_message::create();

	signalingMsgReq->get_map()[MSG] = msg;
	signalingMsgReq->get_map()[STREAMID] = int_message::create(streamId);

	return signalingMsgReq;
}
