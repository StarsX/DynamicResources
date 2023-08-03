//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#define GROUP_SIZE		8
#define RADIUS			16
#define SHARED_MEM_SIZE	(GROUP_SIZE + 2 * RADIUS)

#define DIV_UP(x, n)	(((x) + (n) - 1) / (n))

struct ResourceIndices
{
	uint TexIn;
	uint TexOut;
	uint SmpLinear;
};

ConstantBuffer<ResourceIndices> g_cbResIdx;

groupshared float4 g_srcs[SHARED_MEM_SIZE][SHARED_MEM_SIZE];
groupshared float4 g_dsts[SHARED_MEM_SIZE][GROUP_SIZE];

float GaussianSigmaFromRadius(float r)
{
	//return (r - 1) / 6.0;
	//return (r + 1) / sqrt(2.0 * log(255.0));
	//return (r + 1) / sqrt(6.0);
	return (r + 1.0f) / 3.0;
}

float Gaussian(float r, float sigma)
{
	const float a = r / sigma;

	return exp(-0.5 * a * a);
}

[numthreads(GROUP_SIZE, GROUP_SIZE, 1)]
void main(uint2 DTid : SV_DispatchThreadID, uint2 Gid : SV_GroupID, uint2 GTid : SV_GroupThreadID)
{
	const Texture2D texIn = ResourceDescriptorHeap[g_cbResIdx.TexIn];
	const RWTexture2D<float4> texOut = ResourceDescriptorHeap[g_cbResIdx.TexOut];

	const SamplerState smp = SamplerDescriptorHeap[g_cbResIdx.SmpLinear];

	float2 texSize;
	texIn.GetDimensions(texSize.x, texSize.y);

	// Load data into group-shared memory
	const uint n = DIV_UP(SHARED_MEM_SIZE, GROUP_SIZE);
	const int2 uvIdx = GROUP_SIZE * Gid - RADIUS;
	for (int i = 0; i < n; ++i)
	{
		const int x = GROUP_SIZE * i + GTid.x;
		if (x < SHARED_MEM_SIZE)
		{
			for (int j = 0; j < n; ++j)
			{
				const int y = GROUP_SIZE * j + GTid.y;
				if (y < SHARED_MEM_SIZE)
				{
					const float2 uv = (uvIdx + int2(x, y) + 0.5) / texSize;
					g_srcs[y][x] = texIn.SampleLevel(smp, uv, 0.0);;
				}
			}
		}
	}

	GroupMemoryBarrierWithGroupSync();

	const float sigma = GaussianSigmaFromRadius(RADIUS);
	const int x = GTid.x + RADIUS;

	// Horizontal filter
	for (uint k = 0; k < n; ++k)
	{
		const uint y = GROUP_SIZE * k + GTid.y;
		if (y >= SHARED_MEM_SIZE) break;

		float4 mu = 0.0;
		float ws = 0.0;
		for (i = -RADIUS; i <= RADIUS; ++i)
		{
			const int xi = x + i;
			const float4 src = g_srcs[y][xi];

			const float w = Gaussian(i, sigma);
			mu += src * w;
			ws += w;
		}

		g_dsts[y][GTid.x] = mu / ws;
	}

	GroupMemoryBarrierWithGroupSync();

	// Vertical filter
	const int y = GTid.y + RADIUS;

	float4 mu = 0.0;
	float ws = 0.0;
	for (i = -RADIUS; i <= RADIUS; ++i)
	{
		const int yi = y + i;
		const float4 src = g_dsts[yi][GTid.x];

		const float w = Gaussian(i, sigma);
		mu += src * w;
		ws += w;
	}

	texOut[DTid] = mu / ws;
}
