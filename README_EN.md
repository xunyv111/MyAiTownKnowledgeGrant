# My AI Town Knowledge Grant

**Author and copyright holder: xunyv**

[Chinese documentation](README.md) | [English installation checklist](INSTALL_EN.md) | [Verification summary](Docs/VERIFICATION.md)

My AI Town Knowledge Grant is an Unreal Engine Runtime plugin for guarded AI-NPC knowledge in single-player and networked ARPGs. It provides server-authoritative knowledge grants, context-scoped dialogue views, deterministic NPC reactions, strict model-response validation, and a fail-closed semantic review contract.

Version 0.4.0 introduced an optional bring-your-own-key (BYOK) AI-NPC roleplay entry point. A player can connect an OpenAI Chat Completions-compatible service from a runtime settings panel. Each NPC with an `AI NPC Dialogue Component` receives an independent local session. API keys, session state, and recently approved dialogue remain in memory for the current game process and are not persisted by the plugin.

The Fab source package contains complete human-readable C++ source. It intentionally excludes generated `Binaries`, `Build`, `Intermediate`, and `Saved` directories.

## Core Pipeline

```text
Server confirms quest completion
  -> fixed registry grants a Knowledge Pack
  -> context-scoped dialogue view is created
  -> model returns strict JSON claims
  -> mechanical evidence and metadata guard
  -> independent semantic review
  -> FinalApprovedText
```

When semantic review is enabled, subtitles, voice, behavior, and production logs must consume only `FinalApprovedText`. Raw model output and the intermediate mechanical `SafeText` are not approved dialogue.

## Requirements

- Unreal Engine 5.8.4 (validated version)
- Windows 64-bit (validated platform)
- A C++ toolchain compatible with the target Unreal Engine installation
- Unreal Engine built-in `GameplayTags`, `HTTP`, `JSON`, `UMG`, and `Slate` modules
- No third-party model SDK or database dependency

Other Unreal Engine versions or platforms must compile the source and repeat project-level build, automation, cook, package, and multiplayer validation.

## Installation

1. Close Unreal Editor.
2. Copy the complete `MyAiTownKnowledgeGrant` folder into `YourProject/Plugins/`.
3. Regenerate project files when required by the host project.
4. Compile the project or allow Unreal Engine to compile the plugin when the project opens.
5. Open **Edit > Plugins**, enable **My AI Town Knowledge Grant**, and restart the editor.

See [INSTALL_EN.md](INSTALL_EN.md) for the complete integration and release checklist.

## Minimal Knowledge-Grant Integration

1. Add a `Knowledge Grant Component` to the server-authoritative player Actor or `PlayerState`. Do not add one to every NPC.
2. Create `Knowledge Pack` Data Assets. Give every pack a unique `Pack Id`, one quest or world-event source, prerequisite pack IDs when needed, and atomic facts.
3. For production quest rewards, add an `ARPG Knowledge Grant Adapter Component`, configure a fixed registry, and let the server quest system implement `ARPG Quest Authority Provider`.
4. Grant quest knowledge through `Grant Completed Quest Knowledge`. The adapter verifies completion, player ownership, event order, and registry lookup before producing a stable grant event.
5. Add a `Knowledge Context Collector Component` to the player, `PlayerState`, or dialogue coordinator. Use it to collect scene, interaction, and explicit-mention Gameplay Tags.
6. Call `Build Dialogue Knowledge View` before model generation. Send only `Allowed Documents` and their facts to the model.
7. Require the model to return the strict JSON contract shown below.
8. Pass the raw JSON, the original view, the actual context collectors, and a trusted guard policy to `Guard Knowledge Model Json Response For Collectors`.
9. Send the mechanical result to `Knowledge Semantic Review Component` for free-form dialogue.
10. Publish only `FinalApprovedText` after successful semantic review.

## Strict Model JSON Contract

```json
{
  "claims": [
    {
      "claimId": "claim-1",
      "text": "The eastern gate is closed.",
      "evidenceFactIds": ["fact.east_gate.closed"],
      "claimedEntities": ["Eastern Gate"],
      "claimedNumbers": [],
      "negated": false
    }
  ]
}
```

The root object and each claim reject extra fields. Empty or malformed payloads, invalid types, duplicate IDs, excessive arrays, oversized text, missing evidence, incorrect protected names or numbers, and inconsistent negation fail closed.

## Context and Authorization Model

The final visible knowledge set is:

```text
player-authorized packs
  intersect context-activated packs
  intersect mechanically validated claims
  intersect semantically approved claims
```

Gameplay Tags select currently relevant information; they never grant knowledge. A future event does not become available merely because its scene tag is active. Grants are authority-only and are deduplicated with a stable normalized `GrantEventId -> PackId` record.

`FDialogueKnowledgeView` is the single dialogue snapshot. It contains context tags, authorization revision and epoch, allowed documents, exclusion reasons, and a CRC32 consistency value. If context, authorization, or the view changes while a response is in flight, the output gate rejects the stale response. CRC32 detects accidental modification; it is not a cryptographic signature.

## Optional AI-NPC Provider

The host project can call `Show AI NPC Provider Settings` from a pause or settings menu. The runtime panel accepts a provider name, complete Chat Completions endpoint URL, model name, API key, and explicit transmission consent.

The API key is stored only in runtime memory. It is not written to `.ini`, SaveGame, Data Assets, or plugin logs. **Clear credentials** removes the key from the plugin subsystem and blocks new requests. It cannot recall a request that has already been sent and does not claim secure erasure of engine or operating-system memory.

Character description, current authorized facts, and the player's current message are sent only after consent. Candidate dialogue and allowed facts may also be sent to the configured semantic review service.

Remote endpoints must use HTTPS. Plain HTTP is allowed only for explicitly permitted loopback addresses used by local model services.

Player BYOK is intended for single-player games and local listen servers. A production dedicated server should not request or trust client API keys. It should use a server-owned provider configuration and server-controlled privacy, cost, prompt, and audit policies.

The core knowledge-grant, personality-reaction, and mechanical guard features do not require an API key.

## NPC Reactions

`Npc Knowledge Receiver Component` stores deterministic reaction receipts, not knowledge authorization. A personality profile can influence interest, approval, perceived threat, dialogue tone, behavior intent, and bounded affect suggestions without changing the player's authorized fact set.

Shared NPCs can keep reactions isolated by stable `AudienceId` and `ReceiptEventId`. Reactions do not create an `NPC x player` copy of the knowledge database.

## Persistence and Replication

- Grant and restore operations are authority-only.
- Component state replicates through Unreal Engine networking.
- Current player knowledge and receiver reaction snapshots use schema version 3.
- Restore rejects incompatible schemas, registry mismatches, unknown packs, missing prerequisite closure, cycles, forged tags, missing event history, and records that reference unauthorized packs.
- Successful restore advances the authorization revision and creates a new authorization epoch, invalidating all earlier dialogue views.
- Version 1 and version 2 snapshots are not migrated automatically.
- Optional record limits fail explicitly with `grant_history_full` or `receipt_history_full`; records are never silently discarded.

## Runtime Safety Rules

- Formal quest grants must resolve `QuestId -> KnowledgePack` through a fixed server registry.
- Do not let client Blueprints supply arbitrary player IDs, packs, or grant event IDs for production quest completion.
- Use `On Grant Audited` to connect the host persistence or outbox layer when durable audit is required.
- Do not ship plaintext story secrets in client-cooked Data Assets if they must resist extraction.
- Do not expose raw model output, stale views, or mechanical `SafeText` as approved free-form dialogue.
- Treat missing, timed-out, malformed, incomplete, or stale semantic review as blocked.

## Validation

Run the following tests in Unreal Automation:

- `MyAiTown.KnowledgeGrant.Runtime.QuestFlow`
- `MyAiTown.KnowledgeGrant.Runtime.Validation`
- `MyAiTown.KnowledgeGrant.Runtime.CausalChain`
- `MyAiTown.KnowledgeGrant.Runtime.DialogueGuard`
- `MyAiTown.KnowledgeGrant.Runtime.ModelAdapterContract`
- `MyAiTown.KnowledgeGrant.Runtime.SnapshotConsistency`
- `MyAiTown.KnowledgeGrant.Runtime.ContextTagLibraries`
- `MyAiTown.KnowledgeGrant.Runtime.ContextCollector`
- `MyAiTown.KnowledgeGrant.Runtime.SemanticReviewContract`
- `MyAiTown.KnowledgeGrant.Runtime.AiNpcProviderContract`
- `MyAiTown.KnowledgeGrant.Reaction.PersonalityAndAffect`
- `MyAiTown.KnowledgeGrant.Reaction.DelayedReceipt`

Version 0.4.2 passed UnrealHeaderTool, Editor Development, Game Development, Game Shipping, and 12 of 12 plugin automation tests with 0 warnings and 0 failures on Unreal Engine 5.8.4 / Win64. See [Docs/VERIFICATION.md](Docs/VERIFICATION.md) for the public machine-independent evidence summary.

Without the Unreal toolchain, `node Scripts/validate-plugin.mjs` performs structural and boundary checks. It does not replace UnrealHeaderTool, C++ compilation, Automation Test, Cook, Package, or multiplayer testing.

## Privacy and Network Boundary

The plugin does not scan project files or provide vector search. Network access occurs only through the optional AI-NPC provider configured by the player or host project. The host is responsible for endpoint ownership, pricing, authentication, availability, privacy disclosure, data retention, and model quality.

The mechanical guard checks evidence references and declared metadata. It cannot mathematically prove complete open-domain semantic entailment. Important story facts should use host-controlled canonical templates or an independently validated semantic service.

## Known Limits

- No cross-server authorization ledger or server migration system.
- No server-side story-content delivery service.
- No global persistent NPC identity service.
- No operations audit database.
- No automatic migration from snapshot schema v1/v2 to v3.
- No built-in rolling compaction of unlimited grant or reaction history.
- No guarantee against extraction of plaintext assets shipped to clients.
- No guarantee if the host bypasses the guard or semantic review output gate.
- No certification for unlisted Unreal Engine versions or platforms.

## License

The plugin source is distributed under the included [MIT License](LICENSE). Fab marketplace licensing and listing terms are separate from the repository license and must be configured by the publisher during submission.
