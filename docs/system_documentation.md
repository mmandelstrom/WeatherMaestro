# WeatherMaestro System Documentation

This document describes the current WeatherMaestro architecture, build/run flow, and the state of implemented APIs. It supersedes earlier drafts and reflects the code in this repository.

---

## System Overview

WeatherMaestro is a C-based weather API server built as a set of cooperating state machines driven by a cooperative scheduler:

- **Weather Server** – Top-level orchestrator that owns the HTTP server and instantiates per-request weather instances.
- **HTTP Server** – Wraps a TCP listener, manages accepted sockets, and spawns `HTTP_Server_Connection` state machines.
- **TCP Server** – Non-blocking accept loop that hands off connected file descriptors.
- **HTTP Connection** – Parses HTTP requests, coordinates with Weather instances, and sends responses.
- **Weather Server Instance** – Per-request worker that maps HTTP requests to weather/geo logic and builds responses.
- **Scheduler** – Global cooperative loop that ticks all tasks.

Supporting modules:
- **HTTP Parser** – Request line/header parsing helpers.
- **TCP Client / HTTP Client** – Simple non-blocking clients used for outbound calls (Open-Meteo, geo lookups).
- **Weather API** – Dispatch layer that maps paths to handlers (weather, forecast, geo search).
- **Geo Parser** – Geo lookup + caching, backed by Nominatim and BigDataCloud.
- **Weather Parser** – Weather lookup + caching, backed by Open-Meteo.
- **Utilities/Libraries** – JSON (cJSON), HTTP status codes, MD5 hashing, file/time helpers, linked lists.

Entry point: `server/src/main.c` initializes scheduler + Weather server, installs SIGINT handler, and runs `scheduler_work(SystemMonotonicMS())` until shutdown.

---

## Build and Run

- Root make targets: `make server` (builds `server_app`), `make server/run`, `make server/clean`.
- Server Makefile (in `server/`) builds all C sources under `server/src`, `core/modules/src`, `core/utils/src`, plus selected libs. Link dependency: `-lcurl`.
- Binary output: `server/server_app`.
- Sample run: `make server/run` or `cd server && ./server_app`.
- Fuzzing targets: `make server/fuzz` (AFL) and `make server/fuzz-asan`.

---

## High-Level Request Flow

```text
TCP_SERVER_LISTENING
  └─ accept(fd) →
HTTP_SERVER_CONNECTING → HTTP_Server_Connection created
  └─ WEATHER_SERVER_CONNECTING → Weather_Server_Instance created
HTTP_CONNECTION parses request → Weather instance handles endpoint
Weather instance builds JSON + HTTP response
HTTP_CONNECTION sends response → disposes connection
Weather instance notified → instance disposed
```

---

## Component Details

### Scheduler
- Functions: `scheduler_init`, `scheduler_create_task`, `scheduler_work`, `scheduler_destroy_task`, `scheduler_dispose`.
- Drives every state machine via callbacks (e.g., `*_taskwork`).

### TCP Server (`server/src/tcp/tcp_server.c`)
- Non-blocking listener; hands accepted fds to a callback (`tcp_server_on_accept`).
- States: `INIT → LISTENING → CONNECTING → CONNECTED → LISTENING`, with `ERROR` and `DISPOSING` paths.
- Key functions: `tcp_server_init(_ptr)`, `tcp_server_accept`, `tcp_server_taskwork`, `tcp_server_dispose(_ptr)`; helpers for non-blocking setup and handover.

### HTTP Server (`server/src/http/http_server.c`)
- Owns the TCP server and dispatches accepted fds to new `HTTP_Server_Connection` objects.
- States: `INIT`, `IDLE`, `CONNECTING`, `CONNECTED`, `ERROR` (with retry), `DISPOSING`.
- Error enum tracks accept/TCP init failures. Retry path attempts TCP re-init before disposal.

### HTTP Connection (`server/src/http/http_connection.c`)
- Per-connection state machine:
  - `INITIALIZING`
  - `READING_FIRSTLINE`
  - `READING_HEADERS`
  - `READING_BODY`
  - `VALIDATING`
  - `WEATHER_HANDOVER`
  - `RESPONDING`
  - `DISPOSING` / `ERROR`
- Uses `http_parser_*` helpers for request parsing; builds `HTTP_Response` and writes back over the socket.

### Weather Server (`server/src/weather/weather_server.c`)
- Owns the HTTP server and receives connection callbacks.
- States: `INIT`, `IDLE`, `CONNECTING`, `CONNECTED`, `ERROR`, `DISPOSING`.
- On connection handover it creates a `Weather_Server_Instance` and tracks its lifecycle.

### Weather Server Instance (`server/src/weather/weather_instance.c`)
- Per-request worker:
  - `INITIALIZING`
  - `REQUEST_PARSING` (delegates to Weather API)
  - `RESPONSE_BUILDING`
  - `RESPONSE_SENDING`
  - `DISPOSING` / `ERROR`
- Holds pointers to the HTTP connection, parsed request/response, and Weather/Geo structures.

### Weather API (`server/src/api/weather_api.c`)
- Dispatch map (under `/api` root) currently includes:
  - `/weather` (`GET`) → implemented
  - `/forecast` (`GET`) → stubbed 404
  - `/geo/list` (`GET`) → stubbed 404
  - `/geo` (`GET`) → implemented (city/coord lookup)
  - `/cities/add` (`POST`) → stubbed 404 (intended for admin use)
  - `/cities/remove` (`DELETE`) → stubbed 404 (intended for admin use)
- `weather_api_handle_endpoint` selects handler by path; defaults to 404 for unknown endpoints.
- Implemented handlers:
  - `ENDPOINT_WEATHER_GET`: expects `lat/latitude` and `lon/longitude` query params; fetches weather via `weather_parser_get_weather_by_coords`; returns 400 if params missing, 500 on internal errors.
  - `ENDPOINT_GEO_GET`: expects `q`/`city` and optional `count`; uses `geo_parser_get_geo_by_query` to return up to `count` results (default 3), with caching.

### Geo Parser (`server/src/api/geo_parser.c`)
- Provides location lookups and caching.
- Interfaces:
  - `geo_parser_init_ptr(Geos**, count, weather, forecast)` allocates `Geos` and nested `Geo` entries.
  - `geo_parser_get_geo_by_query` uses query string; caches in `data/cache/` using MD5-hashed filenames.
  - `geo_parser_get_geo_by_coords` (partial) uses BigDataCloud by default; also caches by lat/lon hash.
  - `geo_parser_lat_lon` helper validates coordinate strings.
- External providers:
  - **Nominatim (OSM)** via `nominatim_get_geo_by_query` for search.
  - **BigDataCloud** via `bigdatacloud_get_geo_by_coords` for reverse lookup.
- Disposal: `geo_parser_dispose_ptr` frees nested allocations.

### Weather Parser (`server/src/api/weather_parser.c`)
- Interfaces:
  - `weather_parser_init_ptr(Weather**, Forecast**)`
  - `weather_parser_get_weather_by_coords` (Open-Meteo), `weather_parser_get_forecast_by_coords` (stub)
  - `weather_parser_dispose_ptr`
- Uses caching under `data/cache/` keyed by MD5 hash of lat/lon.
- Converts `Meteo_Weather` data into internal `Weather`/`Forecast` structs and JSON payloads.

### Meteo API (`server/src/api/meteo.c`)
- Thin wrapper around Open-Meteo:
  - Builds URL with current-weather fields (temperature, humidity, precipitation, wind, WMO code, etc.).
  - Uses `http_client` + curl to fetch JSON.
  - Parses JSON into `Meteo_Weather`; returns units, timezone info, and coordinates.

### HTTP Client (`core/modules/src/http_client.c`)
- Non-blocking HTTP client built atop `tcp_client`.
- States: `INITIALIZING`, `CONNECTING`, `BUILD_REQUEST`, `SEND_REQUEST`, `READ_FIRSTLINE`, `READ_HEADERS`, `READ_BODY`, `RETURNING`, `DISPOSING`, `ERROR`.
- Responsible for outbound calls to external APIs (Open-Meteo, geo providers).

### Utilities and Libraries
- `core/utils`: file IO (`file_utils`), JSON helpers (`json_utils`), time helpers (`time_utils`), misc parsing (`misc_utils`), HTTP parsing (`http_parser`).
- `core/modules`: `tcp_client`, `linked_list`, `curl` wrapper, HTTP client.
- `libs`: bundled cJSON, MD5 hashing, HTTP status codes, URL parsing (`yuarel`).

### Clients
- **C client (client/):** WIP placeholder with TCP enums.
- **C++ client (client_cpp/):** Example console client with `WeatherClient`, `City`, and `Cities` abstractions (see `client_cpp/src/*.cpp`). Demonstrates fetching weather and coordinates but not wired to the production HTTP server.

---

## HTTP Interface (current behavior)

- Base path: `/api`.
- Implemented endpoints:
  - `GET /api/weather?lat={float}&lon={float}` (aliases `latitude`/`longitude`)
    - Success: `200` + weather JSON (current conditions from Open-Meteo).
    - Missing/invalid coords: `400`.
    - Upstream/internal failures: `500`.
  - `GET /api/geo?q={query}&count={n}` (aliases `city`)
    - Success: `200` + geo JSON list (via Nominatim + cache).
    - Missing query: `400`.
    - Upstream/internal failures: `500`.
- Stubbed endpoints (return `404`): `/api/forecast`, `/api/geo/list`, `/api/cities/add`, `/api/cities/remove`.

---

## Caching

- Cache directory: `data/cache/`.
- Filenames are MD5-hashed strings derived from query or `lat/lon` templates to avoid collisions.
- Geo cache: keyed by query string (`CACHE_FILENAME_GEO_QUERY`) or lat/lon (`CACHE_FILENAME_GEO_COORDS`).
- Weather cache: keyed by lat/lon; stores serialized JSON from parsers.

---

## Error Handling and Cleanup

- Every state machine has `ERROR` and `DISPOSING` states; disposal frees owned memory and destroys scheduler tasks.
- HTTP connection errors yield 500 responses when possible before disposal.
- External API failures bubble up as `500` and skip caching writes.

---

## Development Notes

- Coding style: see `docs/Contribution.md` (2-space tabs, pointer asterisk on type side, enums in PascalCase with ALLCAPS members).
- Build flags: `-std=c99 -Wall -Wextra -Wfatal-errors` plus debug symbols; link with `-lcurl`.
- Signals: SIGINT triggers graceful shutdown via `handle_sigint` in `server/src/main.c`.
- Future work (in code as stubs): forecast endpoint, geo list endpoint, city add/remove, HTTPS/TLS support in HTTP client, robust cache eviction.


