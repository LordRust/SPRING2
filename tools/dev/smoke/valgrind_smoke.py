#!/usr/bin/env python3

"""Run the SPRING2 smoke test suite under valgrind."""

from __future__ import annotations

import os
import pathlib
import shutil
import subprocess
import sys

ROOT_DIR = pathlib.Path(__file__).resolve().parents[3]
BUILD_DIR = ROOT_DIR / "out" / "build"
VALGRIND_SUPPRESSIONS = pathlib.Path(__file__).resolve().with_name("valgrind.supp")
SMOKE_TEST_BIN = BUILD_DIR / "smoke-tests"


def print_error(message: str) -> None:
    """Write an error message to stderr."""

    print(message, file=sys.stderr)


def require_no_targets(arguments: list[str]) -> None:
    """Reject file or directory arguments for the smoke-test driver."""

    if arguments:
        print_error(
            "valgrind_smoke.py does not accept file or directory targets; it only "
            "runs the SPRING2 smoke test."
        )
        raise SystemExit(1)


def require_command(command_name: str) -> None:
    """Ensure a required executable is present on PATH."""

    if shutil.which(command_name):
        return
    print_error(f"Missing required command: {command_name}")
    raise SystemExit(1)


def require_build_dir() -> None:
    """Ensure the configured build directory exists."""

    if BUILD_DIR.is_dir():
        return
    print_error(
        f"Expected build directory at {BUILD_DIR}\n"
        f"Configure SPRING2 first, for example: cmake -S {ROOT_DIR} -B {BUILD_DIR}"
    )
    raise SystemExit(1)


def build_smoke_tests() -> None:
    """Build the smoke-tests target before execution."""

    build_result = subprocess.run(
        ["cmake", "--build", str(BUILD_DIR), "--target", "smoke-tests", "--parallel"],
        cwd=ROOT_DIR,
        check=False,
    )
    if build_result.returncode != 0:
        raise SystemExit(build_result.returncode)


def require_smoke_test_binary() -> pathlib.Path:
    """Return the smoke-test executable path after validating its presence."""

    candidate_paths = [SMOKE_TEST_BIN, SMOKE_TEST_BIN.with_suffix(".exe")]
    for candidate in candidate_paths:
        if candidate.exists():
            return candidate
    print_error(f"Expected smoke test binary at {SMOKE_TEST_BIN}")
    raise SystemExit(1)


def run_smoke_tests(smoke_test_bin: pathlib.Path) -> int:
    """Run the smoke-test binary with the valgrind wrapper environment."""

    env = os.environ.copy()
    env["SPRING_BIN_WRAPPER"] = (
        "valgrind --leak-check=full --show-leak-kinds=definite,indirect,possible "
        "--errors-for-leak-kinds=definite,indirect "
        f"--suppressions={VALGRIND_SUPPRESSIONS} --error-exitcode=1 --quiet"
    )
    env["RUNNING_ON_VALGRIND"] = "1"
    env["BUILD_DIR"] = str(BUILD_DIR)

    smoke_result = subprocess.run(
        [str(smoke_test_bin)], cwd=ROOT_DIR, env=env, check=False
    )
    return smoke_result.returncode


def main() -> int:
    """Build and run the Linux smoke test target under valgrind."""

    require_no_targets(sys.argv[1:])
    require_command("valgrind")
    require_build_dir()
    build_smoke_tests()
    smoke_test_bin = require_smoke_test_binary()
    return run_smoke_tests(smoke_test_bin)


if __name__ == "__main__":
    raise SystemExit(main())
