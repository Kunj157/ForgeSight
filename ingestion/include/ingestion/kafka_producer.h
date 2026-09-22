#pragma once

#include <memory>
#include <string>

#include "reading.h"

namespace ingestion {

/// Thin produce() so KafkaReadingSink can be unit-tested without a broker.
class KafkaClient {
  public:
    virtual ~KafkaClient() = default;
    virtual bool produce(const std::string& topic, const std::string& key,
                         const std::string& payload) = 0;
};

/// EventBus subscriber that publishes each reading as JSON to Kafka, keyed
/// by device_id so a partition stays ordered per device.
class KafkaReadingSink {
  public:
    KafkaReadingSink(std::unique_ptr<KafkaClient> client, std::string topic);
    void publish(const Reading& reading);

  private:
    std::unique_ptr<KafkaClient> client_;
    std::string topic_;
};

/// Real librdkafka producer. Returns nullptr when `brokers` is empty so the
/// ingestion binary can stay MQTT-only unless explicitly configured.
std::unique_ptr<KafkaClient> make_rdkafka_client(const std::string& brokers);

} // namespace ingestion
