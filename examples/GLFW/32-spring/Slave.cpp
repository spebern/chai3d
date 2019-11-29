#include "Slave.h"
#include "Config.h"

void Slave::spin()
{
	HapticMessageM2S msgM2S;
	auto const controlAlgorithm = m_config->controlAlgorithm();
	auto const receivedNewMsg = m_network->tryReceiveM2S(msgM2S);

	const auto indention = m_spring->calcIndention(m_pos);
	const auto springForce = m_spring->updatePositionAndCalculateForce(m_pos, m_vel);

	cVector3d vel, pos;
	if (receivedNewMsg)
	{
		if constexpr (SAVE_MSG_STREAM_TO_DB)
		{
			const auto dbMsg = hapticMessageM2StoDbMsg(msgM2S);
			const auto isReference = m_config->isReference();
			db_insert_haptic_message_m2s(m_db, Device::Slave, isReference, dbMsg);
		}

		m_posRef = msgM2S.pos;
		switch (controlAlgorithm)
		{
		case ControlAlgorithm::WAVE:
			m_velRef = m_wave.calculateVel(m_force);
			break;
		case ControlAlgorithm::ISS:
			m_velRef = m_iss.calculateVel(msgM2S.vel, springForce, m_force);
			break;
		default:
			m_velRef = msgM2S.vel;
			break;
		}
	}

	// auto const pdForce = m_pdController.calculateForce(m_posRef, m_pos, m_velRef, m_vel);
	auto const pdForce = m_pidController.calculateForce(m_posRef, m_pos, m_velRef, m_vel);

	auto const totalForce = pdForce + springForce;

	auto const acceleration = totalForce / m_mass;

	m_prevVel = m_vel;
	m_vel += acceleration * DT;
	m_vel *= (1 - m_damping);

	m_prevPos = m_pos;
	m_pos += m_vel * DT;

	updateToolTipPos();

	HapticMessageS2M msgS2M;
	msgS2M.sequenceNumber = m_sequenceNumber;
	m_sequenceNumber++;

	switch (controlAlgorithm)
	{
	case ControlAlgorithm::WAVE:
		msgS2M.force = m_wave.calculateVs(m_force);
		m_force = springForce;
		break;
	case ControlAlgorithm::PC:
		m_force = m_passivityControl.calculateForce(springForce, m_vel);
		msgS2M.force = m_force;
		break;
	case ControlAlgorithm::ISS:
		msgS2M.force = m_iss.calculateForce(springForce, m_force);
		m_force = springForce;
		break;
	case ControlAlgorithm::MMT:
		m_mmt.updateK(springForce.y(), m_pos);
		msgS2M.force.x(0);
		msgS2M.force.y(m_mmt.k());
		msgS2M.force.z(0);
		break;
	default:
		m_force = springForce;
		msgS2M.force = m_force;
		break;
	}

	const auto transmit = DEADBAND_ACTIVE && m_deadbandDetector.inDeadband(springForce);
	if (transmit && !m_packetRateLimiter.limited())
		m_network->sendS2M(msgS2M);

	if constexpr (SAVE_MSG_STREAM_TO_DB)
	{
		const auto dbMsg = hapticMessageS2MtoDbMsg(msgS2M);
		const auto isReference = m_config->isReference();
		db_insert_haptic_message_s2m(m_db, Device::Slave, isReference, dbMsg);
	}

	if (!m_packetRateLimiter.limited())
		m_network->sendS2M(msgS2M);
}
