#pragma once
#ifndef _DATA_STRUCT_H_
#define _DATA_STRUCT_H_
#include "DXUT.h"
#define MAX_TRIANGLE_COUNT 2000
#define MAX_MATERIAL_COUNT 16
//class AABB {
//	D3DXVECTOR3 bottom_pos;
//	D3DXVECTOR3 top_pos;
//	float XRange, YRange, ZRange;
//	AABB(D3DXVECTOR3 b_pos = D3DXVECTOR3(0.0f, 0.0f ,0.0f), D3DXVECTOR3 t_pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f)) :bottom_pos(b_pos), top_pos(t_pos)
//	{
//		XRange = top_pos.x - bottom_pos.x;
//		YRange = top_pos.y - bottom_pos.y;
//		ZRange = top_pos.z - bottom_pos.z;
//	}
//
//};


struct VertexType
{
	int matId;
	D3DXVECTOR3 normal;
	D3DXVECTOR4 position;
};

struct TriangleMesh {
	VertexType v1, v2, v3;
};
struct Material {
	float  Ns;
	D3DXVECTOR3 ks;
	float  Tr;
	D3DXVECTOR3 kd;
	float  Ni;
	D3DXVECTOR3 ka;

};


struct MatrixBufferType
{
	D3DXMATRIX world;
	D3DXMATRIX view;
	D3DXMATRIX projection;
};

struct constBufferStruct {
	int numOfTriangles;
	D3DXVECTOR3 cameraPos;
	D3DXVECTOR4 cameraDir;
	D3DXVECTOR4 cameraUp;
	D3DXVECTOR4 cameraLeft;
	D3DXVECTOR4 cameraUpperLeft;
	Material materialBuffer[MAX_MATERIAL_COUNT];
};




#endif // !_DATA_STRUCT_H_
