#pragma once

#include "cstdint"
#include "array"
#include "string"
#include "Slave.h"
#include "Master.h"
#include "Network.h"
#include "haptic_db_ffi.h"
#include "chai3d.h"
#include "Config.h"
#include "random"
#include "Controller.h"

using namespace chai3d;

class JNDTrialController: public Controller
{
private:
	void initCurrentTrial()
	{
		m_trialConfig.shouldRecord = true;
		for (auto i = 0; i < 4; i++)
		{
			const auto packetRate = m_trialConfig.subTrialConfigs[i].packetRate;
			m_sideLabels[i]->setText("Packet rate: " + std::to_string(int64_t(packetRate)));
		}
	}

	void initCurrentSubTrial() override
	{
		
		const auto subTrialIdx = m_config->subTrialIdx();
		const auto controlAlgo = m_trialConfig.subTrialConfigs[subTrialIdx].controlAlgorithm;
		m_config->isReference(false);
		m_config->controlAlgorithm(controlAlgo);
		m_master->packetRate(m_trialConfig.subTrialConfigs[subTrialIdx].packetRate);
		m_slave->packetRate(m_trialConfig.subTrialConfigs[subTrialIdx].packetRate);
		m_springs[subTrialIdx]->unmarkReference();
		m_slave->spring(m_springs[subTrialIdx]);
	}

	void clearConfig()
	{
		for (auto& label : m_algorithmLabels)
			label->setText("");
	}
	
	void showConfig() override
	{
		m_delayLabel->setText(std::to_string(int64_t(m_trialConfig.delay)) + " ms");
		for (auto i = 0; i < m_algorithmLabels.size(); i++)
		{
			switch (m_trialConfig.subTrialConfigs[i].controlAlgorithm)
			{
			case ControlAlgorithm::None:
				m_algorithmLabels[i]->setText("REF");
				break;
			case ControlAlgorithm::WAVE:
				m_algorithmLabels[i]->setText("WAVE");
				break;
			case ControlAlgorithm::ISS:
				m_algorithmLabels[i]->setText("ISS");
				break;
			case ControlAlgorithm::PC:
				m_algorithmLabels[i]->setText("TDPA");
				break;
			case ControlAlgorithm::MMT:
				m_algorithmLabels[i]->setText("MMT");
				break;
			}
		}
	}

public:
	JNDTrialController(Slave* slave, Master* master, Config* config, Network* network, DB* db,
		const array<cLabel*, 4>& sideLabels, const array<Spring*, 4>& springs, const array<cLabel*, 4>& algorithmLabels,
		cLabel* packetRateLabel, cLabel* delayLabel)
		: Controller(
			slave, master, config, network, db, sideLabels, springs, algorithmLabels, packetRateLabel, delayLabel)
	{
		array<ControlAlgorithm, 4> controlAlgorithms;
		controlAlgorithms[0] = ControlAlgorithm::None;
		controlAlgorithms[1] = ControlAlgorithm::ISS;
		controlAlgorithms[2] = ControlAlgorithm::PC;
		controlAlgorithms[3] = ControlAlgorithm::WAVE;

		std::srand(std::time(0));
		random_shuffle(controlAlgorithms.begin(), controlAlgorithms.end());

		m_trialConfig.delay = 0.0;
		for (auto i = 0; i < 4; i++)
		{
			m_trialConfig.subTrialConfigs[i].controlAlgorithm = controlAlgorithms[i];
			m_trialConfig.subTrialConfigs[i].packetRate = 15.0;
		}
	}

	void init() override
	{
		initCurrentTrial();
		m_master->packetRate(m_trialConfig.subTrialConfigs[0].packetRate);
		m_slave->packetRate(m_trialConfig.subTrialConfigs[0].packetRate);
	}

	bool saveToDb() override
	{
		for (auto& subTrialConfig: m_trialConfig.subTrialConfigs)
			db_save_jnd(m_db, subTrialConfig.controlAlgorithm, subTrialConfig.packetRate);
		return true;
	}

	void leftKey() override
	{
		const auto subTrialIdx = m_config->subTrialIdx();
		const auto oldPacketRate = m_trialConfig.subTrialConfigs[subTrialIdx].packetRate;
		const auto newPacketRate = max(15.0, oldPacketRate - 10.0);
		m_trialConfig.subTrialConfigs[subTrialIdx].packetRate = newPacketRate;
		m_sideLabels[subTrialIdx]->setText(
			"Packet rate: " + std::to_string(int64_t(newPacketRate))
		);
		initCurrentSubTrial();
	}

	void rightKey() override
	{
		const auto subTrialIdx = m_config->subTrialIdx();
		const auto oldPacketRate = m_trialConfig.subTrialConfigs[subTrialIdx].packetRate;
		const auto newPacketRate = min(200.0, oldPacketRate + 10.0);
		m_trialConfig.subTrialConfigs[subTrialIdx].packetRate = newPacketRate;
		m_sideLabels[subTrialIdx]->setText(
			"Packet rate: " + std::to_string(int64_t(newPacketRate))
		);
		initCurrentSubTrial();
	}

	void downKey() override
	{
		const auto subTrialIdx = m_config->subTrialIdx();
		if (subTrialIdx == m_springs.size() - 1)
			m_config->subTrialIdx(0);
		else

			m_config->subTrialIdx(subTrialIdx + 1);
		initCurrentSubTrial();
	}

	void upKey() override
	{
		const auto subTrialIdx = m_config->subTrialIdx();
		if (subTrialIdx == 0)
			m_config->subTrialIdx(m_springs.size() - 1);
		else
			m_config->subTrialIdx(subTrialIdx - 1);
		initCurrentSubTrial();
	}

	void toggleReference() override {}
};
