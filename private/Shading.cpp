#include "../shader/shader.h"
#include "../public/Sample.h"
#include "stdlib.h"

static vec3 cal_normal(vec3& normal, vec3* world_coords, const vec2* uvs, const vec2& uv, Image* normal_map);


void PhongShader::vertex_shader(int nfaces, int nvertex)
{	
	//获取面的v，vn，uv属性,加入点属性和裁剪点属性
	payload.tmp1_uv[nvertex] = payload.uv_attri[nvertex] = payload.model->uv_of_facei(nfaces, nvertex);
	payload.tmp1_clipcoord[nvertex] = payload.clipcoord_attri[nvertex] = payload.mvp_matrix * up_to_vec4(payload.model->v_of_facei(nfaces, nvertex), 1.0f);
	payload.tmp1_worldcoord[nvertex] = payload.worldcoord_attri[nvertex] = payload.model->v_of_facei(nfaces, nvertex);
	payload.tmp1_normal[nvertex] = payload.normal_attri[nvertex] = payload.model->vn_of_facei(nfaces, nvertex);
}


vec3 PhongShader::fragment_shader(float alpha, float beta, float gamma)
{
	vec4* clip_coords = payload.clipcoord_attri;
	vec3* world_coords = payload.worldcoord_attri;
	vec3* normals = payload.normal_attri;
	vec2* uvs = payload.uv_attri;
	
	// interpolate attribute 透视校正，注意Z_view = - W_clip，理论上Z会带一个负号，这里的负号抵消了
	float Z = 1.0 / (alpha / clip_coords[0].w() + beta / clip_coords[1].w() + gamma / clip_coords[2].w());
	vec3 normal = (alpha*normals[0] / clip_coords[0].w() + beta * normals[1] / clip_coords[1].w() +
		gamma * normals[2] / clip_coords[2].w()) * Z;
	vec2 uv = (alpha*uvs[0] / clip_coords[0].w() + beta * uvs[1] / clip_coords[1].w() +
		gamma * uvs[2] / clip_coords[2].w()) * Z;
	vec3 worldpos = (alpha*world_coords[0] / clip_coords[0].w() + beta * world_coords[1] / clip_coords[1].w() +
		gamma * world_coords[2] / clip_coords[2].w()) * Z;
	float shadow = 1.f;
	
	// shadow map compare
	if (payload.light != nullptr) {
		vec3 LightNDC;
		vec4 LightClipCoords = payload.light->getProjectionMat() * (payload.light->getViewMat() * up_to_vec4(worldpos, 1.f));
		
		LightNDC[0] = LightClipCoords[0] / LightClipCoords.w();
		LightNDC[1] = LightClipCoords[1] / LightClipCoords.w();
		int LightU = (LightNDC[0] * 0.5f + 0.5f) * (SHADOW_SIZE - 1);
		int LightV = (LightNDC[1] * 0.5f + 0.5f) * (SHADOW_SIZE - 1);
		
		//solve z-fighting according to Similarity between light ray and normal
		float Similarity = dot(normal, (payload.light->getpos() - worldpos).normalize());
		float zbias = Similarity > 0.f ? 1.f - Similarity : 0.f;
		float DistanceFromLight;
		float* ShadowMap = payload.light->GetShadowMap();

		
		LightType type = payload.light->getType();
		if (type == PointType) {
			//point light don't have soft shadow
			DistanceFromLight = -LightClipCoords.w() - 0.3f - zbias * 0.2f;
			if (Similarity <= 0) DistanceFromLight = -LightClipCoords.w();
			shadow = payload.light->getMapVal(LightU, LightV) > DistanceFromLight ? 1.f : 0.f;
		}
		else if (type == DirectionalType) {
			float ProjectionDepth = 2.0 / payload.light->getProjectionMat()[2][2];
			DistanceFromLight = (LightClipCoords[2] * -0.5 + 0.5) * ProjectionDepth;
			if (Similarity <= 0) DistanceFromLight = LightClipCoords[2];
			float PenumbraSize = 0;
			float blockDepth = 0;
			//float AverageBlockDepth = payload.light->getPcssAverageBlockDepth(LightU, LightV, DistanceFromLight, payload.samples);
			float VssmAverageBlockDepth = payload.light->getVssmPcssAverageBlockDepth(LightU, LightV, DistanceFromLight);

			blockDepth = VssmAverageBlockDepth;
			if (blockDepth > 0.2f) {
				PenumbraSize = (DistanceFromLight - blockDepth) / blockDepth *(2.0 / payload.light->getProjectionMat()[0][0]);
			}
			
			int PcfKernel = PenumbraSize * 2;
			
			float VsmVisbility = payload.light->getVssmPcfVal(LightU, LightV, PcfKernel, DistanceFromLight -0.01f - zbias * 0.2f);
			//float Visbility = payload.light->getPcfVal(LightU, LightV, PcfKernel, DistanceFromLight - 0.3f - zbias * 0.5f, payload.samples);
			shadow = VsmVisbility;
		}
	}
	
	//return vec3();
	if (payload.model->normal_map)	//法线贴图修正
		normal = cal_normal(normal, world_coords, uvs, uv, payload.model->normal_map).normalize();

	//phong model
	//环境光、漫反射光、镜面反射光参数
	vec3 ka(0.35, 0.35, 0.35);
	vec3 kd = payload.model->get_diffusemap(uv);
	vec3 ks(0.8, 0.8, 0.8);

	float glossy = 150.f;
	vec3 lightRay;
	switch (payload.light->getType()) {
	case PointType:
		lightRay = (payload.light->getpos() - worldpos).normalize();
		break;
	case DirectionalType:
		lightRay = vec3(0, 0, 0) - (payload.light->getLightDir());
		break;
	default:
		break;
	}
	
	vec3 light_ambient_intensity = kd;
	vec3 light_diffuse_intensity = vec3(0.9, 0.9, 0.9);
	vec3 light_specular_intensity = vec3(0.25, 0.25, 0.25);

	vec3 v = (payload.camera->pos - worldpos).normalize();
	vec3 h = (lightRay + v).normalize();

	vec3 ambient = ka * light_ambient_intensity;	//简易环境光，没有计算环境光遮蔽
	vec3 diffuse = (kd * light_diffuse_intensity) * fmax(0, dot(lightRay, normal));	//纹理信息也埋在里面了，可以分离
	vec3 specular = (ks * light_specular_intensity) * fmax(0, pow(dot(normal, h), glossy));	//镜面反射

	vec3 result_color = (ambient + shadow * (diffuse + specular)) * 255.f;
	return result_color;
}


void SkyboxShader::vertex_shader(int nfaces, int nvertex)	//same as phong model
{
	//获取面的v，vn，uv属性,加入点属性和输入点属性（用于裁剪）
	payload.tmp1_uv[nvertex] = payload.uv_attri[nvertex] = payload.model->uv_of_facei(nfaces, nvertex);
	payload.tmp1_clipcoord[nvertex] = payload.clipcoord_attri[nvertex] = payload.mvp_matrix * up_to_vec4(payload.model->v_of_facei(nfaces, nvertex), 1.0f);
	payload.tmp1_worldcoord[nvertex] = payload.worldcoord_attri[nvertex] = payload.model->v_of_facei(nfaces, nvertex);
	payload.tmp1_normal[nvertex] = payload.normal_attri[nvertex] = payload.model->vn_of_facei(nfaces, nvertex);
}


vec3 SkyboxShader::fragment_shader(float alpha, float beta, float gamma)
{
	vec4* clip_coords = payload.clipcoord_attri;
	vec3* world_coords = payload.worldcoord_attri;
	// world coordinate persective recorrection
	float Z = 1.0 / (alpha / clip_coords[0].w() + beta / clip_coords[1].w() + gamma / clip_coords[2].w());
	vec3 worldpos = (alpha * world_coords[0] / clip_coords[0].w() + beta * world_coords[1] / clip_coords[1].w() + gamma * world_coords[2] / clip_coords[2].w()) * Z;

	vec3 result_color = cubemap_sampling(worldpos, payload.model->environment_map);//ͨ通过世界坐标，获取环境贴图颜色
	
	return result_color * 255.f;
}



static vec3 cal_normal(vec3& normal, vec3* world_coords, const vec2* uvs, const vec2& uv, Image* normal_map)
{
	// calculate the difference in UV coordinate
	float x1 = uvs[1][0] - uvs[0][0];
	float y1 = uvs[1][1] - uvs[0][1];
	float x2 = uvs[2][0] - uvs[0][0];
	float y2 = uvs[2][1] - uvs[0][1];
	float det = (x1 * y2 - x2 * y1);

	// calculate the difference in world pos
	vec3 e1 = world_coords[1] - world_coords[0];
	vec3 e2 = world_coords[2] - world_coords[0];

	vec3 t = e1 * y2 + e2 * (-y1);
	vec3 b = e1 * (-x2) + e2 * x1;
	t = t / det;
	b = b / det;

	// schmidt orthogonalization
	normal = normal.normalize();
	t = (t - dot(t, normal) * normal).normalize();
	b = (b - dot(b, normal) * normal - dot(b, t) * t).normalize();

	vec3 sample = texture_sample(uv, normal_map);
	// modify the range from 0 ~ 1 to -1 ~ +1
	sample = vec3(sample[0] * 2 - 1, sample[1] * 2 - 1, sample[2] * 2 - 1);

	vec3 normal_new = t * sample[0] + b * sample[1] + normal * sample[2];
	return normal_new;
}