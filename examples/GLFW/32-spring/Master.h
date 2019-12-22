#pragma once

#include "chai3d.h"
#include "Network.h"
#include "RateLimiter.h"
#include "Config.h"
#include "WAVE.h"
#include "MMT.h"
#include "haptic_db_ffi.h"
#include "Deadband.h"
#include "Environment.h"

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

	cVector3d m_pos;
	cVector3d m_vel;

	cVector3d m_force;

	WAVEMaster m_wave;
	MMTMaster m_mmt;

	DB* m_db;

	Environment* m_env;

	DeadbandDetector m_deadbandDetector;
public:
	Master(Network* network, const cGenericHapticDevicePtr hapticDevice, Config* config, DB* db, Environment* env)
		: m_network(network)
		, m_hapticDevice(hapticDevice)
		, m_config(config)
		, m_pos(0, 0, 0)
		, m_vel(0, 0, 0)
		, m_force(0, 0, 0)
		, m_db(db)
		, m_env(env)
	{
	}

	void spin();

	void sample();

	static cVector3d limitForce(cVector3d& force)
	{
		return force.length() > MAX_FORCE ? force / force.length() * 6.0 : force;
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
