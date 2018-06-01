#pragma once
#include "IModel.h"
#include "ICamera.h"
#include "ILights.h"
#include "ITransforms.h"
#include "IEnvironmentMap.h"
#include "IShadows.h"

struct RenderArgs
{
	const IModel* model = nullptr;
	ICamera* camera = nullptr;
	ILights* lights = nullptr;
	ITransforms* transforms = nullptr;
	const IEnvironmentMap* environment = nullptr;
	const IShadows* shadows = nullptr;

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
			environment != nullptr &&
			shadows != nullptr;
	}

	bool hasNull() const
	{
		return !isNotNull();
	}

	// binds lights shadows and envmap (relevant light information)
	void bindLightData() const
	{
		if (lights)
			lights->bind();
		if (shadows)
			shadows->bind();
		if (environment)
			environment->bind();
		if (transforms)
			transforms->bind();
	}
};
