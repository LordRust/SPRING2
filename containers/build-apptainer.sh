#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
repo_root="$(cd -- "${script_dir}/.." && pwd -P)"
definition_file="${script_dir}/Apptainer.def"

for command_name in apptainer git grep sha256sum; do
    if ! command -v "${command_name}" >/dev/null 2>&1; then
        echo "Required command not found: ${command_name}" >&2
        exit 1
    fi
done

if ! apptainer build --help | grep -q -- "--build-arg"; then
    echo "The image builder requires Apptainer 1.2 or newer." >&2
    exit 1
fi

cd "${repo_root}"

if [[ -n "$(git status --porcelain)" ]]; then
    echo "Refusing to build from a dirty worktree." >&2
    echo "Commit or remove local changes so the SIF maps to one Git revision." >&2
    exit 1
fi

if [[ "$(uname -m)" != "x86_64" ]]; then
    echo "This recipe currently builds only the x86_64 image." >&2
    exit 1
fi

build_jobs="${SPRING_BUILD_JOBS:-$(getconf _NPROCESSORS_ONLN)}"
if [[ ! "${build_jobs}" =~ ^[1-9][0-9]*$ ]]; then
    echo "SPRING_BUILD_JOBS must be a positive integer." >&2
    exit 1
fi

spring_revision="$(git rev-parse HEAD)"
spring_describe="$(git describe --tags --always)"
output_dir="${repo_root}/dist"
output_image="${output_dir}/spring2-${spring_describe}-x86_64.sif"
checksum_file="${output_image}.sha256"

if [[ -e "${output_image}" || -e "${checksum_file}" ]]; then
    echo "Refusing to overwrite an existing image or checksum:" >&2
    echo "  ${output_image}" >&2
    exit 1
fi

mkdir -p "${output_dir}"
source_archive="$(mktemp --tmpdir spring2-source.XXXXXX.tar.gz)"
trap 'rm -f "${source_archive}"' EXIT

git archive --format=tar.gz --prefix=SPRING2/ \
    --output="${source_archive}" HEAD

echo "Building SPRING2 ${spring_describe} (${spring_revision})"
echo "Build parallelism: ${build_jobs}"
echo "Output: ${output_image}"

apptainer build \
    --build-arg "SOURCE_ARCHIVE=${source_archive}" \
    --build-arg "BUILD_JOBS=${build_jobs}" \
    --build-arg "SPRING_REVISION=${spring_revision}" \
    --build-arg "SPRING_DESCRIBE=${spring_describe}" \
    "${output_image}" \
    "${definition_file}"

(
    cd "${output_dir}"
    sha256sum "$(basename -- "${output_image}")" \
        > "$(basename -- "${checksum_file}")"
)

echo "Built ${output_image}"
echo "Wrote ${checksum_file}"
