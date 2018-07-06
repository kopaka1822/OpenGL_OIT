#pragma once
#include <algorithm>
#include <vector>
#include <glad/glad.h>
#include <cassert>
#include <queue>
#include "../Framework/Profiler.h"
#include "../Dependencies/gl/query.h"

class GpuTimer
{
public:
	// generate some queries
	GpuTimer(GLsizei capacity = 5)
	{
		m_freeQueries.resize(5);
	}
	void lock()
	{
		assert(m_currentQuery.getId() == 0);
		// no other timer may run
		assert(curTimer() == nullptr);

		if(!m_freeQueries.empty())
		{
			m_currentQuery = std::move(m_freeQueries.back());
			m_freeQueries.pop_back();
			m_currentQuery.begin();
			curTimer() = this;
		}
	}
	void unlock()
	{
		assert(m_currentQuery.getId() != 0);
		// timer should have been started
		assert(curTimer() == this);

		glEndQuery(GL_TIME_ELAPSED);
		m_runningQueries.push(std::move(m_currentQuery));
		curTimer() = nullptr;
		receive();
	}
	Profiler::Profile get() const
	{
		return {
			min(), max(), latest(), average(), median()
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
	double median() const
	{
		//return m_weighted;
		if (m_lastValues.empty()) return 0.0;
		return m_lastValues[m_lastValues.size() / 2] / TIME_DIVIDE;
		//return std::accumulate(m_lastValues.begin(), m_lastValues.end(), size_t(0)) / m_lastValues.size() / TIME_DIVIDE;
	}
	void receive()
	{
		while(!m_runningQueries.empty())
		{
			auto [ready, res] = m_runningQueries.front().receiveRaw(false);
			if (!ready)
				return; // query was not ready

			// update statistics
			m_latestTime = size_t(res);
			m_sumTime += size_t(res);
			++m_sumCount;
			m_minTime = (std::min)(m_latestTime, m_minTime);
			m_maxTime = (std::max)(m_latestTime, m_maxTime);
			updateMedian(size_t(res));
			if (m_weighted == 0.0)
				m_weighted = res / TIME_DIVIDE;
			else
				m_weighted = m_weighted * 0.99 + res / TIME_DIVIDE * 0.01;

			auto q = std::move(m_runningQueries.front());
			m_runningQueries.pop();
			m_freeQueries.push_back(std::move(q));
		}
	}

private:
	// this assures that only one timer is running at a time
	static GpuTimer*& curTimer()
	{
		static GpuTimer* ptr = nullptr;
		return ptr;
	}
	void updateMedian(size_t time)
	{
		// sortedinsert
		m_lastValues.insert(
			std::upper_bound(m_lastValues.begin(), m_lastValues.end(), time),
			time
		);
		if (m_lastValues.size() > MAX_LAST_VALUES)
			m_lastValues.pop_back();
	}
private:
	std::queue<gl::TimeElapsedQuery> m_runningQueries;
	std::vector<gl::TimeElapsedQuery> m_freeQueries;
	gl::TimeElapsedQuery m_currentQuery = gl::TimeElapsedQuery::empty();
	size_t m_sumTime = 0;
	size_t m_sumCount = 0;
	size_t m_latestTime = 0;
	size_t m_minTime = size_t(-1);
	size_t m_maxTime = 0;
	double m_weighted = 0.0;

	// this is used for the median
	std::vector<size_t> m_lastValues;
	inline static size_t MAX_LAST_VALUES = 4096;
	static constexpr double TIME_DIVIDE = 1000000.0;
};
