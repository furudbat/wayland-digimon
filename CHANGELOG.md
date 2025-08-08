# Changelog

All notable changes to this project will be documented in this file.

## [1.2.4] - 2025-08-08

### Added
- **Multi-Monitor Support** - Choose which monitor to display bongocat on using the `monitor` configuration option
- **Monitor Detection** - Automatic detection of available monitors with fallback to first monitor if specified monitor not found
- **XDG Output Protocol** - Proper Wayland protocol implementation for monitor identification

### Fixed
- **Memory Leaks** - Fixed memory leak in monitor configuration cleanup
- **Process Cleanup** - Resolved child process cleanup warnings during shutdown
- **Segmentation Fault** - Fixed crash during application exit related to Wayland resource cleanup

### Improved
- **Error Handling** - Better error messages when specified monitor is not found
- **Resource Management** - Improved cleanup order for Wayland resources
- **Logging** - Enhanced debug logging for monitor detection and selection

## [1.2.3] - 2025-08-02

### Added
- **Smart Fullscreen Detection** - Automatically hides overlay during fullscreen applications for a cleaner experience
- **Enhanced Artwork** - Custom-drawn bongocat image files by [@Shreyabardia](https://github.com/Shreyabardia)
- **Modular Architecture** - Reorganized codebase into logical modules for better maintainability

### Improved
- **Signal Handling** - Fixed duplicate log messages during shutdown
- **Code Organization** - Separated concerns into core, graphics, platform, config, and utils modules
- **Build System** - Updated to support new modular structure

## [1.2.2] - Previous Release

### Added
- Automatic screen detection for all sizes and orientations
- Enhanced performance optimizations

## [1.2.1] - Previous Release

### Added
- Configuration hot-reload system
- Dynamic device detection

## [1.2.0] - Previous Release

### Added
- Hot-reload configuration support
- Dynamic Bluetooth/USB keyboard detection
- Performance optimizations with adaptive monitoring
- Batch processing for improved efficiency

## [1.1.x] - Previous Releases

### Added
- Multi-device support
- Embedded assets
- Cross-platform compatibility (x86_64 and ARM64)
- Basic configuration
