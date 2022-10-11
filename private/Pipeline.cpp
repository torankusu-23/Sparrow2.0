#include "../public/Pipeline.h"
#include<typeinfo>
#include<thread>
#include<list>
#include<mutex>

std::mutex mtx;
static bool is_back_facing(vec3 ndc_pos[3]);
static vec3 compute_barycentric2D(float x, float y, const vec3* v);
static void set_color(unsigned char* framebuffer, int x, int y, int width, int height, unsigned char color[]);
static bool is_inside_triangle(float alpha, float beta, float gamma);
static int get_index(int x, int y, int width, int height);

typedef enum
{
	W_PLANE,
	X_RIGHT,
	X_LEFT,
	Y_TOP,
	Y_BOTTOM,
	Z_NEAR,
	Z_FAR
} clip_plane;
static int is_inside_plane(clip_plane c_plane, vec4 vertex, payload_t& payload);
static float get_intersect_ratio(vec4 prev, vec4 curv, clip_plane c_plane);
static int clip_with_plane(clip_plane c_plane, int num_vert, payload_t& payload);
static int homo_clipping(payload_t& payload);
static void transform_attri(payload_t& payload, int index0, int index1, int index2);


void rasterize_multhread(vec4* clipcoord_attri, unsigned char* framebuffer, float* zbuffer, IShader& shader) {
	vec3 ndc_pos[3];
	vec3 screen_pos[3];
	unsigned char c[3];
	int width = shader.payload.width;
	int height = shader.payload.height;
	bool is_skybox = shader.payload.model->is_skybox;

	// homogeneous division 透视除法
	for (int i = 0; i < 3; i++)
	{
		ndc_pos[i][0] = clipcoord_attri[i][0] / clipcoord_attri[i].w();
		ndc_pos[i][1] = clipcoord_attri[i][1] / clipcoord_attri[i].w();
		ndc_pos[i][2] = clipcoord_attri[i][2] / clipcoord_attri[i].w();
	}

	// viewport transformation 视口变换
	for (int i = 0; i < 3; i++)
	{
		screen_pos[i][0] = 0.5 * (width - 1) * (ndc_pos[i][0] + 1.0);
		screen_pos[i][1] = 0.5 * (height - 1) * (ndc_pos[i][1] + 1.0);
		screen_pos[i][2] = is_skybox ? 1000 : -clipcoord_attri[i].w();	//view space z-value 天空盒固定z轴1000，一般三角形裁剪坐标w,
	}

	// backface clip (skybox didnit need it) 剔除方向向后的面
	if (!is_skybox)
	{
		if (is_back_facing(ndc_pos)) return;
	}

	// get bounding box 包围盒
	float xmin = 10000, xmax = -10000, ymin = 10000, ymax = -10000;
	for (int i = 0; i < 3; i++)
	{
		xmin = fmin(xmin, screen_pos[i][0]);
		xmax = fmax(xmax, screen_pos[i][0]);
		ymin = fmin(ymin, screen_pos[i][1]);
		ymax = fmax(ymax, screen_pos[i][1]);
	}

	
	// rasterization 光栅化
	for (int x = (int)xmin; x <= (int)xmax; ++x)
	{
		for (int y = (int)ymin; y <= (int)ymax; ++y)
		{
			vec3 barycentric = compute_barycentric2D((float)(x + 0.5), (float)(y + 0.5), screen_pos);
			float alpha = barycentric.x(); float beta = barycentric.y(); float gamma = barycentric.z();

			if (is_inside_triangle(alpha, beta, gamma))
			{

				int index = get_index(x, y, width, height);	//获取坐标在zbuffer坐标
				// z坐标透视校正,这里的z是view space的
				float z = -1.0 / (alpha / clipcoord_attri[0].w() + beta / clipcoord_attri[1].w() + gamma / clipcoord_attri[2].w());

				//方向光的shadow map是需要正交投影
				if (shader.payload.isShaderMapPass && shader.payload.light->getType() == DirectionalType) {
					float ProjectionDepth = 2.0 / shader.payload.light->getProjectionMat()[2][2];
					z = alpha * clipcoord_attri[0].z() + beta * clipcoord_attri[1].z() + gamma * clipcoord_attri[2].z();
					z = (z * -0.5 + 0.5) * ProjectionDepth;
				}
				//mtx.lock();
				if (zbuffer[index] > z)
				{
					zbuffer[index] = z;
					if (shader.payload.isShaderMapPass) continue;		//shadow map pass

					vec3 color = shader.fragment_shader(alpha, beta, gamma);

					for (int i = 0; i < 3; i++)
					{
						c[i] = (int)float_clamp(color[i], 0, 255);
					}
					set_color(framebuffer, x, y, width, height, c);
				}
				//mtx.unlock();
			}
		}
	}
}


void rasterize_singlethread(vec4* clipcoord_attri, unsigned char* framebuffer, float* zbuffer, IShader& shader) {
	vec3 ndc_pos[3];
	vec3 screen_pos[3];
	unsigned char c[3];
	int width = shader.payload.width;
	int height = shader.payload.height;
	bool is_skybox = shader.payload.model->is_skybox;

	// homogeneous division 透视除法
	for (int i = 0; i < 3; i++)
	{
		ndc_pos[i][0] = clipcoord_attri[i][0] / clipcoord_attri[i].w();
		ndc_pos[i][1] = clipcoord_attri[i][1] / clipcoord_attri[i].w();
		ndc_pos[i][2] = clipcoord_attri[i][2] / clipcoord_attri[i].w();
	}

	// viewport transformation 视口变换
	for (int i = 0; i < 3; i++)
	{
		screen_pos[i][0] = 0.5 * (width - 1) * (ndc_pos[i][0] + 1.0);
		screen_pos[i][1] = 0.5 * (height - 1) * (ndc_pos[i][1] + 1.0);
		screen_pos[i][2] = is_skybox ? 1000 : -clipcoord_attri[i].w();	//view space z-value 天空盒固定z轴1000，一般三角形裁剪坐标w,
	}

	// backface clip (skybox didnit need it) 剔除方向向后的面
	if (!is_skybox)
	{
		if (is_back_facing(ndc_pos)) return;
	}

	// get bounding box 包围盒
	float xmin = 10000, xmax = -10000, ymin = 10000, ymax = -10000;
	for (int i = 0; i < 3; i++)
	{
		xmin = fmin(xmin, screen_pos[i][0]);
		xmax = fmax(xmax, screen_pos[i][0]);
		ymin = fmin(ymin, screen_pos[i][1]);
		ymax = fmax(ymax, screen_pos[i][1]);
	}
	
	// rasterization 光栅化
	for (int x = (int)xmin; x <= (int)xmax; ++x)
	{
		for (int y = (int)ymin; y <= (int)ymax; ++y)
		{
			vec3 barycentric = compute_barycentric2D((float)(x + 0.5), (float)(y + 0.5), screen_pos);
			float alpha = barycentric.x(); float beta = barycentric.y(); float gamma = barycentric.z();
			
			if (is_inside_triangle(alpha, beta, gamma))
			{
				
				int index = get_index(x, y, width, height);	//获取坐标在zbuffer坐标
				// z坐标透视校正,这里的z是view space的
				float z = -1.0 / (alpha / clipcoord_attri[0].w() + beta / clipcoord_attri[1].w() + gamma / clipcoord_attri[2].w());
				
				//方向光的shadow map是需要正交投影
				if (shader.payload.isShaderMapPass && shader.payload.light->getType() == DirectionalType) {
					float ProjectionDepth = 2.0 / shader.payload.light->getProjectionMat()[2][2];
					z = alpha * clipcoord_attri[0].z() + beta * clipcoord_attri[1].z() + gamma * clipcoord_attri[2].z();
					z = (z * -0.5 + 0.5) * ProjectionDepth;
				}
				
				if (zbuffer[index] > z)
				{
					zbuffer[index] = z;
					if (shader.payload.isShaderMapPass) continue;		//shadow map pass
					
					vec3 color = shader.fragment_shader(alpha, beta, gamma);

					for (int i = 0; i < 3; i++)
					{
						c[i] = (int)float_clamp(color[i], 0, 255);
					}
					set_color(framebuffer, x, y, width, height, c);
					
				}
			}
		}
	}
}


void draw_triangles(unsigned char* framebuffer, float* zbuffer, IShader& shader, int nface)
{
	//std::cout << typeid(shader).name() << std::endl;
	for (int i = 0; i < 3; i++)
	{
		shader.vertex_shader(nface, i);
	}
	
	int num_vertex = homo_clipping(shader.payload);// homogeneous clipping
	

	for (int i = 0; i < num_vertex - 2; i++) {// triangle assembly
		
		int index0 = 0;
		int index1 = i + 1;
		int index2 = i + 2;
		
		transform_attri(shader.payload, index0, index1, index2);// 顶点数据

		//lstThread.push_back(std::thread(rasterize_multhread, shader.payload.clipcoord_attri, framebuffer, zbuffer, std::ref(shader)));
		
		rasterize_singlethread(shader.payload.clipcoord_attri, framebuffer, zbuffer, shader);

	}
}


static bool is_back_facing(vec3 ndc_pos[3])
{
	vec3 a = ndc_pos[0];
	vec3 b = ndc_pos[1];
	vec3 c = ndc_pos[2];
	float signed_area = a.x() * b.y() - a.y() * b.x() +
		b.x() * c.y() - b.y() * c.x() +
		c.x() * a.y() - c.y() * a.x(); 
	return signed_area <= 0;
}

static vec3 compute_barycentric2D(float x, float y, const vec3* v)
{
	float c1 = (x * (v[1].y() - v[2].y()) + (v[2].x() - v[1].x()) * y + v[1].x() * v[2].y() - v[2].x() * v[1].y()) / (v[0].x() * (v[1].y() - v[2].y()) + (v[2].x() - v[1].x()) * v[0].y() + v[1].x() * v[2].y() - v[2].x() * v[1].y());
	float c2 = (x * (v[2].y() - v[0].y()) + (v[0].x() - v[2].x()) * y + v[2].x() * v[0].y() - v[0].x() * v[2].y()) / (v[1].x() * (v[2].y() - v[0].y()) + (v[0].x() - v[2].x()) * v[1].y() + v[2].x() * v[0].y() - v[0].x() * v[2].y());
	return vec3(c1, c2, 1 - c1 - c2);
}

static void set_color(unsigned char* framebuffer, int x, int y, int width, int height, unsigned char color[])
{
	int index = ((height - y - 1) * width + x) * 4; // the origin for pixel is bottom-left, but the framebuffer index counts from top-left
	for (int i = 0; i < 3; i++)
	{
		framebuffer[index + i] = color[i];
	}
		
}

static bool is_inside_triangle(float alpha, float beta, float gamma)
{
	if (alpha > -EPSILON && beta > -EPSILON && gamma > -EPSILON) return true;
	return false;
}

static int get_index(int x, int y, int width, int height)
{
	return (height - y - 1) * width + x;
}

static int is_inside_plane(clip_plane c_plane, vec4 vertex, payload_t& payload)
{	
	//ortho projection clip
	if (payload.isShaderMapPass && payload.light->getType() == DirectionalType) {
		switch (c_plane)
		{
		case W_PLANE:
			return 1;		
		case X_RIGHT:							
			return vertex.x() <= vertex.w();
		case X_LEFT:
			return vertex.x() >= -vertex.w();
		case Y_TOP:
			return vertex.y() <= vertex.w();
		case Y_BOTTOM:
			return vertex.y() >= -vertex.w();
		case Z_NEAR:
			return vertex.z() <= vertex.w();
		case Z_FAR:
			return vertex.z() >= -vertex.w();
		default:
			return 0;
		}
	}
	//perspect projection clip
	switch (c_plane)
	{
	case W_PLANE:
		return vertex.w() <= -EPSILON;		//保留w>0的点，防止裁剪之后的透视除法阶段出现除以0的错误
	case X_RIGHT:							//这个模型中所有的w值都是负值
		return vertex.x() >= vertex.w();
	case X_LEFT:
		return vertex.x() <= -vertex.w();
	case Y_TOP:
		return vertex.y() >= vertex.w();
	case Y_BOTTOM:
		return vertex.y() <= -vertex.w();
	case Z_NEAR:
		return vertex.z() >= vertex.w();
	case Z_FAR:
		return vertex.z() <= -vertex.w();
	default:
		return 0;
	}
}

static float get_intersect_ratio(vec4 prev, vec4 curv, clip_plane c_plane)
{
	switch (c_plane)
	{
	case W_PLANE:
		return (prev.w() + EPSILON) / (prev.w() - curv.w());
	case X_RIGHT:
		return (prev.w() - prev.x()) / ((prev.w() - prev.x()) - (curv.w() - curv.x()));
	case X_LEFT:
		return (prev.w() + prev.x()) / ((prev.w() + prev.x()) - (curv.w() + curv.x()));
	case Y_TOP:
		return (prev.w() - prev.y()) / ((prev.w() - prev.y()) - (curv.w() - curv.y()));
	case Y_BOTTOM:
		return (prev.w() + prev.y()) / ((prev.w() + prev.y()) - (curv.w() + curv.y()));
	case Z_NEAR:
		return (prev.w() - prev.z()) / ((prev.w() - prev.z()) - (curv.w() - curv.z()));
	case Z_FAR:
		return (prev.w() + prev.z()) / ((prev.w() + prev.z()) - (curv.w() + curv.z()));
	default:
		return 0;
	}
}

static int clip_with_plane(clip_plane c_plane, int num_vert, payload_t& payload)
{
	int out_vert_num = 0;
	int previous_index, current_index;
	int is_odd = (c_plane + 1) % 2; //1 0 1 0 1 0 1

	// 裁剪平面输入、输出定点列表指针，输出顶点在tmp1和tmp2之间交换存储。
	vec4* input_clipcoord = is_odd ? payload.tmp1_clipcoord : payload.tmp2_clipcoord;
	vec3* input_worldcoord = is_odd ? payload.tmp1_worldcoord : payload.tmp2_worldcoord;
	vec3* input_normal = is_odd ? payload.tmp1_normal : payload.tmp2_normal;
	vec2* input_uv = is_odd ? payload.tmp1_uv : payload.tmp2_uv;
	vec4* output_clipcoord = is_odd ? payload.tmp2_clipcoord : payload.tmp1_clipcoord;
	vec3* output_worldcoord = is_odd ? payload.tmp2_worldcoord : payload.tmp1_worldcoord;
	vec3* output_normal = is_odd ? payload.tmp2_normal : payload.tmp1_normal;
	vec2* output_uv = is_odd ? payload.tmp2_uv : payload.tmp1_uv;

	// tranverse all the edges from first vertex
	for (int i = 0; i < num_vert; i++)
	{
		
		current_index = i;
		previous_index = (i - 1 + num_vert) % num_vert;	//上一个点
		vec4 cur_vertex = input_clipcoord[current_index];//线终止点
		vec4 pre_vertex = input_clipcoord[previous_index];//线起始点

		int is_cur_inside = is_inside_plane(c_plane, cur_vertex, payload);
		int is_pre_inside = is_inside_plane(c_plane, pre_vertex, payload);

		if (is_cur_inside != is_pre_inside)	//两个点分布在平面两侧，则与平面交点属于内部点
		{
			float ratio = get_intersect_ratio(pre_vertex, cur_vertex, c_plane);	//获取与平面交点的坐标比例（相对于两点之间的线段）
			output_clipcoord[out_vert_num] = vec4_lerp(pre_vertex, cur_vertex, ratio);//插值计算平面上分割点的参数，该分割点属于输出点
			output_worldcoord[out_vert_num] = vec3_lerp(input_worldcoord[previous_index], input_worldcoord[current_index], ratio);
			output_normal[out_vert_num] = vec3_lerp(input_normal[previous_index], input_normal[current_index], ratio);
			output_uv[out_vert_num] = vec2_lerp(input_uv[previous_index], input_uv[current_index], ratio);

			out_vert_num++;
		}

		if (is_cur_inside)					//终止点在内侧，则该终点属于内部点
		{
			output_clipcoord[out_vert_num] = cur_vertex;
			output_worldcoord[out_vert_num] = input_worldcoord[current_index];
			output_normal[out_vert_num] = input_normal[current_index];
			output_uv[out_vert_num] = input_uv[current_index];

			out_vert_num++;
		}
	}

	return out_vert_num;
}

static int homo_clipping(payload_t& payload)
{
	int num_vertex = 3;
	num_vertex = clip_with_plane(W_PLANE, num_vertex, payload); 	
	num_vertex = clip_with_plane(X_RIGHT, num_vertex, payload);
	num_vertex = clip_with_plane(X_LEFT, num_vertex, payload);	
	num_vertex = clip_with_plane(Y_TOP, num_vertex, payload);	
	num_vertex = clip_with_plane(Y_BOTTOM, num_vertex, payload);
	num_vertex = clip_with_plane(Z_NEAR, num_vertex, payload);	
	num_vertex = clip_with_plane(Z_FAR, num_vertex, payload);	
	return num_vertex;
}

static void transform_attri(payload_t& payload, int index0, int index1, int index2)
{
	payload.clipcoord_attri[0] = payload.tmp2_clipcoord[index0];
	payload.clipcoord_attri[1] = payload.tmp2_clipcoord[index1];
	payload.clipcoord_attri[2] = payload.tmp2_clipcoord[index2];
	payload.worldcoord_attri[0] = payload.tmp2_worldcoord[index0];
	payload.worldcoord_attri[1] = payload.tmp2_worldcoord[index1];
	payload.worldcoord_attri[2] = payload.tmp2_worldcoord[index2];
	payload.normal_attri[0] = payload.tmp2_normal[index0];
	payload.normal_attri[1] = payload.tmp2_normal[index1];
	payload.normal_attri[2] = payload.tmp2_normal[index2];
	payload.uv_attri[0] = payload.tmp2_uv[index0];
	payload.uv_attri[1] = payload.tmp2_uv[index1];
	payload.uv_attri[2] = payload.tmp2_uv[index2];
}

