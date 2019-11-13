#pragma once

#include "array"
#include "Slave.h"
#include "Master.h"
#include "chai3d.h"
#include "haptic_db_ffi.h"

using namespace chai3d;

struct SubTrialConfig
{
	ControlAlgorithm controlAlgorithm;
	double packetRate;
};

struct TrialConfig
{
	array<SubTrialConfig, 4> subTrialConfigs;
	double delay;
	bool shouldRecord;
};

class Controller
{
protected:
	Slave* m_slave;
	Master* m_master;
	Network* m_network;
	DB* m_db;
	Config* m_config;
	array<cLabel*, 4> m_sideLabels;
	array<cLabel*, 4> m_algorithmLabels;
	array<Spring*, 4> m_springs;

	cLabel* m_packetRateLabel;
	cLabel* m_delayLabel;

	bool m_showingConfig = false;
	bool m_useReference = false;

	TrialConfig m_trialConfig;

	void clearConfig()
	{
		m_packetRateLabel->setText("");
		m_delayLabel->setText("");
		for (auto& label : m_algorithmLabels)
			label->setText("");
	}

	virtual void showConfig() {}

	void initCurrentSubTrial()
	{
		const auto subTrialIdx = m_config->subTrialIdx();
		if (m_useReference)
		{
			m_config->controlAlgorithm(ControlAlgorithm::None);
			m_config->shouldRecord(false);
			m_master->packetRate(1000.0);
			m_slave->packetRate(1000.0);
			m_springs[subTrialIdx]->markReference();
		}
		else
		{
			const auto controlAlgo = m_trialConfig.subTrialConfigs[subTrialIdx].controlAlgorithm;
			m_config->shouldRecord(m_trialConfig.shouldRecord);
			m_config->controlAlgorithm(controlAlgo);
			m_master->packetRate(m_trialConfig.subTrialConfigs[subTrialIdx].packetRate);
			m_slave->packetRate(m_trialConfig.subTrialConfigs[subTrialIdx].packetRate);
			m_springs[subTrialIdx]->unmarkReference();
		}
		m_slave->spring(m_springs[subTrialIdx]);
	}

public:
	Controller(Slave* slave, Master* master, Config* config, Network* network, DB* db,
	                array<cLabel*, 4> sideLabels, array<Spring*, 4> springs, array<cLabel*, 4> algorithmLabels,
	                cLabel* packetRateLabel, cLabel* delayLabel)
		: m_slave(slave)
		, m_master(master)
		, m_network(network)
		, m_db(db)
		, m_config(config)
		, m_sideLabels(sideLabels)
		, m_algorithmLabels(algorithmLabels)
		, m_springs(springs)
		, m_packetRateLabel(packetRateLabel)
		, m_delayLabel(delayLabel)
	{
		m_trialConfig.delay = 0.0;
		for (auto& subTrialConfig: m_trialConfig.subTrialConfigs)
		{
			subTrialConfig.controlAlgorithm = ControlAlgorithm::None;
			subTrialConfig.packetRate = 1000.0;
		}
		clearConfig();
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

	virtual bool saveToDb() { return true; }

	virtual void rightKey() {}

	virtual void leftKey() {}

	virtual void init() {}

	virtual void rate(int32_t rating) {}
};
