# Architecture

This document describes the intended MVP boundary; it is not a claim that every component is implemented yet.

```text
Codex / Claude / Cursor / custom clients
                 |
       MCP Streamable HTTP or stdio
                 |
          protocol adapters
                 |
           MemoryService
                 |
       agent-memory-cpp + MDBX
```

## Responsibilities

- `agent-memory-cpp`: canonical content, revisions, provenance, lifecycle, storage and retrieval.
- `agent-memory-server`: process lifecycle, configuration, workspace routing, authentication and transport adapters.
- MCP adapters: validate protocol envelopes and map them to `MemoryService`; they do not implement a second memory model.

## HTTP defaults

The first local HTTP mode should bind to `127.0.0.1` and expose `/mcp`. It targets the MCP `2026-07-28` protocol revision; compatibility with older clients is an adapter concern and unsupported revisions must fail clearly. A present invalid `Origin` must be rejected; an absent `Origin` is a separate case because native clients may omit it. Whether an absent header is accepted depends on the declared transport and authentication profile, and it never replaces authentication or workspace authorization. An unauthenticated loopback mode is allowed only as an explicit single-user configuration. Multi-client or multi-workspace mode requires authentication on every request and operation, server-established client identity and authorization for the requested workspace. Listening beyond loopback additionally requires TLS and an access-control model. Streamable HTTP is an MCP transport, not a replacement for a future REST adapter.

## Memory lifecycle semantics

The MVP uses these deliberately separate meanings:

- `forget` performs a logical hide/tombstone: active retrieval indexes suppress the item, tombstone metadata and the revision remain available to authorized history queries, and exports/backups follow the configured retention policy. It does not promise physical erasure.
- Superseding a fact creates a newer revision and preserves the previous revision and provenance.
- Physical erasure is a separate future administrative operation. Its contract must define removal from active indexes, history, exports and backups before it is exposed.

This distinction prevents a caller from confusing search visibility with deletion obligations.

## stdio rules

stdout is reserved for protocol messages. Diagnostics go to stderr. A stdio process may own a private local context; a later proxy mode can connect stdio clients to a shared HTTP service.

## Non-goals for MVP

No web UI, OAuth identity provider, distributed cluster or mandatory cloud dependency. These can be added as adapters around the same `MemoryService` after the core scenario is working.
