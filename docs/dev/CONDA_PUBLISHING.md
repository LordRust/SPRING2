# Conda Packaging and Publishing

This guide covers building and publishing SPRING2 as a conda package.

## Recipe Layout

The repository includes a conda recipe in:

- `tools/conda/recipe/recipe.yaml`
- `tools/conda/recipe/build.sh`
- `tools/conda/recipe/build.bat`

## Prerequisites

Install rattler-build in your packaging environment:

```bash
conda install -n base rattler-build
```

## Local Build

From repository root, build against the current checkout by pointing rattler-build
at the recipe and adding conda-forge as a dependency channel:

```bash
rattler-build build \
  --recipe-dir tools/conda/recipe \
  --output-dir output/conda \
  -c conda-forge
```

> **Note:** The `recipe.yaml` source block references the tagged GitHub tarball.
> For local builds you can temporarily change it to `path: ../../..` or use the
> CI helper script in `.github/workflows/ci.yml` as a reference.

After a successful build, packages land under `output/conda/`.

## Local Install Test

Install from the local build output:

```bash
LOCAL_CHANNEL=$(python3 -c "import pathlib; print(pathlib.Path('output/conda').resolve().as_uri())")
conda create -n spring2-test
conda install -n spring2-test -c "$LOCAL_CHANNEL" -c conda-forge spring2
conda run -n spring2-test spring2 --version
```

## Publish via conda-forge (Recommended for broad discovery)

1. Create a feedstock using staged-recipes.
2. Submit the SPRING2 recipe (`tools/conda/recipe/recipe.yaml`).
3. Let conda-forge CI build platform packages.
4. Maintain updates through feedstock pull requests.

## Version Bumps

When releasing a new SPRING2 version:

1. Update `version` in `tools/conda/recipe/recipe.yaml`.
2. Update the `sha256` to match the new release tarball.
3. Reset `build: number` to `0` (increment only for recipe-only fixes).
4. Rebuild and retest locally.
5. Upload artifacts or update the conda-forge feedstock.
