#include "ingestion/kafka_producer.h"

#include "ingestion/reading_json.h"

#include <librdkafka/rdkafka.h>
#include <spdlog/spdlog.h>

namespace ingestion {

KafkaReadingSink::KafkaReadingSink(std::unique_ptr<KafkaClient> client, std::string topic)
    : client_(std::move(client)), topic_(std::move(topic)) {}

void KafkaReadingSink::publish(const Reading& reading) {
    if (!client_)
        return;
    client_->produce(topic_, reading.device_id, reading_to_json(reading));
}

namespace {

class RdKafkaClient : public KafkaClient {
  public:
    explicit RdKafkaClient(rd_kafka_t* rk) : rk_(rk) {}

    ~RdKafkaClient() override {
        if (!rk_)
            return;
        rd_kafka_flush(rk_, 2000);
        rd_kafka_destroy(rk_);
    }

    bool produce(const std::string& topic, const std::string& key,
                 const std::string& payload) override {
        if (!rk_)
            return false;
        rd_kafka_resp_err_t err = rd_kafka_producev(
            rk_, RD_KAFKA_V_TOPIC(topic.c_str()), RD_KAFKA_V_KEY(key.data(), key.size()),
            RD_KAFKA_V_VALUE(const_cast<char*>(payload.data()), payload.size()),
            RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY), RD_KAFKA_V_END);
        rd_kafka_poll(rk_, 0);
        if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
            spdlog::warn("Kafka produce failed: {}", rd_kafka_err2str(err));
            return false;
        }
        return true;
    }

  private:
    rd_kafka_t* rk_ = nullptr;
};

} // namespace

std::unique_ptr<KafkaClient> make_rdkafka_client(const std::string& brokers) {
    if (brokers.empty())
        return nullptr;

    char errstr[512];
    rd_kafka_conf_t* conf = rd_kafka_conf_new();
    if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers.c_str(), errstr, sizeof(errstr)) !=
        RD_KAFKA_CONF_OK) {
        spdlog::error("Kafka conf: {}", errstr);
        rd_kafka_conf_destroy(conf);
        return nullptr;
    }
    // Don't block ingestion if the broker is down — queue and drop oldest.
    rd_kafka_conf_set(conf, "socket.timeout.ms", "3000", nullptr, 0);
    rd_kafka_conf_set(conf, "message.timeout.ms", "5000", nullptr, 0);

    rd_kafka_t* rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
    if (!rk) {
        spdlog::error("Kafka producer: {}", errstr);
        return nullptr;
    }
    // rd_kafka_new takes ownership of conf on success.
    spdlog::info("Kafka producer ready — brokers={}", brokers);
    return std::make_unique<RdKafkaClient>(rk);
}

} // namespace ingestion
