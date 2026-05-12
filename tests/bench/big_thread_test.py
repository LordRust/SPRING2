#!/usr/bin/env python3
"""Probe thread-count sensitivity by comparing CRC output across runs."""

from __future__ import annotations

import re
import subprocess
import time

from bench_common import (
    ROOT_DIR,
    default_spring_binary,
    env_or_default_path,
    ensure_directory,
    ensure_spring_binary,
    run_logged_process,
)

SPRING_BIN = env_or_default_path("SPRING_BIN", default_spring_binary())
INPUT_R1 = ROOT_DIR / "tests" / "fixtures" / "input" / "SRR8185389_1.fastq.gz"
INPUT_R2 = ROOT_DIR / "tests" / "fixtures" / "input" / "SRR8185389_2.fastq.gz"
OUTPUT_DIR = ROOT_DIR / "out" / "tests" / "bench" / "thread_test"


def extract_crc(output: str, name: str) -> str:
    """Extract a named CRC value from debug output."""

    match = re.search(rf"{name}=(\d+)", output)
    return match.group(1) if match else "N/A"


def main() -> int:
    """Run the CRC threading experiment and print a pass/fail summary."""

    ensure_directory(OUTPUT_DIR)
    ensure_spring_binary(SPRING_BIN, copy_runtime_dlls=True)
    print("=== CRC Threading Test ===")
    print("Testing different thread counts with SRR8185389 dataset\n")
    results: list[dict[str, object]] = []
    for threads in (1, 2, 4, 8):
        print(f"\n--- Testing with {threads} thread(s) ---")
        archive_path = OUTPUT_DIR / f"test_t{threads}.sp"
        out_path = OUTPUT_DIR / f"out_t{threads}.fastq"
        for stale in OUTPUT_DIR.glob(f"out_t{threads}.fastq*"):
            stale.unlink(missing_ok=True)
        archive_path.unlink(missing_ok=True)

        print(f"  Compressing with {threads} threads...", end="", flush=True)
        comp_start = time.perf_counter()
        try:
            comp_metrics = run_logged_process(
                [
                    str(SPRING_BIN),
                    "-v",
                    "debug",
                    "-c",
                    "--R1",
                    str(INPUT_R1),
                    "--R2",
                    str(INPUT_R2),
                    "-o",
                    str(archive_path),
                    "-t",
                    str(threads),
                ],
                echo=False,
                check=True,
            )
        except subprocess.CalledProcessError:
            print(" FAILED")
            print("  Error during compression")
            continue
        comp_time = time.perf_counter() - comp_start
        print(f" OK ({comp_time:.1f}s)")
        seq_crc_1 = extract_crc(comp_metrics.output, "sequence_crc_1")
        seq_crc_2 = extract_crc(comp_metrics.output, "sequence_crc_2")
        qual_crc_1 = extract_crc(comp_metrics.output, "quality_crc_1")
        qual_crc_2 = extract_crc(comp_metrics.output, "quality_crc_2")
        id_crc_1 = extract_crc(comp_metrics.output, "id_crc_1")
        id_crc_2 = extract_crc(comp_metrics.output, "id_crc_2")
        print("  Compression CRCs:")
        print(f"    Stream 1: seq={seq_crc_1} qual={qual_crc_1} id={id_crc_1}")
        print(f"    Stream 2: seq={seq_crc_2} qual={qual_crc_2} id={id_crc_2}")

        print("  Decompressing...", end="", flush=True)
        decomp_start = time.perf_counter()
        decomp_metrics = run_logged_process(
            [
                str(SPRING_BIN),
                "-v",
                "debug",
                "-d",
                "-i",
                str(archive_path),
                "-o",
                str(out_path),
            ],
            echo=False,
            check=False,
        )
        decomp_time = time.perf_counter() - decomp_start
        if "digest mismatch" not in decomp_metrics.output:
            print(f" OK ({decomp_time:.1f}s)")
            status = "PASS"
        else:
            print(" FAILED")
            status = "FAIL"
            for label, marker in (
                ("Stream 1 sequence mismatch", "Stream 1 sequence digest mismatch"),
                ("Stream 1 quality mismatch", "Stream 1 quality digest mismatch"),
                ("Stream 1 ID mismatch", "Stream 1 ID digest mismatch"),
                ("Stream 2 sequence mismatch", "Stream 2 sequence digest mismatch"),
                ("Stream 2 quality mismatch", "Stream 2 quality digest mismatch"),
                ("Stream 2 ID mismatch", "Stream 2 ID digest mismatch"),
            ):
                if marker in decomp_metrics.output:
                    print(f"    - {label}")

        archive_size_mb = archive_path.stat().st_size / (1024 * 1024)
        results.append(
            {
                "Threads": threads,
                "Status": status,
                "CompTime": round(comp_time, 1),
                "DecompTime": round(decomp_time, 1),
                "ArchiveSizeMB": round(archive_size_mb, 2),
                "SeqCRC1": seq_crc_1,
                "QualCRC1": qual_crc_1,
                "IDCRC1": id_crc_1,
                "SeqCRC2": seq_crc_2,
                "QualCRC2": qual_crc_2,
                "IDCRC2": id_crc_2,
            }
        )

    print("\n\n=== RESULTS SUMMARY ===")
    for result in results:
        print(result)
    print("\n=== CRC COMPARISON ===")
    all_same = True
    for field in ("SeqCRC1", "QualCRC1", "IDCRC1", "SeqCRC2", "QualCRC2", "IDCRC2"):
        values = sorted(
            {str(result[field]) for result in results if result[field] != "N/A"}
        )
        if len(values) > 1:
            print(f"{field} varies: {', '.join(values)}")
            all_same = False
        elif values:
            print(f"{field}: {values[0]}")
    if all_same:
        print("\nCRC values are CONSISTENT across all thread counts.")
        print("The issue is in decompression, not compression.")
    else:
        print("\nCRC values VARY across thread counts!")
        print("This indicates a race condition during compression.")
    pass_count = sum(1 for result in results if result["Status"] == "PASS")
    fail_count = sum(1 for result in results if result["Status"] == "FAIL")
    print("\n=== PASS/FAIL PATTERN ===")
    print(f"Passed: {pass_count} / {len(results)}")
    print(f"Failed: {fail_count} / {len(results)}")
    if fail_count == 0:
        print("\nAll tests PASSED! Issue may be intermittent.")
    elif pass_count == 0:
        print("\nAll tests FAILED! Issue is systematic.")
    else:
        passing = ", ".join(
            str(result["Threads"]) for result in results if result["Status"] == "PASS"
        )
        failing = ", ".join(
            str(result["Threads"]) for result in results if result["Status"] == "FAIL"
        )
        print("\nMixed results. Check if failures correlate with thread count.")
        print(f"Passing thread counts: {passing}")
        print(f"Failing thread counts: {failing}")
    print("\n=== TEST COMPLETE ===")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
