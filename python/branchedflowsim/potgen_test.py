import pytest
from branchedflowsim import potgen
from branchedflowsim import config


class TC:
    @property
    def args(self): return ["COR"]


def test_make_potgen_command_simple():
    cmd = potgen.make_potgen_command("target_file", 2, 64, TC(), 1.0, 1234, 2, 4, False, True, [])

    assert " ".join(cmd) == config.potgen_exe + " -d 2 -s 64 --derivative-order 2 --threads 4 --output target_file " \
                                                "--seed 1234 --strength 1.0 COR"


def test_make_potgen_command_extents():
    cmd = potgen.make_potgen_command("target_file", 2, [64, 64], TC(), 1.0, 1234, 2, 4, False, True, [])

    assert " ".join(cmd) == config.potgen_exe + " -d 2 -s 64 64 --derivative-order 2 --threads 4 --output target_file " \
                                                "--seed 1234 --strength 1.0 COR"
    # it is important that the sizes are considered independent arguments, so check the list too
    assert cmd == [config.potgen_exe, "-d", "2", "-s", "64", "64", "--derivative-order", "2", "--threads", "4",
                   "--output", "target_file", "--seed", "1234", "--strength", "1.0" , "COR"]

    with pytest.raises(ValueError):
        potgen.make_potgen_command("target_file", 2, [12, 54, 23], TC(), 1.0, 1234, 2, 4, False, True, [])


def test_make_potgen_command_profile():
    cmd = potgen.make_potgen_command("target_file", 2, 64, TC(), 2.0, 1234, 2, 4, True, True, [])

    assert " ".join(cmd) == config.potgen_exe + " -d 2 -s 64 --derivative-order 2 --threads 4 --output target_file " \
                                                "--seed 1234 --strength 2.0 --print-profile COR"


def test_make_potgen_command_no_wisdom():
    cmd = potgen.make_potgen_command("target_file", 2, 64, TC(), 1.0, 1234, 1, 4, False, False, [])

    assert " ".join(cmd) == config.potgen_exe + " -d 2 -s 64 --derivative-order 1 --threads 4 --output target_file " \
                                                "--seed 1234 --strength 1.0 --no-wisdom COR"
