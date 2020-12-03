/*
 *  Copyright 2011 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef EXAMPLES_PEERCONNECTION_CLIENT_PEER_CONNECTION_CLIENT_H_
#define EXAMPLES_PEERCONNECTION_CLIENT_PEER_CONNECTION_CLIENT_H_

#include <map>
#include <memory>
#include <string>

#include "rtc_base/net_helpers.h"
#include "rtc_base/physical_socket_server.h"
#include "rtc_base/signal_thread.h"
#include "rtc_base/third_party/sigslot/sigslot.h"

#include "janus/janus_conf.hpp"

#include "janus/peer_factory.hpp"

#include "janus/protocol_delegate.hpp"
#include "janus/janus_impl.h"
#include "janus/constraints.hpp"
#include "janus/peer.hpp"
#include "janus/bundle_impl.h"

typedef std::map<int, std::string> Peers;

struct PeerConnectionClientObserver {
  virtual void OnSignedIn() = 0;  // Called when we're logged on.
  virtual void OnDisconnected() = 0;
  virtual void OnPeerConnected(int id, const std::string& name) = 0;
  virtual void OnPeerDisconnected(int peer_id) = 0;
  virtual void OnMessageFromPeer(int peer_id, const std::string& message) = 0;
  virtual void OnMessageSent(int err) = 0;
  virtual void OnServerConnectionFailure() = 0;
  virtual void OnReady() = 0;
  virtual void OnReady_withId(int64_t peer_id, int offer) = 0;
  virtual void addIceCandidate(std::string& sdp_mid, int sdp_mlineindex, std::string& sdp) = 0;
  virtual void setRemoteDescription(int c_type, const std::string& c_sdp) = 0;
  virtual void createOffer() = 0;
 protected:
  virtual ~PeerConnectionClientObserver() {}
};


class JanusProxyProtocolDelegate: public Janus::ProtocolDelegate {
public:
    //JanusProxyProtocolDelegate(){};
    //~JanusProxyProtocolDelegate(){};

    void onReady() override;
	void onReady_withId(int64_t peer_id, int offer) override;
    void onClose() override;
    void onError(const ::Janus::JanusError & error, const std::shared_ptr<::Janus::Bundle> & context) override;
    void onEvent(const std::shared_ptr<::Janus::JanusEvent> & event, const std::shared_ptr<::Janus::Bundle> & context) override;
    void onHangup(const std::string & reason) override;

	
	PeerConnectionClientObserver* callback_;
	void setCallback(PeerConnectionClientObserver* callback);
};

class JanusPeerFactory : public Janus::PeerFactory
//	class JavaProxy final : ::djinni::JavaProxyHandle<JavaProxy>, public ::Janus::PeerFactory
{
public:
	JanusPeerFactory(){}
	~JanusPeerFactory(){}

	std::shared_ptr<::Janus::Peer> create(int64_t id, const std::shared_ptr<::Janus::Protocol> & owner) override;
	void onIceCompleted();
	std::shared_ptr<::Janus::Protocol> c_owner;
	void setCallback(PeerConnectionClientObserver* callback){
		callback_ = callback;
	}
	PeerConnectionClientObserver* callback_;
	int64_t c_id;
};

class PeerImpl : public Janus::Peer{
	
		void createOffer(const ::Janus::Constraints & c_constraints, const std::shared_ptr<::Janus::Bundle> & c_context) {
			static_cast<PeerConnectionClientObserver *>(callback_)->createOffer(/*c_constraints, c_context*/);
		}
		void createAnswer(const ::Janus::Constraints & c_constraints, const std::shared_ptr<::Janus::Bundle> & c_context) {
			
		}
		void setLocalDescription(::Janus::SdpType c_type, const std::string & c_sdp) {
			
		}
		void setRemoteDescription(::Janus::SdpType c_type, const std::string & c_sdp) {
			static_cast<PeerConnectionClientObserver *>(callback_)->setRemoteDescription((int)c_type, c_sdp);
		}
		void addIceCandidate(const std::string & c_mid, int32_t c_index, const std::string & c_sdp) {
		}
		void close() {
			
		}

		void setCallback(void* callback){
			callback_ = callback;
		}
	};

class PeerConnectionClient : public sigslot::has_slots<>,
                             public rtc::MessageHandler {
 public:
  enum State {
    NOT_CONNECTED,
    RESOLVING,
    SIGNING_IN,
    CONNECTED,
    SIGNING_OUT_WAITING,
    SIGNING_OUT,
  };

  PeerConnectionClient();
  ~PeerConnectionClient();

  int id() const;
  bool is_connected() const;
  const Peers& peers() const;

  void RegisterObserver(PeerConnectionClientObserver* callback);

  void Connect(const std::string& server,
               int port,
               const std::string& client_name);
  void Connect2janusServer(bool isp2p);//0:janus-camera; 1:camera-p2p; 2:file-p2p; 3:
  
  void onIceCompleted();
  void onIceCandidate(const std::string& mid, int32_t index, const std::string& sdp);

  bool SendToPeer(int peer_id, const std::string& message);
  bool SendHangUp(int peer_id);
  bool IsSendingMessage();

  bool SignOut();

  // implements the MessageHandler interface
  void OnMessage(rtc::Message* msg);

 protected:
  void DoConnect();
  void Close();
  void InitSocketSignals();
  bool ConnectControlSocket();
  void OnConnect(rtc::AsyncSocket* socket);
  void OnHangingGetConnect(rtc::AsyncSocket* socket);
  void OnMessageFromPeer(int peer_id, const std::string& message);

  // Quick and dirty support for parsing HTTP header values.
  bool GetHeaderValue(const std::string& data,
                      size_t eoh,
                      const char* header_pattern,
                      size_t* value);

  bool GetHeaderValue(const std::string& data,
                      size_t eoh,
                      const char* header_pattern,
                      std::string* value);

  // Returns true if the whole response has been read.
  bool ReadIntoBuffer(rtc::AsyncSocket* socket,
                      std::string* data,
                      size_t* content_length);

  void OnRead(rtc::AsyncSocket* socket);

  void OnHangingGetRead(rtc::AsyncSocket* socket);

  // Parses a single line entry in the form "<name>,<id>,<connected>"
  bool ParseEntry(const std::string& entry,
                  std::string* name,
                  int* id,
                  bool* connected);

  int GetResponseStatus(const std::string& response);

  bool ParseServerResponse(const std::string& response,
                           size_t content_length,
                           size_t* peer_id,
                           size_t* eoh);

  void OnClose(rtc::AsyncSocket* socket, int err);

  void OnResolveResult(rtc::AsyncResolverInterface* resolver);

  PeerConnectionClientObserver* callback_;
  rtc::SocketAddress server_address_;
  rtc::AsyncResolver* resolver_;
  std::unique_ptr<rtc::AsyncSocket> control_socket_;
  std::unique_ptr<rtc::AsyncSocket> hanging_get_;
  std::string onconnect_data_;
  std::string control_data_;
  std::string notification_data_;
  std::string client_name_;
  Peers peers_;
  State state_;
  int my_id_;
  
public:
	std::shared_ptr<Janus::JanusConf> _conf;
	std::shared_ptr<JanusPeerFactory> _factory;
	std::shared_ptr<Janus::PlatformImplImpl> _platformImpl;
	std::shared_ptr<JanusProxyProtocolDelegate> _delegate;
	std::shared_ptr<Janus::Janus> _janusImpl;

	std::shared_ptr<Janus::BundleImpl> _bundle;
	
	void setRemoteDescription(int c_type, const std::string & c_sdp){
		callback_->setRemoteDescription(c_type, c_sdp);
	}
	void dispatch(std::string& cmd){
		_janusImpl->dispatch(cmd, _bundle);
	}
	void createOffer(const ::Janus::Constraints & c_constraints, const std::shared_ptr<::Janus::Bundle> & c_context){
		callback_->createOffer();
	}
	void onOffer(std::string& sdp){
		_platformImpl->get_protocol()->onOffer(sdp, _bundle);
	}
	void onAnswer(std::string& sdp){
		_platformImpl->get_protocol()->onAnswer(sdp, _bundle);
	}
};



#endif  // EXAMPLES_PEERCONNECTION_CLIENT_PEER_CONNECTION_CLIENT_H_
