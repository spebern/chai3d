#pragma once

#include "chai3d.h"
#include "Config.h"

using namespace chai3d;

class DeadbandDetector
{
private:
	double m_threshold;
	cVector3d m_previousValue;
public:
	explicit DeadbandDetector(const double threshold = DEADBAND_THRESHOLD)
		: m_threshold(threshold)
		, m_previousValue(0, 0, 0)
	{}
public:
	bool inDeadband(const cVector3d &value)
	{
		const auto diff = value - m_previousValue;
		if (abs(diff.length()) / abs(m_previousValue.length()) > m_threshold)
		{
			m_previousValue = value;
			return false;
		}
		return true;
	}
};
