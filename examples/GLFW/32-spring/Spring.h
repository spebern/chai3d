#pragma once

#include "chai3d.h"

using namespace std;
using namespace chai3d;

class Spring
{
private:
	double m_k;
	double m_height;
	double m_base;
	cMesh* m_animation;
public:
	explicit Spring(cMesh* mesh): m_k(200.0), m_height(0.085), m_base(0), m_animation(mesh)
	{
	}

	cVector3d updatePositionAndCalculateForce(cVector3d& pos, cVector3d& vel) const
	{
		const auto indention = max(0.0, pos.y() + m_height);
		if (indention < 0.0)
		{
			m_animation->setLocalPos(0, 0, 0);
			return cVector3d(0, 0, 0);
		}
		m_animation->setLocalPos(0, m_base + indention, 0);
		const auto force = m_k * pow(indention, 1.5) + vel.y() * pow(indention, 1.5) * 0.5;
		return cVector3d(0, -force, 0);
	}
};
