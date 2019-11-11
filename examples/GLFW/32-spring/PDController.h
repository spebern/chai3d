#pragma once

#include "chai3d.h"
#include "Config.h"

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

class PIDController
{
	double m_kP;
	double m_kI;
	double m_kD;
	cVector3d m_integralError;
	cVector3d m_prevError;
public:
	explicit PIDController(const double kP = 300.0, const double kI = 1.0, const double kD = 1.5)
		: m_kP(kP)
		, m_kI(kI)
		, m_kD(kD)
	{
	}

	cVector3d calculateForce(cVector3d& posRef, cVector3d& pos) 
	{
		const auto error = posRef - pos;
		m_integralError += error * DT;
		const auto compP = error * m_kP;
		const auto compI = m_integralError * m_kI;
		const auto compD = ((error - m_prevError) / DT) * m_kD;
		m_prevError = error;
		return compP + compI + compD;
	}

};
