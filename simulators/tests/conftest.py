import os
import pytest

FIXTURES_DIR = os.path.join(os.path.dirname(__file__), "fixtures")


@pytest.fixture
def valid_config_path():
    return os.path.join(FIXTURES_DIR, "valid_config.yaml")


@pytest.fixture
def minimal_config_path():
    return os.path.join(FIXTURES_DIR, "minimal_config.yaml")
