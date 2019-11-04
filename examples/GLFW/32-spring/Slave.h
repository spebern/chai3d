#pragma once

#include "chai3d.h"
#include "Network.h"
#include "Spring.h"
#include "RateLimiter.h"
#include "PDController.h"
#include "Config.h"
#include "WAVE.h"
#include "PassivityControl.h"
#include "ISS.h"
#include "array"

using namespace chai3d;

class Slave
{
private:
	Network* m_network;
	array<Spring*, 4> m_springs;
	PDController m_pdController;
	RateLimiter m_packetRateLimiter;
	Config* m_config;

	double m_mass = 0.1; // [g]
	double m_damping = 0.04;

	int64_t m_sequenceNumber = 0;

	cVector3d m_posRef;
	cVector3d m_pos;
	cVector3d m_prevPos;

	cVector3d m_velRef;
	cVector3d m_vel;
	cVector3d m_prevVel;

	cVector3d m_prevForce;
	cVector3d m_force;

	uint64_t m_currentSpringIndex;

	WAVE m_wave;
	PassivityControl m_passivityControl;
	ISS m_iss;
public:
	Slave(Network* network, array<Spring*, 4> springs, Config* config)
		: m_network(network)
		, m_springs(springs)
		, m_config(config)
		, m_posRef(0, 0, 0)
		, m_pos(0, 0, 0)
		, m_prevPos(0, 0, 0)
		, m_velRef(0, 0, 0)
		, m_vel(0, 0, 0)
		, m_prevVel(0, 0, 0)
		, m_prevForce(0, 0, 0)
		, m_force(0, 0, 0)
		, m_currentSpringIndex(0)
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

	cVector3d dForce() const
	{
		return (m_force - m_prevForce) / DT;
	}

	cVector3d dVel() const
	{
		return (m_vel - m_prevVel) / DT;
	}

	cVector3d dPos() const
	{
		return (m_pos - m_prevPos) / DT;
	}

	Spring* currentSpring()
	{
		return m_springs[m_currentSpringIndex];
	}

	bool nextSpring()
	{
		if (currentSpring()->indented())
			return false;
		if (m_currentSpringIndex == m_springs.size() - 1)
			m_currentSpringIndex = 0;
		else
			m_currentSpringIndex++;
		return true;
	}

	bool prevSpring()
	{
		if (currentSpring()->indented())
			return false;
		if (m_currentSpringIndex == 0)
			m_currentSpringIndex = m_springs.size() - 1;
		else
			m_currentSpringIndex--;
		return true;
	}
};
