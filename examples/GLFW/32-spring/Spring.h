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

	cVector3d m_initialPos;
public:
	explicit Spring(const cVector3d pos)
		: m_k(800.0)
		, m_length(0.3)
		, m_width(0.05)
		, m_animation(nullptr)
		, m_initialPos(pos)
	{
		m_animation = new cShapeBox(0.0, m_length, m_width);
		m_animation->m_material->setWhite();
		m_animation->setLocalPos(pos);
	}

	~Spring()
	{
		delete m_animation;
	}

	cVector3d updatePositionAndCalculateForce(cVector3d& pos, cVector3d& vel) const
	{
		const auto indention = max(0.0, pos.y() - m_initialPos.y());
		if (indention < 0.00)
		{
			m_animation->setLocalPos(m_initialPos);
			return cVector3d(0, 0, 0);
		}
		auto newPos = m_initialPos;
		newPos.y(m_initialPos.y() + indention);
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

	void markReference() const
	{
		m_animation->m_material->setRed();
	}

	void unmarkReference() const
	{
		m_animation->m_material->setWhite();
	}
};
