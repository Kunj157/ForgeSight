#pragma once

#include <optional>
#include <string>
#include <variant>

#include "reading.h"

namespace ingestion {

struct ParseError {
    std::string reason;
};

using ParseResult = std::variant<ParseError, Reading>;

class ReadingParser {
  public:
    ParseResult parse(const std::string& json_payload) const;
};

} // namespace ingestion
