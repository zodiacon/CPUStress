#include "pch.h"
#include "ThreadManager.h"
#include "Thread.h"
#include "NtDll.h"
#include <algorithm>

#pragma comment(lib, "ntdll")

int ThreadManager::GetThreadCount() const {
	return static_cast<int>(_threads.size());
}

void ThreadManager::EnumThreads() {
	HANDLE hThread = nullptr;

	int index = 0;
	std::vector<HANDLE> hThreads;
	hThreads.reserve(16);
	auto it = std::remove_if(_threads.begin(), _threads.end(), [](auto& t) { return !t->IsUserCreated(); });
	_threads.erase(it, _threads.end());
	_userThreads = _threads;

	while (STATUS_SUCCESS == NT::NtGetNextThread(GetCurrentProcess(), hThread, THREAD_ALL_ACCESS, 0, 0, &hThread)) {
		if (::WaitForSingleObject(hThread, 0) == WAIT_OBJECT_0) {
			::CloseHandle(hThread);
			continue;
		}
		auto it = std::find_if(_threads.begin(), _threads.end(), [hThread, this](auto& t) { return ::GetThreadId(hThread) == t->GetId(); });
		if (it == _threads.end()) {
			auto t = std::make_shared<Thread>(hThread, index);
			_threads.push_back(t);
		}
		else {
			(*it)->UpdateIndex(index);
			hThreads.push_back(hThread);
		}
		index++;
	}

	std::for_each(hThreads.begin(), hThreads.end(), [](auto h) { ::CloseHandle(h); });
}

std::shared_ptr<Thread> ThreadManager::AddNewThread(ThreadCreateParams* params) {
	auto t = Thread::Create(params);
	_threads.push_back(t);
	return t;
}

const std::vector<std::shared_ptr<Thread>>& ThreadManager::GetThreads() const {
	return _threads;
}

const std::vector<std::shared_ptr<Thread>>& ThreadManager::GetUserThreads() const {
	return _userThreads;
}

bool ThreadManager::RemoveThread(int id) {
	auto it = std::find_if(_threads.begin(), _threads.end(), [id](auto& t) { return t->GetId() == id; });
	if (it == _threads.end())
		return false;

	auto t = *it;
	t->Shutdown();
	_threads.erase(it);
	return true;
}
