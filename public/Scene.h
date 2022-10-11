#pragma once
#include "Image.h"
#include "../shader/shader.h"

Image* texture_from_file(const char* file_name);

void load_ibl_map(payload_t& p, const char* env_path);

void build_scene(std::string scene_name, Model** model, int& m, IShader** shader_use, IShader** shader_skybox, mat4 perspective, Camera* camera);