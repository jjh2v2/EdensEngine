#include "Render/Texture/Texture.h"

Texture::Texture()
{
	mTextureResource = NULL;
	mFormat = DXGI_FORMAT_UNKNOWN;
	mDimensions = Vector3::Zero();
	mMipCount = 0;
	mArraySize = 0;
	mIsCubeMap = false;
}


Texture::~Texture()
{

}