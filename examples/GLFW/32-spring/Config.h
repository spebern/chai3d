#pragma once

#define DT 0.001

#include "haptic_db_ffi.h"

class Config
{
private:
	ControlAlgorithm m_controlAlgorithm;
	uint32_t m_subTrialIdx = 0;
public:
	Config(): m_controlAlgorithm(ControlAlgorithm::None) {}

	void controlAlgorithm(const ControlAlgorithm controlAlgorithm)
	{
		m_controlAlgorithm = controlAlgorithm;
	}

	ControlAlgorithm controlAlgorithm() const
	{
		return m_controlAlgorithm;
	}

	void subTrialIdx(const uint32_t subTrialIdx)
	{
		m_subTrialIdx = subTrialIdx;
	}

	uint32_t subTrialIdx() const
	{
		return m_subTrialIdx;
	}
};
