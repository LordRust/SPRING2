# Conda Packaging and Publishing

This guide covers building and publishing SPRING2 as a conda package.

## Recipe Layout

The repository includes a conda recipe in:

- `conda/recipe/meta.yaml`
- `conda/recipe/build.sh`
- `conda/recipe/bld.bat`

## Prerequisites

Install conda-build tooling in your packaging environment:

```bash
conda install -n base conda-build anaconda-client
```

Optional (faster solver/build frontend):

```bash
conda install -n base boa
```

## Local Build

From repository root:

```bash
conda build conda/recipe
```

With boa:

```bash
conda mambabuild conda/recipe
```

After a successful build, the artifact is produced under your conda build cache, typically:

- Linux/macOS: `~/miniconda3/conda-bld/`
- Windows: `%USERPROFILE%\miniconda3\conda-bld\`

## Local Install Test

Install from local build output:

```bash
conda install --use-local spring2
spring2 --version
```

## Publish via conda-forge (Recommended for broad discovery)

1. Create a feedstock using staged-recipes.
2. Submit the SPRING2 recipe.
3. Let conda-forge CI build platform packages.
4. Maintain updates through feedstock pull requests.

## Version Bumps

When releasing a new SPRING2 version:

1. Update `version` in `conda/recipe/meta.yaml`.
2. Increment `build:number` only for recipe-only fixes.
3. Rebuild and retest locally.
4. Upload artifacts or update conda-forge feedstock.
