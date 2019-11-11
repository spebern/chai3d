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

using namespace chai3d;

class JNDTrialController
{
private:
	Slave* m_slave;
	Master* m_master;
	Network* m_network;
	DB* m_db;
	Config* m_config;
	array<cLabel*, 4> m_packetRateLabels;
	array<cLabel*, 4> m_algorithmLabels;
	array<int32_t, 4> m_packetRates;
	array<Spring*, 4> m_springs;
	vector<ControlAlgorithm> m_controlAlgos;

	bool m_showingConfig = false;
	bool m_useReference = false;

	void initCurrentSubTrial()
	{
		const auto subTrialIdx = m_config->subTrialIdx();
		if (m_useReference)
		{
			m_config->controlAlgorithm(ControlAlgorithm::None);
			m_config->isRef(true);
			m_master->packetRate(1000.0);
			m_slave->packetRate(1000.0);
			m_springs[subTrialIdx]->markReference();
		}
		else
		{
			const auto controlAlgo = m_controlAlgos[subTrialIdx];
			m_config->isRef(false);
			m_config->controlAlgorithm(controlAlgo);
			m_master->packetRate(m_packetRates[subTrialIdx]);
			m_slave->packetRate(m_packetRates[subTrialIdx]);
			m_springs[subTrialIdx]->unmarkReference();
		}
		m_slave->spring(m_springs[subTrialIdx]);
	}

	void initCurrentTrial()
	{
		for (auto i = 0; i < m_packetRates.size(); i++)
			m_packetRateLabels[i]->setText("Packet rate: " + std::to_string(m_packetRates[i]));
	}

	void clearConfig()
	{
		for (auto& label : m_algorithmLabels)
			label->setText("");
	}

	void showConfig() const
	{
		for (auto i = 0; i < m_algorithmLabels.size(); i++)
		{
			switch (m_controlAlgos[i])
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
	                array<cLabel*, 4> packetRateLabels, array<Spring*, 4> springs, array<cLabel*, 4> algorithmLabels )
		: m_slave(slave)
		, m_master(master)
		, m_network(network)
		, m_db(db)
		, m_config(config)
		, m_packetRateLabels(packetRateLabels)
		, m_algorithmLabels(algorithmLabels)
		, m_springs(springs)
	{
		m_controlAlgos.push_back(ControlAlgorithm::None);
		m_controlAlgos.push_back(ControlAlgorithm::WAVE);
		m_controlAlgos.push_back(ControlAlgorithm::ISS);
		m_controlAlgos.push_back(ControlAlgorithm::PC);
		for (auto i = 0; i < 4; i ++)
		{
			m_packetRates[i] = 30;
		}
		shuffle(m_controlAlgos.begin(), m_controlAlgos.end(), std::default_random_engine());

		initCurrentTrial();
		initCurrentSubTrial();
	}

	void nextSubTrial()
	{
		const auto subTrialIdx = m_config->subTrialIdx();
		if (subTrialIdx == m_springs.size() - 1)
			m_config->subTrialIdx(0);
		else

			m_config->subTrialIdx(subTrialIdx + 1);
		initCurrentSubTrial();
	}

	void previousSubTrial()
	{
		const auto subTrialIdx = m_config->subTrialIdx();
		if (subTrialIdx == 0)
			m_config->subTrialIdx(m_springs.size() - 1);
		else
			m_config->subTrialIdx(subTrialIdx - 1);
		initCurrentSubTrial();
	}

	void submitJNDs()
	{
		for (auto i = 0; i < 4; i++)
		{
			db_save_jnd(m_db, m_controlAlgos[i], m_packetRates[i]);
		}
	}

	void toggleConfig()
	{
		if (m_showingConfig)
		{
			clearConfig();
			m_showingConfig = false;
		}
		else
		{
			showConfig();
			m_showingConfig = true;
		}
	}

	void toggleReference()
	{
		if (m_useReference)
		{
			m_useReference = false;
			initCurrentSubTrial();
		}
		else
		{
			m_useReference = true;
			initCurrentSubTrial();
		}
	}

	void decreasePacketRate()
	{
		const auto subTrialIdx = m_config->subTrialIdx();
		m_packetRates[subTrialIdx] = max(30.0, m_packetRates[subTrialIdx] - 5.0);
		m_packetRateLabels[subTrialIdx]->setText(
			"Packet rate: " + std::to_string(m_packetRates[subTrialIdx])
		);
		initCurrentSubTrial();
	}

	void increasePacketRate()
	{
		const auto subTrialIdx = m_config->subTrialIdx();
		m_packetRates[subTrialIdx] = min(1000.0, m_packetRates[subTrialIdx] + 5.0);
		m_packetRateLabels[subTrialIdx]->setText(
			"Packet rate: " + std::to_string(m_packetRates[subTrialIdx])
		);
		initCurrentSubTrial();
	}
};
