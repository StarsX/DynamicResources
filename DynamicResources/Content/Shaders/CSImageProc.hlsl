//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

struct ResourceIndices
{
	uint TexIn;
	uint TexOut;
	uint SmpLinear;
};

ConstantBuffer<ResourceIndices> g_cbResIdx;

[numthreads(8, 8, 1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
	const Texture2D texIn = ResourceDescriptorHeap[g_cbResIdx.TexIn];
	const RWTexture2D<float4> texOut = ResourceDescriptorHeap[g_cbResIdx.TexOut];

	const SamplerState smp = SamplerDescriptorHeap[g_cbResIdx.SmpLinear];

	float2 texSize;
	texIn.GetDimensions(texSize.x, texSize.y);

	const float2 uv = (DTid + 0.5) / texSize;
	const float4 color = texIn.SampleLevel(smp, uv, 0.0);

	const float grey = dot(color.xyz, float3(0.299, 0.587, 0.114));

	texOut[DTid] = float4(grey.xxx, color.w);
}
