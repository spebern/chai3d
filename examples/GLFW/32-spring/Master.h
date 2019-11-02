#pragma once

#include "chai3d.h"
#include "Network.h"
#include "RateLimiter.h"

using namespace std;
using namespace chai3d;

class Master
{
private:
	RateLimiter m_packetRateLimiter;
	Network* m_network;
	cGenericHapticDevicePtr m_hapticDevice;
	cVector3d m_previousForce;
	int64_t m_sequenceNumber = 0;
public:
	Master(Network* network, const cGenericHapticDevicePtr hapticDevice)
		: m_network(network)
		, m_hapticDevice(hapticDevice)
		, m_previousForce(0, 0, 0)
	{
	}

	void spin();

	void sample(cVector3d& pos, cVector3d& vel) const;

	static cVector3d limitForce(cVector3d& force)
	{
		return force.length() > 5.0 ? force / force.length() * 5.0 : force;
	}
};
