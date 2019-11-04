#pragma once

#include "chai3d.h"

using namespace std;
using namespace chai3d;

class Spring
{
private:
	double m_k;
	double m_length;
	double m_width;
	cShapeBox* m_animation;
public:
	explicit Spring(const cVector3d pos): m_k(200.0), m_length(0.2), m_width(0.05), m_animation(nullptr)
	{
		m_animation = new cShapeBox(0, m_length, m_width);
		m_animation->setLocalPos(pos);
	}

	~Spring()
	{
		delete m_animation;
	}

	cVector3d updatePositionAndCalculateForce(cVector3d& pos, cVector3d& vel) const
	{
		const auto indention = max(0.0, pos.y());
		auto newPos = m_animation->getLocalPos();
		if (indention < 0.0)
		{
			newPos.y(0);
			m_animation->setLocalPos(newPos);
			return cVector3d(0, 0, 0);
		}
		newPos.y(indention);
		std::cout << newPos << std::endl;
		m_animation->setLocalPos(newPos);
		const auto force = m_k * pow(indention, 1.5) + vel.y() * pow(indention, 1.5) * 0.5;
		return cVector3d(0, -force, 0);
	}

	cShapeBox* animation() const
	{
		return m_animation;
	}

	bool indented() const
	{
		return !m_animation->getLocalPos().y() == 0.0;
	}

	cVector3d pos() const
	{
		return m_animation->getLocalPos();
	}

	double length() const
	{
		return m_length;
	}

	double width() const
	{
		return m_width;
	}
};
