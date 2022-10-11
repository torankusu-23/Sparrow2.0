#pragma once
#include "tools.h"
#include "struct.h"

class Camera
{
public:
	Camera(vec3 pos, vec3 target, vec3 up);
	~Camera();

	vec3 pos;
	vec3 target;
	vec3 up;
	vec3 x;
	vec3 y;
	vec3 z;
};

void updata_camera(Camera& camera);		//��������ӽ�
void handle_events(Camera& camera);		//ÿ֡�����á����������������ü����¼�����������¼�����������ӽ