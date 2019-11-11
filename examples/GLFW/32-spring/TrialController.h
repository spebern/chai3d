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

using namespace chai3d;

class TrialController
{
private:
	Slave* m_slave;
	Master* m_master;
	Network* m_network;
	DB* m_db;
	Config* m_config;
	array<cLabel*, 4> m_ratingLabels;
	array<cLabel*, 4> m_algorithmLabels;
	array<int32_t, 4> m_ratings;
	array<Spring*, 4> m_springs;

	cLabel* m_packetRateLabel;
	cLabel* m_delayLabel;

	TrialInfo m_currentTrialInfo;

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
			const auto controlAlgo = m_currentTrialInfo.controlAlgos[subTrialIdx];
			m_config->isRef(false);
			m_config->controlAlgorithm(controlAlgo);
			m_master->packetRate(m_currentTrialInfo.packetRate);
			m_slave->packetRate(m_currentTrialInfo.packetRate);
			m_springs[subTrialIdx]->unmarkReference();
		}
		m_slave->spring(m_springs[subTrialIdx]);
	}

	void initCurrentTrial()
	{
		m_currentTrialInfo = db_current_trial_info(m_db);
		for (auto& ratingLabel: m_ratingLabels)
			ratingLabel->setText("Rating: ");
		for (auto& rating: m_ratings)
			rating = 0;
	}

	void clearConfig()
	{
		m_packetRateLabel->setText("");
		m_delayLabel->setText("");
		for (auto& label : m_algorithmLabels)
			label->setText("");
	}

	void showConfig() const
	{
		m_packetRateLabel->setText(std::to_string(m_currentTrialInfo.packetRate) + " Hz");
		m_delayLabel->setText(std::to_string(m_currentTrialInfo.delay) + " ms");
		for (auto i = 0; i < m_algorithmLabels.size(); i++)
		{
			switch (m_currentTrialInfo.controlAlgos[i])
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
	TrialController(Slave* slave, Master* master, Config* config, Network* network, DB* db,
	                array<cLabel*, 4> ratingLabels, array<Spring*, 4> springs, array<cLabel*, 4> algorithmLabels,
	                cLabel* packetRateLabel, cLabel* delayLabel)
		: m_slave(slave)
		, m_master(master)
		, m_network(network)
		, m_db(db)
		, m_config(config)
		, m_ratingLabels(ratingLabels)
		, m_algorithmLabels(algorithmLabels)
		, m_springs(springs)
		, m_packetRateLabel(packetRateLabel)
		, m_delayLabel(delayLabel)
	{
		clearConfig();
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

	void rate(const int32_t rating)
	{
		const auto subTrialIdx = m_config->subTrialIdx();
		m_ratingLabels[subTrialIdx]->setText("Rating: " + std::to_string(rating));
		m_ratings[subTrialIdx] = rating;
	}

	bool submitRatings()
	{
		for (auto& rating: m_ratings)
		{
			if (rating == 0)
				return true;
		}
		db_rate_trial(m_db, m_ratings.data(), m_ratings.size());

		// move to the next trial if there is one left
		const auto trialLeft = db_next_trial(m_db);
		if (!trialLeft)
			return false;
		clearConfig();
		initCurrentTrial();
		initCurrentSubTrial();
		if (m_showingConfig)
			showConfig();
		return true;
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
};
