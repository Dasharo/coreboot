Change log for PC Engines fork of coreboot
==========================================

Releases 4.0.x are based on PC Engines 20160304 release.
Releases 4.5.x are based on mainline support submitted in 
[this gerrit ref](https://review.coreboot.org/#/c/14138/).

## [Unreleased]
## [v4.0.4] - 2016-12-20
### Changed
- sgabios output disabled

## [v4.0.3] - 2016-12-14
### Added
- info about coreboot build date in initial sign of life message

### Changed
- update to SeaBIOS v4.0.2

## [v4.0.2] - 2016-12-09
### Changed
- made coreboot config version independent

### Fixed
- config to use correct version of SeaBIOS

## [v4.0.1.1] - 2016-09-12
## [v4.0.1] - 2016-09-12
### Added
- introduced versioning
- obtained `smbios` fields, such as `board serial` and `SKU`
- nct5104d driver backport
- check if `git` exists before calling it
- missing `cbfs_find_string` in `libpayload`
- executable bit to `xcompile`

### Changed
- reduced log level: display mainboard, DRAM and ECC info only
- improved SD card performance
- forced to use SD in 2.0 mode
- git repository in `Makefile`
