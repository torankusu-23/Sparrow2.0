#pragma once
#include "../shader/shader.h"


vec3 texture_sample(vec2 uv, Image* image);					//�������в���������ɫ
vec3 cubemap_sampling(vec3 direction, cubemap_t* cubemap);	//������ͼ����

void generate_prefilter_map(int thread_id, int face_id, int mip_level, Image& image);	//����Ԥ����ͼ�����淴�䲿�֣�����
void generate_irradiance_map(int thread_id, int face_id, Image& image);					//���ɷ���ͼ