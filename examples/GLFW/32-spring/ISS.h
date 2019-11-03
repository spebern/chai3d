#pragma once

#include "chai3d.h"
#include "Common.h"

using namespace std;
using namespace chai3d;

class ISS
{
private:
	double m_tau;
	double m_muMax = 0.0;
	double m_muMaxScale;
public:
	explicit ISS(const double tau = 0.03, const double muMaxScale = 1.7)
		: m_tau(tau)
		, m_muMaxScale(muMaxScale)
	{
	}

	cVector3d calculateForce(const cVector3d& force, const cVector3d& dForce) const
	{
		return force + m_tau * dForce;
	}

	cVector3d calculateVel(const cVector3d& vel, const cVector3d& dForce) const
	{
		if (m_muMax == 0.0)
			return vel;
		else
			return vel - dForce / m_muMax;
	}

	void updateMuMax(const cVector3d& force, const cVector3d& const prevForce, const cVector3d& pos, const cVector3d& prevPos)
	{
		const auto muMax = m_muMaxScale * (force - prevForce).length() / (pos - prevPos).length();
		if (muMax > m_muMax)
			m_muMax = muMax;
	}
};
