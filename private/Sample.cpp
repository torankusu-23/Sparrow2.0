#include "../public/Sample.h"
#include "cstdlib"
#include "thread"

using namespace std;

//���ߺ���ǰ������
static int cal_cubemap_uv(vec3 direction, vec2& uv);	//�������������ȡ���������
vec2 hammersley2d(unsigned int i, unsigned int N);		//Hammersley�����㷨
vec3 hemisphereSample_uniform(float u, float v);
vec3 hemisphereSample_cos(float u, float v);
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness);//GGX���Աƽ�����Ҫ�Բ����㷨
static float SchlickGGX_geometry(float n_dot_v, float roughness);
static float geometry_Smith(float n_dot_v, float n_dot_l, float roughness);
void set_normal_coord(int face_id, int x, int y, float& x_coord, float& y_coord, float& z_coord, float length = 255);//���岻ͬ��ķ���


vec3 texture_sample(vec2 uv, Image* image)
{
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * image->get_width();	//������ͼ�л�ȡ����λ��
	int uv1 = uv[1] * image->get_height();
	Color c = image->get_color(uv0, uv1);
	vec3 res;
	for (int i = 0; i < 3; i++)
		res[i] = (float)c[i] / 255.f;
	return res;
}


vec3 cubemap_sampling(vec3 direction, cubemap_t* cubemap) {	//�������꣬����ͼ
	vec2 uv;
	int face_index = cal_cubemap_uv(direction, uv);			//����������кŲ�����������浽uv
	vec3 color = texture_sample(uv, cubemap->faces[face_index]);	//������������uv�͸����кŵ�����ͼ�����ɫ
	return color;
}

/* specular part */
void generate_prefilter_map(int thread_id, int face_id, int mip_level, Image& image)	//MIPMAP������ͼ�ȼ������������5���ƺ����������
{
	int factor = 1;
	for (int temp = 0; temp < mip_level; temp++) factor *= 2;

	int width = 512 / factor;
	int height = 512 / factor;

	if (width < 64) width = 64;		//��ɶ��˼���������ƣ��߶Ȳ��ޣ�

	int x, y;
	const char* modelname5[] =		//�����ģ��˽�˶��Ƶ�·����
	{
		"obj/gun/Cerberus.obj",
		"obj/skybox4/box.obj",
	};
		
	float roughness[10];			//�ֲڶȣ�
	for (int i = 0; i < 10; i++) roughness[i] = (float)i / 9.0;
	
	// for multi-thread 
	//int interval = width / thread_num;
	//int start = thread_id * interval;
	//int end = (thread_id + 1) * interval;
	//if (thread_id == thread_num - 1)
	//	end = width;

	Model* model = new Model(modelname5[1], true);
	payload_t p;
	p.model = model;

	vec3 prefilter_color(0, 0, 0);
	for (x = 0; x < height; x++)
	{
		for (y = 0; y < width; y++)
		{
			float x_coord, y_coord, z_coord;
			set_normal_coord(face_id, x, y, x_coord, y_coord, z_coord, float(width - 1)); //���ݲ�ͬ�������÷������ꣿ

			vec3 normal = vec3(x_coord, y_coord, z_coord);
			normal = normal.normalize();					//z-axis
			vec3 up = fabs(normal[1]) < 0.999f ? vec3(0.0f, 1.0f, 0.0f) : vec3(0.0f, 0.0f, 1.0f);
			vec3 right = cross(up, normal).normalize();	//x-axis
			up = cross(normal, right);						//y-axis

			vec3 r = normal;
			vec3 v = r;

			prefilter_color = vec3(0, 0, 0);
			float total_weight = 0.0f;
			int numSamples = 1024;
			for (int i = 0; i < numSamples; i++)
			{
				vec2 Xi = hammersley2d(i, numSamples);
				vec3 h = ImportanceSampleGGX(Xi, normal, roughness[mip_level]);
				vec3 l = (2.0 * dot(v, h) * h - v).normalize();

				vec3 radiance = cubemap_sampling(l, p.model->environment_map);
				float n_dot_l = fmax(dot(normal, l), 0.0);

				if (n_dot_l > 0)
				{
					prefilter_color = prefilter_color + radiance * n_dot_l;
					total_weight = total_weight + n_dot_l;
				}
			}

			prefilter_color = prefilter_color / total_weight;
			//cout << irradiance << endl;
			int red = fmin(prefilter_color.x() * 255.0f, 255);
			int green = fmin(prefilter_color.y() * 255.0f, 255);
			int blue = fmin(prefilter_color.z() * 255.0f, 255);

			//cout << irradiance << endl;
			Color temp(red, green, blue);
			image.set_color(x, y, temp);
		}
		printf("%f% \n", x / 512.0f);
	}
}


/* diffuse part */
void generate_irradiance_map(int thread_id, int face_id, Image& image)
{
	int x, y;
	const char* modelname5[] =	//Ӳ�������Ҫ��
	{
		"obj/gun/Cerberus.obj",
		"obj/skybox4/box.obj",
	};

	/* for multi-thread */
	//int interval = 256 / thread_num;
	//int start = thread_id * interval;
	//int end = (thread_id + 1) * interval;
	//if (thread_id == thread_num - 1)
	//	end = 256;

	Model* model[1];
	model[0] = new Model(modelname5[1], 1);

	payload_t p;
	p.model = model[0];

	vec3 irradiance(0, 0, 0);
	for (x = 0; x < 256; x++)
	{
		for (y = 0; y < 256; y++)
		{
			float x_coord, y_coord, z_coord;
			set_normal_coord(face_id, x, y, x_coord, y_coord, z_coord);
			vec3 normal = vec3(x_coord, y_coord, z_coord).normalize();					 //z-axis
			vec3 up = fabs(normal[1]) < 0.999f ? vec3(0.0f, 1.0f, 0.0f) : vec3(0.0f, 0.0f, 1.0f);
			vec3 right = cross(up, normal).normalize();								 //tagent x-axis
			up = cross(normal, right);					                                 //tagent y-axis

			irradiance = vec3(0, 0, 0);
			float sampleDelta = 0.025f;
			int numSamples = 0;
			for (float phi = 0.0f; phi < 2.0 * PI; phi += sampleDelta)
			{
				for (float theta = 0.0f; theta < 0.5 * PI; theta += sampleDelta)
				{
					// spherical to cartesian (in tangent space)
					vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
					// tangent space to world
					vec3 sampleVec = tangentSample.x() * right + tangentSample.y() * up + tangentSample.z() * normal;
					sampleVec = sampleVec.normalize();
					vec3 color = cubemap_sampling(sampleVec, p.model->environment_map);

					irradiance = irradiance + color * sin(theta) * cos(theta);
					numSamples++;
				}
			}

			irradiance = PI * irradiance * (1.0f / numSamples);
			int red = fmin(irradiance.x() * 255.0f, 255);
			int green = fmin(irradiance.y() * 255.0f, 255);
			int blue = fmin(irradiance.z() * 255.0f, 255);

			Color temp(red, green, blue);
			image.set_color(x, y, temp);
		}
		printf("%f% \n", x / 256.0f);
	}
}

//-------------------------------------------------------------------���ߺ���

static int cal_cubemap_uv(vec3 direction, vec2& uv)	//����һ���������꣬����õ�������������������к�
{
	int face_index = -1;
	float ma = 0, sc = 0, tc = 0;
	float abs_x = fabs(direction[0]), abs_y = fabs(direction[1]), abs_z = fabs(direction[2]);//���������Ǻ��������������������ô����õ��ģ���ͷ����������

	if (abs_x > abs_y && abs_x > abs_z)			/* major axis -> x ��x��������ᣬ�������������*/
	{
		ma = abs_x;
		if (direction.x() > 0)					/* positive x ������x�����棬���к�0*/
		{
			face_index = 0;
			sc = +direction.z();
			tc = +direction.y();
		}
		else									/* negative x ������x�����棬���к�1*/
		{
			face_index = 1;
			sc = -direction.z();
			tc = +direction.y();
		}
	}
	else if (abs_y > abs_z)						/* major axis -> y */
	{
		ma = abs_y;
		if (direction.y() > 0)					/* positive y */
		{
			face_index = 2;
			sc = +direction.x();
			tc = +direction.z();
		}
		else									/* negative y */
		{
			face_index = 3;
			sc = +direction.x();
			tc = -direction.z();
		}
	}
	else										/* major axis -> z */
	{
		ma = abs_z;
		if (direction.z() > 0)					/* positive z */
		{
			face_index = 4;
			sc = -direction.x();
			tc = +direction.y();
		}
		else									/* negative z */
		{
			face_index = 5;
			sc = +direction.x();
			tc = +direction.y();
		}
	}

	uv[0] = (sc / ma + 1.0f) / 2.0f;
	uv[1] = (tc / ma + 1.0f) / 2.0f;

	return face_index;
}


void set_normal_coord(int face_id, int x, int y, float& x_coord, float& y_coord, float& z_coord, float length)
{
	switch (face_id)
	{
	case 0:   //positive x (right face)
		x_coord = 0.5f;
		y_coord = -0.5f + y / length;
		z_coord = -0.5f + x / length;
		break;
	case 1:   //negative x (left face)		
		x_coord = -0.5f;
		y_coord = -0.5f + y / length;
		z_coord = 0.5f - x / length;
		break;
	case 2:   //positive y (top face)
		x_coord = -0.5f + x / length;
		y_coord = 0.5f;
		z_coord = -0.5f + y / length;
		break;
	case 3:   //negative y (bottom face)
		x_coord = -0.5f + x / length;
		y_coord = -0.5f;
		z_coord = 0.5f - y / length;
		break;
	case 4:   //positive z (back face)
		x_coord = 0.5f - x / length;
		y_coord = -0.5f + y / length;
		z_coord = 0.5f;
		break;
	case 5:   //negative z (front face)
		x_coord = -0.5f + x / length;
		y_coord = -0.5f + y / length;
		z_coord = -0.5f;
		break;
	default:
		break;
	}
}

/* lut part */
vec3 IntegrateBRDF(float NdotV, float roughness)
{
	// ���ڸ���ͬ�ԣ�����ȡһ�� V ����
	vec3 V;
	V[0] = 0;
	V[1] = sqrt(1.0 - NdotV * NdotV);
	V[2] = NdotV;

	float A = 0.0;
	float B = 0.0;
	float C = 0.0;

	vec3 N = vec3(0.0, 0.0, 1.0);

	const int SAMPLE_COUNT = 1024;
	for (int i = 0; i < SAMPLE_COUNT; ++i)
	{
		// generates a sample vector that's biased towards the
		// preferred alignment direction (importance sampling).
		vec2 Xi = hammersley2d(i, SAMPLE_COUNT);

		{ // A and B
			vec3 H = ImportanceSampleGGX(Xi, N, roughness);
			vec3 L = (2.0 * dot(V, H) * H - V).normalize();

			float NdotL = fmax(L.z(), 0.0);
			float NdotV = fmax(V.z(), 0.0);
			float NdotH = fmax(H.z(), 0.0);
			float VdotH = fmax(dot(V, H), 0.0);

			if (NdotL > 0.0)
			{
				float G = geometry_Smith(NdotV, NdotL, roughness);
				float G_Vis = (G * VdotH) / (NdotH * NdotV);
				float Fc = pow(1.0 - VdotH, 5.0);

				A += (1.0 - Fc) * G_Vis;
				B += Fc * G_Vis;
			}
		}
	}
	return vec3(A, B, C) / float(SAMPLE_COUNT);
}

/* traverse all 2d coord for lut part */
void calculate_BRDF_LUT(Image& image)
{
	int i, j;
	for (i = 0; i < 256; i++)
	{
		for (j = 0; j < 256; j++)
		{
			vec3 color;
			if (i == 0)
				color = IntegrateBRDF(0.002f, j / 256.0f);
			else
				color = IntegrateBRDF(i / 256.0f, j / 256.0f);
			//cout << irradiance << endl;
			int red = fmin(color.x() * 255.0f, 255);
			int green = fmin(color.y() * 255.0f, 255);
			int blue = fmin(color.z() * 255.0f, 255);

			//cout << irradiance << endl;
			Color temp(red, green, blue);
			image.set_color(i, j, temp);
		}
	}
}

/* traverse all mipmap level for prefilter map */
void foreach_prefilter_miplevel(Image& image)
{
	const char* faces[6] = { "px", "nx", "py", "ny", "pz", "nz" };
	char paths[6][256];
	const int thread_num = 4;

	for (int mip_level = 8; mip_level < 10; mip_level++)
	{
		for (int j = 0; j < 6; j++) {
			sprintf_s(paths[j], "%s/m%d_%s.tga", "./obj/common2", mip_level, faces[j]);
		}
		int factor = 1;
		for (int temp = 0; temp < mip_level; temp++)
			factor *= 2;
		int w = 512 / factor;
		int h = 512 / factor;

		if (w < 64)
			w = 64;
		if (h < 64)
			h = 64;
		cout << w << h << endl;
		image = Image(w, h, Image::RGB);
		for (int face_id = 0; face_id < 6; face_id++)
		{
			std::thread thread[thread_num];
			for (int i = 0; i < thread_num; i++)
				thread[i] = std::thread(generate_prefilter_map, i, face_id, mip_level, std::ref(image));
			for (int i = 0; i < thread_num; i++)
				thread[i].join();

			//calculate_BRDF_LUT();
			image.flip_vertically(); // to place the origin in the bottom left corner of the image
			image.write_tga_file(paths[face_id]);
		}
	}

}

/* traverse all faces of cubemap for irradiance map */
void foreach_irradiance_map(Image& image)
{
	const char* faces[6] = { "px", "nx", "py", "ny", "pz", "nz" };
	char paths[6][256];
	const int thread_num = 4;


	for (int j = 0; j < 6; j++) {
		sprintf_s(paths[j], "%s/i_%s.tga", "./obj/common2", faces[j]);
	}
	image = Image(256, 256, Image::RGB);
	for (int face_id = 0; face_id < 6; face_id++)
	{
		std::thread thread[thread_num];
		for (int i = 0; i < thread_num; i++)
			thread[i] = std::thread(generate_irradiance_map, i, face_id, std::ref(image));
		for (int i = 0; i < thread_num; i++)
			thread[i].join();



		image.flip_vertically(); // to place the origin in the bottom left corner of the image
		image.write_tga_file(paths[face_id]);
	}

}

//hammersley�����㷨��RadicalInverse�������в���
//RadicalInverse�㷨��ʮ���������ж����Ʊ�ʾ���ٹ�����������ֲ��㡣
//�ο���https://blog.csdn.net/i_dovelemon/article/details/76599923?spm=1001.2014.3001.5501
vec2 hammersley2d(unsigned int i, unsigned int N) {
	float ans = 0;
	float f = 0.5;
	while (i != 0) {
		ans += f * (i & 1);
		i /= 2;
		f /= 2;
	}
	return vec2(float(i) / float(N), ans);	
}

vec3 hemisphereSample_uniform(float u, float v) {
	float phi = v * 2.0f * PI;
	float cosTheta = 1.0f - u;
	float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
	return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

vec3 hemisphereSample_cos(float u, float v) {
	float phi = v * 2.0 * PI;
	float cosTheta = sqrt(1.0 - u);
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
	float a = roughness * roughness;

	float phi = 2.0 * PI * Xi.x();
	float cosTheta = sqrt((1.0 - Xi.y()) / (1.0 + (a * a - 1.0) * Xi.y()));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	// from spherical coordinates to cartesian coordinates
	vec3 H;
	H[0] = cos(phi) * sinTheta;
	H[1] = sin(phi) * sinTheta;
	H[2] = cosTheta;

	// from tangent-space vector to world-space sample vector
	vec3 up = abs(N.z()) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = cross(up, N).normalize();
	vec3 bitangent = cross(N, tangent);

	vec3 sampleVec = tangent * H.x() + bitangent * H.y() + N * H.z();
	return sampleVec.normalize();
}

static float SchlickGGX_geometry(float n_dot_v, float roughness)
{
	float r = (1 + roughness);
	float k = r * r / 8.0;
	k = roughness * roughness / 2.0f;
	return n_dot_v / (n_dot_v * (1 - k) + k);
}

static float geometry_Smith(float n_dot_v, float n_dot_l, float roughness)
{
	float g1 = SchlickGGX_geometry(n_dot_v, roughness);
	float g2 = SchlickGGX_geometry(n_dot_l, roughness);

	return g1 * g2;
}