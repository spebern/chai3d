#pragma once

#include <queue>
#include <chrono>

using namespace std;

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

	m_q.push(packet);
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
	if (!m_q.empty() && isFrontDue())
	{
		msg = m_q.front().msg;
		m_q.pop();
		return true;
	}
	return false;
}

template <typename T>
void Channel<T>::clear()
{
	while (!m_q.empty())
		m_q.pop();
}
