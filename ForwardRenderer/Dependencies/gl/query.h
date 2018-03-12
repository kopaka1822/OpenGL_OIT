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

		GLuint getId() const
		{
			return m_id;
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
		BeginEndQuery(Query<TType>&& q) :
		Query(std::move(q)) {}
	public:
		static BeginEndQuery<TType> empty()
		{
			return BeginEndQuery(Query<TType>::empty());
		}
		BeginEndQuery() = default;
		~BeginEndQuery() = default;
		BeginEndQuery(const BeginEndQuery&) = delete;
		BeginEndQuery& operator=(const BeginEndQuery&) = delete;
		BeginEndQuery(BeginEndQuery&&) = default;
		BeginEndQuery& operator=(BeginEndQuery&&) = default;

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
		~IntegerQuery() = default;
		IntegerQuery(const IntegerQuery&) = delete;
		IntegerQuery& operator=(const IntegerQuery&) = delete;
		IntegerQuery(IntegerQuery&&) = default;
		IntegerQuery& operator=(IntegerQuery&&) = default;

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
		~BooleanQuery() = default;
		BooleanQuery(const BooleanQuery&) = delete;
		BooleanQuery& operator=(const BooleanQuery&) = delete;
		BooleanQuery(BooleanQuery&&) = default;
		BooleanQuery& operator=(BooleanQuery&&) = default;

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
	public:
		~SamplesPassedQuery() = default;
		SamplesPassedQuery(const SamplesPassedQuery&) = delete;
		SamplesPassedQuery& operator=(const SamplesPassedQuery&) = delete;
		SamplesPassedQuery(SamplesPassedQuery&&) = default;
		SamplesPassedQuery& operator=(SamplesPassedQuery&&) = default;
	};

	class AnySamplesPassedQuery : public BooleanQuery<GL_ANY_SAMPLES_PASSED>
	{
	public:
		~AnySamplesPassedQuery() = default;
		AnySamplesPassedQuery(const AnySamplesPassedQuery&) = delete;
		AnySamplesPassedQuery& operator=(const AnySamplesPassedQuery&) = delete;
		AnySamplesPassedQuery(AnySamplesPassedQuery&&) = default;
		AnySamplesPassedQuery& operator=(AnySamplesPassedQuery&&) = default;
	};

	class AnySamplesPassedConservativeQuery : public BooleanQuery<GL_ANY_SAMPLES_PASSED_CONSERVATIVE>
	{
	public:
		~AnySamplesPassedConservativeQuery() = default;
		AnySamplesPassedConservativeQuery(const AnySamplesPassedConservativeQuery&) = delete;
		AnySamplesPassedConservativeQuery& operator=(const AnySamplesPassedConservativeQuery&) = delete;
		AnySamplesPassedConservativeQuery(AnySamplesPassedConservativeQuery&&) = default;
		AnySamplesPassedConservativeQuery& operator=(AnySamplesPassedConservativeQuery&&) = default;
	};

	class PrimitivesGeneratedQuery : public IntegerQuery<GL_PRIMITIVES_GENERATED>
	{
	public:
		~PrimitivesGeneratedQuery() = default;
		PrimitivesGeneratedQuery(const PrimitivesGeneratedQuery&) = delete;
		PrimitivesGeneratedQuery& operator=(const PrimitivesGeneratedQuery&) = delete;
		PrimitivesGeneratedQuery(PrimitivesGeneratedQuery&&) = default;
		PrimitivesGeneratedQuery& operator=(PrimitivesGeneratedQuery&&) = default;
	};

	class TransformFeedbackPrimivesWrittenQuery : public IntegerQuery<GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN>
	{
	public:
		~TransformFeedbackPrimivesWrittenQuery() = default;
		TransformFeedbackPrimivesWrittenQuery(const TransformFeedbackPrimivesWrittenQuery&) = delete;
		TransformFeedbackPrimivesWrittenQuery& operator=(const TransformFeedbackPrimivesWrittenQuery&) = delete;
		TransformFeedbackPrimivesWrittenQuery(TransformFeedbackPrimivesWrittenQuery&&) = default;
		TransformFeedbackPrimivesWrittenQuery& operator=(TransformFeedbackPrimivesWrittenQuery&&) = default;
	};

	class TimeElapsedQuery : public BeginEndQuery<GL_TIME_ELAPSED>
	{
		TimeElapsedQuery(BeginEndQuery<GL_TIME_ELAPSED>&& q)
			:
		BeginEndQuery(std::move(q))
		{}
	public:
		static TimeElapsedQuery empty()
		{
			return TimeElapsedQuery(BeginEndQuery<GL_TIME_ELAPSED>::empty());
		}
		TimeElapsedQuery() = default;
		~TimeElapsedQuery() = default;
		TimeElapsedQuery(const TimeElapsedQuery&) = delete;
		TimeElapsedQuery& operator=(const TimeElapsedQuery&) = delete;
		TimeElapsedQuery(TimeElapsedQuery&&) = default;
		TimeElapsedQuery& operator=(TimeElapsedQuery&&) = default;

		// \brief elapsed time in milliseconds
		// \param wait waiting for the query to finish
		// \return true if the result was ready and the time in milliseconds
		std::tuple<bool, double> receive(bool wait) const
		{
			const auto res = Query::receiveUint(wait);
			return { res != 0xFFFFFFFF, res * TIME_FACTOR };
		}

		// \brief elapsed time in milliseconds
		// \param wait waiting for the query to finish
		// \return true if the result was ready and the time
		std::tuple<bool, GLuint> receiveRaw(bool wait) const
		{
			const auto res = Query::receiveUint(wait);
			return { res != 0xFFFFFFFF, res };
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

		~TimestampQuery() = default;
		TimestampQuery(const TimestampQuery&) = delete;
		TimestampQuery& operator=(const TimestampQuery&) = delete;
		TimestampQuery(TimestampQuery&&) = default;
		TimestampQuery& operator=(TimestampQuery&&) = default;

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
