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
		m_posRef = msgM2S.pos;
		switch (controlAlgorithm)
		{
		case ControlAlgorithm::WAVE:
			m_velRef = m_wave.calculateVelS(msgM2S.vel, m_prevForce);
			break;
		case ControlAlgorithm::ISS:
			m_velRef = m_iss.calculateVel(msgM2S.vel, dForce());
			break;
		default:
			m_velRef = msgM2S.vel;
			break;
		}
	}
	else
		m_posRef = m_posRef + m_velRef * DT;

	auto const pdForce = m_pdController.calculateForce(m_posRef, m_pos, m_velRef, m_vel);

	auto springForce = currentSpring()->updatePositionAndCalculateForce(m_pos, m_vel);

	updateToolTipPos();

	auto const totalForce = pdForce + springForce;

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
		m_iss.updateMuMax(m_force, m_prevForce, m_pos, m_prevPos);
		m_prevForce = m_force;
		m_force = m_iss.calculateForce(springForce, dForce());
		msgS2M.force = m_force;
		break;
	default:
		m_prevForce = m_force;
		m_force = springForce;
		msgS2M.force = m_force;
		break;
	}

	if (!m_packetRateLimiter.limited())
		m_network->sendS2M(msgS2M);
}
