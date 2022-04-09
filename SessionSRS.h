#ifndef __SessionSRS__
#define __SessionSRS__
#include <memory>
#include<string>

#include "janus/async.h"
#include "HttpTranceiver.h"
//#include "conductor.h"
//#include "main_wnd.h"
#include "peer_connection_client.h"

//namespace{

//using namespace std;
#define HTTP_CLIENT_POOL_SIZE 2

class SessionSRS{
	//using namespace Janus;

	public:
		SessionSRS(std::string      server);
		void sendSDP(const std::shared_ptr<std::string> &sdp, const std::string role);
		void RegisterObserver(PeerConnectionClientObserver* callback);
	private:
		std::string server_ip;
		//const int port;
		//HttpTranceiver *pHttpTranceiver;
		std::queue<std::shared_ptr<HttpTranceiver>> mHttpTranceivers;

		PeerConnectionClientObserver* _callback;

		std::shared_ptr<Janus::AsyncImpl> asyncImpl;

		std::mutex _tranceiversMutex;
      	std::condition_variable _notEmpty;

		void onMessage(std::shared_ptr<Janus::HttpResponse> httpResponse);
		void _sendAsync(const std::string body, const std::string path);
};
//}
#endif
