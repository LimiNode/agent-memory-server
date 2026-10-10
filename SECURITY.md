# Security policy

## Supported versions

The repository is an early scaffold. Until a first release is tagged, report security issues against the default branch and include the commit SHA.

## Reporting a vulnerability

Do not post credentials, exploit details or customer data in a public issue. Prefer a private security channel configured for the repository owner. If no private channel is available, open a minimal issue asking for a secure contact and do not include the vulnerability details.

Please include the affected component, reproduction steps, impact and a suggested mitigation when safe to do so.

## Deployment baseline

Bind local HTTP to loopback by default. Do not expose the service to a network without authentication, origin validation, TLS and explicit access-control configuration. Keep secrets outside the repository and rotate them if they are exposed.
