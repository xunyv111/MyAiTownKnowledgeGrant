# Security Validation 0.4.9

## Release package check

The Win64 Build ZIP ships `Scripts/check-release.mjs` but previously omitted
`CHANGELOG.md`, so the script failed with `ENOENT` when run from the extracted build
package. `Config/FilterPlugin.ini` now includes `/CHANGELOG.md`.

Both release packages are expected to pass:

```text
node Scripts/validate-plugin.mjs
node Scripts/check-release.mjs
```

## Multi-NPC persona isolation

The new `Runtime.PersonaReviewIsolation` test creates two dialogue components that
share one semantic-review component. It switches the persona of the first NPC and
asserts that:

- the first NPC's review is removed;
- the second NPC's review remains pending;
- the second NPC's review correlation remains intact.

This complements the 0.4.8 production fix that replaced `CancelAllPendingReviews()`
with per-NPC `CancelSemanticReview()` and `CancelSemanticReviewRequest()` calls.

## Verification

- UE 5.8.4 BuildPlugin: Editor, Game Development, Game Shipping.
- Plugin Automation: 21/21.
- Release package check on Source and Win64 Build ZIPs.

## Not covered

- Dedicated server and multi-client replication.
- Real-provider cancellation timing and billing behavior.
- Full open-domain semantic entailment proof.
