#pragma once
//#pragma pack(push, 1)

namespace MyFileFormat
{


	/*
		Model Header always assumes the existence of at least:
		float3	position
		float3	normal
		float2	UV

		May add:

		float3	tangent
		float3	bitangent
		float4	weights
		uchar4	boneID
	*/

#define MESH_NAME_MAX_LENGTH 128
#define MATERIAL_NAME_MAX_LENGTH 64
#define SHADING_MODEL_NAME_MAX_LENGTH 64
#define TEXTURE_NAME_MAX_LENGTH 64
#define FILE_PATH_MAX_LENGTH 512
#define SURFACE_TYPE_NAME_MAX_LENGTH 64
#define MAX_TEXTURE_CONNECTIONS 4

	enum SectionType
	{
		UNKNOWN = 0,
		FILE_INFO = 1,
		MESH = 2,
		MATERIAL = 3,
		TEXTURE = 4,
		MATERIAL_TEXTURE_CONNECTION = 5,

	};

	struct SectionHeader
	{
		SectionType Type;

		SectionHeader(SectionType _type)
		{
			Type = _type;
		}
		SectionHeader()
		{
			Type = UNKNOWN;
		}
	};

	struct FileHeader :SectionHeader
	{
		unsigned int FileFormatVersion;
		FileHeader() :SectionHeader(FILE_INFO){}
	};


	struct MeshHeader:SectionHeader
	{
		char Name[MESH_NAME_MAX_LENGTH];
		unsigned int MeshID;
		unsigned int NumVertices;
		unsigned int HasTangent : 1;
		unsigned int HasBiNormal : 1;
		unsigned int HasWeights : 1;
		unsigned int HasBones : 1;
		unsigned int Padding : 4;

		MeshHeader() :SectionHeader(MESH) {}
	};

	struct Material :SectionHeader
	{
		char Name[MATERIAL_NAME_MAX_LENGTH];
		char ShadingModel[SHADING_MODEL_NAME_MAX_LENGTH];

		float Ambient[3];
		float Diffuse[3];
		float Specular[3];
		float Emissive[3];
		float Opacity;
		float Shininess;
		float Reflectivity;
		unsigned int ID;

		Material() :SectionHeader(MATERIAL) {}
	};

	struct Material_LoaderSide :SectionHeader
	{
		char Name[MATERIAL_NAME_MAX_LENGTH];
		char ShadingModel[SHADING_MODEL_NAME_MAX_LENGTH];

		float Ambient[3];
		float Diffuse[3];
		float Specular[3];
		float Emissive[3];
		float Opacity;
		float Shininess;
		float Reflectivity;
		unsigned int ID;

		unsigned int TextureConnections[MAX_TEXTURE_CONNECTIONS];

		Material_LoaderSide() :SectionHeader(MATERIAL)
		{
			for (int i = 0; i < MAX_TEXTURE_CONNECTIONS; i++)
			{
				TextureConnections[i] = (unsigned int)-1;
			}
		}
	};

	struct Texture :SectionHeader
	{
		unsigned int ID;
		char Name[TEXTURE_NAME_MAX_LENGTH];
		char Path[FILE_PATH_MAX_LENGTH];
		// ... //

		Texture() :SectionHeader(TEXTURE){}
	};

	struct MaterialTextureConnection :SectionHeader
	{
		char SurfaceType[SURFACE_TYPE_NAME_MAX_LENGTH];
		char TextureName[TEXTURE_NAME_MAX_LENGTH];
		unsigned int MaterialID;

		MaterialTextureConnection() :SectionHeader(MATERIAL_TEXTURE_CONNECTION) {}
	};


	struct Model
	{
		unsigned int MaterialID;
		unsigned int VertexDataCount;
		float* VertexData;

	};

	//Only basic data
	struct VertexData_Basic
	{
		float Position[3];
		float Normal[3];
		float UV[2];
	};

	//Has tangent and binormals
	struct VertexData_TBN
	{
		float Position[3];
		float Normal[3];
		float UV[2];

		float Tangent[3];
		float BiNormal[3];
	};

	//Has weights and bone ids
	struct VertexData_Animation
	{
		float Position[3];
		float Normal[3];
		float UV[2];

		float Weights[4];
		unsigned char BoneID[4];
	};

	//Everything is 4 bytes
	struct VertexData_All
	{
		float Position[3];
		float Normal[3];
		float UV[2];

		float Tangent[3];
		float BiNormal[3];

		float Weights[4];
		unsigned int BoneID[4];
	};
}

//#pragma pack(pop)