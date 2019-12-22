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
#include "haptic_db_ffi.h"
#include "MMT.h"
#include "Deadband.h"
#include "Environment.h"

using namespace chai3d;

class Slave
{
private:
	Network* m_network;
	Spring m_spring;
	PDController m_pdController;
	PIDController m_pidController;
	RateLimiter m_packetRateLimiter;
	Config* m_config;

	double m_mass = 0.04; // [g]
	double m_damping;

	int64_t m_sequenceNumber = 0;

	cVector3d m_posRef;
	cVector3d m_pos;
	cVector3d m_prevPos;

	cVector3d m_velRef;
	cVector3d m_vel;
	cVector3d m_prevVel;

	cVector3d m_prevForce;
	cVector3d m_force;

	WAVESlave m_wave;
	PassivityControl m_passivityControl;
	ISS m_iss;
	MMTSlave m_mmt;

	DeadbandDetector m_deadbandDetector;

	DB* m_db;

	Environment* m_env;
public:
	Slave(Network* network, Config* config, DB* db, const double maxStiffness, Environment* env,
	      const double mass = SLAVE_MASS, const double damping = SLAVE_DAMPING)
		: m_network(network)
		, m_config(config)
		, m_mass(mass)
		, m_damping(damping)
		, m_posRef(0, 0, 0)
		, m_pos(0, 0, 0)
		, m_prevPos(0, 0, 0)
		, m_velRef(0, 0, 0)
		, m_vel(0, 0, 0)
		, m_prevVel(0, 0, 0)
		, m_prevForce(0, 0, 0)
		, m_force(0, 0, 0)
		, m_iss(maxStiffness)
		, m_db(db)
	    , m_env(env)
		, m_spring()
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

	cVector3d dVel() const
	{
		return (m_vel - m_prevVel) / DT;
	}

	cVector3d dPos() const
	{
		return (m_pos - m_prevPos) / DT;
	}
};
