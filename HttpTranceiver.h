#ifndef __HTTPTRANCEIVER__
#define __HTTPTRANCEIVER__
#include "janus/http.h"
#include <curl/curl.h>
#include "main_wnd.h"



//namespace {

//using namespace Janus;

class HttpTranceiver: public Janus::Http{
	

	public:
	HttpTranceiver(/*const std::string& baseUrl*/);
	virtual ~HttpTranceiver();
	
	//std::shared_ptr<HttpResponse> get(const std::string& path);

	std::shared_ptr<Janus::HttpResponse> _request(const std::string& path, const std::string& method, const std::string& body = "");
	std::shared_ptr<Janus::HttpResponse> get(const std::string& path);
	std::shared_ptr<Janus::HttpResponse> post(const std::string& path, const std::string& body);
	static size_t _writeFunction(void* ptr, size_t size, size_t nmemb, std::string* data);
};
//}
#endif 
