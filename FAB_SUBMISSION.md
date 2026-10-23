# Fab Submission Checklist

Status: **not submitted**. The 0.3.0 package is an ARPG code-plugin integration candidate, not yet a completed Fab listing.

## Current candidate

- Engine: Unreal Engine 5.7.4
- Platform built: Win64
- Package type: code plugin with source and precompiled Editor/Game objects
- Plugin version: 0.3.0
- Snapshot schema: 3
- License: MIT License

## Required before submission

1. Confirm Fab license terms for an MIT-licensed code plugin. The GitHub license and the Fab listing license are separate decisions.
2. Decide whether `CanContainContent` stays `true`. The plugin currently ships no content assets; either set it to `false` or add a real sample map/data asset.
3. Add the engine version metadata expected by Fab and verify the exact supported-version range.
4. Prepare store media: icon, thumbnail, screenshots, and a short feature video. The current acceptance screenshots are diagnostic HUD captures and are not store media.
5. Test in a clean UE 5.7 project: enable the plugin, compile, cook/package, and exercise a real dialogue outlet that consumes only `FinalApprovedText` after external semantic review.
6. Decide whether to ship as Beta. `IsBetaVersion` is currently `true`.
7. Prepare listing text: short description, long description, feature list, supported platforms, support contact, documentation link, changelog, and known limitations.
8. Remove all host projects, `Saved`, `Intermediate`, `Binaries` beyond the packaged plugin, logs, crash dumps, and machine paths from the submission package.
9. Confirm no API keys, tokens, or local environment overrides are present.
10. Run Fab's technical review package check and a fresh-project compile before upload.

## Known technical gaps

- No manual keyboard/mouse Play acceptance for the saved map.
- No target-project Cook/Package or deployment acceptance.
- No formal external dedicated-server process.
- No 10k NPC / 1k pack scale benchmark.
- The mechanical gate is a structural evidence/metadata check; the plugin now exposes a provider-neutral semantic review contract, but the target ARPG must supply and validate the real review service.
- No cross-server ledger, server-side content delivery, global NPC identity, or client anti-datamining.

## Suggested listing boundaries

Describe the plugin only as a **runtime knowledge authorization, quest-adapter, and semantic-review contract for single-player or networked ARPG projects**. Do not market it as a generic RPG framework, cross-server backend, model SDK, or absolute truth verifier.
