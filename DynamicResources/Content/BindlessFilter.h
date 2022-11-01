//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DXFramework.h"
#include "Core/XUSG.h"

class BindlessFilter
{
public:
	BindlessFilter();
	virtual ~BindlessFilter();

	bool Init(XUSG::CommandList* pCommandList, const XUSG::DescriptorTableLib::sptr& descriptorTableLib,
		std::vector<XUSG::Resource::uptr>& uploaders, XUSG::Format rtFormat, const wchar_t* fileName);

	void Process(XUSG::CommandList* pCommandList);
	void GetImageSize(uint32_t& width, uint32_t& height) const;

	XUSG::Resource* GetResult() const;

protected:
	enum PipelineIndex : uint8_t
	{
		IMAGE_PROC,

		NUM_PIPELINE
	};

	bool createPipelineLayouts();
	bool createPipelines(XUSG::Format rtFormat);
	bool createDescriptorTables();

	XUSG::ShaderLib::uptr				m_shaderLib;
	XUSG::Graphics::PipelineLib::uptr	m_graphicsPipelineLib;
	XUSG::Compute::PipelineLib::uptr	m_computePipelineLib;
	XUSG::PipelineLayoutLib::uptr		m_pipelineLayoutLib;
	XUSG::DescriptorTableLib::sptr		m_descriptorTableLib;

	XUSG::PipelineLayout	m_pipelineLayouts[NUM_PIPELINE];
	XUSG::Pipeline			m_pipelines[NUM_PIPELINE];

	XUSG::Texture::sptr					m_source;
	XUSG::Texture::sptr					m_result;

	DirectX::XMUINT2					m_imageSize;

	XUSG::ResourceBarrier				m_barriers[2];
	uint32_t							m_numBarriers;
};
