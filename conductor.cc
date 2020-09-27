/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "examples/peerconnection/client/conductor.h"

#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <utility>
#include <vector>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <sstream>


#include "absl/memory/memory.h"
#include "absl/types/optional.h"
#include "api/audio/audio_mixer.h"
#include "api/audio_codecs/audio_decoder_factory.h"
#include "api/audio_codecs/audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/audio_options.h"
#include "api/create_peerconnection_factory.h"
#include "api/rtp_sender_interface.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "examples/peerconnection/client/defaults.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"
#include "p2p/base/port_allocator.h"
#include "pc/video_track_source.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/rtc_certificate_generator.h"
#include "rtc_base/strings/json.h"
#include "test/vcm_capturer.h"

#include "media/common/fileCapturer.h"

namespace {
// Names used for a IceCandidate JSON object.
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";

// Names used for a SessionDescription JSON object.
const char kSessionDescriptionTypeName[] = "type";
const char kSessionDescriptionSdpName[] = "sdp";

class DummySetSessionDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
 public:
  static DummySetSessionDescriptionObserver* Create() {
    return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
  }
  virtual void OnSuccess() { RTC_LOG(INFO) << __FUNCTION__; }
  virtual void OnFailure(webrtc::RTCError error) {
    RTC_LOG(INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": "
                  << error.message();
  }
};

class CapturerTrackSource : public webrtc::VideoTrackSource {
 public:
  static rtc::scoped_refptr<CapturerTrackSource> Create() {
    const size_t kWidth = 640;
    const size_t kHeight = 480;
    const size_t kFps = 15;
    std::unique_ptr<webrtc::test::VcmCapturer> capturer;
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
        webrtc::VideoCaptureFactory::CreateDeviceInfo());
    if (!info) {
      return nullptr;
    }
    int num_devices = info->NumberOfDevices();
    for (int i = 0; i < num_devices; ++i) {
      capturer = absl::WrapUnique(
          webrtc::test::VcmCapturer::Create(kWidth, kHeight, kFps, i));
      if (capturer) {
        return new
            rtc::RefCountedObject<CapturerTrackSource>(std::move(capturer));
      }
    }

    return nullptr;
  }

 protected:
  explicit CapturerTrackSource(
      std::unique_ptr<webrtc::test::VcmCapturer> capturer)
      : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}

 private:
  rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
    return capturer_.get();
  }
  std::unique_ptr<webrtc::test::VcmCapturer> capturer_;
};
#if 1
class FileCapturerTrackSource : public webrtc::VideoTrackSource {
 public:
  static rtc::scoped_refptr<FileCapturerTrackSource> Create() {
	const size_t kWidth = 640;
	const size_t kHeight = 480;
	const size_t kFps = 15;
	std::unique_ptr<FileCapturer> capturer;
	/*
	std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
		webrtc::VideoCaptureFactory::CreateDeviceInfo());
	if (!info) {
	  return nullptr;
	}
	
	int num_devices = info->NumberOfDevices();
	*/
	capturer = absl::WrapUnique(
		  FileCapturer::Create(kWidth, kHeight, kFps, 0));
	return new
			rtc::RefCountedObject<FileCapturerTrackSource>(std::move(capturer));
	
	//return nullptr;
  }
 protected:
  explicit FileCapturerTrackSource(
	  std::unique_ptr<FileCapturer> capturer)
	  : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}
 private:
  rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
	return capturer_.get();
  }
  std::unique_ptr<FileCapturer> capturer_;
};
#endif 


}  // namespace

Conductor::Conductor(PeerConnectionClient* client, MainWindow* main_wnd)
    : peer_id_(-1), loopback_(false), client_(client), main_wnd_(main_wnd) {
  client_->RegisterObserver(this);
  main_wnd->RegisterObserver(this);
}

Conductor::~Conductor() {
  RTC_DCHECK(!peer_connection_);
}

bool Conductor::connection_active() const {
  return peer_connection_ != nullptr;
}

void Conductor::Close() {
  client_->SignOut();
  client_->_janusImpl->close();
  DeletePeerConnection();
}

bool Conductor::InitializePeerConnection() {
  RTC_DCHECK(!peer_connection_factory_);
  RTC_DCHECK(!peer_connection_);

  peer_connection_factory_ = webrtc::CreatePeerConnectionFactory(
      nullptr /* network_thread */, nullptr /* worker_thread */,
      nullptr /* signaling_thread */, nullptr /* default_adm */,
      webrtc::CreateBuiltinAudioEncoderFactory(),
      webrtc::CreateBuiltinAudioDecoderFactory(),
      webrtc::CreateBuiltinVideoEncoderFactory(),
      webrtc::CreateBuiltinVideoDecoderFactory(), nullptr /* audio_mixer */,
      nullptr /* audio_processing */);

  if (!peer_connection_factory_) {
    main_wnd_->MessageBox("Error", "Failed to initialize PeerConnectionFactory",
                          true);
    DeletePeerConnection();
    return false;
  }

  if (!CreatePeerConnection(/*dtls=*/true)) {
    main_wnd_->MessageBox("Error", "CreatePeerConnection failed", true);
    DeletePeerConnection();
  }
 
  AddTracks();

  return peer_connection_ != nullptr;
}

void Conductor::setbitrate(){
  webrtc::PeerConnectionInterface::BitrateParameters bitrate;
  bitrate.min_bitrate_bps = 100;
  bitrate.current_bitrate_bps = 300;
  bitrate.max_bitrate_bps = 300;
  peer_connection_->SetBitrate(bitrate);
}

bool Conductor::ReinitializePeerConnectionForLoopback() {
  loopback_ = true;
  std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders =
      peer_connection_->GetSenders();
  peer_connection_ = nullptr;
  if (CreatePeerConnection(/*dtls=*/false)) {
    for (const auto& sender : senders) {
      peer_connection_->AddTrack(sender->track(), sender->stream_ids());
    }
    peer_connection_->CreateOffer(
        this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
  }
  return peer_connection_ != nullptr;
}

bool Conductor::CreatePeerConnection(bool dtls) {
  RTC_DCHECK(peer_connection_factory_);
  RTC_DCHECK(!peer_connection_);

  webrtc::PeerConnectionInterface::RTCConfiguration config;
  config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
  config.enable_dtls_srtp = dtls;
  webrtc::PeerConnectionInterface::IceServer server;
  server.uri = GetPeerConnectionString();
  webrtc::PeerConnectionInterface::IceServer server2;
  //server2.uri = "turn:139.196.204.25:3478";
  server2.uri = "turn:192.168.8.109:3478";
  server2.username = "ts";
  server2.password = "12345678";
  config.servers.push_back(server);
  config.servers.push_back(server2);

  peer_connection_ = peer_connection_factory_->CreatePeerConnection(
      config, nullptr, nullptr, this);
  return peer_connection_ != nullptr;
}

void Conductor::DeletePeerConnection() {
  main_wnd_->StopLocalRenderer();
  main_wnd_->StopRemoteRenderer();
  peer_connection_ = nullptr;
  peer_connection_factory_ = nullptr;
  peer_id_ = -1;
  loopback_ = false;
}

void Conductor::EnsureStreamingUI() {
  RTC_DCHECK(peer_connection_);
  if (main_wnd_->IsWindow()) {
    if (main_wnd_->current_ui() != MainWindow::STREAMING)
      main_wnd_->SwitchToStreamingUI();
  }
}

//
// PeerConnectionObserver implementation.
//

void Conductor::OnAddTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
    const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
        streams) {
  RTC_LOG(INFO) << __FUNCTION__ << " " << receiver->id();
  main_wnd_->QueueUIThreadCallback(NEW_TRACK_ADDED,
                                   receiver->track().release());
}

void Conductor::OnRemoveTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) {
  RTC_LOG(INFO) << __FUNCTION__ << " " << receiver->id();
  main_wnd_->QueueUIThreadCallback(TRACK_REMOVED, receiver->track().release());
}
/*
void Conductor::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
  RTC_LOG(INFO) << __FUNCTION__ << " " << candidate->sdp_mline_index();
  // For loopback test. To save some connecting delay.
  if (loopback_) {
    if (!peer_connection_->AddIceCandidate(candidate)) {
      RTC_LOG(WARNING) << "Failed to apply the received candidate";
    }
    return;
  }

  Json::StyledWriter writer;
  Json::Value jmessage;

  jmessage[kCandidateSdpMidName] = candidate->sdp_mid();
  jmessage[kCandidateSdpMlineIndexName] = candidate->sdp_mline_index();
  std::string sdp;
  if (!candidate->ToString(&sdp)) {
    RTC_LOG(LS_ERROR) << "Failed to serialize candidate";
    return;
  }
  jmessage[kCandidateSdpName] = sdp;
  RTC_LOG(LS_INFO) << "ppt, in Conductor::OnIceCandidate, jmessage: " << jmessage;
  SendMessage(writer.write(jmessage));
}
*/
void Conductor::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
  RTC_LOG(INFO) << __FUNCTION__ << " " << candidate->sdp_mline_index();
  // For loopback test. To save some connecting delay.
  if (loopback_) {
    if (!peer_connection_->AddIceCandidate(candidate)) {
      RTC_LOG(WARNING) << "Failed to apply the received candidate";
    }
    return;
  }

//  Json::StyledWriter writer;
//  Json::Value jmessage;

//  jmessage[kCandidateSdpMidName] = candidate->sdp_mid();
//  jmessage[kCandidateSdpMlineIndexName] = candidate->sdp_mline_index();
  std::string sdp;
  if (!candidate->ToString(&sdp)) {
    RTC_LOG(LS_ERROR) << "Failed to serialize candidate";
    return;
  }
//  jmessage[kCandidateSdpName] = sdp;
//  RTC_LOG(LS_INFO) << "ppt, in Conductor::OnIceCandidate, jmessage: " << jmessage;
  RTC_LOG(LS_INFO) << "ppt, in Conductor::OnIceCandidate, sdp: " << sdp;

  
  client_->onIceCandidate(candidate->sdp_mid(), candidate->sdp_mline_index(), sdp);
  //SendMessage(writer.write(jmessage));
}



void Conductor::OnIceGatheringChange(
    webrtc::PeerConnectionInterface::IceGatheringState new_state) {
    if(new_state != webrtc::PeerConnectionInterface::kIceConnectionCompleted) {
      return;
    }
    client_->onIceCompleted();
}


//
// PeerConnectionClientObserver implementation.
//

void Conductor::OnSignedIn() {
  RTC_LOG(INFO) << __FUNCTION__;
  main_wnd_->SwitchToPeerList(client_->peers());
}

void Conductor::OnDisconnected() {
  RTC_LOG(INFO) << __FUNCTION__;

  DeletePeerConnection();

  if (main_wnd_->IsWindow())
    main_wnd_->SwitchToConnectUI();
}

void Conductor::OnPeerConnected(int id, const std::string& name) {
  RTC_LOG(INFO) << __FUNCTION__;
  // Refresh the list if we're showing it.
  if (main_wnd_->current_ui() == MainWindow::LIST_PEERS)
    main_wnd_->SwitchToPeerList(client_->peers());
}

void Conductor::OnPeerDisconnected(int id) {
  RTC_LOG(INFO) << __FUNCTION__;
  std::cout<< "Our peer disconnected.\n";
  if (id == peer_id_) {
    RTC_LOG(INFO) << "Our peer disconnected";
    main_wnd_->QueueUIThreadCallback(PEER_CONNECTION_CLOSED, NULL);
  } else {
    // Refresh the list if we're showing it.
    if (main_wnd_->current_ui() == MainWindow::LIST_PEERS)
      main_wnd_->SwitchToPeerList(client_->peers());
  }
}

void Conductor::OnMessageFromPeer(int peer_id, const std::string& message) {
  RTC_DCHECK(peer_id_ == peer_id || peer_id_ == -1);
  RTC_DCHECK(!message.empty());

  if (!peer_connection_.get()) {
    RTC_DCHECK(peer_id_ == -1);
    peer_id_ = peer_id;

    if (!InitializePeerConnection()) {
      RTC_LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
      client_->SignOut();
      return;
    }
  } else if (peer_id != peer_id_) {
    RTC_DCHECK(peer_id_ != -1);
    RTC_LOG(WARNING)
        << "Received a message from unknown peer while already in a "
           "conversation with a different peer.";
    return;
  }

  Json::Reader reader;
  Json::Value jmessage;
  RTC_LOG(LS_INFO) << "ppt, message: " << message;
  if (!reader.parse(message, jmessage)) {
    RTC_LOG(WARNING) << "Received unknown message. " << message;
    return;
  }
  std::string type_str;
  std::string json_object;
  
  rtc::GetStringFromJsonObject(jmessage, kSessionDescriptionTypeName,
                               &type_str);
	if (!type_str.empty()) {
    if (type_str == "offer-loopback") {
      // This is a loopback call.
      // Recreate the peerconnection with DTLS disabled.
      if (!ReinitializePeerConnectionForLoopback()) {
        RTC_LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
        DeletePeerConnection();
        client_->SignOut();
      }
      return;
    }
    absl::optional<webrtc::SdpType> type_maybe =
        webrtc::SdpTypeFromString(type_str);
    if (!type_maybe) {
      RTC_LOG(LS_ERROR) << "Unknown SDP type: " << type_str;
      return;
    }
    webrtc::SdpType type = *type_maybe;
    std::string sdp;
    if (!rtc::GetStringFromJsonObject(jmessage, kSessionDescriptionSdpName,
                                      &sdp)) {
      RTC_LOG(WARNING) << "Can't parse received session description message.";
      return;
    }
    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
        webrtc::CreateSessionDescription(type, sdp, &error);
    if (!session_description) {
      RTC_LOG(WARNING) << "Can't parse received session description message. "
                       << "SdpParseError was: " << error.description;
      return;
    }
    RTC_LOG(INFO) << " Received session description :" << message;
    peer_connection_->SetRemoteDescription(
        DummySetSessionDescriptionObserver::Create(),
        session_description.release());
    if (type == webrtc::SdpType::kOffer) {
      peer_connection_->CreateAnswer(
          this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
    }
  } else {
    std::string sdp_mid;
    int sdp_mlineindex = 0;
    std::string sdp;
    if (!rtc::GetStringFromJsonObject(jmessage, kCandidateSdpMidName,
                                      &sdp_mid) ||
        !rtc::GetIntFromJsonObject(jmessage, kCandidateSdpMlineIndexName,
                                   &sdp_mlineindex) ||
        !rtc::GetStringFromJsonObject(jmessage, kCandidateSdpName, &sdp)) {
      RTC_LOG(WARNING) << "Can't parse received message.";
      return;
    }
    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::IceCandidateInterface> candidate(
        webrtc::CreateIceCandidate(sdp_mid, sdp_mlineindex, sdp, &error));
    if (!candidate.get()) {
      RTC_LOG(WARNING) << "Can't parse received candidate message. "
                       << "SdpParseError was: " << error.description;
      return;
    }
    if (!peer_connection_->AddIceCandidate(candidate.get())) {
      RTC_LOG(WARNING) << "Failed to apply the received candidate";
      return;
    }
    RTC_LOG(INFO) << " Received candidate :" << message;
  }
}

void Conductor::addIceCandidate(std::string& sdp_mid, int sdp_mlineindex, std::string& sdp){
	webrtc::SdpParseError error;
    std::unique_ptr<webrtc::IceCandidateInterface> candidate(
        webrtc::CreateIceCandidate(sdp_mid, sdp_mlineindex, sdp, &error));
    if (!candidate.get()) {
      RTC_LOG(WARNING) << "Can't parse received candidate message. "
                       << "SdpParseError was: " << error.description;
      return;
    }
	RTC_LOG(INFO) << " candidate ok";
	if(!peer_connection_.get()){
		RTC_LOG(INFO) << "peer_connection_ failed";
	}
    if (!peer_connection_->AddIceCandidate(candidate.get())) {
      RTC_LOG(WARNING) << "Failed to apply the received candidate";
      return;
    }
    RTC_LOG(INFO) << " Received candidate :" << sdp;
}


void Conductor::OnMessageSent(int err) {
  // Process the next pending message if any.
  main_wnd_->QueueUIThreadCallback(SEND_MESSAGE_TO_PEER, NULL);
}

void Conductor::OnServerConnectionFailure() {
  main_wnd_->MessageBox("Error", ("Failed to connect to " + server_).c_str(),
                        true);
}

//
// MainWndCallback implementation.
//

void Conductor::StartLogin(const std::string& server, int port) {
  if (client_->is_connected())
    return;
  server_ = server;
  client_->Connect(server, port, GetPeerName());
}

void* Conductor::janus_fun(void *callback){
	std::cout<<"janus_fun.\n";
	//((PeerConnectionClient*)callback)->Start();
	std::cout<<"janus_fun over.\n";
	while(1){
		std::cout<<"while Conductor::janus_fun.\n";
		sleep(1000);
	}
}

void Conductor::start(int mode){
	//std::thread thread1(janus_fun, std::ref(client_));
	//pthread_create(&hHandle, NULL, janus_fun, (void *)client_);	   //create a thread;
	this->mode = mode;
	client_->Start(mode);
}

void Conductor::replace_all_distinct(std::string&           str, const std::string& old_value,const std::string& new_value)     
{     
    for(std::string::size_type   pos(0);   pos!=std::string::npos;   pos+=new_value.length())   {     
        if(   (pos=str.find(old_value,pos))!=std::string::npos   )     
            str.replace(pos,old_value.length(),new_value);     
        else   break;     
    }     
    //return str;     
}     

void Conductor::setRemoteDescription(int c_type, const std::string& c_sdp){//SetRemoteDescription
	//RTC_DCHECK(peer_id_ == peer_id || peer_id_ == -1);
	  RTC_DCHECK(!c_sdp.empty());
	
	  if (!peer_connection_.get()) {
		RTC_DCHECK(peer_id_ == -1);
		//peer_id_ = 111111;
	
		if (!InitializePeerConnection()) {
		  RTC_LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
		  client_->SignOut();
		  return;
		}
	  } /*else if (peer_id != peer_id_) {
		RTC_DCHECK(peer_id_ != -1);
		RTC_LOG(WARNING)
			<< "Received a message from unknown peer while already in a "
			   "conversation with a different peer.";
		return;
	  }
	*/
	/*
	  Json::Reader reader;
	  Json::Value jmessage;
	  RTC_LOG(LS_INFO) << "ppt, message: " << message;
	  if (!reader.parse(message, jmessage)) {
		RTC_LOG(WARNING) << "Received unknown message. " << message;
		return;
	  }
	  std::string type_str;
	  std::string json_object;
	  
	  rtc::GetStringFromJsonObject(jmessage, kSessionDescriptionTypeName,
								   &type_str);
		if (!type_str.empty()) {
		if (type_str == "offer-loopback") {
		  // This is a loopback call.
		  // Recreate the peerconnection with DTLS disabled.
		  if (!ReinitializePeerConnectionForLoopback()) {
			RTC_LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
			DeletePeerConnection();
			client_->SignOut();
		  }
		  return;
		}
		*/
		/*
		absl::optional<webrtc::SdpType> type_maybe =
			webrtc::SdpTypeFromString(type_str);
		if (!type_maybe) {
		  RTC_LOG(LS_ERROR) << "Unknown SDP type: " << type_str;
		  return;
		}
		*/
		webrtc::SdpType type;// = *type_maybe;
		if(c_type == 0){
			RTC_LOG(LS_ERROR) << "setRemoteDescription offer";
			type = webrtc::SdpType::kOffer;
		}
		else if(c_type == 1){
			RTC_LOG(LS_ERROR) << "setRemoteDescription answer";
			type = webrtc::SdpType::kAnswer;
		}
		//webrtc::SdpType type = *type_maybe;
		/*
		std::string sdp;
		if (!rtc::GetStringFromJsonObject(jmessage, kSessionDescriptionSdpName,
										  &sdp)) {
		  RTC_LOG(WARNING) << "Can't parse received session description message.";
		  return;
		}
		*/
		webrtc::SdpParseError error;
		
		//c_sdp.replace("c=IN IP4 139.196.204.25\r\n", "c=IN IP4 139.196.204.25\r\nb=AS:100\r\n");
		std::string s = sdp_rate_set(-1, c_sdp);
		
		RTC_LOG(INFO) << " Received session description :" << s;
		std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
			webrtc::CreateSessionDescription(type, s, &error);
		if (!session_description) {
		  RTC_LOG(WARNING) << "Can't parse received session description message. "
						   << "SdpParseError was: " << error.description;
		  return;
		}
		
		peer_connection_->SetRemoteDescription(
			DummySetSessionDescriptionObserver::Create(),
			session_description.release());
		if (type == webrtc::SdpType::kOffer) {
		  RTC_LOG(LS_ERROR) << "peer_connection_->CreateAnswer";
		  peer_connection_->CreateAnswer(
			  this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
		}
}

std::string Conductor::sdp_rate_set(int rate, const std::string &sdp)
{
	if(rate == -1){
		return sdp;
	}
	std::string s; 
	std::stringstream ss;
	//ss<<"c=IN IP4 139.196.204.25\r\nb=AS:"<<rate<<"\r\n";
	ss<<"c=IN IP4 192.168.8.109\r\nb=AS:"<<rate<<"\r\n";
	std::string strtEST = ss.str();
	//s = str( boost::format("c=IN IP4 139.196.204.25\r\nb=AS:%d\r\n") % rate ); 
	//sprintf(sc, "c=IN IP4 139.196.204.25\r\nb=AS:%s\r\n", rate)
	//replace_all_distinct(s, std::string("c=IN IP4 139.196.204.25\r\n"), strtEST);
	replace_all_distinct(s, std::string("c=IN IP4 192.168.8.109\r\n"), strtEST);
	return s;
}
void Conductor::createOffer(){
	RTC_LOG(INFO) << "ppt, go to createOffer.";
	peer_connection_->CreateOffer(
		this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
}

void Conductor::DisconnectFromServer() {
  if (client_->is_connected())
    client_->SignOut();
}

void Conductor::ConnectToPeer(int peer_id) {
  RTC_DCHECK(peer_id_ == -1);
  RTC_DCHECK(peer_id != -1);

  if (peer_connection_.get()) {
    main_wnd_->MessageBox(
        "Error", "We only support connecting to one peer at a time", true);
    return;
  }

  if (InitializePeerConnection()) {
    peer_id_ = peer_id;
    peer_connection_->CreateOffer(
        this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
  } else {
    main_wnd_->MessageBox("Error", "Failed to initialize PeerConnection", true);
  }
}

void Conductor::OnReady_noId() {
  if (peer_connection_.get()) {
    main_wnd_->MessageBox(
        "Error", "We only support connecting to one peer at a time", true);
    return;
  }
  RTC_LOG(LS_ERROR) << "ppt, Conductor::OnReady_noId.";
  //while(1){}
  if (InitializePeerConnection()) {
    peer_id_ = 11111;
	std::string cmd = std::string("13Q3wnLuN7");
    std::cout << "client_->dispatch call" << std::endl;
	client_->dispatch(cmd);
	
    //peer_connection_->CreateOffer(
    //    this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
  } else {
    main_wnd_->MessageBox("Error", "Failed to initialize PeerConnection", true);
  }
}

void Conductor::OnReady_Id() {
  if (peer_connection_.get()) {
    main_wnd_->MessageBox(
        "Error", "We only support connecting to one peer at a time", true);
    return;
  }
  RTC_LOG(LS_ERROR) << "ppt, Conductor::OnReady_Id.";
  if (InitializePeerConnection()) {
  	RTC_LOG(LS_ERROR) << "InitializePeerConnection ok.";
	//while(1){sleep(1);}
    //peer_id_ = id;
    std::string cmd;
    if(offer_type){
		cmd = std::string("13Q3wnLuN7");//JanusCommands::CALL
  	}
	else
	{
		cmd = std::string("CALL_ANSWER");//JanusCommands::CALL
	}
    std::cout << "client_->dispatch call" << std::endl;
	client_->_bundle->setInt("peer_id", peer_id_);
	
	client_->dispatch(cmd);
	
  } else {
  	RTC_LOG(LS_ERROR) << "InitializePeerConnection failed.";
    main_wnd_->MessageBox("Error", "Failed to initialize PeerConnection", true);
  }
}


void Conductor::OnReady() {
  main_wnd_->QueueUIThreadCallback(ON_READY_NOID, NULL);
}

void Conductor::OnReady_withId(int64_t id, int offer) {
  peer_id_ = id;
  offer_type = offer;
  main_wnd_->QueueUIThreadCallback(ON_READY_WITHID, NULL);
}


void Conductor::AddTracks() {
  if (!peer_connection_->GetSenders().empty()) {
    return;  // Already added tracks.
  }
  RTC_LOG(LS_ERROR) << "ppt, AddTracks.";

  rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
      peer_connection_factory_->CreateAudioTrack(
          kAudioLabel, peer_connection_factory_->CreateAudioSource(
                           cricket::AudioOptions())));
  auto result_or_error = peer_connection_->AddTrack(audio_track, {kStreamId});
  if (!result_or_error.ok()) {
    RTC_LOG(LS_ERROR) << "Failed to add audio track to PeerConnection: "
                      << result_or_error.error().message();
  }

  
  
  if(mode == 2) {
  	
  	rtc::scoped_refptr<FileCapturerTrackSource> file_video_device =
      FileCapturerTrackSource::Create();
	if(file_video_device){
	    rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_(
	        peer_connection_factory_->CreateVideoTrack(kVideoLabel, file_video_device));
	    main_wnd_->StartLocalRenderer(video_track_);

	    result_or_error = peer_connection_->AddTrack(video_track_, {kStreamId});
	    if (!result_or_error.ok()) {
	      RTC_LOG(LS_ERROR) << "Failed to add video track to PeerConnection: "
	                        << result_or_error.error().message();
	    }
	}
	else {
    	RTC_LOG(LS_ERROR) << "file OpenVideoCaptureDevice failed";
  	}
  	
  }
  else{
  	rtc::scoped_refptr<CapturerTrackSource> video_device =
      CapturerTrackSource::Create();
	if(video_device){
	    rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_(
	        peer_connection_factory_->CreateVideoTrack(kVideoLabel, video_device));
	    main_wnd_->StartLocalRenderer(video_track_);

	    result_or_error = peer_connection_->AddTrack(video_track_, {kStreamId});
	    if (!result_or_error.ok()) {
	      RTC_LOG(LS_ERROR) << "Failed to add video track to PeerConnection: "
	                        << result_or_error.error().message();
    	}
	}
	else {
    	RTC_LOG(LS_ERROR) << "OpenVideoCaptureDevice failed";
  	}
  } 
  std::cout << "go to SwitchToStreamingUI" << std::endl;
  main_wnd_->SwitchToStreamingUI1();
}

void Conductor::DisconnectFromCurrentPeer() {
  RTC_LOG(INFO) << __FUNCTION__;
  if (peer_connection_.get()) {
    client_->SendHangUp(peer_id_);
    DeletePeerConnection();
  }

  if (main_wnd_->IsWindow())
    main_wnd_->SwitchToPeerList(client_->peers());
}

void Conductor::UIThreadCallback(int msg_id, void* data) {
  switch (msg_id) {
    case PEER_CONNECTION_CLOSED:
      RTC_LOG(INFO) << "PEER_CONNECTION_CLOSED";
      DeletePeerConnection();

      if (main_wnd_->IsWindow()) {
        if (client_->is_connected()) {
          main_wnd_->SwitchToPeerList(client_->peers());
        } else {
          main_wnd_->SwitchToConnectUI();
        }
      } else {
        DisconnectFromServer();
      }
      break;

    case SEND_MESSAGE_TO_PEER: {
      RTC_LOG(INFO) << "SEND_MESSAGE_TO_PEER";
      std::string* msg = reinterpret_cast<std::string*>(data);
      if (msg) {
        // For convenience, we always run the message through the queue.
        // This way we can be sure that messages are sent to the server
        // in the same order they were signaled without much hassle.
        pending_messages_.push_back(msg);
      }

      if (!pending_messages_.empty() && !client_->IsSendingMessage()) {
        msg = pending_messages_.front();
        pending_messages_.pop_front();

        if (!client_->SendToPeer(peer_id_, *msg) && peer_id_ != -1) {
          RTC_LOG(LS_ERROR) << "SendToPeer failed";
          DisconnectFromServer();
        }
        delete msg;
      }

      if (!peer_connection_.get())
        peer_id_ = -1;

      break;
    }

    case NEW_TRACK_ADDED: {
		std::cout << "NEW_TRACK_ADDED" << std::endl;
      auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>(data);
      if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
        auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
        main_wnd_->StartRemoteRenderer(video_track);
      }
      track->Release();
      break;
    }

    case TRACK_REMOVED: {
      // Remote peer stopped sending a track.
      auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>(data);
      track->Release();
      break;
    }
	case ON_READY_NOID:{
		OnReady_noId();
		//main_wnd_->SwitchToStreamingUI1();
		break;
	}
	case ON_READY_WITHID:{
		//int64_t* id = reinterpret_cast<int64_t *>(data);
		OnReady_Id();
		//main_wnd_->SwitchToStreamingUI1();
		break;
	}
    default:
      RTC_NOTREACHED();
      break;
  }
}

void Conductor::OnSuccess(webrtc::SessionDescriptionInterface* desc) {//InternalCreateOffer或者InternalCreateAnswer success
  RTC_LOG(LS_ERROR) << "Conductor::OnSuccess";
  peer_connection_->SetLocalDescription(
      DummySetSessionDescriptionObserver::Create(), desc);

  std::string sdp;
  desc->ToString(&sdp);
	RTC_LOG(LS_ERROR) << "sdp: " << sdp;
  // For loopback test. To save some connecting delay.
  if (loopback_) {
    // Replace message type from "offer" to "answer"
    std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
        webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, sdp);
    peer_connection_->SetRemoteDescription(
        DummySetSessionDescriptionObserver::Create(),
        session_description.release());
	
    return;
  }

  Json::StyledWriter writer;
  Json::Value jmessage;
  jmessage[kSessionDescriptionTypeName] =
      webrtc::SdpTypeToString(desc->GetType());
  jmessage[kSessionDescriptionSdpName] = sdp;
  //SendMessage(writer.write(jmessage));
  if(!strcmp(webrtc::SdpTypeToString(desc->GetType()), "offer")){
  	RTC_LOG(LS_ERROR) << "is offer";
  	client_->onOffer(sdp);
  }else{
  	RTC_LOG(LS_ERROR) << "is answer";
  	client_->onAnswer(sdp);
  }
}

void Conductor::OnFailure(webrtc::RTCError error) {
  RTC_LOG(LERROR) << ToString(error.type()) << ": " << error.message();
}

void Conductor::SendMessage(const std::string& json_object) {
  std::string* msg = new std::string(json_object);
  main_wnd_->QueueUIThreadCallback(SEND_MESSAGE_TO_PEER, msg);
}
