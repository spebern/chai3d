#pragma once

#include "cstdint"
#include "string"
#include "Slave.h"
#include "Master.h"
#include "Network.h"
#include "haptic_db_ffi.h"
#include "chai3d.h"
#include "Config.h"
#include "Controller.h"
#include <iomanip>
#include <chrono>

using namespace chai3d;
using namespace std::chrono;

struct DelayTrialConfig
{
	int32_t packetRate;
	ControlAlgorithm controlAlgorithm;
};

class DelayJDNController: public Controller
{
private:
	int32_t m_delay = 0;
	vector<DelayTrialConfig> m_trialConfigs;

	DelayTrialConfig m_currentTrialConfig;

	void initCurrentTrial() const
	{
		m_config->lock();
		m_config->controlAlgorithm(m_currentTrialConfig.controlAlgorithm);
		m_slave->packetRate(m_currentTrialConfig.packetRate);
		m_master->packetRate(m_currentTrialConfig.packetRate);
		if (m_useReference)
		{
			m_network->delay(microseconds(0));
			auto red = cColorb(255, 0, 0);
			m_env->springColor(2, red);
			m_env->rightInput(2, "Delay: " + std::to_string(m_delay) + "ms");
		}
		else
		{
			m_network->delay(microseconds(m_delay * 1000));
			auto white = cColorb(255, 255, 255);
			m_env->springColor(2, white);
			m_env->rightInput(2, "Delay: " + std::to_string(m_delay) + "ms");
		}
		m_config->unlock();
	}

	void clearConfig() override
	{
		m_env->bottomRightLabel("");
		m_env->bottomMiddleLabel("");
	}
	
	void showConfig() override
	{
		switch (m_currentTrialConfig.controlAlgorithm)
		{
		case ControlAlgorithm::ISS:
			m_env->bottomRightLabel("ISS");
			break;
		case ControlAlgorithm::None:
			m_env->bottomRightLabel("None");
			break;
		case ControlAlgorithm::WAVE:
			m_env->bottomRightLabel("WAVE");
			break;
		case ControlAlgorithm::PC:
			m_env->bottomRightLabel("PC");
			break;
		case ControlAlgorithm::MMT:
			m_env->bottomRightLabel("MMT");
			break;
		default: ;
			m_env->bottomRightLabel("UNKNOWN");
			break;
		}
		std::stringstream ss;
		ss << std::fixed << std::setprecision(2) << m_currentTrialConfig.packetRate;
		const auto packetRate = ss.str();
		m_env->bottomMiddleLabel(packetRate + " Hz");
	}

public:
	DelayJDNController(Slave* slave, Master* master, Config* config, Network* network, DB* db, Environment* env, const vector<DelayTrialConfig> trialConfigs)
		: Controller(slave, master, config, network, db, env)
		, m_trialConfigs(trialConfigs)
	{
		nextTrial();
	}

	bool nextTrial()
	{
		if (m_trialConfigs.empty()) 
			return false;

		m_currentTrialConfig = m_trialConfigs.back();
		m_trialConfigs.pop_back();

		m_delay = 50;

		initCurrentTrial();

		return true;
	}

	void init() override
	{
		m_config->lock();
		m_config->springIdx(2);
		m_config->unlock();
		for (auto i = 0; i < 5; i++)
			m_env->showSpring(i, i == 2);
		initCurrentTrial();
	}

	bool saveToDb() override
	{
		db_insert_delay_jnd_trial(m_db, m_currentTrialConfig.controlAlgorithm, m_currentTrialConfig.packetRate, m_delay);
		return !nextTrial();
	}

	void leftKey() override
	{
		m_delay = max(2.0, m_delay - 2.0);
		initCurrentTrial();
	}

	void rightKey() override
	{
		m_delay = min(50.0, m_delay + 2.0);
		initCurrentTrial();
	}

	void toggleReference() override
	{
		m_useReference = !m_useReference;
		initCurrentTrial();
	}
};
