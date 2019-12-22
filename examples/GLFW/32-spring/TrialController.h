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
#include "Controller.h"

using namespace chai3d;

/*
class TrialController: public Controller
{
private:
	Trial m_trial;

	void initCurrentTrial()
	{
		m_trial = db_current_trial(m_db);
		m_sideLabels[0]->setText("Rating: 0");
		initCurrentSubTrial();
	}

	void initCurrentSubTrial() override
	{
		m_config->lock();
		if (m_useReference)
		{
			m_config->controlAlgorithm(ControlAlgorithm::None);
			m_config->isReference(true);
			m_master->packetRate(1000.0);
			m_slave->packetRate(1000.0);
			m_network->delay(chrono::microseconds(0));
			m_springs[0]->markReference();
		}
		else
		{
			const auto controlAlgo = m_trial.controlAlgo;
			m_config->isReference(false);
			m_config->controlAlgorithm(controlAlgo);
			m_master->packetRate(m_trial.packetRate);
			m_slave->packetRate(m_trial.packetRate);
			const auto delay = chrono::duration_cast<chrono::microseconds>(chrono::milliseconds(m_trial.delay));
			m_network->delay(delay);
			m_springs[0]->markReference();
			m_springs[0]->unmarkReference();
		}
		m_config->unlock();
		m_slave->spring(m_springs[0]);
	}

	void showConfig() override
	{
		m_packetRateLabel->setText(std::to_string(int64_t(m_trial.packetRate)) + " Hz");
		m_delayLabel->setText(std::to_string(int64_t(m_trial.delay)) + " ms");
		switch (m_trial.controlAlgo)
		{
			case ControlAlgorithm::None:
				m_algorithmLabels[0]->setText("REF");
				break;
			case ControlAlgorithm::WAVE:
				m_algorithmLabels[0]->setText("WAVE");
				break;
			case ControlAlgorithm::ISS:
				m_algorithmLabels[0]->setText("ISS");
				break;
			case ControlAlgorithm::PC:
				m_algorithmLabels[0]->setText("TDPA");
				break;
			case ControlAlgorithm::MMT:
				m_algorithmLabels[0]->setText("MMT");
				break;
		}
	}

public:
	TrialController(Slave* slave, Master* master, Config* config, Network* network, DB* db,
		const array<cLabel*, 5>& sideLabels, const array<Spring*, 5>& springs, const array<cLabel*, 5>& algorithmLabels,
		cLabel* packetRateLabel, cLabel* delayLabel)
		: Controller(slave, master, config, network, db, sideLabels, springs, algorithmLabels, packetRateLabel, delayLabel)
	{
	}

	void init() override
	{
		initCurrentTrial();
		m_master->packetRate(m_trialConfig.subTrialConfigs[0].packetRate);
		m_slave->packetRate(m_trialConfig.subTrialConfigs[0].packetRate);
	}

	void rightKey() override
	{
		auto const rating = min(5, m_trial.rating + 1);
		m_trial.rating = rating;
		m_sideLabels[0]->setText("Rating: " + std::to_string(rating));
	}

	void leftKey() override
	{
		auto const rating = max(1, m_trial.rating - 1);
		m_trial.rating = rating;
		m_sideLabels[0]->setText("Rating: " + std::to_string(rating));
	}

	bool saveToDb() override
	{
		if (m_trial.rating == 0)
			return false;

		db_rate_trial(m_db, m_trial.rating);

		// move to the next trial if there is one left
		const auto trialLeft = db_next_trial(m_db);
		if (!trialLeft)
			return true;
		clearConfig();
		initCurrentTrial();
		std::cout << m_showingConfig << std::endl;
		if (m_showingConfig)
			showConfig();
		return false;
	}

	void rate(int32_t rating) override
	{
		m_trial.rating = rating;
		m_sideLabels[0]->setText("Rating: " + std::to_string(rating));
	}
};
*/
