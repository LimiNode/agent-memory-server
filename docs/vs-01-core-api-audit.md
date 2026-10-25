# VS-01: `agent-memory-cpp` API capability audit

Status: design/audit only. This document records what is implemented by the
core library; it is not a claim that the server is implemented.

## Reproducible baseline

| Item | Value |
| --- | --- |
| Server base | `7eca2ec9710ab345ce523b63c4e6ea786afddac1` |
| Core repository | `LimiNode/agent-memory-cpp` |
| Core audit SHA | `4018fb124b9da21814f82c2b90706bed87991cfc` |
| Core language/target | C++17, static target `agent_memory`, alias `agent_memory::agent_memory` |

The audit used public headers, implementations, tests and CMake at the exact
core SHA above. No experimental or unmerged core API is assumed.

## API inventory and evidence

### Import and lexical retrieval

`MdbxResourceImporter` is the narrowest API that combines canonical content,
lexical retrieval and a current resource manifest. Its public contract is in
[`MdbxResourceImporter.hpp`](https://github.com/LimiNode/agent-memory-cpp/blob/4018fb124b9da21814f82c2b90706bed87991cfc/src/agent_memory/infrastructure/mdbx/MdbxResourceImporter.hpp):

- `ResourceImportPublicationProfile::LexicalOnly` and importer options are at
  lines 46–92;
- `ResourceId` is globally unique for the manifest profile and publication is
  fenced/recoverable (lines 98–109);
- `import(ObservedResource)` and `find_manifest(ResourceId)` are declared at
  lines 131–137;
- `search(scope, query, limit)` is declared at lines 139–150, with a bounded
  candidate limit (`kMdbxResourceImporterMaxSearchLimit`, line 95).

`ObservedResource` carries `ScopeId`, stable `ResourceId`, source URI, UTF-8
body, text origin/provenance and pipeline hash
([`ResourceImporter.hpp`](https://github.com/LimiNode/agent-memory-cpp/blob/4018fb124b9da21814f82c2b90706bed87991cfc/src/agent_memory/ingestion/ResourceImporter.hpp),
lines 17–26). `ResourceImportResult` returns the resource revision, document,
chunk and (only for raw profiles) raw unit IDs (lines 28–43). The revision
contains generation, pipeline hash, optional SHA-256 body digest and provenance
([`Resource.hpp`](https://github.com/LimiNode/agent-memory-cpp/blob/4018fb124b9da21814f82c2b90706bed87991cfc/src/agent_memory/domain/Resource.hpp),
lines 53–64).

The importer hides a resource with a pending update until manifest publication
completes and leaves the previous manifest visible after an interrupted
publication; this is documented in the importer header (lines 139–145) and
covered by the recovery/reopen cases in
`tests/infrastructure/mdbx/mdbx_resource_importer_test.cpp` (roughly
1169–1226 and 1339–1516 at this SHA).

### Minimal durable lexical adapter

`MdbxDurableLexicalDocumentMemory` exposes `upsert_text` and scope-local BM25
`search` ([header](https://github.com/LimiNode/agent-memory-cpp/blob/4018fb124b9da21814f82c2b90706bed87991cfc/src/agent_memory/infrastructure/mdbx/MdbxDurableLexicalDocumentMemory.hpp),
lines 69–134). It stores one canonical text block per resource, rebuilds the
derived lexical view on reopen, advances a one-based generation only when the
body changes, and keeps the resource identity stable (lines 71–81 and
100–112). The corresponding test proves reopen, body update/stale-term removal,
URI-only update semantics and interruption recovery at
`tests/infrastructure/mdbx/mdbx_durable_lexical_document_memory_test.cpp:71-124`
and `:127-200`.

This adapter is intentionally smaller, but it has no manifest lookup, body
digest/provenance result, history enumeration, logical tombstone or source
validation API. It remains a useful implementation reference, not the selected
server baseline.

### Manifest, canonical history and derived stores

- `MdbxResourceManifestStorage::find_manifest` and `erase_manifest` are the
  only manifest read/remove methods
  ([header](https://github.com/LimiNode/agent-memory-cpp/blob/4018fb124b9da21814f82c2b90706bed87991cfc/src/agent_memory/infrastructure/mdbx/MdbxResourceManifestStorage.hpp),
  lines 61–83). `erase_manifest` physically removes the manifest; it is not a
  logical hide. The test explicitly checks removal and idempotence at
  `tests/infrastructure/mdbx/mdbx_resource_manifest_storage_test.cpp:500-508`.
- `MdbxCanonicalContentStore::read_revision(document_id, revision)` reads one
  revision only when the caller already knows its number
  ([header](https://github.com/LimiNode/agent-memory-cpp/blob/4018fb124b9da21814f82c2b90706bed87991cfc/src/agent_memory/infrastructure/mdbx/MdbxCanonicalContentStore.hpp),
  lines 88–110). There is no resource-level history listing method.
- `IProjectionStore::list` is a historical/diagnostic projection view, not an
  active resource-history or retrieval API; the interface says so explicitly at
  `src/agent_memory/storage/IProjectionStore.hpp:23-33`. `delete_revision` is
  physical reclaim and creates no tombstone (`:35-47`).
- `ResourceManifestState` has only `Active` and `ErasePending`
  (`src/agent_memory/domain/Resource.hpp:94-98`); neither value is a public
  user-facing forget contract.
- `MdbxWorkspaceStorageRegistry` provides explicit create-only placement and
  fail-closed generation checks (`src/agent_memory/infrastructure/mdbx/MdbxWorkspaceStorageRegistry.hpp:88-118`).
  It is process-local composition-root state, not authentication or
  authorization. `MdbxStorageContext::open/attach` and its thread-bound
  transaction/lifecycle rule are documented at
  `src/agent_memory/infrastructure/mdbx/MdbxStorageContext.hpp:24-48`.

## Capability matrix

Status meanings: **Ready** means the selected profile can implement the
operation without inventing storage semantics; **Limited** means a narrow,
explicit subset is possible; **Blocked** means the requested operation would be
misleading or unsafe with the current public API.

| Service operation | Core API used by the selected `LexicalOnly` profile | Implemented evidence | Missing or unsafe part | Guarantee we can make now | Decision |
| --- | --- | --- | --- | --- | --- |
| `remember` | `MdbxResourceImporter::import(ObservedResource)` | Validates and publishes a resource revision; returns generation, digest/provenance metadata and stable IDs. Recovery and reopen are tested. | No server auth, transport or policy layer yet. | Durable publish of one UTF-8 source revision, with stable `ResourceId`, scope and generation semantics. | **Ready** |
| `search` | `MdbxResourceImporter::search(scope, query, limit)` plus `find_manifest` | Deterministic lexical search over importer-visible resources; pending publications are hidden; limit is bounded. | No dense/hybrid ranking, history view or authorization. | Scope-local active lexical results with source URI, resource ID and generation; stale pending revisions are not returned as active results. | **Ready** |
| `get_history` | `MdbxCanonicalContentStore::read_revision` (known revision only); projection `list` is diagnostic | Individual known revisions can be read; projection revisions can be listed. | No API enumerates all revisions for a resource; no complete resource-level provenance history query. | Only “read this already-known revision”, not a history list. | **Blocked** |
| `forget` | `MdbxResourceManifestStorage::erase_manifest` / store erases | Physical manifest/envelope/projection deletion exists and is idempotent. | No atomic logical tombstone that suppresses search while retaining authorized history; physical erase is not equivalent. | No public forget guarantee. | **Blocked** |
| `validate_source` | `ResourceRevision::body_digest`, generation and provenance in `find_manifest` | Stored metadata can be inspected and compared when a caller already supplies the same source bytes/digest. | No source connector, URI re-fetch, or service operation that performs the comparison; URI changes are metadata, not proof of current external content. | At most metadata inspection; no external freshness claim. | **Limited** |

The matrix deliberately does not promote a low-level delete or a known-revision
read into a stronger service promise.

## Selected profile

The first server baseline is **`MdbxResourceImporter` with
`ResourceImportPublicationProfile::LexicalOnly`**.

| Criterion | `MdbxDurableLexicalDocumentMemory` | Importer `LexicalOnly` | Decision |
| --- | --- | --- | --- |
| Durability/reopen | Proven, one canonical body and rebuildable BM25 view | Proven, canonical body, manifest and recoverable publication | Both pass |
| Revision/provenance | Generation and URI in lexical hit; no manifest/digest API | Revision includes generation, pipeline hash, optional SHA-256 digest and provenance | Importer wins |
| Search visibility | Scope-local lexical view | Scope-local importer-visible view; pending update hidden | Importer wins for service semantics |
| Recovery | Durable intent and reopen recovery | Durable intent, fencing and manifest publication recovery | Importer wins |
| Complexity | Smallest code path | Adds manifest and globally unique `ResourceId` rule | Accepted trade-off |
| Missing operations | History/forget/validation absent | Same missing lifecycle APIs | Neither solves VS-01 blockers |

This is one concrete composition, not a new generic memory abstraction. Raw
chunks, projections and `ProjectionAwareLexicalRetriever` are deferred because
they add topology and lifecycle rules without unblocking the first remember /
search vertical slice.

## Reopen proof available at the baseline

The existing core tests, rather than a new server fake, prove the selected
storage behavior:

1. Import a resource and search it.
2. Reopen the importer against the same MDBX context.
3. Find the manifest and search again.
4. Update the body and observe a new generation with stale lexical terms gone.
5. Inject an interruption and verify recovery on reopen/retry.

Evidence is in `mdbx_resource_importer_test.cpp:1339-1443` (initial import,
reopen, generation and digest), `:1451-1516` (recovery), and the durable lexical
test at `mdbx_durable_lexical_document_memory_test.cpp:71-124` (including
URI-only update and stale-term removal). This PR adds no duplicate backend or
fake proof.

## CMake/integration check

The core CMake graph was inspected at the exact SHA: it defines the static
`agent_memory` target and `agent_memory::agent_memory` alias at
`CMakeLists.txt:26-102`, and adds MDBX adapters at `:130-148` when
`AGENT_MEMORY_ENABLE_MDBX=ON`.

The non-MDBX configuration completed successfully with CMake 4.3.3 and C++17.
The opt-in server smoke harness in `CMakeLists.txt` and
`tests/vs01_core_smoke.cpp` configures the audited core as an external
subdirectory, links the public `agent_memory::agent_memory` target and enables
MDBX without copying core sources into this repository. It was configured with:

```text
cmake -S . -B .build/vs01 -DCMAKE_BUILD_TYPE=Release \
  -DAGENT_MEMORY_ENABLE_MDBX=ON \
  -DAGENT_MEMORY_MDBX_DEPS_MODE=BUNDLED \
  -DAGENT_MEMORY_BUILD_TESTS=OFF -DAGENT_MEMORY_BUILD_EXAMPLES=OFF \
  -DAGENT_MEMORY_ENABLE_JSON=OFF
```

The exact audit checkout first needed its pinned git submodules initialized. With
those dependencies present, CMake configure, build and CTest all passed on the
Windows MinGW toolchain:

```text
1/1 Test #1: agent_memory_server_vs01_core_smoke ... Passed
100% tests passed, 0 tests failed out of 1
```

The smoke test proves compile/link, fresh import/search, process-style object
destruction, reopen/search, generation increment and stale-term removal. It is
deliberately opt-in because the server repository does not vendor the core or
its MDBX submodules. No production `MemoryService` is introduced in VS-01.

## VS-02 recommendation

Implement one in-process `MemoryService` composition around the selected
`LexicalOnly` importer with only `remember` and `search`, plus a cross-process
reopen integration test. Keep transport out of the service and make workspace
placement explicit. Do not add `get_history`, `forget` or `validate_source`
until their lifecycle contracts have corresponding core APIs and tests.
