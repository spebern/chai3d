#pragma once

#include "chai3d.h"

using namespace chai3d;

class WAVE
{
private:
	double m_b;
public:
	explicit WAVE(const double b = 3.0): m_b(b) {}

	cVector3d calculateUm(cVector3d& velM, cVector3d& forceM) const
	{
		return (m_b * velM + forceM) / sqrt(2.0 * m_b);
	}

	cVector3d calculateVs(cVector3d& velS, cVector3d& forceS) const
	{
		return (m_b * velS - forceS) / sqrt(2.0 * m_b);
	}

	cVector3d calculateVelM(cVector3d& vm, cVector3d& forceM) const
	{
		return sqrt(2.0 / m_b) * vm + 1.0 / m_b * forceM;
	}

	cVector3d calculateVelS(cVector3d& us, cVector3d& forceS) const
	{
		return sqrt(2.0 / m_b) * us - 1.0 / m_b * forceS;
	}

	cVector3d calculateFm(cVector3d& velM, cVector3d& vm) const
	{
		return m_b * velM - sqrt(2.0 * m_b) * vm;
	}

	cVector3d calculateFs(cVector3d& velS, cVector3d& us) const
	{
		return -m_b * velS + sqrt(2.0 * m_b) * us;
	}
};
