# Installation and Integration Checklist

[Chinese installation guide](INSTALL.md) | [English overview](README_EN.md)

## Compatibility

- Validated engine: Unreal Engine 5.8.4
- Validated platform: Windows 64-bit
- Plugin type: Runtime
- Built-in module dependencies: GameplayTags, HTTP, JSON, UMG, and Slate
- Third-party model SDKs or databases: none

Other Unreal Engine versions and platforms must rebuild from source and repeat the host project's Automation, multiplayer, Cook, and Package tests.

## Install

1. Close Unreal Editor.
2. Copy the complete `MyAiTownKnowledgeGrant` folder into `YourProject/Plugins/`.
3. Confirm that Gameplay Tags are available in the host project. The plugin declares the required module dependency in its Build.cs file.
4. Regenerate project files and compile a C++ project, or allow Unreal Engine to compile the plugin when a Blueprint-only project first opens.
5. Open **Edit > Plugins**, enable **My AI Town Knowledge Grant**, and restart the editor.
6. Confirm that the Output Log contains no compile or load errors for `MyAiTownKnowledgeGrant`.

The Fab source package intentionally excludes `Binaries`, `Build`, `Intermediate`, and `Saved`. Unreal Engine regenerates those directories locally.

## Minimal Integration

1. Add `Knowledge Grant Component` to the server-authoritative player Actor or `PlayerState`.
2. Create `Knowledge Pack` Data Assets with unique pack, document, and fact IDs.
3. Configure quest or world-event sources and prerequisite pack chains.
4. Add `ARPG Knowledge Grant Adapter Component` for formal quest rewards and configure its fixed pack registry.
5. Let the server quest system implement `ARPG Quest Authority Provider` and call `Grant Completed Quest Knowledge`.
6. Add `Knowledge Context Collector Component` to collect scene, interaction, and explicit-mention tags.
7. Call `Build Dialogue Knowledge View` and send only allowed documents and facts to the model.
8. Pass strict model JSON through `Guard Knowledge Model Json Response For Collectors` with the original view, actual collectors, and a trusted guard policy.
9. For free-form dialogue, pass the mechanical result through `Knowledge Semantic Review Component`.
10. Use only `Final Approved Text` for subtitles, voice, behavior, and production logs.

## Optional AI Provider

1. Call `Show AI NPC Provider Settings` from the host pause or settings menu.
2. Let the player enter a provider name, complete Chat Completions URL, model, API key, and transmission consent.
3. Add `AI NPC Dialogue Component` to each NPC that needs free-form roleplay.
4. Configure a stable NPC ID, public character description, knowledge component, candidate packs, context collectors, guard policy, and semantic review component or policy.
5. Use HTTPS for remote endpoints. Permit HTTP only for explicitly allowed loopback addresses used by local services.

Credentials remain in runtime memory and are not persisted by the plugin. Production dedicated servers should use server-owned provider credentials rather than client BYOK.

## Release Checklist

- [ ] The plugin archive contains exactly one top-level plugin folder.
- [ ] `MyAiTownKnowledgeGrant.uplugin` is directly inside that folder.
- [ ] The archive contains no `.git`, `Binaries`, `Build`, `Intermediate`, or `Saved` directory.
- [ ] Every archive path, measured from the top-level plugin folder, is 170 characters or fewer.
- [ ] English documentation is included and linked from the Chinese documentation.
- [ ] Every Pack, Document, and Fact ID is unique.
- [ ] Prerequisite chains contain no missing references or cycles.
- [ ] Future events and items are stored in later packs, not exposed in an initial pack.
- [ ] Every production model outlet uses the mechanical guard.
- [ ] Free-form dialogue uses independent semantic review and revalidates live context after the response.
- [ ] Quest Blueprints cannot supply arbitrary player IDs, packs, or grant event IDs.
- [ ] `On Grant Audited` is connected to the host persistence or outbox layer when required.
- [ ] Multiplayer projects perform all grants and restores on server authority.
- [ ] SaveGame data uses snapshot schema v3 and a stable Pack Registry Version.
- [ ] Plaintext client assets contain no story secrets that must resist extraction.
- [ ] The target engine and platform pass compile, Automation, Cook, Package, and project multiplayer acceptance.

## Known Boundaries

- The mechanical guard checks evidence references and metadata consistency; it does not prove open-domain semantic entailment.
- Bypassing the guard or publishing raw model text removes the plugin's leakage protections.
- Snapshot schema v1/v2 is not automatically migrated to v3.
- Grant and reaction history has no built-in rolling compaction. Long-running projects must set capacity and persistence policies.
- Runtime authorization does not prevent extraction of plaintext Data Assets cooked into a client build.
- The plugin does not implement a cross-server ledger, server migration, server-side content storage, or global persistent NPC identity.

See [README_EN.md](README_EN.md) for the complete English overview and [Docs/VERIFICATION.md](Docs/VERIFICATION.md) for validated and unverified boundaries.
