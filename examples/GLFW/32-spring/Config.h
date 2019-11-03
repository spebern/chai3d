#pragma once

#define DT 0.001

enum class ControlAlgorithm
{
	None,
	WAVE,
	PassivityControl,
	ISS
};

class Config
{
private:
	ControlAlgorithm m_controlAlgorithm;
public:
	Config(): m_controlAlgorithm(ControlAlgorithm::None) {}

	void controlAlgorithm(const ControlAlgorithm controlAlgorithm)
	{
		m_controlAlgorithm = controlAlgorithm;
	}

	ControlAlgorithm controlAlgorithm() const
	{
		return m_controlAlgorithm;
	}
};
