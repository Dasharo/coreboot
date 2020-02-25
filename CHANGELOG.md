Change log for PC Engines fork of coreboot
==========================================

Releases 4.0.x are based on PC Engines 20160304 release.

Releases 4.5.x and 4.6.x are based on mainline support submitted in
[this gerrit ref](https://review.coreboot.org/#/c/14138/).

## [Unreleased]
## [v4.0.30] - 2020-02-28
### Fixed
- microSD card boot order on apu5

### Added
- ARM management controller interaction on apu5

## [v4.0.29] - 2019-10-07
### Fixed
- SD 3.0 mode is correctly configurable now

### Added
- [ACPI support for GPIOs](https://github.com/pcengines/apu2-documentation/blob/master/docs/gpios.md)
- SuperIO GPIOs are reset to default state with platform soft reset
- SuperIO GPIOs can be controlled within IO port range

## [v4.0.28] - 2019-09-09
### Fixed
- watchdog is now available on APU3

## [v4.0.27] - 2019-07-05
### Changed
- [updated SeaBIOS to rel-1.12.1.3](https://github.com/pcengines/seabios/blob/apu_support/CHANGELOG.md#rel-11213---2019-07-05)
- [updated sortbootorder to v4.6.15](https://github.com/pcengines/sortbootorder/blob/master/CHANGELOG.md#v4615---2019-07-05)
- disabled IPv6 in iPXE that often caused the dhcp/autoboot command to time out
- updated iPXE revision to the 2019.3

## [v4.0.26] - 2019-06-07
### Changed
- [updated SeaBIOS to rel-1.12.1.2](https://github.com/pcengines/seabios/blob/apu_support/CHANGELOG.md#rel-11212---2019-06-04)
- [updated sortbootorder to v4.6.14](https://github.com/pcengines/sortbootorder/blob/master/CHANGELOG.md#v4614---2019-06-04)
- SD 3.0 mode is now runtime configurable
- watchdog is now runtime configurable
- all USB devices have now a single boot order priority
- ECC presence is reported in SMBIOS tables

## [v4.0.25] - 2019-03-07
### Changed
- sortbootorder updated to v4.6.13 introducing CPU boost runtime configuration
- configured pull-ups on WLAN_DISABLE# pins on the mPCIe connectors which could
  cause issues with certain modems when floating

### Fixed
- [memtest86+ and iPXE revisions for fully reproducible builds](https://github.com/pcengines/coreboot/issues/267)

### Added
- CPU boost runtime configuration in coreboot

## [v4.0.24] - 2019-02-04
### Added
- enabled Core Performance Boost feature

## [v4.0.23] - 2019-01-09
### Fixed
- enabled ECC
- disabled EHCI0 controller on apu2 which caused problems with pfSense booting

### Changed
- updated SeaBIOS to rel-1.12.0.1
- introduced reproducible builds by providing build ID to iPXE

## [v4.0.22] - 2018-12-03
### Changed
- Updated SeaBIOS to rel-1.11.0.7
- Updated sortbootorder to v4.6.12

### Added
- [experimental option for adding microcode update](https://github.com/pcengines/apu2-documentation/blob/master/docs/microcode_patching.md)
- [COM2 redirection runtime configuration](https://github.com/pcengines/apu2-documentation/blob/master/docs/serial_console.md)
- SeaBIOS can be built from arbitrary revision
- SeaBIOS debug level can be set via Kconfig

### Fixed
- generating serial number from MAC address of first NIC

## [v4.0.21] - 2018-11-08
### Changed
- Updated SeaBIOS to rel-1.11.0.6

### Fixed
- Release date format in DMI tables

### Added
- [Console output redirection to COM2 option](https://github.com/pcengines/apu2-documentation/blob/master/docs/serial_console.md)

## [v4.0.20] - 2018-09-28
### Changed
- Updated sortbootorder to v4.6.11

## [v4.0.19] - 2018-09-02
### Changed
- Enabled autoboot for all interfaces in iPXE

## [v4.0.18] - 2018-06-08
### Changed
- Updated SeaBIOS to 1.11.0.5
- Updated sortbootorder to v4.6.9

### Added
- S1 button support for apu5b

## [v4.0.17] - 2018-05-11
### Added
- Infrastructure to include microcode update
- Support for new building system pce-fw-builder

### Fixed
- Memtest86+ is now a part of coreboot build system and does not have to be
built externally

## [v4.0.16] - 2018-04-06
### Added
- APU4 target
- Enable serial console with S1 button feature

### Changed
- Updated SeaBIOS to rel-1.11.0.4

### Fixed
- `make distclean` affects payloads too

## [v4.0.15] - 2018-03-01
### Fixed
- SeaBIOS default repository URL
- coreboot default console loglevel
- SMBIOS part number format

### Changed
- upgraded SeaBIOS to rel-1.11.0.3
- upgraded sortbootorder to v4.6.5
- iPXE is built from master branch

### Added
- network card PCI ID is set properly according to target board
- apu features can be now restored to deafults by pressing `R` in sortbootorder

## [v4.0.14] - 2017-12-22
### Changed
- upgraded SeaBIOS to 1.11.0.2
- removed sgabios
- iPXE is now built from mainline during coreboot compilation

## [v4.0.13] - 2017-09-29
### Changed
- Generating build info header file `build.h` is now handled by shell script
  `util/genbuild_h/genbuild_h.sh`
- Removed duplicated sign of life strings in `mainboard.c` and `romstage.c`
- Moved sign of life strings (except for memory information) from mainboard.c
  to romstage.c. They are printed ~0.3s after power on, instead of over 2s.

### Fixed
- Date format in sign of life string

## [v4.0.12] - 2017-08-30
### Added
- APU5 target

## [v4.0.11] - 2017-07-21
### Added
- Allow to force GPP3 PCIe CLK attached to mPCIe2 slot based on
  [sortbootorder option](https://github.com/pcengines/sortbootorder/blob/master/README.md#settings-description)

### Changed
- updated sortbootorder to v4.5.7

## [v4.0.10] - 2017-06-30
### Added
- added sortbootorder to option menu

### Changed
- updated sortbootorder to v4.5.6

## [v4.0.9] - 2017-05-30
### Changed
- updated sortbootorder to v4.0.6

## [v4.0.8] - 2017-03-31
### Added
- added BIOS version in boot welcome string

### Changed
- updated SeaBIOS to rel-1.10.2.1
- updated sortbootorder to v4.0.5.1
- added option to enable/disable EHCI0 controller. By default disabled in APU2
  and enabled in APU3
- UART C and D are enabled by default
- changed 'PCEngines' to 'PC Engines' in smbios tables and welcome string
- changed 'APU' to 'apu' in smbios tables and welcome string

### Fixed
- fixed serial number in smbios tables for some apu boards (routine backported
  from mainline)

## [v4.0.7.2] - 2017-03-03
### Changed
- (APU3 only) set GPIO33 (SIMSWAP) to output/low by default

## [v4.0.7.1] - 2017-03-02
### Changed
- (APU3 only) set GPIO33 (SIMSWAP) to output/high by default

## [v4.0.7] - 2017-02-28
### Added
- APU3 target with EHCI0 enabled

### Changed
- update to SeaBIOS rel-1.10.0.1
- update sortbootorder to v4.0.4
- disabled EHCI0 in APU2
- fixed RAM size displaying during the boot (for 2GB sku's)

### Fixed
- mPCIe1 working with ASM1061 based sata controllers
- USB drive stability improvements

## [v4.0.6] - 2017-01-12
### Changed
- enable EHCI0 controller

## [v4.0.5] - 2017-01-03
### Changed
- update sortbootorder to v4.0.3 (UART C/D toggling)
- reset J17 GPIO's (`NCT5104D`) during boot to inputs
- update to SeaBIOS rel-1.9.2.3

## [v4.0.4] - 2016-12-20
### Changed
- sgabios output disabled

## [v4.0.3] - 2016-12-14
### Added
- info about coreboot build date in initial sign of life message

### Changed
- update to SeaBIOS rel-1.9.2.2

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

[Unreleased]: https://github.com/pcengines/coreboot/compare/v4.0.30...coreboot-4.0.x
[v4.0.30]: https://github.com/pcengines/coreboot/compare/v4.0.29...v4.0.30
[v4.0.29]: https://github.com/pcengines/coreboot/compare/v4.0.28...v4.0.29
[v4.0.28]: https://github.com/pcengines/coreboot/compare/v4.0.27...v4.0.28
[v4.0.27]: https://github.com/pcengines/coreboot/compare/v4.0.26...v4.0.27
[v4.0.26]: https://github.com/pcengines/coreboot/compare/v4.0.25...v4.0.26
[v4.0.25]: https://github.com/pcengines/coreboot/compare/v4.0.24...v4.0.25
[v4.0.24]: https://github.com/pcengines/coreboot/compare/v4.0.23...v4.0.24
[v4.0.23]: https://github.com/pcengines/coreboot/compare/v4.0.22...v4.0.23
[v4.0.22]: https://github.com/pcengines/coreboot/compare/v4.0.21...v4.0.22
[v4.0.21]: https://github.com/pcengines/coreboot/compare/v4.0.20...v4.0.21
[v4.0.20]: https://github.com/pcengines/coreboot/compare/v4.0.19...v4.0.20
[v4.0.19]: https://github.com/pcengines/coreboot/compare/v4.0.18...v4.0.19
[v4.0.18]: https://github.com/pcengines/coreboot/compare/v4.0.17...v4.0.18
[v4.0.17]: https://github.com/pcengines/coreboot/compare/v4.0.16...v4.0.17
[v4.0.16]: https://github.com/pcengines/coreboot/compare/v4.0.15...v4.0.16
[v4.0.15]: https://github.com/pcengines/coreboot/compare/v4.0.14...v4.0.15
[v4.0.14]: https://github.com/pcengines/coreboot/compare/v4.0.13...v4.0.14
[v4.0.13]: https://github.com/pcengines/coreboot/compare/v4.0.12...v4.0.13
[v4.0.12]: https://github.com/pcengines/coreboot/compare/v4.0.11...v4.0.12
[v4.0.11]: https://github.com/pcengines/coreboot/compare/v4.0.10...v4.0.11
[v4.0.10]: https://github.com/pcengines/coreboot/compare/v4.0.9...v4.0.10
[v4.0.9]: https://github.com/pcengines/coreboot/compare/v4.0.8...v4.0.9
[v4.0.8]: https://github.com/pcengines/coreboot/compare/v4.0.7.2...v4.0.8
[v4.0.7.2]: https://github.com/pcengines/coreboot/compare/v4.0.7.1...v4.0.7.2
[v4.0.7.1]: https://github.com/pcengines/coreboot/compare/v4.0.7...v4.0.7.1
[v4.0.7]: https://github.com/pcengines/coreboot/compare/v4.0.6...v4.0.7
[v4.0.6]: https://github.com/pcengines/coreboot/compare/v4.0.5...v4.0.6
[v4.0.5]: https://github.com/pcengines/coreboot/compare/v4.0.4...v4.0.5
[v4.0.4]: https://github.com/pcengines/coreboot/compare/v4.0.3...v4.0.4
[v4.0.3]: https://github.com/pcengines/coreboot/compare/v4.0.2...v4.0.3
[v4.0.2]: https://github.com/pcengines/coreboot/compare/v4.0.1.1...v4.0.2
[v4.0.1.1]: https://github.com/pcengines/coreboot/compare/v4.0.1....v4.0.1.1
[v4.0.1]: https://github.com/pcengines/coreboot/compare/88a4f96110fbd3f55ee727bd01f53875f1c6c398...v4.0.1
