#include "janus/janus_api.h"

#include "janus/bundle_impl.h"
#include "janus/janus_error.hpp"
#include "janus/janus_commands.hpp"
#include <iostream>

namespace Janus {

  /* Janus API message Factories */
  
  namespace Messages {
	std::string p2p = "no";
	std::string root_id = "0";
	int64_t session_id = -1;
	
    nlohmann::json create(const std::string& transaction) {
      return {
        { "janus", JanusCommands::CREATE },
        { "transaction", transaction }
      };
    }

    nlohmann::json attach(const std::string& transaction, const std::string& plugin) {
      return {
      	{ "p2p", p2p.c_str()},
      	{ "session_id", session_id},
      	{ "room", root_id.c_str()},
        { "janus", JanusCommands::ATTACH },
        { "plugin", plugin },
        { "transaction", transaction }
      };
    }

    nlohmann::json destroy(const std::string& transaction) {
      return {
	  	{ "p2p", p2p.c_str()},
	  	{ "session_id", session_id},
        { "janus", JanusCommands::DESTROY },
        { "transaction", transaction }
      };
    }

    nlohmann::json trickle(const std::string& transaction, int64_t handleId, const std::string& sdpMid, int32_t sdpMLineIndex, 
    	const std::string& candidate, int64_t peer_id) {
      return {
	  	{ "p2p", p2p.c_str()},
	  	{ "session_id", session_id},
        { "janus", JanusCommands::TRICKLE },
        { "transaction", transaction },
        { "handle_id", handleId },
        { "peer_id", peer_id },
        { "candidate", { { "sdpMid", sdpMid }, { "sdpMLineIndex", sdpMLineIndex }, { "candidate", candidate } } }
      };
    }

    nlohmann::json trickleCompleted(const std::string& transaction, int64_t handleId) {
      return {
	  	{ "p2p", p2p.c_str()},
	  	{ "session_id", session_id},
        { "janus", JanusCommands::TRICKLE },
        { "transaction", transaction },
        { "handle_id", handleId },
        { "candidate", { { "completed", true } } }
      };
    }

    nlohmann::json message(const std::string& transaction, int64_t handleId, nlohmann::json body, int64_t peer_id) {
      body["p2p"] = p2p.c_str();
      body["janus"] = "message";
	  body["session_id"] = session_id;
      body["transaction"] = transaction;
      body["handle_id"] = handleId;
	  body["peer_id"] = peer_id;
      return body;
    }

    nlohmann::json hangup(const std::string& transaction, int64_t handleId) {
      return {
	  	{ "p2p", p2p.c_str()},
	  	{ "session_id", session_id},
        { "janus", JanusCommands::HANGUP },
        { "transaction", transaction },
        { "handle_id", handleId }
      };
    }

  }

  /* Janus API */

  JanusApi::JanusApi(const std::shared_ptr<Random>& random, const std::shared_ptr<TransportFactory>& transportFactory) {
    this->_transportFactory = transportFactory;
    this->_random = random;
	this->isp2p = false;
  }

  JanusApi::~JanusApi() {
    this->close();
  }

  void JanusApi::init(const std::shared_ptr<JanusConf>& conf, const std::shared_ptr<Platform>& platform, const std::shared_ptr<ProtocolDelegate>& delegate) {
    this->readyState(ReadyState::INIT);

    this->_transport = this->_transportFactory->create(conf->url(), this->shared_from_this());
	this->isp2p = conf->isp2p;
	if(conf->isp2p){
		Messages::p2p = "yes";
	}
    this->_delegate = delegate;
    this->_platform = std::static_pointer_cast<PlatformImpl>(platform);

    auto bundle = Bundle::create();
    bundle->setString("plugin", conf->plugin());
	std::cout << "JanusApi::init:JanusCommands::CREATE" << std::endl;
    this->dispatch(JanusCommands::CREATE, bundle);
  }

  void JanusApi::dispatch(const std::string& command, const std::shared_ptr<Bundle>& payload) {
    payload->setString("command", command);
    auto transaction = this->_random->generate();
    auto handleId = this->handleId(payload);
	std::cout << "JanusApi::dispatch: " << command << std::endl;

    if(command == JanusCommands::CREATE) {
      auto msg = Messages::create(transaction);
      this->_transport->send(msg, payload);

      return;
    }

    if(command == JanusCommands::ATTACH) {
      auto plugin = payload->getString("plugin", "");
	  //int64_t peerid = payload->getInt("peer_id", 0);
      this->_transport->send(Messages::attach(transaction, plugin), payload);

      return;
    }

    if(command == JanusCommands::DESTROY) {
      this->_transport->send(Messages::destroy(transaction), payload);

      return;
    }

    if(command == JanusCommands::HANGUP) {
      this->_transport->send(Messages::hangup(transaction, handleId), payload);

      return;
    }

    if(command == JanusCommands::TRICKLE) {
      auto sdpMid = payload->getString("sdpMid", "");
      auto sdpMLineIndex = payload->getInt("sdpMLineIndex", -1);
      auto candidate = payload->getString("candidate", "");
	  auto peer_id = payload->getInt("peer_id", -1);

      auto msg = Messages::trickle(transaction, handleId, sdpMid, sdpMLineIndex, candidate, peer_id);
      this->_transport->send(msg, payload);

      return;
    }

    if(command == JanusCommands::TRICKLE_COMPLETED) {
      auto msg = Messages::trickleCompleted(transaction, handleId);
      this->_transport->send(msg, payload);

      return;
    }

    if(this->_plugin != nullptr) {
      this->_plugin->command(command, payload);
    }
  }

  void JanusApi::hangup() {
    auto bundle = Bundle::create();
    this->dispatch(JanusCommands::HANGUP, bundle);
  }

  void JanusApi::close() {
    if(this->readyState() != ReadyState::READY) {
      return;
    }

    this->readyState(ReadyState::CLOSING);

    auto bundle = Bundle::create();
    this->dispatch(JanusCommands::DESTROY, bundle);
  }

  void JanusApi::onMessage(const nlohmann::json& message, const std::shared_ptr<Bundle>& context) {
    auto header = message.value("janus", "");
    auto str = message.dump();
	std::cout << "JanusApi::onMessage, header:" << header << std::endl;
	
    if(header == "error") {
      auto errorContent = message.value("error", nlohmann::json::object());
      auto code = errorContent.value("code", -1);
      auto reason = errorContent.value("reason", "");

      JanusError error(code, reason);
      this->_delegate->onError(error, context);

      return;
    }

    if(header == "success" && context->getString("command", "") == JanusCommands::CREATE) {
      auto id = message.value("data", nlohmann::json::object()).value("id", (int64_t) 0);
      auto idAsString = std::to_string(id);
	  Messages::session_id = id;
      this->_transport->sessionId(idAsString);
	  std::cout << "CREATE ok, session_id:" << id << ", go to ATTACH" << std::endl;
      this->dispatch(JanusCommands::ATTACH, context);

      return;
    }
	
	if(header == "_new_peer"){
		int64_t peer_id = message.value("data", nlohmann::json::object()).value("peer_id", (int64_t) 0);
		#define OFFER_ANSWER 0
		this->_delegate->onReady_withId(peer_id, OFFER_ANSWER);//本机是answer
		return;
	}

    if(header == "success" && context->getString("command", "") == JanusCommands::ATTACH 
		&& this->_plugin == nullptr) {
      this->_handleId = message.value("data", nlohmann::json::object()).value("id", (int64_t) 0);

      auto pluginId = context->getString("plugin", "");
      this->_plugin = this->_platform->plugin(pluginId, this->_handleId, this->shared_from_this());

      this->readyState(ReadyState::READY);
	  std::cout << "ppt, go to onReady janusApi" << std::endl;
	  if(!isp2p){
      	this->_delegate->onReady();
	  }
	  else{
	  	std::string peers = message.value("connections", "0");
		int index = peers.find(",");
	  	if(index >= 1){
	  		std::string peer = peers.substr(0, index);
			int64_t peer_id = std::stoll(peer);
			std::cout << "ppt, peer:" + peer + ", go to onReady janusApi, to int64: " << peer_id << std::endl;	
			#define OFFER_OFFER 1
			this->_delegate->onReady_withId(peer_id, OFFER_OFFER);//创建peerconnection等webrtc相关内容，本机是offer
	  	}
		else{
			std::cout << "no peers" << std::endl;
		}
	  }
      return;
    }
	

    if(header == "success" && context->getString("command", "") == JanusCommands::DESTROY) {
      this->_transport->close();
      this->readyState(ReadyState::CLOSED);
      this->_delegate->onClose();

      return;
    }

    if(header == "hangup") {
      auto reason = message.value("reason", "");

      this->_plugin->onHangup(reason);
      this->_delegate->onHangup(reason);

      return;
    }

    auto sender = message.value("sender", this->_handleId);
	
	if(header == "trickle") {
	  auto data = message.value("data", nlohmann::json::object());
	  
	  auto sdpMid = data.value("sdpMid", "");
      auto sdpMLineIndex = data.value("sdpMLineIndex", -1);
      auto candidate = data.value("candidate", "");
		std::cout
        << "onMessage, sdpMid: " << sdpMid
        << ", sdpMLineIndex: " << sdpMLineIndex 
        << ", candidate:" << candidate << std::endl;
      //std::shared_ptr<Bundle> bundle_trickle = std::shared_ptr<BundleImpl>();

	  //auto peer_id = data.value("peer_id", "");//每个不同的peer发过来的，都要发送到不同的pc，待处理

	  std::shared_ptr<JanusEventImpl> evt = std::make_shared<JanusEventImpl>(sender, data);
	  this->_delegate->onEvent(evt, context);
	  
	  return;
	}

    if(header == "event") {
      auto data = message.value("plugindata", nlohmann::json::object()).value("data", nlohmann::json::object());
      auto jsep = message.value("jsep", nlohmann::json::object());

      std::shared_ptr<JanusEventImpl> evt;
      if(jsep.empty()) {
        evt = std::make_shared<JanusEventImpl>(sender, data);
      } else {
      	std::cout << "has jsep" << std::endl;
        evt = std::make_shared<JanusEventImpl>(sender, data, jsep);
      }
      this->_plugin->onEvent(evt, context);

      return;
    }

    auto evt = std::make_shared<JanusEventImpl>(sender, message);

    if(header == "success" && context->getString("command", "") == JanusCommands::ATTACH && this->_plugin != nullptr) {
      this->_plugin->onEvent(evt, context);

      return;
    }

    //this->_delegate->onEvent(evt, context);
  }

  void JanusApi::onOffer(const std::string& sdp, const std::shared_ptr<Bundle>& context) {
  	std::cout << "JanusApi::onOffer" << std::endl;
    this->_plugin->onOffer(sdp, context);
  }

  void JanusApi::onAnswer(const std::string& sdp, const std::shared_ptr<Bundle>& context) {
  	std::cout << "JanusApi::onAnswer" << std::endl;
    this->_plugin->onAnswer(sdp, context);
  }

  void JanusApi::onIceCandidate(const std::string& mid, int32_t index, const std::string& sdp, int64_t id, int64_t peer_id) {
    auto bundle = Bundle::create();
    bundle->setString("sdpMid", mid);
    bundle->setInt("sdpMLineIndex", index);
    bundle->setString("candidate", sdp);
    bundle->setInt("handleId", id);
	bundle->setInt("peer_id", peer_id);
	

    this->dispatch(JanusCommands::TRICKLE, bundle);
  }

  void JanusApi::onIceCompleted(int64_t id) {
    auto bundle = Bundle::create();
    bundle->setInt("handleId", id);

    this->dispatch(JanusCommands::TRICKLE_COMPLETED, bundle);
  }

  ReadyState JanusApi::readyState() {
    std::lock_guard<std::mutex> lock(this->_readyStateMutex);

    return this->_readyState;
  }

  void JanusApi::readyState(ReadyState readyState) {
    std::lock_guard<std::mutex> lock(this->_readyStateMutex);
    this->_readyState = readyState;
  }

  int64_t JanusApi::handleId(const std::shared_ptr<Bundle>& context) {
    return context->getInt("handleId", this->_handleId);
  }

  void JanusApi::onCommandResult(const nlohmann::json& body, const std::shared_ptr<Bundle>& context) {
    auto transaction = this->_random->generate();
    auto handleId = this->handleId(context);
	int64_t peerid = context->getInt("peer_id", 0);
	std::cout << "JanusApi::onCommandResult, peerid: " << peerid << std::endl;

    auto message = Messages::message(transaction, handleId, body, peerid);
    this->_transport->send(message, context);
  }

  void JanusApi::onPluginEvent(const std::shared_ptr<JanusEvent>& event, const std::shared_ptr<Bundle>& context) {
    this->_delegate->onEvent(event, context);
  }

}
