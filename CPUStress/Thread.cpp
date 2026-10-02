#include "pch.h"
#include "Thread.h"
#include "NtDll.h"
#include <immintrin.h>
#include <cmath>

Thread::Thread(HANDLE hThread, int index) 
	: _hThread(hThread), _index(index), _userCreated(false), _level(ActivityLevel::None) {
}

std::shared_ptr<Thread> Thread::Create(ThreadCreateParams* params) {
	auto t = std::make_shared<Thread>();
	HANDLE hThread = ::CreateThread(nullptr, 0, Thread::ThreadFunction, t.get(), 0, nullptr);
	t->_hThread.reset(hThread);
	return t;
}

DWORD Thread::GetId() const {
	return ::GetThreadId(_hThread.get());
}

CTime Thread::GetCreateTime() const {
	FILETIME create, dummy;
	::GetThreadTimes(_hThread.get(), &create, &dummy, &dummy, &dummy);
	return CTime(create);
}

int Thread::GetCreateTimeMilliseconds() const {
	FILETIME create, dummy;
	::GetThreadTimes(_hThread.get(), &create, &dummy, &dummy, &dummy);
	return (int)(*(ULONGLONG*)&create / 10000 % 1000);
}

CFileTimeSpan Thread::GetCPUTime() const {
	FILETIME kernel, user, dummy;
	::GetThreadTimes(_hThread.get(), &dummy, &dummy, &kernel, &user);
	return CFileTimeSpan(*(ULONGLONG*)&kernel + *(ULONGLONG*)&user);
}

void Thread::SetActivityLevel(ActivityLevel level) {
	_level = level;
}

int Thread::GetHistory(uint8_t* samples) const {
	auto total = _historyCount.load();
	int n = min(total, HistorySize);
	for (int i = 0; i < n; i++)
		samples[i] = _history[(total - n + i) % HistorySize];
	return n;
}

bool Thread::IsWorkloadSupported(WorkloadType type) {
	if (type == WorkloadType::AVX2)
		return ::IsProcessorFeaturePresent(PF_AVX2_INSTRUCTIONS_AVAILABLE);
	return true;
}

bool Thread::SetWorkloadType(WorkloadType type) {
	if (!IsWorkloadSupported(type))
		return false;
	_type = type;
	return true;
}

bool Thread::IsSuspended() const {
	return _suspended;
}

void Thread::Suspend() {
	_suspended = true;
}

void Thread::Resume() {
	_suspended = false;
}

void Thread::UpdateIndex(int index) {
	_index = index;
}

int Thread::GetCPUCount() {
	static int count = 0;
	if (count == 0)
		count = ::GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
	return count;
}

int Thread::GetIdealCPU() const {
	PROCESSOR_NUMBER proc;
	::GetThreadIdealProcessorEx(_hThread.get(), &proc);
	return proc.Group * 64 + proc.Number;
}

void Thread::SetIdealCPU(int cpu) {
	PROCESSOR_NUMBER n = { 0 };
	n.Number = cpu;
	::SetThreadIdealProcessorEx(_hThread.get(), &n, nullptr);
}

DWORD_PTR Thread::GetAffinity() const {
	NT::THREAD_BASIC_INFORMATION info;
	NT::NtQueryInformationThread(_hThread.get(), NT::ThreadInfoClass::ThreadBasicInformation, &info, sizeof(info), nullptr);
	return info.AffinityMask;
}

void Thread::SetAffinity(DWORD_PTR affinity) {
	::SetThreadAffinityMask(_hThread.get(), affinity);
}

void Thread::GetStackLimits(void*& start, void*& end) const {
	NT::THREAD_BASIC_INFORMATION info;
	NT::NtQueryInformationThread(_hThread.get(), NT::ThreadInfoClass::ThreadBasicInformation, &info, sizeof(info), nullptr);
	auto tib = (NT::NT_TIB*)info.TebBaseAddress;
	if (tib) {
		start = tib->StackBase;
		end = tib->StackLimit;
	}
}

int Thread::GetBasePriority() const {
	return ::GetThreadPriority(_hThread.get());
}

int Thread::GetPriority() const {
	NT::THREAD_BASIC_INFORMATION info;
	NT::NtQueryInformationThread(_hThread.get(), NT::ThreadInfoClass::ThreadBasicInformation, &info, sizeof(info), nullptr);
	return info.Priority;
}

 bool Thread::GetCpuSet(std::vector<ULONG>& sets ) const {
	ULONG count = 0;
	::GetThreadSelectedCpuSets(_hThread.get(), nullptr, 0, &count);		// fails, but reports the size needed
	sets.resize(count);
	if (count == 0)
		return true;
	return ::GetThreadSelectedCpuSets(_hThread.get(), sets.data(), count, &count) != FALSE;
}

void Thread::SetBasePriority(int priority) {
	::SetThreadPriority(_hThread.get(), priority);
}

bool Thread::SetCPUSet(ULONG* set, ULONG count) {
	return ::SetThreadSelectedCpuSets(_hThread.get(), set, count);
}

void* Thread::GetTeb() const {
	NT::THREAD_BASIC_INFORMATION info;
	NT::NtQueryInformationThread(_hThread.get(), NT::ThreadInfoClass::ThreadBasicInformation, &info, sizeof(info), nullptr);
	return info.TebBaseAddress;
}

void Thread::Shutdown() {
	::SetEvent(_hTerminate.get());
}

Thread::Thread() : _userCreated(true) {
	Suspend();
}

DWORD __stdcall Thread::ThreadFunction(PVOID p) {
	reinterpret_cast<Thread*>(p)->DoWork();
	return 0;
}

void Thread::DoWork() {
	for (;;) {
		if (::WaitForSingleObject(_hTerminate.get(), 0) == WAIT_OBJECT_0)
			break;

		if (_suspended) {
			::Sleep(100);
			continue;
		}
		ULONGLONG kernel, user, dummy;
		if (::GetThreadTimes(_hThread.get(), (FILETIME*)&dummy, (FILETIME*)&dummy, (FILETIME*)&kernel, (FILETIME*)&user)) {
			auto current = ::GetTickCount64();
			if (current - _lastCpuTick > 400) {
				_cpuConsumption = (int)((kernel + user - _lastCpuTime) / (current - _lastCpuTick)) / GetCPUCount();
				_lastCpuTick = current;
				_lastCpuTime = kernel + user;
				// _cpuConsumption is in 1/100 percent of the whole machine; store percent of a single CPU
				auto count = _historyCount.load();
				_history[count % HistorySize] = (uint8_t)min(100, _cpuConsumption * GetCPUCount() / 100);
				_historyCount = count + 1;
			}
		}
		auto level = _level.load();
		auto type = _type.load();
		if (level == ActivityLevel::Maximum) {
			RunChunk(type);
		}
		else {
			auto time = ::GetTickCount64();
			while (::GetTickCount64() - time < (unsigned)level * 25)
				RunChunk(type);
			::Sleep(100 - (int)level * 25);
		}
	}
}

// one short slice of work (well under a millisecond) so activity level and workload changes take effect quickly
void Thread::RunChunk(WorkloadType type) {
	static volatile uint64_t sink;	// keeps the optimizer from deleting the loops; races between threads are harmless

	switch (type) {
		case WorkloadType::Integer: {
			uint64_t a = 88172645463325252ULL, b = 0x9E3779B97F4A7C15ULL, c = 1, d = 7;
			for (int i = 0; i < 20000; i++) {
				a = a * 6364136223846793005ULL + 1442695040888963407ULL;
				b ^= b << 13; b ^= b >> 7; b ^= b << 17;
				c = _rotl64(c * 0x100000001B3ULL ^ a, 5);
				d += (c ^ b) * 31;
			}
			sink = a ^ b ^ c ^ d;
			break;
		}

		case WorkloadType::Float: {
			double x = 0.5, y = 1.5;
			for (int i = 0; i < 5000; i++) {
				x = std::sin(x) * std::cos(y) + 1.0001;
				y = std::sqrt(x * x + y * 0.001) + 0.5;
			}
			sink = (uint64_t)(x * 1000 + y);
			break;
		}

		case WorkloadType::AVX2: {
			// independent FMA chains so the FMA units stay saturated; values stay bounded
			__m256d acc[8];
			for (int i = 0; i < 8; i++)
				acc[i] = _mm256_set1_pd(0.1 * (i + 1));
			const __m256d mul = _mm256_set1_pd(0.999999), add = _mm256_set1_pd(0.000001);
			for (int i = 0; i < 20000; i++)
				for (int j = 0; j < 8; j++)
					acc[j] = _mm256_fmadd_pd(acc[j], mul, add);
			double out[4];
			_mm256_storeu_pd(out, _mm256_add_pd(acc[0], acc[7]));
			sink = (uint64_t)(out[0] * 1000);
			break;
		}

		case WorkloadType::Memory: {
			// random read-modify-write over a buffer much larger than any cache
			constexpr size_t Size = 64 << 20;
			constexpr size_t Count = Size / sizeof(uint64_t);
			if (!_buffer)
				_buffer.reset(::VirtualAlloc(nullptr, Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
			if (!_buffer) {		// out of memory: degrade to a spin
				RunChunk(WorkloadType::Spin);
				break;
			}
			auto p = static_cast<uint64_t*>(_buffer.get());
			uint64_t x = sink | 1;
			for (int i = 0; i < 8000; i++) {
				x ^= x << 13; x ^= x >> 7; x ^= x << 17;
				p[x % Count] += x;
			}
			sink = x;
			break;
		}

		default: {
			uint64_t n = 0;
			for (int i = 0; i < 2000; i++)
				n += sink + i;	// volatile read per iteration: can't be folded to a constant
			sink = n;
			break;
		}
	}
}
