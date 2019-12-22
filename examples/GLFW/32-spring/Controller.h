#pragma once

#include "Slave.h"
#include "Master.h"
#include "chai3d.h"
#include "haptic_db_ffi.h"
#include "Environment.h"

using namespace chai3d;

class Controller
{
protected:
	Slave* m_slave;
	Master* m_master;
	Network* m_network;
	DB* m_db;
	Config* m_config;

	bool m_showingConfig = false;
	bool m_useReference = false;

	Environment* m_env;
public:
	Controller(Slave* slave, Master* master, Config* config, Network* network, DB* db, Environment* env)
		: m_slave(slave)
		, m_master(master)
		, m_network(network)
		, m_db(db)
		, m_config(config)
		, m_env(env)
	{
		clearConfig();
	}

	virtual void clearConfig()
	{
		m_env->bottomMiddleLabel("");
		m_env->bottomRightLabel("");
		for (auto i = 0; i < 5; i++)
			m_env->hiddenLabel(i, "");
	}

	virtual void showConfig() {}


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

	virtual bool saveToDb() { return true; }

	virtual void rightKey() {}

	virtual void leftKey() {}

	virtual void upKey() {}

	virtual void downKey() {}

	virtual void init() {}

	virtual void rate(int32_t rating) {}

	virtual void toggleReference() {}
};
