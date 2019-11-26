#pragma once

#include <chrono>;
#include <random>;
#include "chai3d.h"
#include "Channel.h"
#include "haptic_db_ffi.h"

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
};

inline DBHapticMessageM2S hapticMessageM2StoDbMsg(HapticMessageM2S& msg)
{
	DBHapticMessageM2S dbMsg{};
	dbMsg.sequenceNumber = msg.sequenceNumber;
	for (auto i = 0; i < 3; i++)
	{
		dbMsg.pos[i] = msg.pos.get(i);
		dbMsg.vel[i] = msg.vel.get(i);
	}
	return dbMsg;
}

inline DBHapticMessageS2M hapticMessageS2MtoDbMsg(HapticMessageS2M& msg)
{
	DBHapticMessageS2M dbMsg;
	dbMsg.sequenceNumber = msg.sequenceNumber;
	for (auto i = 0; i < 3; i++)
	{
		dbMsg.force[i] = msg.force.get(i);
	}
	return dbMsg;
}

class Network
{
private:
	chrono::microseconds m_delay;
	double m_varDelayPercentage;
	Channel<HapticMessageM2S> m_channelM2S;
	Channel<HapticMessageS2M> m_channelS2M;

	std::default_random_engine m_generator;
	std::normal_distribution<double> m_dist;

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

	void increaseDelay(const chrono::microseconds dDelay)
	{
		const auto newDelay = min(chrono::microseconds(200000), m_delay + dDelay);
		delay(newDelay);
	}

	void decreaseDelay(const chrono::microseconds dDelay)
	{
		const auto newDelay = max(chrono::microseconds(0), m_delay - dDelay);
		delay(newDelay);
	}

	void delay(const chrono::microseconds delay)
	{
		m_delay = delay;
		const normal_distribution<double> dist(
			0.0, 
			static_cast<double>(delay.count())* m_varDelayPercentage
		);
		m_dist = dist;
	}

	chrono::microseconds delay() const
	{
		return m_delay;
	}
};
