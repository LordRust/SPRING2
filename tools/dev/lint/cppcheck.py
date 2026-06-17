#!/usr/bin/env python3

"""Cross-platform cppcheck driver for the SPRING2 repository."""

from __future__ import annotations

import pathlib
import shutil
import subprocess
import sys

ROOT_DIR = pathlib.Path(__file__).resolve().parents[3]
DEFAULT_TARGETS = (ROOT_DIR / "src", ROOT_DIR / "vendor", ROOT_DIR / "tests")
CHECKING_LINE_PREFIX = "Checking "

ZSTD_INCLUDE_DIR = ROOT_DIR / "vendor" / "zstd" / "lib"
LIBBSC_INCLUDE_DIR = ROOT_DIR / "vendor" / "libbsc"
LIBDEFLATE_INCLUDE_DIR = ROOT_DIR / "vendor" / "libdeflate"
LIBARCHIVE_INCLUDE_DIR = ROOT_DIR / "vendor" / "libarchive" / "lib"
ZLIB_INCLUDE_DIR = ROOT_DIR / "vendor" / "cloudflare_zlib"
BZIP2_INCLUDE_DIR = ROOT_DIR / "vendor" / "indexed_bzip2" / "src"
PTHASH_INCLUDE_DIR = ROOT_DIR / "vendor" / "pthash" / "include"
PTHASH_EXTERNAL_DIR = ROOT_DIR / "vendor" / "pthash" / "external"
QVZ_INCLUDE_DIR = ROOT_DIR / "vendor" / "qvz" / "include"

INCLUDE_DIRS = (
    ROOT_DIR / "src",
    ROOT_DIR / "vendor",
    ZSTD_INCLUDE_DIR,
    LIBBSC_INCLUDE_DIR,
    LIBDEFLATE_INCLUDE_DIR,
    LIBARCHIVE_INCLUDE_DIR,
    ZLIB_INCLUDE_DIR,
    BZIP2_INCLUDE_DIR,
    PTHASH_INCLUDE_DIR,
    PTHASH_EXTERNAL_DIR / "xxHash",
    PTHASH_EXTERNAL_DIR / "bits" / "include",
    PTHASH_EXTERNAL_DIR / "bits" / "external" / "essentials" / "include",
    PTHASH_EXTERNAL_DIR / "mm_file" / "include",
    QVZ_INCLUDE_DIR,
)

SUPPRESSIONS = (
    "missingInclude",
    "missingIncludeSystem",
    "normalCheckLevelMaxBranches",
    "toomanyconfigs",
    f"*:{(ROOT_DIR / 'tests' / 'support' / 'doctest.h').as_posix()}",
    "preprocessorErrorDirective:*vendor/libarchive/*",
    "syntaxError:*vendor/libarchive/*",
    "sizeofwithnumericparameter:*vendor/libarchive/*",
    "nullPointerRedundantCheck:*vendor/libarchive/*",
    "memleak:*vendor/libarchive/*",
    "uninitvar:*vendor/libarchive/*",
    "pointerSize:*vendor/libarchive/*",
    "literalWithCharPtrCompare:*vendor/libarchive/*",
    "internalAstError:*vendor/libarchive/*",
    "unknownMacro:*vendor/libarchive/*",
    "nullPointerOutOfMemory:*vendor/libarchive/*",
    "nullPointerArithmeticRedundantCheck:*vendor/libarchive/*",
    "resourceLeak:*vendor/libbsc/*",
    "preprocessorErrorDirective:*vendor/libbsc/*",
    "dangerousTypeCast:*vendor/libbsc/*",
    "identicalInnerCondition:*vendor/libbsc/detectors.cpp",
    "identicalInnerCondition:*vendor/libbsc/filters/detectors.cpp",
    "legacyUninitvar:*vendor/libbsc/st/st.cpp",
    "nullPointerOutOfMemory:*vendor/qvz/*",
    "duplInheritedMember:*vendor/indexed_bzip2/*",
    (
        "identicalConditionAfterEarlyExit:*vendor/indexed_bzip2/src/rapidgzip/"
        "chunkdecoding/GzipChunk.hpp"
    ),
    "sameIteratorExpression:*vendor/indexed_bzip2/src/core/FasterVector.hpp",
    "uninitvar:*vendor/indexed_bzip2/isa-l/*",
    "arrayIndexOutOfBoundsCond:*vendor/libdeflate/*",
    "unknownMacro:*vendor/pthash/*",
    "ctunullpointerOutOfMemory:*vendor/qvz/*",
    "ctuuninitvar:*vendor/libarchive/*",
    "invalidPrintfArgType_sint:*vendor/zstd/*",
    "invalidPrintfArgType_uint:*vendor/zstd/*",
    "uninitvar:*vendor/cloudflare_zlib/deflate.c",
    "localMutex:*vendor/libdeflate/matchfinder_common.h",
    "preprocessorErrorDirective:*vendor/zstd/*",
)


def print_error(message: str) -> None:
    """Write an error message to stderr."""

    print(message, file=sys.stderr)


def require_cppcheck() -> str:
    """Return the cppcheck executable name or fail if it is unavailable."""

    resolved = shutil.which("cppcheck")
    if resolved:
        return "cppcheck"
    print_error("Missing required command: cppcheck")
    raise SystemExit(1)


def resolve_repo_path(raw_path: str) -> pathlib.Path:
    """Resolve a user-supplied path relative to the repository root."""

    path = pathlib.Path(raw_path)
    if path.is_absolute():
        return path.resolve()
    return (ROOT_DIR / path).resolve()


def resolve_targets(arguments: list[str]) -> list[pathlib.Path]:
    """Resolve the cppcheck target list from CLI arguments or defaults."""

    if arguments:
        return [resolve_repo_path(argument) for argument in arguments]
    return list(DEFAULT_TARGETS)


def build_cppcheck_command(cppcheck_bin: str, targets: list[pathlib.Path]) -> list[str]:
    """Build the full cppcheck command line for the selected targets."""

    command = [
        cppcheck_bin,
        "--error-exitcode=1",
        "--enable=warning,performance,portability",
        "-D__BYTE_ORDER__=1",
        "-D__ORDER_LITTLE_ENDIAN__=1",
    ]
    command.extend(f"--suppress={suppression}" for suppression in SUPPRESSIONS)
    for include_dir in INCLUDE_DIRS:
        command.extend(["-I", str(include_dir)])
    command.extend(str(target) for target in targets)
    return command


def run_cppcheck(command: list[str]) -> int:
    """Run cppcheck and hide its per-file checking chatter."""

    process = subprocess.Popen(
        command,
        cwd=ROOT_DIR,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )

    assert process.stdout is not None
    for line in process.stdout:
        stripped = line.lstrip()
        if stripped.startswith(CHECKING_LINE_PREFIX):
            continue
        print(line, end="")

    return process.wait()


def main() -> int:
    """Run cppcheck across the requested repository targets."""

    cppcheck_bin = require_cppcheck()
    targets = resolve_targets(sys.argv[1:])
    if not targets:
        print_error("No targets selected for cppcheck.")
        return 1

    print(f"Running cppcheck on {len(targets)} targets...")
    cppcheck_command = build_cppcheck_command(cppcheck_bin, targets)
    return run_cppcheck(cppcheck_command)


if __name__ == "__main__":
    raise SystemExit(main())
