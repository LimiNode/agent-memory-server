# Roadmap

## Now: repository and MVP contracts

- Define request/response schemas for `remember`, `search`, `get_history`, `forget`, `validate_source`.
- Integrate the existing `agent-memory-cpp` library without duplicating domain logic.
- Implement one-process local service and a cross-process integration test.

## Next: usable local service

- MCP Streamable HTTP on loopback.
- stdio transport with strict stdout/stderr separation.
- Workspace isolation, configuration validation and basic authentication.
- Deterministic lexical retrieval, then dense/hybrid retrieval where supported by the library.

## Later, only after evidence

- stdio proxy to a shared HTTP service;
- REST adapter;
- operational tooling, replication, HA and managed hosting;
- enterprise identity, audit/SIEM connectors and commercial licensing workflows.

Performance, quality and token-savings claims must be published only with reproducible methodology and p50/p95/p99 measurements.
