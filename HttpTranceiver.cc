#include "HttpTranceiver.h"
  using namespace Janus;
/*
  HttpResponse::HttpResponse(int status, const std::string& body) {
    this->_status = status;
    this->_body = body;
  }

  int HttpResponse::status() {
    return this->_status;
  }

  std::string HttpResponse::body() {
    return this->_body;
  }
  */
 //namespace{

  HttpTranceiver::HttpTranceiver(/*const std::string& baseUrl*/) {
    curl_global_init(CURL_GLOBAL_ALL);
    //this->_baseUrl = baseUrl;
  }

  HttpTranceiver::~HttpTranceiver() {
    curl_global_cleanup();
  }

  std::shared_ptr<HttpResponse> HttpTranceiver::get(const std::string& path) {
    return this->_request(path, "GET");
  }

  std::shared_ptr<HttpResponse> HttpTranceiver::post(const std::string& path, const std::string& body) {
    return this->_request(path, "POST", body);
  }

  std::shared_ptr<HttpResponse> HttpTranceiver::_request(const std::string& path, const std::string& method, const std::string& body) {
    auto handle = curl_easy_init();

    curl_easy_setopt(handle, CURLOPT_USERAGENT, "Janus Native HTTP Client");

    auto fullUrl = path;
    curl_easy_setopt(handle, CURLOPT_URL, fullUrl.c_str());

    curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, method.c_str());

    struct curl_slist* headers = curl_slist_append(nullptr, "Content-Type: application/json");
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, std::strlen(body.c_str()));

    std::string bodyString = "";
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, HttpTranceiver::_writeFunction);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &bodyString);

    long status = curl_easy_perform(handle);
    if (status == CURLE_OK) {
      curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &status);
    }
	else{
		return std::make_shared<HttpResponse>(status, "");
	}
    curl_slist_free_all(headers);
    curl_easy_cleanup(handle);

    return std::make_shared<HttpResponse>(status, bodyString);
  }

  size_t HttpTranceiver::_writeFunction(void* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append(reinterpret_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
  }
//}

