# Security Policy

## Reporting

Do not open a public issue for a vulnerability involving secret leakage, authority bypass, unsafe model output, or save tampering. Report it privately to the repository maintainer with reproduction steps, affected version, and impact.

## Scope

The plugin is deployed server-side. The authority checks in `UArpgKnowledgeGrantAdapterComponent`, `UKnowledgeGrantComponent`, `UNpcKnowledgeReceiverComponent`, `UKnowledgeResolverLibrary`, the mechanical guard, and the semantic review coordinator are part of the trust boundary. `SafeText` is only an intermediate mechanical result. A production host that bypasses `FinalApprovedText`, accepts stale semantic responses, or ships plaintext secrets in client-cooked assets is outside the plugin's protection.

## Secrets

The repository must not contain API keys, access tokens, Android File Server security tokens, private certificates, or local environment overrides. Generated logs, crash dumps, and Saved directories must not be committed.
