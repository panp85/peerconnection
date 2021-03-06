// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from janus-client.djinni

#pragma once

#include <string>

namespace Janus {

class JanusConf {
public:
    virtual ~JanusConf() {}

    virtual std::string url() = 0;

    virtual std::string plugin() = 0;

	bool isp2p;
};

class JanusProxyConf final : public JanusConf
{
	public:
		JanusProxyConf(){isp2p = false;}
		~JanusProxyConf() = default;

		std::string url() override;
		std::string plugin() override;

		
};



}  // namespace Janus
