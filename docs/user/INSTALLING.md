# Installing SPRING2

This page covers installing SPRING2 from prebuilt release artifacts. If you
want to compile from source instead, see [Building SPRING2](BUILDING.md).

## Download Releases

Download platform artifacts from the GitHub releases page:

- <https://github.com/thisisamirv/SPRING2/releases>

Available release artifacts currently include:

- Linux AppImages for `x86_64` and `ARM64`
- macOS universal DMG bundles
- macOS universal `.pkg` installers
- Windows `x86_64` and `ARM64` setup installers
- Raw standalone binaries for platforms where they are published

## Linux

Linux releases provide portable AppImages and `.tar.gz` archives.

### AppImage

1. Download the AppImage matching your architecture.
2. Mark it executable.
3. Run it directly.

Example:

```bash
chmod +x spring2-linux-x86_64.AppImage
./spring2-linux-x86_64.AppImage --version
```

If you prefer, you can keep the AppImage in a tools directory and invoke it
from there.

### Tarball

If you want a plain extracted layout instead of AppImage packaging:

```bash
tar -xzf spring2-linux-x86_64.tar.gz
./dist/bin/spring2 --version
```

You can then copy `dist/bin/spring2` to a directory on your `PATH`.

## macOS

macOS releases provide both a DMG and a `.pkg` installer.

### PKG Installer

The `.pkg` installer is the simplest GUI path and does not require Terminal.
It installs `spring2` into `/usr/local/bin`.

1. Download `spring2-macos-universal.pkg`.
2. Open it in the macOS Installer app.
3. Complete the standard installation flow.
4. Open a new Terminal window and run:

```bash
spring2 --version
```

### DMG Bundle

The DMG includes:

- the universal `spring2` binary
- a small `install-spring2.command` helper
- a README with install notes

Use the DMG if you prefer to inspect the bundled files before installing.

## Windows

Windows releases provide architecture-specific setup installers and raw
executables.

### Setup Installer

The Windows installer is the easiest option.

1. Download the setup executable for your architecture:
   - `spring2-windows-x86_64-setup.exe`
   - `spring2-windows-arm64-setup.exe`
2. Run the installer.
3. Optionally enable installer tasks such as:
   - adding SPRING2 to the system `PATH`
   - creating a desktop shortcut
4. Launch `spring2` from the installed location or from a new terminal session
   if you enabled the `PATH` task.

The installer also creates a Start Menu shortcut by default.

### Portable Executable

If you prefer not to install system-wide, you can use the raw `spring2.exe`
artifact directly from any directory.

## Notes on Trust and Security

- Windows signing is optional in the release workflow, so some releases may be
  unsigned.
- macOS packages are currently provided as convenience installers, but if they
  are not Apple-signed or notarized, Gatekeeper may still show warnings on some
  machines.
- Linux AppImages are portable artifacts and do not require system-wide
  installation.
