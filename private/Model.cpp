#include <io.h> 
#include <iostream>
#include <fstream>
#include <sstream>
#include "../public/Model.h"
#include "../shader/shader.h"

Model::Model(const char* filename, bool skybox, bool reverse) : is_skybox(skybox), is_reverse(reverse)
{
	std::ifstream in;
	in.open(filename, std::ifstream::in);
	if (in.fail()) {
		printf("load model failed\n");
		return;
	}

	std::string line;
	while (!in.eof()) {
		std::getline(in, line);					//读取一行数据
		std::istringstream iss(line.c_str());
		char trash;							//垃圾桶
		if (!line.compare(0, 2, "v "))		//比对首2个字符和v+空格，匹配则返回0
		{
			iss >> trash;
			vec3 v;
			for (int i = 0; i < 3; i++)
				iss >> v[i];
			vert_buffer.push_back(v);		//vert存储顶点坐标序列，下标从0开始
		}
		else if (!line.compare(0, 3, "vn "))//比对首3个字符和v+n+空格，匹配则返回0
		{
			iss >> trash >> trash;
			vec3 n;
			for (int i = 0; i < 3; i++)
				iss >> n[i];
			norm_buffer.push_back(n);		//norm存储顶点法线坐标序列，下标从0开始
		}
		else if (!line.compare(0, 3, "vt "))//比对首3个字符和v+t+空格，匹配则返回0
		{
			iss >> trash >> trash;
			vec2 uv;
			for (int i = 0; i < 2; i++)
				iss >> uv[i];

			if (is_reverse)					//翻转
				uv[1] = 1 - uv[1];
			uv_buffer.push_back(uv);		//norm存储顶点纹理坐标序列，下标从0开始
		}
		else if (!line.compare(0, 2, "f "))
		{
			std::vector<int> f;
			int tmp[3];
			iss >> trash;
			while (iss >> tmp[0] >> trash >> tmp[1] >> trash >> tmp[2])	//读取三角形平面一个顶点的坐标、法线、纹理坐标的索引
			{															//读取一行，f存入了3个顶点的各种索引，即9个索引
				for (int i = 0; i < 3; i++)
				{
					tmp[i]--;
					f.push_back(tmp[i]);// 文件中的序列下标从1开始，减一匹配buffer中的下标
				}		
			}
			face_index.push_back(f);									//存入了一个三角形三个顶点的9个索引
		}
	}
	std::cerr << "# v# " << vert_buffer.size() << " f# " << face_index.size() << " vt# " << uv_buffer.size() << " vn# " << norm_buffer.size() << std::endl;
	create_map(filename);

	environment_map = NULL;
	if (is_skybox) {
		environment_map = new cubemap_t();
		load_cubemap(filename);
	}
}

Model::~Model()
{
	if (diffuse_map) delete diffuse_map; diffuse_map = NULL;
	if (normal_map) delete normal_map; normal_map = NULL;
	if (specular_map) delete specular_map; specular_map = NULL;
	if (roughness_map) delete roughness_map; roughness_map = NULL;
	if (metalness_map) delete metalness_map; metalness_map = NULL;
	if (occlusion_map) delete occlusion_map; occlusion_map = NULL;
	if (emision_map) delete emision_map; emision_map = NULL;
	if (environment_map){
		for (int i = 0; i < 6; i++)
			delete environment_map->faces[i];
			delete environment_map;
	}
}

void Model::create_map(const char* filename)
{
	std::string texfile(filename);
	size_t dot = texfile.find_last_of(".");

	texfile = texfile.substr(0, dot) + std::string("_diffuse.tga");
	if (_access(texfile.data(), 0) != -1)							//_access,0判断文件是否存在
	{
		diffuse_map = new Image();
		diffuse_map->read_tga_file(texfile.c_str());
		diffuse_map->flip_vertically();
	}

	texfile = texfile.substr(0, dot) + std::string("_normal.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		normal_map = new Image();
		normal_map->read_tga_file(texfile.c_str());
		normal_map->flip_vertically();
	}

	texfile = texfile.substr(0, dot) + std::string("_spec.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		specular_map = new Image();
		specular_map->read_tga_file(texfile.c_str());
		specular_map->flip_vertically();
	}

	texfile = texfile.substr(0, dot) + std::string("_roughness.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		roughness_map = new Image();
		roughness_map->read_tga_file(texfile.c_str());
		roughness_map->flip_vertically();
	}

	texfile = texfile.substr(0, dot) + std::string("_metalness.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		metalness_map = new Image();
		metalness_map->read_tga_file(texfile.c_str());
		metalness_map->flip_vertically();
	}

	texfile = texfile.substr(0, dot) + std::string("_emission.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		occlusion_map = new Image();
		occlusion_map->read_tga_file(texfile.c_str());
		occlusion_map->flip_vertically();
	}

	texfile = texfile.substr(0, dot) + std::string("_occlusion.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		emision_map = new Image();
		emision_map->read_tga_file(texfile.c_str());
		emision_map->flip_vertically();
	}
}

void Model::load_cubemap(const char* filename)	//../obj/skybox4/box.obj
{	

	std::string texfile(filename);
	size_t dot = texfile.find_last_of(".");
	for(int i = 0; i < 6; ++i) environment_map->faces[i] = new Image();

	texfile = texfile.substr(0, dot) + std::string("_right.tga");
	environment_map->faces[0]->read_tga_file(texfile.c_str());
	environment_map->faces[0]->flip_vertically();

	texfile = texfile.substr(0, dot) + std::string("_left.tga");
	environment_map->faces[1]->read_tga_file(texfile.c_str());
	environment_map->faces[1]->flip_vertically();

	texfile = texfile.substr(0, dot) + std::string("_top.tga");
	environment_map->faces[2]->read_tga_file(texfile.c_str());
	environment_map->faces[2]->flip_vertically();

	texfile = texfile.substr(0, dot) + std::string("_bottom.tga");
	environment_map->faces[3]->read_tga_file(texfile.c_str());
	environment_map->faces[3]->flip_vertically();

	texfile = texfile.substr(0, dot) + std::string("_back.tga");
	environment_map->faces[4]->read_tga_file(texfile.c_str());
	environment_map->faces[4]->flip_vertically();

	texfile = texfile.substr(0, dot) + std::string("_front.tga");
	environment_map->faces[5]->read_tga_file(texfile.c_str());
	environment_map->faces[5]->flip_vertically();
}

//------------------------------------------------------------------------

int Model::number_of_verts() {
	return vert_buffer.size();
}

int Model::number_of_faces() {
	return face_index.size();
}

std::vector<int> Model::allv_of_facei(int idx) {
	std::vector<int> verts;
	for (int i = 0; i < 3; ++i) verts.push_back(face_index[idx][i * 3]);
	return verts;
}

vec3 Model::get_vert(int i)
{
	return vert_buffer[i];
}

vec3 Model::v_of_facei(int iface, int nthvert)
{
	return vert_buffer[face_index[iface][nthvert * 3]];
}

vec3 Model::vn_of_facei(int iface, int nthvert)
{
	int idx = face_index[iface][nthvert * 3 + 2];
	return norm_buffer[idx].normalize();
}

vec3 Model::get_normalmap(vec2 uv)
{
	uv[0] = fmod(uv[0], 1);							//取小数部分
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * normal_map->get_width();
	int uv1 = uv[1] * normal_map->get_height();
	Color c = normal_map->get_color(uv0, uv1);
	vec3 res;
	for (int i = 0; i < 3; i++)
		res[2 - i] = (float)c[i] / 255.f * 2.f - 1.f; //法线贴图坐标范围从-1~+1
	return res;
}

vec2 Model::uv_of_facei(int iface, int nthvert)
{
	return uv_buffer[face_index[iface][nthvert * 3 + 1]];
}

vec3 Model::get_diffusemap(vec2 uv)
{
	//default material
	if (diffuse_map == nullptr) {
		return vec3(0.41f, 0.41f, 0.41f);
	}
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * diffuse_map->get_width();
	int uv1 = uv[1] * diffuse_map->get_height();
	Color c = diffuse_map->get_color(uv0, uv1);
	vec3 res;
	for (int i = 0; i < 3; i++)
		res[i] = (float)c[i] / 255.f;
	return res;
}

float Model::get_roughnessmap(vec2 uv)
{
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * roughness_map->get_width();
	int uv1 = uv[1] * roughness_map->get_height();
	return roughness_map->get_color(uv0, uv1)[0] / 255.f;
}

float Model::get_metalnessmap(vec2 uv)
{
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * metalness_map->get_width();
	int uv1 = uv[1] * metalness_map->get_height();
	return metalness_map->get_color(uv0, uv1)[0] / 255.f;
}

float Model::get_specularmap(vec2 uv)
{
	int uv0 = uv[0] * specular_map->get_width();
	int uv1 = uv[1] * specular_map->get_height();
	return specular_map->get_color(uv0, uv1)[0] / 1.f;
}

float Model::get_occlusionmap(vec2 uv)
{
	if (!occlusion_map)
		return 1;
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * occlusion_map->get_width();
	int uv1 = uv[1] * occlusion_map->get_height();
	return occlusion_map->get_color(uv0, uv1)[0] / 255.f;
}

vec3 Model::get_emissionmap(vec2 uv)
{
	if (!emision_map)
		return vec3(0.0f, 0.0f, 0.0f);
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * emision_map->get_width();
	int uv1 = uv[1] * emision_map->get_height();
	Color c = emision_map->get_color(uv0, uv1);
	vec3 res;
	for (int i = 0; i < 3; i++)
		res[2 - i] = (float)c[i] / 255.f;
	return res;
}