#pragma once

namespace NT {
	enum class ThreadInfoClass {
		ThreadBasicInformation, // q: THREAD_BASIC_INFORMATION
		ThreadTimes, // q: KERNEL_USER_TIMES
		ThreadPriority, // s: KPRIORITY
		ThreadBasePriority, // s: LONG
		ThreadAffinityMask, // s: KAFFINITY
		ThreadImpersonationToken, // s: HANDLE
		ThreadDescriptorTableEntry, // q: DESCRIPTOR_TABLE_ENTRY (or WOW64_DESCRIPTOR_TABLE_ENTRY)
		ThreadEnableAlignmentFaultFixup, // s: BOOLEAN
		ThreadEventPair,
		ThreadQuerySetWin32StartAddress, // q: PVOID
		ThreadZeroTlsCell, // 10
		ThreadPerformanceCount, // q: LARGE_INTEGER
		ThreadAmILastThread, // q: ULONG
		ThreadIdealProcessor, // s: ULONG
		ThreadPriorityBoost, // qs: ULONG
		ThreadSetTlsArrayAddress,
		ThreadIsIoPending, // q: ULONG
		ThreadHideFromDebugger, // s: void
		ThreadBreakOnTermination, // qs: ULONG
		ThreadSwitchLegacyState,
		ThreadIsTerminated, // q: ULONG // 20
		ThreadLastSystemCall, // q: THREAD_LAST_SYSCALL_INFORMATION
		ThreadIoPriority, // qs: IO_PRIORITY_HINT
		ThreadCycleTime, // q: THREAD_CYCLE_TIME_INFORMATION
		ThreadPagePriority, // q: ULONG
		ThreadActualBasePriority,
		ThreadTebInformation, // q: THREAD_TEB_INFORMATION (requires THREAD_GET_CONTEXT + THREAD_SET_CONTEXT)
		ThreadCSwitchMon,
		ThreadCSwitchPmu,
		ThreadWow64Context, // q: WOW64_CONTEXT
		ThreadGroupInformation, // q: GROUP_AFFINITY // 30
		ThreadUmsInformation, // q: THREAD_UMS_INFORMATION
		ThreadCounterProfiling,
		ThreadIdealProcessorEx, // q: PROCESSOR_NUMBER
		ThreadCpuAccountingInformation, // since WIN8
		ThreadSuspendCount, // since WINBLUE
		ThreadHeterogeneousCpuPolicy, // q: KHETERO_CPU_POLICY // since THRESHOLD
		ThreadContainerId, // q: GUID
		ThreadNameInformation, // qs: THREAD_NAME_INFORMATION
		ThreadSelectedCpuSets,
		ThreadSystemThreadInformation, // q: SYSTEM_THREAD_INFORMATION // 40
		ThreadActualGroupAffinity, // since THRESHOLD2
		ThreadDynamicCodePolicyInfo,
		ThreadExplicitCaseSensitivity, // qs: ULONG; s: 0 disables, otherwise enables
		ThreadWorkOnBehalfTicket,
		ThreadSubsystemInformation, // q: SUBSYSTEM_INFORMATION_TYPE // since REDSTONE2
		ThreadDbgkWerReportActive,
		ThreadAttachContainer,
		ThreadManageWritesToExecutableMemory, // MANAGE_WRITES_TO_EXECUTABLE_MEMORY // since REDSTONE3
		ThreadPowerThrottlingState, // THREAD_POWER_THROTTLING_STATE
		ThreadWorkloadClass, // THREAD_WORKLOAD_CLASS // since REDSTONE5 // 50
		MaxThreadInfoClass
	};

	typedef struct _CLIENT_ID {
		HANDLE UniqueProcess;
		HANDLE UniqueThread;
	} CLIENT_ID, * PCLIENT_ID;

	typedef LONG KPRIORITY;

	typedef struct _NT_TIB {
		PVOID ExceptionList;
		PVOID StackBase;
		PVOID StackLimit;
		PVOID SubSystemTib;
		PVOID FiberData;
		ULONG Version;
		PVOID ArbitraryUserPointer;
		_NT_TIB* Self;
	} NT_TIB;

	typedef struct _THREAD_BASIC_INFORMATION {
		NTSTATUS ExitStatus;
		PVOID TebBaseAddress;
		CLIENT_ID ClientId;
		ULONG_PTR AffinityMask;
		KPRIORITY Priority;
		LONG BasePriority;
	} THREAD_BASIC_INFORMATION, * PTHREAD_BASIC_INFORMATION;

	extern "C" {
		NTSTATUS NTAPI NtGetNextThread(
			_In_ HANDLE ProcessHandle,
			_In_opt_ HANDLE ThreadHandle,
			_In_ ACCESS_MASK DesiredAccess,
			_In_ ULONG HandleAttributes,
			_In_ ULONG Flags,
			_Out_ PHANDLE NewThreadHandle);

		NTSTATUS NTAPI NtQueryInformationThread(
			_In_ HANDLE ThreadHandle,
			_In_ ThreadInfoClass ThreadInformationClass,
			_Out_writes_bytes_(ThreadInformationLength) PVOID ThreadInformation,
			_In_ ULONG ThreadInformationLength,
			_Out_opt_ PULONG ReturnLength);

	}
}

