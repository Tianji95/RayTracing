#define SAMPLE_COUNT 100
#define SAMPLE_COUNT_F 100.0f
#define MAX_TRIANGLE_COUNT 2000
#define MAX_MATERIAL_COUNT 16
#define MAX_DISTANCE 9999999
#define MAX_DEPTH 5
#define EPSILON 0.0001
#define LIGHT_COLOR float4(1.0f, 1.0f, 1.0f, 1.0f)
#define ILLUM 70
#define ENVIRONMENT_COLOR float4(0.0f, 0.0f, 0.0f, 1.0f)
#define PI 3.1415926535
#define DIFFUSE_COFF 30


struct randomResult {
	uint randomNum;
	uint seed;
};

struct VertexType
{
	int matId;
	float3 normal;
	float4 position;

};

struct TriangleMesh {
	VertexType v1;
	VertexType v2;
	VertexType v3;
};

struct RayHit {
	float3 rayOri;
	float3 rayDir;
};

struct Material {
	float  Ns;
	float3 Ks;
	float  Tr;
	float3 Kd;
	float  Ni;
	float3 Ka;
};

struct SampleResult {
	float3 rayDir;
	float  isInsecLight;
	float4 color;
};

struct SampleResultChild {
	float3 rayDir;
	float3 colorCoffi;
};

struct MCresult {
	RayHit outputRay;
	float4 color;
	bool isInsecLight;
	bool isIntersec;
};

cbuffer constBuffer:register(b0)
{
	int numOfTriangles;
	float3 cameraPos;
	float4 cameraDir;
	float4 cameraUp;
	float4 cameraLeft;
	float4 cameraUpperLeft;
	Material materialBuffer[MAX_MATERIAL_COUNT];
};

StructuredBuffer<TriangleMesh> triangleBuffer: register(t0);
RWTexture2D<float4> outColor : register(u0);


randomResult getRandomNum(uint seed)
{
	randomResult resultStruct;
	uint next = seed;
	uint result;
	//stdlib里面rand_r方法
	next *= 1103515245;
	next += 12345;
	result = (uint)(next / 65536) % 2048;

	next *= 1103515245;
	next += 12345;
	result <<= 10;
	result ^= (uint)(next / 65536) % 1024;

	next *= 1103515245;
	next += 12345;
	result <<= 10;
	result ^= (uint)(next / 65536) % 1024;
	resultStruct.randomNum = result;
	resultStruct.seed = next;
	return resultStruct;
}


float3 rotateVector(float3 originVec, float3 from, float3 to) {
	float3 crossRow = cross(from, to);
	float  cosRow = dot(from, to);
	float  sinRow = sqrt(dot(crossRow, crossRow));
	if (crossRow.x < EPSILON && crossRow.y < EPSILON && crossRow.x < EPSILON) {
	}
	else {
		crossRow = normalize(crossRow);
	}
	float3 ARow = crossRow;
	float rotateX = originVec.x * (cosRow + ARow.x * ARow.x * (1.0f - cosRow))          + originVec.y * (ARow.x * ARow.y * (1.0f - cosRow) - ARow.z * sinRow) + originVec.z * (ARow.x * ARow.z * (1.0f - cosRow) + ARow.y * sinRow);
	float rotateY = originVec.x * (ARow.x * ARow.y * (1.0f - cosRow) + ARow.z * sinRow) + originVec.y * (cosRow + ARow.y * ARow.y * (1.0f - cosRow))          + originVec.z * (ARow.y * ARow.z * (1.0f - cosRow) - ARow.x * sinRow);
	float rotateZ = originVec.x * (ARow.x * ARow.z * (1.0f - cosRow) - ARow.y * sinRow) + originVec.y * (ARow.y * ARow.z * (1.0f - cosRow) + ARow.x * sinRow) + originVec.z * (cosRow + ARow.z * ARow.z * (1.0f - cosRow));
	return float3(rotateX, rotateY, rotateZ);
}

SampleResultChild MCSamplingDiffuse(uint seed, Material nowMaterial, float3 normal) {
	SampleResultChild result;
	float random1, random2;
	randomResult rr;
	rr = getRandomNum(seed);
	random1 = rr.randomNum % 65535 / 65535.0f;
	rr = getRandomNum(rr.seed);
	random2 = rr.randomNum % 65535 / 65535.0f;
	float x, y, z;
	x = cos(2.0f * PI * random2)*sqrt(1.0f - random1 * random1);
	z = sin(2.0f * PI * random2)*sqrt(1.0f - random1 * random1);
	y = random1;

	result.rayDir = normalize(rotateVector(float3(x, y, z), float3(0, 1, 0), normal));
	result.colorCoffi = nowMaterial.Kd * random1;
	return result;
}


SampleResultChild MCSamplingSpecular(uint seed, Material nowMaterial, float3 normal, RayHit ray) {
	SampleResultChild result;
	float random1, random2;
	randomResult rr;
	rr = getRandomNum(seed);
	random1 = rr.randomNum % 65535 / 65535.0f;
	rr = getRandomNum(rr.seed);
	random2 = rr.randomNum % 65535 / 65535.0f;
	float theta = acos(pow(random1, 1.0f / (nowMaterial.Ns + 1.0f)));
	float phi = 2 * PI * random2;
	float x, y, z;
	//todo: rotate to the normal
	x = sin(theta) * sin(phi);
	y = cos(theta);
	z = -sin(theta) * cos(phi);

	float3 halfVec = rotateVector(float3(x, y, z), float3(0, 1, 0), normal);
	result.rayDir = normalize(ray.rayDir - halfVec * dot(ray.rayDir, halfVec) * 2);
	result.colorCoffi = nowMaterial.Ks;
	return result;
}

SampleResultChild MCSamplingTransparent(uint seed, Material nowMaterial, float3 normal, RayHit ray) {
	SampleResultChild result;
	float random1;
	float Tr;
	float dotNum;
	float x, y, z;
	randomResult rr;

	rr = getRandomNum(seed);
	random1 = rr.randomNum % 65535 / 65535.0f;

	dotNum = dot(ray.rayDir, normal);
	//https://zhuanlan.zhihu.com/p/31534769 菲涅尔反射的Schlick 近似
	//Tr = nowMaterial.Tr * (1.0f - pow(1.0f - abs(dotNum), 5));
	Tr = nowMaterial.Tr;
	if (random1 > Tr) {
		//反射
		result.rayDir = normalize(ray.rayDir - normal * dot(ray.rayDir, normal) * 2);
		result.colorCoffi = float3(1.0f, 1.0f, 1.0f);
	}
	else {
		//折射
		if (dotNum <= 0) {
			//进入透射球
			//ray_tracing_2017.ppt第16页
			result.rayDir = normalize(normal * (-dotNum / nowMaterial.Ni - sqrt(1.0f - (1.0f - dotNum * dotNum) / nowMaterial.Ni / nowMaterial.Ni)) + ray.rayDir / nowMaterial.Ni);
			result.colorCoffi = float3(1.0f, 1.0f, 1.0f);
		}
		else {
			//射出透射球
			float delta = 1.0f - (1.0f - dotNum * dotNum) * nowMaterial.Ni * nowMaterial.Ni;
			if (delta < 0) {
				//射出的时候全部反射回来了
				result.rayDir = normalize(ray.rayDir - normal * dot(ray.rayDir, normal) * 2);
				result.colorCoffi = float3(1.0f, 1.0f, 1.0f);
			}
			else {
				result.rayDir = normalize(normal * (-dotNum * nowMaterial.Ni + sqrt(delta)) + ray.rayDir * nowMaterial.Ni);
				result.colorCoffi = float3(1.0f, 1.0f, 1.0f);
			}
		}
	}
	return result;
}


SampleResult MCSamplingRay(uint seed, float3 intersecPoint, Material nowMaterial, float3 normal, RayHit ray, float4 color) {
	
	SampleResult result;
	SampleResultChild resultChild;
	result.isInsecLight = false;
	if (nowMaterial.Ka.x > EPSILON && nowMaterial.Ka.y > EPSILON && nowMaterial.Ka.z > EPSILON) {
		result.isInsecLight = true;
		float4 coffi = LIGHT_COLOR * ILLUM;
		result.color = float4(coffi.x * color.x, coffi.y * color.y, coffi.z * color.z, coffi.w * color.w);
	}
	else if (nowMaterial.Tr > EPSILON) {
		resultChild = MCSamplingTransparent(seed, nowMaterial, normal, ray);
		result.rayDir = resultChild.rayDir;
		result.color = float4(resultChild.colorCoffi.x * color.x, resultChild.colorCoffi.y * color.y, resultChild.colorCoffi.z * color.z, 1.0f);

	}
	else if (nowMaterial.Ns - 1.0f> EPSILON) {
		resultChild = MCSamplingSpecular(seed, nowMaterial, normal, ray);
		result.rayDir = resultChild.rayDir;
		result.color = float4(resultChild.colorCoffi.x * color.x, resultChild.colorCoffi.y * color.y, resultChild.colorCoffi.z * color.z, 1.0f);
	}
	else{
		resultChild = MCSamplingDiffuse(seed, nowMaterial, normal);
		result.rayDir = resultChild.rayDir;
		result.color = float4(resultChild.colorCoffi.x * color.x, resultChild.colorCoffi.y * color.y, resultChild.colorCoffi.z * color.z, 1.0f);
	}
	return result;
}

MCresult doMCRayTracing(uint seed, RayHit ray, float4 color) {
	float3 coeffi;
	int triangleIdx;
	float beta, gamma, t, tmin;
	bool isIntersec = false;
	tmin = 100000.0f;
	float4 intersecPoint;
	float3 normal;
	float4 pos1;
	float4 pos2;
	float4 pos3;
	RayHit outRay;

	Material nowMaterial;
	float3x3 originMat, betaMat, gammaMat, tMat;
	for (triangleIdx = 0; triangleIdx < numOfTriangles; triangleIdx++) {
		pos1 = triangleBuffer[triangleIdx].v1.position;
		pos2 = triangleBuffer[triangleIdx].v2.position;
		pos3 = triangleBuffer[triangleIdx].v3.position;

		originMat = float3x3(
			pos1.x - pos2.x, pos1.x - pos3.x, ray.rayDir.x,
			pos1.y - pos2.y, pos1.y - pos3.y, ray.rayDir.y,
			pos1.z - pos2.z, pos1.z - pos3.z, ray.rayDir.z
		);

		betaMat = float3x3(
			pos1.x - ray.rayOri.x, pos1.x - pos3.x, ray.rayDir.x,
			pos1.y - ray.rayOri.y, pos1.y - pos3.y, ray.rayDir.y,
			pos1.z - ray.rayOri.z, pos1.z - pos3.z, ray.rayDir.z
		);

		gammaMat = float3x3(
			pos1.x - pos2.x, pos1.x - ray.rayOri.x, ray.rayDir.x,
			pos1.y - pos2.y, pos1.y - ray.rayOri.y, ray.rayDir.y,
			pos1.z - pos2.z, pos1.z - ray.rayOri.z, ray.rayDir.z
		);

		tMat = float3x3(
			pos1.x - pos2.x, pos1.x - pos3.x, pos1.x - ray.rayOri.x,
			pos1.y - pos2.y, pos1.y - pos3.y, pos1.y - ray.rayOri.y,
			pos1.z - pos2.z, pos1.z - pos3.z, pos1.z - ray.rayOri.z
		);
		beta = determinant(betaMat) / determinant(originMat);
		gamma = determinant(gammaMat) / determinant(originMat);
		t = determinant(tMat) / determinant(originMat);
		if (beta + gamma - 1.0f < EPSILON && beta > EPSILON && gamma > EPSILON && t > EPSILON && t < tmin) {
			isIntersec = true;
			//intersecPoint = ray.rayOri + t * ray.rayDir;
			intersecPoint = (1.0f - beta - gamma) * pos1 + beta * pos2 + gamma * pos3;
			normal = normalize((1.0f - beta - gamma) * triangleBuffer[triangleIdx].v1.normal + beta * triangleBuffer[triangleIdx].v2.normal + gamma * triangleBuffer[triangleIdx].v3.normal);
			nowMaterial = materialBuffer[triangleBuffer[triangleIdx].v1.matId];
			tmin = t;
		}
	}


	SampleResult samResult;
	MCresult result;

	result.isInsecLight = false;
	if (isIntersec == false) {
		outRay.rayOri = float3(0.0f, 0.0f, 0.0f);
		outRay.rayDir = float3(0.0f, 0.0f, 0.0f);
		result.outputRay = outRay;
		result.color = float4(0.0f, 0.0f, 0.0f, 1.0f);
		result.isIntersec = false;
	}
	else {
		samResult = MCSamplingRay(seed, intersecPoint, nowMaterial, normal, ray, color);
		outRay.rayOri = intersecPoint;
		outRay.rayDir = samResult.rayDir;

		result.outputRay = outRay;
		result.color = samResult.color;
		result.isInsecLight = samResult.isInsecLight;
		result.isIntersec = true;
	}
	return result;
}

[numthreads(1, 1, 1)]

void ComputeShaderMain( uint3 Gid : SV_GroupID, uint3 DTid:SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GIdx:SV_GroupIndex)
{

	float xrand, yrand;
	float3 pixelPos;
	int i = 0, j = 0;
	float4 sumColor, color;
	randomResult rr;
	RayHit ray;
	MCresult result;
	bool isInsecLight;

	rr = getRandomNum(DTid.x * 1000+ DTid.y);
	sumColor = float4(0.0f, 0.0f,0.0f, 1.0f);
	for (j = 0; j < SAMPLE_COUNT; j++) {
		rr = getRandomNum(rr.seed+j);
		xrand = rr.randomNum % 65535 / 65535.0f;
		rr = getRandomNum(rr.seed);
		yrand = rr.randomNum % 65535 / 65535.0f;
		pixelPos = cameraUpperLeft - (cameraLeft * (Gid.x + xrand) + cameraUp * (Gid.y + yrand));
		ray.rayOri = cameraPos;
		ray.rayDir = normalize(pixelPos - cameraPos);
		color = LIGHT_COLOR;
		isInsecLight = false;
		for (i = 0; i < MAX_DEPTH; i++) {
			result = doMCRayTracing(rr.seed, ray, color);
			color = result.color;
			if (result.isIntersec==false) {
				rr = getRandomNum(rr.seed);
				break;
			}
			if (result.isInsecLight) {
				isInsecLight = true;
				rr = getRandomNum(rr.seed);
				break;
			}
			ray = result.outputRay;
			rr = getRandomNum(rr.seed);
		}
		if (isInsecLight) {
			sumColor += color;
		}
	}
	sumColor = float4(sumColor.x, sumColor.y, sumColor.z, SAMPLE_COUNT_F);
	outColor[Gid.xy] = sumColor / SAMPLE_COUNT_F;
}