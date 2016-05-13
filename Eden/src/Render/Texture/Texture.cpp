#include "Render/Texture/Texture.h"

Texture::Texture()
{
	mTexture = NULL;
	mAsyncTempView = NULL;
	mAsyncLoadResult = E_FAIL;
}


Texture::~Texture()
{
	mAsyncTempView = NULL;

	if (mTexture)
	{
		mTexture->Release();
		mTexture = NULL;
	}
}

bool Texture::Initialize(ID3D11Device* device, WCHAR* filename)
{
	HRESULT result;
	result = D3DX11CreateShaderResourceViewFromFile(device, filename, NULL, NULL, &mTexture, NULL);
	if(FAILED(result))
	{
		return false;
	}
	return true;
}

bool Texture::Initialize(ID3D11Device* device, WCHAR* filename, ID3DX11ThreadPump *threadPump, ID3D11ShaderResourceView *asyncTempView)
{
	mAsyncTempView = asyncTempView;

	HRESULT result;
	result = D3DX11CreateShaderResourceViewFromFile(device, filename, NULL, threadPump, &mTexture, &mAsyncLoadResult);
	if (FAILED(result))
	{
		return false;
	}
	return true;
}


bool Texture::Initialize(ID3D11Device* device, WCHAR* filename, D3DX11_IMAGE_LOAD_INFO *info)
{
	HRESULT result;
	result = D3DX11CreateShaderResourceViewFromFile(device, filename, info, NULL, &mTexture, NULL);
	if(FAILED(result))
	{
		return false;
	}
	return true;
}

ID3D11ShaderResourceView* Texture::GetTexture()
{
	if (mAsyncTempView)
	{
		if (!FAILED(mAsyncLoadResult))
		{
			mAsyncTempView = NULL;
			return mTexture;
		}
		
		return mAsyncTempView;
	}

	return mTexture;
}