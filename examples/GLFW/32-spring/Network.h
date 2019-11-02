#pragma once

#include <chrono>;
#include "chai3d.h"
#include "Channel.h"

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

class Network
{
private:
	chrono::microseconds m_delay;
	chrono::microseconds m_varDelay;
	Channel<HapticMessageM2S> m_channelM2S;
	Channel<HapticMessageS2M> m_channelS2M;
public:
	Network(const chrono::microseconds delay, const chrono::microseconds varDelay)
		: m_delay(delay)
		, m_varDelay(varDelay)
	{
	}

	Network(): m_delay(0), m_varDelay(0)
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
};
