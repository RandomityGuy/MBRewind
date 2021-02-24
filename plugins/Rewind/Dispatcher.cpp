#include "Dispatcher.h"

void Dispatcher::tick()
{
	this->executeMutex.lock();
	for (int i = 0; i < executeQueue.size(); i++)
	{
		std::function<void()> fn = executeQueue[i];
		fn();
	}
	executeQueue.clear();
	this->executeMutex.unlock();
}

void Dispatcher::run(std::function<void()> fn)
{
	this->executeMutex.lock();
	this->executeQueue.push_back(fn);
	this->executeMutex.unlock();
}