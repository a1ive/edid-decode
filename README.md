# edid-decode

![License](https://img.shields.io/badge/license-MIT-blue.svg)

Win32 port of the [edid-decode](https://git.linuxtv.org/v4l-utils.git/tree/utils/edid-decode) utility from the Linux V4L-utils package.

## Usage

### Decoding from a Connected Monitor

To get and decode the EDID from a specific monitor, use the special `/MONITORX` path, where `X` is the 0-based index of the monitor.

```
edid-decode.exe /MONITOR0
```

### Decoding from a File

If you have the EDID data saved as a binary file (e.g., `edid.bin``), you can pass the file path as an argument.

```
edid-decode.exe path\to\edid.bin
```

### Save the EDID binary to a File

```
edid-decode.exe /MONITOR0 edid.bin
```
