#include "alarm_store.h"

#include <libpq-fe.h>
#include <spdlog/spdlog.h>

namespace alarm_engine {

AlarmStore::AlarmStore(const AlarmStoreConfig& config) {
    conn_ = PQconnectdb(config.connection_string.c_str());
    if (PQstatus(static_cast<PGconn*>(conn_)) != CONNECTION_OK) {
        spdlog::warn("AlarmStore DB connection failed: {}",
                     PQerrorMessage(static_cast<PGconn*>(conn_)));
        PQfinish(static_cast<PGconn*>(conn_));
        conn_ = nullptr;
    }
}

AlarmStore::~AlarmStore() {
    if (conn_) PQfinish(static_cast<PGconn*>(conn_));
}

bool AlarmStore::is_connected() const {
    return conn_ != nullptr;
}

bool AlarmStore::create_tables() {
    if (!conn_) return false;

    const char* ddl_rules =
        "CREATE TABLE IF NOT EXISTS alarm_rules ("
        "  id BIGSERIAL PRIMARY KEY,"
        "  device_id TEXT NOT NULL,"
        "  sensor TEXT NOT NULL,"
        "  condition TEXT NOT NULL,"
        "  threshold DOUBLE PRECISION NOT NULL,"
        "  severity TEXT NOT NULL DEFAULT 'warning',"
        "  message_template TEXT NOT NULL DEFAULT '',"
        "  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()"
        ")";

    const char* ddl_alarms =
        "CREATE TABLE IF NOT EXISTS alarms ("
        "  id BIGSERIAL PRIMARY KEY,"
        "  rule_id BIGINT REFERENCES alarm_rules(id),"
        "  device_id TEXT NOT NULL,"
        "  sensor TEXT NOT NULL,"
        "  value DOUBLE PRECISION NOT NULL,"
        "  severity TEXT NOT NULL DEFAULT 'warning',"
        "  message TEXT NOT NULL DEFAULT '',"
        "  timestamp TIMESTAMPTZ NOT NULL DEFAULT NOW(),"
        "  acknowledged BOOLEAN NOT NULL DEFAULT FALSE,"
        "  acknowledged_at TIMESTAMPTZ,"
        "  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()"
        ")";

    auto* r1 = PQexec(static_cast<PGconn*>(conn_), ddl_rules);
    bool ok1 = PQresultStatus(r1) == PGRES_COMMAND_OK;
    PQclear(r1);

    auto* r2 = PQexec(static_cast<PGconn*>(conn_), ddl_alarms);
    bool ok2 = PQresultStatus(r2) == PGRES_COMMAND_OK;
    PQclear(r2);

    if (!ok1 || !ok2) {
        spdlog::error("Failed to create alarm tables: {}",
                      PQerrorMessage(static_cast<PGconn*>(conn_)));
    }
    return ok1 && ok2;
}

bool AlarmStore::add_rule(const Rule& rule) {
    if (!conn_) return false;

    const char* param_values[5];
    std::string dev = rule.device_id;
    std::string sen = rule.sensor;
    std::string cond = condition_to_string(rule.condition);
    auto thr = std::to_string(rule.threshold);
    std::string sev = severity_to_string(rule.severity);

    param_values[0] = dev.c_str();
    param_values[1] = sen.c_str();
    param_values[2] = cond.c_str();
    param_values[3] = thr.c_str();
    param_values[4] = sev.c_str();

    int lengths[5] = {
        static_cast<int>(dev.size()),
        static_cast<int>(sen.size()),
        static_cast<int>(cond.size()),
        static_cast<int>(thr.size()),
        static_cast<int>(sev.size()),
    };
    int formats[5] = {0, 0, 0, 0, 0};

    auto* res = PQexecParams(
        static_cast<PGconn*>(conn_),
        "INSERT INTO alarm_rules (device_id, sensor, condition, threshold, severity) "
        "VALUES ($1, $2, $3, $4, $5)",
        5, nullptr, param_values, lengths, formats, 0);

    bool ok = PQresultStatus(res) == PGRES_COMMAND_OK;
    if (!ok) {
        spdlog::error("Failed to add rule: {}",
                      PQerrorMessage(static_cast<PGconn*>(conn_)));
    }
    PQclear(res);
    return ok;
}

bool AlarmStore::update_rule(const Rule& rule) {
    if (!conn_) return false;

    auto id = std::to_string(rule.id);
    std::string dev = rule.device_id;
    std::string sen = rule.sensor;
    std::string cond = condition_to_string(rule.condition);
    auto thr = std::to_string(rule.threshold);
    std::string sev = severity_to_string(rule.severity);

    const char* param_values[6] = {
        dev.c_str(), sen.c_str(), cond.c_str(), thr.c_str(), sev.c_str(), id.c_str()
    };
    int lengths[6] = {
        static_cast<int>(dev.size()), static_cast<int>(sen.size()),
        static_cast<int>(cond.size()), static_cast<int>(thr.size()),
        static_cast<int>(sev.size()), static_cast<int>(id.size())
    };
    int formats[6] = {0, 0, 0, 0, 0, 0};

    auto* res = PQexecParams(
        static_cast<PGconn*>(conn_),
        "UPDATE alarm_rules SET device_id=$1, sensor=$2, condition=$3, "
        "threshold=$4, severity=$5 WHERE id=$6",
        6, nullptr, param_values, lengths, formats, 0);

    bool ok = PQresultStatus(res) == PGRES_COMMAND_OK;
    PQclear(res);
    return ok;
}

bool AlarmStore::delete_rule(std::int64_t rule_id) {
    if (!conn_) return false;

    auto id = std::to_string(rule_id);
    const char* param_values[1] = {id.c_str()};
    int lengths[1] = {static_cast<int>(id.size())};
    int formats[1] = {0};

    auto* res = PQexecParams(
        static_cast<PGconn*>(conn_),
        "DELETE FROM alarm_rules WHERE id=$1",
        1, nullptr, param_values, lengths, formats, 0);

    bool ok = PQresultStatus(res) == PGRES_COMMAND_OK;
    PQclear(res);
    return ok;
}

std::vector<Rule> AlarmStore::load_rules() const {
    std::vector<Rule> rules;
    if (!conn_) return rules;

    auto* res = PQexec(static_cast<PGconn*>(conn_),
        "SELECT id, device_id, sensor, condition, threshold, severity "
        "FROM alarm_rules ORDER BY id");

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return rules;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        Rule r;
        r.id = std::stoll(PQgetvalue(res, i, 0));
        r.device_id = PQgetvalue(res, i, 1);
        r.sensor = PQgetvalue(res, i, 2);
        r.condition = condition_from_string(PQgetvalue(res, i, 3));
        r.threshold = std::stod(PQgetvalue(res, i, 4));
        r.severity = severity_from_string(PQgetvalue(res, i, 5));
        rules.push_back(std::move(r));
    }

    PQclear(res);
    return rules;
}

bool AlarmStore::write_alarm(const Alarm& alarm) {
    if (!conn_) return false;

    auto rule_id = std::to_string(alarm.rule_id);
    std::string dev = alarm.device_id;
    std::string sen = alarm.sensor;
    auto val = std::to_string(alarm.value);
    std::string sev = severity_to_string(alarm.severity);
    std::string msg = alarm.message;
    std::string ts = alarm.timestamp;

    const char* param_values[7] = {
        rule_id.c_str(), dev.c_str(), sen.c_str(), val.c_str(),
        sev.c_str(), msg.c_str(), ts.c_str()
    };
    int lengths[7] = {
        static_cast<int>(rule_id.size()), static_cast<int>(dev.size()),
        static_cast<int>(sen.size()), static_cast<int>(val.size()),
        static_cast<int>(sev.size()), static_cast<int>(msg.size()),
        static_cast<int>(ts.size())
    };
    int formats[7] = {0, 0, 0, 0, 0, 0, 0};

    auto* res = PQexecParams(
        static_cast<PGconn*>(conn_),
        "INSERT INTO alarms (rule_id, device_id, sensor, value, severity, message, timestamp) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7)",
        7, nullptr, param_values, lengths, formats, 0);

    bool ok = PQresultStatus(res) == PGRES_COMMAND_OK;
    if (!ok) {
        spdlog::error("Failed to write alarm: {}",
                      PQerrorMessage(static_cast<PGconn*>(conn_)));
    }
    PQclear(res);
    return ok;
}

std::vector<Alarm> AlarmStore::load_alarms(bool unacknowledged_only) const {
    std::vector<Alarm> alarms;
    if (!conn_) return alarms;

    const char* sql = unacknowledged_only
        ? "SELECT id, rule_id, device_id, sensor, value, severity, message, "
          "timestamp::text, acknowledged FROM alarms WHERE acknowledged = FALSE ORDER BY id"
        : "SELECT id, rule_id, device_id, sensor, value, severity, message, "
          "timestamp::text, acknowledged FROM alarms ORDER BY id";

    auto* res = PQexec(static_cast<PGconn*>(conn_), sql);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return alarms;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        Alarm a;
        a.id = std::stoll(PQgetvalue(res, i, 0));
        a.rule_id = std::stoll(PQgetvalue(res, i, 1));
        a.device_id = PQgetvalue(res, i, 2);
        a.sensor = PQgetvalue(res, i, 3);
        a.value = std::stod(PQgetvalue(res, i, 4));
        a.severity = severity_from_string(PQgetvalue(res, i, 5));
        a.message = PQgetvalue(res, i, 6);
        a.timestamp = PQgetvalue(res, i, 7);
        a.acknowledged = (PQgetvalue(res, i, 8)[0] == 't');
        alarms.push_back(std::move(a));
    }

    PQclear(res);
    return alarms;
}

bool AlarmStore::acknowledge_alarm(std::int64_t alarm_id) {
    if (!conn_) return false;

    auto id = std::to_string(alarm_id);
    const char* param_values[1] = {id.c_str()};
    int lengths[1] = {static_cast<int>(id.size())};
    int formats[1] = {0};

    auto* res = PQexecParams(
        static_cast<PGconn*>(conn_),
        "UPDATE alarms SET acknowledged = TRUE, acknowledged_at = NOW() WHERE id = $1",
        1, nullptr, param_values, lengths, formats, 0);

    bool ok = PQresultStatus(res) == PGRES_COMMAND_OK;
    PQclear(res);
    return ok;
}

}  // namespace alarm_engine
