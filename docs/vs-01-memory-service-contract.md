# VS-01: minimal `MemoryService` contract

This is a boundary document for the first server implementation. It describes
what a service wrapper may promise over the selected
`MdbxResourceImporter`/`LexicalOnly` profile at core SHA
`4018fb124b9da21814f82c2b90706bed87991cfc`. It does not implement a transport,
authentication framework or a second storage model.

## Scope and composition

The service owns process lifecycle, configuration, workspace routing and the
future protocol adapters. `agent-memory-cpp` owns canonical content, revisions,
provenance, manifest publication and lexical retrieval. A request is routed to
one explicitly configured workspace placement and then to one importer/context;
the client-provided `scope_id` is a data partition, not proof of authorization.

The first implementation must use one configured workspace and one
`LexicalOnly` importer. `MdbxWorkspaceStorageRegistry` may provide the explicit
workspace-to-context binding and stale-generation check, but it is process-local
and does not authenticate a caller.

## Value types

The names below are service boundary values, not duplicate persistent entities.
The implementation should translate directly to the core types where possible.

### `RememberRequest`

```text
workspace_id: non-empty string selected by the server configuration
workspace_generation: server-issued routing generation
scope_id: non-empty string
resource_id: non-empty, globally unique ResourceId for this importer profile
source_uri: non-empty string
utf8_body: non-empty valid UTF-8 text
text_origin: OriginalText (DerivedExtraction is allowed only with complete provenance)
derived_text_provenance: optional { source_media_type, extractor_id, extractor_version }
pipeline_config_hash: uint64, default 1
```

The server must reject an oversized body before calling core. VS-01 does not
choose a public byte limit because the lexical-only core API does not expose an
admission constant; the limit must be an explicit server configuration and its
value must be documented when implementation begins. Raw-profile bounds in the
core header do not apply to `LexicalOnly`.

### `RememberResult`

```text
resource_id
generation: uint64 (one-based; advances only when body bytes change)
document_id
chunk_id
body_digest: optional algorithm-tagged SHA-256 from the manifest
text_origin / derived_text_provenance
```

`source_uri` is retained as revision metadata. A URI-only reimport does not
advance the body generation; callers must not interpret URI change as a new
canonical body revision.

### `SearchRequest` and `SearchResult`

```text
workspace_id
workspace_generation
scope_id
query: non-empty UTF-8 text
limit: 1..1024 (the core importer bound)
```

Each result contains `resource_id`, `source_uri`, `generation`, canonical text
and the lexical score. The service may attach manifest digest/provenance by
looking up the same `ResourceId`; it must not invent provenance fields when no
manifest is available. Results are limited to the selected scope and active,
importer-visible manifests.

## Error model

These are service-level categories. They are not claims that the core throws a
distinct exception for every category; the wrapper must preserve a distinction
only when it has reliable evidence.

| Category | Use in the first service | Core evidence / mapping rule |
| --- | --- | --- |
| `NotFound` | Unknown workspace placement or no manifest for an explicitly requested resource. | `find_manifest` returns `nullopt`; workspace registry has an explicit unknown-workspace error. Do not map an arbitrary storage exception to `NotFound`. |
| `Conflict` | Stale workspace generation, duplicate global `ResourceId` ownership, or guarded publication conflict. | Core exposes stale-placement and publication conflict errors. Preserve them; do not use `Conflict` for ordinary body updates. |
| `InvalidArgument` | Empty identity/URI/body, invalid UTF-8, zero limit, malformed provenance or out-of-range limit. | Core documents `std::invalid_argument` for importer/upsert/search validation. |
| `Unauthorized` | Caller failed server authentication/authorization before storage access. | Not produced by core. This category belongs to the future server boundary and must never be inferred from `scope_id`. |
| `Unavailable` | Server is not configured for the requested workspace/profile or is intentionally offline. | Server policy only; do not disguise a durable corruption as unavailable. |
| `Unsupported` | `get_history`, logical `forget`, external source re-fetch, dense retrieval or another operation outside this profile. | Return explicitly; do not emulate it with physical erase or a guessed revision scan. |
| `StorageFailure` (internal) | MDBX open/transaction failure, unrecoverable intent, corrupt/missing canonical body or other backend exception. | Preserve the failure as an internal/storage error. The core does not provide a complete portable exception taxonomy. |

For an error that cannot be distinguished reliably, the implementation must use
`StorageFailure` rather than guessing `NotFound`, `Conflict` or
`InvalidArgument`.

## Operation guarantees and non-guarantees

### `remember`

The operation validates the request, routes to the configured workspace and
calls `MdbxResourceImporter::import`. On success it guarantees durable
publication of one source revision and returns the generation and IDs from the
core result. An interrupted publication is recovered by the core on retry or
reopen; the service must not expose an in-memory success before the import call
returns.

It does not guarantee authorization merely because a scope was supplied, and
it does not guarantee that an external URI still points to the same bytes.

### `search`

The operation calls importer-aware lexical search with a bounded limit. It
returns only active importer-visible results in the selected scope. A pending
update is hidden until manifest publication completes. The service must not
return low-level raw staging rows, stale lexical hits or results from another
workspace.

### `get_history`, `forget`, `validate_source`

These operations are explicitly `Unsupported` in VS-02 until core contracts
exist:

- `get_history` cannot enumerate all resource revisions. `read_revision` needs
  a revision number supplied by the caller.
- `forget` cannot be implemented by `erase_manifest`, knowledge-unit erase or
  projection deletion: those are physical operations and do not provide an
  atomic hide/tombstone plus authorized-history view.
- `validate_source` can inspect a stored digest/generation but cannot fetch or
  compare the current external source. Any later connector must be explicit and
  must not be hidden inside the core importer.

## Security boundary

VS-01 has no HTTP or MCP endpoint. Future adapters must enforce authentication
before workspace resolution, use server-established client identity, validate
HTTP `Origin`, default network listeners to loopback, and keep stdio protocol
messages on stdout with diagnostics on stderr. Workspace routing is a storage
selection mechanism, not an authorization decision.

## Acceptance for VS-02

The smallest useful implementation is `remember` + `search` over one configured
workspace, followed by process shutdown, reopen and search. It must use the
existing core APIs and include tests for scope isolation, stable identity,
generation update, stale-term removal and durable reopen. No fake five-operation
service, transport framework or enterprise layer is in scope.
