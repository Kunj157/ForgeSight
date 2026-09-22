#include "api/grpc_server.h"

#include "alarm-engine/types.h"
#include "api/device_service.h"
#include "api/rule_service.h"
#include "forgesight.grpc.pb.h"
#include "forgesight.pb.h"

#include <grpcpp/grpcpp.h>

#include <utility>

namespace api {
namespace {

bool authorized(grpc::ServerContext* ctx, const std::string& api_key) {
    if (api_key.empty())
        return true;
    const auto& md = ctx->client_metadata();
    auto it = md.find("x-api-key");
    if (it == md.end())
        return false;
    return std::string(it->second.begin(), it->second.end()) == api_key;
}

void fill_reading(::forgesight::Reading* out, const ingestion::Reading& r) {
    out->set_device_id(r.device_id);
    out->set_sensor(r.sensor);
    out->set_value(r.value);
    out->set_unit(r.unit);
    out->set_timestamp(r.timestamp);
    out->set_anomaly(r.anomaly);
}

class ServiceImpl final : public ::forgesight::ForgeSight::Service {
  public:
    ServiceImpl(DeviceService& devices, RuleService& rules, std::string api_key)
        : devices_(devices), rules_(rules), api_key_(std::move(api_key)) {}

    grpc::Status ListDevices(grpc::ServerContext* ctx, const ::forgesight::Empty*,
                             ::forgesight::DeviceList* out) override {
        if (!authorized(ctx, api_key_))
            return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "missing or invalid x-api-key");
        for (const auto& d : devices_.list_devices()) {
            auto* m = out->add_devices();
            m->set_id(d.id);
            m->set_name(d.name);
            m->set_sensor(d.sensor);
            m->set_last_reading_time(d.last_reading_time);
            m->set_last_value(d.last_value);
            m->set_last_unit(d.last_unit);
            m->set_anomaly(d.anomaly);
            m->set_plant(d.plant);
            m->set_floor(d.floor);
        }
        return grpc::Status::OK;
    }

    grpc::Status GetHistory(grpc::ServerContext* ctx, const ::forgesight::HistoryRequest* req,
                            ::forgesight::ReadingList* out) override {
        if (!authorized(ctx, api_key_))
            return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "missing or invalid x-api-key");
        const int max_points = req->max_points() == 0 ? 1000 : req->max_points();
        const std::string since =
            req->since().empty() ? std::string("1970-01-01T00:00:00Z") : req->since();
        for (const auto& r :
             devices_.get_history(req->device_id(), req->sensor(), since, max_points))
            fill_reading(out->add_readings(), r);
        return grpc::Status::OK;
    }

    grpc::Status ListAlarms(grpc::ServerContext* ctx, const ::forgesight::AlarmQuery* req,
                            ::forgesight::AlarmList* out) override {
        if (!authorized(ctx, api_key_))
            return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "missing or invalid x-api-key");
        for (const auto& a : rules_.list_alarms(req->unacknowledged_only())) {
            auto* m = out->add_alarms();
            m->set_id(a.id);
            m->set_rule_id(a.rule_id);
            m->set_device_id(a.device_id);
            m->set_sensor(a.sensor);
            m->set_value(a.value);
            m->set_severity(alarm_engine::severity_to_string(a.severity));
            m->set_message(a.message);
            m->set_timestamp(a.timestamp);
            m->set_acknowledged(a.acknowledged);
        }
        return grpc::Status::OK;
    }

    grpc::Status AcknowledgeAlarm(grpc::ServerContext* ctx, const ::forgesight::AckRequest* req,
                                  ::forgesight::AckReply* out) override {
        if (!authorized(ctx, api_key_))
            return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "missing or invalid x-api-key");
        out->set_ok(rules_.acknowledge_alarm(req->id()));
        return grpc::Status::OK;
    }

  private:
    DeviceService& devices_;
    RuleService& rules_;
    std::string api_key_;
};

} // namespace

class GrpcServer::Impl {
  public:
    Impl(DeviceService& devices, RuleService& rules, std::string api_key)
        : service_(devices, rules, std::move(api_key)) {}

    bool start(const std::string& bind_address, int& port_out) {
        grpc::ServerBuilder builder;
        int selected = 0;
        builder.AddListeningPort(bind_address, grpc::InsecureServerCredentials(), &selected);
        builder.RegisterService(&service_);
        server_ = builder.BuildAndStart();
        if (!server_)
            return false;
        port_out = selected;
        return true;
    }

    void stop() {
        if (server_) {
            server_->Shutdown();
            server_->Wait();
            server_.reset();
        }
    }

  private:
    ServiceImpl service_;
    std::unique_ptr<grpc::Server> server_;
};

GrpcServer::GrpcServer(DeviceService& devices, RuleService& rules, std::string api_key)
    : impl_(std::make_unique<Impl>(devices, rules, std::move(api_key))) {}

GrpcServer::~GrpcServer() {
    stop();
}

bool GrpcServer::start(const std::string& bind_address) {
    if (!impl_)
        return false;
    return impl_->start(bind_address, port_);
}

void GrpcServer::stop() {
    if (impl_)
        impl_->stop();
}

} // namespace api
