# Core Flight System : Framework : App : Telemetry Output Lab

This repository contains NASA's Telemetry Output Lab (to_lab), which is a framework component of the Core Flight System.

This lab application is a non-flight utility to downlink telemetry from the cFS Bundle. It is intended to be located in the `apps/to_lab` subdirectory of a cFS Mission Tree. The Core Flight System is bundled at <https://github.com/nasa/cFS> (which includes to_lab as a submodule), which includes build and execution instructions.

to_lab is a simple telemetry downlink application that sends CCSDS telecommand packets over a UDP/IP port. The UDP port and IP address are specified in the "Enable Telemetry" command. It does not provide a full CCSDS Telecommand stack implementation.

To send telemtry to the "ground" or UDP/IP port, edit the subscription table in the platform include file: fsw/platform_inc/to_lab_sub_table.h. to_lab will subscribe to the packet IDs that are listed in this table and send the telemetry packets it receives to the UDP/IP port.

## Version History

### Development Build: 2.3.6
- Replace references to `ccsds.h` types with the `cfe_sb.h`-provided type. 
- See <https://github.com/nasa/to_lab/pull/44>

### Development Build: 2.3.5

- Apply code style
- See <https://github.com/nasa/to_lab/pull/43>
 
### Development Build: 2.3.4

- Configure the maximum depth supported by OSAL, rather than a hard coded 64.
- See <https://github.com/nasa/to_lab/pull/39>

### Development Build: 2.3.3

- Apply the CFE_SB_MsgIdToValue() and CFE_SB_ValueToMsgId() routines where compatibility with an integer MsgId is necessary - syslog prints, events, compile-time MID #define values.
- Deprecates shell tlm subscription
- Changes to documentation
- See <https://github.com/nasa/to_lab/pull/38>

### Development Build: 2.3.2

- Use OSAL socket API instead of BSD Sockets

- Use global namespace to isolate variables

- Minor updates (see <https://github.com/nasa/to_lab/pull/27>)

### Development Build: 2.3.1

- Fix for a clean build with OMIT_DEPRECATED
- Minor updates (see <https://github.com/nasa/to_lab/pull/26>)

### _**OFFICIAL RELEASE: 2.3.0**_

- Minor updates (see <https://github.com/nasa/to_lab/pull/13>)

- Not backwards compatible with OSAL 4.2.1

- Released as part of cFE 6.7.0, Apache 2.0

### _**OFFICIAL RELEASE: 2.2.0a**_

- Released as part of cFE 6.6.0a, Apache 2.0

## Known issues

As a lab application, extensive testing is not performed prior to release and only minimal functionality is included.

## Getting Help

For best results, submit issues:questions or issues:help wanted requests at <https://github.com/nasa/cFS>.

Official cFS page: <http://cfs.gsfc.nasa.gov>
