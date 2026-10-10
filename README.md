# agent-memory-server

Open-source, self-hosted service for persistent AI-agent memory, built on [`agent-memory-cpp`](https://github.com/LimiNode/agent-memory-cpp). Local-first, model-agnostic, with history, retrieval and provenance.

> Project status: early scaffold. The public repository documents the target boundary and MVP; it does not yet claim production readiness or benchmark results.

## Why a server repository?

`agent-memory-cpp` owns domain logic, storage contracts, history, provenance and retrieval. This repository owns the service lifecycle, configuration, isolation, access control and protocol adapters. MCP is one supported interface, not the internal architecture.

The intended product is one process that can serve multiple local agents over MCP Streamable HTTP, with stdio for clients that launch a local child process.

## Planned MVP

- C++17 process with no mandatory Python, GPU or internet access.
- One `MemoryService` behind both transports: MCP Streamable HTTP and stdio.
- Stable operations: `remember`, `search`, `get_history`, `forget`, `validate_source`.
- MDBX-backed storage through `agent-memory-cpp` contracts.
- Deterministic lexical/BM25 retrieval first; dense and hybrid retrieval can follow.
- Workspace/source isolation and explicit authentication for network access.
- Integration scenario: save a decision, restart, retrieve it with provenance, change the source and detect stale knowledge.

## Transport direction

The preferred long-running local service is Streamable HTTP, bound to `127.0.0.1` by default. stdio remains useful for clients that manage a child process. Both transports must call the same domain service; protocol code must not duplicate storage or lifecycle logic.

REST, a web UI, OAuth server and distributed clustering are deliberately out of the first release.

## License and commercial terms

This project is licensed under [AGPL-3.0](LICENSE). You may use and modify it under the license terms, including the requirements that apply when a modified version is offered over a network. Commercial licensing with alternative terms may be available from the copyright holder; it is not required for ordinary compliant AGPL use.

The underlying `agent-memory-cpp` library is a separate MIT-licensed project. Check the license of every dependency and contribution before redistribution.

## Security defaults

Local HTTP should listen only on loopback unless explicitly configured otherwise. Network exposure requires authentication, origin checks, TLS and an access-control model. Do not put secrets in this repository or in issue reports.

## Development

The implementation is being added incrementally. See [docs/architecture.md](docs/architecture.md), [docs/roadmap.md](docs/roadmap.md) and [CONTRIBUTING.md](CONTRIBUTING.md) for the current boundary and contribution expectations.

Commercial licensing is available for organizations requiring alternative licensing terms. Contact the copyright holder with the intended deployment and distribution model.
