#pragma once

#include <queue>
#include <chrono>
#include "chai3d.h"

using namespace std;
using namespace chai3d;

template <typename T>
struct Packet
{
	T msg;
	chrono::high_resolution_clock::time_point arrivalTime;
};

template <typename T>
class Channel
{
private:
	std::queue<Packet<T>> m_q;
	bool isFrontDue();
	cMutex m_mu;
public:
	void send(T& msg, chrono::microseconds delay);
	bool tryReceive(T& msg);
	void clear();
};

template <typename T>
void Channel<T>::send(T& msg, const chrono::microseconds delay)
{
	Packet<T> packet;

	const auto now = chrono::high_resolution_clock::now();
	const auto arrivalTime = now + delay;

	packet.msg = msg;
	packet.arrivalTime = arrivalTime;

	if (!m_mu.acquire())
		return;
	m_q.push(packet);
	m_mu.release();
}

template <typename T>
bool Channel<T>::isFrontDue()
{
	const auto now = chrono::high_resolution_clock::now();
	if (m_q.front().arrivalTime <= now)
	{
		return true;
	}
	return false;
}

template <typename T>
bool Channel<T>::tryReceive(T& msg)
{
	if (!m_mu.acquire())
		return false;
	if (!m_q.empty() && isFrontDue())
	{
		msg = m_q.front().msg;
		m_q.pop();
		m_mu.release();
		return true;
	}
	m_mu.release();
	return false;
}

template <typename T>
void Channel<T>::clear()
{
	if (!m_mu.acquire())
		return;
	while (!m_q.empty())
		m_q.pop();
	m_mu.release();
}
