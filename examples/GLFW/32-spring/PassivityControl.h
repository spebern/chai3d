#pragma once

#include "chai3d.h"
#include "Common.h"

using namespace chai3d;

class PassivityControl
{
private:
	double m_alpha;
	double m_energy;
	cVector3d m_prevVel;
public:
	explicit PassivityControl()
		: m_alpha(0)
		, m_energy(0)
		, m_prevVel(0, 0, 0)
	{
	}

	cVector3d calculateForce(const cVector3d& force, const cVector3d& vel)
	{
		m_energy += (force.dot(vel) + m_alpha * m_prevVel.dot(m_prevVel));
		m_prevVel = vel;
		m_alpha = m_energy < 0 ? -m_energy / vel.dot(vel) : 0;
		return force + m_alpha * vel;
	}
};
