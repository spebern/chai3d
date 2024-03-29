#pragma once

#include <queue>
#include <chrono>
#include <mutex>
#include "chai3d.h"

using namespace std;
using namespace chai3d;

template <typename T>
struct Packet
{
	T msg;
	chrono::high_resolution_clock::time_point arrivalTime;
};

/**
 * \brief model of a channel that delivers messages with an optional delay
 * \tparam T type of the messages sent through this channel
 */
template <typename T>
class Channel
{
private:
	std::queue<Packet<T>> m_q;
	bool isFrontDue(chrono::high_resolution_clock::time_point now);
	std::mutex m_mu;
public:
	/**
	 * \brief send a message through the channel
	 * \param msg message to send  through the channel
	 * \param delay delay of the message
	 */
	void send(T& msg, chrono::microseconds delay);

	/**
	 * \brief try to receive a message from the channel
	 * \param msg destination of the message data if a new one was received
	 * \return whether we received a new message or not
	 */
	bool tryReceive(T& msg);

	/**
	 * \brief clear all pending messages in the channel
	 */
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

	std::unique_lock<std::mutex> lock(m_mu);
	m_q.push(packet);
}

template <typename T>
bool Channel<T>::isFrontDue(chrono::high_resolution_clock::time_point now)
{
	if (m_q.front().arrivalTime <= now)
	{
		return true;
	}
	return false;
}

template <typename T>
bool Channel<T>::tryReceive(T& msg)
{
	std::unique_lock<std::mutex> lock(m_mu);

	const auto now = chrono::high_resolution_clock::now();
	auto received = false;
	while (!m_q.empty() && isFrontDue(now))
	{
		msg = m_q.front().msg;
		m_q.pop();
		received = true;
	}
	return received;
}

template <typename T>
void Channel<T>::clear()
{
	std::unique_lock<std::mutex> lock(m_mu);
	while (!m_q.empty())
		m_q.pop();
}
