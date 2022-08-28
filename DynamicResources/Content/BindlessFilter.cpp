//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "BindlessFilter.h"
#define _INDEPENDENT_DDS_LOADER_
#include "Advanced/XUSGDDSLoader.h"
#undef _INDEPENDENT_DDS_LOADER_

using namespace std;
using namespace DirectX;
using namespace XUSG;

struct ResourceIndices
{
	uint32_t TexIn = 0;
	uint32_t TexOut = TexIn + 1;
	uint32_t SmpLinear = 0;
};

constexpr ResourceIndices g_resIdx;
const auto g_resourceCount = g_resIdx.TexOut + 1;
const auto g_samplerCount = g_resIdx.SmpLinear + 1;

BindlessFilter::BindlessFilter() :
	m_imageSize(1, 1)
{
	m_shaderPool = ShaderPool::MakeUnique();
}

BindlessFilter::~BindlessFilter()
{
}

bool BindlessFilter::Init(CommandList* pCommandList, const XUSG::DescriptorTableCache::sptr& descriptorTableCache,
	vector<Resource::uptr>& uploaders, Format rtFormat, const wchar_t* fileName)
{
	const auto pDevice = pCommandList->GetDevice();
	m_graphicsPipelineCache = Graphics::PipelineCache::MakeUnique(pDevice);
	m_computePipelineCache = Compute::PipelineCache::MakeUnique(pDevice);
	m_pipelineLayoutCache = PipelineLayoutCache::MakeUnique(pDevice);
	m_descriptorTableCache = descriptorTableCache;

	// Load input image
	{
		DDS::Loader textureLoader;
		DDS::AlphaMode alphaMode;

		uploaders.emplace_back(Resource::MakeUnique());
		XUSG_N_RETURN(textureLoader.CreateTextureFromFile(pCommandList, fileName,
			8192, false, m_source, uploaders.back().get(), &alphaMode), false);
	}

	// Create resources and pipelines
	m_imageSize.x = static_cast<uint32_t>(m_source->GetWidth());
	m_imageSize.y = m_source->GetHeight();

	m_result = Texture::MakeUnique();
	XUSG_N_RETURN(m_result->Create(pDevice, m_imageSize.x, m_imageSize.y, rtFormat, 1,
		ResourceFlag::ALLOW_UNORDERED_ACCESS, 1, 1, false, MemoryFlag::NONE, L"Result"), false);

	XUSG_N_RETURN(createPipelineLayouts(), false);
	XUSG_N_RETURN(createPipelines(rtFormat), false);
	XUSG_N_RETURN(createDescriptorTables(), false);

	return true;
}

void BindlessFilter::Process(CommandList* pCommandList)
{
	ResourceBarrier barrier;
	const auto numBarriers = m_result->SetBarrier(&barrier, ResourceState::UNORDERED_ACCESS);
	pCommandList->Barrier(numBarriers, &barrier);

	pCommandList->SetComputePipelineLayout(m_pipelineLayouts[IMAGE_PROC]);
	pCommandList->SetPipelineState(m_pipelines[IMAGE_PROC]);

	pCommandList->SetCompute32BitConstants(0, XUSG_SizeOfInUint32(g_resIdx), &g_resIdx);

	pCommandList->Dispatch(XUSG_DIV_UP(m_imageSize.x, 8), XUSG_DIV_UP(m_imageSize.y, 8), 1);
}

void BindlessFilter::GetImageSize(uint32_t& width, uint32_t& height) const
{
	width = m_imageSize.x;
	height = m_imageSize.y;
}

Resource* BindlessFilter::GetResult() const
{
	return m_result.get();
}

bool BindlessFilter::createPipelineLayouts()
{
	// Dynamic resources
	{
		const auto utilPipelineLayout = Util::PipelineLayout::MakeUnique();
		utilPipelineLayout->SetConstants(0, XUSG_SizeOfInUint32(ResourceIndices), 0);

		XUSG_X_RETURN(m_pipelineLayouts[IMAGE_PROC], utilPipelineLayout->GetPipelineLayout(
			m_pipelineLayoutCache.get(), PipelineLayoutFlag::CBV_SRV_UAV_POOL_DIRECTLY_INDEXED |
			PipelineLayoutFlag::SAMPLER_POOL_DIRECTLY_INDEXED, L"ImageProcLayout"), false);
	}

	return true;
}

bool BindlessFilter::createPipelines(Format rtFormat)
{
	auto csIndex = 0u;

	// One-pass MIP-Gen
	{
		XUSG_N_RETURN(m_shaderPool->CreateShader(Shader::Stage::CS, csIndex, L"CSImageProc.cso"), false);

		const auto state = Compute::State::MakeUnique();
		state->SetPipelineLayout(m_pipelineLayouts[IMAGE_PROC]);
		state->SetShader(m_shaderPool->GetShader(Shader::Stage::CS, csIndex++));
		XUSG_X_RETURN(m_pipelines[IMAGE_PROC], state->GetPipeline(m_computePipelineCache.get(), L"ImageProc"), false);
	}

	return true;
}

bool BindlessFilter::createDescriptorTables()
{
	// Create a resource table with all used CBVs/SRVs/UAVs
	{
		const auto descriptorTable = Util::DescriptorTable::MakeUnique();

		Descriptor descriptors[g_resourceCount];
		descriptors[g_resIdx.TexIn] = m_source->GetSRV();
		descriptors[g_resIdx.TexOut] = m_result->GetUAV();

		descriptorTable->SetDescriptors(0, static_cast<uint32_t>(size(descriptors)), descriptors);
		const auto table = descriptorTable->GetCbvSrvUavTable(m_descriptorTableCache.get());
		XUSG_N_RETURN(table, false);
	}

	// Create a sampler table with all used samplers
	{
		const auto descriptorTable = Util::DescriptorTable::MakeUnique();

		SamplerPreset samplers[g_samplerCount];
		samplers[g_resIdx.SmpLinear] = LINEAR_CLAMP;

		descriptorTable->SetSamplers(0, static_cast<uint32_t>(size(samplers)), samplers, m_descriptorTableCache.get());
		const auto table = descriptorTable->GetSamplerTable(m_descriptorTableCache.get());
		XUSG_N_RETURN(table, false);
	}

	return true;
}
