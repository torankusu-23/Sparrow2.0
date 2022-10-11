#include "../public/struct.h"

//vec2��Ա����

 vec2::vec2() : p{ 0, 0 } {}
 vec2::vec2(float x, float y) : p{ x, y } {}
 float vec2::x() const { return p[0]; }
 float vec2::y() const { return p[1]; }
 float vec2::operator[](int i) const { return p[i]; }	//���������þ��޷��޸�
 float& vec2::operator[](int i) { return p[i]; }//�������ÿ��ܵ��¸�ֵ���޸ģ�����vec����ʧЧ��ָ������ݣ���Ҫ�������ֺ���
 vec2 vec2::operator+(const vec2& n) const { return vec2(p[0] + n.p[0], p[1] + n.p[1]); }
 vec2 vec2::operator-(const vec2& n) const { return vec2(p[0] - n.p[0], p[1] - n.p[1]); }
 vec2 vec2::operator*(float n) const { return vec2((float)p[0] * n, (float)p[1] * n); }
 vec2 vec2::operator/(float n) const { return *this * (1 / n); }
 vec2 vec2::operator*(const vec2& v) { return vec2(p[0] * v.p[0], p[1] * v.p[1]); }
 vec2 vec2::normaliz() { return *this / (sqrt(p[0] * p[0] + p[1] * p[1])); }

//vec2�ǳ�Ա��������Ϊʹ��struct�ṹ�壬Ĭ����������public������ʹ����Ԫ��������������˰�ȫ��Ҫ�����ʹ����ʽprivate����Ԫ������
vec2 operator*(float n, vec2& u) { return u * n; };

std::ostream& operator<<(std::ostream& out, const vec2& v) { return out << v.p[0] << ' ' << v.p[1]; };

//vec3��Ա����
vec3::vec3() :p{ 0, 0, 0 } {};
vec3::vec3(float x, float y, float z) : p{ x, y, z } {}
float vec3::x() const { return p[0]; }
float vec3::y() const { return p[1]; }
float vec3::z() const { return p[2]; }
float vec3::operator[](int i) const { return p[i]; }
float& vec3::operator[](int i) { return p[i]; }
vec3 vec3::operator+(const vec3& n) const { return vec3(p[0] + n.p[0], p[1] + n.p[1], p[2] + n.p[2]); }//�����ò���fun�Ŀ���ɾ��
vec3 vec3::operator-(const vec3& n) const { return vec3(p[0] - n.p[0], p[1] - n.p[1], p[2] - n.p[2]); }
vec3 vec3::operator*(float n) const { return vec3(p[0] * n, p[1] * n, p[2] * n); }
vec3 vec3::operator/(float n) const { return *this * (1 / n); }
vec3 vec3::operator*(const vec3& v) { return vec3(p[0] * v.p[0], p[1] * v.p[1], p[2] * v.p[2]); }
float vec3::norm() { return sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]); }	//Ĥ��
vec3 vec3::normalize() { return *this / this->norm(); }	//��һ��


//vec3�ǳ�Ա����
vec3 operator*(float t, const vec3& v) { return v * t; }

std::ostream& operator<<(std::ostream& out, const vec3& v) { return out << v.p[0] << ' ' << v.p[1] << ' ' << v.p[2]; }

float dot(vec3& u, vec3& v) { return u.p[0] * v.p[0] + u.p[1] * v.p[1] + u.p[2] * v.p[2]; }

vec3 cross(const vec3& u, const vec3& v) { return vec3(u.p[1] * v.p[2] - u.p[2] * v.p[1], u.p[2] * v.p[0] - u.p[0] * v.p[2], u.p[0] * v.p[1] - u.p[1] * v.p[0]); }


//vec4�ǳ�Ա����
vec4 operator*(float t, const vec4& v) { return v * t; }

std::ostream& operator<<(std::ostream& out, const vec4& v) { return out << v.p[0] << ' ' << v.p[1] << ' ' << v.p[2] << ' ' << v.p[3]; }

vec3 down_to_vec3(const vec4& u) { return vec3(u[0] / u[3], u[1] / u[3], u[2] / u[3]); }

vec4 up_to_vec4(const vec3& u, float w) { return vec4(u[0], u[1], u[2], w); }

//mat3��Ա����
mat3 mat3::transpose() const {
	mat3 trans;
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			trans[i][j] = rows[j][i];
	return trans;
}

float determinant(const mat3& rows)	//������ʽ�͸�������
{
	return rows[0][0] * (rows[1][1] * rows[2][2] - rows[1][2] * rows[2][1])
		- rows[0][1] * (rows[1][0] * rows[2][2] - rows[1][2] * rows[2][0])
		+ rows[0][2] * (rows[1][0] * rows[2][1] - rows[1][1] * rows[2][0]);
}

mat3 mat3::inverse() const {
	mat3 adjoint;//�������
	adjoint[0][0] = +(rows[1][1] * rows[2][2] - rows[2][1] * rows[1][2]);
	adjoint[0][1] = -(rows[1][0] * rows[2][2] - rows[2][0] * rows[1][2]);
	adjoint[0][2] = +(rows[1][0] * rows[2][1] - rows[2][0] * rows[1][1]);
	adjoint[1][0] = -(rows[0][1] * rows[2][2] - rows[2][1] * rows[0][2]);
	adjoint[1][1] = +(rows[0][0] * rows[2][2] - rows[2][0] * rows[0][2]);
	adjoint[1][2] = -(rows[0][0] * rows[2][1] - rows[2][0] * rows[0][1]);
	adjoint[2][0] = +(rows[0][1] * rows[1][2] - rows[1][1] * rows[0][2]);
	adjoint[2][1] = -(rows[0][0] * rows[1][2] - rows[1][0] * rows[0][2]);
	adjoint[2][2] = +(rows[0][0] * rows[1][1] - rows[1][0] * rows[0][1]);

	mat3 inve;
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			inve[i][j] = adjoint[i][j] / determinant(*this);

	return inve;
}

//mat3�ǳ�Ա����
std::ostream& operator<<(std::ostream& out, const mat3& m) {return out << m[0] << "\n" << m[1] << "\n" << m[2];}


//mat4��Ա����
mat4 mat4::transpose() const {
	mat4 trans;
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			trans[i][j] = rows[j][i];
	return trans;
}

mat4 mat4::inverse() const {
	mat4 adjoint;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			mat3 last;
			for (int m = 0; m < 3; ++m) {
				for (int n = 0; n < 3; ++n) {
					int row = m < i ? m : m + 1;
					int col = n < j ? n : n + 1;
					last[m][n] = rows[row][col];
				}
			}
			adjoint[i][j] = determinant(last) * pow(-1, i + j);
		}
	}
	float dete4 = rows[0][0] * adjoint[0][0] + rows[1][1] * adjoint[1][1] + rows[2][2] * adjoint[2][2] + rows[3][3] * adjoint[3][3];
	mat4 inve;
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			inve[i][j] = adjoint[i][j] / dete4;
	return inve;
}

mat4 mat4::operator*(const mat4& mat) {
		mat4 m;
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				for (int k = 0; k < 4; k++)
					m[i][j] = m[i][j] + this->rows[i][k] * mat[k][j];
		return m;
}



//mat4�ǳ�Ա����
std::ostream& operator<<(std::ostream& out, const mat4& m) { return out << m[0] << "\n" << m[1] << "\n" << m[2] << "\n" << m[3]; }

vec4 operator*(const mat4& m, const vec4 v) {
	float product[4];
	for (int i = 0; i < 4; i++)
	{
		product[i] = m[i][0] * v[0] +m[i][1] * v[1] +m[i][2] * v[2] +m[i][3] * v[3];
	}
	return vec4(product[0], product[1], product[2], product[3]);
}