#include "RateLimiter.h"
#include <chrono>

RateLimiter::RateLimiter(const double rate) {
	rate_ = rate;
	tokens_ = rate;
	last_ = chrono::high_resolution_clock::now();
}

bool RateLimiter::limited() {
	const auto now = chrono::high_resolution_clock::now();
	const auto lapse = chrono::duration_cast<std::chrono::microseconds>(now - last_).count();
	last_ = now;
	tokens_ += double(lapse / 1e-6) * rate_;

	if (tokens_ > rate_)
		tokens_ = rate_;

	if (tokens_ >= 1.0) {
		tokens_ -= 1.0;
		return false;
	}
	return true;
}
