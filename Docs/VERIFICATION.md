# Verification Summary

This file is the public, machine-independent summary of the 0.4.3 validation. Raw automation reports and logs are not distributed in this repository because they contain machine paths and host metadata. The publisher retains the local evidence bundle separately.

## Build

- Unreal Engine 5.8.4, Win64.
- `RunUAT BuildPlugin` completed `UnrealEditor Win64 Development`, `UnrealGame Win64 Development`, and `UnrealGame Win64 Shipping`.
- UnrealHeaderTool ran with `-WarningsAsErrors`.
- AutomationTool exited with code 0.

## Automation

| Suite | Result |
|---|---|
| Plugin (`MyAiTown.KnowledgeGrant`) | 20 succeeded, 0 warnings, 0 failed |

Strict UE 5.8 totals: 20 cases executed, 20 clean successes, 0 failures, 0 not run.

The merge branch adds eight focused suites: `QuestAuthorityBlueprintDispatch`, `SemanticRequestCorrelation`, `RequestIdempotency`, `StaleResponseRejection`, `MultiProviderRouting`, `DialogueFormatRepair`, `DeterministicAuthorizedFallback`, and `SemanticClaimGates`.

The ARPG host, demo, multiplayer PIE, and offscreen-level results below are retained as a UE 5.7.4 historical integration baseline. They were not rerun as part of this UE 5.8 plugin-package verification.

## Real-level offscreen acceptance

- Map: `/Game/Maps/KnowledgeAcceptance`
- Mode: `UnrealEditor-Cmd -game -RenderOffscreen`
- Result: 8/8 checks passed, process exit code 0.
- Checks covered saved map actors, pre-quest lock, scene enter/exit, interaction enter/exit, future-knowledge blocking, causal unlock, and NPC reactions.

This is an offscreen automation result, not a manual Play or retail Cook/Package certification.

## Not covered

- Manual keyboard/mouse play.
- Target-project Cook/Package and deployment.
- A saved formal multiplayer level or an external dedicated-server process.
- 10k NPC / 1k pack scale benchmark.
- A real external semantic-review provider, its authentication, availability, cost, and model quality.
- Mathematical proof of full open-domain semantic entailment.
