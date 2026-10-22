#!/usr/bin/env node

import fs from "node:fs";
import path from "node:path";
import process from "node:process";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const errors = [];
const read = (relative) => fs.readFileSync(path.join(root, relative), "utf8");
const required = [
  "LICENSE",
  "NOTICE",
  "MyAiTownKnowledgeGrant.uplugin",
  "Source/MyAiTownKnowledgeGrant/MyAiTownKnowledgeGrant.Build.cs",
  "Source/MyAiTownKnowledgeGrant/Public/KnowledgePack.h",
  "Source/MyAiTownKnowledgeGrant/Public/KnowledgeGrantComponent.h",
  "Source/MyAiTownKnowledgeGrant/Public/KnowledgeDialogueGuardLibrary.h",
  "Source/MyAiTownKnowledgeGrant/Public/KnowledgeModelAdapterLibrary.h",
  "Source/MyAiTownKnowledgeGrant/Public/KnowledgeContextTags.h",
  "Source/MyAiTownKnowledgeGrant/Public/KnowledgeContextCollectorComponent.h",
  "Source/MyAiTownKnowledgeGrant/Public/KnowledgeResolverLibrary.h",
  "Source/MyAiTownKnowledgeGrant/Public/KnowledgeReaction.h",
  "Source/MyAiTownKnowledgeGrant/Public/NpcKnowledgeReceiverComponent.h",
  "Source/MyAiTownKnowledgeGrant/Private/KnowledgePack.cpp",
  "Source/MyAiTownKnowledgeGrant/Private/KnowledgeGrantComponent.cpp",
  "Source/MyAiTownKnowledgeGrant/Private/KnowledgeDialogueGuardLibrary.cpp",
  "Source/MyAiTownKnowledgeGrant/Private/KnowledgeModelAdapterLibrary.cpp",
  "Source/MyAiTownKnowledgeGrant/Private/KnowledgeContextTags.cpp",
  "Source/MyAiTownKnowledgeGrant/Private/KnowledgeContextCollectorComponent.cpp",
  "Source/MyAiTownKnowledgeGrant/Private/KnowledgeResolverLibrary.cpp",
  "Source/MyAiTownKnowledgeGrant/Private/KnowledgeReaction.cpp",
  "Source/MyAiTownKnowledgeGrant/Private/NpcKnowledgeReceiverComponent.cpp",
  "Source/MyAiTownKnowledgeGrant/Private/Tests/KnowledgeGrantTests.cpp",
];

for (const relative of required) {
  if (!fs.existsSync(path.join(root, relative))) errors.push(`missing required file: ${relative}`);
}

try {
  const descriptor = JSON.parse(read("MyAiTownKnowledgeGrant.uplugin"));
  if (descriptor.Modules?.length !== 1 || descriptor.Modules[0]?.Name !== "MyAiTownKnowledgeGrant") {
    errors.push("plugin descriptor must declare exactly the MyAiTownKnowledgeGrant module");
  }
  if (descriptor.Modules?.[0]?.Type !== "Runtime") errors.push("plugin module must be Runtime");
  if (descriptor.CreatedBy !== "xunyv") errors.push("plugin descriptor author must be xunyv");
  if (descriptor.Version !== 2 || descriptor.VersionName !== "0.2.1") errors.push("plugin descriptor must be version 0.2.1");
} catch (error) {
  errors.push(`plugin descriptor is invalid JSON: ${error.message}`);
}

if (fs.existsSync(path.join(root, "LICENSE"))) {
  const license = read("LICENSE");
  if (!license.includes("Apache License") || !license.includes("Version 2.0, January 2004")) {
    errors.push("LICENSE must contain the Apache License 2.0 text");
  }
}

if (fs.existsSync(path.join(root, "NOTICE"))) {
  const notice = read("NOTICE");
  if (!notice.includes("Copyright 2026 xunyv")) errors.push("NOTICE must identify xunyv as copyright holder");
}

const buildSource = "Source/MyAiTownKnowledgeGrant/MyAiTownKnowledgeGrant.Build.cs";
if (fs.existsSync(path.join(root, buildSource))) {
  const build = read(buildSource);
  for (const dependency of ["Core", "CoreUObject", "Engine", "GameplayTags", "Json"]) {
    if (!build.includes(`"${dependency}"`)) errors.push(`missing build dependency: ${dependency}`);
  }
  for (const forbidden of ["OpenAI", "HTTP", "SQLite", "AIModule"]) {
    if (build.includes(`"${forbidden}"`)) errors.push(`first version contains forbidden dependency: ${forbidden}`);
  }
}

const modelAdapterSource = "Source/MyAiTownKnowledgeGrant/Private/KnowledgeModelAdapterLibrary.cpp";
if (fs.existsSync(path.join(root, modelAdapterSource))) {
  const adapter = read(modelAdapterSource);
  for (const marker of ["GuardKnowledgeModelJsonResponse", "MaxPayloadCharacters", "HasExactFields", "model_json_malformed", "model_json_schema_invalid", "model_json_evidence_invalid", "model_json_claim_text_invalid", "GuardDialogueKnowledgeView"]) {
    if (!adapter.includes(marker)) errors.push(`model adapter is missing invariant: ${marker}`);
  }
  for (const forbidden of ["Http", "OpenAI", "RawText", "bKnowledgeBearing"]) {
    if (adapter.includes(forbidden)) errors.push(`model adapter contains forbidden path: ${forbidden}`);
  }
}

const contextCollectorSource = "Source/MyAiTownKnowledgeGrant/Private/KnowledgeContextCollectorComponent.cpp";
if (fs.existsSync(path.join(root, contextCollectorSource))) {
  const collector = read(contextCollectorSource);
  for (const marker of ["EnterSceneContext", "ExitSceneContext", "SetInteractionContext", "ClearInteractionContext", "SetExplicitMentionContext", "GetActiveContextTags", "ResetAllContexts", "InteractionSource.IsValid"]) {
    if (!collector.includes(marker)) errors.push(`context collector is missing invariant: ${marker}`);
  }
  for (const forbidden of ["GrantKnowledgeFromQuest", "GrantKnowledgeFromWorldEvent", "BuildDialogueKnowledgeView"]) {
    if (collector.includes(forbidden)) errors.push(`context collector crosses its selection-only boundary: ${forbidden}`);
  }
}

const grantComponentSource = "Source/MyAiTownKnowledgeGrant/Private/KnowledgeGrantComponent.cpp";
if (fs.existsSync(path.join(root, grantComponentSource))) {
  const component = read(grantComponentSource);
  for (const marker of ["HasAuthorityForMutation", "ProcessedGrants", "grant_event_id_conflict", "AuthorizationEpoch = FGuid::NewGuid", "quest_pack_mismatch", "world_event_pack_mismatch", "knowledge_prerequisite_missing", "DOREPLIFETIME", "ValidateSnapshotAgainstRegistry", "snapshot_schema_unsupported", "snapshot_registry_version_mismatch", "snapshot_pack_unregistered", "snapshot_prerequisite_missing", "snapshot_prerequisite_cycle", "snapshot_granted_tags_mismatch", "snapshot_grant_history_incomplete", "snapshot_grant_pack_mismatch", "HasAllExact"]) {
    if (!component.includes(marker)) errors.push(`grant component is missing invariant: ${marker}`);
  }
}

const resolverSource = "Source/MyAiTownKnowledgeGrant/Private/KnowledgeResolverLibrary.cpp";
if (fs.existsSync(path.join(root, resolverSource))) {
  const resolver = read(resolverSource);
  for (const marker of ["HasAuthority", "HasKnowledgePack", "DocumentIds.Contains", "ActivationTags.IsEmpty", "HasAny", "BuildDialogueKnowledgeView", "ComputeScopeFingerprint", "AuthorizationEpoch", "CurrentActiveContextTags", "dialogue_view_stale", "dialogue_view_authorization_changed", "dialogue_view_context_changed", "dialogue_view_tampered"]) {
    if (!resolver.includes(marker)) errors.push(`resolver is missing invariant: ${marker}`);
  }
}

const dialogueGuardSource = "Source/MyAiTownKnowledgeGrant/Private/KnowledgeDialogueGuardLibrary.cpp";
if (fs.existsSync(path.join(root, dialogueGuardSource))) {
  const guard = read(dialogueGuardSource);
  for (const marker of ["GuardDialogueKnowledgeView", "EvidenceFactIds.IsEmpty", "dialogue_claim_fact_not_grounded", "dialogue_claim_entity_mismatch", "dialogue_claim_number_mismatch", "dialogue_claim_negation_mismatch", "AllOrNothing", "PolicyText", "ExtractNumericTokens", "FilterMachineNumericTokens", "PartiallyAllowed", "dialogue_claims_empty"]) {
    if (!guard.includes(marker)) errors.push(`dialogue guard is missing invariant: ${marker}`);
  }
  for (const forbidden of ["bKnowledgeBearing", "BlockedFallback", "EvidenceDocumentIds", "ResolveAllUnlockedKnowledge", "FilterGroundedDialogueForContextTags"]) {
    if (guard.includes(forbidden)) errors.push(`dialogue guard retains unsafe compatibility path: ${forbidden}`);
  }
}

const testSource = "Source/MyAiTownKnowledgeGrant/Private/Tests/KnowledgeGrantTests.cpp";
if (fs.existsSync(path.join(root, testSource))) {
  const tests = read(testSource);
  for (const marker of ["Locked documents never enter", "Another player remains locked", "Repeated completion is idempotent", "Grant event identifiers are normalized", "Reusing one grant event identifier for a different pack fails explicitly", "An invalid but ungranted candidate does not fail", "Knowledge survives save and restore", "A different quest cannot grant", "A registry-matched snapshot restores successfully", "Restore derives both unlock tags from registered packs", "A cross-version snapshot is rejected", "Forged snapshot tags are rejected instead of trusted", "A snapshot missing its prerequisite closure is rejected", "An unregistered snapshot pack is rejected", "A cyclic prerequisite registry is rejected during restore", "Every failed restore preserves the pre-existing granted pack", "A granted pack without grant history is rejected", "Grant history cannot refer to a pack outside the authorized state", "Restore advances revision even when the snapshot carries the current revision", "A pre-restore view is rejected", "A later domino cannot fall before its prerequisite", "Rejected future event leaks no future document", "Unsupported factual claim is blocked after generation", "Every model claim without evidence fails closed", "The same claim is allowed only after its evidence unlocks", "A proper-noun mismatch is blocked mechanically", "A protected proper noun omitted from claim metadata is blocked", "A shorter protected entity occurring outside its declared longer token is blocked", "A numeric mismatch is blocked mechanically", "A numeric sign reversal is blocked", "A decimal is treated as one numeric token", "A host-declared Chinese written number is accepted", "The positive word bucuo is not treated", "A negation mismatch is blocked mechanically", "Malformed JSON fails closed", "Unknown rawText never reaches SafeText", "Empty FactId evidence is rejected by the adapter contract", "Duplicate FactId evidence is rejected", "A protected entity omitted from parsed metadata still fails the fact guard", "Overlong claim text is rejected before the fact guard", "Overlapping scene sources are unioned", "Updating one scene source replaces its previous tags", "A stale interaction end cannot clear the newer target", "Explicit proper-noun context is added", "Reset removes every context source", "North gate marker activates its information library", "Output guard blocks a fact from an inactive scene library", "A view from the previous scene is rejected", "Matching scene marker never grants an unobserved future fact", "All-or-nothing policy removes otherwise valid claims", "A view is rejected after player knowledge revision changes", "A view with a modified consistency checksum is rejected"]) {
    if (!tests.includes(marker)) errors.push(`automation coverage marker missing: ${marker}`);
  }
}

const reactionSource = "Source/MyAiTownKnowledgeGrant/Private/KnowledgeReaction.cpp";
if (fs.existsSync(path.join(root, reactionSource))) {
  const reaction = read(reactionSource);
  for (const marker of ["EvaluateKnowledgeReaction", "Personality->Curiosity", "DialogueTone", "SuggestedValenceDelta"]) {
    if (!reaction.includes(marker)) errors.push(`reaction evaluator is missing invariant: ${marker}`);
  }
}

const receiverSource = "Source/MyAiTownKnowledgeGrant/Private/NpcKnowledgeReceiverComponent.cpp";
if (fs.existsSync(path.join(root, receiverSource))) {
  const receiver = read(receiverSource);
  for (const marker of ["audience_id_invalid", "NormalizedAudienceId", "receipt_event_id_conflict", "HasReceivedEventForAudience", "AudienceReceiptIds", "reaction_snapshot_reaction_invalid", "ValidTones", "ValidIntents", "knowledge_pack_locked", "bDuplicate", "EvaluateKnowledgeReaction", "CreateReactionSnapshot"]) {
    if (!receiver.includes(marker)) errors.push(`delayed receiver is missing invariant: ${marker}`);
  }
}

if (fs.existsSync(path.join(root, testSource))) {
  const tests = read(testSource);
  for (const marker of ["The same fact produces different dialogue tones", "Current tension raises reaction intensity", "Locked knowledge cannot trigger", "An empty audience key is rejected", "A receiver without an authoritative owner rejects mutation", "The same NPC records the same event independently for another audience", "Repeated delivery remains idempotent inside one audience", "Audience and receipt identifiers are normalized", "One audience receipt event cannot silently change packs", "Out-of-range reaction values in a snapshot are rejected", "Legacy unscoped reaction snapshots fail closed"]) {
    if (!tests.includes(marker)) errors.push(`reaction coverage marker missing: ${marker}`);
  }
}

if (errors.length) {
  for (const error of errors) console.error(`ERROR: ${error}`);
  process.exit(1);
}

console.log(`Plugin structure valid: ${root}`);
console.log("Scope: descriptor, dependency boundary, authority/idempotence/replication markers, registry-validated snapshots, context collector, unified knowledge view, strict JSON adapter, fact guard, and test coverage markers.");
console.log("This does not replace UnrealHeaderTool, C++ compilation, Automation Test, replication PIE, or real-server acceptance.");
