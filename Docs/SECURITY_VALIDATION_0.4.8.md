# Security Validation 0.4.8

## Issue

`UAiNpcDialogueComponent::ApplyPersona` called `CancelAllPendingReviews()` on a
semantic-review component that can be shared by multiple NPCs. Changing one NPC's
persona could cancel another NPC's pending review and emit an unrelated fallback.

## Fix

- Collect only the ReviewIds owned by the current dialogue component.
- Cancel those reviews through `CancelSemanticReview`.
- Cancel the matching provider HTTP requests through `CancelSemanticReviewRequest`.
- Do not call the global `CancelAllPendingReviews` from persona replacement.

## Verification

- Source review of the changed path.
- UE 5.8.4 BuildPlugin: Editor, Game Development, Game Shipping.
- Plugin Automation: 20/20.

## Not covered

- A dedicated two-NPC runtime regression test with a shared semantic-review component
  has not been added to the 20-test suite; the primitive single-review cancellation
  behavior is covered by `Runtime.SemanticReviewContract`.
- Dedicated server and multi-client replication remain out of scope.
