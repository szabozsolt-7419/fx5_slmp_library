# fx5_client

CLI tool for testing Mitsubishi FX5 PLC communication using the fx5 library.

This tool is part of the fx5 project and is intended for debugging, testing, and development.

---

## Overview

`fx5_client` is a simple interactive command-line tool that allows you to:

- connect to a Mitsubishi FX5 PLC over TCP
- send SLMP (MC Protocol) requests
- inspect raw request/response frames
- quickly test read/write operations

---

## Build

From project root:

```
mkdir build
cd build
cmake ..
cmake --build .
```

---

## Usage

```
fx5_client <host> <port> [options]
```

Example:

```
fx5_client 192.168.0.10 5000 --trace
```

---

## Interactive Mode

After startup:

```
fx5>
```

Type:

```
help
```

---

## Commands

### Read

```
get D100
get D100-D110
get M51-M60
```

### Write

```
set D100=55
set D88-D92=33,34,35,36,37
set M51=true
set M51-M60=true
set M51-M56=true,false,true,false,true,true
```

### Runtime

```
trace on
trace off

header 3e
header 4e

quit
exit
```

---

## Configuration (INI)

Default file:

```
fx5_client.ini
```

Example:

```
# Connection
host=127.0.0.1
port=5000

# Protocol
header=4e

# Debug
trace=false

# Network settings
network_no=0
pc_no=255
module_io_no=1023
module_station_no=0
monitoring_timer=16

# Transport
recv_timeout_ms=1000
```

### Priority Order

1. built-in defaults  
2. INI file  
3. CLI arguments  
4. runtime commands  

---

## Trace Output

When enabled:

```
TX: 54 00 ...
RX: D4 00 ...
```

---

## Notes

- TCP only (no UDP)
- no reconnect logic (yet)
- intended for development/debugging use
