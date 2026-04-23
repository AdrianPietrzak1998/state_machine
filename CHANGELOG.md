# Changelog

All notable changes to this project will be documented in this file.
---

## [1.0.1] - 2026-04-23

### Changed
- Elapsed time calculation is now fully explicit and strongly typed via `SM_tick_elapsed(SM_TIME_t current_tick, SM_TIME_t reference_tick)`.
- Time delta handling consistently uses `SM_TIME_t`, improving type safety and making overflow-safe tick arithmetic clearer in the implementation.

## [1.0.0] - 2026-01-18

### Added
- Initial public release
- Documentation describing usage and configuration

### Notes
- This is the first stable release of the project.
- APIs and configuration options may evolve in future versions.
