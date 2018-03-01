#pragma once
#include "../Dependencies/gl/sampler.hpp"
#include <map>

class SamplerCache
{
	using SamplerDesc = std::tuple<gl::MinFilter, gl::MagFilter, gl::MipFilter, gl::BorderHandling, gl::DepthCompareFunc, std::array<float, 4>>;
public:
	SamplerCache() = delete;

	static gl::Sampler& getSampler(gl::MinFilter minFilter, gl::MagFilter magFilter, gl::MipFilter mipFilter = gl::MipFilter::NONE,
	                        gl::BorderHandling border = gl::BorderHandling::REPEAT, gl::DepthCompareFunc depthFunc = gl::DepthCompareFunc::DISABLE, std::array<float, 4> borderColor = { 0.0f, 0.0f, 0.0f, 0.0f })
	{
		static std::map<SamplerDesc, gl::Sampler> s_samplers;

		const auto d = std::make_tuple(minFilter, magFilter, mipFilter, border, depthFunc, borderColor);
		const auto it = s_samplers.find(d);
		if (it != s_samplers.end())
			return it->second;

		// insert new sampler
		return s_samplers[d] = gl::Sampler(minFilter, magFilter, mipFilter, border, depthFunc, borderColor);
	}
};
