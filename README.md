# netconflictscand

OpenWrt feed providing the `netconflictscand` daemon.

`netconflictscand` monitors network configuration changes through Linux netlink events and detects IP subnet conflicts between interfaces. It is intended for devices with multiple network uplinks where different DHCP servers may assign overlapping address ranges.

## Package

### netconflictscand

Daemon for detecting overlapping network subnets.

Features include:

- Netlink-based interface monitoring
- Detection of overlapping IPv4 subnets
- DHCP-assigned network conflict detection
- UBUS integration
- Automatic monitoring of network configuration changes

## Dependencies

### netconflictscand

- libubus
- libubox
- libblobmsg-json

## Installation

Add feed:

```text
src-git netconflictscand https://github.com/a96t27/netconflictscand.git
```

Update and install:

```bash
./scripts/feeds update netconflictscand
./scripts/feeds install -a -p netconflictscand
```

## Configuration (menuconfig)

Run configuration menu:

```bash
make menuconfig
```

Locate package:

```text
Network  --->
    <*> netconflictscand
```

After selecting the package:

- Save configuration
- Exit menuconfig

## Build

Build package:

```bash
make package/netconflictscand/compile V=s
```

Or build full firmware:

```bash
make -j$(nproc)
```

## Installing produced .ipk package

After building, the generated `.ipk` package will be located in:

```text
bin/packages/<arch>/netconflictscand/
```

Example:

```text
bin/packages/aarch64_cortex-a53/netconflictscand/
```

### Find generated package

```bash
find bin/packages -name "netconflictscand*.ipk"
```

## Installing on a running OpenWrt device

Copy package to the router:

```bash
scp bin/packages/*/netconflictscand/netconflictscand*.ipk \
    root@<DEVICE_IP>:/tmp/
```

Install with opkg:

```bash
opkg install /tmp/netconflictscand*.ipk
```

## Service Management

Enable service:

```bash
/etc/init.d/netconflictscand enable
```

Start service:

```bash
/etc/init.d/netconflictscand start
```

Stop service:

```bash
/etc/init.d/netconflictscand stop
```

Restart service:

```bash
/etc/init.d/netconflictscand restart
```

Check status:

```bash
/etc/init.d/netconflictscand status
```

## Example Use Case

A device has multiple interfaces connected to different networks:

```text
eth0 -> DHCP server A -> 192.168.1.0/24
wwan0 -> DHCP server B -> 192.168.1.0/24
```

Both interfaces receive addresses from the same subnet range. This can lead to routing ambiguity and connectivity issues.

`netconflictscand` detects such overlaps when interface configuration changes occur and can expose conflict information through UBUS for use by other services.

## Verify Installation

```bash
opkg list-installed | grep netconflictscand
```
