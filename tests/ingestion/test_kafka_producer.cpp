#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "ingestion/event_bus.h"
#include "ingestion/kafka_producer.h"
#include "ingestion/reading.h"
#include "ingestion/reading_json.h"
#include "ingestion/reading_parser.h"

using namespace ingestion;

TEST(ReadingJsonTest, RoundTripsThroughParser) {
    Reading r{"pump-001", "temperature", 72.5, "C", "2026-09-22T08:00:00Z", true};
    const auto json = reading_to_json(r);

    ReadingParser parser;
    auto parsed = parser.parse(json);
    ASSERT_TRUE(std::holds_alternative<Reading>(parsed));
    EXPECT_EQ(std::get<Reading>(parsed), r);
}

TEST(ReadingJsonTest, ContainsRequiredFields) {
    Reading r{"compressor-001", "current", 12.0, "A", "2026-09-22T08:00:00Z", false};
    const auto json = reading_to_json(r);
    EXPECT_NE(json.find("\"device_id\":\"compressor-001\""), std::string::npos);
    EXPECT_NE(json.find("\"sensor\":\"current\""), std::string::npos);
    EXPECT_NE(json.find("\"anomaly\":false"), std::string::npos);
}

namespace {

class FakeKafkaClient : public KafkaClient {
  public:
    struct Call {
        std::string topic;
        std::string key;
        std::string payload;
    };
    std::vector<Call> calls;
    bool next_ok = true;

    bool produce(const std::string& topic, const std::string& key,
                 const std::string& payload) override {
        calls.push_back({topic, key, payload});
        return next_ok;
    }
};

} // namespace

TEST(KafkaReadingSinkTest, PublishesJsonKeyedByDevice) {
    auto fake = std::make_unique<FakeKafkaClient>();
    auto* rec = fake.get();
    KafkaReadingSink sink(std::move(fake), "forgesight.readings");

    Reading r{"pump-001", "pressure", 2.8, "bar", "2026-09-22T08:00:00Z", false};
    sink.publish(r);

    ASSERT_EQ(rec->calls.size(), 1u);
    EXPECT_EQ(rec->calls[0].topic, "forgesight.readings");
    EXPECT_EQ(rec->calls[0].key, "pump-001");

    ReadingParser parser;
    auto parsed = parser.parse(rec->calls[0].payload);
    ASSERT_TRUE(std::holds_alternative<Reading>(parsed));
    EXPECT_EQ(std::get<Reading>(parsed), r);
}

TEST(KafkaReadingSinkTest, EmptyBrokersReturnsNull) {
    EXPECT_EQ(make_rdkafka_client(""), nullptr);
}

TEST(KafkaReadingSinkTest, EventBusFanoutHitsSink) {
    auto fake = std::make_unique<FakeKafkaClient>();
    auto* rec = fake.get();
    auto sink = std::make_shared<KafkaReadingSink>(std::move(fake), "t");

    EventBus bus;
    bus.subscribe([sink](const ReadingEvent& ev) { sink->publish(ev.reading); });

    Reading r{"d", "s", 1.0, "u", "2026-09-22T08:00:00Z", false};
    bus.publish({r, std::chrono::system_clock::now()});

    ASSERT_EQ(rec->calls.size(), 1u);
    EXPECT_EQ(rec->calls[0].key, "d");
}
