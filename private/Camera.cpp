#pragma once
#include "../public/Camera.h"
#include "../public/Win.h"
#include "../public/Controller.h"

Camera::Camera(vec3 e, vec3 t, vec3 up) : pos(e), target(t), up(up) {}

Camera::~Camera() {}

void updata_camera(Camera& camera)
{
	vec3 target_to_camera = camera.pos - camera.target;			// Ŀ��㵽���������
	float radius = target_to_camera.norm();						// �����Ŀ��ľ��룬����ת�뾶

	float phi = (float)atan2(target_to_camera[0], target_to_camera[2]); // azimuth angle(��λ��), angle between target_to_camera and z-axis��[-pi, pi]
	float theta = (float)acos(target_to_camera[1] / radius);		  // zenith angle(�춥��), angle between target_to_camera and y-axis, [0, pi]
	float x_delta = window->mouse_info.orbit_delta[0] / window->width;
	float y_delta = window->mouse_info.orbit_delta[1] / window->height;

	radius *= (float)pow(0.95, window->mouse_info.wheel_delta);		//������ͨ������radius�����������Ŀ���ľ��롣����Ŀ��Խ�������ŷ���ԽС��

	float factor = 1.5 * PI;						//��������ٶȣ�������������ת����
	phi += x_delta * factor;						
	theta += y_delta * factor;
	if (theta > PI) theta = PI - EPSILON * 100;		//����������ת�Ƕ�0~180
	if (theta < 0)  theta = EPSILON * 100;

	camera.pos[0] = camera.target[0] + radius * sin(phi) * sin(theta);
	camera.pos[1] = camera.target[1] + radius * cos(theta);
	camera.pos[2] = camera.target[2] + radius * sin(theta) * cos(phi);

	
	factor = radius * (float)tan(60.0 / 360 * PI) * 2.2;// ����Ҽ�����λ�ò���
	x_delta = window->mouse_info.fv_delta[0] / window->width;
	y_delta = window->mouse_info.fv_delta[1] / window->height;
	vec3 left = x_delta * factor * camera.x;
	vec3 up = y_delta * factor * camera.y;

	camera.pos = camera.pos + (left - up);
	camera.target = camera.target + (left - up);
}

void handle_events(Camera& camera)
{
	camera.z = (camera.pos - camera.target).normalize();	
	camera.x = cross(camera.up, camera.z).normalize();
	camera.y = cross(camera.z, camera.x).normalize();

	handle_key_events(camera);								
	handle_mouse_events(camera);							
}



