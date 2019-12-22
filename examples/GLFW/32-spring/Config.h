#pragma once

#define SAVE_MSG_STREAM_TO_DB 1

#define DT 0.001

#define SPRING_K 800.0
#define SPRING_Y 0.00
#define SPRING_LENGTH 0.3
#define SPRING_WIDTH 0.05

#define WAVE_B 0.3

#define ISS_TAU          0.005
#define ISS_MU_MAX_SCALE 2.0

// PID or PD
#define USE_PD 0

#define PD_KP 300.0
#define PD_KD 1.0

#define PID_KP  300.0
#define PID_KD  0.15
#define PID_KI  1.0

#define SLAVE_MASS    0.04
#define SLAVE_DAMPING 0.04

#define MMT_LAMBDA 0.1
#define MMT_INITIAL_K 200

#define DEADBAND_ACTIVE 0
#define DEADBAND_THRESHOLD 0.1

#define MAX_FORCE 20.0

#include "haptic_db_ffi.h"
#include "mutex"

class Config
{
private:
	ControlAlgorithm m_controlAlgorithm;
	uint32_t m_subTrialIdx = 0;
	bool m_forceFeedback = false;
	bool m_isReference = false;

	std::mutex m_mu;
public:
	Config(): m_controlAlgorithm(ControlAlgorithm::None) {}

	void lock()
	{
		m_mu.lock();
	}

	void unlock()
	{
		m_mu.unlock();
	}

	void controlAlgorithm(const ControlAlgorithm controlAlgorithm)
	{
		m_controlAlgorithm = controlAlgorithm;
	}

	ControlAlgorithm controlAlgorithm() const
	{
		return m_controlAlgorithm;
	}

	void springIdx(const uint32_t subTrialIdx)
	{
		m_subTrialIdx = subTrialIdx;
	}

	uint32_t springIdx() const
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
