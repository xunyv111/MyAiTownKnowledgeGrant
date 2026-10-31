# Host Output Contract

## One official output

NPC subtitles, voice, behavior, and production logs must consume only:

```text
OnFinalReply.FinalApprovedText
```

`FinalApprovedText` is either:

- text that passed the mechanical guard and independent semantic review; or
- a host-trusted fallback selected after a blocked, failed, cancelled, or timed-out review.

## Low-level provider events are diagnostics

`UAiNpcProviderSubsystem::OnDialogueProviderCompleted` is a transport-level diagnostic event. Its `RawProviderPayload` is untrusted model output and must never be displayed as NPC dialogue.

The plugin cannot prevent a hostile or careless host Blueprint from binding that event and rendering `RawProviderPayload`. The host is responsible for enforcing the single output path.

Do not use these as gameplay output:

- `RawProviderPayload`
- mechanical `SafeText`
- `RejectedCandidateText`
- `SemanticReviewedClaims` or `ClaimAssessments`
- provider error bodies

## Blueprint wiring

```text
AI NPC Dialogue Component
  -> On Final Reply
  -> read FinalApprovedText only
  -> subtitles / voice / behavior / production log
```

Do not connect `On Dialogue Provider Completed` to any player-facing widget, subtitle, voice, or behavior node.

## Host verification

Before shipping, search the host project for:

- `OnDialogueProviderCompleted`
- `RawProviderPayload`
- direct provider-result binding to UMG, audio, AI behavior, or save data

Any such connection must be removed or explicitly classified as an internal diagnostic sink. Production diagnostic sinks must redact raw text and credentials.
