#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "reading.h"

namespace ingestion {

struct DbConfig {
    std::string connection_string;
    std::size_t batch_size = 100;
};

class DbWriter {
  public:
    explicit DbWriter(const DbConfig& config);
    ~DbWriter();

    DbWriter(const DbWriter&) = delete;
    DbWriter& operator=(const DbWriter&) = delete;

    bool write(const Reading& reading);
    std::size_t flush();
    bool is_connected() const;

  private:
    bool ensure_table();
    bool insert_batch(std::vector<Reading>& batch);

    DbConfig config_;
    void* conn_ = nullptr;
    std::vector<Reading> buffer_;
    mutable std::mutex mutex_;
};

} // namespace ingestion
