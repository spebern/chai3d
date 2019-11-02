#pragma once

#include "chrono"

using namespace std;

class RateLimiter
{
private:
	chrono::high_resolution_clock::time_point m_last;
	double m_tokens;
	double m_rate;
public:
	explicit RateLimiter(double rate = 1000.0): m_tokens(rate), m_rate(rate) {}
	bool limited();
};
