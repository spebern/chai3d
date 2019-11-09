#include "Slave.h"
#include "Config.h"

void Slave::spin()
{
	HapticMessageM2S msgM2S;
	auto const controlAlgorithm = m_config->controlAlgorithm();
	auto const receivedNewMsg = m_network->tryReceiveM2S(msgM2S);

	cVector3d vel, pos;
	if (receivedNewMsg)
	{
		if (SAVE_MSG_STREAM_TO_DB && !m_config->isRef())
		{
			auto dbMsg = hapticMessageM2StoDbMsg(msgM2S);
			db_insert_haptic_message_m2s(m_db, Device::Slave, m_config->subTrialIdx(), dbMsg);
		}

		m_posRef = msgM2S.pos;
		switch (controlAlgorithm)
		{
		case ControlAlgorithm::WAVE:
			m_velRef = m_wave.calculateVelS(msgM2S.vel, m_force);
			break;
		case ControlAlgorithm::ISS:
			m_velRef = m_iss.calculateVel(msgM2S.vel, dForce());
			break;
		default:
			m_velRef = msgM2S.vel;
			break;
		}
	}

	auto const pdForce = m_pdController.calculateForce(m_posRef, m_pos, m_velRef, m_vel);

	auto springForce = m_spring->updatePositionAndCalculateForce(m_pos, m_vel);

	updateToolTipPos();

	const auto totalForce = pdForce + springForce;

	auto const acceleration = (totalForce - m_mass * m_vel) / m_damping;

	m_prevVel = m_vel;
	m_vel += acceleration * DT;

	m_prevPos = m_pos;
	m_pos += m_vel * DT;

	HapticMessageS2M msgS2M;
	msgS2M.sequenceNumber = m_sequenceNumber;
	m_sequenceNumber++;

	switch (controlAlgorithm)
	{
	case ControlAlgorithm::WAVE:
		m_prevForce = m_force;
		m_force = springForce;
		msgS2M.force = m_wave.calculateVs(m_vel, m_force);
		break;
	case ControlAlgorithm::PC:
		m_prevForce = m_force;
		m_force = m_passivityControl.calculateForce(springForce, m_vel);
		msgS2M.force = m_force;
		break;
	case ControlAlgorithm::ISS:
		m_force = m_iss.calculateForce(springForce, dForce());
		m_iss.updateMuMax(m_force, m_prevForce, m_pos, m_prevPos);
		m_prevForce = m_force;
		msgS2M.force = m_force;
		break;
	default:
		m_prevForce = m_force;
		m_force = springForce;
		msgS2M.force = m_force;
		break;
	}

	if (SAVE_MSG_STREAM_TO_DB && !m_config->isRef())
	{
		auto dbMsg = hapticMessageS2MtoDbMsg(msgS2M);
		db_insert_haptic_message_s2m(m_db, Device::Slave, m_config->subTrialIdx(), dbMsg);
	}


	if (!m_packetRateLimiter.limited())
		m_network->sendS2M(msgS2M);
}
