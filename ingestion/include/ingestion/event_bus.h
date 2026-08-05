#pragma once

#include <functional>
#include <mutex>
#include <vector>

#include "reading.h"

namespace ingestion {

using EventHandler = std::function<void(const ReadingEvent&)>;

class EventBus {
  public:
    void subscribe(EventHandler handler);
    void publish(const ReadingEvent& event);
    std::size_t subscriber_count() const;

  private:
    std::vector<EventHandler> handlers_;
    mutable std::mutex mutex_;
};

} // namespace ingestion
