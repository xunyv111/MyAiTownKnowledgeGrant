# Security Policy

## Reporting

Do not open a public issue for a vulnerability involving secret leakage, authority bypass, unsafe model output, or save tampering. Report it privately to the repository maintainer with reproduction steps, affected version, and impact.

## Scope

The plugin is deployed server-side. The authority checks in `UKnowledgeGrantComponent`, `UNpcKnowledgeReceiverComponent`, `UKnowledgeResolverLibrary`, and the model guard are part of the trust boundary. A host that bypasses `SafeText` or ships plaintext secrets in client-cooked assets is outside the plugin's protection.

## Secrets

The repository must not contain API keys, access tokens, Android File Server security tokens, private certificates, or local environment overrides. Generated logs, crash dumps, and Saved directories must not be committed.
