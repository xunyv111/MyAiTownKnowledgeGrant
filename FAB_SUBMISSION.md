# Fab Submission Checklist

Status: **ready for source-only resubmission**.

## Current candidate

- Engine: Unreal Engine 5.8.4
- Supported target platform: Win64
- Package type: source code plugin
- Plugin version: 0.4.4 (Beta)
- Snapshot schema: 3
- Repository license: MIT License
- Fab archive root: `MyAiTownKnowledgeGrant/`

## Technical review corrections

- The Fab archive contains complete human-readable C++ source and no generated `Binaries`, `Build`, `Intermediate`, or `Saved` directory.
- The archive contains one top-level plugin folder.
- Every archive path is 170 characters or fewer.
- Buyer-facing documentation in the Fab archive is English.
- The repository retains additional Chinese and internal design documentation, but those files are not part of the Fab review archive.
- `MyAiTownKnowledgeGrant.uplugin` declares Unreal Engine 5.8 and a Win64 `PlatformAllowList`.
- All C++ source and header files contain the publisher copyright notice.

## Validation evidence

- UnrealHeaderTool completed with warnings treated as errors.
- UnrealEditor Win64 Development completed.
- UnrealGame Win64 Development and Shipping completed.
- Plugin Automation completed 20 of 20 cases with 0 warnings and 0 failures on Unreal Engine 5.8.4 using the 0.4.4 build.
- `Scripts/validate-plugin.mjs` validates the source structure and guarded runtime boundary markers.

The 0.4.4 source was rebuilt with `RunUAT BuildPlugin` in a short ASCII path. UnrealHeaderTool, Editor Development, Game Development, and Game Shipping all succeeded. The generated plugin was then loaded by a minimal Unreal Engine 5.8.4 host and its 20 automation cases passed with no warnings or failures.

## Listing boundaries

Describe the plugin as a runtime knowledge-authorization, quest-adapter, guarded AI-NPC provider, and semantic-review contract for single-player or networked ARPG projects.

Do not market it as a generic RPG framework, cross-server backend, vector database, model SDK, absolute truth verifier, or protection against extraction of plaintext client assets.

Player BYOK is intended for single-player games and local listen servers. A production dedicated server should use server-owned provider credentials and server-controlled privacy, cost, prompt, and audit policies.

## Final publisher checks

- Confirm the Fab listing license and support contact.
- Confirm listing text identifies Unreal Engine 5.8 and Win64 only.
- Upload store media that shows the actual plugin workflow.
- Confirm no API keys, tokens, logs, crash dumps, host projects, or machine paths are present.
- Retain the final archive checksum with the submission record.
