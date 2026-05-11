SPRING2 for macOS

This disk image contains the universal SPRING2 command-line binary for both
Apple Silicon and Intel Macs.

If you downloaded the `.pkg` installer instead of the disk image, you can
install SPRING2 through the standard macOS Installer app without using
 Terminal. The package installs `spring2` into `/usr/local/bin`.

Quick install

1. Open Terminal.
2. Run the included "install-spring2.command" helper.
3. Or copy the spring2 binary manually to a directory on your PATH, such as:

   /usr/local/bin

Example manual install

  cp spring2 /usr/local/bin/spring2
  chmod +x /usr/local/bin/spring2

Then verify the installation:

  spring2 --version

Documentation

https://spring2.readthedocs.io/
