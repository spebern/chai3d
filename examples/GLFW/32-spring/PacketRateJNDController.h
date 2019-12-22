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

struct PacketRateTrialConfig
{
	int32_t delay;
	ControlAlgorithm controlAlgorithm;
};

class PacketRateJDNController: public Controller
{
private:
	int32_t m_packetRate = 10;
	vector<PacketRateTrialConfig> m_trialConfigs;

	PacketRateTrialConfig m_currentTrialConfig;

	void initCurrentTrial() const
	{
		m_config->lock();
		m_config->controlAlgorithm(m_currentTrialConfig.controlAlgorithm);
		m_network->delay(microseconds(m_currentTrialConfig.delay * 1000));
		if (m_useReference)
		{
			m_slave->packetRate(400);
			m_master->packetRate(400);
			auto red = cColorb(255, 0, 0);
			m_env->springColor(2, red);
			m_env->rightInput(2, "Rate: " + std::to_string(400));
		}
		else
		{
			m_slave->packetRate(m_packetRate);
			m_master->packetRate(m_packetRate);
			auto white = cColorb(255, 255, 255);
			m_env->springColor(2, white);
			m_env->rightInput(2, "Rate: " + std::to_string(m_packetRate));
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
		ss << std::fixed << std::setprecision(2) << m_currentTrialConfig.delay;
		const auto delay = ss.str();
		m_env->bottomMiddleLabel(delay + " ms");
	}

public:
	PacketRateJDNController(Slave* slave, Master* master, Config* config, Network* network, DB* db, Environment* env, const vector<PacketRateTrialConfig> trialConfigs)
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

		m_packetRate = 10;

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
		db_insert_packet_rate_jnd_trial(m_db, m_currentTrialConfig.controlAlgorithm, m_packetRate, m_currentTrialConfig.delay);
		return !nextTrial();
	}

	void leftKey() override
	{
		m_packetRate = max(10.0, m_packetRate - 10.0);
		initCurrentTrial();
	}

	void rightKey() override
	{
		m_packetRate = min(200.0, m_packetRate + 10.0);
		initCurrentTrial();
	}

	void toggleReference() override
	{
		m_useReference = !m_useReference;
		initCurrentTrial();
	}
};
