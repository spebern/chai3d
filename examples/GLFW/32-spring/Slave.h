#pragma once

#include "chai3d.h"
#include "Network.h"
#include "Spring.h"
#include "RateLimiter.h"
#include "PDController.h"
#include "Config.h"

using namespace chai3d;

class Slave
{
private:
	Network* m_network;
	Spring* m_spring;
	PDController m_pdController;
	RateLimiter m_packetRateLimiter;
	Config* m_config;

	double m_mass = 0.1; // [g]
	double m_damping = 0.04;

	int64_t m_sequenceNumber = 0;

	cVector3d m_posRef;
	cVector3d m_pos;

	cVector3d m_velRef;
	cVector3d m_vel;

	cVector3d m_force;

public:
	Slave(Network* network, Spring* spring, Config* config)
		: m_network(network)
		, m_spring(spring)
		, m_config(config)
		, m_posRef(0, 0, 0)
		, m_pos(0, 0, 0)
		, m_velRef(0, 0, 0)
		, m_vel(0, 0, 0)
		, m_force(0, 0, 0)
	{
	}

	void spin();

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
