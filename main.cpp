// ============================================================================
// NEED FOR SPEED: ENDLESS HORIZON (ULTRAREALISTIC EDITION)
// Native Win32 + OpenGL 3D Endless Highway Driving Game (.exe)
// ============================================================================
// Features:
//   - Correct Left/Right Steering (A/Left = Left, D/Right = Right)
//   - Real-Time Planar Road Reflections (Mirrored 3D world on wet asphalt)
//   - Spherical Environment Mapping (GL_SPHERE_MAP automotive clearcoat gloss)
//   - Procedural High-Resolution Mipmapped Textures (Asphalt, Skyscrapers,
//     Concrete, Anamorphic Lens Flares, Studio/Highway Reflection Map)
//   - Sculpted Multi-Section 3D Supercars with Pillars, Mirrors, Intercooler,
//     Brake Calipers, Anamorphic LED Headlights/Taillights & Volumetric Beams
//   - Real-Time Procedural Audio Synthesizer (Engine harmonics, Turbo, NOS,
//     Drift screech, Near-miss whoosh, Synthwave radio)
// ============================================================================

#ifndef UNICODE
#define UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <mmsystem.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <string>
#include <algorithm>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "winmm.lib")

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_MULTISAMPLE_ARB
#define GL_MULTISAMPLE_ARB 0x809D
#endif

static const float PI = 3.14159265358979323846f;
inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * clampf(t, 0.0f, 1.0f); }
inline float randf(float lo, float hi) { return lo + (hi - lo) * (float(rand() % 10000) / 10000.0f); }

struct Vec3 {
    float x, y, z;
    Vec3(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}
    Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
};

struct Color {
    float r, g, b, a;
    Color(float _r = 1, float _g = 1, float _b = 1, float _a = 1) : r(_r), g(_g), b(_b), a(_a) {}
};

// ============================================================================
// PROCEDURAL GPU TEXTURES (MIPMAPPED)
// ============================================================================
static GLuint g_texAsphalt  = 0;
static GLuint g_texConcrete = 0;
static GLuint g_texBuilding = 0;
static GLuint g_texEnvMap   = 0;
static GLuint g_texGlow     = 0;
static GLuint g_texCarbon   = 0;

static unsigned int texHash(unsigned int x) {
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x = (x >> 16) ^ x;
    return x;
}

void initTextures() {
    glEnable(GL_TEXTURE_2D);

    // 1. High-Detail Asphalt Texture (512x512) with aggregate grain & wheel tracks
    {
        const int W = 512, H = 512;
        std::vector<unsigned char> data(W * H * 4);
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                unsigned int h1 = texHash(x + y * 512u + 101u) & 0xFF;
                unsigned int h2 = texHash((x / 2) + (y / 4) * 256u + 997u) & 0xFF;
                float grain = (float(h1) * 0.65f + float(h2) * 0.35f) / 255.0f;

                // Subtle darker polished tire tracks along each lane
                float laneU = fmodf(float(x) / float(W) * 6.0f, 1.0f);
                float trackDarken = 1.0f;
                if (fabsf(laneU - 0.30f) < 0.12f || fabsf(laneU - 0.70f) < 0.12f) {
                    trackDarken = 0.86f;
                }
                // Tar seam between lanes
                if (laneU < 0.018f || laneU > 0.982f) {
                    trackDarken = 0.65f;
                }

                float baseVal = (26.0f + grain * 32.0f) * trackDarken;
                // Micro specular pebble sparkles
                if ((h1 > 246) && (h2 > 210)) baseVal += 38.0f;

                int idx = (y * W + x) * 4;
                data[idx + 0] = (unsigned char)clampf(baseVal * 0.96f, 0.0f, 255.0f);
                data[idx + 1] = (unsigned char)clampf(baseVal * 0.98f, 0.0f, 255.0f);
                data[idx + 2] = (unsigned char)clampf(baseVal * 1.06f, 0.0f, 255.0f);
                data[idx + 3] = 255;
            }
        }
        glGenTextures(1, &g_texAsphalt);
        glBindTexture(GL_TEXTURE_2D, g_texAsphalt);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, W, H, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    }

    // 2. Concrete Jersey Barrier & Tunnel Wall Texture (256x256)
    {
        const int W = 256, H = 256;
        std::vector<unsigned char> data(W * H * 4);
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                unsigned int h = texHash(x * 3u + y * 257u) & 0xFF;
                float n = float(h) / 255.0f;
                // Bottom road grime & expansion joints
                float grime = (y < 45) ? (0.55f + 0.45f * (y / 45.0f)) : 1.0f;
                if (x % 64 < 3 || y % 64 < 2) grime *= 0.65f;
                float v = (115.0f + n * 38.0f) * grime;
                int idx = (y * W + x) * 4;
                data[idx + 0] = (unsigned char)clampf(v * 0.96f, 0, 255);
                data[idx + 1] = (unsigned char)clampf(v * 0.98f, 0, 255);
                data[idx + 2] = (unsigned char)clampf(v * 1.04f, 0, 255);
                data[idx + 3] = 255;
            }
        }
        glGenTextures(1, &g_texConcrete);
        glBindTexture(GL_TEXTURE_2D, g_texConcrete);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, W, H, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    }

    // 3. Modern Glass Skyscraper Facade Texture with Lit Office Floors (512x512)
    {
        const int W = 512, H = 512;
        std::vector<unsigned char> data(W * H * 4);
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                int cellX = x / 16;
                int cellY = y / 24;
                int localX = x % 16;
                int localY = y % 24;

                bool isMullion = (localX < 2 || localX > 14 || localY < 4 || localY > 21);
                unsigned int roomSeed = texHash(cellX * 37u + cellY * 131u + 55u);
                bool isLit = ((roomSeed & 100) < 58);

                float r = 14.0f, g = 20.0f, b = 32.0f;
                if (!isMullion) {
                    if (isLit) {
                        int colorType = roomSeed % 4;
                        float grad = 0.7f + 0.3f * (float(localY) / 24.0f);
                        // Interior ceiling light & blinds effect
                        if (localY % 4 == 0) grad *= 0.85f;
                        if (colorType == 0) { r = 245 * grad; g = 205 * grad; b = 135 * grad; } // Warm office
                        else if (colorType == 1) { r = 165 * grad; g = 215 * grad; b = 250 * grad; } // Cool LED office
                        else if (colorType == 2) { r = 255 * grad; g = 235 * grad; b = 185 * grad; } // Bright floor
                        else { r = 120 * grad; g = 165 * grad; b = 210 * grad; }
                    } else {
                        // Dark reflective glass pane
                        float skyRefl = 22.0f + (float(y) / float(H)) * 28.0f;
                        r = skyRefl * 0.7f; g = skyRefl * 0.9f; b = skyRefl * 1.35f;
                    }
                } else {
                    r = 10.0f; g = 13.0f; b = 19.0f;
                }
                int idx = (y * W + x) * 4;
                data[idx + 0] = (unsigned char)clampf(r, 0, 255);
                data[idx + 1] = (unsigned char)clampf(g, 0, 255);
                data[idx + 2] = (unsigned char)clampf(b, 0, 255);
                data[idx + 3] = 255;
            }
        }
        glGenTextures(1, &g_texBuilding);
        glBindTexture(GL_TEXTURE_2D, g_texBuilding);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, W, H, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    }

    // 4. Automotive Clearcoat Spherical Environment Map (256x256 GL_SPHERE_MAP)
    {
        const int W = 256, H = 256;
        std::vector<unsigned char> data(W * H * 4);
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                float u = (float(x) / float(W)) * 2.0f - 1.0f;
                float v = (float(y) / float(H)) * 2.0f - 1.0f;
                // Simulate horizon line, sky dome, and passing highway light streaks
                float r, g, b;
                if (v < -0.05f) {
                    // Upper sky reflection + overhead highway sodium/LED light strips
                    float skyT = clampf(-v, 0.0f, 1.0f);
                    r = lerpf(160.0f, 35.0f, skyT);
                    g = lerpf(185.0f, 55.0f, skyT);
                    b = lerpf(225.0f, 105.0f, skyT);
                    // Bright studio/streetlamp specular bands across the car curves!
                    float band1 = expf(-powf((v + 0.35f) * 9.0f, 2.0f)) * expf(-powf(u * 2.0f, 2.0f));
                    float band2 = expf(-powf((fabsf(u) - 0.55f) * 11.0f, 2.0f)) * (1.0f - skyT * 0.5f);
                    r += (band1 * 240.0f + band2 * 180.0f);
                    g += (band1 * 225.0f + band2 * 195.0f);
                    b += (band1 * 200.0f + band2 * 235.0f);
                } else {
                    // Lower dark asphalt reflection
                    float gndT = clampf(v, 0.0f, 1.0f);
                    r = lerpf(90.0f, 18.0f, gndT);
                    g = lerpf(85.0f, 20.0f, gndT);
                    b = lerpf(105.0f, 28.0f, gndT);
                }
                int idx = (y * W + x) * 4;
                data[idx + 0] = (unsigned char)clampf(r, 0, 255);
                data[idx + 1] = (unsigned char)clampf(g, 0, 255);
                data[idx + 2] = (unsigned char)clampf(b, 0, 255);
                data[idx + 3] = 255;
            }
        }
        glGenTextures(1, &g_texEnvMap);
        glBindTexture(GL_TEXTURE_2D, g_texEnvMap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, W, H, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    }

    // 5. Soft Gaussian + Anamorphic Flare Glow Sprite (128x128)
    {
        const int W = 128, H = 128;
        std::vector<unsigned char> data(W * H * 4);
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                float dx = (float(x) - 63.5f) / 63.5f;
                float dy = (float(y) - 63.5f) / 63.5f;
                float dist2 = dx * dx + dy * dy;
                float radial = expf(-dist2 * 5.2f);
                // Anamorphic horizontal streak (classic NFS cinema lens flare!)
                float anamorphic = expf(-dy * dy * 48.0f) * expf(-dx * dx * 1.8f) * 0.65f;
                float core = expf(-dist2 * 22.0f) * 0.6f;
                float alpha = clampf(radial + anamorphic + core, 0.0f, 1.0f);
                if (dist2 > 0.98f) alpha = 0.0f;

                int idx = (y * W + x) * 4;
                data[idx + 0] = 255;
                data[idx + 1] = 255;
                data[idx + 2] = 255;
                data[idx + 3] = (unsigned char)(alpha * 255.0f);
            }
        }
        glGenTextures(1, &g_texGlow);
        glBindTexture(GL_TEXTURE_2D, g_texGlow);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    }

    glDisable(GL_TEXTURE_2D);
}

// ============================================================================
// CAR SPECS & PRESETS
// ============================================================================
struct CarSpec {
    const char* name;
    const char* subtitle;
    float maxSpeedMps;
    float accel;
    float handling;
    float driftFactor;
    float width;
    float length;
    float height;
    Color bodyColor;
    Color accentColor;
    Color neonColor;
    int style; // 0=GTR, 1=Diablo, 2=M3GTR, 3=911, 4=Charger
};

static const CarSpec CAR_SPECS[5] = {
    { "NISSAN SKYLINE GT-R R34", "Twin-Turbo RB26 AWD Spec-V",     99.0f,  35.0f, 1.18f, 1.00f, 2.06f, 4.68f, 1.28f, Color(0.08f, 0.32f, 0.88f), Color(0.90f, 0.92f, 0.98f), Color(0.0f, 0.78f, 1.0f), 0 },
    { "LAMBORGHINI DIABLO SV",   "Naturally Aspirated V12 Wedge", 114.0f,  39.0f, 1.12f, 0.92f, 2.24f, 4.78f, 1.11f, Color(0.98f, 0.74f, 0.04f), Color(0.12f, 0.12f, 0.15f), Color(1.0f, 0.35f, 0.0f), 1 },
    { "BMW M3 GTR MOST WANTED",  "P60B40 V8 Street GT Legend",    104.0f,  37.0f, 1.30f, 1.05f, 2.04f, 4.62f, 1.29f, Color(0.84f, 0.87f, 0.92f), Color(0.04f, 0.34f, 0.85f), Color(0.1f, 0.55f, 1.0f), 2 },
    { "PORSCHE 911 TURBO S",     "Twin-Turbo Flat-6 AWD Coupe",   108.0f,  42.0f, 1.25f, 0.95f, 2.09f, 4.56f, 1.24f, Color(0.86f, 0.08f, 0.18f), Color(0.10f, 0.10f, 0.13f), Color(1.0f, 0.05f, 0.40f), 3 },
    { "DODGE CHARGER R/T V8",    "Supercharged HEMI Street King",  96.0f,  36.0f, 1.02f, 1.35f, 2.14f, 4.98f, 1.31f, Color(0.08f, 0.08f, 0.10f), Color(0.85f, 0.18f, 0.06f), Color(1.0f, 0.25f, 0.0f), 4 }
};

static const Color PAINT_PALETTE[8] = {
    Color(0.08f, 0.32f, 0.88f), // Bayside Blue Metallic
    Color(0.98f, 0.74f, 0.04f), // Giallo Orion Pearl
    Color(0.86f, 0.08f, 0.18f), // Rosso Corsa Crimson
    Color(0.84f, 0.87f, 0.92f), // Liquid Titanium Silver
    Color(0.06f, 0.78f, 0.38f), // Verde Mantis Pearl
    Color(0.42f, 0.08f, 0.78f), // Midnight Purple III
    Color(0.96f, 0.97f, 1.00f), // Championship White
    Color(0.07f, 0.07f, 0.09f)  // Satin Stealth Black
};

static const Color NEON_PALETTE[8] = {
    Color(0.00f, 0.82f, 1.00f), // Ice Cyan
    Color(1.00f, 0.08f, 0.55f), // Neon Magenta
    Color(0.12f, 1.00f, 0.35f), // Nitro Green
    Color(1.00f, 0.42f, 0.00f), // Amber Blaze
    Color(0.55f, 0.18f, 1.00f), // Ultraviolet
    Color(1.00f, 0.88f, 0.10f), // Golden Pulse
    Color(1.00f, 0.12f, 0.12f), // Crimson Glow
    Color(0.00f, 0.00f, 0.00f)  // Underglow Off
};

// ============================================================================
// ATMOSPHERE / LIGHTING BIOMES
// ============================================================================
struct Atmosphere {
    const char* name;
    Color skyTop;
    Color skyMid;
    Color skyHorizon;
    Color fogColor;
    Color roadTint;
    Color terrainCol;
    Color sunColor;
    float sunElevation;
    bool streetLightsOn;
    bool synthGrid;
    bool raining;
    float reflectionStrength; // Real-time planar wet road reflection alpha!
};

static const Atmosphere ATMOSPHERES[5] = {
    // 0: Midnight Underground (Wet Reflective Tarmac & Sodium/LED Highway)
    {
        "MIDNIGHT WET ASPHALT",
        Color(0.01f, 0.02f, 0.05f),
        Color(0.03f, 0.05f, 0.12f),
        Color(0.08f, 0.12f, 0.24f),
        Color(0.05f, 0.07f, 0.14f),
        Color(0.42f, 0.45f, 0.52f),
        Color(0.02f, 0.03f, 0.05f),
        Color(0.80f, 0.88f, 1.00f),
        20.0f, true, false, false, 0.44f
    },
    // 1: Golden Hour Sunset (Warm Anamorphic Sun & Long Reflections)
    {
        "GOLDEN HOUR HORIZON",
        Color(0.08f, 0.04f, 0.18f),
        Color(0.42f, 0.14f, 0.26f),
        Color(0.98f, 0.42f, 0.14f),
        Color(0.55f, 0.24f, 0.18f),
        Color(0.55f, 0.46f, 0.48f),
        Color(0.08f, 0.04f, 0.06f),
        Color(1.00f, 0.68f, 0.22f),
        7.5f, true, false, false, 0.36f
    },
    // 2: Thunderstorm Downpour (Mirror-Wet Highway, Rain & Lightning)
    {
        "THUNDERSTORM DOWNPOUR",
        Color(0.03f, 0.04f, 0.07f),
        Color(0.06f, 0.08f, 0.13f),
        Color(0.12f, 0.16f, 0.22f),
        Color(0.09f, 0.12f, 0.17f),
        Color(0.36f, 0.40f, 0.46f),
        Color(0.02f, 0.03f, 0.04f),
        Color(0.65f, 0.75f, 0.88f),
        28.0f, true, false, true, 0.58f
    },
    // 3: Synthwave Cyber Night
    {
        "CYBERPUNK NEON NIGHT",
        Color(0.02f, 0.00f, 0.08f),
        Color(0.18f, 0.02f, 0.28f),
        Color(0.82f, 0.08f, 0.55f),
        Color(0.24f, 0.03f, 0.28f),
        Color(0.38f, 0.34f, 0.48f),
        Color(0.03f, 0.01f, 0.06f),
        Color(1.00f, 0.18f, 0.65f),
        9.0f, true, true, false, 0.50f
    },
    // 4: Crisp Coastal Daylight
    {
        "COASTAL SUNLIGHT",
        Color(0.08f, 0.32f, 0.72f),
        Color(0.28f, 0.56f, 0.88f),
        Color(0.68f, 0.84f, 0.98f),
        Color(0.58f, 0.76f, 0.92f),
        Color(0.68f, 0.70f, 0.74f),
        Color(0.10f, 0.22f, 0.12f),
        Color(1.00f, 0.98f, 0.88f),
        40.0f, false, false, false, 0.14f
    }
};

// ============================================================================
// HIGHWAY GEOMETRY & TRAFFIC ENTITIES
// ============================================================================
static const float HIGHWAY_HALF_WIDTH = 14.0f; // 6 lanes (28m total width)
static const float SEGMENT_LEN = 12.0f;
static const int   VISIBLE_SEGMENTS_AHEAD = 115;
static const int   VISIBLE_SEGMENTS_BEHIND = 12;
static const float LANE_CENTERS[6] = { -10.5f, -6.3f, -2.1f, 2.1f, 6.3f, 10.5f };

inline void getHighwayPoint(float s, float& outX, float& outY, float& outTunnelFactor) {
    outX = 95.0f * sinf(s * 0.0011f)
         + 48.0f * cosf(s * 0.0027f + 1.3f)
         + 22.0f * sinf(s * 0.0053f + 2.8f);

    outY = 16.0f * sinf(s * 0.0016f + 0.5f)
         + 7.0f  * cosf(s * 0.0039f);

    float modS = fmodf(fabsf(s), 2400.0f);
    outTunnelFactor = (modS > 1700.0f && modS < 2050.0f) ? 1.0f : 0.0f;
}

inline void getHighwayTangent(float s, float& dxds, float& dyds) {
    float x1, y1, t1, x2, y2, t2;
    getHighwayPoint(s - 1.0f, x1, y1, t1);
    getHighwayPoint(s + 1.0f, x2, y2, t2);
    dxds = (x2 - x1) * 0.5f;
    dyds = (y2 - y1) * 0.5f;
}

struct TrafficCar {
    float s;
    float laneOffset;
    float targetLaneOffset;
    float laneChangeTimer;
    float speed;
    int   laneIndex;
    int   type; // 0=Sedan, 1=SUV, 2=Sports, 3=SemiTruck, 4=PoliceInterceptor
    Color color;
    float width, length, height;
    bool  passedNearMiss;
    float hitCooldown;
    float spinYaw;
    float lateralVel;
    int   turnSignal;
};

struct Particle {
    Vec3 pos;
    Vec3 vel;
    Color color;
    float size;
    float life;
    float maxLife;
    int type; // 0=smoke/spray, 1=nitro flame, 2=spark
};

struct SkidMark {
    float s0, x0;
    float s1, x1;
    float alpha;
};

struct Building {
    float s;
    float sideOffset;
    float width, depth, height;
    Color tint;
};

// ============================================================================
// GLOBAL GAME STATE
// ============================================================================
struct GameState {
    int carIndex = 0;
    int paintOverride = -1;
    int neonIndex = 0;

    float s = 100.0f;
    float lateralX = 2.1f; // Starts in forward right-hand lane (+2.1m)
    float speed = 35.0f;
    float lateralVel = 0.0f;
    float yawOffset = 0.0f;
    float pitchAngle = 0.0f;
    float rollAngle = 0.0f;
    float steerInput = 0.0f; // -1.0 = Left, +1.0 = Right
    float wheelRot = 0.0f;

    int gear = 3;
    float rpm = 3500.0f;
    float turboPsi = 0.0f;
    float backfireTimer = 0.0f;

    float nos = 1.0f;
    bool nosActive = false;
    bool drifting = false;
    bool braking = false;
    float driftAngleDeg = 0.0f;

    double totalDistanceM = 0.0;
    float topSpeedMps = 0.0f;
    int nearMissStreak = 0;
    float comboTimer = 0.0f;
    long long flowScore = 0;
    float currentComboPoints = 0.0f;
    std::string popupText;
    float popupTimer = 0.0f;
    Color popupColor = Color(0.2f, 1.0f, 0.9f);

    int cameraMode = 0; // 0=Dynamic Chase, 1=Low Action, 2=Hood/Cockpit, 3=Helicopter
    float camS = 93.0f;
    float camLatX = 2.1f;
    float camHeight = 2.05f;
    float camFov = 64.0f;
    float camShake = 0.0f;
    float camYawLag = 0.0f;

    int atmosphereIdx = 0;
    int trafficDensity = 1;
    bool ghostMode = false;
    bool policeHeat = false;
    bool useKmh = true;
    bool showHelp = true;
    bool minimalHud = false;
    bool paused = false;
    int audioMode = 0;

    float worldTime = 0.0f;
    float lightningFlash = 0.0f;
    float sirenPhase = 0.0f;

    std::vector<TrafficCar> traffic;
    std::vector<Particle> particles;
    std::vector<SkidMark> skidMarks;
    std::vector<Building> buildings;
} g_game;

// ============================================================================
// REAL-TIME PROCEDURAL AUDIO SYNTHESIZER (WinMM WaveOut)
// ============================================================================
static HWAVEOUT g_hWaveOut = NULL;
static const int AUDIO_SAMPLE_RATE = 44100;
static const int AUDIO_BUFFER_SAMPLES = 1470;
static const int AUDIO_NUM_BUFFERS = 4;
static WAVEHDR g_waveHeaders[AUDIO_NUM_BUFFERS];
static short g_audioBuffers[AUDIO_NUM_BUFFERS][AUDIO_BUFFER_SAMPLES];
static bool g_audioRunning = false;

static float g_engPhase1 = 0.0f;
static float g_engPhase2 = 0.0f;
static float g_turboPhase = 0.0f;
static float g_bassPhase = 0.0f;
static float g_leadPhase = 0.0f;
static float g_musicBeat = 0.0f;
static float g_whooshEnv = 0.0f;
static float g_crashEnv = 0.0f;
static unsigned int g_noiseSeed = 1234567u;

inline float fastNoise() {
    g_noiseSeed = g_noiseSeed * 1664525u + 1013904223u;
    return (float(g_noiseSeed & 0xFFFF) / 32768.0f) - 1.0f;
}

void fillAudioBuffer(short* buf, int numSamples) {
    if (g_game.audioMode == 2 || g_game.paused) {
        memset(buf, 0, numSamples * sizeof(short));
        return;
    }

    const CarSpec& spec = CAR_SPECS[g_game.carIndex];
    float rpmNorm = clampf((g_game.rpm - 900.0f) / 7500.0f, 0.0f, 1.0f);
    float basePitchMultiplier = (spec.style == 1) ? 1.35f : (spec.style == 4 ? 0.78f : 1.05f);
    float engFreq = (32.0f + rpmNorm * 145.0f) * basePitchMultiplier;

    static const float bassNotes[8] = { 73.42f, 73.42f, 58.27f, 58.27f, 65.41f, 65.41f, 82.41f, 87.31f };
    static const float arpNotes[8]  = { 293.66f, 349.23f, 440.0f, 587.33f, 523.25f, 440.0f, 349.23f, 440.0f };

    for (int i = 0; i < numSamples; ++i) {
        float dt = 1.0f / float(AUDIO_SAMPLE_RATE);
        float sample = 0.0f;

        g_engPhase1 += engFreq * dt;
        if (g_engPhase1 >= 1.0f) g_engPhase1 -= 1.0f;
        g_engPhase2 += (engFreq * 0.5f) * dt;
        if (g_engPhase2 >= 1.0f) g_engPhase2 -= 1.0f;

        float saw = 2.0f * g_engPhase1 - 1.0f;
        float sub = sinf(g_engPhase2 * 2.0f * PI);
        float harmonic = sinf(g_engPhase1 * 4.0f * PI) * 0.45f + sinf(g_engPhase1 * 6.0f * PI) * 0.25f;
        float engWave = (saw * 0.35f + sub * 0.45f + harmonic * 0.35f);

        if (g_game.rpm > 7850.0f && (int(g_game.worldTime * 35.0f) % 2 == 0)) {
            engWave *= 0.25f;
        }
        if (g_game.backfireTimer > 0.0f) {
            engWave += fastNoise() * 0.9f;
        }
        sample += engWave * (0.14f + rpmNorm * 0.14f);

        if (rpmNorm > 0.35f || g_game.nosActive) {
            float turboFreq = 1100.0f + rpmNorm * 2200.0f + (g_game.nosActive ? 900.0f : 0.0f);
            g_turboPhase += turboFreq * dt;
            if (g_turboPhase >= 1.0f) g_turboPhase -= 1.0f;
            sample += sinf(g_turboPhase * 2.0f * PI) * 0.024f * rpmNorm;
        }
        if (g_game.nosActive) {
            sample += fastNoise() * 0.085f;
        }

        if (g_game.drifting || (g_game.braking && g_game.speed > 20.0f)) {
            float screechFreq = 880.0f + sinf(g_engPhase1 * 20.0f) * 140.0f;
            float screech = sinf(g_engPhase2 * screechFreq * 0.1f) * fastNoise();
            sample += screech * 0.08f;
        }

        if (g_whooshEnv > 0.001f) {
            sample += fastNoise() * g_whooshEnv * 0.25f;
            g_whooshEnv *= 0.9992f;
        }
        if (g_crashEnv > 0.001f) {
            sample += fastNoise() * g_crashEnv * 0.45f;
            g_crashEnv *= 0.9990f;
        }

        if (g_game.audioMode == 0 && g_game.speed > 5.0f) {
            float tempo = 2.1f + clampf(g_game.speed / 120.0f, 0.0f, 0.4f);
            g_musicBeat += dt * tempo * 4.0f;
            int step = int(g_musicBeat) % 32;
            float beatFrac = g_musicBeat - floorf(g_musicBeat);

            float bassFreq = bassNotes[(step / 4) % 8];
            g_bassPhase += bassFreq * dt;
            if (g_bassPhase >= 1.0f) g_bassPhase -= 1.0f;

            float bassEnv = (1.0f - beatFrac);
            bassEnv *= bassEnv;
            float bassSaw = (g_bassPhase < 0.5f ? 0.8f : -0.8f) + sinf(g_bassPhase * 2.0f * PI) * 0.5f;
            sample += bassSaw * bassEnv * 0.082f;

            float arpFreq = arpNotes[step % 8];
            g_leadPhase += arpFreq * dt;
            if (g_leadPhase >= 1.0f) g_leadPhase -= 1.0f;
            float arpEnv = (1.0f - beatFrac * 0.85f);
            sample += sinf(g_leadPhase * 2.0f * PI) * arpEnv * 0.030f;
        }

        if (g_game.policeHeat) {
            float sirenFreq = 650.0f + 280.0f * sinf(g_game.worldTime * 5.5f);
            static float sPhase = 0.0f;
            sPhase += sirenFreq * dt;
            if (sPhase >= 1.0f) sPhase -= 1.0f;
            sample += sinf(sPhase * 2.0f * PI) * 0.028f;
        }

        buf[i] = (short)int(clampf(sample, -0.95f, 0.95f) * 32000.0f);
    }
}

void initAudio() {
    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = AUDIO_SAMPLE_RATE;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = 2;
    wfx.nAvgBytesPerSec = AUDIO_SAMPLE_RATE * 2;

    if (waveOutOpen(&g_hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) == MMSYSERR_NOERROR) {
        g_audioRunning = true;
        for (int i = 0; i < AUDIO_NUM_BUFFERS; ++i) {
            memset(&g_waveHeaders[i], 0, sizeof(WAVEHDR));
            g_waveHeaders[i].lpData = (LPSTR)g_audioBuffers[i];
            g_waveHeaders[i].dwBufferLength = AUDIO_BUFFER_SAMPLES * sizeof(short);
            fillAudioBuffer(g_audioBuffers[i], AUDIO_BUFFER_SAMPLES);
            waveOutPrepareHeader(g_hWaveOut, &g_waveHeaders[i], sizeof(WAVEHDR));
            waveOutWrite(g_hWaveOut, &g_waveHeaders[i], sizeof(WAVEHDR));
        }
    }
}

void updateAudio() {
    if (!g_audioRunning || !g_hWaveOut) return;
    for (int i = 0; i < AUDIO_NUM_BUFFERS; ++i) {
        if (g_waveHeaders[i].dwFlags & WHDR_DONE) {
            fillAudioBuffer(g_audioBuffers[i], AUDIO_BUFFER_SAMPLES);
            waveOutWrite(g_hWaveOut, &g_waveHeaders[i], sizeof(WAVEHDR));
        }
    }
}

void shutdownAudio() {
    if (g_hWaveOut) {
        g_audioRunning = false;
        waveOutReset(g_hWaveOut);
        for (int i = 0; i < AUDIO_NUM_BUFFERS; ++i) {
            waveOutUnprepareHeader(g_hWaveOut, &g_waveHeaders[i], sizeof(WAVEHDR));
        }
        waveOutClose(g_hWaveOut);
        g_hWaveOut = NULL;
    }
}

// ============================================================================
// TRAFFIC & WORLD INITIALIZATION
// ============================================================================
void spawnTrafficCar(TrafficCar& tc, float playerS, bool initialSpread) {
    int lane = rand() % 6;
    tc.laneIndex = lane;
    tc.laneOffset = LANE_CENTERS[lane];
    tc.targetLaneOffset = tc.laneOffset;
    tc.laneChangeTimer = randf(3.0f, 10.0f);
    tc.passedNearMiss = false;
    tc.hitCooldown = 0.0f;
    tc.spinYaw = 0.0f;
    tc.lateralVel = 0.0f;
    tc.turnSignal = 0;

    if (initialSpread) {
        tc.s = playerS + randf(45.0f, VISIBLE_SEGMENTS_AHEAD * SEGMENT_LEN);
    } else {
        tc.s = playerS + VISIBLE_SEGMENTS_AHEAD * SEGMENT_LEN + randf(10.0f, 180.0f);
    }

    if (g_game.policeHeat && (rand() % 6 == 0) && lane >= 2) {
        tc.type = 4;
    } else {
        int r = rand() % 100;
        if (r < 42) tc.type = 0;
        else if (r < 72) tc.type = 1;
        else if (r < 88) tc.type = 2;
        else tc.type = 3;
    }

    static const Color trafficColors[8] = {
        Color(0.76f, 0.79f, 0.84f),
        Color(0.14f, 0.16f, 0.20f),
        Color(0.68f, 0.10f, 0.12f),
        Color(0.12f, 0.30f, 0.62f),
        Color(0.90f, 0.91f, 0.94f),
        Color(0.18f, 0.38f, 0.26f),
        Color(0.75f, 0.52f, 0.12f),
        Color(0.25f, 0.20f, 0.34f)
    };
    tc.color = trafficColors[rand() % 8];

    if (tc.type == 3) {
        tc.width = 2.55f; tc.length = 11.8f; tc.height = 3.30f;
    } else if (tc.type == 1) {
        tc.width = 2.15f; tc.length = 4.85f; tc.height = 1.68f;
    } else if (tc.type == 4) {
        tc.width = 2.08f; tc.length = 4.82f; tc.height = 1.38f;
        tc.color = Color(0.08f, 0.10f, 0.14f);
    } else {
        tc.width = 1.98f; tc.length = 4.58f; tc.height = 1.32f;
    }

    if (lane < 2) {
        // Leftmost 2 lanes are oncoming traffic
        float baseSpd = (lane == 1) ? randf(28.0f, 38.0f) : randf(22.0f, 30.0f);
        tc.speed = -baseSpd;
    } else {
        float laneBonus = (5 - lane) * 5.0f;
        tc.speed = randf(22.0f, 32.0f) + laneBonus;
        if (tc.type == 3) tc.speed = randf(19.0f, 26.0f);
        if (tc.type == 4) tc.speed = std::max(45.0f, g_game.speed * 0.92f);
    }
}

void rebuildTrafficPool() {
    int targetCount = 0;
    if (g_game.trafficDensity == 0) targetCount = 12;
    else if (g_game.trafficDensity == 1) targetCount = 26;
    else if (g_game.trafficDensity == 2) targetCount = 44;
    else targetCount = 0;

    g_game.traffic.resize(targetCount);
    for (int i = 0; i < targetCount; ++i) {
        spawnTrafficCar(g_game.traffic[i], g_game.s, true);
    }
}

void initWorld() {
    srand((unsigned int)time(NULL));
    rebuildTrafficPool();

    g_game.buildings.resize(96);
    for (int i = 0; i < 96; ++i) {
        Building& b = g_game.buildings[i];
        b.s = g_game.s + i * 17.0f;
        bool leftSide = (i % 2 == 0);
        b.width = randf(14.0f, 28.0f);
        b.depth = randf(14.0f, 28.0f);
        b.height = randf(24.0f, 98.0f);
        b.sideOffset = (leftSide ? -1.0f : 1.0f) * (HIGHWAY_HALF_WIDTH + 16.0f + randf(4.0f, 36.0f));
        float shade = randf(0.55f, 0.95f);
        b.tint = Color(shade * 0.92f, shade * 0.96f, shade);
    }
}

void triggerPopup(const std::string& text, const Color& col) {
    g_game.popupText = text;
    g_game.popupColor = col;
    g_game.popupTimer = 1.65f;
}

void spawnParticle(const Vec3& pos, const Vec3& vel, const Color& col, float size, float life, int type) {
    if (g_game.particles.size() > 750) return;
    Particle p;
    p.pos = pos;
    p.vel = vel;
    p.color = col;
    p.size = size;
    p.life = life;
    p.maxLife = life;
    p.type = type;
    g_game.particles.push_back(p);
}

// ============================================================================
// PHYSICS & GAMEPLAY UPDATE
// ============================================================================
void updateGame(float dt, HWND hwnd) {
    if (g_game.paused) return;
    dt = clampf(dt, 0.001f, 0.05f);
    g_game.worldTime += dt;
    g_game.sirenPhase += dt * 10.0f;
    if (g_game.popupTimer > 0.0f) g_game.popupTimer -= dt;
    if (g_game.backfireTimer > 0.0f) g_game.backfireTimer -= dt;
    if (g_game.camShake > 0.0f) g_game.camShake = std::max(0.0f, g_game.camShake - dt * 2.8f);

    if (ATMOSPHERES[g_game.atmosphereIdx].raining) {
        if (g_game.lightningFlash > 0.0f) {
            g_game.lightningFlash = std::max(0.0f, g_game.lightningFlash - dt * 3.5f);
        } else if (rand() % 420 == 0) {
            g_game.lightningFlash = randf(0.5f, 1.0f);
        }
    } else {
        g_game.lightningFlash = 0.0f;
    }

    bool focused = (GetForegroundWindow() == hwnd);
    bool keyThrottle  = focused && ((GetAsyncKeyState('W') & 0x8000) || (GetAsyncKeyState(VK_UP) & 0x8000));
    bool keyBrake     = focused && ((GetAsyncKeyState('S') & 0x8000) || (GetAsyncKeyState(VK_DOWN) & 0x8000));
    bool keyLeft      = focused && ((GetAsyncKeyState('A') & 0x8000) || (GetAsyncKeyState(VK_LEFT) & 0x8000));
    bool keyRight     = focused && ((GetAsyncKeyState('D') & 0x8000) || (GetAsyncKeyState(VK_RIGHT) & 0x8000));
    bool keyNos       = focused && ((GetAsyncKeyState(VK_SPACE) & 0x8000) || (GetAsyncKeyState(VK_LSHIFT) & 0x8000) || (GetAsyncKeyState(VK_RSHIFT) & 0x8000));
    bool keyHandbrake = focused && ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) || (GetAsyncKeyState('X') & 0x8000));

    const CarSpec& spec = CAR_SPECS[g_game.carIndex];

    // Steering: targetSteer < 0 is LEFT (-X), targetSteer > 0 is RIGHT (+X)
    float targetSteer = 0.0f;
    if (keyLeft)  targetSteer -= 1.0f;
    if (keyRight) targetSteer += 1.0f;
    float steerSpeed = (targetSteer != 0.0f) ? 8.8f : 11.5f;
    g_game.steerInput = lerpf(g_game.steerInput, targetSteer, dt * steerSpeed);

    // Nitrous Oxide (NOS)
    if (keyNos && g_game.nos > 0.04f && g_game.speed > 12.0f) {
        g_game.nosActive = true;
        g_game.nos = std::max(0.0f, g_game.nos - dt * 0.23f);
    } else {
        g_game.nosActive = false;
        float rechargeRate = 0.045f;
        if (g_game.speed > 55.0f) rechargeRate += 0.04f;
        if (g_game.lateralX < -4.2f) rechargeRate += 0.06f; // Oncoming lanes recharge bonus
        if (g_game.drifting) rechargeRate += 0.08f;
        g_game.nos = std::min(1.0f, g_game.nos + dt * rechargeRate);
    }

    // Throttle & Brakes
    g_game.braking = keyBrake || keyHandbrake;
    float maxSpd = spec.maxSpeedMps * (g_game.nosActive ? 1.24f : 1.0f);

    if (keyThrottle) {
        float speedRatio = clampf(g_game.speed / maxSpd, 0.0f, 1.0f);
        float torqueCurve = 1.0f - 0.54f * (speedRatio * speedRatio);
        float boostMult = g_game.nosActive ? 1.85f : 1.0f;
        g_game.speed += spec.accel * torqueCurve * boostMult * dt;
        g_game.turboPsi = lerpf(g_game.turboPsi, g_game.nosActive ? 28.0f : 18.5f * (0.4f + 0.6f * speedRatio), dt * 6.0f);
    } else if (keyBrake) {
        g_game.turboPsi = lerpf(g_game.turboPsi, -8.0f, dt * 10.0f);
        if (g_game.speed > 2.0f) {
            g_game.speed -= 56.0f * dt;
        } else {
            g_game.speed = std::max(-18.0f, g_game.speed - 18.0f * dt);
        }
    } else {
        g_game.turboPsi = lerpf(g_game.turboPsi, -4.0f, dt * 7.0f);
        float drag = 0.0022f * g_game.speed * fabsf(g_game.speed) + 3.2f;
        if (g_game.speed > 0.0f) g_game.speed = std::max(0.0f, g_game.speed - drag * dt);
        else g_game.speed = std::min(0.0f, g_game.speed + 8.0f * dt);
    }

    if (keyHandbrake && g_game.speed > 5.0f) {
        g_game.speed = std::max(0.0f, g_game.speed - 28.0f * dt);
    }

    g_game.speed = clampf(g_game.speed, -18.0f, maxSpd);
    if (g_game.speed > g_game.topSpeedMps) g_game.topSpeedMps = g_game.speed;

    // 6-Speed Gearbox
    static const float gearTopSpeeds[6] = { 21.0f, 36.0f, 52.0f, 69.0f, 86.0f, 125.0f };
    if (g_game.speed < -0.5f) {
        g_game.gear = -1;
        g_game.rpm = 1200.0f + (fabsf(g_game.speed) / 18.0f) * 5500.0f;
    } else if (g_game.speed < 1.0f && !keyThrottle) {
        g_game.gear = 0;
        g_game.rpm = lerpf(g_game.rpm, 920.0f + sinf(g_game.worldTime * 14.0f) * 45.0f, dt * 8.0f);
    } else {
        int newGear = 1;
        for (int g = 0; g < 6; ++g) {
            if (g_game.speed > gearTopSpeeds[g] * 0.92f && g < 5) newGear = g + 2;
        }
        if (newGear != g_game.gear) {
            if (newGear > g_game.gear && g_game.speed > 30.0f) g_game.backfireTimer = 0.16f;
            g_game.gear = newGear;
        }
        float prevTop = (g_game.gear <= 1) ? 0.0f : gearTopSpeeds[g_game.gear - 2] * 0.62f;
        float currTop = gearTopSpeeds[g_game.gear - 1];
        float gearRatio = clampf((g_game.speed - prevTop) / (currTop - prevTop), 0.0f, 1.05f);
        float targetRpm = 2100.0f + gearRatio * 6100.0f;
        if (g_game.nosActive) targetRpm = std::min(8450.0f, targetRpm + 550.0f);
        g_game.rpm = lerpf(g_game.rpm, targetRpm, dt * 14.0f);
    }

    // Lateral Steering & Drift Physics
    float speedFactor = clampf(fabsf(g_game.speed) / 15.0f, 0.0f, 1.0f);
    float steerAuthority = (24.0f + fabsf(g_game.speed) * 0.16f) * spec.handling * speedFactor;

    bool wantDrift = (keyHandbrake || (keyBrake && fabsf(g_game.steerInput) > 0.5f)) && g_game.speed > 24.0f;
    float targetYaw = g_game.steerInput * (wantDrift ? 0.42f : 0.15f) * spec.driftFactor * speedFactor;
    float gripRecover = wantDrift ? 3.2f : 7.5f;
    g_game.yawOffset = lerpf(g_game.yawOffset, targetYaw, dt * gripRecover);
    g_game.driftAngleDeg = fabsf(g_game.yawOffset) * (180.0f / PI);
    g_game.drifting = (g_game.driftAngleDeg > 9.5f && g_game.speed > 20.0f);

    float prevLatX = g_game.lateralX;
    float prevS = g_game.s;

    float targetLatVel = g_game.steerInput * steerAuthority + sinf(g_game.yawOffset) * g_game.speed * 0.22f;
    g_game.lateralVel = lerpf(g_game.lateralVel, targetLatVel, dt * (g_game.drifting ? 4.5f : 10.5f));
    g_game.lateralX += g_game.lateralVel * dt;

    g_game.s += g_game.speed * dt;
    if (g_game.speed > 0.0f) g_game.totalDistanceM += g_game.speed * dt;
    g_game.wheelRot += g_game.speed * dt * 2.2f;

    // Suspension Pitch & Roll
    float targetPitch = 0.0f;
    if (keyThrottle) targetPitch = g_game.nosActive ? -2.4f : -1.2f;
    if (g_game.braking && g_game.speed > 5.0f) targetPitch = 2.4f;
    g_game.pitchAngle = lerpf(g_game.pitchAngle, targetPitch, dt * 8.5f);

    float targetRoll = -g_game.steerInput * clampf(g_game.speed / 45.0f, 0.0f, 1.4f) * 3.4f;
    g_game.rollAngle = lerpf(g_game.rollAngle, targetRoll, dt * 9.0f);

    // Guardrail Collision
    float maxLat = HIGHWAY_HALF_WIDTH - spec.width * 0.58f;
    if (fabsf(g_game.lateralX) > maxLat) {
        float sign = (g_game.lateralX > 0.0f) ? 1.0f : -1.0f;
        g_game.lateralX = sign * maxLat;
        g_game.lateralVel = -sign * std::min(fabsf(g_game.lateralVel) * 0.45f, 8.0f);
        g_game.speed *= (1.0f - 0.35f * dt);
        g_game.camShake = std::max(g_game.camShake, 0.25f);
        g_crashEnv = std::max(g_crashEnv, 0.35f);

        float roadX, roadY, tun;
        getHighwayPoint(g_game.s, roadX, roadY, tun);
        for (int i = 0; i < 5; ++i) {
            spawnParticle(
                Vec3(roadX + g_game.lateralX + sign * 0.9f, roadY + 0.5f, g_game.s + randf(-1.5f, 1.5f)),
                Vec3(-sign * randf(2.0f, 8.0f), randf(2.0f, 7.0f), g_game.speed * 0.5f + randf(-5.0f, 5.0f)),
                Color(1.0f, randf(0.55f, 0.95f), 0.15f),
                0.22f, randf(0.2f, 0.45f), 2
            );
        }
    }

    // Tire Smoke & Skidmarks
    if ((g_game.drifting || (g_game.braking && g_game.speed > 28.0f)) && g_game.skidMarks.size() < 400) {
        SkidMark sm;
        sm.s0 = prevS - 1.6f; sm.x0 = prevLatX;
        sm.s1 = g_game.s - 1.6f; sm.x1 = g_game.lateralX;
        sm.alpha = 0.78f;
        g_game.skidMarks.push_back(sm);

        float roadX, roadY, tun;
        getHighwayPoint(g_game.s - 1.8f, roadX, roadY, tun);
        for (int side = -1; side <= 1; side += 2) {
            spawnParticle(
                Vec3(roadX + g_game.lateralX + side * 0.75f, roadY + 0.15f, g_game.s - 1.8f),
                Vec3(randf(-1.5f, 1.5f), randf(0.4f, 1.4f), g_game.speed * 0.25f),
                Color(0.82f, 0.85f, 0.90f, 0.45f),
                randf(0.55f, 0.95f), randf(0.35f, 0.7f), 0
            );
        }
    }

    // Wet Road Rooster-Tail Spray in Rain!
    if (ATMOSPHERES[g_game.atmosphereIdx].raining && g_game.speed > 22.0f) {
        float roadX, roadY, tun;
        getHighwayPoint(g_game.s - 2.1f, roadX, roadY, tun);
        if (tun < 0.5f) {
            for (int side = -1; side <= 1; side += 2) {
                spawnParticle(
                    Vec3(roadX + g_game.lateralX + side * 0.8f, roadY + 0.18f, g_game.s - 2.1f),
                    Vec3(side * randf(0.5f, 2.0f), randf(0.8f, 2.2f), g_game.speed * 0.35f),
                    Color(0.75f, 0.82f, 0.92f, 0.28f),
                    randf(0.45f, 0.85f), randf(0.20f, 0.40f), 0
                );
            }
        }
    }

    // NOS & Backfire Plasma Particles
    if (g_game.nosActive || g_game.backfireTimer > 0.0f) {
        float roadX, roadY, tun;
        getHighwayPoint(g_game.s - 2.2f, roadX, roadY, tun);
        Color flameCol = g_game.nosActive
            ? Color(0.20f, 0.72f, 1.0f, 0.95f)
            : Color(1.0f, 0.48f, 0.1f, 0.95f);
        for (int side = -1; side <= 1; side += 2) {
            spawnParticle(
                Vec3(roadX + g_game.lateralX + side * 0.48f, roadY + 0.32f, g_game.s - 2.3f),
                Vec3(randf(-0.4f, 0.4f), randf(-0.1f, 0.3f), g_game.speed - randf(12.0f, 22.0f)),
                flameCol,
                randf(0.35f, 0.60f), randf(0.06f, 0.14f), 1
            );
        }
    }

    // Flow Score
    if (g_game.comboTimer > 0.0f) {
        g_game.comboTimer -= dt;
        if (g_game.comboTimer <= 0.0f) {
            g_game.flowScore += (long long)(g_game.currentComboPoints * std::max(1, g_game.nearMissStreak));
            g_game.currentComboPoints = 0.0f;
            g_game.nearMissStreak = 0;
        }
    }
    if (g_game.speed > 45.0f) {
        float mult = 1.0f + float(g_game.nearMissStreak) * 0.5f;
        if (g_game.lateralX < -4.2f) mult *= 1.5f;
        if (g_game.drifting) mult *= 2.0f;
        g_game.flowScore += (long long)(g_game.speed * mult * dt * 2.5f);
    }

    // Update Traffic
    for (size_t i = 0; i < g_game.traffic.size(); ++i) {
        TrafficCar& tc = g_game.traffic[i];
        if (tc.hitCooldown > 0.0f) tc.hitCooldown -= dt;

        if (tc.type == 4 && g_game.policeHeat) {
            float targetSpd = std::max(42.0f, g_game.speed * (tc.s > g_game.s ? 0.88f : 1.08f));
            tc.speed = lerpf(tc.speed, targetSpd, dt * 2.0f);
            tc.targetLaneOffset = clampf(g_game.lateralX, -2.0f, 10.5f);
            tc.laneOffset = lerpf(tc.laneOffset, tc.targetLaneOffset, dt * 1.4f);
        } else {
            tc.laneChangeTimer -= dt;
            if (tc.laneChangeTimer <= 0.0f) {
                tc.laneChangeTimer = randf(5.0f, 14.0f);
                if (tc.laneIndex >= 2 && tc.type != 3 && (rand() % 3 == 0)) {
                    int newLane = (int)clampf(float(tc.laneIndex + ((rand() % 2 == 0) ? 1 : -1)), 2.0f, 5.0f);
                    tc.turnSignal = (newLane > tc.laneIndex) ? 1 : -1;
                    tc.laneIndex = newLane;
                    tc.targetLaneOffset = LANE_CENTERS[newLane];
                } else {
                    tc.turnSignal = 0;
                }
            }
            tc.laneOffset = lerpf(tc.laneOffset, tc.targetLaneOffset, dt * 1.8f);
            if (fabsf(tc.laneOffset - tc.targetLaneOffset) < 0.15f) tc.turnSignal = 0;
        }

        tc.laneOffset += tc.lateralVel * dt;
        tc.lateralVel = lerpf(tc.lateralVel, 0.0f, dt * 4.0f);
        tc.spinYaw = lerpf(tc.spinYaw, 0.0f, dt * 3.5f);
        tc.s += tc.speed * dt;

        for (size_t j = 0; j < g_game.traffic.size(); ++j) {
            if (i == j) continue;
            TrafficCar& other = g_game.traffic[j];
            if (fabsf(tc.laneOffset - other.laneOffset) < 2.2f) {
                float ds = other.s - tc.s;
                float minGap = (tc.length + other.length) * 0.65f + 8.0f;
                if (tc.speed > 0 && ds > 0 && ds < minGap && tc.speed > other.speed) {
                    tc.speed = other.speed;
                }
            }
        }

        float ds = tc.s - g_game.s;
        float dx = tc.laneOffset - g_game.lateralX;
        float sumHalfLen = (tc.length + spec.length) * 0.46f;
        float sumHalfWid = (tc.width + spec.width) * 0.46f;

        if (!g_game.ghostMode && tc.hitCooldown <= 0.0f &&
            fabsf(ds) < sumHalfLen && fabsf(dx) < sumHalfWid) {
            tc.hitCooldown = 0.65f;
            g_game.camShake = 0.65f;
            g_crashEnv = 0.85f;

            float pushDir = (dx >= 0.0f) ? 1.0f : -1.0f;
            tc.lateralVel = pushDir * 14.0f;
            tc.spinYaw = pushDir * 0.35f;
            if (ds > 0.0f && tc.speed > 0.0f) {
                tc.speed = std::max(tc.speed, g_game.speed * 0.85f + 10.0f);
            }
            g_game.lateralVel = -pushDir * 7.5f;
            g_game.speed = std::max(18.0f, g_game.speed * 0.72f);
            g_game.nearMissStreak = 0;
            g_game.comboTimer = 0.0f;
            triggerPopup("TRAFFIC CLIP! KEEP ROLLING", Color(1.0f, 0.35f, 0.25f));

            float roadX, roadY, tun;
            getHighwayPoint(g_game.s, roadX, roadY, tun);
            for (int k = 0; k < 20; ++k) {
                spawnParticle(
                    Vec3(roadX + g_game.lateralX + dx * 0.5f, roadY + 0.65f, g_game.s + ds * 0.5f),
                    Vec3(randf(-12.0f, 12.0f), randf(3.0f, 11.0f), g_game.speed * 0.6f + randf(-10.0f, 10.0f)),
                    Color(1.0f, randf(0.4f, 0.95f), 0.1f),
                    randf(0.22f, 0.38f), randf(0.3f, 0.65f), 2
                );
            }
        } else if (!tc.passedNearMiss && ds < 0.0f && ds > -12.0f) {
            tc.passedNearMiss = true;
            float clearance = fabsf(dx) - sumHalfWid;
            if (clearance < 1.85f && g_game.speed > 36.0f) {
                g_game.nearMissStreak++;
                g_game.comboTimer = 4.0f;
                g_game.nos = std::min(1.0f, g_game.nos + 0.12f);
                g_whooshEnv = 0.75f;

                bool oncoming = (tc.speed < 0.0f);
                int pts = (oncoming ? 750 : 400) * g_game.nearMissStreak;
                g_game.currentComboPoints += pts;

                char buf[96];
                if (g_game.nearMissStreak >= 5) {
                    snprintf(buf, sizeof(buf), "THREAD THE NEEDLE! x%d (+%d)", g_game.nearMissStreak, pts);
                    triggerPopup(buf, Color(1.0f, 0.25f, 0.85f));
                } else if (oncoming) {
                    snprintf(buf, sizeof(buf), "ONCOMING NEAR MISS! x%d (+%d)", g_game.nearMissStreak, pts);
                    triggerPopup(buf, Color(1.0f, 0.85f, 0.15f));
                } else {
                    snprintf(buf, sizeof(buf), "NEAR MISS! x%d (+%d)", g_game.nearMissStreak, pts);
                    triggerPopup(buf, Color(0.15f, 0.95f, 1.0f));
                }
            }
        }

        if (tc.s < g_game.s - 80.0f || tc.s > g_game.s + VISIBLE_SEGMENTS_AHEAD * SEGMENT_LEN + 220.0f) {
            spawnTrafficCar(tc, g_game.s, false);
        }
    }

    for (size_t i = 0; i < g_game.buildings.size(); ++i) {
        if (g_game.buildings[i].s < g_game.s - 90.0f) {
            g_game.buildings[i].s += 96.0f * 17.0f;
        }
    }

    for (int i = (int)g_game.skidMarks.size() - 1; i >= 0; --i) {
        g_game.skidMarks[i].alpha -= dt * 0.25f;
        if (g_game.skidMarks[i].alpha <= 0.0f || g_game.skidMarks[i].s1 < g_game.s - 60.0f) {
            g_game.skidMarks.erase(g_game.skidMarks.begin() + i);
        }
    }

    for (int i = (int)g_game.particles.size() - 1; i >= 0; --i) {
        Particle& p = g_game.particles[i];
        p.life -= dt;
        if (p.life <= 0.0f) {
            g_game.particles[i] = g_game.particles.back();
            g_game.particles.pop_back();
            continue;
        }
        p.pos = p.pos + p.vel * dt;
        if (p.type == 2) p.vel.y -= 18.0f * dt;
        else if (p.type == 0) p.size += dt * 1.4f;
    }

    // Camera Dynamics
    float targetFov = 60.0f + clampf(g_game.speed / 110.0f, 0.0f, 1.0f) * 20.0f + (g_game.nosActive ? 10.0f : 0.0f);
    g_game.camFov = lerpf(g_game.camFov, targetFov, dt * 6.0f);
    g_game.camYawLag = lerpf(g_game.camYawLag, g_game.yawOffset * 0.65f + g_game.steerInput * 0.08f, dt * 6.5f);

    float distBehind = 6.4f;
    float heightOffset = 1.92f;
    if (g_game.cameraMode == 1) {
        distBehind = 4.9f;
        heightOffset = 1.32f;
    } else if (g_game.cameraMode == 2) {
        distBehind = -0.65f;
        heightOffset = 1.12f;
    } else if (g_game.cameraMode == 3) {
        distBehind = 10.2f;
        heightOffset = 3.70f;
    }
    if (g_game.nosActive && g_game.cameraMode != 2) distBehind += 0.8f;

    g_game.camS = g_game.s - distBehind;
    g_game.camLatX = lerpf(g_game.camLatX, g_game.lateralX, dt * (g_game.cameraMode == 2 ? 25.0f : 9.5f));
    g_game.camHeight = lerpf(g_game.camHeight, heightOffset, dt * 8.0f);
}

// ============================================================================
// ULTRAREALISTIC 3D GEOMETRY, SHADING & FLARE HELPERS
// ============================================================================
// Draw an additive anamorphic lens flare / glow sprite in 3D space facing camera
void drawGlowBillboard(float x, float y, float z, float w, float h, const Color& col) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_texGlow);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    glColor4f(col.r, col.g, col.b, col.a);
    float hw = w * 0.5f;
    float hh = h * 0.5f;
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x - hw, y - hh, z);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x + hw, y - hh, z);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x + hw, y + hh, z);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x - hw, y + hh, z);
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// Draw a sculpted 3D bevelled / aerodynamic car body section with smooth vertex normals
// for realistic OpenGL specular lighting and spherical environment reflections!
void drawSculptedSection(
    float z0, float z1,
    float wBot0, float wTop0, float yBot0, float yTop0,
    float wBot1, float wTop1, float yBot1, float yTop1,
    const Color& col, float topBr = 1.12f, float sideBr = 0.85f)
{
    float xB0 = wBot0 * 0.5f, xT0 = wTop0 * 0.5f;
    float xB1 = wBot1 * 0.5f, xT1 = wTop1 * 0.5f;

    glBegin(GL_QUADS);
    // Top Hood / Roof / Decklid Surface
    glNormal3f(0.0f, 0.96f, 0.28f);
    glColor4f(col.r * topBr, col.g * topBr, col.b * topBr, col.a);
    glVertex3f(-xT0, yTop0, z0);
    glVertex3f(-xT1, yTop1, z1);
    glVertex3f( xT1, yTop1, z1);
    glVertex3f( xT0, yTop0, z0);

    // Right Sculpted Body Side (+X)
    glNormal3f(0.92f, 0.38f, 0.0f);
    glColor4f(col.r * sideBr, col.g * sideBr, col.b * sideBr, col.a);
    glVertex3f( xB0, yBot0, z0);
    glVertex3f( xT0, yTop0, z0);
    glVertex3f( xT1, yTop1, z1);
    glVertex3f( xB1, yBot1, z1);

    // Left Sculpted Body Side (-X)
    glNormal3f(-0.92f, 0.38f, 0.0f);
    glVertex3f(-xB0, yBot0, z0);
    glVertex3f(-xB1, yBot1, z1);
    glVertex3f(-xT1, yTop1, z1);
    glVertex3f(-xT0, yTop0, z0);

    // Front Face at z1
    glNormal3f(0.0f, 0.1f, 1.0f);
    glColor4f(col.r * 0.92f, col.g * 0.92f, col.b * 0.92f, col.a);
    glVertex3f(-xB1, yBot1, z1);
    glVertex3f( xB1, yBot1, z1);
    glVertex3f( xT1, yTop1, z1);
    glVertex3f(-xT1, yTop1, z1);

    // Rear Face at z0
    glNormal3f(0.0f, 0.1f, -1.0f);
    glColor4f(col.r * 0.76f, col.g * 0.76f, col.b * 0.76f, col.a);
    glVertex3f(-xB0, yBot0, z0);
    glVertex3f(-xT0, yTop0, z0);
    glVertex3f( xT0, yTop0, z0);
    glVertex3f( xB0, yBot0, z0);
    glEnd();
}

void drawBox(float w, float h, float l, const Color& col, float topShade = 1.12f, float sideShade = 0.82f) {
    float hw = w * 0.5f, hh = h * 0.5f, hl = l * 0.5f;
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glColor4f(col.r * topShade, col.g * topShade, col.b * topShade, col.a);
    glVertex3f(-hw,  hh, -hl); glVertex3f(-hw,  hh,  hl);
    glVertex3f( hw,  hh,  hl); glVertex3f( hw,  hh, -hl);

    glNormal3f(0.0f, -1.0f, 0.0f);
    glColor4f(col.r * 0.4f, col.g * 0.4f, col.b * 0.4f, col.a);
    glVertex3f(-hw, -hh, -hl); glVertex3f( hw, -hh, -hl);
    glVertex3f( hw, -hh,  hl); glVertex3f(-hw, -hh,  hl);

    glNormal3f(0.0f, 0.0f, 1.0f);
    glColor4f(col.r * 0.95f, col.g * 0.95f, col.b * 0.95f, col.a);
    glVertex3f(-hw, -hh,  hl); glVertex3f( hw, -hh,  hl);
    glVertex3f( hw,  hh,  hl); glVertex3f(-hw,  hh,  hl);

    glNormal3f(0.0f, 0.0f, -1.0f);
    glColor4f(col.r * 0.75f, col.g * 0.75f, col.b * 0.75f, col.a);
    glVertex3f(-hw, -hh, -hl); glVertex3f(-hw,  hh, -hl);
    glVertex3f( hw,  hh, -hl); glVertex3f( hw, -hh, -hl);

    glNormal3f(-1.0f, 0.0f, 0.0f);
    glColor4f(col.r * sideShade, col.g * sideShade, col.b * sideShade, col.a);
    glVertex3f(-hw, -hh, -hl); glVertex3f(-hw, -hh,  hl);
    glVertex3f(-hw,  hh,  hl); glVertex3f(-hw,  hh, -hl);

    glNormal3f(1.0f, 0.0f, 0.0f);
    glColor4f(col.r * sideShade, col.g * sideShade, col.b * sideShade, col.a);
    glVertex3f( hw, -hh, -hl); glVertex3f( hw,  hh, -hl);
    glVertex3f( hw,  hh,  hl); glVertex3f( hw, -hh,  hl);
    glEnd();
}

void drawWheelDetailed(float x, float y, float z, float radius, float width, float steerDeg, float spinRad, bool glowingDisc) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(steerDeg, 0.0f, 1.0f, 0.0f);

    const int segs = 18;
    float hw = width * 0.5f;

    // 1. Rubber Tire Tread with Sidewall Shoulder
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segs; ++i) {
        float ang = (float(i) / segs) * 2.0f * PI + spinRad;
        float cy = sinf(ang) * radius;
        float cz = cosf(ang) * radius;
        float shade = 0.08f + 0.04f * cosf(ang * 2.0f);
        glColor3f(shade, shade, shade + 0.01f);
        glVertex3f(-hw, cy, cz);
        glVertex3f( hw, cy, cz);
    }
    glEnd();

    // 2. Deep-Dish Metallic Alloy Rim Barrel & 5 Split Spokes
    float rimX = (x >= 0.0f) ? hw + 0.01f : -hw - 0.01f;
    float innerX = (x >= 0.0f) ? hw * 0.65f : -hw * 0.65f;

    // Drilled Brake Rotor Disc + Brembo Caliper inside wheel
    glBegin(GL_TRIANGLE_FAN);
    if (glowingDisc) glColor3f(1.0f, 0.35f, 0.06f);
    else glColor3f(0.38f, 0.40f, 0.44f);
    glVertex3f(innerX, 0.0f, 0.0f);
    for (int i = 0; i <= segs; ++i) {
        float ang = (float(i) / segs) * 2.0f * PI;
        glVertex3f(innerX, sinf(ang) * radius * 0.60f, cosf(ang) * radius * 0.60f);
    }
    glEnd();

    // Outer Alloy Rim Lip
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segs; ++i) {
        float ang = (float(i) / segs) * 2.0f * PI;
        glColor3f(0.78f, 0.82f, 0.88f);
        glVertex3f(rimX, sinf(ang) * radius * 0.78f, cosf(ang) * radius * 0.78f);
        glColor3f(0.45f, 0.48f, 0.54f);
        glVertex3f(rimX, sinf(ang) * radius * 0.68f, cosf(ang) * radius * 0.68f);
    }
    glEnd();

    // 6 Forged Alloy Spokes
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glColor3f(0.84f, 0.88f, 0.94f);
    for (int s = 0; s < 6; ++s) {
        float ang = (float(s) / 6.0f) * 2.0f * PI + spinRad;
        glVertex3f(rimX, 0.0f, 0.0f);
        glVertex3f(rimX, sinf(ang) * radius * 0.72f, cosf(ang) * radius * 0.72f);
    }
    glEnd();

    glPopMatrix();
}

// ============================================================================
// HIGH-DETAIL SCULPTED PLAYER SUPERCAR WITH GL_SPHERE_MAP CLEARCOAT
// ============================================================================
void drawPlayerCar(bool isReflectionPass) {
    if (g_game.cameraMode == 2 && !isReflectionPass) return;

    const CarSpec& spec = CAR_SPECS[g_game.carIndex];
    Color bodyCol = (g_game.paintOverride >= 0) ? PAINT_PALETTE[g_game.paintOverride] : spec.bodyColor;
    Color neonCol = (g_game.neonIndex == 0) ? spec.neonColor : NEON_PALETTE[g_game.neonIndex - 1];

    float roadX, roadY, tun;
    float dxds, dyds;
    getHighwayPoint(g_game.s, roadX, roadY, tun);
    getHighwayTangent(g_game.s, dxds, dyds);
    float roadYawDeg = atan2f(dxds, 1.0f) * (180.0f / PI);
    float roadPitchDeg = -atan2f(dyds, 1.0f) * (180.0f / PI);

    glPushMatrix();
    glTranslatef(roadX + g_game.lateralX, roadY, g_game.s);
    glRotatef(roadYawDeg + g_game.yawOffset * (180.0f / PI), 0.0f, 1.0f, 0.0f);
    glRotatef(roadPitchDeg + g_game.pitchAngle, 1.0f, 0.0f, 0.0f);
    glRotatef(g_game.rollAngle, 0.0f, 0.0f, 1.0f);

    float w = spec.width;
    float l = spec.length;
    float h = spec.height;
    float hl = l * 0.5f;

    // 1. Soft Ambient Occlusion Contact Shadow + Neon Underglow
    if (!isReflectionPass) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBegin(GL_QUADS);
        glColor4f(0.0f, 0.0f, 0.0f, 0.84f);
        glVertex3f(-w * 0.56f, 0.025f, -hl * 1.05f);
        glVertex3f(-w * 0.56f, 0.025f,  hl * 1.05f);
        glVertex3f( w * 0.56f, 0.025f,  hl * 1.05f);
        glVertex3f( w * 0.56f, 0.025f, -hl * 1.05f);
        glEnd();

        if (neonCol.r + neonCol.g + neonCol.b > 0.05f) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            float pulse = 0.50f + 0.14f * sinf(g_game.worldTime * 6.0f);
            glBegin(GL_QUADS);
            glColor4f(neonCol.r, neonCol.g, neonCol.b, pulse);
            glVertex3f(-w * 0.68f, 0.035f, -hl * 1.02f);
            glVertex3f(-w * 0.68f, 0.035f,  hl * 1.02f);
            glVertex3f( w * 0.68f, 0.035f,  hl * 1.02f);
            glVertex3f( w * 0.68f, 0.035f, -hl * 1.02f);
            glEnd();
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
    }

    // 2. Sculpted Multi-Section Aerodynamic Bodywork (Two Passes: Base Paint + Spherical Clearcoat Reflection!)
    for (int pass = 0; pass < 2; ++pass) {
        Color c = bodyCol;
        if (pass == 1) {
            // Pass 1: Glossy Spherical Environment Map Reflection Layer!
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, g_texEnvMap);
            glEnable(GL_TEXTURE_GEN_S);
            glEnable(GL_TEXTURE_GEN_T);
            glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            c = Color(0.75f, 0.82f, 0.95f, 0.36f);
        }

        // Rear Bumper & Quarter Panels (-hl to -hl*0.35)
        drawSculptedSection(
            -hl, -hl * 0.35f,
            w * 0.94f, w * 0.86f, 0.20f, h * 0.58f,
            w * 1.02f, w * 0.92f, 0.18f, h * 0.60f, c);

        // Center Doors & Widebody Side Skirts (-hl*0.35 to +hl*0.28)
        drawSculptedSection(
            -hl * 0.35f, hl * 0.28f,
            w * 1.02f, w * 0.92f, 0.16f, h * 0.60f,
            w * 1.00f, w * 0.90f, 0.16f, h * 0.56f, c);

        // Sculpted Sloping Front Hood & Fenders (+hl*0.28 to +hl*0.86)
        drawSculptedSection(
            hl * 0.28f, hl * 0.86f,
            w * 1.00f, w * 0.90f, 0.16f, h * 0.56f,
            w * 0.94f, w * 0.82f, 0.17f, h * 0.44f, c);

        // Front Aero Nose Cone (+hl*0.86 to +hl)
        drawSculptedSection(
            hl * 0.86f, hl,
            w * 0.94f, w * 0.82f, 0.17f, h * 0.44f,
            w * 0.86f, w * 0.74f, 0.16f, h * 0.34f, c);

        // Roof Panel & A/C Pillars
        float cabZ0 = (spec.style == 1) ? -hl * 0.55f : -hl * 0.58f;
        float cabZ1 = (spec.style == 1) ?  hl * 0.38f :  hl * 0.28f;
        drawSculptedSection(
            cabZ0, cabZ1,
            w * 0.84f, w * 0.66f, h * 0.58f, h * 0.96f,
            w * 0.82f, w * 0.64f, h * 0.56f, h * 0.94f,
            (pass == 0) ? Color(0.06f, 0.08f, 0.12f, 1.0f) : Color(0.85f, 0.92f, 1.0f, 0.48f));

        // Roof Metal Top Cap
        glPushMatrix();
        glTranslatef(0.0f, h * 0.96f, (cabZ0 + cabZ1) * 0.42f);
        drawBox(w * 0.65f, 0.04f, (cabZ1 - cabZ0) * 0.55f, c);
        glPopMatrix();

        // Aerodynamic Side Mirrors
        for (int side = -1; side <= 1; side += 2) {
            glPushMatrix();
            glTranslatef(side * w * 0.52f, h * 0.64f, hl * 0.18f);
            drawBox(0.16f, 0.09f, 0.20f, c);
            glPopMatrix();
        }

        if (pass == 1) {
            glDisable(GL_TEXTURE_GEN_S);
            glDisable(GL_TEXTURE_GEN_T);
            glDisable(GL_TEXTURE_2D);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
    }

    // 3. Carbon Fiber Front Lip, Side Skirt Blades, Rear Diffuser & Intercooler Grille
    Color carbon(0.08f, 0.09f, 0.11f);
    // Front Splitter Lip
    glPushMatrix();
    glTranslatef(0.0f, 0.14f, hl * 0.98f);
    drawBox(w * 0.94f, 0.06f, 0.24f, carbon);
    glPopMatrix();

    // Front Intercooler Metallic Core visible inside Front Bumper Intake!
    glPushMatrix();
    glTranslatef(0.0f, 0.23f, hl * 1.002f);
    drawBox(w * 0.46f, 0.15f, 0.04f, Color(0.55f, 0.58f, 0.64f));
    glPopMatrix();

    // Rear Multi-Fin Carbon Diffuser & License Plate
    glPushMatrix();
    glTranslatef(0.0f, 0.18f, -hl * 0.99f);
    drawBox(w * 0.88f, 0.14f, 0.16f, carbon);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.0f, h * 0.34f, -hl * 1.005f);
    drawBox(0.38f, 0.16f, 0.02f, Color(0.92f, 0.94f, 0.98f)); // Illuminated JDM/Euro Plate
    glPopMatrix();

    // Racing Livery Stripes (M3 GTR / Charger)
    if (spec.style == 2 || spec.style == 4) {
        glPushMatrix();
        glTranslatef(0.0f, h * 0.58f, 0.0f);
        drawBox(w * 0.30f, 0.05f, l * 0.92f, spec.accentColor);
        glPopMatrix();
    }

    // Supercharger Blower Scoop for Muscle V8
    if (spec.style == 4) {
        glPushMatrix();
        glTranslatef(0.0f, h * 0.62f, hl * 0.48f);
        drawBox(0.46f, 0.22f, 0.65f, Color(0.78f, 0.82f, 0.88f));
        glPopMatrix();
    }

    // 4. Sculpted Rear GT Wing & Endplates
    if (spec.style != 4) {
        float wingH = (spec.style == 2 || spec.style == 0) ? h * 0.82f : h * 0.68f;
        for (int side = -1; side <= 1; side += 2) {
            glPushMatrix();
            glTranslatef(side * w * 0.28f, (h * 0.56f + wingH) * 0.5f, -hl * 0.86f);
            drawBox(0.05f, wingH - h * 0.52f, 0.24f, carbon);
            glPopMatrix();
            // Wing Endplates
            glPushMatrix();
            glTranslatef(side * w * 0.47f, wingH, -hl * 0.88f);
            drawBox(0.03f, 0.16f, 0.36f, bodyCol);
            glPopMatrix();
        }
        glPushMatrix();
        glTranslatef(0.0f, wingH, -hl * 0.88f);
        drawBox(w * 0.94f, 0.05f, 0.32f, (spec.style == 2) ? carbon : bodyCol);
        glPopMatrix();
    }

    // 5. Signature LED Taillights + Anamorphic Red Lens Flares!
    Color tailCore = g_game.braking ? Color(1.0f, 0.25f, 0.18f) : Color(1.0f, 0.06f, 0.06f);
    float rearZ = -hl * 1.005f;
    float tailY = h * 0.44f;

    if (spec.style == 0) {
        // GT-R Iconic Quad Afterburner Rings
        for (int side = -1; side <= 1; side += 2) {
            for (int ring = 0; ring < 2; ++ring) {
                float rx = side * (w * 0.25f + ring * 0.19f);
                float rad = (ring == 1) ? 0.11f : 0.088f;
                glBegin(GL_TRIANGLE_FAN);
                glColor3f(tailCore.r, tailCore.g, tailCore.b);
                glVertex3f(rx, tailY, rearZ);
                for (int k = 0; k <= 14; ++k) {
                    float a = k * (2.0f * PI / 14.0f);
                    glVertex3f(rx + cosf(a) * rad, tailY + sinf(a) * rad, rearZ);
                }
                glEnd();
            }
        }
    } else if (spec.style == 3) {
        // 911 Full-Width LED Lightbar
        glPushMatrix();
        glTranslatef(0.0f, tailY, rearZ);
        drawBox(w * 0.88f, 0.08f, 0.04f, tailCore, 1.0f, 1.0f);
        glPopMatrix();
    } else {
        for (int side = -1; side <= 1; side += 2) {
            glPushMatrix();
            glTranslatef(side * w * 0.33f, tailY, rearZ);
            drawBox(w * 0.24f, 0.11f, 0.04f, tailCore, 1.0f, 1.0f);
            glPopMatrix();
        }
    }

    // Anamorphic Taillight Lens Flare Billboards
    if (!isReflectionPass) {
        float flareW = g_game.braking ? 1.85f : 1.15f;
        float flareH = g_game.braking ? 0.72f : 0.42f;
        float flareA = g_game.braking ? 0.92f : 0.58f;
        drawGlowBillboard(-w * 0.32f, tailY, rearZ - 0.04f, flareW, flareH, Color(1.0f, 0.10f, 0.08f, flareA));
        drawGlowBillboard( w * 0.32f, tailY, rearZ - 0.04f, flareW, flareH, Color(1.0f, 0.10f, 0.08f, flareA));
    }

    // 6. Quad Exhaust Tips + Volumetric NOS Plasma Cone
    for (int side = -1; side <= 1; side += 2) {
        for (int pipe = 0; pipe < 2; ++pipe) {
            glPushMatrix();
            glTranslatef(side * (w * 0.24f + pipe * 0.11f), 0.19f, -hl * 1.01f);
            drawBox(0.09f, 0.09f, 0.14f, Color(0.72f, 0.75f, 0.82f));
            glPopMatrix();
        }

        if (g_game.nosActive || g_game.backfireTimer > 0.0f) {
            float ex = side * w * 0.28f;
            float ey = 0.19f;
            float ez = -hl * 1.03f;
            float flameLen = g_game.nosActive ? randf(1.1f, 1.7f) : randf(0.6f, 1.0f);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glBegin(GL_TRIANGLES);
            if (g_game.nosActive) glColor4f(0.30f, 0.82f, 1.0f, 0.95f);
            else glColor4f(1.0f, 0.58f, 0.12f, 0.95f);
            glVertex3f(ex - 0.14f, ey, ez);
            glVertex3f(ex + 0.14f, ey, ez);
            glColor4f(0.65f, 0.15f, 1.0f, 0.0f);
            glVertex3f(ex, ey + randf(-0.04f, 0.04f), ez - flameLen);
            glEnd();
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            if (!isReflectionPass) {
                drawGlowBillboard(ex, ey, ez - 0.15f, 1.3f, 0.65f,
                    g_game.nosActive ? Color(0.25f, 0.78f, 1.0f, 0.88f) : Color(1.0f, 0.55f, 0.15f, 0.88f));
            }
        }
    }

    // 7. Xenon / Laser Front Headlights + Volumetric Light Cones on Road
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        glTranslatef(side * w * 0.34f, h * 0.38f, hl * 0.99f);
        drawBox(w * 0.20f, 0.09f, 0.06f, Color(0.92f, 0.97f, 1.0f), 1.0f, 1.0f);
        glPopMatrix();
    }
    if (ATMOSPHERES[g_game.atmosphereIdx].streetLightsOn && !isReflectionPass) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        glBegin(GL_TRIANGLES);
        for (int side = -1; side <= 1; side += 2) {
            float hx = side * w * 0.34f;
            glColor4f(0.75f, 0.90f, 1.0f, 0.20f);
            glVertex3f(hx, 0.25f, hl);
            glColor4f(0.75f, 0.90f, 1.0f, 0.0f);
            glVertex3f(hx - 3.2f, 0.04f, hl + 36.0f);
            glVertex3f(hx + 3.2f, 0.04f, hl + 36.0f);
        }
        glEnd();
        glDepthMask(GL_TRUE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    // 8. 4 Detailed Forged Alloy Wheels + Glowing Carbon-Ceramic Rotors
    float wheelR = 0.35f;
    float wheelW = 0.29f;
    float frontSteerDeg = g_game.steerInput * 24.0f;
    drawWheelDetailed(-w * 0.49f, wheelR,  l * 0.31f, wheelR, wheelW, frontSteerDeg, g_game.wheelRot, g_game.braking);
    drawWheelDetailed( w * 0.49f, wheelR,  l * 0.31f, wheelR, wheelW, frontSteerDeg, g_game.wheelRot, g_game.braking);
    drawWheelDetailed(-w * 0.50f, wheelR, -l * 0.31f, wheelR, wheelW, 0.0f,          g_game.wheelRot, g_game.braking);
    drawWheelDetailed( w * 0.50f, wheelR, -l * 0.31f, wheelR, wheelW, 0.0f,          g_game.wheelRot, g_game.braking);

    glPopMatrix();
}

void drawTrafficCar(const TrafficCar& tc, bool isReflectionPass) {
    float roadX, roadY, tun;
    float dxds, dyds;
    getHighwayPoint(tc.s, roadX, roadY, tun);
    getHighwayTangent(tc.s, dxds, dyds);
    float roadYawDeg = atan2f(dxds, 1.0f) * (180.0f / PI);
    float roadPitchDeg = -atan2f(dyds, 1.0f) * (180.0f / PI);
    bool oncoming = (tc.speed < 0.0f);

    glPushMatrix();
    glTranslatef(roadX + tc.laneOffset, roadY, tc.s);
    glRotatef(roadYawDeg + (oncoming ? 180.0f : 0.0f) + tc.spinYaw * (180.0f / PI), 0.0f, 1.0f, 0.0f);
    glRotatef(oncoming ? -roadPitchDeg : roadPitchDeg, 1.0f, 0.0f, 0.0f);

    float w = tc.width, l = tc.length, h = tc.height;
    float hl = l * 0.5f;
    float alpha = g_game.ghostMode ? 0.48f : 1.0f;
    Color col(tc.color.r, tc.color.g, tc.color.b, alpha);

    if (!isReflectionPass) {
        glBegin(GL_QUADS);
        glColor4f(0.0f, 0.0f, 0.0f, 0.72f * alpha);
        glVertex3f(-w * 0.54f, 0.025f, -hl * 1.04f);
        glVertex3f(-w * 0.54f, 0.025f,  hl * 1.04f);
        glVertex3f( w * 0.54f, 0.025f,  hl * 1.04f);
        glVertex3f( w * 0.54f, 0.025f, -hl * 1.04f);
        glEnd();
    }

    if (tc.type == 3) {
        // Semi Truck Cab + Reflective Trailer
        glPushMatrix();
        glTranslatef(0.0f, h * 0.54f, -l * 0.12f);
        drawBox(w, h * 0.78f, l * 0.74f, Color(0.82f, 0.85f, 0.90f, alpha));
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0.0f, h * 0.42f, l * 0.36f);
        drawBox(w * 0.92f, h * 0.62f, l * 0.24f, col);
        glPopMatrix();
    } else {
        // Sculpted Body + Clearcoat Reflection Pass
        drawSculptedSection(-hl, hl * 0.30f, w, w * 0.92f, 0.18f, h * 0.56f, w, w * 0.90f, 0.18f, h * 0.54f, col);
        drawSculptedSection(hl * 0.30f, hl, w, w * 0.90f, 0.18f, h * 0.54f, w * 0.90f, w * 0.78f, 0.18f, h * 0.38f, col);
        Color cabCol = (tc.type == 4) ? Color(0.92f, 0.95f, 1.0f, alpha) : Color(0.08f, 0.11f, 0.16f, alpha);
        drawSculptedSection(-hl * 0.55f, hl * 0.28f, w * 0.86f, w * 0.68f, h * 0.54f, h * 0.96f, w * 0.84f, w * 0.66f, h * 0.54f, h * 0.94f, cabCol);

        // Police Lightbar + Anamorphic Red/Blue Siren Flares!
        if (tc.type == 4) {
            bool redPhase = (sinf(g_game.sirenPhase + tc.s * 0.1f) > 0.0f);
            for (int side = -1; side <= 1; side += 2) {
                bool isRed = (side < 0) ? redPhase : !redPhase;
                Color sCol = isRed ? Color(1.0f, 0.08f, 0.08f, 0.95f) : Color(0.08f, 0.40f, 1.0f, 0.95f);
                glPushMatrix();
                glTranslatef(side * w * 0.22f, h * 1.02f, -0.15f);
                drawBox(0.32f, 0.12f, 0.24f, sCol, 1.0f, 1.0f);
                glPopMatrix();
                if (!isReflectionPass) {
                    drawGlowBillboard(side * w * 0.22f, h * 1.04f, -0.15f, 1.8f, 0.8f, sCol);
                }
            }
        }
    }

    // Headlights & Taillights with Anamorphic Flares
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        glTranslatef(side * w * 0.35f, 0.48f, hl * 1.005f);
        drawBox(0.26f, 0.12f, 0.04f, Color(1.0f, 0.98f, 0.90f, alpha), 1.0f, 1.0f);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(side * w * 0.35f, 0.52f, -hl * 1.005f);
        drawBox(0.30f, 0.12f, 0.04f, Color(1.0f, 0.08f, 0.08f, alpha), 1.0f, 1.0f);
        glPopMatrix();

        if (!isReflectionPass) {
            if (oncoming) {
                // Bright oncoming Xenon headlight glare!
                drawGlowBillboard(side * w * 0.35f, 0.48f, hl * 1.02f, 1.65f, 0.75f, Color(0.90f, 0.96f, 1.0f, 0.85f * alpha));
            } else {
                // Glowing red LED taillight flare ahead
                drawGlowBillboard(side * w * 0.35f, 0.52f, -hl * 1.02f, 1.15f, 0.48f, Color(1.0f, 0.08f, 0.06f, 0.72f * alpha));
            }
        }

        if (tc.turnSignal == side && (int(g_game.worldTime * 6.0f) % 2 == 0)) {
            drawGlowBillboard(side * w * 0.44f, 0.52f, -hl * 1.03f, 0.85f, 0.45f, Color(1.0f, 0.65f, 0.0f, 0.9f * alpha));
        }
    }

    glPopMatrix();
}

// ============================================================================
// SKY, HORIZON, REAL-TIME PLANAR REFLECTIONS & HIGHWAY ENVIRONMENT
// ============================================================================
void drawSkyAndHorizon(int width, int height) {
    const Atmosphere& atm = ATMOSPHERES[g_game.atmosphereIdx];

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_TEXTURE_2D);

    float flash = g_game.lightningFlash * 0.45f;
    Color top(atm.skyTop.r + flash, atm.skyTop.g + flash, atm.skyTop.b + flash * 1.2f);
    Color mid(atm.skyMid.r + flash, atm.skyMid.g + flash, atm.skyMid.b + flash);
    Color hor(atm.skyHorizon.r + flash, atm.skyHorizon.g + flash, atm.skyHorizon.b + flash);

    float horizonY = height * 0.52f;
    float midY = horizonY * 0.55f;

    // 3-Stop Rich Atmospheric Sky Gradient
    glBegin(GL_QUADS);
    glColor3f(top.r, top.g, top.b);
    glVertex2f(0.0f, 0.0f);
    glVertex2f((float)width, 0.0f);
    glColor3f(mid.r, mid.g, mid.b);
    glVertex2f((float)width, midY);
    glVertex2f(0.0f, midY);

    glColor3f(mid.r, mid.g, mid.b);
    glVertex2f(0.0f, midY);
    glVertex2f((float)width, midY);
    glColor3f(hor.r, hor.g, hor.b);
    glVertex2f((float)width, horizonY);
    glVertex2f(0.0f, horizonY);

    // Lower Ground Backdrop
    glColor3f(atm.fogColor.r, atm.fogColor.g, atm.fogColor.b);
    glVertex2f(0.0f, horizonY);
    glVertex2f((float)width, horizonY);
    glColor3f(atm.terrainCol.r, atm.terrainCol.g, atm.terrainCol.b);
    glVertex2f((float)width, (float)height);
    glVertex2f(0.0f, (float)height);
    glEnd();

    // Stars
    if (atm.streetLightsOn && !atm.raining) {
        glPointSize(2.0f);
        glBegin(GL_POINTS);
        for (int i = 0; i < 95; ++i) {
            float sx = fmodf(i * 173.3f, (float)width);
            float sy = fmodf(i * 97.7f, horizonY * 0.78f);
            float twinkle = 0.35f + 0.55f * sinf(g_game.worldTime * 3.0f + i);
            glColor4f(0.90f, 0.95f, 1.0f, twinkle);
            glVertex2f(sx, sy);
        }
        glEnd();
    }

    // Sun / Moon with Anamorphic Corona Glow
    float dxds, dyds;
    getHighwayTangent(g_game.s, dxds, dyds);
    float sunX = width * 0.5f - dxds * 220.0f;
    float sunY = horizonY - (atm.sunElevation / 45.0f) * (horizonY * 0.55f);
    float sunR = (g_game.atmosphereIdx == 1 || g_game.atmosphereIdx == 3) ? height * 0.14f : height * 0.06f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(atm.sunColor.r, atm.sunColor.g, atm.sunColor.b, 0.48f);
    glVertex2f(sunX, sunY);
    glColor4f(atm.sunColor.r, atm.sunColor.g, atm.sunColor.b, 0.0f);
    for (int i = 0; i <= 32; ++i) {
        float a = i * (2.0f * PI / 32.0f);
        glVertex2f(sunX + cosf(a) * sunR * 2.8f, sunY + sinf(a) * sunR * 2.2f);
    }
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(1.0f, 0.98f, 0.90f, 0.96f);
    glVertex2f(sunX, sunY);
    glColor4f(atm.sunColor.r, atm.sunColor.g, atm.sunColor.b, 0.92f);
    for (int i = 0; i <= 32; ++i) {
        float a = i * (2.0f * PI / 32.0f);
        glVertex2f(sunX + cosf(a) * sunR, sunY + sinf(a) * sunR);
    }
    glEnd();

    // Distant Layered Mountain Range Silhouette
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 64; ++i) {
        float x = (float(i) / 64.0f) * width;
        float worldAng = x * 0.011f - dxds * 1.4f;
        float mtnH = 22.0f + 26.0f * sinf(worldAng) + 14.0f * cosf(worldAng * 2.7f) + 6.0f * sinf(worldAng * 6.1f);
        glColor4f(atm.fogColor.r * 0.38f, atm.fogColor.g * 0.40f, atm.fogColor.b * 0.50f, 0.96f);
        glVertex2f(x, horizonY - mtnH);
        glVertex2f(x, horizonY + 2.0f);
    }
    glEnd();

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// Render Buildings, Streetlamps & Overhead Gantries (called both in normal pass and mirrored reflection pass!)
void drawEnvironmentStructures(bool isReflectionPass) {
    const Atmosphere& atm = ATMOSPHERES[g_game.atmosphereIdx];
    int startSeg = int(floorf(g_game.s / SEGMENT_LEN)) - VISIBLE_SEGMENTS_BEHIND;
    int endSeg   = startSeg + VISIBLE_SEGMENTS_BEHIND + VISIBLE_SEGMENTS_AHEAD;

    // 1. Textured Glass Skyscrapers with Lit Office Floors
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_texBuilding);
    for (size_t i = 0; i < g_game.buildings.size(); ++i) {
        const Building& b = g_game.buildings[i];
        if (b.s < g_game.s - 60.0f || b.s > g_game.s + VISIBLE_SEGMENTS_AHEAD * SEGMENT_LEN) continue;
        float rx, ry, tun;
        getHighwayPoint(b.s, rx, ry, tun);
        if (tun > 0.5f) continue;

        float bx = rx + b.sideOffset;
        float by0 = ry - 2.0f;
        float by1 = by0 + b.height;
        float hw = b.width * 0.5f;
        float hd = b.depth * 0.5f;
        float uRep = floorf(b.width / 8.0f);
        float vRep = floorf(b.height / 12.0f);

        glColor3f(b.tint.r, b.tint.g, b.tint.b);
        glBegin(GL_QUADS);
        // Front (-Z facing camera)
        glTexCoord2f(0.0f, 0.0f); glVertex3f(bx - hw, by0, b.s - hd);
        glTexCoord2f(uRep, 0.0f); glVertex3f(bx + hw, by0, b.s - hd);
        glTexCoord2f(uRep, vRep); glVertex3f(bx + hw, by1, b.s - hd);
        glTexCoord2f(0.0f, vRep); glVertex3f(bx - hw, by1, b.s - hd);
        // Side facing highway
        float sideX = (b.sideOffset < 0.0f) ? (bx + hw) : (bx - hw);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(sideX, by0, b.s - hd);
        glTexCoord2f(uRep, 0.0f); glVertex3f(sideX, by0, b.s + hd);
        glTexCoord2f(uRep, vRep); glVertex3f(sideX, by1, b.s + hd);
        glTexCoord2f(0.0f, vRep); glVertex3f(sideX, by1, b.s - hd);
        glEnd();
    }
    glDisable(GL_TEXTURE_2D);

    // 2. Streetlamps, Tunnels & Highway Gantries
    for (int seg = startSeg; seg < endSeg; ++seg) {
        float s0 = seg * SEGMENT_LEN;
        float s1 = (seg + 1) * SEGMENT_LEN;
        float x0, y0, tun0;
        float x1, y1, tun1;
        getHighwayPoint(s0, x0, y0, tun0);
        getHighwayPoint(s1, x1, y1, tun1);
        bool alt = (seg % 2 == 0);
        float bw = HIGHWAY_HALF_WIDTH + 0.85f;
        float bh = 1.05f;

        if (tun0 > 0.5f) {
            float archH = 9.5f;
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, g_texConcrete);
            glColor3f(0.45f, 0.48f, 0.54f);
            glBegin(GL_QUADS);
            // Left tunnel wall
            glTexCoord2f(0, 0); glVertex3f(x0 - bw, y0 + bh,    s0);
            glTexCoord2f(1, 0); glVertex3f(x1 - bw, y1 + bh,    s1);
            glTexCoord2f(1, 2); glVertex3f(x1 - bw, y1 + archH, s1);
            glTexCoord2f(0, 2); glVertex3f(x0 - bw, y0 + archH, s0);
            // Right tunnel wall
            glTexCoord2f(0, 0); glVertex3f(x0 + bw, y0 + bh,    s0);
            glTexCoord2f(1, 0); glVertex3f(x1 + bw, y1 + bh,    s1);
            glTexCoord2f(1, 2); glVertex3f(x1 + bw, y1 + archH, s1);
            glTexCoord2f(0, 2); glVertex3f(x0 + bw, y0 + archH, s0);
            // Ceiling
            glColor3f(0.28f, 0.30f, 0.35f);
            glTexCoord2f(0, 0); glVertex3f(x0 - bw, y0 + archH, s0);
            glTexCoord2f(1, 0); glVertex3f(x1 - bw, y1 + archH, s1);
            glTexCoord2f(1, 4); glVertex3f(x1 + bw, y1 + archH, s1);
            glTexCoord2f(0, 4); glVertex3f(x0 + bw, y0 + archH, s0);
            glEnd();
            glDisable(GL_TEXTURE_2D);

            if (alt) {
                for (int side = -1; side <= 1; side += 2) {
                    float lx = x0 + side * 6.5f;
                    drawGlowBillboard(lx, y0 + archH - 0.25f, s0, 3.2f, 1.4f, Color(1.0f, 0.78f, 0.32f, 0.9f));
                }
            }
        } else if (seg % 4 == 0) {
            float poleH = 8.8f;
            float armLen = 3.4f;
            for (int side = -1; side <= 1; side += 2) {
                float px = x0 + side * (HIGHWAY_HALF_WIDTH + 1.1f);
                glPushMatrix();
                glTranslatef(px, y0 + poleH * 0.5f, s0);
                drawBox(0.20f, poleH, 0.20f, Color(0.28f, 0.30f, 0.35f));
                glPopMatrix();

                glPushMatrix();
                glTranslatef(px - side * armLen * 0.5f, y0 + poleH, s0);
                drawBox(armLen, 0.14f, 0.16f, Color(0.32f, 0.34f, 0.38f));
                glPopMatrix();

                if (atm.streetLightsOn) {
                    Color lampCol = atm.synthGrid ? Color(0.0f, 0.90f, 1.0f, 0.92f) : Color(1.0f, 0.82f, 0.45f, 0.92f);
                    float lx = px - side * armLen;
                    drawGlowBillboard(lx, y0 + poleH - 0.18f, s0, 3.4f, 1.6f, lampCol);

                    // Volumetric Light Cone under streetlamp!
                    if (!isReflectionPass) {
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                        glDepthMask(GL_FALSE);
                        glBegin(GL_TRIANGLES);
                        glColor4f(lampCol.r, lampCol.g, lampCol.b, 0.085f);
                        glVertex3f(lx, y0 + poleH - 0.2f, s0);
                        glColor4f(lampCol.r, lampCol.g, lampCol.b, 0.0f);
                        glVertex3f(lx - 4.8f, y0 + 0.05f, s0);
                        glVertex3f(lx + 4.8f, y0 + 0.05f, s0);
                        glEnd();
                        glDepthMask(GL_TRUE);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    }
                }
            }
        }

        if (tun0 < 0.5f && (seg % 32 == 0) && !isReflectionPass) {
            float gH = 8.2f;
            float gW = (HIGHWAY_HALF_WIDTH + 1.5f) * 2.0f;
            glPushMatrix();
            glTranslatef(x0, y0 + gH, s0);
            drawBox(gW, 0.32f, 0.32f, Color(0.35f, 0.38f, 0.42f));
            for (int b = -1; b <= 1; b += 2) {
                glPushMatrix();
                glTranslatef(b * 5.2f, -0.35f, 0.22f);
                drawBox(6.8f, 2.1f, 0.12f, Color(0.04f, 0.36f, 0.18f));
                glPopMatrix();
            }
            glPopMatrix();
        }
    }
}

void drawHighwaySurfaceAndBarriers() {
    const Atmosphere& atm = ATMOSPHERES[g_game.atmosphereIdx];
    int startSeg = int(floorf(g_game.s / SEGMENT_LEN)) - VISIBLE_SEGMENTS_BEHIND;
    int endSeg   = startSeg + VISIBLE_SEGMENTS_BEHIND + VISIBLE_SEGMENTS_AHEAD;

    // 1. Outer Terrain Floor
    for (int seg = startSeg; seg < endSeg; ++seg) {
        float s0 = seg * SEGMENT_LEN;
        float s1 = (seg + 1) * SEGMENT_LEN;
        float x0, y0, tun0, x1, y1, tun1;
        getHighwayPoint(s0, x0, y0, tun0);
        getHighwayPoint(s1, x1, y1, tun1);

        float terrainW = 200.0f;
        glBegin(GL_QUADS);
        glColor3f(atm.terrainCol.r, atm.terrainCol.g, atm.terrainCol.b);
        glVertex3f(x0 - terrainW, y0 - 0.18f, s0);
        glVertex3f(x1 - terrainW, y1 - 0.18f, s1);
        glVertex3f(x1 - HIGHWAY_HALF_WIDTH, y1 - 0.05f, s1);
        glVertex3f(x0 - HIGHWAY_HALF_WIDTH, y0 - 0.05f, s0);

        glVertex3f(x0 + HIGHWAY_HALF_WIDTH, y0 - 0.05f, s0);
        glVertex3f(x1 + HIGHWAY_HALF_WIDTH, y1 - 0.05f, s1);
        glVertex3f(x1 + terrainW, y1 - 0.18f, s1);
        glVertex3f(x0 + terrainW, y0 - 0.18f, s0);
        glEnd();
    }

    // 2. High-Resolution Mipmapped Asphalt Road Surface (Blended over Real-Time Planar Reflections!)
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_texAsphalt);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float roadAlpha = 1.0f - atm.reflectionStrength;
    glColor4f(atm.roadTint.r * 1.45f, atm.roadTint.g * 1.45f, atm.roadTint.b * 1.45f, roadAlpha);

    glBegin(GL_QUADS);
    for (int seg = startSeg; seg < endSeg; ++seg) {
        float s0 = seg * SEGMENT_LEN;
        float s1 = (seg + 1) * SEGMENT_LEN;
        float x0, y0, tun0, x1, y1, tun1;
        getHighwayPoint(s0, x0, y0, tun0);
        getHighwayPoint(s1, x1, y1, tun1);

        float v0 = float(seg);
        float v1 = float(seg + 1);
        glTexCoord2f(0.0f, v0); glVertex3f(x0 - HIGHWAY_HALF_WIDTH, y0, s0);
        glTexCoord2f(0.0f, v1); glVertex3f(x1 - HIGHWAY_HALF_WIDTH, y1, s1);
        glTexCoord2f(1.0f, v1); glVertex3f(x1 + HIGHWAY_HALF_WIDTH, y1, s1);
        glTexCoord2f(1.0f, v0); glVertex3f(x0 + HIGHWAY_HALF_WIDTH, y0, s0);
    }
    glEnd();

    // 3. Textured Concrete Jersey Barriers
    glBindTexture(GL_TEXTURE_2D, g_texConcrete);
    glColor4f(0.78f, 0.80f, 0.85f, 1.0f);
    glBegin(GL_QUADS);
    for (int seg = startSeg; seg < endSeg; ++seg) {
        float s0 = seg * SEGMENT_LEN;
        float s1 = (seg + 1) * SEGMENT_LEN;
        float x0, y0, tun0, x1, y1, tun1;
        getHighwayPoint(s0, x0, y0, tun0);
        getHighwayPoint(s1, x1, y1, tun1);
        float bw = HIGHWAY_HALF_WIDTH + 0.85f;
        float bh = 1.08f;

        glTexCoord2f(0, 0); glVertex3f(x0 - bw, y0,      s0);
        glTexCoord2f(2, 0); glVertex3f(x1 - bw, y1,      s1);
        glTexCoord2f(2, 1); glVertex3f(x1 - bw, y1 + bh, s1);
        glTexCoord2f(0, 1); glVertex3f(x0 - bw, y0 + bh, s0);

        glTexCoord2f(0, 0); glVertex3f(x0 + bw, y0,      s0);
        glTexCoord2f(2, 0); glVertex3f(x1 + bw, y1,      s1);
        glTexCoord2f(2, 1); glVertex3f(x1 + bw, y1 + bh, s1);
        glTexCoord2f(0, 1); glVertex3f(x0 + bw, y0 + bh, s0);
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);

    // 4. Reflective Painted Highway Lane Lines, Double-Yellow Divider & Cat's-Eye Studs
    glBegin(GL_QUADS);
    for (int seg = startSeg; seg < endSeg; ++seg) {
        float s0 = seg * SEGMENT_LEN;
        float s1 = (seg + 1) * SEGMENT_LEN;
        float x0, y0, tun0, x1, y1, tun1;
        getHighwayPoint(s0, x0, y0, tun0);
        getHighwayPoint(s1, x1, y1, tun1);
        bool alt = (seg % 2 == 0);

        // Rumble curbs
        Color shoulderCol = alt ? Color(0.82f, 0.16f, 0.14f) : Color(0.90f, 0.92f, 0.95f);
        float rw = 0.85f;
        glColor3f(shoulderCol.r, shoulderCol.g, shoulderCol.b);
        glVertex3f(x0 - HIGHWAY_HALF_WIDTH - rw, y0 + 0.02f, s0);
        glVertex3f(x1 - HIGHWAY_HALF_WIDTH - rw, y1 + 0.02f, s1);
        glVertex3f(x1 - HIGHWAY_HALF_WIDTH,      y1 + 0.02f, s1);
        glVertex3f(x0 - HIGHWAY_HALF_WIDTH,      y0 + 0.02f, s0);

        glVertex3f(x0 + HIGHWAY_HALF_WIDTH,      y0 + 0.02f, s0);
        glVertex3f(x1 + HIGHWAY_HALF_WIDTH,      y1 + 0.02f, s1);
        glVertex3f(x1 + HIGHWAY_HALF_WIDTH + rw, y1 + 0.02f, s1);
        glVertex3f(x0 + HIGHWAY_HALF_WIDTH + rw, y0 + 0.02f, s0);

        // Double Yellow Oncoming Divider at x = -4.2m
        float divX = -4.2f;
        glColor3f(0.98f, 0.74f, 0.06f);
        for (int line = -1; line <= 1; line += 2) {
            float lx = divX + line * 0.15f;
            glVertex3f(x0 + lx - 0.06f, y0 + 0.025f, s0);
            glVertex3f(x1 + lx - 0.06f, y1 + 0.025f, s1);
            glVertex3f(x1 + lx + 0.06f, y1 + 0.025f, s1);
            glVertex3f(x0 + lx + 0.06f, y0 + 0.025f, s0);
        }

        // Dashed White Lane Markers
        if (alt) {
            static const float laneDividers[4] = { -8.4f, 0.0f, 4.2f, 8.4f };
            glColor3f(0.92f, 0.94f, 0.98f);
            for (int d = 0; d < 4; ++d) {
                float lx = laneDividers[d];
                glVertex3f(x0 + lx - 0.07f, y0 + 0.025f, s0);
                glVertex3f(x1 + lx - 0.07f, y1 + 0.025f, s1);
                glVertex3f(x1 + lx + 0.07f, y1 + 0.025f, s1);
                glVertex3f(x0 + lx + 0.07f, y0 + 0.025f, s0);
            }
        }
    }
    glEnd();

    // 5. Tire Skidmarks
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    for (size_t i = 0; i < g_game.skidMarks.size(); ++i) {
        const SkidMark& sm = g_game.skidMarks[i];
        float rx0, ry0, rx1, ry1, t;
        getHighwayPoint(sm.s0, rx0, ry0, t);
        getHighwayPoint(sm.s1, rx1, ry1, t);
        glColor4f(0.03f, 0.03f, 0.04f, sm.alpha);
        glVertex3f(rx0 + sm.x0 - 0.75f, ry0 + 0.035f, sm.s0);
        glVertex3f(rx1 + sm.x1 - 0.75f, ry1 + 0.035f, sm.s1);
        glVertex3f(rx0 + sm.x0 + 0.75f, ry0 + 0.035f, sm.s0);
        glVertex3f(rx1 + sm.x1 + 0.75f, ry1 + 0.035f, sm.s1);
    }
    glEnd();
}

void drawParticlesAndWeather() {
    for (size_t i = 0; i < g_game.particles.size(); ++i) {
        const Particle& p = g_game.particles[i];
        float alpha = clampf(p.life / p.maxLife, 0.0f, 1.0f) * p.color.a;
        if (p.type == 1 || p.type == 2) {
            drawGlowBillboard(p.pos.x, p.pos.y, p.pos.z, p.size * 2.2f, p.size * 2.2f, Color(p.color.r, p.color.g, p.color.b, alpha));
        } else {
            drawGlowBillboard(p.pos.x, p.pos.y, p.pos.z, p.size * 1.8f, p.size * 1.2f, Color(p.color.r, p.color.g, p.color.b, alpha * 0.55f));
        }
    }

    const Atmosphere& atm = ATMOSPHERES[g_game.atmosphereIdx];
    float roadX, roadY, tun;
    getHighwayPoint(g_game.s, roadX, roadY, tun);

    if (atm.raining && tun < 0.5f) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glLineWidth(1.5f);
        glBegin(GL_LINES);
        glColor4f(0.72f, 0.82f, 0.96f, 0.42f);
        for (int i = 0; i < 160; ++i) {
            float rx = roadX + g_game.lateralX + sinf(i * 19.3f + g_game.worldTime * 8.0f) * 18.0f;
            float ry = roadY + fmodf(i * 3.7f - g_game.worldTime * 28.0f + 400.0f, 14.0f);
            float rz = g_game.s + fmodf(i * 7.1f, 55.0f) - 8.0f;
            glVertex3f(rx, ry, rz);
            glVertex3f(rx, ry - 1.4f, rz - g_game.speed * 0.02f);
        }
        glEnd();
    }
}

// ============================================================================
// 2D HUD & TELEMETRY RENDERING
// ============================================================================
static GLuint g_fontSmall = 0;
static GLuint g_fontMedium = 0;
static GLuint g_fontLarge = 0;

void initFonts(HDC hdc) {
    g_fontSmall = glGenLists(96);
    HFONT hFontS = CreateFontA(-15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
        OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Consolas");
    HFONT hOld = (HFONT)SelectObject(hdc, hFontS);
    wglUseFontBitmapsA(hdc, 32, 96, g_fontSmall);
    SelectObject(hdc, hOld);
    DeleteObject(hFontS);

    g_fontMedium = glGenLists(96);
    HFONT hFontM = CreateFontA(-22, 0, 0, 0, FW_BOLD, TRUE, FALSE, FALSE, ANSI_CHARSET,
        OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Segoe UI");
    SelectObject(hdc, hFontM);
    wglUseFontBitmapsA(hdc, 32, 96, g_fontMedium);
    SelectObject(hdc, hOld);
    DeleteObject(hFontM);

    g_fontLarge = glGenLists(96);
    HFONT hFontL = CreateFontA(-44, 0, 0, 0, FW_HEAVY, TRUE, FALSE, FALSE, ANSI_CHARSET,
        OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Segoe UI");
    SelectObject(hdc, hFontL);
    wglUseFontBitmapsA(hdc, 32, 96, g_fontLarge);
    SelectObject(hdc, hOld);
    DeleteObject(hFontL);
}

void drawText(float x, float y, const char* text, GLuint fontBase, const Color& col, bool shadow = true) {
    if (!text || !*text) return;
    if (shadow) {
        glColor4f(0.0f, 0.0f, 0.0f, col.a * 0.85f);
        glRasterPos2f(x + 2.0f, y + 2.0f);
        glPushAttrib(GL_LIST_BIT);
        glListBase(fontBase - 32);
        glCallLists((GLsizei)strlen(text), GL_UNSIGNED_BYTE, text);
        glPopAttrib();
    }
    glColor4f(col.r, col.g, col.b, col.a);
    glRasterPos2f(x, y);
    glPushAttrib(GL_LIST_BIT);
    glListBase(fontBase - 32);
    glCallLists((GLsizei)strlen(text), GL_UNSIGNED_BYTE, text);
    glPopAttrib();
}

void drawRect2D(float x, float y, float w, float h, const Color& col) {
    glColor4f(col.r, col.g, col.b, col.a);
    glBegin(GL_QUADS);
    glVertex2f(x,     y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x,     y + h);
    glEnd();
}

void drawHUD(int width, int height) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Subtle Cinematic Vignette at Screen Corners
    glBegin(GL_QUADS);
    glColor4f(0.0f, 0.0f, 0.0f, 0.42f);
    glVertex2f(0.0f, 0.0f); glVertex2f((float)width, 0.0f);
    glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
    glVertex2f((float)width, 90.0f); glVertex2f(0.0f, 90.0f);

    glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
    glVertex2f(0.0f, height - 90.0f); glVertex2f((float)width, height - 90.0f);
    glColor4f(0.0f, 0.0f, 0.0f, 0.48f);
    glVertex2f((float)width, (float)height); glVertex2f(0.0f, (float)height);
    glEnd();

    if (g_game.minimalHud && !g_game.paused) {
        glEnable(GL_DEPTH_TEST);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        return;
    }

    const CarSpec& spec = CAR_SPECS[g_game.carIndex];
    char buf[128];

    // 1. BOTTOM-RIGHT NFS TACHOMETER, TURBO BOOST, GEAR, SPEED & NITROUS GAUGE
    float cx = width - 160.0f;
    float cy = height - 140.0f;
    float rad = 105.0f;

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(0.03f, 0.05f, 0.09f, 0.85f);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 40; ++i) {
        float a = i * (2.0f * PI / 40.0f);
        glVertex2f(cx + cosf(a) * (rad + 16.0f), cy + sinf(a) * (rad + 16.0f));
    }
    glEnd();

    float startAng = 135.0f * (PI / 180.0f);
    float totalArc = 260.0f * (PI / 180.0f);
    float rpmFrac = clampf(g_game.rpm / 8500.0f, 0.0f, 1.0f);

    // Background RPM Track
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 36; ++i) {
        float f = float(i) / 36.0f;
        float a = startAng + f * totalArc;
        Color c = (f > 0.82f) ? Color(0.45f, 0.08f, 0.12f, 0.7f) : Color(0.14f, 0.20f, 0.28f, 0.7f);
        glColor4f(c.r, c.g, c.b, c.a);
        glVertex2f(cx + cosf(a) * (rad - 15.0f), cy + sinf(a) * (rad - 15.0f));
        glVertex2f(cx + cosf(a) * rad,           cy + sinf(a) * rad);
    }
    glEnd();

    // Active Glowing RPM Arc
    glBegin(GL_QUAD_STRIP);
    int activeSteps = int(rpmFrac * 36.0f);
    for (int i = 0; i <= activeSteps; ++i) {
        float f = float(i) / 36.0f;
        float a = startAng + f * totalArc;
        Color c = (f > 0.82f)
            ? Color(1.0f, 0.15f, 0.22f, 0.96f)
            : (g_game.nosActive ? Color(0.20f, 0.88f, 1.0f, 0.96f) : Color(0.08f, 0.68f, 1.0f, 0.95f));
        glColor4f(c.r, c.g, c.b, c.a);
        glVertex2f(cx + cosf(a) * (rad - 15.0f), cy + sinf(a) * (rad - 15.0f));
        glVertex2f(cx + cosf(a) * rad,           cy + sinf(a) * rad);
    }
    glEnd();

    // Analog Tick Marks around Tachometer
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    for (int t = 0; t <= 9; ++t) {
        float f = float(t) / 9.0f;
        float a = startAng + f * totalArc;
        glColor4f(0.85f, 0.90f, 1.0f, 0.85f);
        glVertex2f(cx + cosf(a) * (rad - 20.0f), cy + sinf(a) * (rad - 20.0f));
        glVertex2f(cx + cosf(a) * (rad + 2.0f),  cy + sinf(a) * (rad + 2.0f));
    }
    glEnd();

    // Tachometer Needle
    float needleAng = startAng + rpmFrac * totalArc;
    glLineWidth(3.5f);
    glBegin(GL_LINES);
    glColor4f(1.0f, 0.28f, 0.18f, 1.0f);
    glVertex2f(cx, cy);
    glVertex2f(cx + cosf(needleAng) * (rad - 4.0f), cy + sinf(needleAng) * (rad - 4.0f));
    glEnd();

    // Digital Speed Readout
    float displaySpeed = fabsf(g_game.speed) * (g_game.useKmh ? 3.6f : 2.23694f);
    snprintf(buf, sizeof(buf), "%03d", int(displaySpeed + 0.5f));
    drawText(cx - 42.0f, cy + 8.0f, buf, g_fontLarge,
        g_game.nosActive ? Color(0.35f, 0.95f, 1.0f) : Color(0.98f, 0.98f, 1.0f));
    drawText(cx - 38.0f, cy + 28.0f, g_game.useKmh ? "KM/H" : "MPH", g_fontSmall, Color(0.65f, 0.75f, 0.88f));
    snprintf(buf, sizeof(buf), "%+.0f PSI", g_game.turboPsi);
    drawText(cx + 8.0f, cy + 28.0f, buf, g_fontSmall, Color(0.35f, 0.88f, 1.0f));

    // Gear Indicator
    const char* gearStr = (g_game.gear == -1) ? "R" : (g_game.gear == 0 ? "N" : "");
    char gBuf[8];
    if (g_game.gear > 0) snprintf(gBuf, sizeof(gBuf), "%d", g_game.gear);
    else snprintf(gBuf, sizeof(gBuf), "%s", gearStr);
    drawRect2D(cx - 18.0f, cy - 60.0f, 36.0f, 30.0f, Color(0.10f, 0.15f, 0.24f, 0.9f));
    drawText(cx - 7.0f, cy - 37.0f, gBuf, g_fontMedium,
        (rpmFrac > 0.85f) ? Color(1.0f, 0.25f, 0.25f) : Color(1.0f, 0.82f, 0.15f));

    // Nitrous (NOS) Bar
    float nosX = cx - 95.0f;
    float nosY = cy + 72.0f;
    float nosW = 190.0f;
    float nosH = 14.0f;
    drawRect2D(nosX - 2.0f, nosY - 2.0f, nosW + 4.0f, nosH + 4.0f, Color(0.05f, 0.08f, 0.14f, 0.9f));
    Color nosCol = g_game.nosActive
        ? Color(0.35f, 0.95f, 1.0f, 0.98f)
        : Color(0.08f, 0.58f, 0.98f, 0.92f);
    drawRect2D(nosX, nosY, nosW * g_game.nos, nosH, nosCol);
    drawText(nosX, nosY + 29.0f, g_game.nosActive ? ">>> N2O BOOST ACTIVE <<<" : "N2O NITROUS [SPACE]",
        g_fontSmall, g_game.nosActive ? Color(0.35f, 0.98f, 1.0f) : Color(0.55f, 0.78f, 0.95f));

    // 2. TOP-RIGHT FLOW SCORE & TELEMETRY
    float rx = width - 295.0f;
    drawRect2D(rx - 14.0f, 18.0f, 290.0f, 112.0f, Color(0.03f, 0.05f, 0.10f, 0.78f));
    drawRect2D(rx - 14.0f, 18.0f, 4.0f, 112.0f, Color(0.15f, 0.80f, 1.0f, 0.95f));

    snprintf(buf, sizeof(buf), "FLOW PTS : %lld", g_game.flowScore);
    drawText(rx, 42.0f, buf, g_fontMedium, Color(0.95f, 0.98f, 1.0f));

    float distVal = float(g_game.totalDistanceM / 1000.0) * (g_game.useKmh ? 1.0f : 0.621371f);
    float topVal  = g_game.topSpeedMps * (g_game.useKmh ? 3.6f : 2.23694f);
    snprintf(buf, sizeof(buf), "CRUISED  : %.2f %s", distVal, g_game.useKmh ? "KM" : "MI");
    drawText(rx, 68.0f, buf, g_fontSmall, Color(0.72f, 0.82f, 0.94f));

    snprintf(buf, sizeof(buf), "TOP SPD  : %d %s", int(topVal + 0.5f), g_game.useKmh ? "KM/H" : "MPH");
    drawText(rx, 90.0f, buf, g_fontSmall, Color(0.72f, 0.82f, 0.94f));

    const char* densityNames[4] = { "LIGHT", "NORMAL", "HEAVY NO-HESI", "EMPTY ZEN" };
    snprintf(buf, sizeof(buf), "TRAFFIC  : %s%s", densityNames[g_game.trafficDensity],
        g_game.ghostMode ? " [GHOST]" : (g_game.policeHeat ? " [HEAT!]" : ""));
    drawText(rx, 112.0f, buf, g_fontSmall,
        g_game.policeHeat ? Color(1.0f, 0.35f, 0.35f) : Color(0.45f, 0.92f, 0.72f));

    if (g_game.nearMissStreak > 0 || g_game.drifting) {
        drawRect2D(rx - 14.0f, 138.0f, 290.0f, 44.0f, Color(0.12f, 0.04f, 0.20f, 0.85f));
        if (g_game.drifting) {
            snprintf(buf, sizeof(buf), "DRIFT ANGLE %d DEG!  x%d", int(g_game.driftAngleDeg), std::max(1, g_game.nearMissStreak));
            drawText(rx, 166.0f, buf, g_fontMedium, Color(1.0f, 0.55f, 0.15f));
        } else {
            snprintf(buf, sizeof(buf), "NEAR MISS STREAK x%d", g_game.nearMissStreak);
            drawText(rx, 166.0f, buf, g_fontMedium, Color(0.25f, 0.95f, 1.0f));
        }
    }

    if (g_game.popupTimer > 0.0f && !g_game.popupText.empty()) {
        float a = clampf(g_game.popupTimer * 1.5f, 0.0f, 1.0f);
        float px = width * 0.5f - (float)g_game.popupText.length() * 6.2f;
        drawText(px, height * 0.20f, g_game.popupText.c_str(), g_fontMedium,
            Color(g_game.popupColor.r, g_game.popupColor.g, g_game.popupColor.b, a));
    }

    // 3. TOP-LEFT CAR BADGE & CONTROLS
    drawRect2D(18.0f, 18.0f, 365.0f, 64.0f, Color(0.03f, 0.05f, 0.10f, 0.80f));
    drawRect2D(18.0f, 18.0f, 4.0f, 64.0f, Color(1.0f, 0.65f, 0.10f, 0.95f));
    drawText(32.0f, 44.0f, spec.name, g_fontMedium, Color(1.0f, 0.85f, 0.25f));
    snprintf(buf, sizeof(buf), "%s  |  %s", spec.subtitle, ATMOSPHERES[g_game.atmosphereIdx].name);
    drawText(32.0f, 68.0f, buf, g_fontSmall, Color(0.75f, 0.84f, 0.95f));

    if (g_game.showHelp) {
        drawRect2D(18.0f, 92.0f, 365.0f, 245.0f, Color(0.03f, 0.05f, 0.10f, 0.74f));
        float hy = 114.0f;
        float step = 20.0f;
        drawText(30.0f, hy, "W/S or Up/Down   : Throttle / Brake", g_fontSmall, Color(0.88f, 0.92f, 0.98f)); hy += step;
        drawText(30.0f, hy, "A/D or Left/Right: Steer Left / Right", g_fontSmall, Color(0.88f, 0.92f, 0.98f)); hy += step;
        drawText(30.0f, hy, "SPACE / SHIFT    : Nitrous Oxide (NOS)", g_fontSmall, Color(0.35f, 0.92f, 1.00f)); hy += step;
        drawText(30.0f, hy, "CTRL / X         : Handbrake Drift", g_fontSmall, Color(0.88f, 0.92f, 0.98f)); hy += step;
        drawText(30.0f, hy, "1 - 5            : Switch Supercar", g_fontSmall, Color(1.00f, 0.85f, 0.30f)); hy += step;
        drawText(30.0f, hy, "C / TAB          : Camera View", g_fontSmall, Color(0.88f, 0.92f, 0.98f)); hy += step;
        drawText(30.0f, hy, "V                : Time / Weather", g_fontSmall, Color(0.88f, 0.92f, 0.98f)); hy += step;
        drawText(30.0f, hy, "T / G            : Traffic / Ghost Mode", g_fontSmall, Color(0.88f, 0.92f, 0.98f)); hy += step;
        drawText(30.0f, hy, "H / U            : Police Heat / Neon", g_fontSmall, Color(0.88f, 0.92f, 0.98f)); hy += step;
        drawText(30.0f, hy, "M / K            : Audio / KM-H or MPH", g_fontSmall, Color(0.88f, 0.92f, 0.98f)); hy += step;
        drawText(30.0f, hy, "F1: Hide Help | F2: Zen HUD | F11: Full", g_fontSmall, Color(0.58f, 0.72f, 0.88f));
    } else {
        drawText(22.0f, 104.0f, "[F1] Controls  [F2] Zen HUD", g_fontSmall, Color(0.55f, 0.68f, 0.82f, 0.75f));
    }

    if (g_game.paused) {
        drawRect2D(0.0f, 0.0f, (float)width, (float)height, Color(0.02f, 0.03f, 0.06f, 0.65f));
        drawText(width * 0.5f - 95.0f, height * 0.46f, "HIGHWAY PAUSED", g_fontLarge, Color(1.0f, 0.85f, 0.25f));
        drawText(width * 0.5f - 145.0f, height * 0.53f, "Press [ESC] or [P] to Resume  |  [Q] to Quit", g_fontMedium, Color(0.9f, 0.95f, 1.0f));
    }

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ============================================================================
// MAIN SCENE RENDER (WITH REAL-TIME PLANAR MIRROR REFLECTION PASS!)
// ============================================================================
void renderScene(int width, int height) {
    if (height <= 0) height = 1;
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. 2D Atmospheric Sky, Sun/Moon Corona & Horizon Mountain Silhouette
    drawSkyAndHorizon(width, height);

    // 2. 3D Perspective Camera Setup
    // NOTE: glScalef(-1.0f, 1.0f, 1.0f) ensures +X is Screen-Right and -X is Screen-Left
    // when looking down the +Z highway axis!
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(g_game.camFov, (double)width / (double)height, 0.4, VISIBLE_SEGMENTS_AHEAD * SEGMENT_LEN);
    glScalef(-1.0f, 1.0f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    const Atmosphere& atm = ATMOSPHERES[g_game.atmosphereIdx];
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    GLfloat fogCol[4] = { atm.fogColor.r, atm.fogColor.g, atm.fogColor.b, 1.0f };
    glFogfv(GL_FOG_COLOR, fogCol);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_START, 140.0f);
    glFogf(GL_FOG_END, VISIBLE_SEGMENTS_AHEAD * SEGMENT_LEN * 0.94f);

    float camRoadX, camRoadY, tun;
    float lookRoadX, lookRoadY;
    getHighwayPoint(g_game.camS, camRoadX, camRoadY, tun);
    float lookAheadDist = (g_game.cameraMode == 2) ? 45.0f : 26.0f;
    getHighwayPoint(g_game.s + lookAheadDist, lookRoadX, lookRoadY, tun);

    float shakeX = (g_game.camShake > 0.0f) ? randf(-g_game.camShake, g_game.camShake) * 0.45f : 0.0f;
    float shakeY = (g_game.camShake > 0.0f) ? randf(-g_game.camShake, g_game.camShake) * 0.35f : 0.0f;

    float eyeX = camRoadX + g_game.camLatX + shakeX;
    float eyeY = camRoadY + g_game.camHeight + shakeY;
    float eyeZ = g_game.camS;

    float targetX = lookRoadX + g_game.lateralX * 0.85f + sinf(g_game.camYawLag) * 6.0f;
    float targetY = lookRoadY + 1.10f;
    float targetZ = g_game.s + lookAheadDist;

    gluLookAt(eyeX, eyeY, eyeZ, targetX, targetY, targetZ, 0.0, 1.0, 0.0);

    // ------------------------------------------------------------------------
    // PASS A: REAL-TIME PLANAR WET ASPHALT REFLECTION PASS
    // Mirrors the 3D world across the local road elevation plane!
    // ------------------------------------------------------------------------
    float playerRoadX, playerRoadY, playerTun;
    getHighwayPoint(g_game.s, playerRoadX, playerRoadY, playerTun);

    glPushMatrix();
    glTranslatef(0.0f, playerRoadY * 2.0f, 0.0f);
    glScalef(1.0f, -1.0f, 1.0f);
    drawEnvironmentStructures(true);
    for (size_t i = 0; i < g_game.traffic.size(); ++i) {
        if (g_game.traffic[i].s > g_game.camS - 10.0f &&
            g_game.traffic[i].s < g_game.s + 320.0f) {
            drawTrafficCar(g_game.traffic[i], true);
        }
    }
    drawPlayerCar(true);
    glPopMatrix();

    // Clear depth buffer so the road surface and upright 3D world render cleanly on top
    glClear(GL_DEPTH_BUFFER_BIT);

    // ------------------------------------------------------------------------
    // PASS B: SEMI-TRANSPARENT TEXTURED ASPHALT HIGHWAY & BARRIERS
    // Blends over the planar reflection pass for realistic wet tarmac gloss!
    // ------------------------------------------------------------------------
    drawHighwaySurfaceAndBarriers();

    // ------------------------------------------------------------------------
    // PASS C: MAIN UPRIGHT 3D WORLD, TRAFFIC, SUPERCAR & VOLUMETRIC LIGHTS
    // ------------------------------------------------------------------------
    drawEnvironmentStructures(false);

    for (size_t i = 0; i < g_game.traffic.size(); ++i) {
        if (g_game.traffic[i].s > g_game.camS - 15.0f &&
            g_game.traffic[i].s < g_game.s + VISIBLE_SEGMENTS_AHEAD * SEGMENT_LEN) {
            drawTrafficCar(g_game.traffic[i], false);
        }
    }

    drawPlayerCar(false);
    drawParticlesAndWeather();

    // 4. 2D HUD & Telemetry
    drawHUD(width, height);
}

// ============================================================================
// WIN32 WINDOW & FULLSCREEN MANAGEMENT
// ============================================================================
static bool g_fullscreen = false;
static WINDOWPLACEMENT g_wpPrev = { sizeof(g_wpPrev) };

void toggleFullscreen(HWND hwnd) {
    DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
    if (!g_fullscreen) {
        MONITORINFO mi = { sizeof(mi) };
        if (GetWindowPlacement(hwnd, &g_wpPrev) &&
            GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi)) {
            SetWindowLong(hwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(hwnd, HWND_TOP,
                mi.rcMonitor.left, mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            g_fullscreen = true;
        }
    } else {
        SetWindowLong(hwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hwnd, &g_wpPrev);
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        g_fullscreen = false;
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        if (lParam & 0x40000000) break;
        switch (wParam) {
        case VK_ESCAPE:
        case 'P':
            g_game.paused = !g_game.paused;
            break;
        case 'Q':
            if (g_game.paused) PostQuitMessage(0);
            break;
        case '1': case '2': case '3': case '4': case '5':
            g_game.carIndex = (int)(wParam - '1');
            g_game.paintOverride = -1;
            triggerPopup(std::string("CAR SELECTED: ") + CAR_SPECS[g_game.carIndex].name, Color(1.0f, 0.85f, 0.2f));
            break;
        case 'C':
        case VK_TAB:
            g_game.cameraMode = (g_game.cameraMode + 1) % 4;
            {
                const char* camNames[4] = { "DYNAMIC CHASE CAM", "LOW ACTION CAM", "HOOD / BUMPER CAM", "HELICOPTER FAR CAM" };
                triggerPopup(camNames[g_game.cameraMode], Color(0.4f, 0.9f, 1.0f));
            }
            break;
        case 'V':
            g_game.atmosphereIdx = (g_game.atmosphereIdx + 1) % 5;
            triggerPopup(std::string("ATMOSPHERE: ") + ATMOSPHERES[g_game.atmosphereIdx].name, Color(0.95f, 0.65f, 1.0f));
            break;
        case 'T':
            g_game.trafficDensity = (g_game.trafficDensity + 1) % 4;
            rebuildTrafficPool();
            {
                const char* dNames[4] = { "TRAFFIC: LIGHT", "TRAFFIC: NORMAL", "TRAFFIC: HEAVY NO-HESI", "TRAFFIC: EMPTY HIGHWAY" };
                triggerPopup(dNames[g_game.trafficDensity], Color(0.35f, 1.0f, 0.65f));
            }
            break;
        case 'G':
            g_game.ghostMode = !g_game.ghostMode;
            triggerPopup(g_game.ghostMode ? "GHOST CRUISE MODE: ON (NO COLLISIONS)" : "GHOST CRUISE MODE: OFF", Color(0.4f, 0.95f, 0.85f));
            break;
        case 'H':
            g_game.policeHeat = !g_game.policeHeat;
            rebuildTrafficPool();
            triggerPopup(g_game.policeHeat ? "POLICE PURSUIT HEAT: ACTIVE!" : "POLICE PURSUIT HEAT: OFF (CHILL CRUISE)",
                g_game.policeHeat ? Color(1.0f, 0.25f, 0.25f) : Color(0.4f, 0.95f, 0.65f));
            break;
        case 'U':
            g_game.paintOverride = (g_game.paintOverride + 2) % 9 - 1;
            g_game.neonIndex = (g_game.neonIndex + 1) % 9;
            triggerPopup("CUSTOM PAINT & NEON UNDERGLOW CYCLED", Color(0.95f, 0.4f, 1.0f));
            break;
        case 'M':
            g_game.audioMode = (g_game.audioMode + 1) % 3;
            {
                const char* aNames[3] = { "AUDIO: ENGINE + SYNTH RADIO", "AUDIO: PURE ENGINE ONLY", "AUDIO: MUTED" };
                triggerPopup(aNames[g_game.audioMode], Color(0.6f, 0.85f, 1.0f));
            }
            break;
        case 'K':
            g_game.useKmh = !g_game.useKmh;
            triggerPopup(g_game.useKmh ? "SPEED UNITS: KM/H" : "SPEED UNITS: MPH", Color(0.8f, 0.9f, 1.0f));
            break;
        case 'R':
            g_game.lateralX = 2.1f;
            g_game.lateralVel = 0.0f;
            g_game.yawOffset = 0.0f;
            g_game.speed = 35.0f;
            triggerPopup("CAR RESET TO CENTER LANE", Color(0.8f, 0.95f, 1.0f));
            break;
        case VK_F1:
            g_game.showHelp = !g_game.showHelp;
            break;
        case VK_F2:
            g_game.minimalHud = !g_game.minimalHud;
            break;
        case VK_F11:
            toggleFullscreen(hwnd);
            break;
        }
        return 0;

    case WM_SYSKEYDOWN:
        if (wParam == VK_RETURN && (lParam & (1 << 29))) {
            toggleFullscreen(hwnd);
            return 0;
        }
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    SetProcessDPIAware();

    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"NFSHighwayHorizonWnd";
    RegisterClassEx(&wc);

    int winW = 1440;
    int winH = 810;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowEx(
        0, wc.lpszClassName,
        L"NEED FOR SPEED: ENDLESS HORIZON  |  [WASD/Arrows] Drive  [Space] Nitrous  [1-5] Cars  [V] Weather  [F11] Fullscreen",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        std::max(0, (screenW - winW) / 2), std::max(0, (screenH - winH) / 2), winW, winH,
        NULL, NULL, hInstance, NULL
    );

    HDC hdc = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), 1 };
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pf = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pf, &pfd);
    HGLRC hglrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hglrc);

    glShadeModel(GL_SMOOTH);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    initTextures();
    initFonts(hdc);
    initWorld();
    initAudio();

    triggerPopup("ULTRAREALISTIC EDITION - HIT [SPACE] FOR NITROUS!", Color(0.25f, 0.95f, 1.0f));

    LARGE_INTEGER freq, prevTime, currTime;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prevTime);

    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    bool running = true;
    while (running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!running) break;

        QueryPerformanceCounter(&currTime);
        float dt = float(currTime.QuadPart - prevTime.QuadPart) / float(freq.QuadPart);
        prevTime = currTime;

        updateGame(dt, hwnd);
        updateAudio();

        RECT rc;
        GetClientRect(hwnd, &rc);
        renderScene(rc.right - rc.left, rc.bottom - rc.top);
        SwapBuffers(hdc);
        Sleep(1);
    }

    shutdownAudio();
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
    return 0;
}
