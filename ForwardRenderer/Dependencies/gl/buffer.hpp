#pragma once
#include <cassert>
#include "../opengl.h"
#include "id.h"
#include "format.h"

namespace gl
{
	template <GLenum TType, GLenum TUsage>
	class Buffer
	{
	public:
		explicit Buffer() = default;
		explicit Buffer(GLsizei elementSize, GLsizei elementCount = 1, const void* data = nullptr)
			:
		m_elementCount(elementCount),
		m_elementSize(elementSize),
		m_size(elementCount * elementSize)
		{
			glGenBuffers(1, &m_id);
			bind();
			glBufferStorage(TType, m_size, data, TUsage);
		}

		/// \brief 
		/// \param data buffer data
		/// \param elementStride number of components (T) for one element. E.g. vector<float> would have an elementStride of 3 for vec3 elements.
		template<class T>
		explicit Buffer(const std::vector<T>& data, GLsizei elementStride = 1)
			:
		Buffer(sizeof T * elementStride, GLsizei(data.size()) / elementStride, data.data())
		{}

		virtual ~Buffer()
		{
			glDeleteBuffers(1, &m_id);
		}
		Buffer(const Buffer&) = delete;
		Buffer& operator=(const Buffer&) = delete;
		Buffer(Buffer&&) = default;
		Buffer& operator=(Buffer&&) = default;

		void bind() const
		{
			assert(m_id);
			glBindBuffer(TType, m_id);
		}

		template<bool TEnabled = (TType == GL_ATOMIC_COUNTER_BUFFER) || (TType == GL_TRANSFORM_FEEDBACK_BUFFER)
		|| (TType == GL_UNIFORM_BUFFER) || (TType == GL_SHADER_STORAGE_BUFFER) || (TType == GL_ARRAY_BUFFER)>
		std::enable_if_t<TEnabled> bind(GLuint bindingIndex) const
		{
			if (TType == GL_ARRAY_BUFFER)
			{
				bindAsVertexBuffer(bindingIndex);
			}
			else
			{
				bind();
				glBindBufferRange(TType, bindingIndex, m_id, 0, m_size);
			}

		}

		template<bool TEnabled = (TType == GL_ATOMIC_COUNTER_BUFFER) || (TType == GL_TRANSFORM_FEEDBACK_BUFFER)
			|| (TType == GL_UNIFORM_BUFFER) || (TType == GL_SHADER_STORAGE_BUFFER)>
			std::enable_if_t<TEnabled> bind(GLuint bindingIndex, GLuint elementOffset, GLuint numElements) const
		{
			assert(GLsizei(elementOffset + numElements) <= m_elementCount);
			bind();
			glBindBufferRange(TType, bindingIndex, m_id, elementOffset * m_elementSize, numElements * m_elementSize);
		}

		template<bool TEnabled = (TType == GL_ARRAY_BUFFER)>
		std::enable_if_t<TEnabled> bind(GLuint bindingIndex, GLuint elementOffset) const
		{
			bindAsVertexBuffer(bindingIndex, elementOffset);
		}

		void bindAsVertexBuffer(GLuint bindingIndex, GLuint elementOffset = 0) const
		{
			glBindVertexBuffer(bindingIndex, m_id, elementOffset * m_elementSize, m_elementSize);
		}

		template<bool TEnabled = (TUsage & GL_DYNAMIC_STORAGE_BIT) != 0>
		std::enable_if_t<TEnabled> update(const GLvoid* data, GLintptr byteOffset, GLsizei size)
		{
			assert(size - byteOffset <= m_size);
			bind();
			glBufferSubData(TType, byteOffset, size, data);
		}

		template<bool TEnabled = (TUsage & GL_DYNAMIC_STORAGE_BIT) != 0>
		std::enable_if_t<TEnabled> update(const GLvoid* data)
		{
			update(data, 0, m_size);
		}

		template<class T, bool TEnabled = (TUsage & GL_DYNAMIC_STORAGE_BIT) != 0>
		std::enable_if_t<TEnabled> update(const std::vector<T>& data)
		{
			update(data.data(), 0, data.size() * sizeof T);
		}
		
		void clear()
		{
			bind();
			uint32_t zero = 0;
			glClearBufferData(TType, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);
		}

		template <class T>
		void fill(const T& value, InternalFormat bufferInternalFormat, SetDataFormat format, SetDataType type)
		{
			bind();
			glClearBufferData(TType, GLenum(bufferInternalFormat), GLenum(format), GLenum(type), &value);
		}

		template<class T>
		std::vector<T> getData()
		{
			std::vector<T> res;
			res.resize(m_size / sizeof(T));
			bind();
			glGetBufferSubData(TType, 0, m_size, res.data());
			return res;
		}

		template<class T>
		T getElement(GLsizei index)
		{
			T res;
			assert(sizeof T == m_elementSize);
			assert(index < m_elementCount);
			bind();
			glGetBufferSubData(TType, index * m_elementSize, m_elementSize, &res);
			return res;
		}

		GLsizei size() const
		{
			return m_size;
		}

		GLsizei getNumElements() const
		{
			return m_elementCount;
		}

		GLsizei getElementSize() const
		{
			return m_elementSize;
		}

		bool empty() const
		{
			return m_size == 0;
		}

		GLuint getId() const
		{
			return m_id;
		}
	protected:
		unique<GLuint> m_id;
		unique<GLsizei> m_elementCount;
		unique<GLsizei> m_elementSize;
		unique<GLsizei> m_size;
	};

	class TextureBuffer
	{
	public:
		explicit TextureBuffer() = default;
		template <GLenum TType, GLenum TUsage>
		explicit TextureBuffer(TextureBufferFormat format, const Buffer<TType, TUsage>& buffer)
		{
			glGenTextures(1, &m_texId);
			glBindTexture(GL_TEXTURE_BUFFER, m_texId);
			glTexBuffer(GL_TEXTURE_BUFFER, GLenum(format), buffer.getId());
		}

		virtual ~TextureBuffer()
		{
			glDeleteTextures(1, &m_texId);
		}

		TextureBuffer(const TextureBuffer&) = delete;
		TextureBuffer& operator=(const TextureBuffer&) = delete;
		TextureBuffer(TextureBuffer&&) = default;
		TextureBuffer& operator=(TextureBuffer&&) = default;

		void bind(GLuint slot) const
		{
			glActiveTexture(GL_TEXTURE0 + slot);
			glBindTexture(GL_TEXTURE_BUFFER, m_texId);
		}
	private:
		unique<GLuint> m_texId;
	};

	template <GLenum TUsage>
	using ArrayBufferT = Buffer<GL_ARRAY_BUFFER, TUsage>;
	using StaticArrayBuffer = ArrayBufferT<0>;
	using DynamicArrayBuffer = ArrayBufferT<GL_DYNAMIC_STORAGE_BIT>;

	template <GLenum TUsage>
	using ElementBufferT = Buffer<GL_ELEMENT_ARRAY_BUFFER, TUsage>;
	using StaticElementBuffer = ElementBufferT<0>;
	using DynamicElementBuffer = ElementBufferT<GL_DYNAMIC_STORAGE_BIT>;

	template <GLenum TUsage>
	using ShaderStorageBufferT = Buffer<GL_SHADER_STORAGE_BUFFER, TUsage>;
	using StaticShaderStorageBuffer = ShaderStorageBufferT<0>;
	using DynamicShaderStorageBuffer = ShaderStorageBufferT<GL_DYNAMIC_STORAGE_BIT>;
	using StaticClientShaderStorageBuffer = ShaderStorageBufferT<GL_CLIENT_STORAGE_BIT>;

	template <GLenum TUsage>
	using UniformBufferT = Buffer<GL_UNIFORM_BUFFER, TUsage>;
	using StaticUniformBuffer = UniformBufferT<0>;
	using DynamicUniformBuffer = UniformBufferT<GL_DYNAMIC_STORAGE_BIT>;

	template <GLenum TUsage>
	using AtomicCounterBufferT = Buffer<GL_ATOMIC_COUNTER_BUFFER, TUsage>;
	using StaticAtomicCounterBuffer = AtomicCounterBufferT<0>;
	using DynamicAtomicCounterBuffer = AtomicCounterBufferT<GL_DYNAMIC_STORAGE_BIT>;

	template <GLenum TUsage>
	using IndirectDispatchBufferT = Buffer<GL_DISPATCH_INDIRECT_BUFFER, TUsage>;

	template <GLenum TUsage>
	using IndirectDrawBufferT = Buffer<GL_DRAW_INDIRECT_BUFFER, TUsage>;

	template <GLenum TUsage>
	using TransformFeedbackBuffer = Buffer<GL_TRANSFORM_FEEDBACK_BUFFER, TUsage>;
}
