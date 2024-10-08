#pragma once
#include "../opengl.h"

namespace gl
{
	enum class InternalFormat
	{
		// One channel
		R8 = GL_R8,
		R8S = GL_R8_SNORM,
		R8I = GL_R8I,
		R8UI = GL_R8UI,
		R16 = GL_R16,
		R16S = GL_R16_SNORM,
		R16I = GL_R16I,
		R16UI = GL_R16UI,
		R16F = GL_R16F,
		R32I = GL_R32I,
		R32UI = GL_R32UI,
		R32F = GL_R32F,

		// Two channels
		RG8 = GL_RG8,
		RG8S = GL_RG8_SNORM,
		RG8I = GL_RG8I,
		RG8UI = GL_RG8UI,
		RG16 = GL_RG16,
		RG16S = GL_RG16_SNORM,
		RG16I = GL_RG16I,
		RG16UI = GL_RG16UI,
		RG16F = GL_RG16F,
		RG32I = GL_RG32I,
		RG32UI = GL_RG32UI,
		RG32F = GL_RG32F,

		// Three channels
		R3_G3_B2 = GL_R3_G3_B2,
		RGB4 = GL_RGB4,
		RGB5 = GL_RGB5,
		RGB8 = GL_RGB8,
		RGB8S = GL_RGB8_SNORM,
		RGB8I = GL_RGB8I,
		RGB8UI = GL_RGB8UI,
		SRGB8 = GL_SRGB8,
		RGB10 = GL_RGB10,
		RGB12 = GL_RGB12,
		RGB16 = GL_RGB16,
		RGB16S = GL_RGB16_SNORM,
		RGB16I = GL_RGB16I,
		RGB16UI = GL_RGB16UI,
		RGB16F = GL_RGB16F,
		RGB32I = GL_RGB32I,
		RGB32UI = GL_RGB32UI,
		RGB32F = GL_RGB32F,
		R11F_G11F_B10F = GL_R11F_G11F_B10F,
		RGB9_E5 = GL_RGB9_E5,

		// Four channels
		RGBA2 = GL_RGBA2,
		RGBA4 = GL_RGBA4,
		RGB5_A1 = GL_RGB5_A1,
		RGBA8 = GL_RGBA8,
		RGBA8S = GL_RGBA8_SNORM,
		RGBA8I = GL_RGBA8I,
		RGBA8UI = GL_RGBA8UI,
		SRGB8_ALPHA8 = GL_SRGB8_ALPHA8,
		RGB10_A2 = GL_RGB10_A2,
		RGB10_A2UI = GL_RGB10_A2UI,
		RGBA12 = GL_RGBA12,
		RGBA16 = GL_RGBA16,
		RGBA16S = GL_RGBA16_SNORM,
		RGBA16I = GL_RGBA16I,
		RGBA16UI = GL_RGBA16UI,
		RGBA16F = GL_RGBA16F,
		RGBA32I = GL_RGBA32I,
		RGBA32UI = GL_RGBA32UI,
		RGBA32F = GL_RGBA32F,

		// Depth stencil formats
		DEPTH_COMPONENT32F = GL_DEPTH_COMPONENT32F,
		DEPTH_COMPONENT24 = GL_DEPTH_COMPONENT24,
		DEPTH_COMPONENT16 = GL_DEPTH_COMPONENT16,
		DEPTH32F_STENCIL8 = GL_DEPTH32F_STENCIL8,
		DEPTH24_STENCIL8 = GL_DEPTH24_STENCIL8,
		STENCIL_INDEX8 = GL_STENCIL_INDEX8,
	};

	// Possible data formats for setData functions.
	enum class SetDataType
	{
		UINT8 = GL_UNSIGNED_BYTE,
		INT8 = GL_BYTE,
		UINT16 = GL_UNSIGNED_SHORT,
		INT16 = GL_SHORT,
		UINT32 = GL_UNSIGNED_INT,
		INT32 = GL_INT,
		FLOAT = GL_FLOAT,
		UNSIGNED_BYTE_3_3_2 = GL_UNSIGNED_BYTE_3_3_2,
		UNSIGNED_BYTE_2_3_3_REV = GL_UNSIGNED_BYTE_2_3_3_REV,
		UNSIGNED_SHORT_5_6_5 = GL_UNSIGNED_SHORT_5_6_5,
		UNSIGNED_SHORT_5_6_5_REV = GL_UNSIGNED_SHORT_5_6_5_REV,
		UNSIGNED_SHORT_4_4_4_4 = GL_UNSIGNED_SHORT_4_4_4_4,
		UNSIGNED_SHORT_4_4_4_4_REV = GL_UNSIGNED_SHORT_4_4_4_4_REV,
		UNSIGNED_SHORT_5_5_5_1 = GL_UNSIGNED_SHORT_5_5_5_1,
		UNSIGNED_SHORT_1_5_5_5_REV = GL_UNSIGNED_SHORT_1_5_5_5_REV,
		UNSIGNED_INT_8_8_8_8 = GL_UNSIGNED_INT_8_8_8_8,
		UNSIGNED_INT_8_8_8_8_REV = GL_UNSIGNED_INT_8_8_8_8_REV,
		UNSIGNED_INT_10_10_10_2 = GL_UNSIGNED_INT_10_10_10_2,
		UNSIGNED_INT_2_10_10_10_REV = GL_UNSIGNED_INT_2_10_10_10_REV
	};

	// Possible data formats for setData functions.
	enum class SetDataFormat
	{
		R = GL_RED,
		RG = GL_RG,
		RGB = GL_RGB,
		BGR = GL_BGR,
		RGBA = GL_RGBA,
		BGRA = GL_BGRA,
		DEPTH_COMPONENT = GL_DEPTH_COMPONENT,
		STENCIL_INDEX = GL_STENCIL_INDEX,
		R_INTEGER = GL_RED_INTEGER,
		RG_INTEGER = GL_RG_INTEGER,
		RGB_INTEGER = GL_RGB_INTEGER,
		RGBA_INTEGER = GL_RGBA_INTEGER,
		BGR_INTEGER = GL_BGR_INTEGER,
		BGRA_INTEGER = GL_BGRA_INTEGER
	};

	enum class ImageAccess
	{
		READ_ONLY = GL_READ_ONLY,
		WRITE_ONLY = GL_WRITE_ONLY,
		READ_WRITE = GL_READ_WRITE
	};

	enum class VertexType
	{
		INT8 = GL_BYTE,
		UINT8 = GL_UNSIGNED_BYTE,
		INT16 = GL_SHORT,
		UINT16 = GL_UNSIGNED_SHORT,
		INT32 = GL_INT,
		UINT32 = GL_UNSIGNED_INT,

		FIXED = GL_FIXED,

		FLOAT = GL_FLOAT,
		HALF = GL_HALF_FLOAT,
		DOUBLE = GL_DOUBLE,

		INT_2_10_10_10 = GL_INT_2_10_10_10_REV,
		UINT_2_10_10_10 = GL_UNSIGNED_INT_10_10_10_2,
		UINT_10F_11F_11F = GL_UNSIGNED_INT_10F_11F_11F_REV,
	};

	// Buffers can only use a restricted set of the internal formats
	enum class TextureBufferFormat
	{
		// One channel
		R8 = GL_R8,
		R8I = GL_R8I,
		R8UI = GL_R8UI,
		R16 = GL_R16,
		R16I = GL_R16I,
		R16UI = GL_R16UI,
		R16F = GL_R16F,
		R32I = GL_R32I,
		R32UI = GL_R32UI,
		R32F = GL_R32F,

		// Two channels
		RG8 = GL_RG8,
		RG8I = GL_RG8I,
		RG8UI = GL_RG8UI,
		RG16 = GL_RG16,
		RG16I = GL_RG16I,
		RG16UI = GL_RG16UI,
		RG16F = GL_RG16F,
		RG32I = GL_RG32I,
		RG32UI = GL_RG32UI,
		RG32F = GL_RG32F,

		// Three channels
		RGB32F = GL_RGB32F,
		RGB32I = GL_RGB32I,
		RGB32UI = GL_RGB32UI,

		// Four channels
		RGBA8 = GL_RGBA8,
		RGBA8I = GL_RGBA8I,
		RGBA8UI = GL_RGBA8UI,
		RGBA16 = GL_RGBA16,
		RGBA16I = GL_RGBA16I,
		RGBA16UI = GL_RGBA16UI,
		RGBA16F = GL_RGBA16F,
		RGBA32I = GL_RGBA32I,
		RGBA32UI = GL_RGBA32UI,
		RGBA32F = GL_RGBA32F,
	};

	inline bool isDepthFormat(InternalFormat format)
	{
		switch (format)
		{
		case InternalFormat::DEPTH24_STENCIL8:
		case InternalFormat::DEPTH32F_STENCIL8:
		case InternalFormat::DEPTH_COMPONENT16:
		case InternalFormat::DEPTH_COMPONENT24:
		case InternalFormat::DEPTH_COMPONENT32F:
			return true;
		default:
			return false;
		}
	}

	inline bool isStencilFormat(InternalFormat format)
	{
		switch (format)
		{
		case InternalFormat::DEPTH24_STENCIL8:
		case InternalFormat::DEPTH32F_STENCIL8:
		case InternalFormat::STENCIL_INDEX8:
			return true;
		default:
			return false;
		}
	}

	inline bool isSignedFormat(InternalFormat format)
	{
		switch (format)
		{
		case InternalFormat::R8S:
		case InternalFormat::R16S:
		case InternalFormat::RG8S:
		case InternalFormat::RG16S:
		case InternalFormat::RGB8S:
		case InternalFormat::RGB16S:
		case InternalFormat::RGBA8S:
		case InternalFormat::RGBA16S:
			return true;
		default:
			return false;
		}
	}

	inline bool isIntegerType(VertexType t)
	{
		switch (t)
		{
		case VertexType::INT8:
		case VertexType::UINT8:
		case VertexType::INT16:
		case VertexType::UINT16:
		case VertexType::INT32:
		case VertexType::UINT32:
			return true;
		default:
			return false;
		}
	}
}