# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

CPU Stress is a Windows GUI tool that loads CPUs by spawning worker threads whose activity level (None/Low/Medium/Busy/Maximum) is adjustable at runtime. It also exposes per-thread/per-process tuning of priority, affinity, ideal CPU, and CPU sets, plus system information dialogs. Native C++ on the ATL/WTL framework.

## Build & run

- Open `CPUStress.sln` in Visual Studio 2022 (the project targets platform toolset **v145**), or build from a Developer prompt:
  - `msbuild CPUStress.sln /p:Configuration=Release /p:Platform=x64`
- Platforms: `x64` and `Win32` (x86). Configurations: `Debug`, `Release`, and `ReleaseSigned` (Release plus Authenticode signing — needs a signing cert, so use plain `Release` for normal work).
- **Submodule required:** `WTLHelper` is a git submodule referenced as a `ProjectReference` (`..\WTLHelper\WTLHelper\WTLHelper.vcxproj`) and the build fails without it. After cloning run `git submodule update --init --recursive`.
- **NuGet packages** (restored into `packages/`, committed here): WTL `10.0.9163` (the WTL/ATL UI framework) and Microsoft.Windows.ImplementationLibrary / WIL `1.0.190716.2` (used for `wil::unique_handle` RAII wrappers).
- There is no test suite, linter, or CI — verification is manual by running the app.
- **Exceptions are disabled** (`_HAS_EXCEPTIONS 0` in `pch.h`). Do not introduce code that throws or relies on `try`/`catch`; use return codes.

## Architecture

The app is single-process: the worker threads it stresses run *inside* CPUStress.exe itself.

- **`Thread` (`Thread.cpp/.h`)** — the core model object, an owned/RAII wrapper over a Windows thread.
  - User-created threads run `DoWork()`, an infinite loop that busy-spins for a fraction of each ~100 ms window proportional to `ActivityLevel` (Maximum = never sleeps). It also self-measures CPU consumption via `GetThreadTimes`. Activity level is an `atomic` so the UI thread can change load live.
  - Thread metadata accessors (affinity, priority, ideal CPU, stack limits, TEB, CPU sets) wrap Win32 calls; the ones with no public Win32 form go through native NT APIs declared in **`NtDll.h`** (`NtQueryInformationThread`, `THREAD_BASIC_INFORMATION`, etc., under namespace `NT`).
  - `Thread::GetCPUCount()` uses `ALL_PROCESSOR_GROUPS`; CPU indices are encoded as `group * 64 + number`, which matters for any affinity/CPU-set math.
- **`ThreadManager` (`ThreadManager.cpp/.h`)** — owns two `vector<shared_ptr<Thread>>`: all threads in the process (`EnumThreads`) and only the user-created stress threads. `AddNewThread` / `RemoveThread` manage the user set.
- **`CView` (`View.cpp/.h`)** — the main UI: a virtual (owner-data) list view subclass that renders one row per thread and refreshes on a `WM_TIMER`. Owns the `ThreadManager`. All thread/process actions (set activity, priority, affinity, create/kill threads, selection commands) are command handlers in its `ALT_MSG_MAP(2)`, which `CMainFrame` chains into via `CHAIN_MSG_MAP_ALT_MEMBER(m_view, 2)`. `CCustomDraw` colors rows by activity level. `m_ShowAllThreads` toggles between all process threads and only user threads.
- **`CMainFrame` (`MainFrm.cpp/.h`)** — frame window: command bar, toolbar, multi-pane status bar, and menu-level commands (CPU sets dialogs, system info, always-on-top, launch another instance). Implements **`IMainFrame`** (`IMainFrame.h`), a tiny interface the view uses to call back into the frame for context menus and status text without a hard dependency.
- **Dialogs** — `AffinityDlg`, `CPUSetsDlg`, `SysInfoDlg`, `AboutDlg`: standalone WTL dialogs for the corresponding features.
- **`VirtualListView.h`** — a reusable WTL mixin (`CVirtualListView<T>`) providing sorting (`SortInfo`/`DoSort`) and virtual-list plumbing; `CView` derives from it. Similar reusable WTL helpers come from the `WTLHelper` submodule.

## Conventions

- WTL/ATL message maps drive all event handling. New UI actions = a resource ID in `resource.h` / `CPUStress.rc`, plus a `COMMAND_ID_HANDLER` (or range handler) in the relevant `BEGIN_MSG_MAP`/`ALT_MSG_MAP` and its `On...` method.
- Threads are shared via `std::shared_ptr<Thread>`; `Thread` is non-copyable in spirit (holds `wil::unique_handle`s). Pass shared pointers, not raw `Thread*`, when ownership crosses the manager boundary.
- `pch.h` is the precompiled header and sets `WINVER`/`_WIN32_WINNT` and pulls in ATL/WTL/WIL — add common includes there.
