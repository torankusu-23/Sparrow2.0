#pragma once
#include "tools.h"
#include "struct.h"
#include "Camera.h"
#include <vector>

typedef enum
{
	PointType,
	DirectionalType,
	TypeCount
} LightType;

struct PoissonSamples
{
	std::vector<vec2> DiskSamples;
	int idx = 0;
	int size = 0;

	void Init(int Nums);
	vec2 getVal();
};

class Light 
{
public:
	Light(vec3 _pos, Camera* _LightCam, float _exp);
	~Light();

	float* GetShadowMap();
	Camera* GetCamera();
	void setViewMat(mat4 _ViewMat);
	void setProjectionMat(mat4 _PerspectMat);
	mat4 getViewMat() { return ViewMat; }
	mat4 getProjectionMat() { return ProjectionMat; };
	vec3 getpos() { return pos; }
	float getExposure() { return Exposure; }
	float getMapVal(int x, int y);
	float getPcfVal(int x, int y, int kernel, float distance, PoissonSamples* Psamples);
	float getVssmPcfVal(int x, int y, int kernel, float distance);
	float getPcssAverageBlockDepth(int x, int y, float DistReceiver, PoissonSamples* Psamples);
	float getVssmPcssAverageBlockDepth(int x, int y, float DistReceiver);
	virtual LightType getType() { return LightType::TypeCount; }
	virtual vec3 getLightDir() { return vec3(0, 0, 0); }
	void InitMipMap();
	void InitSATMap();
	void InitSquareSATMap();
	std::vector<std::vector<std::vector<float>>>& getMipMap() { return MipMap; }
	std::vector<std::vector<double>>& getSquareSATMap() { return SquareSATMap; }
	std::vector<std::vector<double>>& getSATMap() { return SATMap; }
	double Light::getSATAverage(int x1, int y1, int x2, int y2);
	double Light::getSquareSATAverage(int x1, int y1, int x2, int y2);
private:
	Camera* LightCamera;
	float Exposure;
	float* shadowMap;
	mat4 ViewMat;
	mat4 ProjectionMat;
	vec3 pos;
	std::vector<std::vector<std::vector<float>>> MipMap;
	std::vector<std::vector<double>> SquareSATMap;
	std::vector<std::vector<double>> SATMap;
};

class PointLight: public Light
{
public:
	PointLight(vec3 _pos, Camera* _LightCam, float _exp) : Light(_pos, _LightCam, _exp) {};
	LightType getType() { return LightType::PointType; }
	//vec3 getLightDir() { return vec3(0, 0, 0); }
private:
};

class DirectionalLight : public Light
{
public:
	DirectionalLight(vec3 _pos, Camera* _LightCam, float _exp) : Light(_pos, _LightCam, _exp) { dir = (_LightCam->target - _LightCam->pos).normalize(); };
	LightType getType() { return LightType::DirectionalType; }
	vec3 getLightDir() { return dir; }
private:
	vec3 dir;
};

void uniformDiskSamples(int NumSamples, std::vector<vec2>& disk);
void poissonDiskSamples(int NumSamples, std::vector<vec2>& disk);


