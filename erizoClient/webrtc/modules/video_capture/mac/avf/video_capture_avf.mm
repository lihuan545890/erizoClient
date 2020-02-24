/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#include "webrtc/base/refcount.h"
#include "webrtc/base/scoped_ref_ptr.h"
#include "webrtc/modules/video_capture/mac/avf/device_info_avf_objc.h"
#include "webrtc/modules/video_capture/mac/avf/rtc_video_capture_avf_objc.h"
#include "webrtc/system_wrappers/include/trace.h"

namespace webrtc {
  namespace videocapturemodule {

VideoCaptureAvf::VideoCaptureAvf(const int32_t capture_id)
    : VideoCaptureImpl(capture_id), is_capturing_(false), id_(capture_id) {
  capability_.width = kDefaultWidth;
  capability_.height = kDefaultHeight;
  capability_.maxFPS = kDefaultFrameRate;
  capture_device_ = nil;
}

VideoCaptureAvf::~VideoCaptureAvf() {
  if (is_capturing_) {
    [capture_device_ stopCapture];
    capture_device_ = nil;
  }
}

rtc::scoped_refptr<VideoCaptureModule> VideoCaptureAvf::Create(const int32_t capture_id,
                                            const char* deviceUniqueIdUTF8) {
  if (!deviceUniqueIdUTF8[0]) {
    return NULL;
  }

  rtc::scoped_refptr<VideoCaptureAvf> capture_module =
      new rtc::RefCountedObject<VideoCaptureAvf>(capture_id);

  const int32_t name_length = strlen(deviceUniqueIdUTF8);
  if (name_length > kVideoCaptureUniqueNameLength)
    return NULL;

  capture_module->_deviceUniqueId = new char[name_length + 1];
  strncpy(capture_module->_deviceUniqueId, deviceUniqueIdUTF8, name_length + 1);
  capture_module->_deviceUniqueId[name_length] = '\0';

  capture_module->capture_device_ =
      [[RTCVideoCaptureAvfObjC alloc] initWithOwner:capture_module
                                          captureId:capture_module->id_];
  if (!capture_module->capture_device_) {
    return NULL;
  }

  if (![capture_module->capture_device_ setCaptureDeviceByUniqueId:[
              [NSString alloc] initWithCString:deviceUniqueIdUTF8
                                      encoding:NSUTF8StringEncoding]]) {
    return NULL;
  }
  return capture_module;
}

int32_t VideoCaptureAvf::StartCapture(
    const VideoCaptureCapability& capability) {
  capability_ = capability;

  if (![capture_device_ startCaptureWithCapability:capability]) {
    return -1;
  }

  is_capturing_ = true;

  return 0;
}

int32_t VideoCaptureAvf::StopCapture() {
  if (![capture_device_ stopCapture]) {
    return -1;
  }

  is_capturing_ = false;
  return 0;
}

bool VideoCaptureAvf::CaptureStarted() { return is_capturing_; }

int32_t VideoCaptureAvf::CaptureSettings(VideoCaptureCapability& settings) {
  settings = capability_;
  settings.rawType = kVideoNV12;
  return 0;
}
    
}  // namespace videocapturemodule

}  // namespace webrtc
