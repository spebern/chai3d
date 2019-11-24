#include "Master.h"

void Master::spin()
{
	auto const controlAlgorithm = m_config->controlAlgorithm();
	cVector3d pos, vel;
	sample(pos, vel);

	HapticMessageM2S msgM2S;
	msgM2S.sequenceNumber = m_sequenceNumber;
	m_sequenceNumber++;

	msgM2S.pos = pos;
	switch (controlAlgorithm)
	{
	case ControlAlgorithm::WAVE:
		msgM2S.vel = m_wave.calculateUm(vel);
		break;
	default:
		msgM2S.vel = vel;
		break;
	}

	if constexpr (SAVE_MSG_STREAM_TO_DB)
	{
		const auto dbMsg = hapticMessageM2StoDbMsg(msgM2S);
		const auto isReference = m_config->isReference();
		db_insert_haptic_message_m2s(m_db, Device::Master, isReference, dbMsg);
	}

	if (!m_packetRateLimiter.limited())
	{
		m_network->sendM2S(msgM2S);
	}

	HapticMessageS2M msgS2M;
	const auto receivedNewMessage = m_network->tryReceiveS2M(msgS2M);
	cVector3d force;
	if (receivedNewMessage)
	{
		if constexpr (SAVE_MSG_STREAM_TO_DB)
		{
			const auto dbMsg = hapticMessageS2MtoDbMsg(msgS2M);
			const auto isReference = m_config->isReference();
			db_insert_haptic_message_s2m(m_db, Device::Master, isReference, dbMsg);
		}

		switch (controlAlgorithm)
		{
		case ControlAlgorithm::WAVE:
			m_wave.vm(msgS2M.force);
			force = m_wave.calculateForce(vel);
			break;
		default:
			force = msgS2M.force;
			break;
		}
		m_force = force;
		m_hapticDevice->setForce(limitForce(m_force));
	}
	else
	{
		m_hapticDevice->setForce(limitForce(m_force));
	}
}

void Master::sample(cVector3d& pos, cVector3d& vel) const
{
	m_hapticDevice->getPosition(pos);
	pos.x(0);
	pos.z(0);
	m_hapticDevice->getLinearVelocity(vel);
	vel.x(0);
	vel.z(0);
}
