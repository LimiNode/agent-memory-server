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

The first local HTTP mode should bind to `127.0.0.1`, expose `/mcp`, reject invalid `Origin` values and require explicit authentication when listening beyond loopback. Streamable HTTP is an MCP transport, not a replacement for a future REST adapter.

## stdio rules

stdout is reserved for protocol messages. Diagnostics go to stderr. A stdio process may own a private local context; a later proxy mode can connect stdio clients to a shared HTTP service.

## Non-goals for MVP

No web UI, OAuth identity provider, distributed cluster or mandatory cloud dependency. These can be added as adapters around the same `MemoryService` after the core scenario is working.
