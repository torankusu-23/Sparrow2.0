#pragma once
#include "tools.h"
#include "windows.h"		//调用系统库

typedef struct mouse	
{
	vec2 orbit_pos;		// 相机环绕位置
	vec2 orbit_delta;	
	vec2 fv_pos;		// 第一人称位置
	vec2 fv_delta;
	float wheel_delta;	
}mouse_t;

typedef struct window	
{
	HWND h_window;		//窗口句柄
	HDC mem_dc;			//dc句柄
	HBITMAP bm_old;		//旧的位图句柄	
	HBITMAP bm_dib;		//设备无关的位图句柄
	unsigned char* window_fb;	//指向位图数据的指针
	int width;
	int height;
	char keys[512];		
	char buttons[2];	
	bool is_close;
	mouse_t mouse_info;
}window_t;

extern window_t* window;

int window_init(int width, int height, const char* title);
int window_destroy();		
void window_draw_depthMap(float* DepthMap , int displayFps);
void window_draw(unsigned char* framebuffer, int displayFps);
void msg_dispatch();			
vec2 get_mouse_pos();		
float platform_get_time(void);