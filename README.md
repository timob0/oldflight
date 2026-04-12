# oldflight

A revived port of the classic SGI-era flight sim.

## Build

### macOS, existing Makefile

```bash
make
./oldflight-bin
```

### Cross-platform CMake build

```bash
cmake -S . -B build
cmake --build build -j4
./build/oldflight
```

The new CMake build is the intended path for modern multiplatform support.

- **macOS**: uses system OpenGL plus the GLUT framework
- **Windows**: targets OpenGL32 + GLU + freeglut/GLUT + WinSock
- **Linux**: targets OpenGL + GLU + freeglut/GLUT

## Vintage Unix build paths

For older Unix workstations, use the conservative makefiles instead of CMake.

### SGI / IRIX style build

```bash
make -f Makefile.sgi
```

### Sun / Solaris style build

```bash
make -f Makefile.sun
```

### Generic Unix / Mesa-GLUT build

```bash
make -f Makefile.unix
```

These makefiles intentionally avoid CMake and modern dependency-generation flags so the code stays friendlier to older toolchains.

### Windows notes

A typical Windows setup is:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build -j4
```

or with Visual Studio:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

You will need a GLUT-compatible library installed, typically **freeglut**.

### GitHub Actions Windows CI

The repository includes `.github/workflows/windows-build.yml` to validate Windows builds in CI.

- **MSVC job**: uses `vcpkg` with the included `vcpkg.json`
- **MinGW job**: uses MSYS2 with `freeglut`

## Command line options

### Replay / airshow files

```bash
./oldflight-bin -i <recording-file>
./oldflight-bin -o <recording-file>
```

or with the CMake build:

```bash
./build/oldflight -i <recording-file>
./build/oldflight -o <recording-file>
```

- `-i <file>` loads a legacy replay/airshow input file
- `-o <file>` writes an output file header for compatibility work (record writing is still incomplete)

### Network play

```bash
./oldflight-bin -net
./oldflight-bin -net 9000
./oldflight-bin -net -name "Timo"
./oldflight-bin -net -host 192.168.1.42
```

- `-net` enables LAN networking on UDP port `7070`
- `-net <port>` enables LAN networking on a custom UDP port
- `-host <ip>` sends packets to a specific host instead of UDP broadcast
- `-name <name>` sets the player name shown to other peers

Without `-name`, oldflight falls back to the local machine hostname.

### Airshow sharing over the network

You can combine replay input with networking so one machine acts as an airshow host and other machines watch it live.

Host machine:

```bash
./oldflight-bin -i myshow.rec -net -name "AirshowHost"
```

Viewer machine(s):

```bash
./oldflight-bin -net -name "Viewer1"
```

This makes the replay planes loaded from `-i` visible to other players on the LAN.

## Portability status

Already done:

- Added a CMake build alongside the legacy macOS Makefile
- Added conservative Unix makefiles for SGI, Sun, and generic Unix builds
- Switched OpenGL/GLUT headers to platform-aware includes without breaking classic `<GL/...>` paths
- Guarded several POSIX-only includes for non-Windows builds
- Guarded MSVC warning pragmas so Clang/GCC stop tripping over them
- Removed several C99-style loop declarations from core files to stay closer to older C compilers
- Replaced a few portability-hostile bits such as raw `#pragma pack(push,1)` assumptions

Still expected before a polished Windows release:

- Build verification on an actual Windows machine
- Possible fixes for any remaining socket/time API differences under MSVC
- Packaging freeglut DLL/runtime expectations for distribution
- Cleanup of old warning debt in legacy headers/source
- Real compile tests on actual IRIX/Solaris hardware or emulators, because vintage C compilers are notoriously temperamental

## Notes

- Networking currently focuses on plane visibility, not synchronized combat/collision gameplay
- LAN discovery uses UDP broadcast by default
- Multiple local sessions on the same machine are supported
- Radar/HUD integrations can read from `planes[]` as usual

## Troubleshooting

- If peers do not appear, make sure all instances use the same UDP port
- If UDP broadcast does not work on your network, try `-host <ip>`
- If names look wrong, pass `-name <value>` explicitly instead of relying on hostname fallback
- If one machine is hosting an airshow, only the host needs `-i <file>`; viewers just need `-net`
- If you test with multiple copies on one machine, that is supported and should now work correctly
