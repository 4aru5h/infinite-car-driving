# infinite-car-driving

<p align="left">
  <img src="https://github.com/4aru5h/infinite-car-driving/actions/workflows/build.yml/badge.svg" alt="Build Windows Binary" />
  <img src="https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white" alt="C++17" />
  <img src="https://img.shields.io/badge/OpenGL-Double--Buffered-5586A4?logo=opengl&logoColor=white" alt="OpenGL" />
  <img src="https://img.shields.io/badge/Platform-Windows_x64-0078D6?logo=windows&logoColor=white" alt="Windows x64" />
  <img src="https://img.shields.io/badge/License-MIT-blue.svg" alt="License" />
</p>


A standalone 3D endless highway driving simulation written in pure C++ using native Win32 window management and double-buffered OpenGL, engineered without external game engines or third-party runtime dependencies.

---

## Technical Architecture

```text
+-------------------------------------------------------------------------+
|                              Win32 Event Loop                           |
|      (Window Creation, Raw Keystroke Processing, High-Res Timers)       |
+-------------------------------------------------------------------------+
                                     |
         +---------------------------+---------------------------+
         |                                                       |
         v                                                       v
+-------------------------------+               +---------------------------------+
|      OpenGL Render Engine     |               |    Procedural Audio Synthesizer |
|  - Double-buffered contexts   |               |  - Windows Multimedia API       |
|  - Planar asphalt reflections |               |  - Frequency-modulated sine     |
|  - Spherical reflection maps  |               |    waves for engine harmonics   |
|  - Dynamic day/rain/night fog |               |  - Band-limited noise synthesis |
+-------------------------------+               |    for tire skid and turbo      |
         |                                      +---------------------------------+
         v                                                       |
+-------------------------------+                                v
| Procedural Highway & Traffic  |                       Direct Audio Output
|  - Dynamic segment streaming  |                          (waveOutWrite)
|  - Multi-lane AI state machine|
|  - AABB collision detection   |
+-------------------------------+
```

### 1. Graphics & Rendering Engine
- **Platform Management:** Native Win32 API (`CreateWindowEx`, message pump dispatch) eliminating runtime abstraction overhead.
- **Double-Buffered Contexts:** WGL context initialization with depth buffering (`GL_DEPTH_TEST`), dynamic lighting models, and smooth shading.
- **Planar Asphalt Reflections:** Multi-pass rendering simulating wet asphalt road conditions using reflection matrices and surface blending.
- **Spherical Clearcoat Mapping:** Hardware texture generation via `GL_SPHERE_MAP` to produce dynamic specular automotive highlights across the vehicle geometry.
- **Atmospheric Simulations:** Configurable atmospheric presets (Day, Twilight, Rainy, Night) controlling ambient illumination, diffuse falloff, and exponential distance fog (`GL_EXP2`).

### 2. Procedural World Generation & Traffic AI
- **Endless Chunk Recycling:** Generates and recycles highway segment meshes in front of and behind the player position, maintaining stable memory consumption.
- **Autonomous Vehicle State Machines:** Procedural traffic generation with speed variance, lane-selection logic, and responsive braking when closing in on surrounding vehicles.
- **Physics & Collision:** Bounding box (AABB) intersection detection for collision response and lane boundary constraints.

### 3. Real-Time Software Audio Wave Synthesis
- **Zero Third-Party Audio Dependencies:** Built using the Windows Multimedia API (`winmm.lib`) and low-level PCM audio streaming (`waveOutWrite`).
- **Engine RPM Modeling:** Real-time frequency-modulated sine waveform generation synthesizing internal combustion engine harmonics that scale dynamically with acceleration and gear ratios.
- **Turbo & Friction Effects:** Algorithmic noise synthesis modeling turbo spooling pressure releases and asphalt tire friction.

---

## Simulation Controls

| Key | Action |
| :--- | :--- |
| `W` / `Up Arrow` | Accelerate engine |
| `S` / `Down Arrow` | Apply brakes / Reverse |
| `A` / `Left Arrow` | Steer left |
| `D` / `Right Arrow` | Steer right |
| `Space` | Engage nitrous oxide boost |
| `C` | Cycle atmospheric conditions (Day, Twilight, Rainy, Night) |
| `Esc` | Terminate simulation |

---

## Compilation & Execution

Requires Windows (x86_64), a standard C++ compiler, OpenGL, and core Windows multimedia libraries.

### MinGW (GCC):
```bash
g++ main.cpp -o GAME.exe -lopengl32 -lglu32 -lwinmm -mwindows -O2
./GAME.exe
```

### Microsoft Visual Studio (MSVC):
Open the Developer Command Prompt and execute:
```cmd
build.bat
```
Or compile directly:
```cmd
cl /O2 /EHsc main.cpp opengl32.lib glu32.lib winmm.lib user32.lib gdi32.lib
GAME.exe
```

---


---

## Controls & Keybindings

| Keybinding | Action |
| :--- | :--- |
| `W` or `↑` | Accelerate / Forward throttle |
| `S` or `↓` | Brake / Reverse gear |
| `A` or `←` | Steer Left |
| `D` or `→` | Steer Right |
| `Spacebar` | Emergency Handbrake |
| `C` | Cycle Camera Perspective |
| `Escape` | Exit Simulation |

---
## Project Structure

```text
infinite-car-driving/
├── main.cpp         # Complete Win32 windowing, OpenGL rendering, and audio synthesizer
├── build.bat        # MSVC automated compilation script
├── .gitignore       # Build artifacts and executable filter rules
├── LICENSE          # MIT License
└── README.md        # Technical architecture and user guide
```

---

## License

Distributed under the [MIT License](LICENSE).
