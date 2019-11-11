#pragma once

#include "chai3d.h"

using namespace chai3d;

class ToolTip
{
private:
	double m_radius;
	cShapeEllipsoid* m_animation;
public:
	explicit ToolTip(const double radius = 0.01): m_radius(radius), m_animation(nullptr)
	{
		m_animation = new cShapeEllipsoid(radius, radius, radius);
	}

	~ToolTip()
	{
		delete m_animation;
	}

	double radius() const
	{
		return m_radius;
	}

	void pos(cVector3d& pos) const
	{
		m_animation->setLocalPos(pos);
	}

	cShapeEllipsoid* animation() const
	{
		return m_animation;
	}
};
