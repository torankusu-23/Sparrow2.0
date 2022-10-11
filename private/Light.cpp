#pragma once
#include "../public/Light.h"
#include "../public/constant.h"
#include <cstdlib>

Light::Light(vec3 _pos, Camera* _LightCam, float _exp) {
	pos = _pos;
	LightCamera = _LightCam;
	Exposure = _exp;
	shadowMap = new float[SHADOW_SIZE * SHADOW_SIZE];
}

Light::~Light() {
	delete [] shadowMap;
}

float* Light::GetShadowMap() {
	return shadowMap;
}

Camera* Light::GetCamera() {
	return LightCamera;
}

void Light::setViewMat(mat4 _ViewMat) {
	ViewMat = _ViewMat;
}

void Light::setProjectionMat(mat4 _ProjectionMat) {
	ProjectionMat = _ProjectionMat;
}

float Light::getMapVal(int x, int y) {
	int index = SHADOW_SIZE * (SHADOW_SIZE - y - 1) + x;
	return shadowMap[index];
}

int getSMX(int index) {
	return index % SHADOW_SIZE;
}

int getSMY(int index) {
	int tmp = index / SHADOW_SIZE;
	return SHADOW_SIZE - 1 - tmp;
}



void uniformDiskSamples(int NumSamples, std::vector<vec2>& disk) {
	// 随机取一个角度,rand()很费
	float sampleX = rand() / float(RAND_MAX);
	float angle = sampleX * PI * 2;
	// 随机取一个半径
	float sampleY = rand() / float(RAND_MAX);
	float radius = sqrt(sampleY);
	for (int i = 0; i < NumSamples; i++) {
		disk[i] = vec2(radius * cos(angle), radius * sin(angle));
		// 继续随机取一个半径
		sampleX = rand() / float(RAND_MAX);
		radius = sqrt(sampleY);
		// 继续随机取一个角度
		sampleY = rand() / float(RAND_MAX);
		angle = sampleX * PI;
	}
}

void poissonDiskSamples(int NumSamples, std::vector<vec2>& disk) {
	// 初始弧度
	float angle = rand() / float(RAND_MAX) * PI * 2;
	// 初始半径
	float INV_NUM_SAMPLES = 1.0 / float(NumSamples);
	float radius = INV_NUM_SAMPLES;
	// 一步的弧度
	float ANGLE_STEP = 3.883222077450933;// (sqrt(5)-1)/2 *2PI
	// 一步的半径
	float radiusStep = radius;

	for (int i = 0; i < NumSamples; i++) {
		disk[i] = vec2(cos(angle), sin(angle)) * pow(radius, 0.75);
		radius += radiusStep;
		angle += ANGLE_STEP;
	}
}

double Light::getSATAverage(int x1, int y1, int x2, int y2) {
	int area = (y2 - y1 + 1) * (x2 - x1 + 1);
	
	double sum = SATMap[x2 + 1][y2 + 1] - SATMap[x1][y2 + 1] - SATMap[x2 + 1][y1] + SATMap[x1][y1];
	//std::cout << sum << std::endl;
	return (double)sum / area;
}

double Light::getSquareSATAverage(int x1, int y1, int x2, int y2) {
	int area = (y2 - y1 + 1) * (x2 - x1 + 1);
	double sum = SquareSATMap[x2 + 1][y2 + 1] - SquareSATMap[x1][y2 + 1] - SquareSATMap[x2 + 1][y1] + SquareSATMap[x1][y1];
	return (double)sum / area;
}

float Light::getPcfVal(int x, int y, int kernel, float distance, PoissonSamples* Psamples) {
	int NumSamples = 9;
	float visible = 0.f;
	if (kernel <= 1) NumSamples = 1;

	//std::vector<vec2> disk(NumSamples);
	//poissonDiskSamples(NumSamples, disk);

	for (int i = 0; i < NumSamples; ++i) {
		vec2 samples = Psamples->getVal();
		int u = (float)kernel * samples.x() + x;
		int v = (float)kernel * samples.y() + y;

		if (u <= 0 || u >= SHADOW_SIZE || v <= 0 || v >= SHADOW_SIZE) {
			visible += 1.f;
			continue;
		}
		if (distance <= getMapVal(u, v)) visible += 1.f;
	}

	return float(visible / NumSamples);
}

float Light::getVssmPcfVal(int x, int y, int kernel, float distance) {
	if (kernel == 0) {
		return distance <= getMapVal(x, y) ? 1.f : 0.f;
	}
	else if (kernel < 0) {
		std::cout << "pcf error" << std::endl;
		return 0.f;
	}

	int maxx = fmin(SHADOW_SIZE - 1, x + kernel);
	int maxy = fmin(SHADOW_SIZE - 1, y + kernel);
	int minx = fmax(0, x - kernel);
	int miny = fmax(0, y - kernel);
	double mean = getSATAverage(minx, miny, maxx, maxy);
	double SquareMean = getSquareSATAverage(minx, miny, maxx, maxy);

	if (distance <= mean) return 1.f; //非常重要，切比雪夫不等式条件：t一定要大于miu。若t小于miu，近似完全可见。
	double variance = SquareMean - mean * mean;
	double visibleRate = variance / (variance + (distance - mean) * (distance - mean));//切比雪夫不等式求出depth>distance的比例

	return visibleRate;
}

float Light::getPcssAverageBlockDepth(int x, int y, float DistReceiver, PoissonSamples* Psamples) {
	//int index = SHADOW_SIZE * (SHADOW_SIZE - y - 1) + x;
	float rate = float(DistReceiver - 0.1f) / DistReceiver;	//默认ortho矩阵near = -0.1
	int LightDistX = x - SHADOW_SIZE / 2;
	int LightDistY = y - SHADOW_SIZE / 2;
	int MapDistX = LightDistX * rate;
	int MapDistY = LightDistY * rate;
	int radius = SHADOW_SIZE / 2 * rate;
	int MapCenterX = LightDistX - MapDistX + SHADOW_SIZE / 2;
	int MapCenterY = LightDistY - MapDistY + SHADOW_SIZE / 2;
	
	float SumBlockDepth = 0.f;
	int NumSamples = 9;
	int count = 0;
	//std::vector<vec2> disk(NumSamples);
	//poissonDiskSamples(NumSamples, disk);
	for (int i = 0; i < NumSamples; ++i) {
		vec2 samples = Psamples->getVal();
		int u = (float)radius * samples.x() + MapCenterX;
		int v = (float)radius * samples.y() + MapCenterY;
		
		if (u < 0 || u >= SHADOW_SIZE || v < 0 || v >= SHADOW_SIZE) {
			continue;
		}
		float BlockDepth = getMapVal(u, v);
		if (BlockDepth < DistReceiver) {
			SumBlockDepth += BlockDepth;
			count++;
		}
	}
	return count == 0 ? 0.f : SumBlockDepth / count;
}

float Light::getVssmPcssAverageBlockDepth(int x, int y, float DistReceiver) {
	float rate = float(DistReceiver - 0.1f) / DistReceiver;	//默认ortho矩阵near = -0.1
	int LightDistX = x - SHADOW_SIZE / 2;
	int LightDistY = y - SHADOW_SIZE / 2;
	int MapDistX = LightDistX * rate;
	int MapDistY = LightDistY * rate;
	//int radius = SHADOW_SIZE / 2 * rate;
	int MapCenterX = LightDistX - MapDistX + SHADOW_SIZE / 2;
	int MapCenterY = LightDistY - MapDistY + SHADOW_SIZE / 2;
	
	int ConstSize = 4;

	int maxx = fmin(SHADOW_SIZE - 1, MapCenterX + ConstSize);
	int maxy = fmin(SHADOW_SIZE - 1, MapCenterY + ConstSize);
	int minx = fmax(0, MapCenterX - ConstSize);
	int miny = fmax(0, MapCenterY - ConstSize);
	double mean = getSATAverage(minx, miny, maxx, maxy);
	double SquareMean = getSquareSATAverage(minx, miny, maxx, maxy);
	
	
	if (DistReceiver - 0.5f < mean) return 0.f; //bias可以消除该值边缘的artifact
	double variance = SquareMean - mean * mean;
	double N1dN = variance / (variance + (DistReceiver - mean) * (DistReceiver - mean));//切比雪夫不等式求出depth>distance的比例
	double N2dN = 1.f - N1dN;
	int N = (2 * ConstSize + 1) * (2 * ConstSize + 1);
	int N2 = N2dN * N;
	int N1 = N - N2;
	float Zunocc = DistReceiver;
	float Zavg = mean;
	
	float Zocc = (N * Zavg - N1 * Zunocc) / N2;

	return Zocc;
}


void Light::InitMipMap() {
	int Level = 0;
	int MipSize = SHADOW_SIZE;
	while (MipSize > 0) {
		std::vector<std::vector<float>> curMap(MipSize, std::vector<float>(MipSize));
		if (Level == 0) {
			for (int i = 0; i < MipSize * MipSize; ++i) {
				int x = getSMX(i);
				int y = getSMY(i);
				curMap[x][y] = shadowMap[i];
			}
		}
		else {
			for (int i = 0; i < MipSize; ++i) {
				for (int j = 0; j < MipSize; ++j) {
					curMap[i][j] = float(MipMap[Level - 1][i * 2][j * 2] + MipMap[Level - 1][i * 2 + 1][j * 2] + 
						MipMap[Level - 1][i * 2][j * 2 + 1] + MipMap[Level - 1][i * 2 + 1][j * 2 + 1]) / 4.f;
				}
			}
		}

		MipMap.push_back(curMap);
		Level++;
		MipSize /= 2;
	}
}

void Light::InitSquareSATMap() {
	SquareSATMap.resize(SHADOW_SIZE + 1, std::vector<double>(SHADOW_SIZE + 1, 0.0));
	for (int i = 0; i < SHADOW_SIZE; i++) {
		for (int j = 0; j < SHADOW_SIZE; j++) {
			int idx = (SHADOW_SIZE - j - 1) * SHADOW_SIZE + i;
			SquareSATMap[i + 1][j + 1] = SquareSATMap[i][j + 1] + SquareSATMap[i + 1][j] - SquareSATMap[i][j] + shadowMap[idx] * shadowMap[idx];
			//square值大概从3600到1e10，数值正确，但位数太多，无法draw出来看效果，画面会错误，显示的位数不够
		}
	}
}

void Light::InitSATMap() {
	SATMap.resize(SHADOW_SIZE + 1, std::vector<double>(SHADOW_SIZE + 1, 0.0));
	for (int x = 0; x < SHADOW_SIZE; x++) {
		for (int y = 0; y < SHADOW_SIZE; y++) {
			int idx = (SHADOW_SIZE - y - 1) * SHADOW_SIZE + x;
			SATMap[x + 1][y + 1] = SATMap[x][y + 1] + SATMap[x + 1][y] - SATMap[x][y] + shadowMap[idx];

		}
	}
}



void PoissonSamples::Init(int Nums) {
	std::vector<vec2> samples;
	for (int i = 0; i < Nums / 10; ++i) {
		std::vector<vec2> disk(10);
		poissonDiskSamples(10, disk);
		for (auto it : disk) {
			samples.push_back(it);
		}
	}
	//poissonDiskSamples(Nums, samples);

	DiskSamples = std::move(samples);
	size = Nums;
	std::cout<< "precompute samples nums: "<< Nums << std::endl;
}

vec2 PoissonSamples::getVal() {
	vec2 val = DiskSamples[idx++];
	idx %= size;
	return val;
}
