#pragma once

#include "chai3d.h"
#include "Network.h"
#include "RateLimiter.h"
#include "Config.h"
#include "WAVE.h"
#include "ISS.h"

using namespace std;
using namespace chai3d;

class Master
{
private:
	RateLimiter m_packetRateLimiter;
	Network* m_network;
	cGenericHapticDevicePtr m_hapticDevice;
	int64_t m_sequenceNumber = 0;
	Config* m_config;

	cVector3d m_prevPos;
	cVector3d m_prevForce;

	WAVE m_wave;
	ISS m_iss;
public:
	Master(Network* network, const cGenericHapticDevicePtr hapticDevice, Config* config)
		: m_network(network)
		, m_hapticDevice(hapticDevice)
		, m_config(config)
		, m_prevPos(0, 0, 0)
		, m_prevForce(0, 0, 0)
	{
	}

	void spin();

	void sample(cVector3d& pos, cVector3d& vel) const;

	static cVector3d limitForce(cVector3d& force)
	{
		return force.length() > 5.0 ? force / force.length() * 5.0 : force;
	}

	void packetRate(const double rate)
	{
		m_packetRateLimiter.rate(rate);
	}

	double packetRate() const
	{
		return m_packetRateLimiter.rate();
	}

	void increasePacketRate(const double dRate)
	{
		m_packetRateLimiter.increaseRate(dRate);
	}

	void decreasePacketRate(const double dRate)
	{
		m_packetRateLimiter.decreaseRate(dRate);
	}
};
