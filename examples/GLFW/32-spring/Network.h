#pragma once

#include <chrono>;
#include <random>;
#include "chai3d.h"
#include "Channel.h"
#include "mutex"

using namespace std;
using namespace chai3d;

struct HapticMessageM2S
{
	int32_t sequenceNumber;
	cVector3d vel;
	cVector3d pos;
};

struct HapticMessageS2M
{
	int32_t sequenceNumber;
	cVector3d force;
	cVector3d pos;
};

class Network
{
private:
	chrono::microseconds m_delay;
	double m_varDelayPercentage;
	Channel<HapticMessageM2S> m_channelM2S;
	Channel<HapticMessageS2M> m_channelS2M;

	std::default_random_engine m_generator;
	std::normal_distribution<double> m_dist;

	std::mutex m_mu;

	chrono::microseconds sampleDelay() {
		return m_delay + chrono::microseconds(static_cast<int64_t>(m_dist(m_generator)));
	}

public:
		Network(const chrono::microseconds delay, const double varDelayPercentage)
		: m_delay(delay)
		, m_varDelayPercentage(varDelayPercentage)
		, m_dist(0.0, static_cast<double>(delay.count()) * varDelayPercentage)
	{
	}

	Network(): m_delay(0), m_varDelayPercentage(0.0), m_dist(0, 0)
	{
	}

	void sendM2S(HapticMessageM2S& msg)
	{
		m_channelM2S.send(msg, m_delay);
	}

	void sendS2M(HapticMessageS2M& msg)
	{
		m_channelS2M.send(msg, m_delay);
	}

	bool tryReceiveM2S(HapticMessageM2S& msg)
	{
		return m_channelM2S.tryReceive(msg);
	}

	bool tryReceiveS2M(HapticMessageS2M& msg)
	{
		return m_channelS2M.tryReceive(msg);
	}

	void clearChannels()
	{
		m_channelM2S.clear();
		m_channelS2M.clear();
	}

	void delay(const chrono::microseconds delay)
	{
		std::unique_lock<std::mutex> lock(m_mu);
		m_delay = delay;
		const normal_distribution<double> dist(
			0.0, 
			static_cast<double>(delay.count())* m_varDelayPercentage
		);
		m_dist = dist;
	}

	chrono::microseconds delay() 
	{
		std::unique_lock<std::mutex> lock(m_mu);
		return m_delay;
	}
};
