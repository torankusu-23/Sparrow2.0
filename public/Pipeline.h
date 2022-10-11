#pragma once

#include "tools.h"
#include "Win.h"
#include "Light.h"
#include "../shader/shader.h"




void rasterize_singlethread(vec4* clipcoord_attri, unsigned char* framebuffer, float* zbuffer, IShader& shader);
void rasterize_multhread(vec4* clipcoord_attri, unsigned char* framebuffer, float* zbuffer, IShader& shader);

void draw_triangles(unsigned char* framebuffer, float* zbuffer, IShader& shader, int nface);
