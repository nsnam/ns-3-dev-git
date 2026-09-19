# Donor: astra-network-ns3 (RoCE/RDMA QP + congestion-control concepts)

- Source repo: ``../ndm/standards/ns3-simulators/astra-network-ns3`` (read-only corpus, M2; machine-specific absolute path avoided per AGENTS.md)
- Commit: f764bed27630ee45368c03e22e4f952b69fab30a
- License: GPLv2 (repo-root LICENSE) — compatible with the ns-3 core; project
  re-implementations that *use* these concepts are Apache-2.0 (see LICENSE-PROJECT).
- Extracted 2026-09-19 for Phase 2 (RoCE baseline).

## Files (verbatim, read-only reference — never included from src/)

| File | Upstream path | Why |
|---|---|---|
| rdma-queue-pair.{h,cc} | src/point-to-point/model/ | QP state model (snd_nxt/snd_una, window, baseRtt, per-CC state structs), Rx QP (expected seq, NACK timer, ECN source accounting) |
| rdma-hw.{h,cc} | src/point-to-point/model/ | NIC TX/RX datapath, packet header (BTH-like), CNP generation, retransmission |
| rdma-driver.{h,cc} | src/point-to-point/model/ | QP lifecycle / rate application |
| rdma-client.{h,cc} | src/applications/model/ | Flow-generation app (request/response sizing) |

## Usage rule (D8)

These files are **concept references only**. The ndm-roce module re-implements the needed
surfaces natively (ns-3.42 style: attributes, Ptr ownership, const-correct accessors,
IPv6 instead of the donor's IPv4) and links against ndm-topology. Nothing from this
directory is compiled into the project build. The donor is IPv4/raw-UDP oriented; our RoCE
baseline is RoCEv2-style (IPv6 + UDP 4791) and transport-neutral at the collective-service
boundary (D9).
