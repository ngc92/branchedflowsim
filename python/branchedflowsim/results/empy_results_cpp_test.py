from branchedflowsim import config
import pytest
import tempfile
import shutil
import subprocess
import os


@pytest.fixture(scope="module")
def results():
    work_dir = tempfile.mkdtemp()
    try:
        print(os.path.join(config.test_dir, "generate_observer_test_files"))
        subprocess.check_call([os.path.join(config.test_dir, "generate_observer_test_files")], cwd=work_dir)
        yield work_dir
    finally:
        shutil.rmtree(work_dir)


def test_empty_result_files(results):
    pass