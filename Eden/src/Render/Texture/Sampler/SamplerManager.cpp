#include "Render/Texture/Sampler/SamplerManager.h"

SamplerManager::SamplerManager(Direct3DManager *direct3DManager)
{
	ID3D12Device *device = direct3DManager->GetDevice();
	mHeapManager = direct3DManager->GetContextManager()->GetHeapManager();

	{
		D3D12_SAMPLER_DESC desc;
		desc.Filter = D3D12_FILTER_ANISOTROPIC;
		desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.MipLODBias = 0.0f;
		desc.MaxAnisotropy = 16;
		desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc.BorderColor[0] = 0;
		desc.BorderColor[1] = 0;
		desc.BorderColor[2] = 0;
		desc.BorderColor[3] = 0;
		desc.MinLOD = 0;
		desc.MaxLOD = D3D12_FLOAT32_MAX;

		DescriptorHeapHandle samplerHandle = mHeapManager->GetNewSamplerDescriptorHeapHandle();
		device->CreateSampler(&desc, samplerHandle.GetCPUHandle());

		Sampler *newSampler = new Sampler(samplerHandle);

		mSamplerLookup.insert(std::pair<SamplerType, Sampler*>(SAMPLER_DEFAULT_ANISO, newSampler));
		mSamplers.Add(newSampler);
	}

	{
		D3D12_SAMPLER_DESC desc;
		desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.MipLODBias = 0.0f;
		desc.MaxAnisotropy = 1;
		desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc.BorderColor[0] = 0;
		desc.BorderColor[1] = 0;
		desc.BorderColor[2] = 0;
		desc.BorderColor[3] = 0;
		desc.MinLOD = 0;
		desc.MaxLOD = D3D12_FLOAT32_MAX;

		DescriptorHeapHandle samplerHandle = mHeapManager->GetNewSamplerDescriptorHeapHandle();
		device->CreateSampler(&desc, samplerHandle.GetCPUHandle());

		Sampler *newSampler = new Sampler(samplerHandle);

		mSamplerLookup.insert(std::pair<SamplerType, Sampler*>(SAMPLER_DEFAULT_POINT_CLAMP, newSampler));
		mSamplers.Add(newSampler);
	}

    {
        D3D12_SAMPLER_DESC desc;
        desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        desc.MipLODBias = 0.0f;
        desc.MaxAnisotropy = 1;
        desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        desc.BorderColor[0] = 0;
        desc.BorderColor[1] = 0;
        desc.BorderColor[2] = 0;
        desc.BorderColor[3] = 0;
        desc.MinLOD = 0;
        desc.MaxLOD = D3D12_FLOAT32_MAX;

        DescriptorHeapHandle samplerHandle = mHeapManager->GetNewSamplerDescriptorHeapHandle();
        device->CreateSampler(&desc, samplerHandle.GetCPUHandle());

        Sampler *newSampler = new Sampler(samplerHandle);

        mSamplerLookup.insert(std::pair<SamplerType, Sampler*>(SAMPLER_DEFAULT_LINEAR_CLAMP, newSampler));
        mSamplers.Add(newSampler);
    }

    {
        D3D12_SAMPLER_DESC desc;
        desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        desc.MipLODBias = 0.0f;
        desc.MaxAnisotropy = 1;
        desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        desc.BorderColor[0] = 0;
        desc.BorderColor[1] = 0;
        desc.BorderColor[2] = 0;
        desc.BorderColor[3] = 0;
        desc.MinLOD = 0;
        desc.MaxLOD = D3D12_FLOAT32_MAX;

        DescriptorHeapHandle samplerHandle = mHeapManager->GetNewSamplerDescriptorHeapHandle();
        device->CreateSampler(&desc, samplerHandle.GetCPUHandle());

        Sampler *newSampler = new Sampler(samplerHandle);

        mSamplerLookup.insert(std::pair<SamplerType, Sampler*>(SAMPLER_DEFAULT_POINT_WRAP, newSampler));
        mSamplers.Add(newSampler);
    }

    {
        D3D12_SAMPLER_DESC desc;
        desc.Filter = D3D12_FILTER_ANISOTROPIC;
        desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        desc.MipLODBias = 0.0f;
        desc.MaxAnisotropy = 16;
        desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        desc.BorderColor[0] = 0;
        desc.BorderColor[1] = 0;
        desc.BorderColor[2] = 0;
        desc.BorderColor[3] = 0;
        desc.MinLOD = 0;
        desc.MaxLOD = D3D12_FLOAT32_MAX;

        DescriptorHeapHandle samplerHandle = mHeapManager->GetNewSamplerDescriptorHeapHandle();
        device->CreateSampler(&desc, samplerHandle.GetCPUHandle());

        Sampler *newSampler = new Sampler(samplerHandle);

        mSamplerLookup.insert(std::pair<SamplerType, Sampler*>(SAMPLER_DEFAULT_ANISO_16_CLAMP, newSampler));
        mSamplers.Add(newSampler);
    }
}

SamplerManager::~SamplerManager()
{
	for (uint32 i = 0; i < mSamplers.CurrentSize(); i++)
	{
		mHeapManager->FreeSamplerDescriptorHeapHandle(mSamplers[i]->GetSamplerHandle());
		delete mSamplers[i];
	}
	mSamplerLookup.clear();
	mSamplers.Clear();
}