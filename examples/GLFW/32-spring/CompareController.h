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

struct TrialConfig
{
	OptimisationParameter optimisationParameter;
	ControlAlgorithm controlAlgorithm;
	int32_t delay;
	int32_t packetRate;
};

class CompareController: public Controller
{
private:
	vector<TrialConfig> m_trialConfigs;

	TrialConfig m_currentTrialConfig;

	array<int32_t, 5> m_inputs;

	void initCurrentSpring() const
	{
		const auto springIdx = m_config->springIdx();

		if (m_useReference)
		{
			if (m_currentTrialConfig.optimisationParameter == OptimisationParameter::Delay)
			{
				m_env->m_rightInputs[springIdx]->setText(std::to_string(0) + " ms");
				m_network->delay(microseconds(0));
			}
			else
			{
				m_env->m_rightInputs[springIdx]->setText(std::to_string(400) + " Hz");
				m_master->packetRate(400);
				m_slave->packetRate(400);
			}
			auto red = cColorb(255, 0, 0);
			m_env->springColor(springIdx, red);
		}
		else
		{
			if (m_currentTrialConfig.optimisationParameter == OptimisationParameter::Delay)
			{
				m_env->m_rightInputs[springIdx]->setText(std::to_string(m_inputs[springIdx]) + " ms");
				m_network->delay(microseconds(m_inputs[springIdx] * 1000));
			}
			else {
				m_env->m_rightInputs[springIdx]->setText(std::to_string(m_inputs[springIdx]) + " Hz");
				m_master->packetRate(double(m_inputs[springIdx]));
				m_slave->packetRate(double(m_inputs[springIdx]));
			}
			auto white = cColorb(255, 255, 255);
			m_env->springColor(springIdx, white);
		}
	}

	void clearConfig() override
	{
		m_env->m_bottomMiddleLabel->setShowEnabled(false);
		m_env->m_bottomRightLabel->setShowEnabled(false);
		for (auto i = 0; i < m_inputs.size(); i++)
			m_env->m_rightInputs[i]->setShowEnabled(false);
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

		m_env->m_bottomMiddleLabel->setShowEnabled(true);
		m_env->m_bottomRightLabel->setShowEnabled(true);
		for (auto i = 0; i < m_inputs.size(); i++)
			m_env->m_rightInputs[i]->setShowEnabled(true);

		std::stringstream ss;
		if (m_currentTrialConfig.optimisationParameter == OptimisationParameter::Delay)
		{
			ss << std::fixed << std::setprecision(2) << m_currentTrialConfig.packetRate;
			const auto packetRate = ss.str();
			m_env->bottomMiddleLabel(packetRate + " Hz");
		}
		else
		{
			ss << std::fixed << std::setprecision(2) << m_currentTrialConfig.delay;
			const auto delay = ss.str();
			m_env->bottomMiddleLabel(delay + " ms");
		}
	}

public:
	CompareController(Slave* slave, Master* master, Config* config, Network* network, DB* db, Environment* env, const vector<TrialConfig> trialConfigs)
		: Controller(slave, master, config, network, db, env)
		, m_trialConfigs(trialConfigs)
	{
		nextTrial();
	}

	bool nextTrial()
	{
		if (m_trialConfigs.empty()) 
			return false;

		clearConfig();

		m_currentTrialConfig = m_trialConfigs.back();
		m_trialConfigs.pop_back();

		m_useReference = false;
		if (m_currentTrialConfig.optimisationParameter == OptimisationParameter::Delay) 
		{
			for (auto& x : m_inputs) x = 80;
			m_network->delay(microseconds(80 * 1000));
			m_master->packetRate(m_currentTrialConfig.packetRate);
			m_slave->packetRate(m_currentTrialConfig.packetRate);
			for (auto& input : m_env->m_rightInputs)
				input->setText("80 ms");
		}
		else
		{
			for (auto& x : m_inputs) x = 10;
			m_network->delay(microseconds(m_currentTrialConfig.delay * 1000));
			m_master->packetRate(10);
			m_slave->packetRate(10);
			for (auto& input : m_env->m_rightInputs)
				input->setText("10 Hz");
		}

		initCurrentSpring();

		return true;
	}

	void init() override
	{
		m_config->lock();
		m_config->springIdx(0);
		m_config->unlock();
		for (auto i = 0; i < 5; i++) m_env->showSpring(i, true);
		initCurrentSpring();
	}

	bool saveToDb() override
	{
		for (auto i = 0; i < m_inputs.size(); i++)
		{
			int32_t delay;
			int32_t packetRate;
			if (m_currentTrialConfig.optimisationParameter == OptimisationParameter::Delay)
			{
				delay = m_inputs[i];
				packetRate = m_currentTrialConfig.packetRate;
			}
			else
			{
				delay = m_currentTrialConfig.delay;
				packetRate = m_inputs[i];
			}
			db_insert_rating(m_db, m_currentTrialConfig.controlAlgorithm, packetRate, delay, m_currentTrialConfig.optimisationParameter, 5 - i);
		}
		return !nextTrial();
	}

	void upKey() override
	{
		m_config->lock();
		const auto springIdx = m_config->springIdx() == 0 ? 4 : m_config->springIdx() - 1;
		m_config->springIdx(springIdx);
		m_config->unlock();
		initCurrentSpring();
	}

	void downKey() override
	{
		m_config->lock();
		const auto springIdx = m_config->springIdx() == 4 ? 0 : m_config->springIdx() + 1;
		m_config->springIdx(springIdx);
		m_config->unlock();
		initCurrentSpring();
	}
	

	void leftKey() override
	{
		m_config->lock();
		const auto springIdx = m_config->springIdx();
		m_config->unlock();
		int32_t step;
		int32_t minInput;
		if (m_currentTrialConfig.optimisationParameter == OptimisationParameter::Delay)
		{
			step = 5;
			minInput = 0;
		}	
		else
		{
			step = 5;
			minInput = 10;
		}
		if (m_inputs[springIdx] != minInput)
			m_env->flash("-");
		m_inputs[springIdx] = max(minInput, m_inputs[springIdx] - step);
		initCurrentSpring();

	}

	void rightKey() override
	{
		m_config->lock();
		const auto springIdx = m_config->springIdx();
		m_config->unlock();
		int32_t step;
		int32_t maxInput;
		if (m_inputs[springIdx] != maxInput)
			m_env->flash("+");
		if (m_currentTrialConfig.optimisationParameter == OptimisationParameter::Delay)
		{
			step = 5;
			maxInput = 100;
		}	
		else
		{
			step = 5;
			maxInput = 300;
		}
		m_inputs[springIdx] = min(maxInput, m_inputs[springIdx] + step);
		initCurrentSpring();
	}

	void toggleReference() override
	{
		m_useReference = !m_useReference;
		initCurrentSpring();
	}
};
