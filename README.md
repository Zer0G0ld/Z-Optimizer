# Z-Optimizer 🚀

**Clean, optimize, and accelerate your Windows system with surgical precision**

[![License: GPLv3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Win32-blue)](https://github.com)
[![Build](https://img.shields.io/badge/build-passing-brightgreen)]()
[![Size](https://img.shields.io/badge/size-~50KB-blue)]()

A **lightweight, high-performance system optimization tool** written in pure C for Windows environments (Vista through 11). No bloatware, no background services, no registry "cleaners" that break your system — just surgical optimization where it actually matters.

## 📊 Real-World Results

```diff
+ RAM Available: 0.35 GB (4.4%) → 1.31 GB (16.9%)
+ Processes Optimized: 116 trimmed
+ Services Reconfigured: 4 (telemetry disabled)
+ Privacy Policies: 3 applied
+ Binary Size: ~50 KB (smaller than a single image!)
```

## 🎯 Why Z-Optimizer?

Most "optimizers" are snake oil — they add RAM, show flashy animations, or just delete temp files. **Z-Optimizer is different:**

| Problem | Solution |
|---------|----------|
| **Standby List bloat** | Kernel-level purge via `NtSetSystemInformation` |
| **Process memory hoarding** | Working Set trimming on 100+ processes |
| **Windows telemetry** | Service and registry-level disable |
| **Stale prefetch files** | Intelligent cleanup (>30 days only) |
| **Corrupted thumbnails** | Safe explorer.exe restart + cache purge |
| **Bing integration** | Registry policy applied (faster Start Menu) |

## 🛠️ Features

### Core Optimization
- ✅ **Kernel Standby List Purge** - Releases cached file pages held "just in case"
- ✅ **Working Set Trimming** - Cycles through active processes to reclaim idle physical RAM
- ✅ **Smart Prefetch Cleanup** - Removes `.pf` files older than 30 days (preserves recent optimizations)
- ✅ **Windows Cache Purge** - Cleans `SoftwareDistribution`, `Temp`, and CBS `.cab` logs

### Privacy & Telemetry
- 🔒 **Disable DiagTrack** - Kills the Connected User Experiences telemetry service
- 🔒 **Disable dmwappushservice** - Blocks WAP push message routing
- 🔒 **Remove Bing Search** - Prevents Start Menu from making HTTP requests
- 🔒 **SpyNet Disabled** - Stops automatic sample submission to Microsoft

### System Maintenance
- 🧹 **Browser Cache Eviction** - Chrome, Edge, Firefox, Brave
- 🖼️ **Thumbnail Cache Reset** - Corrupted database rebuild via safe Explorer restart
- 🌐 **DNS Flush** - Clears stale network resolution cache
- 💾 **Disk Diagnostics** - Real-time RAM and storage analysis

## 📋 System Requirements

| Requirement | Details |
|-------------|---------|
| **OS** | Windows Vista / 7 / 8 / 10 / 11 (32/64-bit) |
| **Privileges** | **Administrator** (required for system paths and process quotas) |
| **RAM** | Any (optimizes regardless of capacity) |
| **Disk** | Any (but <10GB free triggers warnings) |
| **Dependencies** | None (statically linked Win32 APIs) |

## 🔧 Compilation

### Prerequisites
- GCC (MinGW-w64 or TDM-GCC distribution)
- Windows SDK headers (included with MinGW)

### Build Command
```bash
gcc optimizer.c -o ZOptimizer.exe -lshlwapi -lpsapi -O2 -s
```

### Flag Reference
| Flag | Purpose |
|------|---------|
| `-lshlwapi` | Shell Light-weight Utility API (path manipulation) |
| `-lpsapi` | Process Status API (working set enumeration) |
| `-O2` | High-level optimization for execution speed |
| `-s` | Strip debug symbols (~50KB final binary) |

### Quick Build (Cross-Platform)
```bash
# Windows (MinGW)
gcc optimizer.c -o ZOptimizer.exe -lshlwapi -lpsapi -O2 -s

# Windows (MSVC) via Developer Command Prompt
cl optimizer.c /Fe:ZOptimizer.exe /O2 /link shlwapi.lib psapi.lib

# Linux → Windows (cross-compile)
x86_64-w64-mingw32-gcc optimizer.c -o ZOptimizer.exe -lshlwapi -lpsapi -O2 -s
```

## 🚀 Usage

### Quick Start
1. **Right-click** on `ZOptimizer.exe`
2. Select **"Run as administrator"** (strictly required)
3. Review pre-optimization diagnostics
4. Wait 5-10 seconds for completion
5. **Restart your PC** for full effect

### Command Line
```cmd
# Basic execution (requires admin prompt)
ZOptimizer.exe

# Or via elevated command prompt
runas /user:Administrator "ZOptimizer.exe"
```

### Example Output
```
╔══════════════════════════════════════════════════════════╗
║    Z-OPTIMIZER v3.2.2 - HARDENED EDITION                 ║
║    Kernel Purge + Services + Registry + Anti-Underflow   ║
╚══════════════════════════════════════════════════════════╝

DIAGNOSTICS:
  RAM Total:      7.78 GB
  RAM Available:  0.35 GB (4.4%)
  Disk C:\:       8.1 GB free / 232.2 GB

OPTIMIZING:
  ✅ 116 processes trimmed
  ✅ Telemetry disabled
  ✅ Standby List purged
  ✅ DNS cache flushed

RESULT:
  RAM Available:  1.31 GB (16.9%) → +274%!
  
✅ Optimization complete in 7 seconds
```

## 🧠 Architecture Overview

Z-Optimizer uses **native Win32 API calls** — no PowerShell, no WMI, no external dependencies:

```c
// Kernel-level memory purge
NtSetSystemInformation(SystemMemoryListInformation, ...);

// Process working set trimming  
EmptyWorkingSet(hProcess);

// Service Control Manager
OpenSCManager() → ChangeServiceConfig() → ControlService()

// Registry policies
RegCreateKeyEx() → RegSetValueEx()
```

### What Makes It Different

| Approach | Z-Optimizer | Others |
|----------|-------------|--------|
| **Standby List** | ✅ Kernel syscall | ❌ Can't touch |
| **Working Set** | ✅ Iterates all processes | ❌ Self only |
| **Telemetry** | ✅ Service + Registry | ❌ Registry only |
| **Binary Size** | ~50 KB | 5-50 MB |
| **Background Service** | None (run and exit) | Often resident |
| **Dependencies** | None | .NET / PowerShell |

## 📊 Performance Impact

| Metric | Before | After | Improvement |
|--------|--------|-------|-------------|
| RAM Available | 0.35 GB | 1.31 GB | **+274%** |
| Processes Optimized | - | 116 | Working set trimmed |
| Telemetry Services | 2 running | 0 running | Complete disabled |
| Start Menu Speed | Slower (Bing) | Faster | HTTP requests blocked |
| Explorer Stability | Corrupt thumbnails | Clean cache | Reset on next boot |

## 🛡️ Safety & Design Philosophy

**Z-Optimizer will NEVER:**
- ❌ Delete user documents, photos, or personal files
- ❌ Modify critical system registry hives without validation
- ❌ Install background services or autorun entries
- ❌ Send telemetry or usage data anywhere
- ❌ Require internet connection
- ❌ Touch browser passwords, history, or cookies

**What it does safely:**
- ✅ Removes only temporary/cache files (verified paths)
- ✅ Preserves recent prefetch entries (last 30 days)
- ✅ Waits for Explorer process exit before deleting thumbnails
- ✅ Validates privilege escalation before system calls
- ✅ Uses anti-underflow protection for time calculations

## 🔬 Technical Deep Dive

### Kernel Standby List Purge
Uses the undocumented `NtSetSystemInformation` syscall with `SystemMemoryListInformation` (class 80). Falls back through multiple command values for cross-version compatibility (Win7 → Win11).

### Race Condition Protection
Implements active polling via `CreateToolhelp32Snapshot` to verify Explorer process termination before cache deletion — prevents "file in use" errors.

### Smart Prefetch Cleanup
Converts FILETIME to 64-bit arithmetic with underflow protection. Deletes only files >30 days old using precise day calculation (not month approximation).

## 📝 Version History

| Version | Key Features |
|---------|--------------|
| **v3.2.2** | Anti-underflow protection, hardened error handling |
| **v3.2.0** | Service management + Registry policies |
| **v3.1.0** | Standby List kernel purge |
| **v3.0.0** | Working Set trimming + cache cleanup |

## 🤝 Contributing

Contributions are welcome! Areas for improvement:

- Replace `system()` calls with native APIs (`DeleteFile`, `TerminateProcess`)
- Add power scheme optimization (`PowerSetActiveScheme`)
- Implement SSD TRIM via `DeviceIoControl`
- Add network stack reset via `WSA` APIs

### Development Setup
```bash
# Clone
git clone https://github.com/Zer0G0ld/Z-Optimizer.git
cd z-optimizer

# Build
gcc optimizer.c -o ZOptimizer.exe -lshlwapi -lpsapi -O2 -s

# Test (requires admin)
./ZOptimizer.exe
```

## 📄 License

**GNU General Public License v3.0**

This program is free software: you can redistribute it and/or modify it under the terms of the GPLv3. It is distributed WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

## 🙏 Acknowledgments

- Sysinternals RAMMap (inspiration for Standby List purge)
- Windows Internals (M. Russinovich) for kernel architecture insights
- MinGW-w64 team for Windows C compilation toolchain

## 📧 Contact & Support

- **Issues**: [GitHub Issues](https://github.com/Zer0G0ld/z-optimizer/issues)
- **Discussions**: [GitHub Discussions](https://github.com/Zer0G0ld/z-optimizer/discussions)

---

**⚡ 50KB. 7 Seconds. 274% More RAM. Zero Snake Oil.**
