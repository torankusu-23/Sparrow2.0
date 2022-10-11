#pragma once
#include "iostream"
#include "public/Model.h"
#include "public/Win.h"
#include "public/Camera.h"
#include "public/Light.h"
#include "public/Controller.h"
#include "public/Pipeline.h"
#include "public/Scene.h"
#include <ctime>
#include "winuser.rh"
#include<typeinfo>
#include<thread>
#include<list>
#include<mutex>

const vec3 Up(0, 1, 0);
const vec3 CameraPos(10, 10, 30);
const vec3 LightPos(25, 40, 20);
const float Exposure = 300.0f;
const vec3 Target(0, 1, 0);

void update_matrix(Camera& camera, mat4 view_mat, mat4 perspective_mat, IShader* shader_model, IShader* shader_skybox);

int main()
{
	short width = WINDOW_WIDTH, height = WINDOW_HEIGHT;
	float* zbuffer = new float[width * height];
	unsigned char* framebuffer = new unsigned char[width * height * 4];
	memset(framebuffer, 0, sizeof(unsigned char) * width * height * 4);

	Camera camera(CameraPos, Target, Up);
	Camera LightCam(LightPos, Target, Up);
	DirectionalLight* dlight = new DirectionalLight(LightPos, &LightCam, Exposure);
	Light* light = dlight;
	
	mat4 model_mat;		
	model_mat.identity();												
	mat4 view_mat = Model_View(camera.pos, camera.target, camera.up);
	mat4 perspective_mat = mat4_perspective(60, (float)(width) / height, -0.1, -10000);

	//srand((unsigned int)time(NULL));									
	
	int model_num = 0;
	Model* model[MAX_MODEL_NUM];										
	IShader* shader_model;
	IShader* shader_skybox;
	PoissonSamples* Samples = new PoissonSamples();
	
	std::string name;
	std::cout << "请输入obj文件中模型的名字:" << std::endl;
	std::cin >> name;
	//name = "mihoyo";
	//skybox mode has been banned
	build_scene(name, model, model_num, &shader_model, &shader_skybox, perspective_mat, &camera);//加载scene
	if (model_num == 0) {
		system("pause");
		return 0;
	} 
	//system("pause");
	window_init(width, height, "Sparrow");								
	
	float print_time = platform_get_time();								
	
	//precompute shadow map pass
	clear_zbuffer(SHADOW_SIZE, SHADOW_SIZE, light->GetShadowMap());
	mat4 light_view_mat = Model_View(light->GetCamera()->pos, light->GetCamera()->target, light->GetCamera()->up);	// View mat
	mat4 light_projection_mat;								//͸Projection mat
	switch (light->getType()) {
		case PointType: light_projection_mat = mat4_perspective(60, (float)(SHADOW_SIZE) / SHADOW_SIZE, -0.1, -10000); break;
		case DirectionalType:light_projection_mat = mat4_ortho(-20, 20, -20, 20, -0.1, -10000); Samples->Init(1000000); break;
		default:break;
	}
	update_matrix(*light->GetCamera(), light_view_mat, light_projection_mat, shader_model, nullptr);
	light->setViewMat(light_view_mat);
	light->setProjectionMat(light_projection_mat);

	for (int m = 0; m < model_num; m++)//model_num
	{
		
		if (model[m]->is_skybox) continue;
		
		shader_model->payload.model = model[m];	
		shader_model->payload.light = light;
		shader_model->payload.height = SHADOW_SIZE;
		shader_model->payload.width = SHADOW_SIZE;
		shader_model->payload.isShaderMapPass = true;
		IShader* shader;
		shader = shader_model;
		
		for (int i = 0; i < model[m]->number_of_faces(); i++)
		{	
			draw_triangles(framebuffer, light->GetShadowMap(), *shader, i);
		}
	}
	light->InitSATMap();
	light->InitSquareSATMap();
	
	std::cout << "shadow map has been ready" << std::endl;

	//update
	while (!window->is_close)											
	{
		clear_framebuffer(width, height, framebuffer);					
		clear_zbuffer(width, height, zbuffer);							
		
		handle_events(camera);											//更新相机坐标→键盘事件→鼠标事件→更新相机视角

		update_matrix(camera, view_mat, perspective_mat, shader_model, shader_skybox);
		
		for (int m = 0; m < model_num; m++)								//forward rendering 遍历模型
		{
			shader_model->payload.model = model[m];								//normal mode
			shader_model->payload.light = light;
			shader_model->payload.width = width;
			shader_model->payload.height = height;
			shader_model->payload.isShaderMapPass = false;
			shader_model->payload.samples = Samples;
			if (shader_skybox != nullptr) {
				shader_skybox->payload.model = model[m];	//skybox mode
				//shader_model->payload.light = light;
				shader_skybox->payload.width = width;
				shader_skybox->payload.height = height;
				shader_skybox->payload.isShaderMapPass = false;
			}
			std::list<std::thread> lstThread;
			IShader* shader;
			if (model[m]->is_skybox)								
				shader = shader_skybox;
			else
				shader = shader_model;
			for (int i = 0; i < model[m]->number_of_faces(); i++)	
			{
				//phongShader更新多线程，skybox暂时没有动
				//因为要多线程，所以要深拷贝shader，本质上是PhongShader，所以需要类型转换(注意安全)，再执行拷贝，此时同一个model的每一个triangle都有一套独立的数据，线程间不会干扰
				//仅mihoyo场景能用，因为线程数量过多就没意义
				/*
				if (typeid(*shader) == typeid(PhongShader)) {
					//std::cout << typeid(*mtShader).name() << std::endl;
					PhongShader* tmpShader = dynamic_cast<PhongShader*>(shader);
					PhongShader* mtShader = new PhongShader(*tmpShader);
					lstThread.push_back(std::thread(draw_triangles, framebuffer, zbuffer, std::ref(*mtShader), i));
				} else
					draw_triangles(framebuffer, zbuffer, *shader, i);*/
				draw_triangles(framebuffer, zbuffer, *shader, i);
			}
			
			for (auto& th : lstThread)
			{
				th.join();
			}
		}

		float curr_time = platform_get_time();
		//统计帧率										
		int displayFps = 1.f / (curr_time - print_time);
		print_time = curr_time;

		window->mouse_info.wheel_delta = 0;
		window->mouse_info.orbit_delta = vec2(0, 0);
		window->mouse_info.fv_delta = vec2(0, 0);
		
		//window_draw_depthMap(light->GetShadowMap(), displayFps);									//draw depth 
		window_draw(framebuffer, displayFps);											//draw frame
		msg_dispatch();
	}
	//释放内存
	for (int i = 0; i < model_num; i++)
		if (model[i] != NULL)  delete model[i];
	if (shader_model != NULL)  delete shader_model;
	if (shader_skybox != NULL) delete shader_skybox;
	delete [] zbuffer;
	delete [] framebuffer;
	window_destroy();

	system("pause");
	return 0;
}

void update_matrix(Camera& camera, mat4 view_mat, mat4 perspective_mat, IShader* shader_model, IShader* shader_skybox)
{
	view_mat = Model_View(camera.pos, camera.target, camera.up);
	mat4 mvp = perspective_mat * view_mat;
	shader_model->payload.camera_view_matrix = view_mat;
	shader_model->payload.mvp_matrix = mvp;

	if (shader_skybox != nullptr)
	{ 
		mat4 view_skybox = view_mat;
		view_skybox[0][3] = 0;
		view_skybox[1][3] = 0;
		view_skybox[2][3] = 0;
		shader_skybox->payload.camera_view_matrix = view_skybox;
		shader_skybox->payload.mvp_matrix = perspective_mat * view_skybox;
	}
}