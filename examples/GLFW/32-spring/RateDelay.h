#pragma once

#include "chrono"

using namespace std::chrono;

class RateDelay
{
private:
	microseconds m_m;
	microseconds m_t;
public:
	RateDelay(const microseconds m = microseconds(1000), const microseconds t = microseconds(2000))
		: m_m(m)
		, m_t(t)
	{
	}

	microseconds delay(const uint64_t rate) const
	{
		return m_m * rate + m_t;
	}
};
