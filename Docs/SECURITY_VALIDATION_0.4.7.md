# Security Validation 0.4.7

## Scope

Single-player ARPG runtime plugin. Multiplayer dedicated-server validation is explicitly out of scope for this release.

## Automated coverage added

- Semantic Review Policy lost before completion:
  - expected `semantic_review_policy_unavailable`, fail closed.
- Review Provider reconfiguration:
  - Provider Generation advances.
- Review credential clearing:
  - Provider Generation advances and in-flight review requests are cancelled.
- Single-review cancellation:
  - only the target review is removed and completed once.
- Uncited Roleplay:
  - independent maximum character and claim-count gates.
- Request idempotency:
  - 100 repeated pending submissions with one `ClientRequestId` resolve to one provider request.
- Grant revocation replay:
  - old `GrantEventId` remains duplicate and does not re-authorize; only a new event can re-grant.
- Host output audit:
  - `KnowledgeGroupChatHost` has no `RawProviderPayload` UI binding; output is `FinalApprovedText` only.

## Evidence

- UE 5.8.4 BuildPlugin: Editor, Game Development, Game Shipping.
- Plugin Automation: 20/20.
- Single-player Demo concurrency run:
  - 100 same-ID submissions.
  - Mock Provider received 2 POSTs total: 1 dialogue + 1 review.
  - 100 completed duplicates used cached replay.
- Evidence directory:
  `KnowledgeGroupChatHost/Saved/ConstraintTestRuns/2026-10-31-concurrency-01/`

## Not covered

- Real DeepSeek provider-side request-count measurement under concurrency.
- Dedicated server and multi-client replication.
- External host projects that bypass the documented output contract.
- Cook/Package, manual play, long-running memory and scale benchmarks.
