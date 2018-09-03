# Dependencies

Before trying to compile uTox make sure you have all of the required dependencies.

## Operating Systems

- [Linux](#linux)
- [FreeBSD](#freebsd)
- [OpenBSD](#openbsd)
- [Windows](#windows)
- [OSX](#osx)

<a name="linux"></a>
## Linux

| Name         | Required              |
|--------------|-----------------------|
| cmake >= 3.2 | yes                   |
| check        | no (needed for tests) |
| dbus         | no                    |
| filter_audio | no                    |
| freetype     | yes                   |
| GTK          | no (runtime only)     |
| opus         | yes                   |
| libvpx       | yes                   |
| openal       | yes                   |
| toxcore      | yes                   |
| v4l          | yes                   |
| xext         | yes                   |
| xrender      | yes                   |
| libsodium    | yes                   |

<a name="freebsd"></a>
## FreeBSD

| Name         | Required              |
|--------------|-----------------------|
| cmake >= 3.2 | yes                   |
| check        | no (needed for tests) |
| dbus         | no                    |
| filter_audio | no                    |
| freetype     | yes                   |
| GTK          | no (runtime only)     |
| opus         | yes                   |
| libvpx       | yes                   |
| openal-soft  | yes                   |
| libv4l       | yes                   |
| v4l\_compat  | yes                   |
| toxcore      | yes                   |
| xrender      | yes                   |
| xext         | yes                   |
| libsodium    | yes                   |

<a name="openbsd"></a>
## OpenBSD

| Name         | Required              |
|--------------|-----------------------|
| cmake >= 3.2 | yes                   |
| check        | no (needed for tests) |
| dbus         | no                    |
| filter_audio | no                    |
| freetype     | yes                   |
| GTK          | no (runtime only)     |
| opus         | yes                   |
| libvpx       | yes                   |
| openal       | yes                   |
| toxcore      | yes                   |
| xrender      | yes                   |
| xext         | yes                   |
| libsodium    | yes                   |

<a name="windows"></a>
## Windows

| Name         | Required |
|--------------|----------|
| cmake >= 3.2 | yes      |
| filter_audio | no       |
| libvpx       | yes      |
| openal       | yes      |
| opus         | yes      |
| toxcore      | yes      |
| libsodium    | yes      |

<a name="osx"></a>
## OSX

| Name         | Required              |
|--------------|-----------------------|
| cmake >= 3.2 | yes                   |
| filter_audio | no                    |
| libvpx       | yes                   |
| openal       | yes                   |
| opus         | yes                   |
| toxcore      | yes                   |
| check        | no (needed for tests) |
| libsodium    | yes                   |
