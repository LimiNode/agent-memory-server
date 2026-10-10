# Security policy

## Supported versions

The repository is an early scaffold. Until a first release is tagged, report security issues against the default branch and include the commit SHA.

## Reporting a vulnerability

Do not post credentials, exploit details or customer data in a public issue. Repository maintainers should enable GitHub Private Vulnerability Reporting and use the **Report a vulnerability** flow in the [Security tab](https://github.com/LimiNode/agent-memory-server/security). If that private channel is temporarily unavailable, contact the repository owner through a private channel and include only enough public information to request secure coordination.

Please include the affected component, reproduction steps, impact and a suggested mitigation when safe to do so.

## Deployment baseline

Bind local HTTP to loopback by default. Do not expose the service to a network without authentication, origin validation, TLS and explicit access-control configuration. Keep secrets outside the repository and rotate them if they are exposed.
