#include "screen_video_capturer.h"

#include "webrtc/base/bind.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/scoped_ref_ptr.h"
#include "webrtc/base/logging.h"

#include "media/base/videocapturer.h"
#include "media/engine/webrtcvideoframefactory.h"
#include "webrtc/modules/desktop_capture/shared_memory.h"
//#include "webrtc/modules/desktop_capture/screen_capturer.h"
#include "webrtc/modules/desktop_capture/desktop_region.h"
#include "webrtc/modules/desktop_capture/desktop_frame.h"
#include "webrtc/modules/desktop_capture/desktop_capture_options.h"

#include "ErizoLog.h"
using namespace std;

#if WE_UNDER_APPLE
#import <ApplicationServices/ApplicationServices.h>
#define MAX_DISPLAYS    32
#endif /* WE_UNDER_APPLE */

#if !defined(kDoubangoSharedMemoryId)
#	define kDoubangoSharedMemoryId 85697421
#endif /* kDoubangoSharedMemoryId */

static const int64 kNumNanoSecsPerMilliSec = 1000000;
static const int kDefaultScreencastFps = 25;
static const int kLowScreencastFps = 7;

class _SharedMemory : public webrtc::SharedMemory {
public:
	_SharedMemory(char* buffer, size_t size)
		: SharedMemory(buffer, size, 0, kDoubangoSharedMemoryId),
		buffer_(buffer) {
	}
	virtual ~_SharedMemory() {
		if (buffer_) delete[] buffer_;
	}
private:
	char* buffer_;
	DISALLOW_COPY_AND_ASSIGN(_SharedMemory);
};

class _ScreenVideoCapturerThread
	: public rtc::Thread, public rtc::MessageHandler {
public:
	explicit _ScreenVideoCapturerThread(_ScreenVideoCapturer* capturer);

	virtual ~_ScreenVideoCapturerThread();

	// Override virtual method of parent Thread. Context: Worker Thread.
	virtual void Run();

	// Override virtual method of parent MessageHandler. Context: Worker Thread.
	virtual void OnMessage(rtc::Message* /*pmsg*/);

	// Check if Run() is finished.
	bool Finished() const;

private:
	_ScreenVideoCapturer* capturer_;
	mutable rtc::CriticalSection crit_;
	bool finished_;

	DISALLOW_COPY_AND_ASSIGN(_ScreenVideoCapturerThread);
};


_ScreenVideoCapturer::_ScreenVideoCapturer()
	: running_(false),
	capture_(webrtc::ScreenCapturer::Create(webrtc::DesktopCaptureOptions::CreateDefault())),
	capture_thread_(NULL),
	startThread_(NULL),
	start_time_ms_(0),
	last_frame_timestamp_ns_(0),
	frame_timestamp_(0),
	delay(0),
	min_delta_ts(INT64_MAX),
	clientType(DEFAULT),
	fps(kDefaultScreencastFps),
	fps_low(0),
	skip_frames(0),
	active(true),
	rotation_(webrtc::kVideoRotation_0) {

#ifdef HAVE_WEBRTC_VIDEO
	set_frame_factory(new cricket::WebRtcVideoFrameFactory());
#endif

	memset(&curr_frame_, 0, sizeof(curr_frame_));
#if CONFIG_FILE
	int32_t value;
	if (rtc::ReadConfig("fps", &value))
	{
		if (value > 0 && value <= kDefaultScreencastFps)
			fps = value;
	}
#endif
	// Default supported formats. Use ResetSupportedFormats to over write.
	std::vector<cricket::VideoFormat> formats;
#if WE_UNDER_WINDOWS
	static enum cricket::FourCC defaultFourCC = cricket::FOURCC_ARGB;
	formats.push_back(cricket::VideoFormat(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
		cricket::VideoFormat::FpsToInterval(fps), defaultFourCC));
#elif WE_UNDER_APPLE
	static enum cricket::FourCC defaultFourCC = cricket::FOURCC_ARGB;
	CGDisplayCount displayCount;
	CGDirectDisplayID displays[MAX_DISPLAYS];
	if (CGGetActiveDisplayList(sizeof(displays) / sizeof(displays[0]), displays, &displayCount) == kCGErrorSuccess) {
		for (CGDisplayCount i = 0; i<displayCount; i++) {
			// make a snapshot of the current display
			// CGImageRef image = CGDisplayCreateImage(displays[i]);
			formats.push_back(cricket::VideoFormat((int)CGDisplayPixelsWide(displays[i]), (int)CGDisplayPixelsHigh(displays[i]),
				cricket::VideoFormat::FpsToInterval(fps), defaultFourCC));
		}
	}
#endif
	formats.push_back(cricket::VideoFormat(1280, 720,
		cricket::VideoFormat::FpsToInterval(fps), defaultFourCC));
	formats.push_back(cricket::VideoFormat(640, 480,
		cricket::VideoFormat::FpsToInterval(fps), defaultFourCC));
	formats.push_back(cricket::VideoFormat(320, 240,
		cricket::VideoFormat::FpsToInterval(fps), defaultFourCC));
	formats.push_back(cricket::VideoFormat(160, 120,
		cricket::VideoFormat::FpsToInterval(fps), defaultFourCC));
	ResetSupportedFormats(formats);
}

_ScreenVideoCapturer::~_ScreenVideoCapturer() {
	SignalDestroyed(this);
	capture_ = NULL;
	capture_thread_ = NULL;
	next_frame_ = NULL;
	/* curr_frame_ is a struct without allocated pointers */
}

void _ScreenVideoCapturer::ResetSupportedFormats(const std::vector<cricket::VideoFormat>& formats) {
	SetSupportedFormats(formats);
}
bool _ScreenVideoCapturer::GetNextFrame(int* waiting_time_ms)
{
	if (!active)
	{
	//	ERIZOLOG_DEBUG << "Send NULL frame on inactive status!" << endl;
		OnCaptureCompleted(NULL);
		*waiting_time_ms = (1000 / GetCaptureFormat()->framerate());
		return true;
	}
	start_time_ms_ = rtc::Time();
	if (!CaptureFrame())
		return false;
	int32_t capture_time = (int32_t)(rtc::Time() - start_time_ms_);	

//	ERIZOLOG_DEBUG << "clientType = " << clientType << ", fps_low = " << fps_low << endl;
	if (fps_low)
		*waiting_time_ms = (1000 / kLowScreencastFps) - capture_time;
	else
		*waiting_time_ms = (1000 / GetCaptureFormat()->framerate()) - capture_time;
	
	if (*waiting_time_ms < 0) *waiting_time_ms = 0;

	return true;
}
bool _ScreenVideoCapturer::CaptureFrame() {

	capture_->Capture(webrtc::DesktopRegion());
	return IsRunning();
}

cricket::CaptureState _ScreenVideoCapturer::Start(const cricket::VideoFormat& format) {
	if (running_) {
		ERIZOLOG_TRACE << "Screen video capturer '" << GetId() << "' already running" << endl;
		SetCaptureState(cricket::CS_FAILED);
		return cricket::CS_FAILED;
	}

	cricket::VideoFormat supported;
	if (GetBestCaptureFormat(format, &supported)) {
		SetCaptureFormat(&supported);
	}

	// Create the capture thread
	capture_thread_ = new _ScreenVideoCapturerThread(this);

	SetCaptureState(cricket::CS_RUNNING);
	capture_->Start(this);

	// Keep track of which thread capture started on. This is the thread that
	// frames need to be sent to.
	//DCHECK(!startThread_);
	startThread_ = rtc::Thread::Current();

	bool ret = capture_thread_->Start();
	if (ret) {
		ERIZOLOG_TRACE << "Screen video capturer '" << GetId() << "' started" << endl;
	}
	else {
		ERIZOLOG_TRACE << "Screen video capturer '" << GetId() << "' failed to start" << endl;
		SetCaptureState(cricket::CS_FAILED);
		SetCaptureFormat(NULL);
		return cricket::CS_FAILED;
	}

	running_ = true;
	return cricket::CS_RUNNING;
}
void _ScreenVideoCapturer::Stop() {
	running_ = false;
	if (capture_thread_) {
		capture_thread_->Stop();
		capture_thread_ = NULL;
		ERIZOLOG_TRACE << "Screen video capturer '" << GetId() << "' stopped" << endl;
	}
	SetCaptureFormat(NULL);
	SetCaptureState(cricket::CS_STOPPED);
	startThread_ = NULL;
}
bool _ScreenVideoCapturer::IsRunning() {
	return running_ && capture_thread_ && !capture_thread_->Finished();
}
bool _ScreenVideoCapturer::IsScreencast() const { return true; }
bool _ScreenVideoCapturer::GetPreferredFourccs(std::vector<uint32>* fourccs) {
	fourccs->push_back(cricket::FOURCC_I420);
	fourccs->push_back(cricket::FOURCC_ARGB);
	fourccs->push_back(cricket::FOURCC_RGBA);
	return true;
}

void _ScreenVideoCapturer::SetRotation(webrtc::VideoRotation rotation) {
	rotation_ = rotation;
}

void _ScreenVideoCapturer::SetNextFrame(webrtc::DesktopFrame* next_frame) {
	next_frame_.reset(next_frame);
}

bool _ScreenVideoCapturer::GetScreenList(webrtc::ScreenCapturer::ScreenList* screens) {
	return capture_->GetScreenList(screens);
}

bool _ScreenVideoCapturer::SelectScreen(webrtc::ScreenId id) {
	return capture_->SelectScreen(id);
}

webrtc::VideoRotation _ScreenVideoCapturer::GetRotation() { return rotation_; }

// webrtc::ScreenCapturer::Callback::CreateSharedMemory
webrtc::SharedMemory* _ScreenVideoCapturer::CreateSharedMemory(size_t size)
{
	return new _SharedMemory(new char[size], size);
}

// webrtc::ScreenCapturer::Callback::OnCaptureCompleted
void _ScreenVideoCapturer::OnCaptureCompleted(webrtc::DesktopFrame* desktopFrame)
{
	if (IsRunning() && desktopFrame) {
		uint32 data_size = 0;
		if (!GetCaptureFormat()) {
			LOG(LS_ERROR) << "Screen video capturer '" << GetId() << "' GetCaptureFormat failed";
			goto next;
		}
		if (GetCaptureFormat()->fourcc == cricket::FOURCC_ARGB || GetCaptureFormat()->fourcc == cricket::FOURCC_ARGB) {
			data_size = desktopFrame->size().width() * 4 * desktopFrame->size().height();
			if (!fps_low && desktopFrame->updated_region().is_empty()) {
				if (skip_frames >= 5)
					skip_frames = 0;
				else {
					skip_frames++;
				//	ERIZOLOG_DEBUG << "skip a frame, skip_frames = " << skip_frames << endl;
					return;
				}
			}
			skip_frames = 0;
		//	ERIZOLOG_DEBUG << "capture a frame, skip_frames = " << skip_frames << endl;
		}
		else if (GetCaptureFormat()->fourcc == cricket::FOURCC_I420) {
			size_t w = desktopFrame->size().width();
			size_t h = desktopFrame->size().height();
			data_size = w * h + ((w + 1) / 2) * ((h + 1) / 2) * 2;
			// data_size = static_cast<uint32>(cricket::VideoFrame::SizeOf(desktopFrame->size().width(), desktopFrame->size().height()));
		}
		else {
			LOG(LS_ERROR) << "Screen video capturer '" << GetId() << "' Unsupported FOURCC " << GetCaptureFormat()->fourcc;
			goto next;
		}
		curr_frame_.width = desktopFrame->size().width();
		curr_frame_.height = desktopFrame->size().height();
		curr_frame_.fourcc = GetCaptureFormat()->fourcc;
		curr_frame_.data_size = data_size;
		curr_frame_.data = desktopFrame->data();
		frame_timestamp_ = static_cast<int64>(rtc::Time());
		curr_frame_.time_stamp = kNumNanoSecsPerMilliSec * start_time_ms_;

#if XT_STATISTIC
		static int frames = 0;
		static int64_t totaltime = 0;
		static int64_t capture_start_time = static_cast<int64>(rtc::Time());
		int fps;
		frames++;
		totaltime += curr_frame_.elapsed_time;
		if (frames % INTERVAL == 0)
		{
			char str[_MAX_PATH];
			fps = INTERVAL * 1000 / (static_cast<int64>(rtc::Time()) - capture_start_time);
			::_snprintf(str, _MAX_PATH, "OnCaptureCompleted(). Captured frames =%d, \tcapture fps = %d, \tAverage caputre time = %d ms\r\n", frames, fps, totaltime / INTERVAL);
			webrtc::XTLOG_FILE(str);
			totaltime = 0;
			capture_start_time = static_cast<int64>(rtc::Time());
		}
#endif

		if (startThread_->IsCurrent()) {
			SignalFrameCaptured(this, &curr_frame_);
		}
		else {
			startThread_->Invoke<void>(rtc::Bind(&_ScreenVideoCapturer::SignalFrameCapturedOnStartThread, this, &curr_frame_));
		}
	}
next:
	SetNextFrame(desktopFrame);
}

// Used to signal frame capture on the thread that capturer was started on.
void _ScreenVideoCapturer::SignalFrameCapturedOnStartThread(const cricket::CapturedFrame* frame) {
	//DCHECK(startThread_->IsCurrent());
	SignalFrameCaptured(this, frame);
}

void _ScreenVideoCapturer::setDelay(uint64_t delay_, uint64_t clientType) {
	if ((clientType >= 0) && (clientType <= 5))
		this->clientType = (CLIENTTYPE)clientType;
	
//	ERIZOLOG_DEBUG << "clientType = " << clientType << ", delay = " << delay_ << endl;
	if (delay_ >= 0)
	{
		if ((clientType == ANDROID) || (clientType == IOS))
		{
			int64 delta_ts = rtc::Time() - delay_ / 90;
			if (delta_ts > min_delta_ts)
				delay = delta_ts - min_delta_ts;
			else {
				min_delta_ts = delta_ts;
				delay = 0;
			}
//			ERIZOLOG_DEBUG << "min_delta_ts = " << min_delta_ts << ", delta_ts = " << delta_ts << ", delay = " << delay << ", delay_ = " << delay_ << endl;
		}
		else if (clientType == WEB)
		{
			delay = delay_;
		}

		// delay based frame skip
		if (!(fps_low) && (delay >= 300))
		{
			fps_low = 1;
			ERIZOLOG_TRACE << "clientType = " << clientType << ", set fps_low = 1, delay =" << delay << endl;
		}
		else if ((fps_low) && (delay < 130))
		{
			fps_low = 0;
			ERIZOLOG_TRACE << "clientType = " << clientType << ", set fps_low = 0, delay =" << delay << endl;
		}			

//		if(delay > 500)
//			ERIZOLOG_TRACE <<"clientType = " << clientType << ", delay = " << delay << endl;
	}
}

void _ScreenVideoCapturer::activateSession(bool active_)
{
	active = active_;
	ERIZOLOG_TRACE << "Set session active status to  " << active << endl;
}
//
//	_ScreenVideoCapturerFactory
//
_ScreenVideoCapturerFactory::_ScreenVideoCapturerFactory()
	: screen_capturer_(NULL),
	capture_state_(cricket::CS_STOPPED) {
}

cricket::VideoCapturer* _ScreenVideoCapturerFactory::Create(const cricket::ScreencastId& screen) {
	if (screen_capturer_ != NULL) {
		return NULL;
	}
	screen_capturer_ = new _ScreenVideoCapturer;
	screen_capturer_->SelectScreen(webrtc::ScreenId(screen.window().id()));
	screen_capturer_->SignalDestroyed.connect(
		this,
		&_ScreenVideoCapturerFactory::OnscreenCapturerDestroyed);
	screen_capturer_->SignalStateChange.connect(
		this,
		&_ScreenVideoCapturerFactory::OnStateChange);

	return screen_capturer_;
}

_ScreenVideoCapturer* _ScreenVideoCapturerFactory::screen_capturer() {
	return screen_capturer_;
}

cricket::CaptureState _ScreenVideoCapturerFactory::capture_state() {
	return capture_state_;
}

void _ScreenVideoCapturerFactory::OnscreenCapturerDestroyed(_ScreenVideoCapturer* capturer) {
	if (capturer == screen_capturer_) {
		screen_capturer_ = NULL;
	}
}

void _ScreenVideoCapturerFactory::OnStateChange(cricket::VideoCapturer*, cricket::CaptureState state) {
	capture_state_ = state;
}

bool _ScreenVideoCapturerFactory::GetScreenList(webrtc::ScreenCapturer::ScreenList* screens)
{
	_ScreenVideoCapturer* capturer = new _ScreenVideoCapturer;
	if (capturer) {
		bool ok = capturer->GetScreenList(screens);
		delete capturer;
		return ok;
	}
	return false;
}


//
// _ScreenVideoCapturerThread
//
_ScreenVideoCapturerThread::_ScreenVideoCapturerThread(_ScreenVideoCapturer* capturer)
	: capturer_(capturer),
	finished_(false) {
}

_ScreenVideoCapturerThread:: ~_ScreenVideoCapturerThread() {
	Stop();
}

// Override virtual method of parent Thread. Context: Worker Thread.
void _ScreenVideoCapturerThread::Run() {
	// Read the first frame and start the message pump. The pump runs until
	// Stop() is called externally or Quit() is called by OnMessage().
	int waiting_time_ms = 0;
	if (capturer_ && capturer_->GetNextFrame(&waiting_time_ms)) {
		PostDelayed(waiting_time_ms, this);
		Thread::Run();
	}

	rtc::CritScope cs(&crit_);
	finished_ = true;
}

// Override virtual method of parent MessageHandler. Context: Worker Thread.
void _ScreenVideoCapturerThread::OnMessage(rtc::Message* /*pmsg*/) {
	int waiting_time_ms = 0;
	if (capturer_ && capturer_->GetNextFrame(&waiting_time_ms)) {
		PostDelayed(waiting_time_ms, this);
	}
	else {
		Quit();
	}
}

// Check if Run() is finished.
bool _ScreenVideoCapturerThread::Finished() const {
	rtc::CritScope cs(&crit_);
	return finished_;
}

