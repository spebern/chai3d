#include "Slave.h"
#include "Config.h"
#include "Common.h"

void Slave::spin()
{
	m_config->lock();
	auto const controlAlgorithm = m_config->controlAlgorithm();
	const auto isReference = m_config->isReference();
	const auto springIndex = m_config->springIdx();
	m_config->unlock();

	HapticMessageM2S msgM2S;
	auto const receivedNewMsg = m_network->tryReceiveM2S(msgM2S);

	cVector3d springForce(0, 0, 0);
	cVector3d springPos(0, 0,0);
	m_spring.calcForceAndPosition(m_pos, m_vel, springForce, springPos);
	/*
	m_env->springPosY(springIndex, springPos.y());
	m_env->toolTipPos(springIndex, m_pos.y());
	*/

	cVector3d vel, pos;
	if (receivedNewMsg)
	{
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

	auto const pdForce = USE_PD
		? m_pdController.calculateForce(m_posRef, m_pos, m_velRef, m_vel)
		: m_pidController.calculateForce(m_posRef, m_pos, m_velRef, m_vel);;

	auto const totalForce = pdForce + springForce;

	auto const acceleration = totalForce / m_mass;

	m_prevVel = m_vel;
	m_vel += acceleration * DT;
	m_vel *= (1 - m_damping);

	m_prevPos = m_pos;
	m_pos += m_vel * DT;

	HapticMessageS2M msgS2M;
	msgS2M.sequenceNumber = m_sequenceNumber;
	m_sequenceNumber++;
	msgS2M.pos = m_pos;

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

	const auto transmit = DEADBAND_ACTIVE ? !m_deadbandDetector.inDeadband(springForce) : true;
	if (transmit && !m_packetRateLimiter.limited())
		m_network->sendS2M(msgS2M);

	if constexpr (SAVE_MSG_STREAM_TO_DB) {
		State state;
		for (auto i = 0; i < 3; i++)
		{
			state.pos[i] = m_pos.get(i);
			state.vel[i] = m_vel.get(i);
			state.force[i] = springForce.get(i);
		}
		state.slaveUpdate = transmit;
		state.masterUpdate = receivedNewMsg;
		state.device = Device::Slave;
		db_insert_haptic_state(m_db, isReference, state);
	}
}
