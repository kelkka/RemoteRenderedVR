#pragma once
#include "MyFileFormat.h"

namespace AssetLoader
{
	//Loads the model returns the model ID. That model ID is used in GetModel(ID)
	unsigned int LoadModel(const char* _path);

	const MyFileFormat::Model& GetModel(const unsigned int _id);
	const MyFileFormat::Material_LoaderSide& GetMaterial(unsigned int _id);
	const MyFileFormat::Texture& GetTexture(unsigned int _id);
}

