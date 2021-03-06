//--------------------------------------------------------------------------------------
// File: EmptyProject11.cpp
//
// Empty starting point for new Direct3D 9 and/or Direct3D 11 applications
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTcamera.h"
#include "SDKmisc.h"
#include "utils\\tiny_obj_loader.h"
#include "utils//myUtils.h"
#include "utils//dataStruct.h"

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600

#define GROUP_COUNT WINDOW_WIDTH * WINDOW_HEIGHT
using namespace std;
#pragma comment(lib, "legacy_stdio_definitions.lib")
string                           g_inputfile = "scene01.obj";
tinyobj::attrib_t                g_attrib;
std::vector<tinyobj::shape_t>    g_shapes;
std::vector<tinyobj::material_t> g_materials;
std::string                 g_err;

ID3D11ComputeShader         *g_pComputeShader = NULL;
ID3D11Buffer                *g_constBuffer;
ID3D11Buffer                *g_pBufferSRV = NULL;
ID3D11ShaderResourceView    *g_pStructuredBufferSRV;
ID3D11UnorderedAccessView   *g_pStructuredBufferUAV;
CModelViewerCamera          g_Camera;
D3D11_FILL_MODE             g_fillMode = D3D11_FILL_WIREFRAME;
Material                    g_materialBuffer[MAX_MATERIAL_COUNT];
ID3D11Texture2D             *g_pNewTexture = NULL;
ID3D11Device                *g_pd3dDevice;
ID3D11DeviceContext         *g_pd3dImmediateContext;
std::vector<TriangleMesh>   g_triangleMesh;
int                         g_indexCount;
float                       g_modelCenterX = 0.0f, g_modelCenterY = 5.0f, g_modelCenterZ = 0.0f;
float BackgroundColor[4] = { 0.176f, 0.196f, 0.667f, 0.0f };


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT initDevice()
{
	HRESULT hr = S_OK;
	ID3DBlob* pErrorBlob;
	ID3DBlob* pComputeShaderBuffer = NULL;
	D3D11_BUFFER_DESC constBufferDesc;
	D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	int i = 0;
	UINT createDeviceFlags = D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT;
	D3D_FEATURE_LEVEL fl[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};
	hr = DXUT_Dynamic_D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags,
		fl, ARRAYSIZE(fl), D3D11_SDK_VERSION,
		&g_pd3dDevice, NULL, &g_pd3dImmediateContext);

	tinyobj::LoadObj(&g_attrib, &g_shapes, &g_materials, &g_err, g_inputfile.c_str());
	for (i = 0; i < g_materials.size(); i++) {
		g_materialBuffer[i].ks = g_materials[i].specular;//default 0 0 0
		g_materialBuffer[i].Ns = g_materials[i].shininess;//Ns default 1
		g_materialBuffer[i].kd = g_materials[i].diffuse;
		g_materialBuffer[i].Tr = 1.0f - g_materials[i].dissolve; //1-Tr default 1 tr指的是透明度 dissolve指的是不透明度
		g_materialBuffer[i].ka = g_materials[i].ambient;
		g_materialBuffer[i].Ni = g_materials[i].ior;//Ni default 1
	}

	g_indexCount = 0;
	for (size_t s = 0; s < g_shapes.size(); s++) {
		size_t index_offset = 0;
		for (size_t f = 0; f < g_shapes[s].mesh.num_face_vertices.size(); f++) {
			g_indexCount+= g_shapes[s].mesh.num_face_vertices[f];
		}
	}

	i = 0;
	g_triangleMesh.clear();
	for (size_t s = 0; s < g_shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < g_shapes[s].mesh.num_face_vertices.size(); f++) {
			int fv = g_shapes[s].mesh.num_face_vertices[f];
			TriangleMesh triangle;
			VertexType v1, v2, v3;

			tinyobj::index_t idx = g_shapes[s].mesh.indices[index_offset];
			v1.matId = g_shapes[s].mesh.material_ids[f];
			v1.position = D3DXVECTOR4(g_attrib.vertices[3 * idx.vertex_index + 0], g_attrib.vertices[3 * idx.vertex_index + 1], g_attrib.vertices[3 * idx.vertex_index + 2], 1.0f);
			v1.normal = D3DXVECTOR3(g_attrib.normals[3 * idx.normal_index + 0], g_attrib.normals[3 * idx.normal_index + 1], g_attrib.normals[3 * idx.normal_index + 2]);

			idx = g_shapes[s].mesh.indices[index_offset + 1];
			v2.matId = g_shapes[s].mesh.material_ids[f];
			v2.position = D3DXVECTOR4(g_attrib.vertices[3 * idx.vertex_index + 0], g_attrib.vertices[3 * idx.vertex_index + 1], g_attrib.vertices[3 * idx.vertex_index + 2], 1.0f);
			v2.normal = D3DXVECTOR3(g_attrib.normals[3 * idx.normal_index + 0], g_attrib.normals[3 * idx.normal_index + 1], g_attrib.normals[3 * idx.normal_index + 2]);

			idx = g_shapes[s].mesh.indices[index_offset + 2];
			v3.matId = g_shapes[s].mesh.material_ids[f];
			v3.position = D3DXVECTOR4(g_attrib.vertices[3 * idx.vertex_index + 0], g_attrib.vertices[3 * idx.vertex_index + 1], g_attrib.vertices[3 * idx.vertex_index + 2], 1.0f);
			v3.normal = D3DXVECTOR3(g_attrib.normals[3 * idx.normal_index + 0], g_attrib.normals[3 * idx.normal_index + 1], g_attrib.normals[3 * idx.normal_index + 2]);

			triangle.v1 = v1;
			triangle.v2 = v2;
			triangle.v3 = v3;
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = g_shapes[s].mesh.indices[index_offset + v];
				tinyobj::real_t vx = g_attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = g_attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = g_attrib.vertices[3 * idx.vertex_index + 2];
				tinyobj::real_t nx = g_attrib.normals[3 * idx.normal_index + 0];
				tinyobj::real_t ny = g_attrib.normals[3 * idx.normal_index + 1];
				tinyobj::real_t nz = g_attrib.normals[3 * idx.normal_index + 2];
				tinyobj::real_t tx = g_attrib.texcoords[2 * idx.texcoord_index + 0];
				tinyobj::real_t ty = g_attrib.texcoords[2 * idx.texcoord_index + 1];
				// Optional: vertex colors
				// tinyobj::real_t red = attrib.colors[3*idx.vertex_index+0];
				// tinyobj::real_t green = attrib.colors[3*idx.vertex_index+1];
				// tinyobj::real_t blue = attrib.colors[3*idx.vertex_index+2];
			}
			index_offset += fv;
			g_triangleMesh.push_back(triangle);
		}
	}

	// find the file
	WCHAR str[MAX_PATH];
	DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"shader\\ComputeShader.hlsl");

	hr = D3DX11CompileFromFile(str, NULL, NULL, "ComputeShaderMain", "cs_5_0",
		D3DCOMPILE_SKIP_OPTIMIZATION| D3DCOMPILE_DEBUG/**D3DCOMPILE_OPTIMIZATION_LEVEL1**/, 0, NULL, &pComputeShaderBuffer, &pErrorBlob, NULL);

	if (FAILED(hr))
	{
		if (pErrorBlob != NULL)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		SAFE_RELEASE(pErrorBlob);
		return hr;
	}

	hr = g_pd3dDevice->CreateComputeShader(pComputeShaderBuffer->GetBufferPointer(),
		pComputeShaderBuffer->GetBufferSize(), NULL, &g_pComputeShader);

	memset(&constBufferDesc, 0, sizeof(constBufferDesc));
	constBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	constBufferDesc.ByteWidth = sizeof(constBufferStruct);
	constBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constBufferDesc.CPUAccessFlags = 0;

	D3D11_TEXTURE2D_DESC textureDesc;
	memset(&textureDesc, 0, sizeof(textureDesc));
	textureDesc.Width = WINDOW_WIDTH;
	textureDesc.Height = WINDOW_HEIGHT;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	textureDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;

	memset(&SRVDesc, 0, sizeof(SRVDesc));
	SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	SRVDesc.Buffer.ElementWidth = g_triangleMesh.size();

	//声明乱序访问视图
	memset(&UAVDesc, 0, sizeof(UAVDesc));
	UAVDesc.Buffer.NumElements = GROUP_COUNT;
	UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
	UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	UAVDesc.Texture2D.MipSlice = 0;

	D3D11_BUFFER_DESC buffer_desc;
	ZeroMemory(&buffer_desc, sizeof(buffer_desc));
	buffer_desc.ByteWidth = g_triangleMesh.size() * sizeof(TriangleMesh);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	buffer_desc.StructureByteStride = sizeof(TriangleMesh);

	D3D11_SUBRESOURCE_DATA srvData;
	srvData.pSysMem = &g_triangleMesh[0];
	srvData.SysMemPitch = 0;
	srvData.SysMemSlicePitch = 0;


	hr = g_pd3dDevice->CreateBuffer(&buffer_desc, &srvData, &g_pBufferSRV);
	hr = g_pd3dDevice->CreateBuffer(&constBufferDesc, NULL, &g_constBuffer);
	if (FAILED(hr))
	{
		if (pErrorBlob != NULL)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		SAFE_RELEASE(pErrorBlob);
		return hr;
	}
	hr = g_pd3dDevice->CreateTexture2D(&textureDesc, NULL, &g_pNewTexture);
	hr = g_pd3dDevice->CreateShaderResourceView(g_pBufferSRV, &SRVDesc, &g_pStructuredBufferSRV);
	hr = g_pd3dDevice->CreateUnorderedAccessView(g_pNewTexture, &UAVDesc, &g_pStructuredBufferUAV);


	g_Camera.SetModelCenter(D3DXVECTOR3(g_modelCenterX, g_modelCenterY, g_modelCenterZ));
	D3DXVECTOR3 vecEye(0.0f, 5.0f, 20.0f);
	D3DXVECTOR3 vecAt(0.0f, 5.0f, 0.0f);
	g_Camera.SetViewParams(&vecEye, &vecAt);
	float fAspectRatio = WINDOW_WIDTH / (FLOAT)WINDOW_HEIGHT;
	g_Camera.SetProjParams(D3DX_PI / 3, fAspectRatio, 1.0f, 100.0f);
	g_Camera.SetWindow(WINDOW_WIDTH, (FLOAT)WINDOW_HEIGHT);
	return S_OK;
}


void doRayTracing() {
	g_Camera.SetModelCenter(D3DXVECTOR3(g_modelCenterX, g_modelCenterY, g_modelCenterZ));
	// Clear render target and the depth stencil 

	D3DXMATRIX mWorld;
	D3DXMATRIX mView;
	D3DXMATRIX mProj;
	D3DXVECTOR3 cameraPos;
	D3DXVECTOR4 cameraDir;
	D3DXVECTOR4 cameraUp;
	D3DXVECTOR4 cameraLeft;
	D3DXVECTOR4 cameraUpperLeft;
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	MatrixBufferType* dataPtr;

	mProj = *g_Camera.GetProjMatrix();
	mView = *g_Camera.GetViewMatrix();
	mWorld = *g_Camera.GetWorldMatrix();
	cameraPos = D3DXVECTOR3(g_Camera.GetEyePt()->x, g_Camera.GetEyePt()->y, g_Camera.GetEyePt()->z);
	cameraDir = D3DXVECTOR4((*g_Camera.GetLookAtPt() - *g_Camera.GetEyePt()).x, (*g_Camera.GetLookAtPt() - *g_Camera.GetEyePt()).y,(*g_Camera.GetLookAtPt() - *g_Camera.GetEyePt()).z, 1.0f);
	//todo:make cameraPara
	cameraUp = D3DXVECTOR4(0.0f, 1.0f / sqrt(3.0f) / WINDOW_WIDTH * 2.0f, 0.0f, 1.0f);
	cameraLeft = D3DXVECTOR4(1.0f / sqrt(3.0f) / WINDOW_WIDTH * 2.0f, 0.0f, 0.0f, 1.0f);
	cameraUpperLeft = D3DXVECTOR4(1.0f / sqrt(3.0f), 1.0f / sqrt(3.0f) * WINDOW_HEIGHT / WINDOW_WIDTH + 5.0f, 19.0f, 1.0f);


	D3D11_RASTERIZER_DESC rsDesc;
	ID3D11RasterizerState *IrsDesc;
	rsDesc.AntialiasedLineEnable = false;
	rsDesc.CullMode = D3D11_CULL_NONE; // 设置两面都画
	rsDesc.FillMode = g_fillMode;
	rsDesc.DepthBias = 0;
	rsDesc.DepthBiasClamp = 0.0f;
	rsDesc.DepthClipEnable = true;
	rsDesc.FrontCounterClockwise = false;
	rsDesc.MultisampleEnable = false;
	rsDesc.ScissorEnable = false;
	rsDesc.SlopeScaledDepthBias = 0.0f;
	g_pd3dDevice->CreateRasterizerState(&rsDesc, &IrsDesc);
	g_pd3dImmediateContext->RSSetState(IrsDesc);

	int triangleSize = g_triangleMesh.size();
	constBufferStruct cb = {
		triangleSize,
		cameraPos,
		cameraDir,
		cameraUp,
		cameraLeft,
		cameraUpperLeft,
		{g_materialBuffer[0],g_materialBuffer[1],g_materialBuffer[2],g_materialBuffer[3],g_materialBuffer[4],
		 g_materialBuffer[5],g_materialBuffer[6],g_materialBuffer[7],g_materialBuffer[8],g_materialBuffer[9],
		 g_materialBuffer[10],g_materialBuffer[11],g_materialBuffer[12],g_materialBuffer[13],g_materialBuffer[14],
		 g_materialBuffer[15]},
	};


	g_pd3dImmediateContext->UpdateSubresource(g_constBuffer, 0, NULL, &cb, 0, 0);
	g_pd3dImmediateContext->CSSetConstantBuffers(0, 1, &g_constBuffer);

	ID3D11ShaderResourceView* pViewNULL = NULL;
	g_pd3dImmediateContext->CSSetShaderResources(0, 1, &pViewNULL);
	g_pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pStructuredBufferUAV, NULL);
	g_pd3dImmediateContext->CSSetShaderResources(0, 1, &g_pStructuredBufferSRV);
	g_pd3dImmediateContext->CSSetShader(g_pComputeShader, NULL, 0);
	g_pd3dImmediateContext->Dispatch(WINDOW_WIDTH, WINDOW_HEIGHT, 1);


	D3DX11SaveTextureToFileA(g_pd3dImmediateContext, g_pNewTexture, D3DX11_IFF_BMP, "output.bmp");
}




//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	g_Camera.FrameMove(fElapsedTime);
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	bool* pbNoFurtherProcessing, void* pUserContext)
{
	g_Camera.HandleMessages(hWnd, uMsg, wParam, lParam);
	return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext)
{
	if (bKeyDown)
	{
		switch (nChar)
		{
		case VK_F1:
			g_fillMode = D3D11_FILL_WIREFRAME;
			break;
		case VK_F2:
			g_fillMode = D3D11_FILL_SOLID;
			break;
		case VK_LEFT:
			g_modelCenterX--;
			break;
		case VK_UP:
			g_modelCenterY--;
			break;
		case VK_RIGHT:
			g_modelCenterX++;
			break;
		case VK_DOWN:
			g_modelCenterY++;
			break;
		}


	}
}


//--------------------------------------------------------------------------------------
// Initialize everything and go into a render loop
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	initDevice();
	doRayTracing();

	return DXUTGetExitCode();
}


