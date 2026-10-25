# Endless Horizon: 3D Procedural Driving Simulation in C++

[![Language: C++](https://img.shields.io/badge/Language-C%2B%2B-00599C.svg?logo=c%2B%2B)](https://isocpp.org/)
[![Graphics: OpenGL](https://img.shields.io/badge/Graphics-OpenGL%20%2F%20Win32-5586A4.svg?logo=opengl)](https://www.opengl.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

A standalone 3D endless highway driving simulation built in pure **C++** using the **native Win32 API** and **OpenGL**, requiring zero external engine dependencies.

---

## 🏎️ Core Technical Features

- **Rendering Engine:** Native Win32 message loop coupled with double-buffered OpenGL rendering.
- **Atmospheric & Lighting Pipeline:**
  - Real-time planar road reflections modeling wet asphalt surface shaders.
  - Spherical environment mapping (`GL_SPHERE_MAP`) simulating clearcoat automotive paint gloss.
  - Procedural mipmapped textures for asphalt grain, city skylines, and anamorphic lens flares.
- **Physics & Gameplay Mechanics:**
  - Dynamic traffic generation with multi-lane density variation.
  - Responsive vehicle inertia, acceleration, steering, and braking models.
  - Volumetric LED beam projection with near-miss collision detection.
- **Audio Synthesizer:** Real-time software wave synthesis via Windows Multimedia API (`winmm.lib` / `mmsystem.h`) delivering engine harmonics, turbo spool, NOS boost, and synthwave audio.

---

## 🎮 Controls

| Action | Primary Key | Secondary Key |
| :--- | :--- | :--- |
| **Steer Left** | `A` | `Left Arrow` |
| **Steer Right** | `D` | `Right Arrow` |
| **Accelerate** | `W` | `Up Arrow` |
| **Brake / Reverse** | `S` | `Down Arrow` |
| **Nitrous (NOS)** | `Space` | `Shift` |
| **Change Atmosphere** | `C` | *(Cycles Day / Twilight / Rainy / Cyberpunk)* |
| **Quit** | `Esc` | — |

---

## 🛠️ Build and Compilation

### Prerequisites
- Windows OS (x86_64)
- Microsoft Visual C++ (`cl.exe`) or MinGW-w64 (`g++`)
- OpenGL and Windows Multimedia libraries (`opengl32.lib`, `glu32.lib`, `winmm.lib`)

### Building with MinGW (g++):
```bash
g++ main.cpp -o GAME.exe -lopengl32 -lglu32 -lwinmm -mwindows -O2
./GAME.exe
```

### Building with MSVC:
Run the included build script:
```cmd
build.bat
```
Or via Developer Command Prompt:
```cmd
cl /O2 /Fe:GAME.exe main.cpp opengl32.lib glu32.lib winmm.lib user32.lib gdi32.lib
```

---

## 📜 License
Distributed under the **MIT License**. See [`LICENSE`](LICENSE) for details.
