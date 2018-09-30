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

	bool LoadFBX(char *fileName, DynamicArray<MeshVertexData> &outVertexVector, DynamicArray<uint32> &indexSplits)
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
			uint32 indexCount = 0;
			uint32 childCount = pFbxRootNode->GetChildCount();

			for(uint32 childIndex = 0; childIndex < childCount; childIndex++)
			{
				FbxNode* pFbxChildNode = pFbxRootNode->GetChild(childIndex);

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
					pMesh = (FbxMesh*)pFbxChildNode->GetNodeAttribute(); //must reacquire after splitting
				}

				for(int32 attributeIndex = 0; attributeIndex < pFbxChildNode->GetNodeAttributeCount(); attributeIndex++)
				{
					pMesh = (FbxMesh*)pFbxChildNode->GetNodeAttributeByIndex(attributeIndex);
					pMesh->GenerateTangentsDataForAllUVSets(true);
					pMesh = (FbxMesh*)pFbxChildNode->GetNodeAttributeByIndex(attributeIndex);	//must reacquire after generating UV sets

					AddToD3DMesh(pMesh, outVertexVector);
					indexCount += pMesh->GetPolygonCount() * 3;
					indexSplits.Add(indexCount);
				}
			}
		}

		pImporter->Destroy();
		pFbxScene->Destroy();
		return true;
	}


	uint32 AddToD3DMesh(FbxMesh *mesh, DynamicArray<MeshVertexData> &outVertexVector)
	{
		uint32 indexCount = 0;
		FbxVector4* pVertices = mesh->GetControlPoints();

		for (int32 polygonIndex = 0; polygonIndex < mesh->GetPolygonCount(); polygonIndex++)
		{
			uint32 numVertices = mesh->GetPolygonSize(polygonIndex);
			Application::Assert(numVertices == 3);

			bool tangentFound = true;
			bool binormalFound = true;
			for (uint32 vertIndex = 0; vertIndex < numVertices; vertIndex++)
			{
				int controlPointIndex = mesh->GetPolygonVertex(polygonIndex, vertIndex);
				FbxVector2 uv;
				bool isMapped = false;
				FbxStringList lUVNames;
				mesh->GetUVSetNames(lUVNames);
				mesh->GetPolygonVertexUV(polygonIndex, vertIndex, lUVNames.GetStringAt(0), uv, isMapped);
				int testC = mesh->GetElementTangentCount();

				Vector3 placeHolder = Vector3::One();
				Vector3 normal, tangent, binormal;

				ReadNormal(mesh, controlPointIndex, polygonIndex*3 + vertIndex, normal);
				tangentFound = ReadTangent(mesh, controlPointIndex, polygonIndex*3 + vertIndex, tangent);
				binormalFound = ReadBiNormal(mesh, controlPointIndex, polygonIndex*3 + vertIndex, binormal);

				MeshVertexData meshData;
				meshData.Position.X = (float)pVertices[controlPointIndex].mData[0];
				meshData.Position.Y = (float)pVertices[controlPointIndex].mData[1];
				meshData.Position.Z = (float)pVertices[controlPointIndex].mData[2];
				meshData.Position.W = 1.0f;
				meshData.Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
				meshData.Normal = Vector3(normal.X, normal.Y, normal.Z);

				if(tangentFound)
				{
					meshData.Tangent = Vector3(tangent.X, tangent.Y, tangent.Z);
				}
				else
				{
					meshData.Tangent = placeHolder;
				}

				if(binormalFound)
				{
					meshData.Binormal = Vector3(binormal.X, binormal.Y, binormal.Z);
				}
				else
				{
					meshData.Binormal = placeHolder;
				}
				
				meshData.TexCoord = Vector4((float)uv.mData[0], 1.0f - (float)uv.mData[1], (float)uv.mData[0], 1.0f - (float)uv.mData[1]);
				outVertexVector.Add(meshData);
				indexCount++;
			}

			if(!tangentFound)
			{
				Vector3 tangent;
				Vector3 binormal;
				MeshVertexData::GetTangentAndBinormal(outVertexVector[polygonIndex*3], outVertexVector[polygonIndex*3+1], outVertexVector[polygonIndex*3+2], tangent, binormal);
				outVertexVector[polygonIndex*3].Tangent = Vector3(tangent.X, tangent.Y, tangent.Z);
				outVertexVector[polygonIndex*3+1].Tangent = Vector3(tangent.X, tangent.Y, tangent.Z);
				outVertexVector[polygonIndex*3+2].Tangent = Vector3(tangent.X, tangent.Y, tangent.Z);
			}
			if(!binormalFound)
			{
				Vector3 tangent;
				Vector3 binormal;
				MeshVertexData::GetTangentAndBinormal(outVertexVector[polygonIndex*3], outVertexVector[polygonIndex*3+1], outVertexVector[polygonIndex*3+2], tangent, binormal);
				outVertexVector[polygonIndex*3].Binormal = Vector3(binormal.X, binormal.Y, binormal.Z);
				outVertexVector[polygonIndex*3+1].Binormal = Vector3(binormal.X, binormal.Y, binormal.Z);
				outVertexVector[polygonIndex*3+2].Binormal = Vector3(binormal.X, binormal.Y, binormal.Z);
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
