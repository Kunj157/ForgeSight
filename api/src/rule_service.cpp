#include "api/rule_service.h"

#include <libpq-fe.h>
#include <spdlog/spdlog.h>

namespace api {

RuleService::RuleService(void* conn) : conn_(conn) {}

std::vector<alarm_engine::Rule> RuleService::list_rules() const {
    std::vector<alarm_engine::Rule> rules;
    if (!conn_)
        return rules;

    auto* res = PQexec(static_cast<PGconn*>(conn_),
                       "SELECT id, device_id, sensor, condition, threshold, severity "
                       "FROM alarm_rules ORDER BY id");

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return rules;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        alarm_engine::Rule r;
        r.id = std::stoll(PQgetvalue(res, i, 0));
        r.device_id = PQgetvalue(res, i, 1);
        r.sensor = PQgetvalue(res, i, 2);
        r.condition = alarm_engine::condition_from_string(PQgetvalue(res, i, 3));
        r.threshold = std::stod(PQgetvalue(res, i, 4));
        r.severity = alarm_engine::severity_from_string(PQgetvalue(res, i, 5));
        rules.push_back(std::move(r));
    }

    PQclear(res);
    return rules;
}

std::optional<alarm_engine::Rule> RuleService::get_rule(std::int64_t id) const {
    if (!conn_)
        return std::nullopt;

    auto id_str = std::to_string(id);
    const char* params[1] = {id_str.c_str()};
    int lengths[1] = {static_cast<int>(id_str.size())};
    int formats[1] = {0};

    auto* res = PQexecParams(static_cast<PGconn*>(conn_),
                             "SELECT id, device_id, sensor, condition, threshold, severity "
                             "FROM alarm_rules WHERE id = $1",
                             1, nullptr, params, lengths, formats, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return std::nullopt;
    }

    alarm_engine::Rule r;
    r.id = std::stoll(PQgetvalue(res, 0, 0));
    r.device_id = PQgetvalue(res, 0, 1);
    r.sensor = PQgetvalue(res, 0, 2);
    r.condition = alarm_engine::condition_from_string(PQgetvalue(res, 0, 3));
    r.threshold = std::stod(PQgetvalue(res, 0, 4));
    r.severity = alarm_engine::severity_from_string(PQgetvalue(res, 0, 5));

    PQclear(res);
    return r;
}

std::int64_t RuleService::create_rule(const alarm_engine::Rule& rule) {
    if (!conn_)
        return -1;

    std::string dev = rule.device_id;
    std::string sen = rule.sensor;
    std::string cond = alarm_engine::condition_to_string(rule.condition);
    auto thr = std::to_string(rule.threshold);
    std::string sev = alarm_engine::severity_to_string(rule.severity);

    const char* params[5] = {dev.c_str(), sen.c_str(), cond.c_str(), thr.c_str(), sev.c_str()};
    int lengths[5] = {static_cast<int>(dev.size()), static_cast<int>(sen.size()),
                      static_cast<int>(cond.size()), static_cast<int>(thr.size()),
                      static_cast<int>(sev.size())};
    int formats[5] = {0, 0, 0, 0, 0};

    auto* res =
        PQexecParams(static_cast<PGconn*>(conn_),
                     "INSERT INTO alarm_rules (device_id, sensor, condition, threshold, severity) "
                     "VALUES ($1, $2, $3, $4, $5) RETURNING id",
                     5, nullptr, params, lengths, formats, 0);

    std::int64_t new_id = -1;
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        new_id = std::stoll(PQgetvalue(res, 0, 0));
    } else {
        spdlog::error("Failed to create rule: {}", PQerrorMessage(static_cast<PGconn*>(conn_)));
    }
    PQclear(res);
    return new_id;
}

bool RuleService::update_rule(const alarm_engine::Rule& rule) {
    if (!conn_)
        return false;

    auto id = std::to_string(rule.id);
    std::string dev = rule.device_id;
    std::string sen = rule.sensor;
    std::string cond = alarm_engine::condition_to_string(rule.condition);
    auto thr = std::to_string(rule.threshold);
    std::string sev = alarm_engine::severity_to_string(rule.severity);

    const char* params[6] = {dev.c_str(), sen.c_str(), cond.c_str(),
                             thr.c_str(), sev.c_str(), id.c_str()};
    int lengths[6] = {static_cast<int>(dev.size()),  static_cast<int>(sen.size()),
                      static_cast<int>(cond.size()), static_cast<int>(thr.size()),
                      static_cast<int>(sev.size()),  static_cast<int>(id.size())};
    int formats[6] = {0, 0, 0, 0, 0, 0};

    auto* res = PQexecParams(static_cast<PGconn*>(conn_),
                             "UPDATE alarm_rules SET device_id=$1, sensor=$2, condition=$3, "
                             "threshold=$4, severity=$5 WHERE id=$6",
                             6, nullptr, params, lengths, formats, 0);

    bool ok = PQresultStatus(res) == PGRES_COMMAND_OK;
    PQclear(res);
    return ok;
}

bool RuleService::delete_rule(std::int64_t id) {
    if (!conn_)
        return false;

    auto* c = static_cast<PGconn*>(conn_);
    auto id_str = std::to_string(id);
    const char* params[1] = {id_str.c_str()};
    int lengths[1] = {static_cast<int>(id_str.size())};
    int formats[1] = {0};

    // alarms.rule_id references alarm_rules with the default NO ACTION, so a
    // plain DELETE is rejected once the rule has fired. The Rules tab then
    // cannot remove it. Detach the history first (the column is nullable)
    // and keep the alarm rows.
    auto* begin = PQexec(c, "BEGIN");
    bool began = PQresultStatus(begin) == PGRES_COMMAND_OK;
    PQclear(begin);
    if (!began)
        return false;

    auto rollback = [&]() {
        auto* rb = PQexec(c, "ROLLBACK");
        PQclear(rb);
    };

    auto* upd = PQexecParams(c, "UPDATE alarms SET rule_id = NULL WHERE rule_id = $1", 1, nullptr,
                             params, lengths, formats, 0);
    bool upd_ok = PQresultStatus(upd) == PGRES_COMMAND_OK;
    PQclear(upd);
    if (!upd_ok) {
        rollback();
        return false;
    }

    auto* res = PQexecParams(c, "DELETE FROM alarm_rules WHERE id = $1", 1, nullptr, params,
                             lengths, formats, 0);
    bool ok = PQresultStatus(res) == PGRES_COMMAND_OK;
    PQclear(res);
    if (!ok) {
        rollback();
        return false;
    }

    auto* commit = PQexec(c, "COMMIT");
    bool committed = PQresultStatus(commit) == PGRES_COMMAND_OK;
    PQclear(commit);
    if (!committed) {
        rollback();
        return false;
    }
    return true;
}

std::optional<std::int64_t> RuleService::alarm_poll_cursor(const std::string& catchup_since) const {
    if (!conn_)
        return std::nullopt;

    const char* params[1] = {catchup_since.c_str()};
    int lengths[1] = {static_cast<int>(catchup_since.size())};
    int formats[1] = {0};

    auto* res =
        PQexecParams(static_cast<PGconn*>(conn_),
                     "SELECT COALESCE("
                     "  (SELECT MIN(id) - 1 FROM alarms WHERE timestamp >= $1::timestamptz),"
                     "  (SELECT COALESCE(MAX(id), 0) FROM alarms)"
                     ")",
                     1, nullptr, params, lengths, formats, 0);

    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0 && !PQgetisnull(res, 0, 0)) {
        const auto cursor = std::stoll(PQgetvalue(res, 0, 0));
        PQclear(res);
        return cursor;
    }
    PQclear(res);

    auto* max_res = PQexec(static_cast<PGconn*>(conn_), "SELECT COALESCE(MAX(id), 0) FROM alarms");
    std::optional<std::int64_t> cursor;
    if (PQresultStatus(max_res) == PGRES_TUPLES_OK && PQntuples(max_res) > 0 &&
        !PQgetisnull(max_res, 0, 0)) {
        cursor = std::stoll(PQgetvalue(max_res, 0, 0));
    }
    PQclear(max_res);
    return cursor;
}

std::vector<alarm_engine::Alarm> RuleService::alarms_after(std::int64_t after_id) const {
    std::vector<alarm_engine::Alarm> alarms;
    if (!conn_)
        return alarms;

    const std::string id_str = std::to_string(after_id);
    const char* params[1] = {id_str.c_str()};
    int lengths[1] = {static_cast<int>(id_str.size())};
    int formats[1] = {0};

    auto* res = PQexecParams(static_cast<PGconn*>(conn_),
                             "SELECT id, rule_id, device_id, sensor, value, severity, message, "
                             "timestamp::text, acknowledged FROM alarms "
                             "WHERE id > $1::bigint ORDER BY id",
                             1, nullptr, params, lengths, formats, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return alarms;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        alarm_engine::Alarm a;
        a.id = std::stoll(PQgetvalue(res, i, 0));
        // rule_id is nulled when its rule is deleted so the history row can stay.
        a.rule_id = PQgetisnull(res, i, 1) ? 0 : std::stoll(PQgetvalue(res, i, 1));
        a.device_id = PQgetvalue(res, i, 2);
        a.sensor = PQgetvalue(res, i, 3);
        a.value = std::stod(PQgetvalue(res, i, 4));
        a.severity = alarm_engine::severity_from_string(PQgetvalue(res, i, 5));
        a.message = PQgetvalue(res, i, 6);
        a.timestamp = PQgetvalue(res, i, 7);
        a.acknowledged = (PQgetvalue(res, i, 8)[0] == 't');
        alarms.push_back(std::move(a));
    }

    PQclear(res);
    return alarms;
}

std::vector<alarm_engine::Alarm> RuleService::list_alarms(bool unacknowledged_only) const {

    std::vector<alarm_engine::Alarm> alarms;
    if (!conn_)
        return alarms;

    const char* sql = unacknowledged_only
                          ? "SELECT id, rule_id, device_id, sensor, value, severity, message, "
                            "timestamp::text, acknowledged FROM alarms "
                            "WHERE acknowledged = FALSE ORDER BY timestamp DESC LIMIT 200"
                          : "SELECT id, rule_id, device_id, sensor, value, severity, message, "
                            "timestamp::text, acknowledged FROM alarms "
                            "ORDER BY timestamp DESC LIMIT 200";

    auto* res = PQexec(static_cast<PGconn*>(conn_), sql);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return alarms;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        alarm_engine::Alarm a;
        a.id = std::stoll(PQgetvalue(res, i, 0));
        // rule_id is nulled when its rule is deleted so the history row can stay.
        a.rule_id = PQgetisnull(res, i, 1) ? 0 : std::stoll(PQgetvalue(res, i, 1));
        a.device_id = PQgetvalue(res, i, 2);
        a.sensor = PQgetvalue(res, i, 3);
        a.value = std::stod(PQgetvalue(res, i, 4));
        a.severity = alarm_engine::severity_from_string(PQgetvalue(res, i, 5));
        a.message = PQgetvalue(res, i, 6);
        a.timestamp = PQgetvalue(res, i, 7);
        a.acknowledged = (PQgetvalue(res, i, 8)[0] == 't');
        alarms.push_back(std::move(a));
    }

    PQclear(res);
    return alarms;
}

bool RuleService::acknowledge_alarm(std::int64_t id) {
    if (!conn_)
        return false;

    auto id_str = std::to_string(id);
    const char* params[1] = {id_str.c_str()};
    int lengths[1] = {static_cast<int>(id_str.size())};
    int formats[1] = {0};

    auto* res = PQexecParams(static_cast<PGconn*>(conn_),
                             "UPDATE alarms SET acknowledged = TRUE, acknowledged_at = NOW() "
                             "WHERE id = $1 AND acknowledged = FALSE",
                             1, nullptr, params, lengths, formats, 0);

    bool ok = PQresultStatus(res) == PGRES_COMMAND_OK && std::string(PQcmdTuples(res)) == "1";
    PQclear(res);
    return ok;
}

} // namespace api
