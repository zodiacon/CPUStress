#include "stdafx.h"
#include "Thread.h"
#include "NtDll.h"

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

CFileTimeSpan Thread::GetCPUTime() const {
	FILETIME kernel, user, dummy;
	::GetThreadTimes(_hThread.get(), &dummy, &dummy, &kernel, &user);
	return CFileTimeSpan(*(ULONGLONG*)&kernel + *(ULONGLONG*)&user);
}

void Thread::SetActivityLevel(ActivityLevel level) {
	_level = level;
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

int Thread::GetPriority() const {
	return ::GetThreadPriority(_hThread.get());
}

void Thread::SetPriority(int priority) {
	::SetThreadPriority(_hThread.get(), priority);
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
				_cpuConsumption = (kernel + user - _lastCpuTime) / (current - _lastCpuTick) / GetCPUCount();
				_lastCpuTick = current;
				_lastCpuTime = kernel + user;
			}
		}
		auto level = _level.load();
		if (level != ActivityLevel::Maximum) {
			auto time = ::GetTickCount64();
			while (::GetTickCount64() - time < (unsigned)level * 25)
				;
			::Sleep(100 - (int)level * 25);
		}
	}
}
