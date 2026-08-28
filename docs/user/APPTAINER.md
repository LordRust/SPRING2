# Running SPRING2 with Apptainer

An immutable Apptainer image is the recommended way to run SPRING2 on an HPC
system whose host libraries are older than the native SPRING2 build
environment. The image contains its own AlmaLinux 8 userspace and GCC/OpenMP
runtime libraries, so it does not use the host's glibc.

The supplied image recipe targets `x86_64` processors with SSE4.1 and produces
a gzip-compressed SIF suitable for testing with Apptainer 1.0.2.

## Build the Image

Build from a clean, committed checkout on an `x86_64` Linux system with
Apptainer 1.2 or newer. The recipe is tested with Apptainer 1.4.5:

```bash
containers/build-apptainer.sh
```

The helper derives both the source revision and output name from Git. For
example, a checkout described by `v1.3.4-10-gde9529d` produces:

```text
dist/spring2-v1.3.4-10-gde9529d-x86_64.sif
dist/spring2-v1.3.4-10-gde9529d-x86_64.sif.sha256
```

The build compiles the committed source, runs the full CTest suite, creates a
minimal runtime image, and performs an additional round-trip test inside that
image. It requires network access to retrieve the pinned AlmaLinux base images
and build packages.

Set the number of parallel compilation jobs when necessary:

```bash
SPRING_BUILD_JOBS=8 containers/build-apptainer.sh
```

Apptainer temporarily expands the container filesystem during the build. If
`/tmp` is too small, place its temporary data on a larger local filesystem:

```bash
mkdir -p /local/scratch/$USER/apptainer-tmp
APPTAINER_TMPDIR=/local/scratch/$USER/apptainer-tmp \
  SPRING_BUILD_JOBS=8 containers/build-apptainer.sh
```

Do not select zstd or another non-default SquashFS compressor when rebuilding
the SIF for an old cluster. The default gzip compression has the broadest
compatibility with older Apptainer installations.

## Transfer and Verify

Copy both generated files to the cluster, then verify the image before use:

```bash
sha256sum -c spring2-v1.3.4-10-gde9529d-x86_64.sif.sha256
apptainer inspect spring2-v1.3.4-10-gde9529d-x86_64.sif
apptainer run spring2-v1.3.4-10-gde9529d-x86_64.sif --version
```

The checksum file and SIF must remain in the same directory when running
`sha256sum -c`.

Apptainer 1.0.2 is an old runtime. Validate each newly built image with the
three commands above and a small compression/decompression job on a compute
node before using it for production data.

## Generic Shell Usage

The image runscript passes every argument directly to `spring2`:

```bash
image=/path/to/spring2-v1.3.4-10-gde9529d-x86_64.sif
workdir="$(pwd -P)"

apptainer run --cleanenv \
  --bind "$workdir:$workdir" \
  --pwd "$workdir" \
  "$image" \
  -c --R1 reads.fastq.gz -o reads.sp -t 8 -m 16
```

The equivalent explicit executable form is:

```bash
apptainer exec --cleanenv \
  --bind "$workdir:$workdir" \
  --pwd "$workdir" \
  "$image" \
  spring2 --preview reads.sp
```

Current working directories are commonly bound automatically, but explicit
binds make jobs independent of cluster configuration. Add one `--bind` for
each input, output, or scratch filesystem that is not below the working
directory:

```bash
apptainer run --cleanenv \
  --bind /project:/project \
  --bind /scratch:/scratch \
  --pwd /scratch/$USER/job-1234 \
  "$image" \
  -c --R1 /project/reads.fastq.gz \
  -o /scratch/$USER/job-1234/reads.sp -t 16 -m 32
```

SPRING2 creates its disk-backed intermediate work directory next to the output
archive. Put the output on node-local or other fast scratch storage when
possible, then copy the completed archive to durable project storage.

## SLURM Example

The following job requests 16 CPUs and 64 GB of memory. The SPRING2 memory
budget is set slightly below the scheduler allocation to leave headroom for
the container runtime and operating system libraries.

```bash
#!/usr/bin/env bash
#SBATCH --job-name=spring2
#SBATCH --cpus-per-task=16
#SBATCH --mem=64G
#SBATCH --time=08:00:00
#SBATCH --output=spring2-%j.log

set -euo pipefail

module load apptainer

image=/project/software/spring2-v1.3.4-10-gde9529d-x86_64.sif
input=/project/reads/sample.fastq.gz
job_work="${SLURM_TMPDIR:-/scratch/$USER/spring2-${SLURM_JOB_ID}}"
mkdir -p "$job_work"

apptainer run --cleanenv \
  --bind /project:/project \
  --bind "$job_work:$job_work" \
  --pwd "$job_work" \
  "$image" \
  -c --R1 "$input" \
  -o "$job_work/sample.sp" \
  -t "$SLURM_CPUS_PER_TASK" \
  -m 60

cp "$job_work/sample.sp" /project/archives/sample.sp
```

Run one multithreaded SPRING2 process per allocation. SPRING2 does not require
MPI, GPU passthrough, or an Apptainer instance.
