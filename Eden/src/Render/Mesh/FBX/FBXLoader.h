#pragma once

#include <fbxsdk.h>
#include "Core/Containers/DynamicArray.h"
#include "Render/Mesh/Mesh.h"
#include "Util/Math/MathHelper.h"
#include <map>
#include <list>

class FBXLoader
{
public:
	FBXLoader()
	{
		mFBXSDKManager = FbxManager::Create();
		FbxIOSettings* pIOsettings = FbxIOSettings::Create(mFBXSDKManager, IOSROOT );
		mFBXSDKManager->SetIOSettings(pIOsettings);
	}

	~FBXLoader()
	{
		mFBXSDKManager->Destroy();
	}

	bool LoadFBX(char *fileName, DynamicArray<MeshVertexData> &outVertexVector, DynamicArray<int> &indexSplits)
	{
		FbxImporter* pImporter = FbxImporter::Create(mFBXSDKManager,"");
		FbxScene* pFbxScene = FbxScene::Create(mFBXSDKManager,"");

		bool bSuccess = pImporter->Initialize(fileName, -1, mFBXSDKManager->GetIOSettings() );
		if(!bSuccess) 
		{
			return false;
		}

		bSuccess = pImporter->Import(pFbxScene);
		if(!bSuccess)
		{
			return false;
		}

		FbxNode* pFbxRootNode = pFbxScene->GetRootNode();

		if(pFbxRootNode)
		{
			int indexCount = 0;
			int childCount = pFbxRootNode->GetChildCount();

			for(int i = 0; i < childCount; i++)
			{
				FbxNode* pFbxChildNode = pFbxRootNode->GetChild(i);

				if(pFbxChildNode->GetNodeAttribute() == NULL)
				{
					continue;
				}

				FbxNodeAttribute::EType nodeAttribute = pFbxChildNode->GetNodeAttribute()->GetAttributeType();
				int stackCount = pImporter->GetAnimStackCount();
				

				if(nodeAttribute != FbxNodeAttribute::eMesh)
				{
					continue;
				}

				FbxMesh* pMesh = (FbxMesh*) pFbxChildNode->GetNodeAttribute();
				bool isTriangulated = pMesh->IsTriangleMesh();
				if(!isTriangulated)
				{
					FbxGeometryConverter geometryConverter(mFBXSDKManager);
					geometryConverter.Triangulate(pMesh, true);
					pMesh = (FbxMesh*)pFbxChildNode->GetNodeAttribute();

					geometryConverter.SplitMeshPerMaterial(pMesh, true);

					pMesh = (FbxMesh*)pFbxChildNode->GetNodeAttribute();
				}

				for(int i = 0; i < pFbxChildNode->GetNodeAttributeCount(); i++)
				{
					pMesh = (FbxMesh*)pFbxChildNode->GetNodeAttributeByIndex(i);

					pMesh->GenerateTangentsDataForAllUVSets(true);

					pMesh = (FbxMesh*)pFbxChildNode->GetNodeAttributeByIndex(i);

					AddToD3DMesh(pMesh, outVertexVector);
					indexCount += pMesh->GetPolygonCount()*3;
					indexSplits.Add(indexCount);
				}
			}
		}

		pImporter->Destroy();
		pFbxScene->Destroy();
		return true;
	}


	int AddToD3DMesh(FbxMesh *mesh, DynamicArray<MeshVertexData> &outVertexVector)
	{
		int indexCount = 0;
		FbxVector4* pVertices = mesh->GetControlPoints();

		for (int j = 0; j < mesh->GetPolygonCount(); j++)
		{
			int iNumVertices = mesh->GetPolygonSize(j);
			//assert( iNumVertices == 3 );

			bool tangentFound = true;
			bool binormalFound = true;
			for (int k = 0; k < iNumVertices; k++)
			{
				int iControlPointIndex = mesh->GetPolygonVertex(j, k);
				FbxVector2 uv;
				bool mapped = false;
				FbxStringList lUVNames;
				mesh->GetUVSetNames(lUVNames);
				mesh->GetPolygonVertexUV(j, k, lUVNames.GetStringAt(0), uv, mapped);
				int testC = mesh->GetElementTangentCount();

				Vector3 placeHolder = Vector3::One();
				Vector3 normal, tangent, binormal;

				ReadNormal(mesh, iControlPointIndex, j*3 + k, normal);
				tangentFound = ReadTangent(mesh, iControlPointIndex, j*3 + k, tangent);
				binormalFound = ReadBiNormal(mesh, iControlPointIndex, j*3 + k, binormal);

				MeshVertexData meshData;
				meshData.Position.x = (float)pVertices[iControlPointIndex].mData[0];
				meshData.Position.y = (float)pVertices[iControlPointIndex].mData[1];
				meshData.Position.z = (float)pVertices[iControlPointIndex].mData[2];
				meshData.Normal = D3DXVECTOR3(normal.X, normal.Y, normal.Z);

				if(tangentFound)
				{
					meshData.Tangent = D3DXVECTOR3(tangent.X, tangent.Y, tangent.Z);
				}
				else
				{
					meshData.Tangent = placeHolder.AsD3DVector3();
				}

				if(binormalFound)
				{
					meshData.Binormal = D3DXVECTOR3(binormal.X, binormal.Y, binormal.Z);
				}
				else
				{
					meshData.Binormal = placeHolder.AsD3DVector3();
				}
				
				meshData.TexCoord = D3DXVECTOR2((float)uv.mData[0], 1.0f - (float)uv.mData[1]);
				outVertexVector.Add(meshData);
				indexCount++;
			}

			if(!tangentFound)
			{
				Vector3 tangent;
				Vector3 binormal;
				MeshVertexData::GetTangentAndBinormal(outVertexVector[j*3], outVertexVector[j*3+1], outVertexVector[j*3+2], tangent, binormal);
				outVertexVector[j*3].Tangent = tangent.AsD3DVector3();
				outVertexVector[j*3+1].Tangent = tangent.AsD3DVector3();
				outVertexVector[j*3+2].Tangent = tangent.AsD3DVector3();
			}
			if(!binormalFound)
			{
				Vector3 tangent;
				Vector3 binormal;
				MeshVertexData::GetTangentAndBinormal(outVertexVector[j*3], outVertexVector[j*3+1], outVertexVector[j*3+2], tangent, binormal);
				outVertexVector[j*3].Binormal = binormal.AsD3DVector3();
				outVertexVector[j*3+1].Binormal = binormal.AsD3DVector3();
				outVertexVector[j*3+2].Binormal = binormal.AsD3DVector3();
			}
		}

		return indexCount;
	}


	void ReadNormal(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, Vector3& outNormal)
	{
		if(inMesh->GetElementNormalCount() < 1)
		{
			throw std::exception("Invalid Normal Number");
		}

		int ccc = inMesh->GetElementNormalCount();

		FbxGeometryElementNormal* vertexNormal = inMesh->GetElementNormal(0);
		switch(vertexNormal->GetMappingMode())
		{
		case FbxGeometryElement::eByControlPoint:
			switch(vertexNormal->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
				{
					outNormal.X = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[0]);
					outNormal.Y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[1]);
					outNormal.Z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[2]);
				}
				break;

			case FbxGeometryElement::eIndexToDirect:
				{
					int index = vertexNormal->GetIndexArray().GetAt(inCtrlPointIndex);
					outNormal.X = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[0]);
					outNormal.Y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[1]);
					outNormal.Z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[2]);
				}
				break;

			default:
				throw std::exception("Invalid Reference");
			}
			break;

		case FbxGeometryElement::eByPolygonVertex:
			switch(vertexNormal->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
				{
					outNormal.X = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inVertexCounter).mData[0]);
					outNormal.Y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inVertexCounter).mData[1]);
					outNormal.Z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inVertexCounter).mData[2]);
				}
				break;

			case FbxGeometryElement::eIndexToDirect:
				{
					int index = vertexNormal->GetIndexArray().GetAt(inVertexCounter);
					outNormal.X = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[0]);
					outNormal.Y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[1]);
					outNormal.Z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[2]);
				}
				break;

			default:
				throw std::exception("Invalid Reference");
			}
			break;
		}
	}

	bool ReadTangent(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, Vector3& outTangent)
	{
		if(inMesh->GetElementTangentCount() < 1)
		{
			return false;
		}

		FbxGeometryElementTangent* vertexTangent = inMesh->GetElementTangent(0);
		switch(vertexTangent->GetMappingMode())
		{
		case FbxGeometryElement::eByControlPoint:
			switch(vertexTangent->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
				{
					outTangent.X = static_cast<float>(vertexTangent->GetDirectArray().GetAt(inCtrlPointIndex).mData[0]);
					outTangent.Y = static_cast<float>(vertexTangent->GetDirectArray().GetAt(inCtrlPointIndex).mData[1]);
					outTangent.Z = static_cast<float>(vertexTangent->GetDirectArray().GetAt(inCtrlPointIndex).mData[2]);
				}
				break;

			case FbxGeometryElement::eIndexToDirect:
				{
					int index = vertexTangent->GetIndexArray().GetAt(inCtrlPointIndex);
					outTangent.X = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[0]);
					outTangent.Y = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[1]);
					outTangent.Z = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[2]);
				}
				break;

			default:
				throw std::exception("Invalid Reference");
			}
			break;

		case FbxGeometryElement::eByPolygonVertex:
			switch(vertexTangent->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
				{
					outTangent.X = static_cast<float>(vertexTangent->GetDirectArray().GetAt(inVertexCounter).mData[0]);
					outTangent.Y = static_cast<float>(vertexTangent->GetDirectArray().GetAt(inVertexCounter).mData[1]);
					outTangent.Z = static_cast<float>(vertexTangent->GetDirectArray().GetAt(inVertexCounter).mData[2]);
				}
				break;

			case FbxGeometryElement::eIndexToDirect:
				{
					int index = vertexTangent->GetIndexArray().GetAt(inVertexCounter);
					outTangent.X = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[0]);
					outTangent.Y = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[1]);
					outTangent.Z = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[2]);
				}
				break;

			default:
				throw std::exception("Invalid Reference");
			}
			break;
		}
		return true;
	}

	bool ReadBiNormal(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, Vector3& outBiNormal)
	{
		if(inMesh->GetElementBinormalCount() < 1)
		{
			return false;
		}

		FbxGeometryElementBinormal* vertexBiNormal = inMesh->GetElementBinormal(0);
		switch(vertexBiNormal->GetMappingMode())
		{
		case FbxGeometryElement::eByControlPoint:
			switch(vertexBiNormal->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
				{
					outBiNormal.X = static_cast<float>(vertexBiNormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[0]);
					outBiNormal.Y = static_cast<float>(vertexBiNormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[1]);
					outBiNormal.Z = static_cast<float>(vertexBiNormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[2]);
				}
				break;

			case FbxGeometryElement::eIndexToDirect:
				{
					int index = vertexBiNormal->GetIndexArray().GetAt(inCtrlPointIndex);
					outBiNormal.X = static_cast<float>(vertexBiNormal->GetDirectArray().GetAt(index).mData[0]);
					outBiNormal.Y = static_cast<float>(vertexBiNormal->GetDirectArray().GetAt(index).mData[1]);
					outBiNormal.Z = static_cast<float>(vertexBiNormal->GetDirectArray().GetAt(index).mData[2]);
				}
				break;

			default:
				throw std::exception("Invalid Reference");
			}
			break;

		case FbxGeometryElement::eByPolygonVertex:
			switch(vertexBiNormal->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
				{
					outBiNormal.X = static_cast<float>(vertexBiNormal->GetDirectArray().GetAt(inVertexCounter).mData[0]);
					outBiNormal.Y = static_cast<float>(vertexBiNormal->GetDirectArray().GetAt(inVertexCounter).mData[1]);
					outBiNormal.Z = static_cast<float>(vertexBiNormal->GetDirectArray().GetAt(inVertexCounter).mData[2]);
				}
				break;

			case FbxGeometryElement::eIndexToDirect:
				{
					int index = vertexBiNormal->GetIndexArray().GetAt(inVertexCounter);
					outBiNormal.X = static_cast<float>(vertexBiNormal->GetDirectArray().GetAt(index).mData[0]);
					outBiNormal.Y = static_cast<float>(vertexBiNormal->GetDirectArray().GetAt(index).mData[1]);
					outBiNormal.Z = static_cast<float>(vertexBiNormal->GetDirectArray().GetAt(index).mData[2]);
				}
				break;

			default:
				throw std::exception("Invalid Reference");
			}
			break;
		}
		return true;
	}


private:
	FbxManager* mFBXSDKManager;
};
