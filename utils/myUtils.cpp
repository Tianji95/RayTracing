#include "DXUT.h"
#include "myUtils.h"
#include <map>

D3DXVECTOR4 getRandomColor() {
	float colorR = (rand() % 1000)*1.0f / 1000.0f;
	float colorG = (rand() % 1000)*1.0f / 1000.0f;
	float colorB = (rand() % 1000)*1.0f / 1000.0f;
	return D3DXVECTOR4(colorR, colorG, colorB, 1.0f);
}