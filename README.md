# Amplitude 2.0

[![CI](https://github.com/Rise-zen/amplitude/actions/workflows/ci.yml/badge.svg)](https://github.com/Rise-zen/amplitude/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

> One small C++ daemon that collects system metrics — CPU, memory, network
> throughput, battery — by reading `/proc` and `/sys` directly, and writes a
> single JSON file. Built to replace a pile of polling shell scripts
> (`grep`/`jq`/`inotifywait` forks) behind a status bar with one lightweight
> process that idles at ~0% CPU and ~4 MB RAM.

```sh
amplitude --interval 1000 --out /tmp/amplitude.json
```

Then read it from anything — a Quickshell bar via `FileView`, waybar via a
custom module, a script, whatever:

```json
{"cpu":12.0,"memUsed":6235808,"memTotal":16290964,"mem":38.3,
 "netRx":10002,"netTx":9186,"batteryPresent":false,"battery":0,
 "batteryStatus":"Unknown"}
```

## Why

A typical ricing setup runs a separate watcher per metric, each forking
`cat`/`grep`/`jq` on a timer or `inotifywait` loop. That is dozens of
short-lived processes per second. amplitude does the same work as plain file
reads inside one process — no forks, deterministic CPU/net rates from
sampled deltas, atomic writes so a reader never sees a half-written file.

## Fields

| Field | Meaning |
|---|---|
| `cpu` | busy %, 0..100, since the previous sample |
| `mem` / `memUsed` / `memTotal` | memory %, used KiB, total KiB |
| `netRx` / `netTx` | bytes/sec across all non-loopback interfaces |
| `batteryPresent` | whether a `BAT*` supply exists |
| `battery` / `batteryStatus` | capacity 0..100, charge status |

## Install

```sh
./install.sh            # build + install to ~/.local/bin
./install.sh --system   # build + install to /usr/local/bin (sudo)
```

Or build manually:

```sh
meson setup build
meson compile -C build
install -Dm755 build/amplitude ~/.local/bin/amplitude
```

## CLI

```
amplitude [--out PATH] [--interval MS] [--once]
  --out PATH      output file (default /tmp/amplitude.json)
  --interval MS   poll interval, min 100 (default 1000)
  --once          write one sample and exit
```

## Quickshell hookup

```qml
import Quickshell.Io
FileView {
    path: "/tmp/amplitude.json"
    watchChanges: true
    onFileChanged: reload()
    property var data: ({})
    onLoaded: data = JSON.parse(text())
}
```

`data.cpu`, `data.mem`, `data.netRx` … repaint the bar the instant the file
updates — no polling on the QML side either.

## License

MIT — see [LICENSE](LICENSE).
