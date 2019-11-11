#pragma once

#define SAVE_MSG_STREAM_TO_DB 1

#define DT 0.001
#define WAVE_B 0.5

#include "haptic_db_ffi.h"

class Config
{
private:
	ControlAlgorithm m_controlAlgorithm;
	uint32_t m_subTrialIdx = 0;
	bool m_isRef = false;
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

	void isRef(bool yes)
	{
		m_isRef = yes;
	}

	bool isRef()
	{
		return m_isRef;
	}
};
