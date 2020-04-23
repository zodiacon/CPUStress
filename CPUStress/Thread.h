#pragma once

#include <atomic>

struct ThreadCreateParams;

enum class ActivityLevel : uint16_t {
	None,
	Low,
	Medium,
	Busy,
	Maximum
};

class Thread {
public:
	Thread();
	Thread(HANDLE hThread, int index);

	static std::shared_ptr<Thread> Create(ThreadCreateParams* params = nullptr);

	DWORD GetId() const;
	CTime GetCreateTime() const;
	CFileTimeSpan GetCPUTime() const;
	void SetActivityLevel(ActivityLevel level);
	ActivityLevel GetActivityLevel() const {
		return _level;
	}

	int GetIndex() const {
		return _index;
	}

	operator HANDLE() const {
		return _hThread.get();
	}

	bool IsUserCreated() const {
		return _userCreated;
	}
	bool IsSuspended() const;
	int GetIdealCPU() const;
	void SetIdealCPU(int cpu);
	DWORD_PTR GetAffinity() const;
	void SetAffinity(DWORD_PTR affinity);
	void GetStackLimits(void*& start, void*& end) const;
	int GetBasePriority() const;
	int GetPriority() const;
	bool GetCpuSet(std::vector<ULONG>&) const;

	void SetBasePriority(int priority);
	bool SetCPUSet(ULONG* set, ULONG count);

	int GetCPU() const {
		return _cpuConsumption;
	}
	void* GetTeb() const;
	void Shutdown();
	void Suspend();
	void Resume();
	void UpdateIndex(int index);
	static int GetCPUCount();

protected:
	static DWORD WINAPI ThreadFunction(PVOID p);
	void DoWork();

private:
	wil::unique_handle _hThread;
	wil::unique_handle _hTerminate{ ::CreateEvent(nullptr, FALSE, FALSE, nullptr) };
	std::atomic<ActivityLevel> _level{ ActivityLevel::Low };
	int _index;
	ULONGLONG _lastCpuTick{ ::GetTickCount64() };
	ULONGLONG _lastCpuTime{ 0 };
	std::atomic<int> _cpuConsumption{ 0 };
	std::atomic<bool> _suspended{ true };
	bool _userCreated : 1;
};

