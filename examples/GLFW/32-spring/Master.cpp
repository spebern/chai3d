#include "Master.h"

void Master::spin()
{
	cVector3d pos, vel;
	sample(pos, vel);

	HapticMessageM2S msgM2S;
	msgM2S.sequenceNumber = m_sequenceNumber;
	msgM2S.pos = pos;
	msgM2S.vel = vel;
	m_sequenceNumber++;

	if (!m_packetRateLimiter.limited())
		m_network->sendM2S(msgM2S);

	HapticMessageS2M msgS2M;
	const auto receivedNewMessage = m_network->tryReceiveS2M(msgS2M);
	if (receivedNewMessage)
	{
		m_previousForce = limitForce(msgS2M.force);
		m_hapticDevice->setForce(m_previousForce);
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
