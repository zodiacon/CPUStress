#pragma once

class Thread;
struct ThreadCreateParams;

class ThreadManager {
public:
	int GetThreadCount() const;
	void EnumThreads();
	std::shared_ptr<Thread> AddNewThread(ThreadCreateParams* params = nullptr);
	const std::vector<std::shared_ptr<Thread>>& GetThreads() const;
	const std::vector<std::shared_ptr<Thread>>& GetUserThreads() const;
	bool RemoveThread(int id);

private:
	std::vector<std::shared_ptr<Thread>> _threads;
	std::vector<std::shared_ptr<Thread>> _userThreads;
};

