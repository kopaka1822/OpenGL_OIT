#pragma once
#include "IModel.h"
#include "ICamera.h"
#include "ILights.h"
#include "ITransforms.h"
#include "IEnvironmentMap.h"

struct RenderArgs
{
	const IModel* model = nullptr;
	ICamera* camera = nullptr;
	ILights* lights = nullptr;
	ITransforms* transforms = nullptr;
	const IEnvironmentMap* environment = nullptr;

	/**
	 * \brief 
	 * \return true if all components are not null
	 */
	bool isNotNull() const
	{
		return 
			model != nullptr &&
			camera != nullptr &&
			lights != nullptr &&
			transforms != nullptr &&
			environment != nullptr;
	}

	bool hasNull() const
	{
		return !isNotNull();
	}
};
