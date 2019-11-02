#include "Master.h"

void Master::spin()
{
	cVector3d pos, vel;
	sample(pos, vel);

	HapticMessageM2S msgM2S;
	msgM2S.sequenceNumber = m_sequenceNumber;
	m_sequenceNumber++;
	msgM2S.pos = pos;

	switch (m_config->controlAlgorithm())
	{
	case ControlAlgorithm::None:
		msgM2S.vel = vel;
		break;
	case ControlAlgorithm::WAVE:
		msgM2S.vel = m_wave.calculateUm(vel, m_previousForce);
	}

	if (!m_packetRateLimiter.limited())
		m_network->sendM2S(msgM2S);

	HapticMessageS2M msgS2M;
	const auto receivedNewMessage = m_network->tryReceiveS2M(msgS2M);
	cVector3d force;
	if (receivedNewMessage)
	{
		switch (m_config->controlAlgorithm())
		{
		case ControlAlgorithm::None:
			force = msgS2M.force;
			break;
		case ControlAlgorithm::WAVE:
			force = m_wave.calculateFm(vel, msgS2M.force);
			break;
		}
		force = limitForce(force);
		m_hapticDevice->setForce(force);
		m_previousForce = force;
	}
	else
	{
		m_hapticDevice->setForce(m_previousForce);
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
