#pragma once
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

class Dispatcher
{
	std::vector<std::function<void()>> executeQueue;
	std::mutex executeMutex;

public:
	void tick();
	void run(std::function<void()> task);
};