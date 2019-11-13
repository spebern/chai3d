#pragma once

#define SAVE_MSG_STREAM_TO_DB 1

#define DT 0.001

#define SPRING_K 800.0

#define WAVE_B 0.5

#define ISS_TAU          0.005
#define ISS_MU_MAX_SCALE 1.0

#define PD_KP 300.0
#define PD_KD 0.5

#define PID_KP  300.0
#define PID_KD  3.0
#define PID_KI  1.0

#include "haptic_db_ffi.h"

class Config
{
private:
	ControlAlgorithm m_controlAlgorithm;
	uint32_t m_subTrialIdx = 0;
	bool m_shouldRecord = false;
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

	void shouldRecord(const bool yes)
	{
		m_shouldRecord = yes;
	}

	bool shouldRecord() const
	{
		return m_shouldRecord;
	}
};
