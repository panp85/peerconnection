
#ifndef TEST_FILE_CAPTURER_H_
#define TEST_FILE_CAPTURER_H_

#include <memory>
#include <vector>

#include "api/scoped_refptr.h"
#include "modules/video_capture/video_capture.h"
#include "test/test_video_capturer.h"

#include "ffmpeg-process.h"
//namespace webrtc {
//namespace test {

class FileCapturer : public webrtc::test::TestVideoCapturer,
                    public rtc::VideoSinkInterface<webrtc::VideoFrame> {
 public:
  static FileCapturer* Create(size_t width,
                             size_t height,
                             size_t target_fps,
                             size_t capture_device_index);
  virtual ~FileCapturer();

  void OnFrame(const webrtc::VideoFrame& frame) override;

 private:
  FileCapturer();
  bool Init(size_t width,
            size_t height,
            size_t target_fps,
            size_t capture_device_index);
  void Destroy();

  rtc::scoped_refptr<webrtc::VideoCaptureModule> vcm_;
//  VideoCaptureCapability capability_;


  FfmpegMediaProcess* ffmpegMediaProcess;
};

//}  // namespace test
//}  // namespace webrtc

#endif  // TEST_FILE_CAPTURER_H_
