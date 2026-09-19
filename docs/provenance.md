# Provenance manifest (Phase 0)

Exact version pins, licenses, and verified SHAs for everything this repo builds on or draws
from. Rule (AGENTS.md D8): this manifest is the license/provenance source of truth; a file
without an entry here must not enter the repo.

Verification key:
- **verified** — SHA checked out locally in the read-only corpus (`../ndm/`, sibling of this
  repo) and `git rev-parse HEAD` matches on this machine, 2026-09-19.
- **audit-pin** — SHA taken from `reports/recovered-subagent-reports.md`; the local corpus copy
  had not finished transferring on 2026-09-19 and will be re-verified (charter rule: no spec/test
  cycle starts on partially transferred files).

## 1. ns-3 core

| Item | Pin | Status |
|---|---|---|
| Upstream | `nsnam/ns-3-dev` (GitHub fork per Q-001; local repo pinned meanwhile) | — |
| Core commit | `ab4cce021d8f6b2458784704a10af810d3969f0f` (ns-3.42; second parent of `8b4b0f8e2` "Merge tag 'ns-3.42'", present in `astra-network-ns3` history) | audit-pin (in `astra-network-ns3` history, verified transfer) |
| License | **GPLv2** — the root `LICENSE` file is the ns-3 core's license and stays GPLv2 | verified (root LICENSE) |

Core modifications: **none yet**. Any future core change must be a named patch in
`docs/core-patches/` with a reason (AGENTS.md D1).

## 2. Standards (read-only corpus `../ndm/standards/`)

| Item | Pin | License / status |
|---|---|---|
| OCP MRC Specification Rev 1.0 (PDF) | SHA-256 `d59de939f159272c54ec33f924739944431dd5e92a3d3b232bc15315b052fd27` | verified 2026-09-19; OCP Modified OWFa 0.9 (appendices excluded per spec §1) |
| OCP MRC companion API declarations (`OCP-Multipath-Reliable-Connection`) | `662a1509e7e49315ae912099ceec3874a04b1442` (2026-05-19) | root `LICENSE` BSD-2-Clause; **`mrc.h` / `mrc_ctl.h` declare `LicenseRef-NvidiaProprietary` — read-only reference, never copy** (D8) |
| SRv6 RFCs | 8402, 8754, 8986, 9256, 9602 (informational), 9800 — `standards/srv6-rfc/` | IETF RFC text; 9602 is informational and must never be cited as normative |
| UCC | `standards/ucc/ucc` | (for reference only, not a build dependency) |

Missing locally (recorded as missing dependencies, not assumptions): IBTA Vol.1 R1.8,
UltraEthernet 1.01 (NSCC/trimming/DFC), RFC 8200. See `protocol_source_audit` in
`reports/recovered-subagent-reports.md`.

## 3. Donor simulators (corpus `../ndm/standards/ns3-simulators/`)

Scope column = what this project may extract (file-level, into `donors/` with per-file
provenance) per the locked decisions. "Tree import" is never allowed (R7).

| Donor | Pin | Pin status | License (verified where noted) | Reusable scope (per D2–D4, D10) |
|---|---|---|---|---|
| `astra-sim` | `518bd513ae110428cd62eb60efc0f3993fd53c70` | **verified** | MIT (root LICENSE, verified) | D2: AI workload simulator; `AstraNetworkAPI` boundary for the project-owned adapter (Phase 5) |
| `astra-network-ns3` | `f764bed27630ee45368c03e22e4f952b69fab30a` | **verified** | GPLv2 (root LICENSE, verified) | Reference only: closest modern HPCC port on 3.42; QBB/IPv4 pipeline is **not** extended for SRv6 (D10); source of the pristine 3.42 commit |
| `SimAI` | `9bd9f01e561bbbb822c074316e3a392d3ac92aae` | **verified** | Apache-2.0 (root LICENSE, verified); ASTRA subtree MIT | D3 donor: NVSwitch/node types (`.../network_frontend/ns3/common.h:701-739`), NVSwitch-vs-network path selection (`rdma-hw.cc:243-260`), NCCL channel/flow metadata, layer model |
| `astra-sim-stygianet` (STyGIANet) | `b4a9cee0997b5c3cb9b2136b1b3bd6f292b92418` | **verified** | MIT (root LICENSE, verified) | D4 donor: failure-aware path selection; ToR/uplink arithmetic is embedded — extract, don't copy |
| `epic` | `6bf33297ff717ba962f7eabcf3d10ed7dafe5008` | **verified** | GPLv2 (`pkt-sim/LICENSE`, verified) | Concept donor only (R7): EPIC-II/III collective FSMs, duplicate tracking; no tree import |
| `PowerTCP` = `link-xz/ns3-datacenter-powertcp` (corpus dir `PowerTCP/`) | local HEAD `bec7c183abfb017223f6dcc5fb09b22703e11f0f`; audit table pins `ns3-datacenter @ 4dd55d89a46e` | **verified locally; pin mismatch vs audit table** — treat `4dd55d89a46e` as the intended pin and re-verify after transfer completes | No root LICENSE found (ns-3 files carry GPLv2 notices) | Concept donor: PowerTCP/ABM congestion control; resolve licensing before importing any file |
| `hpcc` | `9f4be2a9ead8` (audit table) | audit-pin (local dir empty at 2026-09-19) | GPLv2 (per audit) | D10: DCQCN/HPCC/TIMELY CC reference; `scratch/third.cc` link-failure path is the **anti-pattern** reference (D6) |
| `ns3-PB-FS` | `84674d4acb11` | audit-pin (local dir empty) | GPLv2 (per audit) | D10: IRN selective repeat for lossy; `/1000`, `%1536` constants must be refactored, not copied |
| `ns3-INC` | `08d1a2416028` | audit-pin (local dir empty) | GPLv2 (per audit) | Anti-pattern + concept donor: ATP tag mutation & global DropTail default change must **not** be inherited |
| `ns3-datacenter` | `4dd55d89a46e` | audit-pin (see `PowerTCP` row — corpus dir is named `PowerTCP`) | GPLv2 terms per audit | Modernized QBB/RDMA + buffer management concepts |
| `ns3-rdma` | `e8a27d02bfe5` | audit-pin (local dir empty) | GPLv2 (per audit) | Historical DCQCN/PFC reference only |
| `inc-aggregation` | `600e75a5cda6` | audit-pin (local dir empty) | **no repo-level LICENSE found** (QUIC donor files carry GPLv2) | Resolve license **before** importing any file (R6); concepts only until then |
| `rdma-roce-sim` | `9a144f2610dc` | audit-pin (local dir empty) | **no repo-level LICENSE found** | Topology-generator concepts useful; resolve license before import (R6) |

## 4. Project files

All files added by this project (everything not part of the pristine ns-3.42 tree, `agents/`,
`container/`, `donors/`, `docs/`, `scenarios/`, `src/ndm-*/`, `tests/`, `tools/`, this manifest,
project `README.md` sections, `LICENSE-PROJECT`) are **Apache-2.0** — see `LICENSE-PROJECT`.
Donor snippets extracted into `donors/` keep their **original** license, file by file, with a
per-file `PROVENANCE` note (repo + SHA + path + license).

## 5. License boundary (GPLv2 / Apache-2.0)

- The repo root **is** the GPLv2 ns-3.42 tree; the root `LICENSE` (GPLv2) governs it.
- Project modules are Apache-2.0 and are written as **additive ns-3 modules** (new files,
  `src/ndm-*/`); they link against the core but do not modify core files (unless a named patch
  in `docs/core-patches/` says otherwise).
- This dual structure is the same pattern as any ns-3 fork carrying third-party modules; it is
  **not** a relicensing of ns-3 code (audit rule, D8).
