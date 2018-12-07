import pytest
try:
    import unittest.mock as mock
except ImportError:
    import mock as mock


@pytest.fixture
def file_():
    import tempfile
    with tempfile.TemporaryFile() as tmp_file:
        yield tmp_file
