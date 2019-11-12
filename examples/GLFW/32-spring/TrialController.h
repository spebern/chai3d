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

class TrialController: public Controller
{
private:
	array<int32_t, 4> m_ratings;
public:
	TrialController(Slave* slave, Master* master, Config* config, Network* network, DB* db,
		const array<cLabel*, 4>& sideLabels, const array<Spring*, 4>& springs, const array<cLabel*, 4>& algorithmLabels,
		cLabel* packetRateLabel, cLabel* delayLabel)
		: Controller(slave, master, config, network, db, sideLabels, springs, algorithmLabels, packetRateLabel, delayLabel)
	{
		initCurrentTrial();
	}

private:
	void initCurrentTrial()
	{
		auto currentTrialInfo = db_current_trial_info(m_db);
		m_trialConfig.delay = currentTrialInfo.delay;
		for (auto i = 0; i < 4; i++)
		{
			m_trialConfig.subTrialConfigs[i].controlAlgorithm = currentTrialInfo.controlAlgos[i];
			m_trialConfig.subTrialConfigs[i].packetRate = currentTrialInfo.packetRate;
		}
		for (auto& ratingLabel: m_sideLabels)
			ratingLabel->setText("Rating: ");
		for (auto& rating: m_ratings)
			rating = 0;
	}

	void showConfig() override
	{
		m_packetRateLabel->setText(std::to_string(int64_t(m_trialConfig.subTrialConfigs[0].packetRate)) + " Hz");
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
	void rate(const int32_t rating)
	{
		const auto subTrialIdx = m_config->subTrialIdx();
		m_sideLabels[subTrialIdx]->setText("Rating: " + std::to_string(rating));
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
};
