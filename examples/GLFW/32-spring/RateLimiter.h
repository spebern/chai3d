#pragma once

#include "chrono"
#include "algorithm"

using namespace std;

class RateLimiter
{
private:
	chrono::high_resolution_clock::time_point m_last;
	double m_tokens;
	double m_rate;
public:
	explicit RateLimiter(const double rate = 1000.0): m_tokens(rate), m_rate(rate) {}

	bool limited();

	void rate(const double rate)
	{
		m_rate = rate;
		m_tokens = rate;
	}

	double rate() const
	{
		return m_rate;
	}

	void increaseRate(const double dRate)
	{
		m_rate = min(1000.0, m_rate + dRate);
	}

	void decreaseRate(const double dRate)
	{
		m_rate = max(0.0, m_rate - dRate);
	}
};
