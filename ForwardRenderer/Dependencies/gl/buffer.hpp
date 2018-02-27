#pragma once
#include "../opengl.h"
#include "id.h"

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
		~Buffer()
		{
			glDeleteBuffers(1, &m_id);
		}
		Buffer(const Buffer&) = delete;
		Buffer& operator=(const Buffer&) = delete;
		Buffer(Buffer&&) = default;
		Buffer& operator=(Buffer&&) = default;

		void bind() const
		{
			glBindBuffer(TType, m_id);
		}
		template<bool TEnabled = (TType == GL_ATOMIC_COUNTER_BUFFER) || (TType == GL_TRANSFORM_FEEDBACK_BUFFER)
		|| (TType == GL_UNIFORM_BUFFER) || (TType == GL_SHADER_STORAGE_BUFFER)>
		std::enable_if_t<TEnabled> bind(GLuint bindingIndex) const
		{
			glBindBufferRange(TType, bindingIndex, m_id, 0, m_size);
		} 
		void bindAsVertexBuffer(GLuint bindingIndex, GLuint elementOffset) const
		{
			glBindVertexBuffer(bindingIndex, m_id, elementOffset * m_elementSize, m_elementSize);
		}

		template<bool TEnabled = (TUsage & GL_DYNAMIC_STORAGE_BIT) != 0>
		std::enable_if_t<TEnabled> subDataUpdate(GLintptr byteOffset, GLsizei size, const GLvoid* data)
		{
			bind();
			glBufferSubData(TType, byteOffset, size, data);
		}
		
		void clear()
		{
			bind();
			uint32_t zero = 0;
			glClearBufferData(TType, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);
		}

	private:
		unique<GLuint> m_id;
		unique<GLsizei> m_elementCount;
		unique<GLsizei> m_elementSize;
		unique<GLsizei> m_size;
	};

	template <GLenum TUsage>
	using ArrayBufferT = Buffer<GL_ARRAY_BUFFER, TUsage>;
	using ArrayBuffer = ArrayBufferT<0>;
	using DynamicArrayBuffer = ArrayBufferT<GL_DYNAMIC_STORAGE_BIT>;

	template <GLenum TUsage>
	using ElementBufferT = Buffer<GL_ELEMENT_ARRAY_BUFFER, TUsage>;
	using ElementBuffer = ElementBufferT<0>;
	using DynamicElementBuffer = ElementBufferT<GL_DYNAMIC_STORAGE_BIT>;

	template <GLenum TUsage>
	using ShaderStorageBufferT = Buffer<GL_SHADER_STORAGE_BUFFER, TUsage>;
	using ShaderStorageBuffer = ShaderStorageBufferT<GL_DYNAMIC_STORAGE_BIT>;

	template <GLenum TUsage>
	using TextureBufferT = Buffer<GL_TEXTURE_BUFFER, TUsage>;

	template <GLenum TUsage>
	using UniformBufferT = Buffer<GL_UNIFORM_BUFFER, TUsage>;

	template <GLenum TUsage>
	using AtomicCounterBufferT = Buffer<GL_ATOMIC_COUNTER_BUFFER, TUsage>;

	template <GLenum TUsage>
	using IndirectDispatchBufferT = Buffer<GL_DISPATCH_INDIRECT_BUFFER, TUsage>;

	template <GLenum TUsage>
	using IndirectDrawBufferT = Buffer<GL_DRAW_INDIRECT_BUFFER, TUsage>;

	template <GLenum TUsage>
	using TransformFeedbackBuffer = Buffer<GL_TRANSFORM_FEEDBACK_BUFFER, TUsage>;
}
