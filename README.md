# ndm-sys

Simulation testbed for the OSDI extension **"the network as a recoverable dataflow machine"**:
SRv6-driven in-network computing (INC), recoverable collectives, and framework-level failure
recovery, built on a pinned **ns-3.42** core with **ASTRA-sim** (workload/collectives) and
**SimAI** (NVSwitch donor for evaluation).

## Start here

| File | What it is |
|---|---|
| [`AGENTS.md`](AGENTS.md) | Binding instructions for any coding agent (Codex, Pi) or human working in this repo |
| [`PLAN.md`](PLAN.md) | End-to-end phased build plan with explicit agent invocations and exit gates |
| [`ASK.md`](ASK.md) | Unattended-operation question channel (agent ↔ user) |
| [`agents/`](agents/) | Agent charters (testers, RFC/spec verifiers, integration reviewers) |
| [`.pi/subagents/`](.pi/subagents/) | Pi project-scoped subagent definitions (pointers into `agents/`) |
| [`container/`](container/) | Docker image + provisioning for the experiment container on M3 (wt-1-23) |
| [`tools/remote/`](tools/remote/) | Hop helpers: run commands on M2 (login15) or inside the M3 container from M1 |
| [`reports/recovered-subagent-reports.md`](reports/recovered-subagent-reports.md) | The three read-only audits (simulator choice, protocol sources, ns-3 architecture) that fixed the key decisions |

## Context outside this repo (read-only)

- Paper drafts + writing guide: `../ndm/`
- Research formulation (problem, state machine, proof obligations): `../ndm/osdi-research-foundation.md`
- Standards corpus (MRC spec, SRv6 RFCs, donor simulators, UCC): `../ndm/standards/`

## Execution environment

Agents run unattended on **M2** (`lahme100@login15.imec.be`, `~/Workspace/`); **all builds and
experiments run inside the docker container `ndm-sys` on M3** (`wt-1-23`, via the lynx jump
host). See `AGENTS.md` § Execution environment and `tools/remote/README.md`.

## Status

Phase 0 in progress: governance + cluster + container done (2026-09-19); fork adoption + first
in-container build open. See `PLAN.md` phase status and `ASK.md` for open questions.
