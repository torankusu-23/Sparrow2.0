#pragma once
#include "cstring"
#include <vector>
#include "tools.h"



typedef struct cubemap cubemap_t;

class Model {
private:
	std::vector<std::vector<int>> face_index;		//ͬ同一个面的三个顶点的索引，存在同一块内存face_index[i]中
	std::vector<vec3> vert_buffer;
	std::vector<vec3> norm_buffer;
	std::vector<vec2> uv_buffer;

	void create_map(const char* filename);
	void load_cubemap(const char* filename);
public:
	Model(const char* filename, bool is_skybox = false, bool is_reverse = false);
	~Model();
	cubemap_t* environment_map = nullptr;//skybox天空盒，或者叫环境光照贴图
	bool is_skybox = false;
	bool is_reverse = true;

	Image* diffuse_map = nullptr;	//扩展先准备好
	Image* normal_map = nullptr;
	Image* specular_map = nullptr;
	Image* roughness_map = nullptr;
	Image* metalness_map = nullptr;
	Image* occlusion_map = nullptr;
	Image* emision_map = nullptr;

	int number_of_verts();
	int number_of_faces();
	std::vector<int> allv_of_facei(int idx);

	vec3 get_vert(int i);
	vec3 v_of_facei(int iface, int nthvert);
	vec3 get_normalmap(vec2 uv);
	vec3 vn_of_facei(int iface, int nthvert);
	vec2 uv_of_facei(int iface, int nthvert);
	vec3 get_diffusemap(vec2 uv);
	float get_roughnessmap(vec2 uv);
	float get_metalnessmap(vec2 uv);
	vec3 get_emissionmap(vec2 uv);
	float get_occlusionmap(vec2 uv);
	float get_specularmap(vec2 uv);

};