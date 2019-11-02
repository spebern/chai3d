#include "Slave.h"
#include "Common.h"

void Slave::spin()
{
	HapticMessageM2S msgM2S;
	auto const receivedNewMsg = m_network->tryReceiveM2S(msgM2S);

	cVector3d vel, pos;
	if (receivedNewMsg)
	{
		m_posRef = msgM2S.pos;
		m_velRef = msgM2S.vel;
	}
	else
		m_posRef = m_posRef + m_velRef * DT;

	auto const pdForce = m_pdController.calculateForce(m_posRef, m_pos, m_velRef, m_vel);
	auto const springForce = m_spring->updatePositionAndCalculateForce(m_pos, m_vel);

	auto const totalForce = pdForce + springForce;

	auto const acceleration = (totalForce - m_mass * m_vel) / m_damping;
	m_vel += acceleration * DT;
	m_pos += m_vel * DT;

	HapticMessageS2M msgS2M;
	msgS2M.sequenceNumber = m_sequenceNumber;
	m_sequenceNumber++;
	msgS2M.force = springForce;

	if (!m_packetRateLimiter.limited())
		m_network->sendS2M(msgS2M);
}
