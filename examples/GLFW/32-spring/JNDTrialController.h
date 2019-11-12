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
		for (auto i = 0; i < 4; i++)
		{
			const auto packetRate = m_trialConfig.subTrialConfigs[i].packetRate;
			m_sideLabels[i]->setText("Packet rate: " + std::to_string(int64_t(packetRate)));
		}
			
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
		shuffle(controlAlgorithms.begin(), controlAlgorithms.end(), std::default_random_engine());
		m_trialConfig.delay = 0.0;
		for (auto i = 0; i < 4; i++)
		{
			m_trialConfig.subTrialConfigs[i].controlAlgorithm = controlAlgorithms[i];
			m_trialConfig.subTrialConfigs[i].packetRate = 30.0;
		}
		initCurrentTrial();
	}

	void submitJNDs()
	{
		for (auto& subTrialConfig: m_trialConfig.subTrialConfigs)
			db_save_jnd(m_db, subTrialConfig.controlAlgorithm, subTrialConfig.packetRate);
	}

	void decreasePacketRate()
	{
		const auto subTrialIdx = m_config->subTrialIdx();
		const auto oldPacketRate = m_trialConfig.subTrialConfigs[subTrialIdx].packetRate;
		const auto newPacketRate = max(30.0, oldPacketRate - 5.0);
		m_trialConfig.subTrialConfigs[subTrialIdx].packetRate = newPacketRate;
		m_sideLabels[subTrialIdx]->setText(
			"Packet rate: " + std::to_string(int64_t(newPacketRate))
		);
		initCurrentSubTrial();
	}

	void increasePacketRate()
	{
		const auto subTrialIdx = m_config->subTrialIdx();
		const auto oldPacketRate = m_trialConfig.subTrialConfigs[subTrialIdx].packetRate;
		const auto newPacketRate = min(1000.0, oldPacketRate + 5.0);
		m_trialConfig.subTrialConfigs[subTrialIdx].packetRate = newPacketRate;
		m_sideLabels[subTrialIdx]->setText(
			"Packet rate: " + std::to_string(int64_t(newPacketRate))
		);
		initCurrentSubTrial();
	}
};
