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
#include "RateDelay.h"
#include <iomanip>

using namespace chai3d;

struct NetworkResourceTrialConfig
{
	int32_t rateToDelay;
	int32_t constDelay;
	ControlAlgorithm controlAlgorithm;
};

class NetworkResourcesController: public Controller
{
private:
	RateDelay m_rateDelay;
	array<int32_t, 5> m_packetRates;
	array<int32_t, 5> m_delayRatings;
	array<int32_t, 5> m_smoothnessRatings;
	NetworkResourceTrial m_currentTrial;
	vector<NetworkResourceTrialConfig> m_trialConfigs;
	bool m_rateSmoothness = true;

	double m_currentDelayMS = 0.0;

	void initCurrentTrial()
	{
		m_trialConfig.shouldRecord = true;
		for (auto i = 0; i < 5; i++)
		{
			m_env->rightInput(i, "Rate: " + std::to_string(int64_t(10)));
			m_env->middleInput(i, "Delay: " + std::to_string(int64_t(0)));
			m_env->leftInput(i, "Smooth: " + std::to_string(int64_t(0)));
			m_packetRates[i] = 10;
			m_delayRatings[i] = 0;
			m_smoothnessRatings[i] = 0;
		}
		m_rateSmoothness = true;
	}

	void initCurrentSubTrial() override
	{
		m_config->lock();
		const auto subTrialIdx = m_config->springIdx();

		if (m_useReference)
		{
			m_config->isReference(true);
			m_config->controlAlgorithm(ControlAlgorithm::None);

			m_master->packetRate(400.0);
			m_slave->packetRate(400.0);

			m_network->delay(microseconds(0));
			m_currentDelayMS = 0.0;

			auto red = cColorb(255, 0, 0);
			m_env->springColor(subTrialIdx, red);
		}
		else
		{
			const auto controlAlgorithm = m_currentTrial.controlAlgo;
			m_config->isReference(false);
			m_config->controlAlgorithm(controlAlgorithm);

			const auto packetRate = m_packetRates[subTrialIdx];

			m_master->packetRate(packetRate);
			m_slave->packetRate(packetRate);

			const auto delay = m_rateDelay.delay(packetRate);
			m_network->delay(delay);
			m_currentDelayMS = double(chrono::duration_cast<milliseconds>(delay).count());

			auto white = cColorb(255, 255, 255);
			m_env->springColor(subTrialIdx, white);
		}

		m_config->unlock();

		if (m_showingConfig)
			showConfig();
	}

	void clearConfig() override
	{
		m_env->bottomRightLabel("");
		m_env->bottomMiddleLabel("");
	}
	
	void showConfig() override
	{
		switch (m_currentTrial.controlAlgo)
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
		ss << std::fixed << std::setprecision(2) << m_currentDelayMS;
		const auto delay = ss.str();
		m_env->bottomMiddleLabel(delay + " ms");
	}

public:
	NetworkResourcesController(Slave* slave, Master* master, Config* config, Network* network, DB* db, Environment* env, const vector<NetworkResourceTrialConfig> trialConfigs)
		: Controller(slave, master, config, network, db, env)
		, m_trialConfigs(trialConfigs)
	{
		nextTrial();
	}

	bool nextTrial()
	{
		if (m_trialConfigs.empty()) 
			return false;

		const auto trialConfig = m_trialConfigs.back();
		m_trialConfigs.pop_back();

		m_currentTrial = db_new_network_resource_trial(
			m_db, 
			trialConfig.rateToDelay, 
			trialConfig.constDelay, 
			trialConfig.controlAlgorithm
		);

		m_rateDelay = RateDelay(microseconds(m_currentTrial.rateToDelay), microseconds(m_currentTrial.constDelay));

		initCurrentTrial();
		initCurrentSubTrial();

		return true;
	}

	void init() override
	{
		m_config->lock();
		m_config->springIdx(1);
		m_config->unlock();
		initCurrentTrial();
	}

	bool saveToDb() override
	{
		db_new_network_resource_sub_trial(m_db, &m_currentTrial, m_packetRates[1], m_delayRatings[1], m_smoothnessRatings[1]);
		db_new_network_resource_sub_trial(m_db, &m_currentTrial, m_packetRates[3], m_delayRatings[3], m_smoothnessRatings[3]);
		return !nextTrial();
	}

	void leftKey() override
	{
		m_config->lock();
		const auto subTrialIdx = m_config->springIdx();
		m_config->unlock();
		const auto oldPacketRate = m_packetRates[subTrialIdx];
		const auto newPacketRate = max(10.0, oldPacketRate - 10.0);
		m_packetRates[subTrialIdx] = newPacketRate;
		m_env->rightInput(subTrialIdx, "Rate: " + std::to_string(int64_t(newPacketRate)));
		initCurrentSubTrial();
	}

	void rightKey() override
	{
		m_config->lock();
		const auto subTrialIdx = m_config->springIdx();
		m_config->unlock();
		const auto oldPacketRate = m_packetRates[subTrialIdx];
		const auto newPacketRate = min(200.0, oldPacketRate + 10.0);
		m_packetRates[subTrialIdx] = newPacketRate;
		m_env->rightInput(subTrialIdx, "Rate: " + std::to_string(int64_t(newPacketRate)));
		initCurrentSubTrial();
	}

	void toggleSpring() const
	{
		m_config->lock();
		m_config->springIdx() == 1
			? m_config->springIdx(3)
			: m_config->springIdx(1);
		m_config->unlock();
	}

	void downKey() override
	{
		toggleSpring();
		initCurrentSubTrial();
	}

	void upKey() override
	{
		toggleSpring();
		initCurrentSubTrial();
	}

	void toggleReference() override
	{
		m_useReference = !m_useReference;
		initCurrentSubTrial();
	}

	void rate(const int32_t rating) override
	{
		m_config->lock();
		const auto subTrialIdx = m_config->springIdx();
		m_config->unlock();
		if (m_rateSmoothness)
		{
			m_env->leftInput(subTrialIdx, "Smooth: " + std::to_string(rating));
			m_smoothnessRatings[subTrialIdx] = rating;
		}
		else
		{
			m_env->middleInput(subTrialIdx, "Delay: " + std::to_string(rating));
			m_delayRatings[subTrialIdx] = rating;
		}
		m_rateSmoothness = !m_rateSmoothness;
	}
};
