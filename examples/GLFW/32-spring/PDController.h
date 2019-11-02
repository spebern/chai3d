#pragma once

#include "chai3d.h"

using namespace chai3d;

class PDController
{
private:
	double m_kP;
	double m_kD;
public:
	explicit PDController(const double kP = 300.0, const double kD = 1.5): m_kP(kP), m_kD(kD)
	{
	}

	cVector3d calculateForce(cVector3d& posRef, cVector3d& pos, cVector3d& velRef, cVector3d& vel) const
	{
		return m_kP * (posRef - pos) + m_kD * (velRef - vel);
	}
};
