#ifndef _WEBRTC_COMMON_SCREENVIDEOCAPTURER_H_
#define _WEBRTC_COMMON_SCREENVIDEOCAPTURER_H_

#include "config.h"
#include "_Config.h"

#include "media/base/videocapturer.h"
#include "media/base/videocapturerfactory.h"
#include "webrtc/modules/desktop_capture/screen_capturer.h"

#include "webrtc/system_wrappers/include/clock.h"

class _ScreenVideoCapturerThread;

typedef enum 
{
	DEFAULT = 0,
	WIN = 1,
	MAC = 2,
	IOS = 3,
	ANDROID = 4,
	WEB = 5
}CLIENTTYPE;

class _ScreenVideoCapturer : public cricket::VideoCapturer, public webrtc::DesktopCapturer::Callback {
public:
	_ScreenVideoCapturer();
	~_ScreenVideoCapturer();
	void ResetSupportedFormats(const std::vector<cricket::VideoFormat>& formats);
	bool GetNextFrame(int* waiting_time_ms);
	bool CaptureFrame();

	sigslot::signal1<_ScreenVideoCapturer*> SignalDestroyed;

	virtual cricket::CaptureState Start(const cricket::VideoFormat& format);
	virtual void Stop();
	virtual bool IsRunning();
	virtual bool IsScreencast() const;
	bool GetPreferredFourccs(std::vector<uint32>* fourccs);
	void SetRotation(webrtc::VideoRotation rotation);
	void SetNextFrame(webrtc::DesktopFrame* next_frame);
	bool GetScreenList(webrtc::ScreenCapturer::ScreenList* screens);
	bool SelectScreen(webrtc::ScreenId id);
	webrtc::VideoRotation GetRotation();
	virtual webrtc::SharedMemory* CreateSharedMemory(size_t size);
	virtual void OnCaptureCompleted(webrtc::DesktopFrame* desktopFrame);
	void SignalFrameCapturedOnStartThread(const cricket::CapturedFrame* frame);
	void setDelay(uint64_t delay_, uint64_t clientType);
	void activateSession(bool _active);

private:
	bool running_;
	webrtc::VideoRotation rotation_;
	std::unique_ptr<webrtc::ScreenCapturer> capture_;
	_ScreenVideoCapturerThread* capture_thread_;
	std::unique_ptr<webrtc::DesktopFrame> next_frame_;
	cricket::CapturedFrame curr_frame_;
	rtc::Thread* startThread_;  // Set in Start(), unset in Stop().
	int64 start_time_ms_;  // Time when the video capturer starts.
	int64 last_frame_timestamp_ns_;  // Timestamp of last frame
	int64 frame_timestamp_;  // Timestamp of this frame
	uint64_t delay;  // delay from client
	int64 min_delta_ts;
	CLIENTTYPE clientType;
	uint32_t fps;  // normal fps
	bool fps_low;  // is current in low fps status, low fps when delay > 300ms
	int skip_frames;
	bool active; // current session active status;
	DISALLOW_COPY_AND_ASSIGN(_ScreenVideoCapturer);
};

class _ScreenVideoCapturerFactory : public cricket::ScreenCapturerFactory, public sigslot::has_slots < > {
public:
	_ScreenVideoCapturerFactory();

	virtual cricket::VideoCapturer* Create(const cricket::ScreencastId& screen);

	_ScreenVideoCapturer* screen_capturer();

	cricket::CaptureState capture_state();

	static bool GetScreenList(webrtc::ScreenCapturer::ScreenList* screens);

private:
	void OnscreenCapturerDestroyed(_ScreenVideoCapturer* capturer);
	void OnStateChange(cricket::VideoCapturer*, cricket::CaptureState state);

	_ScreenVideoCapturer* screen_capturer_;
	cricket::CaptureState capture_state_;
};

#endif /* _WEBRTC_COMMON_SCREENVIDEOCAPTURER_H_ */
