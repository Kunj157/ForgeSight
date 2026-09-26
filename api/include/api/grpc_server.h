#pragma once

#include <memory>
#include <string>

namespace api {

class DeviceService;
class RuleService;

/// Optional gRPC surface for a second client type (CLI / non-Qt). Same
/// DeviceService / RuleService as REST. Disabled unless start() is called.
class GrpcServer {
  public:
    GrpcServer(DeviceService& devices, RuleService& rules, std::string api_key = {});
    ~GrpcServer();

    GrpcServer(const GrpcServer&) = delete;
    GrpcServer& operator=(const GrpcServer&) = delete;

    /// `bind_address` is host:port, e.g. "0.0.0.0:50051". Port 0 lets the
    /// OS pick; the chosen port is then available via port().
    bool start(const std::string& bind_address);
    void stop();
    int port() const { return port_; }

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    int port_ = 0;
};

} // namespace api
