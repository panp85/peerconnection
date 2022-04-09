#include <iostream>
#include "SessionSRS.h"
#include "rtc_base/strings/json.h"
#include "api/jsep.h"
#include "rtc_base/logging.h"

using namespace std;
using namespace Janus;

//namespace { 
SessionSRS::SessionSRS(string server)/*:server_ip(server),port(server_port)*/{
	server_ip = server;
	RTC_LOG(LS_ERROR) << "server_ip: " << server_ip;
	//this->port = port;
	for(int i = 0; i < HTTP_CLIENT_POOL_SIZE; i++){
		//pHttpTranceiver = new HttpTranceiver();
		mHttpTranceivers.push(std::make_shared<HttpTranceiver>());
	}

	asyncImpl = std::make_shared<AsyncImpl>();
	
}

void SessionSRS::RegisterObserver(PeerConnectionClientObserver* callback){
	_callback = callback;
}

void SessionSRS::sendSDP(const shared_ptr<string> &sdp, const string role){
  Json::StyledWriter writer;
  Json::Value jmessage;
  RTC_LOG(LS_ERROR) << "in SessionSRS::sendSDP.";
  RTC_LOG(LS_ERROR) << "server: " << server_ip;
  //while(1){}
  string url = string("https://") + server_ip + string(":443") + role;
  //return;
  jmessage["api"] = url.c_str();
  RTC_LOG(LS_ERROR) << url;
	//while(1){}
  time_t now = time(0);
  //char* dt = ctime(&now);
  tm *ltm = localtime(&now);
  //logfileName_ = log_path_+"/webrtc_"+timeString.toStdString()+".log";
  string tid = to_string(1900 + ltm->tm_year)+"_"+to_string(1 + ltm->tm_mon)+"_"+to_string(ltm->tm_mday)+"_"+
  	to_string(ltm->tm_hour)+"_"+to_string(ltm->tm_min)+"_"+to_string(ltm->tm_sec);
  tid = tid.substr(tid.length() - 8, 7);
  
  jmessage["tid"] = tid.c_str();
  jmessage["streamurl"] = "webrtc://139.196.204.25/live/livestream";
  jmessage["clientip"] = "";
  jmessage["sdp"] = sdp->c_str();

  string body = writer.write(jmessage);
  RTC_LOG(LS_ERROR) << body;
  this->_sendAsync(body, role);
  //pHttpTranceiver->post(server, const std :: string & body);
}

void SessionSRS::onMessage(const std::shared_ptr<HttpResponse> httpResponse){
	Json::Reader reader;
	Json::Value jmessage;
	RTC_LOG(LS_INFO) << "ppt, SessionSRS::onMessage, body: " << httpResponse.get()->body().c_str();
	RTC_LOG(LS_INFO) << "ppt, SessionSRS::onMessage, status: " << httpResponse.get()->status();
	if (!reader.parse(httpResponse.get()->body().c_str(), jmessage)) {
		RTC_LOG(WARNING) << "Received unknown message. " << jmessage;
		return;
	}
	
	std::string sdp;
	std::string json_object;

	/*
	string type_str;
    rtc::GetStringFromJsonObject(jmessage, "sdp", &sdp)
    absl::optional<webrtc::SdpType> type_maybe =
        webrtc::SdpTypeFromString(type_str);
    if (!type_maybe) {
      RTC_LOG(LS_ERROR) << "Unknown SDP type: " << type_str;
      return;
    }
    webrtc::SdpType type = *type_maybe;
    */
    if (!rtc::GetStringFromJsonObject(jmessage, "sdp",
                                      &sdp)) {
      RTC_LOG(WARNING) << "Can't parse received session description message.";
      return;
    }
    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
        webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, sdp, &error);
    if (!session_description) {
      RTC_LOG(WARNING) << "From SRS, Can't parse received session description message. "
                       << "SdpParseError was: " << error.description;
      return;
    }
	_callback->setRemoteDescription(1, sdp);
}

void SessionSRS::_sendAsync(const std::string body, const std::string path){
    auto task = [=] {
      std::unique_lock<std::mutex> notEmptyLock(this->_tranceiversMutex);
      this->_notEmpty.wait(notEmptyLock, [this] {
        return this->mHttpTranceivers.size() != 0;
      });
	  RTC_LOG(LS_INFO) << "_sendAsync task.\n";
      auto tranceiver = this->mHttpTranceivers.front();
      this->mHttpTranceivers.pop();

      notEmptyLock.unlock();
      
      this->_notEmpty.notify_one();

      //std::string path = "/rtc/v1/publish/";

      auto reply = tranceiver->post("http://"+server_ip+":1985" + path, body);
	  if(!reply){
	  	std::cout << "HttpTransport::_sendAsync failed.\n" << std::endl;
	  	return;
	  }
      //auto content = nlohmann::json::parse(reply->body());
	  
      //this->_delegate->onMessage(content, context);
	  onMessage(reply);
	  
      notEmptyLock.lock();
      this->mHttpTranceivers.push(tranceiver);
      notEmptyLock.unlock();
      this->_notEmpty.notify_one();
    };

    asyncImpl->submit(task);
}
//}
