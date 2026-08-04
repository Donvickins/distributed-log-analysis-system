# Distributed Log Analysis System

A client/server log analysis system built on Boost.Beast that ingests JSON, XML, and text log files over HTTP and returns aggregated log-level statistics.

## Architecture

- **server** — A Boost.Asio/Beast HTTP server that binds to `127.0.0.1`, serves the `./public` directory, accepts multipart form-data uploads (or raw bodies), parses each supported content type, merges the results into `message_stats`, and returns a JSON analysis. Uses a pool of Asio worker threads sized to the host's hardware concurrency.
- **client** — A Boost.Beast HTTP client that reads `./logs/log_file.json`, `./logs/log_file.xml`, and `./logs/log_file.txt`, sends them in a single multipart request, and prints the analysis summary returned by the server.

## Features

- Ingests three log formats: `application/json`, `application/xml`, and `text/plain`
- Accepts both multipart/form-data uploads and single-content-type request bodies
- Aggregates per-log-level message frequency counts across all uploaded files
- Tracks clients via a `Client-Id` header
- Concurrent request handling via Asio worker threads
- Returns a structured JSON response

## Requirements

- Linux
- CMake >= 3.24
- A C++23 compiler (clang recommended)
- Ninja
- git, curl, unzip
- Dependencies are pulled automatically via vcpkg:
  - boost-asio, boost-beast, boost-system, boost-filesystem, boost-json
  - pugixml
  - simdjson (vendored in `helpers/classes/`)

## Install & Build

### One-shot setup

```bash
./linux_setup.sh
```

This installs any missing system packages, clones and bootstraps vcpkg into `~/vcpkg`, installs the manifest dependencies, and configures the `build/` directory.

### Manual build

```bash
cmake --preset=vcpkg
cmake --build build
```

Artifacts are produced in `build/`:

- `build/server`
- `build/client`

## Usage

### Server

```bash
./build/server                 # default port 9000
./build/server -p 8080         # or specify a port (1024-65535)
```

The server creates `./public` if it does not exist. Press Enter or Ctrl+C to stop it.

### Client

```bash
./build/client -id 42                 # connects to default port 9000
./build/client -id 42 -p 8080         # or specify a port
```

`-id` is required and must be a positive integer. The port defaults to 9000. The client reads the three log files from `./logs/`.

## Input formats

Place your log files in `./logs/` with these names and formats:

### JSON — `logs/log_file.json`

```json
[
  { "log_level": "INFO",  "message": "User logged in" },
  { "log_level": "ERROR", "message": "Disk write failed" },
  { "log_level": "INFO",  "message": "User logged in" }
]
```

### XML — `logs/log_file.xml`

```xml
<logs>
  <log>
    <log_level>INFO</log_level>
    <message>User logged in</message>
  </log>
  <log>
    <log_level>ERROR</log_level>
    <message>Disk write failed</message>
  </log>
</logs>
```

### Text — `logs/log_file.txt`

Pipe-delimited lines; field 2 is the log level, field 3 is the message.

```text
2026-08-04 10:00:00|INFO|User logged in|session=abc
2026-08-04 10:00:01|ERROR|Disk write failed|device=/dev/sda
```

## Response format

The server responds with a JSON object:

```json
{
  "status": "success",
  "total_entries": 3,
  "client_ip": "127.0.0.1",
  "client_port": "55321",
  "analysis_type": "LOG LEVEL",
  "message_stats": {
    "INFO":  { "User logged in": 2 },
    "ERROR": { "Disk write failed": 1 }
  },
  "invalid_data": 0
}
```

| Field          | Description                                             |
|----------------|---------------------------------------------------------|
| `status`       | `"success"` on success, otherwise an error message is returned as an HTTP error response |
| `total_entries`| Total number of log entries processed across all files  |
| `client_ip`    | IP address of the requesting client                     |
| `client_port`  | Source port of the requesting client                    |
| `analysis_type`| Type of analysis performed (currently `LOG LEVEL`)      |
| `message_stats`| Nested map of `log_level` → `message` → occurrence count|
| `invalid_data` | Number of entries skipped due to missing/invalid fields  |
