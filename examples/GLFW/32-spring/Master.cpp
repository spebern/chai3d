#include "Master.h"
#include "Common.h"

void Master::spin()
{
	m_config->lock();
	auto const controlAlgorithm = m_config->controlAlgorithm();
	auto const forceFeedback = m_config->forceFeedback();
	const auto isReference = m_config->isReference();
	const auto springIdx = m_config->springIdx();
	m_config->unlock();

	sample();

	HapticMessageM2S msgM2S;
	msgM2S.sequenceNumber = m_sequenceNumber;
	m_sequenceNumber++;


	msgM2S.pos = m_pos;

	switch (controlAlgorithm)
	{
	case ControlAlgorithm::WAVE:
		msgM2S.vel = m_wave.calculateUm(m_vel);
		break;
	default:
		msgM2S.vel = m_vel;
		break;
	}

	const auto transmit = !m_packetRateLimiter.limited();
	if (transmit)
		m_network->sendM2S(msgM2S);

	HapticMessageS2M msgS2M;
	const auto receivedNewMessage = m_network->tryReceiveS2M(msgS2M);
	cVector3d force;

	if (receivedNewMessage)
	{
		switch (controlAlgorithm)
		{
		case ControlAlgorithm::WAVE:
			m_wave.vm(msgS2M.force);
			force = m_wave.calculateForce(m_vel);
			break;
		case ControlAlgorithm::MMT:
		{
			m_mmt.k(msgS2M.force.y());
			force = m_mmt.calculateForce(m_pos);
			break;
		}
		default:
			force = msgS2M.force;
			break;
		}
		m_force = force;

		if (forceFeedback)
			m_hapticDevice->setForce(limitForce(m_force));

		m_env->springPosY(springIdx, msgS2M.pos.y());
		m_env->toolTipPos(springIdx, msgS2M.pos.y());
	}
	else
	{
		if (controlAlgorithm == ControlAlgorithm::MMT)
			m_force = m_mmt.calculateForce(m_pos);

		if (forceFeedback)
			m_hapticDevice->setForce(limitForce(m_force));
	}

	if constexpr (SAVE_MSG_STREAM_TO_DB) {
		State state;
		for (auto i = 0; i < 3; i++)
		{
			state.pos[i] = m_pos.get(i);
			state.vel[i] = m_vel.get(i);
			state.force[i] = m_force.get(i);
		}
		state.masterUpdate = transmit;
		state.slaveUpdate = receivedNewMessage;
		state.device = Device::Master;
		db_insert_haptic_state(m_db, isReference, state);
	}
}

void Master::sample()
{
	m_hapticDevice->getPosition(m_pos);
	m_pos.x(0);
	m_pos.z(0);
	m_hapticDevice->getLinearVelocity(m_vel);
	m_vel.x(0);
	m_vel.z(0);
}
