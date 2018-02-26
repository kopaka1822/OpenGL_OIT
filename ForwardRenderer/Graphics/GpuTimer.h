#pragma once
#include <algorithm>
#include <vector>
#include <glad/glad.h>
#include <cassert>
#include <queue>
#include "../Framework/Profiler.h"

class GpuTimer
{
public:
	// generate some queries
	GpuTimer(GLsizei capacity = 5)
	{
		m_freeQueries.resize(5);
		glGenQueries(capacity, m_freeQueries.data());
	}
	~GpuTimer()
	{
		while (!m_runningQueries.empty())
		{
			m_freeQueries.push_back(m_runningQueries.front());
			m_runningQueries.pop();
		}
		if(m_currentQuery)
		{
			m_freeQueries.push_back(m_currentQuery);
			m_currentQuery = 0;
		}
		glDeleteQueries(GLsizei(m_freeQueries.size()), m_freeQueries.data());
		m_freeQueries.clear();
	}
	void lock()
	{
		assert(m_currentQuery == 0);
		// no other timer may run
		assert(curTimer() == nullptr);

		if(!m_freeQueries.empty())
		{
			m_currentQuery = m_freeQueries.back();
			m_freeQueries.pop_back();
			glBeginQuery(GL_TIME_ELAPSED, m_currentQuery);
			curTimer() = this;
		}
	}
	void unlock()
	{
		assert(m_currentQuery != 0);
		// timer should have been started
		assert(curTimer() == this);

		glEndQuery(GL_TIME_ELAPSED);
		m_runningQueries.push(m_currentQuery);
		m_currentQuery = 0;
		curTimer() = nullptr;
		receive();
	}
	Profiler::Profile get() const
	{
		return {
			min(), max(), latest(), average()
		};
	}
	double average() const
	{
		return m_sumTime / TIME_DIVIDE / m_sumCount;
	}
	double latest() const
	{
		return m_latestTime / TIME_DIVIDE;
	}
	double max() const
	{
		return m_maxTime / TIME_DIVIDE;
	}
	double min() const
	{
		return m_minTime / TIME_DIVIDE;
	}
	void receive()
	{
		while(!m_runningQueries.empty())
		{
			auto id = m_runningQueries.front();
			GLuint res = GLuint(-1);
			glGetQueryObjectuiv(id, GL_QUERY_RESULT_NO_WAIT, &res);
			if (res == GLuint(-1))
				return; // query was not ready

			// update statistics
			m_latestTime = size_t(res);
			m_sumTime += size_t(res);
			++m_sumCount;
			m_minTime = (std::min)(m_latestTime, m_minTime);
			m_maxTime = (std::max)(m_latestTime, m_maxTime);
			m_runningQueries.pop();
			m_freeQueries.push_back(id);
		}
	}

private:
	// this assures that only one timer is running at a time
	static GpuTimer*& curTimer()
	{
		static GpuTimer* ptr = nullptr;
		return ptr;
	}

private:
	std::queue<GLuint> m_runningQueries;
	std::vector<GLuint> m_freeQueries;
	GLuint m_currentQuery = 0;
	size_t m_sumTime = 0;
	size_t m_sumCount = 0;
	size_t m_latestTime = 0;
	size_t m_minTime = size_t(-1);
	size_t m_maxTime = 0;
	static constexpr double TIME_DIVIDE = 1000000.0;
};
