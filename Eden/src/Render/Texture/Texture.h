#pragma once
#include <d3d11.h>
#include <d3dx11tex.h>

class Texture
{
public:
	Texture();
	~Texture();

	bool Initialize(ID3D11Device* device, WCHAR* filename);
	bool Initialize(ID3D11Device* device, WCHAR* filename, ID3DX11ThreadPump *threadPump, ID3D11ShaderResourceView *asyncTempView);
	bool Initialize(ID3D11Device* device, WCHAR* filename, D3DX11_IMAGE_LOAD_INFO* info);

	ID3D11ShaderResourceView* GetTexture();

	void SetResource(ID3D11ShaderResourceView* texture)
	{
		mTexture = texture;
	}

	HRESULT GetAsyncResult()
	{
		return mAsyncLoadResult;
	}

private:
	ID3D11ShaderResourceView* mTexture;
	ID3D11ShaderResourceView* mAsyncTempView;
	HRESULT mAsyncLoadResult;
};