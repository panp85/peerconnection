/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
#define EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "api/media_stream_interface.h"
#include "api/peer_connection_interface.h"
#include "examples/peerconnection/client/main_wnd.h"
#include "examples/peerconnection/client/peer_connection_client.h"
#include "examples/peerconnection/client/common.h"
#include "SessionSRS.h"

namespace webrtc {
class VideoCaptureModule;
}  // namespace webrtc

namespace cricket {
class VideoRenderer;
}  // namespace cricket

class Conductor : public webrtc::PeerConnectionObserver,
                  public webrtc::CreateSessionDescriptionObserver,
                  public PeerConnectionClientObserver,
                  public MainWndCallback {
 public:
  enum CallbackID {
    MEDIA_CHANNELS_INITIALIZED = 1,
    PEER_CONNECTION_CLOSED,
    SEND_MESSAGE_TO_PEER,
    NEW_TRACK_ADDED,
    TRACK_REMOVED,
    ON_READY_NOID,
    ON_READY_WITHID,
    ON_SET_REMOTE_DESCRIPTION,
    ON_ADD_ICE_CANDIDATE,
  };
  enum OFFER_TYPE{
  	OFFER_ANSWER,
	OFFER_OFFER,
  };
  
  Conductor(PeerConnectionClient* client, MainWindow* main_wnd);

  bool connection_active() const;

  void Close() override;
  void connect2janusServer(bool isp2p) override;

 protected:
  ~Conductor();
  bool InitializePeerConnection();
  bool ReinitializePeerConnectionForLoopback();
  bool CreatePeerConnection(bool dtls);
  void DeletePeerConnection();
  void EnsureStreamingUI();
  void AddTracks();

  //
  // PeerConnectionObserver implementation.
  //

  void OnSignalingChange(
      webrtc::PeerConnectionInterface::SignalingState new_state) override {}
  void OnAddTrack(
      rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
      const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
          streams) override;
  void OnRemoveTrack(
      rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
  void OnDataChannel(
      rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {}
  void OnRenegotiationNeeded() override {}
  void OnIceConnectionChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state) override {}
  void OnIceGatheringChange(
      webrtc::PeerConnectionInterface::IceGatheringState new_state) override;
  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
  void OnIceConnectionReceivingChange(bool receiving) override {}

  //
  // PeerConnectionClientObserver implementation.
  //

  void OnSignedIn() override;

  void OnDisconnected() override;

  void OnPeerConnected(int id, const std::string& name) override;

  void OnPeerDisconnected(int id) override;

  void OnMessageFromPeer(int peer_id, const std::string& message) override;

  void OnMessageSent(int err) override;

  void OnServerConnectionFailure() override;

  //add by pp
  void OnReady() override;//go to create offer
  void OnReady_withId(int64_t peer_id, int offer) override;//go to create offer
  void setRemoteDescription(int c_type, const std::string& c_sdp) override;
  void createOffer() override;
  //
  // MainWndCallback implementation.
  //

  void StartLogin(const std::string& server, int port) override;

  void DisconnectFromServer() override;

  void ConnectToPeer(int peer_id) override;

  void DisconnectFromCurrentPeer() override;

  void UIThreadCallback(int msg_id, void* data) override;

  // CreateSessionDescriptionObserver implementation.
  void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
  void OnFailure(webrtc::RTCError error) override;

 protected:
  // Send a message to the remote peer.
  void SendMessage(const std::string& json_object);

  int64_t peer_id_;
  enum OFFER_TYPE offer_type;
  bool loopback_;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
      peer_connection_factory_;
  PeerConnectionClient* client_;
  MainWindow* main_wnd_;
  std::deque<std::string*> pending_messages_;
  std::string server_;
  
  bool isp2p;
  Media_Source_Type source_type;
  Server_Type server_t;
  Publish_Play_Role srs_role;
  webrtc::SdpType type;
  SessionSRS *pSession;
  
  bool remote_ready;
  void AddIceCandidate(const webrtc::IceCandidateInterface* ice_candidate);
  void SetRemoteDescription(const std::string* c_sdp);
  //pthread_t hHandle;
  static void* janus_fun(void *callback);
  void OnReady_noId();
  void OnReady_Id();
  void addIceCandidate(std::string& sdp_mid, int sdp_mlineindex, std::string& sdp) override;
  void setbitrate();
  void replace_all_distinct(std::string& str, const std::string& old_value,const std::string& new_value);
  std::string sdp_rate_set(int rate, const std::string &sdp);

  void setSourceType(enum Media_Source_Type type) override {source_type = type;}
  void setServerType(Server_Type st) override {server_t = st;}
  void setSrsRole(Publish_Play_Role role) override {srs_role = role;}
};

#endif  // EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
