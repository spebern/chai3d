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
#include "ToolTip.h"
#include "haptic_db_ffi.h"

using namespace chai3d;

class Slave
{
private:
	Network* m_network;
	Spring* m_spring;
	PDController m_pdController;
	PIDController m_pidController;
	RateLimiter m_packetRateLimiter;
	Config* m_config;

	double m_mass = 0.04; // [g]

	int64_t m_sequenceNumber = 0;

	cVector3d m_posRef;
	cVector3d m_pos;
	cVector3d m_prevPos;

	cVector3d m_velRef;
	cVector3d m_vel;
	cVector3d m_prevVel;

	cVector3d m_prevForce;
	cVector3d m_force;

	ToolTip* m_toolTip;

	WAVESlave m_wave;
	PassivityControl m_passivityControl;
	ISS m_iss;

	DB* m_db;
public:
	Slave(Network* network, Spring* spring, Config* config, ToolTip* toolTip, DB* db, const double maxStiffness)
		: m_network(network)
		, m_spring(spring)
		, m_config(config)
		, m_posRef(0, 0, 0)
		, m_pos(0, 0, 0)
		, m_prevPos(0, 0, 0)
		, m_velRef(0, 0, 0)
		, m_vel(0, 0, 0)
		, m_prevVel(0, 0, 0)
		, m_prevForce(0, 0, 0)
		, m_force(0, 0, 0)
		, m_toolTip(toolTip)
		, m_iss(maxStiffness)
		, m_db(db)
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

	void updateToolTipPos() const
	{
		auto const springPos = m_spring->pos();
		const auto width = m_spring->width();
		const auto length = m_spring->length();
		cVector3d newPos(0, m_pos.y() - length / 2 - m_toolTip->radius(), springPos.z());
		m_toolTip->pos(newPos);
	}

	void spring(Spring* spring)
	{
		m_spring = spring;
	}
};
