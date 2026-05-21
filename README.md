# Z-Optimizer

A lightweight, high-performance system optimization and maintenance tool written in pure C for Windows environments. 

`Z-Optimizer` targets common Windows performance bottlenecks, including aggressive log inflation, bloated file caches, stale prefetch definitions, and mismanaged process working sets. It focuses on surgical cleanup and memory pressure release without background persistence.

## Features

*   **Pre-Execution Diagnostics:** Real-time analysis of physical RAM availability and primary partition storage health.
*   **System Cache Purge:** Cleans deep Windows temporary structures and redundant CBS (`.cab`) logs.
*   **Smart Prefetch Cleanup:** Evaluates the `Prefetch` directory and safely removes configuration files (`.pf`) older than 30 days to optimize the Windows boot-strapping manager.
*   **Browser Cache Eviction:** Target-scoped clearing of volatile cache data for Google Chrome, Microsoft Edge, Mozilla Firefox, and Brave.
*   **Shell Architecture Reset:** Forcefully clears and resets corrupted Windows Explorer thumbnail databases (`thumbcache_*.db`) by systematically cycling the `explorer.exe` shell process.
*   **Working Set Trimming:** Cycles through active non-critical processes to release idle physical pages back to the Windows Memory Manager.

## Prerequisites

*   **Operating System:** Windows Vista / 7 / 10 / 11.
*   **Privileges:** **Administrator privileges are strictly required** to manipulate system-wide paths and process quotas.
*   **Compiler:** GCC (MinGW-w64 or TDM-GCC distribution).

## Compilation

The source code relies on specific Win32 API extensions. To expose modern timing structures (like `GetTickCount64`) and successfully bind system subsystems, compile with the following library flags:

```bash
gcc optimizer.c -o ZOptimizer.exe -lshlwapi -lpsapi -O2 -s

```

### Flag Breakdown:

* `-lshlwapi`: Links the Shell Light-weight Utility API (handling paths and directories).
* `-lpsapi`: Links the Process Status API (handling working sets and process enumeration).
* `-O2`: Enforces standard high-level optimizations for runtime execution speed.
* `-s`: Strips symbolic information and debugging tables, severely reducing binary footprint.

## Usage

1. Open a command prompt (`cmd` or `PowerShell`) as **Administrator**.
2. Run the compiled binary:
```cmd
ZOptimizer.exe

```


3. Review the terminal diagnostic output and wait for the execution stages to complete.
4. For optimal results, restarting the system post-execution is highly recommended to allow the OS to rebuild optimized indexes.

## License

This project is open-source and available under the [GNUv3 License](LICENSE).
