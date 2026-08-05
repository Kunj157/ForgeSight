#include "ingestion/event_bus.h"

namespace ingestion {

void EventBus::subscribe(EventHandler handler) {
    std::lock_guard lock(mutex_);
    handlers_.push_back(std::move(handler));
}

void EventBus::publish(const ReadingEvent& event) {
    std::lock_guard lock(mutex_);
    for (auto& handler : handlers_) {
        handler(event);
    }
}

std::size_t EventBus::subscriber_count() const {
    std::lock_guard lock(mutex_);
    return handlers_.size();
}

} // namespace ingestion
