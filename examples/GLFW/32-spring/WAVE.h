#pragma once

#include "chai3d.h"
#include "Config.h"

using namespace chai3d;

class WAVEMaster {
private:
	double m_b;
	cVector3d m_vm;
public:
	explicit WAVEMaster(const double b = WAVE_B) : m_vm(0, 0, 0), m_b(b) {}
	
	void vm(const cVector3d vm)
	{
		m_vm = vm;
	}

	//cVector3d calculateUm(cVector3d& vel, cVector3d& force) const
	cVector3d calculateUm(cVector3d& vel) const
	{
		// return (m_b * vel + force) / sqrt(2.0 * m_b);
		return sqrt(2.0 * m_b) * vel - m_vm;
	}

	cVector3d calculateForce(cVector3d& vel) const
	{
		return m_b * vel - sqrt(2.0 * m_b) * m_vm;
	}
};

class WAVESlave
{
private:
	double m_b;
	cVector3d m_us;

public:
	explicit WAVESlave(const double b = WAVE_B) : m_us(0, 0, 0), m_b(b) {}

	void us(const cVector3d& us)
	{
		m_us = us;
	}

	cVector3d calculateVel(cVector3d& force) const
	{
		return sqrt(2.0 / m_b) * m_us - (1.0 / m_b) * force;
	}

	cVector3d calculateVs(cVector3d& force) const
	{
		return m_us - sqrt(2.0 / m_b) * force;
	}
};
