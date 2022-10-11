#include "../public/Win.h"
#include "cassert"
#include "cstdio"
#include "string"

window_t* window = NULL;

static LRESULT CALLBACK msg_callback(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)	//输入事件，每帧调用
{
	switch (msg)
	{
	case WM_CLOSE:
		window->is_close = true;
		break;
	case WM_KEYDOWN:
		window->keys[wParam & 511] = 1;
		break;
	case WM_KEYUP:
		window->keys[wParam & 511] = 0;
		break;
	case WM_LBUTTONDOWN:
		window->mouse_info.orbit_pos = get_mouse_pos();	//左键按下时每帧获取鼠标位置，更新相机环绕角度
		window->buttons[0] = 1; break;
	case WM_LBUTTONUP:
		window->buttons[0] = 0;
		break;
	case WM_RBUTTONDOWN:
		window->mouse_info.fv_pos = get_mouse_pos();	//右键按下时每帧获取鼠标位置，更新相机空间位置
		window->buttons[1] = 1;
		break;
	case WM_RBUTTONUP:
		window->buttons[1] = 0;
		break;
	case WM_MOUSEWHEEL:									//接受滚轮旋转角度
		window->mouse_info.wheel_delta = GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
		break;

	default: return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

static void register_window_class()							//注册窗口函数
{
	ATOM atom;												//初始化结构体
	WNDCLASS wc;											//
	wc.style = CS_BYTEALIGNCLIENT;							//窗口风格
	wc.lpfnWndProc = (WNDPROC)msg_callback;					//回调函数
	wc.cbClsExtra = 0;										//紧跟在窗口类尾部的一块额外空间，不用则设为0
	wc.cbWndExtra = 0;										//紧跟在窗口对象尾部的一块额外空间，不用则设为0
	wc.hInstance = GetModuleHandle(NULL);					//当前对象句柄
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);				//任务栏图标
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);				//光标样式
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);	//背景样式
	wc.lpszMenuName = NULL;									// 菜单
	wc.lpszClassName = ("Sparrow");							//该窗口类的名字,vs19可能需要在项目-项目名属性-高级-字符集-使用多字节字符集

	atom = RegisterClass(&wc);								//构造注册窗口对象
	assert(atom != 0);
}

static void init_bm_header(BITMAPINFOHEADER& bi, int width, int height)	//初始化位图头格式
{
	memset(&bi, 0, sizeof(BITMAPINFOHEADER));
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = width;
	bi.biHeight = -height;									//从上到下
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = width * height * 4;					//位图尺寸大小
}

int window_init(int width, int height, const char* title)
{
	window = (window_t*)malloc(sizeof(window_t));			//申请窗口格式
	memset(window, 0, sizeof(window_t));					//初始化窗口格式为0
	window->is_close = false;								

	RECT rect = { 0, 0, width, height };					//设置显示矩形范围 左上右下
	int wx, wy, sx, sy;										
	LPVOID ptr;												//LPVOID是没有类型的指针，即空类型指针
	HDC hDC;												//设备句柄
	BITMAPINFOHEADER bi;									//位图头格式

	register_window_class();								//调用注册函数，构造注册窗口对象
	
	window->h_window = CreateWindow(("Sparrow"), title,			//创建窗口，参数：指向的窗口类名，显示的窗口标题，窗口风格，初始位置和宽高
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,//父窗口句柄，菜单句柄，模块实例句柄，参数指针
		0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
	assert(window->h_window != NULL);
	
	init_bm_header(bi, width, height);						//初始化位图头格式
	
	hDC = GetDC(window->h_window);							//从窗口句柄中提取设备环境的句柄
	window->mem_dc = CreateCompatibleDC(hDC);				//创建一个与设备兼容的，内存设备环境mem_dc句柄并返回
	ReleaseDC(window->h_window, hDC);						//释放设备环境
	
	//创建设备无关位图句柄，CreateDIBSection参数：设备环境句柄，位图头格式，属性，指向位图位数据的指针，文件映射对象句柄，文件映射偏移
	window->bm_dib = CreateDIBSection(window->mem_dc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, &ptr, 0, 0); 
	assert(window->bm_dib != NULL);

	window->bm_old = (HBITMAP)SelectObject(window->mem_dc, window->bm_dib);//把新创建的位图句柄写入mem_dc，并替换同类型的旧位图
	window->window_fb = (unsigned char*)ptr;

	window->width = width;
	window->height = height;

	//GetWindowLong传入窗口句柄，获取GWL_STYLE（窗口风格）信息，参考：https://blog.csdn.net/hnhyhongmingjiang/article/details/2154410
	//AdjustWindowRect，依据客户矩形rect大小，调整客户窗口大小。注意显示矩形尺寸和窗口尺寸不同。
	AdjustWindowRect(&rect, GetWindowLong(window->h_window, GWL_STYLE), 0);	
	wx = rect.right - rect.left;				// 计算出窗口宽度和高度
	wy = rect.bottom - rect.top;
	sx = (GetSystemMetrics(SM_CXSCREEN) - wx) / 2; // GetSystemMetrics(SM_CXSCREEN)获取屏幕的宽度，SM_CYSCREEN获取屏幕的高度的
	sy = (GetSystemMetrics(SM_CYSCREEN) - wy) / 2; // 计算出窗口中心位置
	if (sy < 0) sy = 0;
	//设置窗口尺寸，参数：窗口句柄、标识顺序、左边界、有边界、宽度、高度、属性标志，参考：https://baike.baidu.com/item/SetWindowPos/6376849?fr=aladdin#2
	SetWindowPos(window->h_window, NULL, sx, sy, wx, wy, (SWP_NOCOPYBITS | SWP_NOZORDER | SWP_SHOWWINDOW));
	SetForegroundWindow(window->h_window);		//设置前台窗口
	ShowWindow(window->h_window, SW_NORMAL);	//显示窗口

	msg_dispatch();								//消息分派

	memset(window->window_fb, 0, width * height * 4);	//初始化位图内存数据
	memset(window->keys, 0, sizeof(char) * 512);		//初始化按键内存数据
	return 0;
}

int window_destroy()
{
	if (window->mem_dc)
	{
		if (window->bm_old)
		{
			SelectObject(window->mem_dc, window->bm_old); // 写入原来的bitmap，才能释放DC！
			window->bm_old = NULL;
		}
		DeleteDC(window->mem_dc);
		window->mem_dc = NULL;
	}
	if (window->bm_dib)
	{
		DeleteObject(window->bm_dib);
		window->bm_dib = NULL;
	}
	if (window->h_window)
	{
		CloseWindow(window->h_window);
		window->h_window = NULL;
	}

	free(window);
	return 0;
}

void msg_dispatch()
{
	MSG msg;
	while (1)
	{
		// Peek不阻塞，Get会阻塞，PM_NOREMOVE表示如果有消息不处理（留给接下来的Get处理）
		if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) break; //没消息就溜，确定有消息再用Get
		if (!GetMessage(&msg, NULL, 0, 0)) break;

		TranslateMessage(&msg);	 //转换消息 虚拟按键->字符
		DispatchMessage(&msg); //传送消息给回调
	}
}

static void window_display(int displayFps)
{
	LOGFONT logfont;
	ZeroMemory(&logfont, sizeof(LOGFONT));
	logfont.lfCharSet = ANSI_CHARSET;
	logfont.lfHeight = 15; //设置字体的大小
	HFONT hFont = CreateFontIndirect(&logfont);
	std::string fps = "FPS: " + std::to_string(displayFps);
	HDC hDC = GetDC(window->h_window);
	//目标举行的左上角(x,y), 宽度，高度，上下文指针
	SelectObject(window->mem_dc, hFont);
	SetTextColor(window->mem_dc, RGB(190, 190, 190));
	SetBkColor(window->mem_dc, RGB(80, 80, 80));
	TextOut(window->mem_dc, 20, 10, "Keyboard: W/S/A/D/Q/E", strlen("Keyboard: W/S/A/D/Q/E"));
	TextOut(window->mem_dc, 20, 30, "Mouse: Left button to rotate, right button to move，scroll wheel zoom", strlen("Mouse: Left button to rotate, right button to move，scroll wheel zoom"));
	TextOut(window->mem_dc, 20, 50, fps.c_str(), strlen(fps.c_str()));
	// 把兼容性DC的数据传到真正的DC上
	BitBlt(hDC, 0, 0, window->width, window->height, window->mem_dc, 0, 0, SRCCOPY);
	ReleaseDC(window->h_window, hDC);
}

void window_draw_depthMap(float* DepthMap, int displayFps)
{
	for (int i = 0; i < window->height; i++)
	{
		for (int j = 0; j < window->width; j++)
		{
			int index = i * window->width + j;
			window->window_fb[index * 4] = DepthMap[index];
			window->window_fb[index * 4 + 1] = DepthMap[index];
			window->window_fb[index * 4 + 2] = DepthMap[index];
		}
	}
	window_display(displayFps);
}

void window_draw(unsigned char* framebuffer, int displayFps)	//绘制图像，将帧缓存内容复制到位图对应位置
{
	for (int i = 0; i < window->height; i++)
	{
		for (int j = 0; j < window->width; j++)
		{
			int index = (i * window->width + j) * 4;
			window->window_fb[index] = framebuffer[index];
			window->window_fb[index + 1] = framebuffer[index + 1];
			window->window_fb[index + 2] = framebuffer[index + 2];
		}
	}
	window_display(displayFps);
}

vec2 get_mouse_pos()
{
	POINT point;							//鼠标坐标
	GetCursorPos(&point);
	ScreenToClient(window->h_window, &point); // 从屏幕空间转到窗口空间
	return vec2((float)point.x, (float)point.y);
}

static double get_native_time(void) {	
	static double period = -1;
	LARGE_INTEGER counter;
	if (period < 0) {
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);	//获取时钟频率
		period = 1 / (double)frequency.QuadPart;
	}
	QueryPerformanceCounter(&counter);
	return counter.QuadPart * period;
}

float platform_get_time(void) {			//获取平台启动时间
	static double initial = -1;
	if (initial < 0) {
		initial = get_native_time();
	}
	return (float)(get_native_time() - initial);
}
