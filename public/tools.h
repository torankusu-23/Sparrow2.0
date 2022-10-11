#pragma once
#include "struct.h"

//not good enough
float _sin(float a);
float _cos(float a);

mat4 mat4_translate(float tx, float ty, float tz);
mat4 mat4_scale(float sx, float sy, float sz);
mat4 mat4_rotate_x(float angle);
mat4 mat4_rotate_y(float angle);
mat4 mat4_rotate_z(float angle);
mat4 Model_View(vec3 eye, vec3 target, vec3 up);
mat4 mat4_ortho(float left, float right, float bottom, float top, float near, float far);
mat4 mat4_perspective(float fovy, float aspect, float near, float far);

float float_clamp(float f, float min, float max);
float float_lerp(float start, float end, float alpha);
vec2 vec2_lerp(vec2& start, vec2& end, float alpha);
vec3 vec3_lerp(vec3& start, vec3& end, float alpha);
vec4 vec4_lerp(vec4& start, vec4& end, float alpha);


void clear_zbuffer(int width, int height, float* zbuffer);
void clear_framebuffer(int width, int height, unsigned char* framebuffer);
