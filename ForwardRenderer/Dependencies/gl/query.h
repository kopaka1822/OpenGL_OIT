#pragma once
#include "id.h"
#include "../opengl.h"
#include <tuple>

namespace gl
{
	template<GLenum TType>
	class Query
	{
		Query(bool) {}
	public:
		static Query empty()
		{
			return Query(false);
		}
		Query()
		{
			glGenQueries(1, &m_id);
		}
		~Query()
		{
			glDeleteQueries(1, &m_id);
		}
		Query(const Query&) = delete;
		Query& operator=(const Query&) = delete;
		Query(Query&&) = default;
		Query& operator=(Query&&) = default;

		bool available() const
		{
			GLuint res = 0;
			glGetQueryObjectuiv(m_id, GL_QUERY_RESULT_AVAILABLE, &res);
			return res != 0;
		}

	protected:

		// \brief 
		// \param wait waiting for the query to finish
		// \return the queried result or 0xFFFFFFFF if not ready
		GLuint receiveUint(bool wait) const
		{
			GLuint res = 0xFFFFFFFF;
			glGetQueryObjectuiv(m_id, wait ? GL_QUERY_RESULT : GL_QUERY_RESULT_NO_WAIT, &res);
			return res;
		}

	protected:
		unique<GLuint> m_id;
	};

	template<GLenum TType>
	class BeginEndQuery : public Query<TType>
	{
	public:
		void begin()
		{
			glBeginQuery(TType, m_id);
		}

		void end()
		{
			glEndQuery(TType);
		}
	};

	template<GLenum TType>
	class IntegerQuery : public BeginEndQuery<TType>
	{
	public:
		// \brief 
		// \param wait waiting for the query to finish
		// \return the queried result or 0xFFFFFFFF if not ready
		std::tuple<bool, GLuint> receive(bool wait) const
		{
			const auto res = Query<TType>::receiveUint(wait);
			return { res != 0xFFFFFFFF, res };
		}
	};

	template<GLenum TType>
	class BooleanQuery : public BeginEndQuery<TType>
	{
	public:

		// \brief 
		// \param wait waiting for the query to finish
		// \return first result = query was ready, second result = query value
		std::tuple<bool, bool> receive(bool wait) const
		{
			const auto res =  Query<TType>::receiveUint(wait);
			return { res != 0xFFFFFFFF, res != 0 };
		}
	};
	

	class SamplesPassedQuery : public IntegerQuery<GL_SAMPLES_PASSED>
	{
		
	};

	class AnySamplesPassedQuery : public BooleanQuery<GL_ANY_SAMPLES_PASSED>
	{
		
	};

	class AnySamplesPassedConservativeQuery : public BooleanQuery<GL_ANY_SAMPLES_PASSED_CONSERVATIVE>
	{
		
	};

	class PrimitivesGeneratedQuery : public IntegerQuery<GL_PRIMITIVES_GENERATED>
	{
		
	};

	class TransformFeedbackPrimivesWrittenQuery : public IntegerQuery<GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN>
	{
		
	};

	class TimeElapsedQuery : public BeginEndQuery<GL_TIME_ELAPSED>
	{
	public:
		
		// \brief elapsed time in milliseconds
		// \param wait waiting for the query to finish
		// \return true if the result was ready and the time in milliseconds
		std::tuple<bool, double> receive(bool wait) const
		{
			const auto res = Query::receiveUint(wait);
			return { res != 0xFFFFFFFF, res * TIME_FACTOR };
		}

	private:
		static constexpr double TIME_FACTOR = 1.0 / 1000000.0;
	};

	class TimestampQuery : public Query<GL_TIMESTAMP>
	{
	public:
		// sets a timestamp for the current point
		void stamp()
		{
			glQueryCounter(m_id, GL_TIMESTAMP);
		}

		// \brief 
		// \param wait waiting for the query to finish
		// \return the queried result or 0xFFFFFFFF if not ready
		std::tuple<bool, GLuint> receive(bool wait) const
		{
			const auto res = Query::receiveUint(wait);
			return { res != 0xFFFFFFFF, res };
		}
	};
}
