#include "RateLimiter.h"

bool RateLimiter::limited()
{
	const auto now = chrono::high_resolution_clock::now();
	const auto lapse = chrono::duration_cast<std::chrono::microseconds>(now - m_last).count();
	m_last = now;
	m_tokens += double(lapse / 1e-6) * m_rate;

	if (m_tokens > m_rate)
		m_tokens = m_rate;

	if (m_tokens >= 1.0)
	{
		m_tokens -= 1.0;
		return false;
	}
	return true;
}
