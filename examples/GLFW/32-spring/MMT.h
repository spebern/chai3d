#pragma once

#include "Config.h"
#include "chai3d.h"

using namespace chai3d;

class MMTSlave
{
private:
	double m_k;
	double m_lambda;
public:
	explicit MMTSlave(const double k = MMT_INITIAL_K, const double lambda = MMT_LAMBDA): m_k(k), m_lambda(lambda) {}
	
	void updateK(const double force, cVector3d& pos)
	{
		const auto indention = max(0.0, pos.y() - SPRING_Y);
		const auto estimatedForce = -m_k * indention;
		m_k += m_lambda * (estimatedForce - force);
	}

	double k() const
	{
		return m_k;
	}
};

class MMTMaster
{
private:
	double m_k;
public:
	explicit MMTMaster(const double k = MMT_INITIAL_K): m_k(k) {}

	double k() const
	{
		return m_k;
	}

	void k(const double newK)
	{
		m_k = newK;
	}

	cVector3d calculateForce(cVector3d &pos) const
	{
		const auto indention = max(0.0, pos.y() - SPRING_Y);
		return cVector3d(0, -m_k * indention, 0);
	}
};
