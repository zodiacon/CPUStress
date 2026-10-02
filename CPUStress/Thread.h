#pragma once

#include <atomic>
#include <array>

struct ThreadCreateParams;

enum class ActivityLevel : uint16_t {
	None,
	Low,
	Medium,
	Busy,
	Maximum
};

enum class WorkloadType : uint16_t {
	Spin,
	Integer,
	Float,
	AVX2,
	Memory
};

class Thread {
public:
	Thread();
	Thread(HANDLE hThread, int index);

	static std::shared_ptr<Thread> Create(ThreadCreateParams* params = nullptr);

	DWORD GetId() const;
	CTime GetCreateTime() const;
	int GetCreateTimeMilliseconds() const;
	CFileTimeSpan GetCPUTime() const;
	void SetActivityLevel(ActivityLevel level);
	ActivityLevel GetActivityLevel() const {
		return _level;
	}

	// returns false (and leaves the type unchanged) if the CPU can't run the workload
	bool SetWorkloadType(WorkloadType type);
	WorkloadType GetWorkloadType() const {
		return _type;
	}
	static bool IsWorkloadSupported(WorkloadType type);

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

	// recent CPU usage samples (percent of one CPU, oldest first); returns the number copied
	static constexpr int HistorySize = 30;
	int GetHistory(uint8_t* samples) const;
	void* GetTeb() const;
	void Shutdown();
	void Suspend();
	void Resume();
	void UpdateIndex(int index);
	static int GetCPUCount();

protected:
	static DWORD WINAPI ThreadFunction(PVOID p);
	void DoWork();
	void RunChunk(WorkloadType type);

private:
	struct MemoryDeleter {
		void operator()(void* p) const {
			::VirtualFree(p, 0, MEM_RELEASE);
		}
	};
	std::unique_ptr<void, MemoryDeleter> _buffer;	// lazily allocated by the Memory workload (worker thread only)
	std::atomic<WorkloadType> _type{ WorkloadType::Spin };
	wil::unique_handle _hThread;
	wil::unique_handle _hTerminate{ ::CreateEvent(nullptr, FALSE, FALSE, nullptr) };
	std::atomic<ActivityLevel> _level{ ActivityLevel::Low };
	int _index;
	ULONGLONG _lastCpuTick{ ::GetTickCount64() };
	ULONGLONG _lastCpuTime{ 0 };
	std::atomic<int> _cpuConsumption{ 0 };
	std::array<std::atomic<uint8_t>, HistorySize> _history{};	// ring buffer, written by the worker thread
	std::atomic<int> _historyCount{ 0 };						// total samples ever written
	std::atomic<bool> _suspended{ true };
	bool _userCreated : 1;
};

