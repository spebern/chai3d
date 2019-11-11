#pragma once

#include "chai3d.h"
#include "Common.h"
#include "Config.h"

using namespace std;
using namespace chai3d;

class ISS
{
private:
	double m_tau;
	double m_muMax;
public:
	explicit ISS(const double maxStiffness, const double muMaxScale = ISS_MU_MAX_SCALE,  const double tau = ISS_TAU)
		: m_tau(tau)
		, m_muMax(maxStiffness / 3.0 * muMaxScale)
	{
	}

	cVector3d calculateForce(const cVector3d& force, const cVector3d& prevForce) const
	{
		return force + m_tau * (force - prevForce) / DT;
	}

	cVector3d calculateVel(const cVector3d& vel, const cVector3d& force, const cVector3d& prevForce) const
	{
		return vel - (force - prevForce) / DT / m_muMax;
	}
};
