# Verification Summary

This file is the public, machine-independent summary of the 0.2.1 validation. Raw automation reports and logs are not distributed in this repository because they contain machine paths and host metadata. The release archive includes a local evidence bundle for the publisher.

## Build

- Unreal Engine 5.7.4, Win64.
- `RunUAT BuildPlugin` completed `UnrealEditor Win64 Development`, `UnrealGame Win64 Development`, and `UnrealGame Win64 Shipping`.
- UnrealHeaderTool ran with `-WarningsAsErrors`.
- AutomationTool exited with code 0.

## Automation

| Suite | Result |
|---|---|
| Plugin (`MyAiTown.KnowledgeGrant`) | 10 succeeded, 0 warnings, 0 failed |
| Demo (`KnowledgeGrantHost.Demo`) | 2 succeeded, 0 warnings, 0 failed |
| Network PIE (`KnowledgeGrantHost.Network`) | 1 succeeded, 36 UE NetGUID framework warnings, 0 business errors |

Strict totals: 13 cases executed, 12 clean successes, 1 success with framework warnings, 0 failures, 0 not run.

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
- Full open-domain semantic entailment.
