# TaipanMiner

Bitcoin mining firmware for ESP32-S3 boards with optional ASIC support.

## Build

- Framework: ESP-IDF via PlatformIO
- `make help` — show all targets
- `make build` — build all boards (tdongle-s3 + bitaxe-601)
- `make build-<env>` — build specific board (e.g. `make build-bitaxe-601`)
- `make flash-<env>` — flash specific board
- `make test` — host unit tests
- `make check` — static analysis (cppcheck)
- `make coverage` — test + gcovr coverage report
- `make monitor` — serial monitor
- Debug: `make build-tdongle-s3-debug` or `make build-bitaxe-601-debug` (adds `TAIPANMINER_DEBUG=1`)

### Python compatibility

ESP-IDF's pydantic-core dependency requires Python <= 3.13. If your system Python is 3.14+, create the ESP-IDF venv manually with Python 3.13 before building:

```bash
python3.13 -m venv ~/.platformio/penv/.espidf-5.5.3
~/.platformio/penv/.espidf-5.5.3/bin/pip install -r ~/.platformio/packages/framework-espidf/tools/requirements/requirements.core.txt
```

Then create `~/.platformio/penv/.espidf-5.5.3/pio-idf-venv.json` with the correct version info to prevent PlatformIO from overwriting the venv.

## Boards

| Env | Board | ASIC | Console |
|-----|-------|------|---------|
| `tdongle-s3` | LilyGo T-Dongle S3 | none (SW mining) | USB CDC |
| `bitaxe-601` | Bitaxe 601 Gamma | BM1370 | UART0 |

### Adding a new board

1. **Board header** — create `components/board/include/boards/<board>.h` with pin definitions
2. **Board dispatch** — add `#elif defined(BOARD_<NAME>)` to `components/board/include/board.h`
3. **PlatformIO env** — add `[env:<board>]` and `[env:<board>-debug]` to `platformio.ini` with `-DBOARD_<NAME>` (and `-DASIC_<CHIP>` if applicable) in `build_flags`
4. **sdkconfig delta** — create `sdkconfig/<board>` with settings that differ from `sdkconfig.defaults` (e.g. console type, UART ISR); add `sdkconfig/<board>-debug` for debug overlay
5. **Gitignore** — add `sdkconfig.<board>` and `sdkconfig.<board>-debug` to `.gitignore` (ESP-IDF auto-generates these at the project root)
6. **CI/release** — add the env name to the matrix arrays in `ci.yml` and `release.yml`
7. **default_envs** — add the env to `default_envs` in `platformio.ini`

## Project layout

- `src/` — app entry point, version
- `components/` — ESP-IDF components (mining, stratum, board, asic, display, wifi_prov, http_server, led, nv_config)
- `components/board/include/boards/` — per-board pin/peripheral headers
- `sdkconfig/` — hand-authored sdkconfig deltas per board
- `test/test_host/` — host-based unit tests (run without hardware via native env)
- `test/test_device/` — on-device integration tests

## Hardware

- All boards use ESP32-S3 dual-core @ 240MHz
- Pin assignments in `components/board/include/boards/<board>.h`
- Board dispatch via `components/board/include/board.h` (`BOARD_*` defines)

## Architecture

- Core 0: WiFi, Stratum, HTTP server, display, LED
- Core 1: HW SHA mining (priority 20, nonces 0x00000000-0x7FFFFFFF)
- Core 0: SW SHA mining (priority 3, nonces 0x80000000-0xFFFFFFFF, yields to WiFi/Stratum)
- Inter-core: FreeRTOS queues (work_queue, result_queue) + mutex (mining_stats)

### Mining pipeline

- Phase 3 zero-bswap HW-format pipeline: midstate stored in HW-native word order
- `sha256_hw_mine_nonce`: force-inlined, midstate→SHA_H, block2+nonce→SHA_TEXT, SHA_CONTINUE, direct SHA_H→SHA_TEXT copy for pass 2, SHA_START
- SHA_TEXT registers are NOT preserved after SHA operations (verified empirically)
- SHA_START is 21% faster than SHA_CONTINUE+H0 for pass 2
- APB peripheral bus fixed at 80 MHz — HW mining is MMIO-bound, not CPU-bound (~223 kH/s ceiling)
- BIP 320 version rolling when nonce space exhausted
- Yield every 256K nonces (0x3FFFF mask), hashrate log every 1M (0xFFFFF)
- FreeRTOS tick rate: 100 Hz; vTaskDelay uses pdMS_TO_TICKS()
- Combined hashrate logged by HW task (hw + sw + total in kH/s)

### Network

- TCP keepalive (60s idle, 10s interval, 3 probes), TCP_NODELAY
- SO_RCVTIMEO cached to avoid redundant setsockopt calls
- Framework log noise suppressed at init: `esp_log_level_set("wifi", ESP_LOG_WARN)` etc.
- WiFi: infinite retry with 5s backoff, 60s startup timeout with esp_restart()

## Testing

- Host test scope: SHA-256, Stratum parsing, coinbase/merkle, header serialization, target, CRC, PLL, BM1370 framing
- Device test scope: mining integration, NVS persistence, live pool handshake
- Anonymize test data per workspace rules
