# infinite-car-driving

A 3D endless highway driving simulation written in C++ using the native Win32 API and OpenGL. Zero external game engine dependencies.

---

## Features

- Native Win32 window management and double-buffered OpenGL rendering.
- Planar reflections simulating wet asphalt road conditions.
- Spherical environment mapping (`GL_SPHERE_MAP`) for automotive clearcoat reflections.
- Procedural road and traffic generation with multi-lane density variations.
- Real-time software wave synthesis via Windows Multimedia API (`winmm.lib`) for engine harmonics, turbo spool, and sound effects.

---

## Controls

- `W` / `Up Arrow`: Accelerate
- `S` / `Down Arrow`: Brake / Reverse
- `A` / `Left Arrow`: Steer left
- `D` / `Right Arrow`: Steer right
- `Space`: Nitrous boost
- `C`: Cycle atmospheric conditions (Day, Twilight, Rainy, Night)
- `Esc`: Exit

---

## Build

Requires Windows, an x86_64 C++ compiler, OpenGL, and standard Windows multimedia libraries.

### MinGW (g++):
```bash
g++ main.cpp -o GAME.exe -lopengl32 -lglu32 -lwinmm -mwindows -O2
./GAME.exe
```

### MSVC:
```cmd
build.bat
```

---

## License

MIT
