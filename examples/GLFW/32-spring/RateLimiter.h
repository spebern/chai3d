#ifndef RATELIMITER_H
#define RATELIMITER_H

#include <chrono>

using namespace std;

class RateLimiter {
private:
	chrono::high_resolution_clock::time_point last_;
	double tokens_;
	double rate_;
public:
	explicit RateLimiter(double rate);
	bool limited();
};

#endif