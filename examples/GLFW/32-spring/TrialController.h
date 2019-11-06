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
	uint32_t m_currentSubTrialIdx = 0;
	Slave* m_slave;
	Master* m_master;
	Network* m_network;
	DB* m_db;
	Config* m_config;
	array<cLabel*, 4> m_ratingLabels;
	array<int32_t, 4> m_ratings;
	array<Spring*, 4> m_springs;

	TrialInfo m_currentTrialInfo;

	void initCurrentSubTrial()
	{
		const auto controlAlgo = m_currentTrialInfo.controlAlgos[m_currentSubTrialIdx];
		m_config->controlAlgorithm(controlAlgo);
		m_slave->spring(m_springs[m_currentSubTrialIdx]);
		// ToDo: why?
		// m_network->clearChannels();
	}

	void initCurrentTrial()
	{
		m_currentTrialInfo = db_current_trial_info(m_db);
		m_slave->packetRate(m_currentTrialInfo.packetRate);
		m_master->packetRate(m_currentTrialInfo.packetRate);
		for (auto& ratingLabel: m_ratingLabels)
			ratingLabel->setText("Rating: ");
		for (auto& rating: m_ratings)
			rating = 0;
		// ToDo: set delay
		m_network->clearChannels();
	}
public:
	TrialController(Slave* slave, Master* master, Config* config, Network* network, DB* db, array<cLabel*, 4> ratingLabels, array<Spring*, 4> springs)
		: m_slave(slave)
		  , m_master(master)
		  , m_network(network)
		  , m_db(db)
		  , m_config(config)
		  , m_ratingLabels(ratingLabels)
		  , m_springs(springs)
	{
		initCurrentTrial();
		initCurrentSubTrial();
	}

	void nextSubTrial()
	{
		if (m_currentSubTrialIdx == m_springs.size() - 1)
			m_currentSubTrialIdx = 0;
		else
			m_currentSubTrialIdx++;
		initCurrentSubTrial();
	}

	void previousSubTrial()
	{
		if (m_currentSubTrialIdx == 0)
			m_currentSubTrialIdx = m_springs.size() - 1;
		else
			m_currentSubTrialIdx--;
		initCurrentSubTrial();
	}

	void rate(const int32_t rating)
	{
		m_ratingLabels[m_currentSubTrialIdx]->setText("Rating: " + std::to_string(rating));
		m_ratings[m_currentSubTrialIdx] = rating;
	}

	bool submitRatings()
	{
		for (auto& rating: m_ratings)
		{
			if (rating == 0)
				return false;
		}
		db_rate_trial(m_db, m_ratings.data(), m_ratings.size());

		// move to the next trial if there is one left
		const auto trialLeft = db_next_trial(m_db);
		if (!trialLeft)
			return false;
		initCurrentTrial();
		initCurrentSubTrial();
		return true;
	}
};
