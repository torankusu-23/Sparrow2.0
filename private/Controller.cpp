#include "../public/Controller.h"
#include "../public/Win.h"


void handle_mouse_events(Camera& camera)
{
	if (window->buttons[0])
	{
		vec2 cur_pos = get_mouse_pos();
		window->mouse_info.orbit_delta = window->mouse_info.orbit_pos - cur_pos;
		window->mouse_info.orbit_pos = cur_pos;
	}

	if (window->buttons[1])
	{
		vec2 cur_pos = get_mouse_pos();
		window->mouse_info.fv_delta = window->mouse_info.fv_pos - cur_pos;
		window->mouse_info.fv_pos = cur_pos;
	}

	updata_camera(camera);
}
//W\S\Q\E\A\D\ESC
void handle_key_events(Camera& camera)
{
	float distance = (camera.target - camera.pos).norm();

	if (window->keys['W'])
	{
		camera.pos = camera.pos + (-10.0) / window->width * camera.z * distance;
		//camera.pos = camera.pos + (-0.5f) * camera.z;
		//camera.target = camera.target + (-0.5f) * camera.z;
	}
	if (window->keys['S'])
	{
		camera.pos = camera.pos + 0.5f * camera.z;
		//camera.target = camera.target + 0.5f * camera.z;
	}
	if (window->keys['Q'])
	{
		camera.pos = camera.pos + 0.5f * camera.y;
		camera.target = camera.target + 0.5f * camera.y;
	}
	if (window->keys['E'])
	{
		camera.pos = camera.pos + (-0.5f) * camera.y;
		camera.target = camera.target + (-0.5f) * camera.y;
	}
	if (window->keys['A'])
	{
		camera.pos = camera.pos + (-0.5f) * camera.x;
		camera.target = camera.target + (-0.5f) * camera.x;
	}
	if (window->keys['D'])
	{
		camera.pos = camera.pos + 0.5f * camera.x;
		camera.target = camera.target + 0.5f * camera.x;
	}
	if (window->keys[VK_ESCAPE])
	{
		window->is_close = 1;
	}
}