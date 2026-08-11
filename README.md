# Distributed Log Analysis System

A client/server log aggregation tool built on Boost.Asio and Boost.Beast. A small HTTP server parses JSON, XML, and pipe-delimited text logs, merges them into a per-log-level message analysis, and returns it as JSON.

## Highlights

- **Concurrent server** — an Asio `io_context` worker pool sized to the host's hardware concurrency
- **Fast JSON** — log parsing uses [simdjson](https://github.com/simdjson/simdjson), with AVX-accelerated parsing on supported CPUs
- **Three formats, one request** — a single multipart POST carries `log_file.json`, `log_file.xml`, and `log_file.txt`
- **Aggregated analysis** — every upload is merged into one `message_stats` map: `log_level → message → count` across all files
- **Per-client persistence** — uploads are stored as timestamped files under `storage/Client#<id>/`, clients tracked via a `Client-Id` header
- **Static hosting** — also serves the `./public` directory over HTTP
- **Cross-platform** — vcpkg manifest dependencies with `linux` (clang) and `windows` (MSVC) CMake presets

## Architecture

```text
./build/client ──(multipart POST: log_file.json/.xml/.txt)──▶ ./build/server
      ▲                                                       (Asio worker pool ─ parse ─ merge)
      └────────────────(JSON: log-level → message → count)───────────────────────┘
```

- **server** — A Boost.Asio/Beast HTTP server that binds `0.0.0.0`, serves `./public`, accepts multipart form-data uploads (or raw single-content-type bodies), parses each supported format, and merges the results into `message_stats`.
- **client** — A Boost.Beast HTTP client that reads `./logs/log_file.json`, `./logs/log_file.xml`, and `./logs/log_file.txt`, sends them in a single multipart request, and prints the analysis summary.

## Requirements

- CMake >= 3.24
- A C++23 compiler
- Ninja
- git, curl, unzip
- [vcpkg](https://github.com/microsoft/vcpkg) — dependencies are pulled automatically from the manifest (`vcpkg.json`):
  - boost-asio, boost-beast, boost-system, boost-json
  - pugixml
  - cli11
  - simdjson

Per-OS prerequisites:

- **Linux (Debian/Ubuntu)**

  ```bash
  sudo apt install git cmake ninja-build clang curl unzip
  ```

- **macOS**

  ```bash
  xcode-select --install       # provides clang/clang++
  brew install cmake ninja curl
  ```

- **Windows**

  Install [Visual Studio 2022](https://visualstudio.microsoft.com/) with the
  "Desktop development with C++" workload, plus CMake and Ninja. Configure and
  build from a Developer PowerShell (started from the VS menu) so the MSVC
  toolchain is on the PATH.

## Install & Build

### 1. Install vcpkg

```bash
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
```

Bootstrap vcpkg. On Windows use `bootstrap-vcpkg.bat` instead of `bootstrap-vcpkg.sh`:

```bash
~/vcpkg/bootstrap-vcpkg.sh
```

Tell CMake where the toolchain lives (add to your shell profile):

```bash
export VCPKG_ROOT="$HOME/vcpkg"
```

### 2. Configure and build

Linux/macOS:

```bash
cmake --preset=linux
cmake --build build
```

Windows (Developer PowerShell):

```powershell
cmake --preset=windows
cmake --build build
```

The first configure step downloads and builds the manifest dependencies into `build/vcpkg_installed/`.

Artifacts are produced in `build/`:

- `build/server` (Windows: `build/server.exe`)
- `build/client` (Windows: `build/client.exe`)

## Usage

### Server

```bash
./build/server                  # default port 9000
./build/server -p 8080          # or specify a port (1024-65535)
```

The server creates `./public` if it does not exist. Type `/quit` and press Enter, or press Ctrl+C, to stop it. On Windows, Ctrl+C and Ctrl+Break are handled through the native console control handler; `kill`/SIGTERM has no equivalent there.

### Client

```bash
./build/client -id 42           # default client port 7654
./build/client -id 42 -p 9000   # or point at the server
./build/client --client-id 42   # long form
```

`-id`/`--client-id` sets the `Client-Id` header. It defaults to the client port 7654 — the server listens on 9000 by default, so pass `-p 9000` when connecting to a default-configured server. The client reads the three log files from `./logs/`.

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