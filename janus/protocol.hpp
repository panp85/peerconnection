// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from janus-client.djinni

#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace Janus {

class Bundle;
class JanusConf;
class Platform;
class ProtocolDelegate;

class Protocol {
public:
    virtual ~Protocol() {}

    virtual std::string name() = 0;

    virtual void init(const std::shared_ptr<JanusConf> & conf, const std::shared_ptr<Platform> & platform, const std::shared_ptr<ProtocolDelegate> & delegate) = 0;

    virtual void dispatch(const std::string & command, const std::shared_ptr<Bundle> & payload) = 0;

    virtual void hangup() = 0;

    virtual void close() = 0;

    virtual void onOffer(const std::string & sdp, const std::shared_ptr<Bundle> & context) = 0;

    virtual void onAnswer(const std::string & sdp, const std::shared_ptr<Bundle> & context) = 0;

    virtual void onIceCandidate(const std::string & mid, int32_t index, const std::string & sdp, int64_t id) = 0;

    virtual void onIceCompleted(int64_t id) = 0;
};

}  // namespace Janus
