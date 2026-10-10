# Contributing

The project is early and the public boundary is intentionally small. Before implementing a feature, open an issue describing the user scenario, the relevant layer and how it will be tested.

## Boundaries

- Keep domain semantics in `agent-memory-cpp` when possible.
- Keep MCP/HTTP/stdio code as thin adapters around one service interface.
- Do not add a second storage format or silently send memory data to an external service.
- Keep stdout clean in stdio mode; logs belong on stderr.
- Do not add artificial Community limits or unannounced telemetry.

## Rights and dependencies

By contributing, you confirm that you have the right to submit the work and that dependencies are compatible with AGPL-3.0. External source-code contributions will be merged only after the project has established and published a contributor-rights process compatible with its dual-licensing strategy. Until then, maintainers may accept documentation-only changes that do not create new licensing dependencies.

Do not submit credentials, customer data, proprietary code or generated files containing secrets.
