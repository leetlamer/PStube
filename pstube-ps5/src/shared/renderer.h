#pragma once
#include <stdint.h>

namespace cube {
constexpr int Width = 1920;
constexpr int Height = 1080;
constexpr uint32_t Background = 0xff0c1220;
struct Vec3 { float x, y, z; };
struct Point { float x, y; };
struct Angles { float x = 0.35f, y = 0.55f; };
struct Surface { uint32_t* pixels; int width, height, pitch; };
Vec3 rotate(Vec3 v, Angles a);
bool project(Vec3 camera, int width, int height, Point& out);
void advance(Angles& a, double seconds);
void render(Surface target, Angles a);
}
