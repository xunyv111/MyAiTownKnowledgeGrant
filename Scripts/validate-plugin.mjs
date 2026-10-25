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
  "Source/MyAiTownKnowledgeGrant/Public/KnowledgeSemanticReviewComponent.h",
  "Source/MyAiTownKnowledgeGrant/Public/ArpgKnowledgeGrantAdapterComponent.h",
  "Source/MyAiTownKnowledgeGrant/Public/AiNpcProviderTypes.h",
  "Source/MyAiTownKnowledgeGrant/Public/AiNpcProviderSettings.h",
  "Source/MyAiTownKnowledgeGrant/Public/AiNpcProviderSubsystem.h",
  "Source/MyAiTownKnowledgeGrant/Public/AiNpcProviderSettingsWidget.h",
  "Source/MyAiTownKnowledgeGrant/Public/AiNpcProviderUiLibrary.h",
  "Source/MyAiTownKnowledgeGrant/Public/AiNpcDialogueComponent.h",
  "Source/MyAiTownKnowledgeGrant/Public/KnowledgeResolverLibrary.h",
  "Source/MyAiTownKnowledgeGrant/Public/KnowledgeReaction.h",
  "Source/MyAiTownKnowledgeGrant/Public/NpcKnowledgeReceiverComponent.h",
  "Source/MyAiTownKnowledgeGrant/Private/KnowledgePack.cpp",
  "Source/MyAiTownKnowledgeGrant/Private/KnowledgeGrantComponent.cpp",
  "Source/MyAiTownKnowledgeGrant/Private/KnowledgeDialogueGuardLibrary.cpp",
  "Source/MyAiTownKnowledgeGrant/Private/KnowledgeModelAdapterLibrary.cpp",
  "Source/MyAiTownKnowledgeGrant/Private/KnowledgeContextTags.cpp",
  "Source/MyAiTownKnowledgeGrant/Private/KnowledgeContextCollectorComponent.cpp",
  "Source/MyAiTownKnowledgeGrant/Private/KnowledgeSemanticReviewComponent.cpp",
  "Source/MyAiTownKnowledgeGrant/Private/ArpgKnowledgeGrantAdapterComponent.cpp",
  "Source/MyAiTownKnowledgeGrant/Private/AiNpcProviderSettings.cpp",
  "Source/MyAiTownKnowledgeGrant/Private/AiNpcProviderSubsystem.cpp",
  "Source/MyAiTownKnowledgeGrant/Private/AiNpcProviderSettingsWidget.cpp",
  "Source/MyAiTownKnowledgeGrant/Private/AiNpcProviderUiLibrary.cpp",
  "Source/MyAiTownKnowledgeGrant/Private/AiNpcDialogueComponent.cpp",
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
  if (descriptor.Version !== 6 || descriptor.VersionName !== "0.4.2") errors.push("plugin descriptor must be version 0.4.2");
} catch (error) {
  errors.push(`plugin descriptor is invalid JSON: ${error.message}`);
}

if (fs.existsSync(path.join(root, "LICENSE"))) {
  const license = read("LICENSE");
  if (!license.includes("MIT License") || !license.includes("Permission is hereby granted, free of charge") || !license.includes("Copyright (c) 2026 xunyv")) {
    errors.push("LICENSE must contain the MIT License text and xunyv copyright notice");
  }
}

if (fs.existsSync(path.join(root, "NOTICE"))) {
  const notice = read("NOTICE");
  if (!notice.includes("Copyright 2026 xunyv")) errors.push("NOTICE must identify xunyv as copyright holder");
}

const buildSource = "Source/MyAiTownKnowledgeGrant/MyAiTownKnowledgeGrant.Build.cs";
if (fs.existsSync(path.join(root, buildSource))) {
  const build = read(buildSource);
  for (const dependency of ["Core", "CoreUObject", "Engine", "GameplayTags", "DeveloperSettings", "UMG", "HTTP", "Json", "Slate", "SlateCore"]) {
    if (!build.includes(`"${dependency}"`)) errors.push(`missing build dependency: ${dependency}`);
  }
  for (const forbidden of ["SQLite", "AIModule"]) {
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

const arpgAdapterSource = "Source/MyAiTownKnowledgeGrant/Private/ArpgKnowledgeGrantAdapterComponent.cpp";
if (fs.existsSync(path.join(root, arpgAdapterSource))) {
  const adapter = read(arpgAdapterSource);
  for (const marker of ["ResolveCompletedQuest", "arpg_registry_pack_mismatch", "debug_quest_event_rejected", "quest_not_completed", "stable_player_id_invalid", "quest_completion_revision_stale", "BuildGrantEventId", "OnGrantAudited"]) {
    if (!adapter.includes(marker)) errors.push(`ARPG adapter is missing invariant: ${marker}`);
  }
}

const semanticReviewSource = "Source/MyAiTownKnowledgeGrant/Private/KnowledgeSemanticReviewComponent.cpp";
if (fs.existsSync(path.join(root, semanticReviewSource))) {
  const semantic = read(semanticReviewSource);
  for (const marker of ["BeginSemanticReview", "CompleteSemanticReview", "ValidatedScopeFingerprint", "semantic_review_timeout", "semantic_review_coverage_invalid", "ValidateDialogueKnowledgeView", "FinalApprovedText", "OnSemanticReviewRequested", "OnSemanticReviewCompleted"]) {
    if (!semantic.includes(marker)) errors.push(`semantic review coordinator is missing invariant: ${marker}`);
  }
  for (const forbidden of ["OpenAI", "Anthropic", "Authorization: Bearer", "FHttpModule"]) {
    if (semantic.includes(forbidden)) errors.push(`semantic review coordinator contains provider coupling: ${forbidden}`);
  }
}

const providerSource = "Source/MyAiTownKnowledgeGrant/Private/AiNpcProviderSubsystem.cpp";
if (fs.existsSync(path.join(root, providerSource))) {
  const provider = read(providerSource);
  for (const marker of ["provider_transmission_consent_required", "provider_https_required", "OpenNpcSession", "RequestNpcDialogue", "RequestSemanticReview", "Authorization", "BuildDialogueRequestBody", "ParseDialogueResponse", "BuildReviewRequestBody", "ParseReviewResponse"]) {
    if (!provider.includes(marker)) errors.push(`AI NPC provider is missing invariant: ${marker}`);
  }
  for (const forbidden of ["SaveConfig", "GConfig", "ApiKey=" , "UE_LOG"]) {
    if (provider.includes(forbidden)) errors.push(`AI NPC provider may persist or log credentials: ${forbidden}`);
  }
}

const npcDialogueSource = "Source/MyAiTownKnowledgeGrant/Private/AiNpcDialogueComponent.cpp";
if (fs.existsSync(path.join(root, npcDialogueSource))) {
  const dialogue = read(npcDialogueSource);
  for (const marker of ["OpenNpcSession", "BuildDialogueKnowledgeView", "GuardKnowledgeModelJsonResponseForCollectors", "BeginSemanticReview", "FinalApprovedText", "LocalTranscript"]) {
    if (!dialogue.includes(marker)) errors.push(`AI NPC dialogue component is missing invariant: ${marker}`);
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
  for (const marker of ["A mechanical result from another dialogue view cannot enter semantic review", "A provider-neutral semantic request is created", "A complete external approval releases FinalApprovedText", "An API response is rejected after authorization changes while in flight", "A semantic rejection fails closed to the trusted policy text"]) {
    if (!tests.includes(marker)) errors.push(`semantic review coverage marker missing: ${marker}`);
  }
  for (const marker of ["Remote transmission requires explicit player consent", "Plain HTTP to a remote vendor is rejected", "No API key field exists in the serialized body", "The provider request is deterministic and contains only current-turn inputs", "The reviewer response accepts only the structured decision contract"]) {
    if (!tests.includes(marker)) errors.push(`AI NPC provider coverage marker missing: ${marker}`);
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
console.log("Scope: local-BYOK AI NPC provider, isolated NPC sessions, ARPG quest adapter, strict JSON and fact guards, semantic review, and test coverage markers.");
console.log("This does not replace UnrealHeaderTool, C++ compilation, Automation Test, replication PIE, or real-server acceptance.");
