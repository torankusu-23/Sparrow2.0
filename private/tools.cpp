#include "../public/tools.h"
#include "stdio.h"

#define _PI_2  1.57079632679
#define _PI    3.14159265359
#define _3PI_2 4.71238898038
#define _2PI   6.28318530718

#define _round(x) ((x) >= 0 ? (long)((x) + 0.5) : (long)((x) - 0.5))

const int sine_array[200] = {
	0, 79, 158, 237, 316, 395, 473, 552, 631, 710,
	789, 867, 946, 1024, 1103, 1181, 1260, 1338, 1416, 1494,
	1572, 1650, 1728, 1806, 1883, 1961, 2038, 2115, 2192, 2269,
	2346, 2423, 2499, 2575, 2652, 2728, 2804, 2879, 2955, 3030,
	3105, 3180, 3255, 3329, 3404, 3478, 3552, 3625, 3699, 3772,
	3845, 3918, 3990, 4063, 4135, 4206, 4278, 4349, 4420, 4491,
	4561, 4631, 4701, 4770, 4840, 4909, 4977, 5046, 5113, 5181,
	5249, 5316, 5382, 5449, 5515, 5580, 5646, 5711, 5775, 5839,
	5903, 5967, 6030, 6093, 6155, 6217, 6279, 6340, 6401, 6461,
	6521, 6581, 6640, 6699, 6758, 6815, 6873, 6930, 6987, 7043,
	7099, 7154, 7209, 7264, 7318, 7371, 7424, 7477, 7529, 7581,
	7632, 7683, 7733, 7783, 7832, 7881, 7930, 7977, 8025, 8072,
	8118, 8164, 8209, 8254, 8298, 8342, 8385, 8428, 8470, 8512,
	8553, 8594, 8634, 8673, 8712, 8751, 8789, 8826, 8863, 8899,
	8935, 8970, 9005, 9039, 9072, 9105, 9138, 9169, 9201, 9231,
	9261, 9291, 9320, 9348, 9376, 9403, 9429, 9455, 9481, 9506,
	9530, 9554, 9577, 9599, 9621, 9642, 9663, 9683, 9702, 9721,
	9739, 9757, 9774, 9790, 9806, 9821, 9836, 9850, 9863, 9876,
	9888, 9899, 9910, 9920, 9930, 9939, 9947, 9955, 9962, 9969,
	9975, 9980, 9985, 9989, 9992, 9995, 9997, 9999, 10000, 10000
};

float _sin(float a) {
	if (a < _PI_2) {
		return 0.0001 * sine_array[_round(126.6873 * a)];
	}
	else if (a < _PI) {
		return 0.0001 * sine_array[398 - _round(126.6873 * a)];
	}
	else if (a < _3PI_2) {
		return -0.0001 * sine_array[-398 + _round(126.6873 * a)];
	}
	else {
		return -0.0001 * sine_array[796 - _round(126.6873 * a)];
	}
}

float _cos(float a) {
	float a_sin = a + _PI_2;
	a_sin = a_sin > _2PI ? a_sin - _2PI : a_sin;
	return _sin(a_sin);
}



mat4 mat4_translate(float tx, float ty, float tz)
{
	mat4 m;
	m.identity();
	m[0][3] = tx;
	m[1][3] = ty;
	m[2][3] = tz;
	return m;
}

mat4 mat4_scale(float sx, float sy, float sz)
{
	mat4 m;
	m.identity();
	m[0][0] = sx;
	m[1][1] = sy;
	m[2][2] = sz;
	return m;
}

/*
 * angle: the angle of rotation, in degrees
 *
 *  1  0  0  0
 *  0  c -s  0
 *  0  s  c  0
 *  0  0  0  1
 *
 */
mat4 mat4_rotate_x(float angle)
{
	mat4 m;
	m.identity();
	angle = angle / 180.0 * PI;
	float c = cos(angle);
	float s = sin(angle);
	m[1][1] = c;
	m[1][2] = -s;
	m[2][1] = s;
	m[2][2] = c;
	return m;

}

/*
 * angle: the angle of rotation, in degrees
 *
 *  c  0  s  0
 *  0  1  0  0
 * -s  0  c  0
 *  0  0  0  1
 *
 */
mat4 mat4_rotate_y(float angle)
{
	mat4 m;
	m.identity();
	angle = angle / 180.0 * PI;
	float c = cos(angle);
	float s = sin(angle);
	m[0][0] = c;
	m[0][2] = s;
	m[2][0] = -s;
	m[2][2] = c;
	return m;
}

/*
 * angle: the angle of rotation, in degrees
 *
 *  c -s  0  0
 *  s  c  0  0
 *  0  0  1  0
 *  0  0  0  1
 *
 */
mat4 mat4_rotate_z(float angle)
{
	mat4 m;
	m.identity();
	angle = angle / 180.0 * PI;
	float c = cos(angle);
	float s = sin(angle);
	m[0][0] = c;
	m[0][1] = -s;
	m[1][0] = s;
	m[1][1] = c;
	return m;
}


/*
 * x_axis.x  x_axis.y  x_axis.z  -dot(x_axis,eye)
 * y_axis.x  y_axis.y  y_axis.z  -dot(y_axis,eye)
 * z_axis.x  z_axis.y  z_axis.z  -dot(z_axis,eye)
 *        0         0         0                 1
 */
mat4 Model_View(vec3 eye, vec3 target, vec3 up)
{/*
	vec3 z =(eye - target).normalize();
	vec3 x = cross(up, z).normalize();
	vec3 y = cross(z, x).normalize();
	mat4 view;
	view.identity();

	for (int i = 0; i < 3; ++i) {	//0��1��2�д洢x,y,z�����꣬�����й̶�0001�������д洢һ��λ��
		view[0][i] = x[i];			//�ƶ������ԭ�����ƶ��������糡�����꣬��������ƶ���Ϊ��������һ����λ��Mt���ڶ�����ѡ��Mr
		view[1][i] = y[i];
		view[2][i] = z[i];
	}

	view[0][3] = -dot(x, eye);		//Mt * Mr�õ��ĵ����н�����ο� http://www.songho.ca/opengl/gl_camera.html
	view[1][3] = -dot(y, eye);
	view[2][3] = -dot(z, eye);

	return view;*/
	mat4 m;
	m.identity();

	vec3 z = (eye - target).normalize();
	vec3 x = cross(up, z).normalize();
	vec3 y = cross(z, x).normalize();
	//std::cout << eye - target << std::endl;
	//std::cout << cross(up, z) << std::endl;
	//std::cout << cross(z, x) << std::endl;

	m[0][0] = x[0];
	m[0][1] = x[1];
	m[0][2] = x[2];

	m[1][0] = y[0];
	m[1][1] = y[1];
	m[1][2] = y[2];

	m[2][0] = z[0];
	m[2][1] = z[1];
	m[2][2] = z[2];

	m[0][3] = -dot(x, eye); //�൱��ԭ��Ҫλ�Ƶģ����µ�����ϵ����λ�ƶ��٣��и��ı�
	m[1][3] = -dot(y, eye);
	m[2][3] = -dot(z, eye);

	return m;
}


/*
*	2 / (r - l)		0				0			- (r + l) / (r - l)
*		0			2 / (t - b)     0			- (t + b) / (t - b)
*		0			0				2/(n - f)	- (f + n) / (n - f)
*		0			0				0					1
*/
mat4 mat4_ortho(float left, float right, float bottom, float top, float near, float far)
{
	float x_range = right - left;
	float y_range = top - bottom;
	float z_range = near - far;
	mat4 m;
	m.identity();
	m[0][0] = 2 / x_range;
	m[1][1] = 2 / y_range;
	m[2][2] = 2 / z_range;
	m[0][3] = -(left + right) / x_range;
	m[1][3] = -(bottom + top) / y_range;
	m[2][3] = -(near + far) / z_range;
	return m;
}


/*
 * 1/(aspect*tan(fovy/2))              0             0           0
 *                      0  1/tan(fovy/2)             0           0
 *                      0              0  -(f+n)/(f-n)  -2fn/(f-n)
 *                      0              0            -1           0
 */
mat4 mat4_perspective(float fovy, float aspect, float near, float far)
{
	mat4 view;
	view.identity();
	fovy = fovy / 180.0 * PI;			
	float t = fabs(near) * tan(fovy / 2);	//��top
	float r = aspect * t;					//��right��aspect��ʾ���߱�

	view[0][0] = near / r;					//�Ƶ�  http://www.songho.ca/opengl/gl_projectionmatrix.html
	view[1][1] = near / t;
	view[2][2] = (near + far) / (near - far);
	view[2][3] = 2 * near * far / (far - near);
	view[3][2] = 1;
	view[3][3] = 0;
	return view;
}

float float_clamp(float f, float min, float max)
{
	return f < min ? min : (f > max ? max : f);
}

float float_lerp(float start, float end, float alpha)
{
	return start + (end - start) * alpha;
}

vec2 vec2_lerp(vec2& start, vec2& end, float alpha)
{
	return start + (end - start) * alpha;
}

vec3 vec3_lerp(vec3& start, vec3& end, float alpha)
{
	return start + (end - start) * alpha;
}

vec4 vec4_lerp(vec4& start, vec4& end, float alpha)
{
	return start + (end - start) * alpha;
}


//-----------------------------------------------------------------

void clear_zbuffer(int width, int height, float* zbuffer)
{
	for (int i = 0; i < width * height; i++)
		//受SquareSATMaps限制，如果太大，doubles的精度也不足
		zbuffer[i] = 1300;
}

void clear_framebuffer(int width, int height, unsigned char* framebuffer)
{
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			int index = (i * width + j) * 4;

			framebuffer[index + 2] = 94;
			framebuffer[index + 1] = 127;
			framebuffer[index] = 147;
		}
	}
}
