# Changelog

## [3.2.2] - 2026-05-21

### Added
- Anti-underflow protection for Prefetch date calculations
- Active polling for Explorer process termination (race condition fix)
- EnablePrivilege generic function for privilege elevation

### Changed
- Replaced month-based day calculation with 64-bit FILETIME arithmetic
- Improved error messages with GetLastError() codes
- Enhanced WaitForProcessExit with configurable timeout

### Fixed
- False positive success message in PurgeStandbyList
- Potential underflow when system clock changes
- Race condition in thumbnail cache deletion

## [3.2.0] - 2026-05-21

### Added
- Service optimization (DiagTrack, dmwappushservice, SysMain, WSearch)
- Registry policies (Bing search, telemetry, SpyNet)
- Modular architecture with struct-based configuration

## [3.1.0] - 2024-05-21

### Added
- Kernel Standby List purge via NtSetSystemInformation
- SeProfileSingleProcessPrivilege elevation

## [3.0.0] - 2026-05-21

### Added
- Working Set trimming for 100+ processes
- Browser cache cleaning (Chrome, Edge, Firefox)
- Thumbnail cache reset
- Smart Prefetch cleanup (>30 days)
- DNS cache flush
