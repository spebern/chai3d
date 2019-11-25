#pragma once

#define SAVE_MSG_STREAM_TO_DB 1

#define DT 0.001

#define SPRING_K 800.0

#define WAVE_B 0.3

#define ISS_TAU          0.005
#define ISS_MU_MAX_SCALE 1.0

#define PD_KP 300.0
#define PD_KD 0.5

#define PID_KP  300.0
#define PID_KD  0.3
#define PID_KI  1.0

#define SLAVE_MASS    0.04
#define SLAVE_DAMPING 0.04

#include "haptic_db_ffi.h"

class Config
{
private:
	ControlAlgorithm m_controlAlgorithm;
	uint32_t m_subTrialIdx = 0;
	bool m_forceFeedback = false;
	bool m_isReference = false;
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

	void isReference(const bool yes)
	{
		m_isReference = yes;
	}

	bool isReference() const
	{
		return m_isReference;
	}

	void forceFeedback(const bool yes)
	{
		m_forceFeedback = yes;
	}

	bool forceFeedback() const
	{
		return m_forceFeedback;
	}
};
