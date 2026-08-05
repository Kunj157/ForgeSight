import pytest

from simulators.loadgen import build_config, build_device, cpu_percent, parse_proc_stat


class TestBuildDevice:
    def test_id_is_zero_padded_and_unique(self):
        d1 = build_device(1)
        d7 = build_device(7)
        assert d1["id"] == "load-0001"
        assert d7["id"] == "load-0007"

    def test_has_three_sensors(self):
        d = build_device(1)
        assert len(d["sensors"]) == 3
        assert {s["name"] for s in d["sensors"]} == {"temperature", "vibration", "pressure"}

    def test_devices_dont_share_sensor_list_objects(self):
        # Regression guard: sensors must be independent dicts per device, not
        # references to a shared template (mutating one shouldn't leak into
        # another).
        d1 = build_device(1)
        d2 = build_device(2)
        d1["sensors"][0]["min"] = -999
        assert d2["sensors"][0]["min"] != -999


class TestBuildConfig:
    def test_builds_requested_device_count(self):
        cfg = build_config(5)
        assert len(cfg["devices"]) == 5

    def test_default_broker_settings(self):
        cfg = build_config(3)
        assert cfg["broker"]["host"] == "127.0.0.1"
        assert cfg["broker"]["port"] == 1883
        assert cfg["broker"]["topic_prefix"] == "factory/devices"

    def test_custom_publish_interval(self):
        cfg = build_config(3, publish_interval_sec=0.25)
        assert cfg["publish_interval_sec"] == 0.25

    def test_rejects_zero_devices(self):
        with pytest.raises(ValueError, match="n_devices"):
            build_config(0)

    def test_rejects_negative_devices(self):
        with pytest.raises(ValueError, match="n_devices"):
            build_config(-1)


class TestParseProcStat:
    def test_parses_simple_comm(self):
        line = "1234 (ingestion) S 1 1234 1234 0 -1 4194304 100 0 0 0 42 17 0 0 20 0 4 0 12345 " \
               "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 17 3 0 0 0 0 0 0 0 0 0 0 0 0 0"
        utime, stime = parse_proc_stat(line)
        assert utime == 42
        assert stime == 17

    def test_parses_comm_containing_spaces_and_parens(self):
        # comm can be an arbitrary string like "(my (weird) proc)" — parsing
        # must anchor on the *last* ')' in the line, not the first.
        line = "99 (my (weird) proc) R 1 99 99 0 -1 4194304 0 0 0 0 5 9 0 0 20 0 1 0 1 " \
               "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0"
        utime, stime = parse_proc_stat(line)
        assert utime == 5
        assert stime == 9


class TestCpuPercent:
    def test_zero_elapsed_returns_zero(self):
        assert cpu_percent((0, 0), (100, 100), 0.0) == 0.0

    def test_full_core_busy_for_one_second(self):
        # 100 ticks/sec is the common Linux USER_HZ; 100 ticks of combined
        # user+sys time over 1 wall-clock second == one fully busy core.
        assert cpu_percent((0, 0), (50, 50), elapsed_wall_s=1.0, clk_tck=100) == pytest.approx(100.0)

    def test_half_core_busy(self):
        assert cpu_percent((0, 0), (25, 25), elapsed_wall_s=1.0, clk_tck=100) == pytest.approx(50.0)

    def test_never_negative_even_if_counters_look_reversed(self):
        assert cpu_percent((100, 100), (0, 0), elapsed_wall_s=1.0) == 0.0

    def test_accounts_for_prior_baseline(self):
        assert cpu_percent((1000, 500), (1050, 550), elapsed_wall_s=1.0, clk_tck=100) == pytest.approx(100.0)
