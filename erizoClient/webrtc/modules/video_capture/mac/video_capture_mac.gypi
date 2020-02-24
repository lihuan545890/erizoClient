# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.
{
  'targets': [
    {
      'target_name': 'video_capture_module_mac_qtkit',
      'type': 'static_library',
      'dependencies': [
            'video_capture_module',
            '<(webrtc_root)/common.gyp:webrtc_common',
          ],
      'sources': [
        'qtkit/video_capture_qtkit.h',
        'qtkit/video_capture_qtkit.mm',
        'qtkit/video_capture_qtkit_info.h',
        'qtkit/video_capture_qtkit_info.mm',
        'qtkit/video_capture_qtkit_info_objc.h',
        'qtkit/video_capture_qtkit_info_objc.mm',
        'qtkit/video_capture_qtkit_objc.h',
        'qtkit/video_capture_qtkit_objc.mm',
        'qtkit/video_capture_qtkit_utility.h',
      ],
      'link_settings': {
        'xcode_settings': {
          'OTHER_LDFLAGS': [
            '-framework Cocoa',
            '-framework CoreVideo',
            '-framework QTKit',
          ],
         },
       },
    },#mac QTkit
    {
      'target_name': 'video_capture_module_mac_avfoundation',
      'type': 'static_library',
      'dependencies': [
            'video_capture_module',
            '<(webrtc_root)/common.gyp:webrtc_common',
        ],
      'sources': [
        'avf/device_info_avf.h',
        'avf/device_info_avf.mm',
        'avf/device_info_avf_objc.h',
        'avf/device_info_avf_objc.mm',
        'avf/rtc_video_capture_avf_objc.h',
        'avf/rtc_video_capture_avf_objc.mm',
        'avf/video_capture_avf.h',
        'avf/video_capture_avf.mm',
      ],
      'xcode_settings': {
        'CLANG_ENABLE_OBJC_ARC': 'YES',
        # Need to build against 10.7 framework for full ARC support
        # on OSX.
        'MACOSX_DEPLOYMENT_TARGET' : '10.7',
        # Video capture on OSX uses AVFoundation components with partial availability.
        # https://code.google.com/p/webrtc/issues/detail?id=4695
        'WARNING_CFLAGS!': ['-Wpartial-availability'],
       },
      'link_settings': {
        'xcode_settings': {
          'OTHER_LDFLAGS': [
            '-framework AVFoundation',
            '-framework CoreMedia',
            '-framework Cocoa',
            '-framework CoreVideo',
          ],
        },
      },
    },#mac AVFoundation
  ], # targets
 }
