#pragma once

#include "chai3d.h"
#include "Config.h"

using namespace std;
using namespace chai3d;

class Spring
{
private:
	double m_k;
	double m_length;
	double m_width;

	cVector3d m_initialPos;
public:
	explicit Spring()
		: m_k(SPRING_K)
		, m_length(SPRING_LENGTH)
		, m_width(SPRING_WIDTH)
		, m_initialPos(0, SPRING_Y, 0)
	{
	}

	double calcIndention(cVector3d& pos) const
	{
		return max(0.0, pos.y() - m_initialPos.y());	
	}

	void calcForceAndPosition(cVector3d& pos, cVector3d& vel, cVector3d& springForce, cVector3d& springPos) const
	{
		const auto indention = calcIndention(pos);
		if (indention < 0.00)
		{
			springPos = m_initialPos;
			springForce = cVector3d(0, 0, 0);
			return;
		}
		springPos = m_initialPos;
		springPos.y(m_initialPos.y() + indention);
		springForce = cVector3d(0, -m_k * pow(indention, 1.5) + vel.y() * pow(indention, 1.5) * 0.5, 0);
	}
};
