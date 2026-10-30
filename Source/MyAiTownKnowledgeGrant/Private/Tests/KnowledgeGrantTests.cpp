#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITORONLY_DATA

#include "GameFramework/Actor.h"
#include "ArpgKnowledgeGrantAdapterComponent.h"
#include "AiNpcDialogueComponent.h"
#include "AiNpcProviderSubsystem.h"
#include "KnowledgeGrantComponent.h"
#include "KnowledgeDialogueGuardLibrary.h"
#include "KnowledgeModelAdapterLibrary.h"
#include "KnowledgeContextCollectorComponent.h"
#include "KnowledgeContextTags.h"
#include "KnowledgePack.h"
#include "KnowledgeReaction.h"
#include "KnowledgeResolverLibrary.h"
#include "KnowledgeSemanticReviewComponent.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "NpcKnowledgeReceiverComponent.h"
#include "Tests/KnowledgeGrantTestTypes.h"

namespace
{
const FName TestPackRegistryVersion(TEXT("test.knowledge.registry.v1"));

FName FactIdForDocument(FName DocumentId)
{
    return FName(*FString::Printf(TEXT("fact.%s"), *DocumentId.ToString()));
}

void AddDefaultFact(FKnowledgeDocument& Document)
{
    FKnowledgeFact Fact;
    Fact.FactId = FactIdForDocument(Document.DocumentId);
    Fact.Statement = Document.Content;
    Document.Facts.Add(Fact);
}

UKnowledgePack* MakeTestPack()
{
    UKnowledgePack* Pack = NewObject<UKnowledgePack>();
    Pack->PackId = TEXT("quest.ancient_dragon.truth");
    Pack->SourceQuestId = TEXT("quest.ancient_dragon");
    FKnowledgeDocument Origin;
    Origin.DocumentId = TEXT("dragon_origin");
    Origin.Content = TEXT("The ancient dragon was created by the first court mage.");
    Origin.ThreatLevel = 100;
    Origin.Novelty = 80;
    Origin.MoralViolation = 90;
    Origin.OrderImpact = -80;
    AddDefaultFact(Origin);
    Pack->Documents.Add(Origin);
    FKnowledgeDocument Temple;
    Temple.DocumentId = TEXT("ruined_temple_location");
    Temple.Content = TEXT("The ruined temple lies beyond the northern pass.");
    Temple.ThreatLevel = 15;
    Temple.Novelty = 90;
    AddDefaultFact(Temple);
    Pack->Documents.Add(Temple);
    return Pack;
}

UKnowledgePack* MakeWorldEventPack(FName PackId, FName WorldEventId, FName DocumentId,
    const TCHAR* Content, const TArray<FName>& RequiredPackIds = {})
{
    UKnowledgePack* Pack = NewObject<UKnowledgePack>();
    Pack->PackId = PackId;
    Pack->SourceWorldEventId = WorldEventId;
    Pack->RequiredPackIds = RequiredPackIds;
    FKnowledgeDocument Document;
    Document.DocumentId = DocumentId;
    Document.Content = Content;
    AddDefaultFact(Document);
    Pack->Documents.Add(Document);
    return Pack;
}

UKnowledgeGrantComponent* MakeGrantComponent()
{
    AActor* Owner = NewObject<AActor>();
    return NewObject<UKnowledgeGrantComponent>(Owner);
}

UNpcPersonalityProfile* MakeAggressivePersonality()
{
    UNpcPersonalityProfile* Profile = NewObject<UNpcPersonalityProfile>();
    Profile->Empathy = 20;
    Profile->Curiosity = 40;
    Profile->Caution = 20;
    Profile->OrderLoyalty = 80;
    Profile->Aggression = 90;
    return Profile;
}

UNpcPersonalityProfile* MakeEmpathicPersonality()
{
    UNpcPersonalityProfile* Profile = NewObject<UNpcPersonalityProfile>();
    Profile->Empathy = 90;
    Profile->Curiosity = 80;
    Profile->Caution = 75;
    Profile->OrderLoyalty = 20;
    Profile->Aggression = 10;
    return Profile;
}

UNpcKnowledgeReceiverComponent* MakeReceiver(UNpcPersonalityProfile* Profile)
{
    AActor* Owner = NewObject<AActor>();
    UNpcKnowledgeReceiverComponent* Receiver = NewObject<UNpcKnowledgeReceiverComponent>(Owner);
    Receiver->PersonalityProfile = Profile;
    return Receiver;
}

UKnowledgeDialogueGuardPolicy* MakeGuardPolicy(EKnowledgeDialogueGuardMode Mode = EKnowledgeDialogueGuardMode::AllOrNothing)
{
    UKnowledgeDialogueGuardPolicy* Policy = NewObject<UKnowledgeDialogueGuardPolicy>();
    Policy->Mode = Mode;
    Policy->InsufficientEvidenceText = FText::FromString(TEXT("现有证据不足，无法断言。"));
    Policy->InvalidViewText = FText::FromString(TEXT("当前知识视图无效。"));
    Policy->MalformedOutputText = FText::FromString(TEXT("回答格式无效。"));
    Policy->ProtectedEntityLexicon = {TEXT("王廷"), TEXT("王廷秘印"), TEXT("议会"), TEXT("议会秘印"), TEXT("巨龙")};
    return Policy;
}

FKnowledgeSemanticClaimAssessment MakePassingAssessment(
    FName ClaimId, const TArray<FName>& EvidenceFactIds)
{
    FKnowledgeSemanticClaimAssessment Assessment;
    Assessment.ClaimId = ClaimId;
    Assessment.ClaimType = EvidenceFactIds.IsEmpty()
        ? EKnowledgeClaimType::NonFactual : EKnowledgeClaimType::Factual;
    Assessment.Support = EKnowledgeClaimSupport::Supported;
    Assessment.Contradiction = EKnowledgeClaimRisk::None;
    Assessment.Overreach = EKnowledgeClaimRisk::None;
    Assessment.TaskLeak = EKnowledgeClaimRisk::None;
    Assessment.RoleFit = EKnowledgeRoleFit::Good;
    Assessment.Confidence = EKnowledgeReviewConfidence::High;
    Assessment.EvidenceFactIds = EvidenceFactIds;
    Assessment.FactSupportScore = 0.95f;
    Assessment.ContradictionRiskScore = 0.02f;
    Assessment.OverreachRiskScore = 0.02f;
    Assessment.TaskLeakRiskScore = 0.01f;
    Assessment.RoleFitScore = 0.90f;
    Assessment.ConfidenceScore = 0.95f;
    return Assessment;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKnowledgeContextTagLibrariesTest,
    "MyAiTown.KnowledgeGrant.Runtime.ContextTagLibraries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnowledgeContextTagLibrariesTest::RunTest(const FString& Parameters)
{
    const FGameplayTag NorthGateTag = KnowledgeContextTags::SceneNorthGate;
    const FGameplayTag TavernTag = KnowledgeContextTags::SceneTavern;
    TestTrue(TEXT("Example scene tags are registered"), NorthGateTag.IsValid() && TavernTag.IsValid());

    UKnowledgePack* NorthGate = MakeWorldEventPack(
        TEXT("library.north_gate"), TEXT("event.north_gate.observed"), TEXT("north_gate_watch"),
        TEXT("The north gate watch was doubled."));
    NorthGate->ActivationTags.AddTag(NorthGateTag);
    UKnowledgePack* Tavern = MakeWorldEventPack(
        TEXT("library.tavern"), TEXT("event.tavern.heard"), TEXT("tavern_rumor"),
        TEXT("The innkeeper heard a caravan rumor."));
    Tavern->ActivationTags.AddTag(TavernTag);
    UKnowledgePack* LockedNorthSecret = MakeWorldEventPack(
        TEXT("library.north_gate.secret"), TEXT("event.north_gate.secret_found"), TEXT("north_gate_secret"),
        TEXT("A hidden tunnel lies below the gate."), {NorthGate->PackId});
    LockedNorthSecret->ActivationTags.AddTag(NorthGateTag);
    TArray<UKnowledgePack*> Packs{NorthGate, Tavern, LockedNorthSecret};
    UKnowledgeGrantComponent* Player = MakeGrantComponent();
    TestTrue(TEXT("North gate library is granted by an observed event"), Player->GrantKnowledgeFromWorldEvent(
        TEXT("world:north-gate:1"), TEXT("event.north_gate.observed"), NorthGate).bOk);
    TestTrue(TEXT("Tavern library is granted by a separate observed event"), Player->GrantKnowledgeFromWorldEvent(
        TEXT("world:tavern:1"), TEXT("event.tavern.heard"), Tavern).bOk);

    FGameplayTagContainer ActiveTags;
    ActiveTags.AddTag(NorthGateTag);
    const FDialogueKnowledgeView AtNorthGate =
        UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(Player, Packs, ActiveTags);
    TestTrue(TEXT("North gate marker activates its information library"), AtNorthGate.bOk);
    TestEqual(TEXT("Wrong-scene and locked libraries remain absent"), AtNorthGate.AllowedDocuments.Num(), 1);
    TestEqual(TEXT("Resolved fact belongs to the active scene"),
        AtNorthGate.AllowedDocuments[0].DocumentId, FName(TEXT("north_gate_watch")));
    TestEqual(TEXT("View explains both exclusions"), AtNorthGate.ExcludedReasons.Num(), 2);

    FKnowledgeDialogueClaim TavernClaim;
    TavernClaim.ClaimId = TEXT("claim.tavern.rumor");
    TavernClaim.Text = TEXT("酒馆老板听到了商队传闻。");
    TavernClaim.EvidenceFactIds = {FactIdForDocument(TEXT("tavern_rumor"))};
    const FKnowledgeDialogueGuardResult WrongScene =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, AtNorthGate, ActiveTags, {TavernClaim}, MakeGuardPolicy());
    TestTrue(TEXT("Output guard blocks a fact from an inactive scene library"), WrongScene.bBlocked);

    ActiveTags.Reset();
    ActiveTags.AddTag(TavernTag);
    FKnowledgeDialogueClaim NorthGateClaim;
    NorthGateClaim.ClaimId = TEXT("claim.north_gate.watch");
    NorthGateClaim.Text = TEXT("北门守卫人数已经加倍。");
    NorthGateClaim.EvidenceFactIds = {FactIdForDocument(TEXT("north_gate_watch"))};
    const FKnowledgeDialogueGuardResult ReusedNorthGateView =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, AtNorthGate, ActiveTags, {NorthGateClaim}, MakeGuardPolicy());
    TestTrue(TEXT("A view from the previous scene is rejected after context changes"),
        !ReusedNorthGateView.bOk && ReusedNorthGateView.bBlocked
        && ReusedNorthGateView.Error == TEXT("dialogue_view_context_changed"));

    UKnowledgeContextCollectorComponent* Collector = NewObject<UKnowledgeContextCollectorComponent>();
    AActor* ContextSource = NewObject<AActor>();
    FGameplayTagContainer NorthGateContext;
    NorthGateContext.AddTag(NorthGateTag);
    FGameplayTagContainer TavernContext;
    TavernContext.AddTag(TavernTag);
    Collector->EnterSceneContext(ContextSource, NorthGateContext);
    TArray<UKnowledgeContextCollectorComponent*> Collectors{Collector};
    const FKnowledgeDialogueGuardResult CollectorAllowed =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeViewForCollectors(
            Player, AtNorthGate, Collectors, {NorthGateClaim}, MakeGuardPolicy());
    TestTrue(TEXT("Collector-derived live tags allow a matching view"),
        CollectorAllowed.bOk && !CollectorAllowed.bBlocked);

    Collector->EnterSceneContext(ContextSource, TavernContext);
    const FKnowledgeDialogueGuardResult CollectorStale =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeViewForCollectors(
            Player, AtNorthGate, Collectors, {NorthGateClaim}, MakeGuardPolicy());
    TestTrue(TEXT("Collector-derived live tags reject a stale view"),
        !CollectorStale.bOk && CollectorStale.Error == TEXT("dialogue_view_context_changed"));

    const FKnowledgeDialogueGuardResult MissingCollector =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeViewForCollectors(
            Player, AtNorthGate, {}, {NorthGateClaim}, MakeGuardPolicy());
    TestTrue(TEXT("A guard call without a live context collector fails closed"),
        !MissingCollector.bOk && MissingCollector.Error == TEXT("context_collector_missing"));

    const FDialogueKnowledgeView TavernView =
        UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(Player, Packs, ActiveTags);
    const FKnowledgeDialogueGuardResult AtTavern = UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
        Player, TavernView, ActiveTags, {TavernClaim}, MakeGuardPolicy());
    TestTrue(TEXT("Changing scene marker activates the other library"), TavernView.bOk && AtTavern.bOk && !AtTavern.bBlocked);
    TestEqual(TEXT("Grounded tavern statement is preserved"), AtTavern.SafeText, TavernClaim.Text);

    ActiveTags.Reset();
    ActiveTags.AddTag(NorthGateTag);
    const FDialogueKnowledgeView StillLocked =
        UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(Player, Packs, ActiveTags);
    TestEqual(TEXT("Matching scene marker never grants an unobserved future fact"), StillLocked.AllowedDocuments.Num(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKnowledgeContextCollectorTest,
    "MyAiTown.KnowledgeGrant.Runtime.ContextCollector",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnowledgeContextCollectorTest::RunTest(const FString& Parameters)
{
    UKnowledgeContextCollectorComponent* Collector = NewObject<UKnowledgeContextCollectorComponent>();
    AActor* NorthGateVolume = NewObject<AActor>();
    AActor* RoyalCourtVolume = NewObject<AActor>();
    AActor* TavernActor = NewObject<AActor>();
    AActor* BellActor = NewObject<AActor>();

    FGameplayTagContainer NorthGate;
    NorthGate.AddTag(KnowledgeContextTags::SceneNorthGate);
    FGameplayTagContainer RoyalCourt;
    RoyalCourt.AddTag(KnowledgeContextTags::EntityRoyalCourt);
    Collector->EnterSceneContext(NorthGateVolume, NorthGate);
    Collector->EnterSceneContext(RoyalCourtVolume, RoyalCourt);
    FGameplayTagContainer Active = Collector->GetActiveContextTags();
    TestTrue(TEXT("Overlapping scene sources are unioned"),
        Active.HasTagExact(KnowledgeContextTags::SceneNorthGate)
        && Active.HasTagExact(KnowledgeContextTags::EntityRoyalCourt));

    FGameplayTagContainer Tavern;
    Tavern.AddTag(KnowledgeContextTags::SceneTavern);
    Collector->EnterSceneContext(NorthGateVolume, Tavern);
    Active = Collector->GetActiveContextTags();
    TestTrue(TEXT("Updating one scene source replaces its previous tags"),
        Active.HasTagExact(KnowledgeContextTags::SceneTavern)
        && !Active.HasTagExact(KnowledgeContextTags::SceneNorthGate));
    Collector->ExitSceneContext(NorthGateVolume);
    Active = Collector->GetActiveContextTags();
    TestTrue(TEXT("Leaving one overlap preserves the other scene source"),
        Active.HasTagExact(KnowledgeContextTags::EntityRoyalCourt)
        && !Active.HasTagExact(KnowledgeContextTags::SceneTavern));

    Collector->SetInteractionContext(TavernActor, Tavern);
    FGameplayTagContainer Bell;
    Bell.AddTag(KnowledgeContextTags::ItemBronzeControlBell);
    Collector->SetInteractionContext(BellActor, Bell);
    Collector->ClearInteractionContext(TavernActor);
    Active = Collector->GetActiveContextTags();
    TestTrue(TEXT("A stale interaction end cannot clear the newer target"),
        Active.HasTagExact(KnowledgeContextTags::ItemBronzeControlBell)
        && !Active.HasTagExact(KnowledgeContextTags::SceneTavern));

    Collector->SetExplicitMentionContext(NorthGate);
    Active = Collector->GetActiveContextTags();
    TestTrue(TEXT("Explicit proper-noun context is added to scene and interaction context"),
        Active.HasTagExact(KnowledgeContextTags::SceneNorthGate)
        && Active.HasTagExact(KnowledgeContextTags::ItemBronzeControlBell)
        && Active.HasTagExact(KnowledgeContextTags::EntityRoyalCourt));
    Collector->ClearExplicitMentionContext();
    Collector->ClearInteractionContext(BellActor);
    Active = Collector->GetActiveContextTags();
    TestTrue(TEXT("Clearing transient sources retains only active scene overlaps"),
        Active.Num() == 1 && Active.HasTagExact(KnowledgeContextTags::EntityRoyalCourt));

    Collector->ResetAllContexts();
    TestTrue(TEXT("Reset removes every context source"), Collector->GetActiveContextTags().IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKnowledgeDialogueGuardTest,
    "MyAiTown.KnowledgeGrant.Runtime.DialogueGuard",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnowledgeDialogueGuardTest::RunTest(const FString& Parameters)
{
    UKnowledgePack* Seal = MakeWorldEventPack(
        TEXT("caravan.seal"), TEXT("event.seal.found"), TEXT("royal_seal_found"),
        TEXT("A royal seal was found at the wreck."));
    UKnowledgePack* Origin = MakeWorldEventPack(
        TEXT("dragon.origin"), TEXT("event.origin.proved"), TEXT("dragon_origin"),
        TEXT("Later evidence proves that the royal court made the dragon."), {Seal->PackId});
    Seal->Documents[0].Facts[0].Statement = TEXT("现场发现了1枚王廷秘印。");
    Seal->Documents[0].Facts[0].Entities = {TEXT("王廷秘印")};
    Seal->Documents[0].Facts[0].Numbers = {TEXT("1")};
    Origin->Documents[0].Facts[0].Statement = TEXT("王廷制造了巨龙。");
    Origin->Documents[0].Facts[0].Entities = {TEXT("王廷"), TEXT("巨龙")};
    TArray<UKnowledgePack*> Packs{Seal, Origin};
    UKnowledgeGrantComponent* Player = MakeGrantComponent();
    TestTrue(TEXT("Observed seal event is granted"), Player->GrantKnowledgeFromWorldEvent(
        TEXT("world:seal:1"), TEXT("event.seal.found"), Seal).bOk);

    FKnowledgeDialogueClaim Unsupported;
    Unsupported.ClaimId = TEXT("claim.dragon.creator");
    Unsupported.Text = TEXT("王廷制造了巨龙。");
    Unsupported.EvidenceFactIds = {FactIdForDocument(TEXT("dragon_origin"))};
    Unsupported.ClaimedEntities = {TEXT("王廷"), TEXT("巨龙")};
    const FDialogueKnowledgeView SealView =
        UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(Player, Packs, FGameplayTagContainer());
    const FKnowledgeDialogueGuardResult Blocked =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, SealView, FGameplayTagContainer(), {Unsupported}, MakeGuardPolicy());
    TestTrue(TEXT("Unsupported factual claim is blocked after generation"),
        !Blocked.bOk && Blocked.bBlocked && Blocked.Outcome == EKnowledgeDialogueGuardOutcome::Blocked);
    TestEqual(TEXT("Raw unsupported words never reach safe output"), Blocked.SafeText, FString(TEXT("现有证据不足，无法断言。")));

    FKnowledgeDialogueClaim Supported;
    Supported.ClaimId = TEXT("claim.seal.present");
    Supported.Text = TEXT("现场发现了1枚王廷秘印。");
    Supported.EvidenceFactIds = {FactIdForDocument(TEXT("royal_seal_found"))};
    Supported.ClaimedEntities = {TEXT("王廷秘印")};
    Supported.ClaimedNumbers = {TEXT("1")};
    const FKnowledgeDialogueGuardResult Allowed =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, SealView, FGameplayTagContainer(), {Supported}, MakeGuardPolicy());
    TestTrue(TEXT("Claim backed by an unlocked document is allowed"), Allowed.bOk && !Allowed.bBlocked);
    TestEqual(TEXT("Grounded words survive unchanged"), Allowed.SafeText, Supported.Text);

    TArray<FName> FallbackEvidence;
    const FString AuthorizedFallback = UAiNpcDialogueComponent::BuildAuthorizedFactFallback(
        TEXT("王廷秘印在哪里？"), SealView, FText::FromString(TEXT("默认兜底")), FallbackEvidence);
    TestEqual(TEXT("Deterministic fallback returns the authorized fact without adding scripted prose"),
        AuthorizedFallback, Seal->Documents[0].Facts[0].Statement);
    TestTrue(TEXT("Deterministic fallback records its FactId"),
        FallbackEvidence == TArray<FName>{FactIdForDocument(TEXT("royal_seal_found"))});
    FDialogueKnowledgeView EmptyFallbackView;
    TestEqual(TEXT("A view without authorized facts uses only the trusted default fallback"),
        UAiNpcDialogueComponent::BuildAuthorizedFactFallback(
            TEXT("未知问题"), EmptyFallbackView, FText::FromString(TEXT("默认兜底")), FallbackEvidence),
        FString(TEXT("默认兜底")));
    TestTrue(TEXT("The default fallback does not claim an EvidenceFactId"), FallbackEvidence.IsEmpty());

    FKnowledgeDialogueClaim Uncited = Supported;
    Uncited.ClaimId = TEXT("claim.uncited");
    Uncited.EvidenceFactIds.Reset();
    TestTrue(TEXT("Every model claim without evidence fails closed"),
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, SealView, FGameplayTagContainer(), {Uncited}, MakeGuardPolicy()).bBlocked);

    UKnowledgeDialogueGuardPolicy* RoleplayPolicy = MakeGuardPolicy();
    RoleplayPolicy->bAllowUncitedRoleplay = true;
    FKnowledgeDialogueClaim Roleplay;
    Roleplay.ClaimId = TEXT("claim.roleplay.proposal");
    Roleplay.Text = TEXT("先查看周围，再决定从哪里开始。");
    const FKnowledgeDialogueGuardResult AllowedRoleplay =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, SealView, FGameplayTagContainer(), {Roleplay}, RoleplayPolicy);
    TestTrue(TEXT("Opted-in non-factual roleplay can continue without a fact citation"),
        AllowedRoleplay.bOk && !AllowedRoleplay.bBlocked
        && AllowedRoleplay.SafeText == Roleplay.Text);

    FKnowledgeDialogueClaim NoisyRoleplayMetadata = Roleplay;
    NoisyRoleplayMetadata.ClaimId = TEXT("claim.roleplay.noisy_metadata");
    NoisyRoleplayMetadata.ClaimedEntities = {TEXT("周围")};
    NoisyRoleplayMetadata.ClaimedNumbers = {TEXT("起点")};
    NoisyRoleplayMetadata.bNegated = true;
    const FKnowledgeDialogueGuardResult NoisyRoleplayAllowed =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, SealView, FGameplayTagContainer(), {NoisyRoleplayMetadata}, RoleplayPolicy);
    TestTrue(TEXT("Untrusted roleplay metadata cannot falsely block safe text"),
        NoisyRoleplayAllowed.bOk && !NoisyRoleplayAllowed.bBlocked);

    FKnowledgeDialogueClaim UnsafeRoleplay = Roleplay;
    UnsafeRoleplay.ClaimId = TEXT("claim.roleplay.protected_entity");
    UnsafeRoleplay.Text = TEXT("王廷已经下令，我们立刻出发。");
    const FKnowledgeDialogueGuardResult BlockedRoleplay =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, SealView, FGameplayTagContainer(), {UnsafeRoleplay}, RoleplayPolicy);
    TestTrue(TEXT("Uncited roleplay cannot smuggle a protected world-state assertion"),
        BlockedRoleplay.bBlocked
        && BlockedRoleplay.Error == TEXT("dialogue_claim_uncited_roleplay_unsafe"));

    FKnowledgeDialogueClaim WrongEntity = Supported;
    WrongEntity.ClaimId = TEXT("claim.wrong_entity");
    WrongEntity.Text = TEXT("现场发现了1枚议会秘印。");
    WrongEntity.ClaimedEntities = {TEXT("议会秘印")};
    const FKnowledgeDialogueGuardResult EntityMismatch = UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
        Player, SealView, FGameplayTagContainer(), {WrongEntity}, MakeGuardPolicy());
    TestTrue(TEXT("A proper-noun mismatch is blocked mechanically"),
        EntityMismatch.bBlocked && EntityMismatch.Error == TEXT("dialogue_claim_entity_mismatch"));

    FKnowledgeDialogueClaim UndeclaredEntity = Supported;
    UndeclaredEntity.ClaimId = TEXT("claim.undeclared_entity");
    UndeclaredEntity.Text = TEXT("现场发现了1枚王廷秘印，议会在旁监督。");
    const FKnowledgeDialogueGuardResult UndeclaredEntityResult =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, SealView, FGameplayTagContainer(), {UndeclaredEntity}, MakeGuardPolicy());
    TestTrue(TEXT("A protected proper noun omitted from claim metadata is blocked"),
        UndeclaredEntityResult.bBlocked
        && UndeclaredEntityResult.Error == TEXT("dialogue_claim_entity_mismatch"));

    FKnowledgeDialogueClaim WrongNumber = Supported;
    WrongNumber.ClaimId = TEXT("claim.wrong_number");
    WrongNumber.Text = TEXT("现场发现了2枚王廷秘印。");
    WrongNumber.ClaimedNumbers = {TEXT("2")};
    const FKnowledgeDialogueGuardResult NumberMismatch = UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
        Player, SealView, FGameplayTagContainer(), {WrongNumber}, MakeGuardPolicy());
    TestTrue(TEXT("An ordinary number is deferred to the independent semantic reviewer"),
        NumberMismatch.bOk && !NumberMismatch.bBlocked);

    UKnowledgeDialogueGuardPolicy* ProtectedNumberPolicy = MakeGuardPolicy();
    ProtectedNumberPolicy->ProtectedNumberLexicon = {TEXT("2")};
    const FKnowledgeDialogueGuardResult ProtectedNumberMismatch =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, SealView, FGameplayTagContainer(), {WrongNumber}, ProtectedNumberPolicy);
    TestTrue(TEXT("An unavailable host-protected number is blocked mechanically"),
        ProtectedNumberMismatch.bBlocked
        && ProtectedNumberMismatch.Error == TEXT("dialogue_claim_number_mismatch"));

    FKnowledgeDialogueClaim WrongNegation = Supported;
    WrongNegation.ClaimId = TEXT("claim.wrong_negation");
    WrongNegation.Text = TEXT("现场没有发现1枚王廷秘印。");
    WrongNegation.bNegated = true;
    const FKnowledgeDialogueGuardResult NegationMismatch = UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
        Player, SealView, FGameplayTagContainer(), {WrongNegation}, MakeGuardPolicy());
    TestTrue(TEXT("Natural-language negation is deferred to the independent semantic reviewer"),
        NegationMismatch.bOk && !NegationMismatch.bBlocked);

    FKnowledgeDialogueClaim PositiveWithBu = Supported;
    PositiveWithBu.ClaimId = TEXT("claim.positive.bu");
    PositiveWithBu.Text = TEXT("现场情况不错，发现了1枚王廷秘印。");
    const FKnowledgeDialogueGuardResult PositiveWithBuResult =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, SealView, FGameplayTagContainer(), {PositiveWithBu}, MakeGuardPolicy());
    TestTrue(TEXT("The positive word bucuo is not treated as grammatical negation"),
        PositiveWithBuResult.bOk && !PositiveWithBuResult.bBlocked);

    FKnowledgeDialogueClaim CoveredAndIndependentEntity = Supported;
    CoveredAndIndependentEntity.ClaimId = TEXT("claim.covered.and.independent.entity");
    CoveredAndIndependentEntity.Text = TEXT("现场发现了1枚王廷秘印，王廷随后下令封锁道路。");
    const FKnowledgeDialogueGuardResult CoveredAndIndependentEntityResult =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, SealView, FGameplayTagContainer(), {CoveredAndIndependentEntity}, MakeGuardPolicy());
    TestTrue(TEXT("A shorter protected entity occurring outside its declared longer token is blocked"),
        CoveredAndIndependentEntityResult.bBlocked
        && CoveredAndIndependentEntityResult.Error == TEXT("dialogue_claim_entity_mismatch"));

    const FKnowledgeDialogueGuardResult WholeResponseBlocked =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, SealView, FGameplayTagContainer(), {Supported, Unsupported}, MakeGuardPolicy());
    TestTrue(TEXT("All-or-nothing policy removes otherwise valid claims from a mixed response"),
        WholeResponseBlocked.bBlocked && WholeResponseBlocked.AllowedClaimIds.IsEmpty()
        && !WholeResponseBlocked.SafeText.Contains(Supported.Text));

    const FKnowledgeDialogueGuardResult PerClaim =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, SealView, FGameplayTagContainer(), {Supported, Unsupported},
            MakeGuardPolicy(EKnowledgeDialogueGuardMode::PerClaim));
    TestTrue(TEXT("Per-claim policy preserves only grounded claims"),
        PerClaim.bBlocked && PerClaim.AllowedClaimIds.Num() == 1 && PerClaim.SafeText == Supported.Text);

    TestTrue(TEXT("Origin proof event is granted after its prerequisite"), Player->GrantKnowledgeFromWorldEvent(
        TEXT("world:origin:1"), TEXT("event.origin.proved"), Origin).bOk);
    const FKnowledgeDialogueGuardResult Stale = UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
        Player, SealView, FGameplayTagContainer(), {Supported}, MakeGuardPolicy());
    TestTrue(TEXT("A view is rejected after player knowledge revision changes"),
        Stale.bBlocked && Stale.Error == TEXT("dialogue_view_stale"));
    const FDialogueKnowledgeView OriginView =
        UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(Player, Packs, FGameplayTagContainer());
    FDialogueKnowledgeView TamperedView = OriginView;
    TamperedView.ScopeFingerprint ^= 1;
    const FKnowledgeDialogueGuardResult Tampered =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, TamperedView, FGameplayTagContainer(), {Unsupported}, MakeGuardPolicy());
    TestTrue(TEXT("A view with a modified consistency checksum is rejected"),
        !Tampered.bOk && Tampered.Error == TEXT("dialogue_view_tampered"));
    const FKnowledgeDialogueGuardResult LaterAllowed =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, OriginView, FGameplayTagContainer(), {Unsupported}, MakeGuardPolicy());
    TestTrue(TEXT("The same claim is allowed only after its evidence unlocks"), LaterAllowed.bOk && !LaterAllowed.bBlocked);
    TestEqual(TEXT("Newly grounded words survive unchanged"), LaterAllowed.SafeText, Unsupported.Text);

    FKnowledgeDialogueClaim PartialGrounded = Unsupported;
    PartialGrounded.ClaimId = TEXT("claim.dragon.only");
    PartialGrounded.Text = TEXT("巨龙确实与此事有关。");
    PartialGrounded.ClaimedEntities = {TEXT("巨龙")};
    const FKnowledgeDialogueGuardResult PartialAllowed =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, OriginView, FGameplayTagContainer(), {PartialGrounded}, MakeGuardPolicy());
    TestTrue(TEXT("A claim may safely use only a subset of entities from its cited fact"),
        PartialAllowed.bOk && !PartialAllowed.bBlocked && PartialAllowed.SafeText == PartialGrounded.Text);

    FKnowledgeDialogueClaim NoisyEntityMetadata = PartialGrounded;
    NoisyEntityMetadata.ClaimId = TEXT("claim.dragon.noisy_metadata");
    NoisyEntityMetadata.ClaimedEntities = {TEXT("巨龙"), TEXT("此事")};
    const FKnowledgeDialogueGuardResult NoisyMetadataAllowed =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, OriginView, FGameplayTagContainer(), {NoisyEntityMetadata}, MakeGuardPolicy());
    TestTrue(TEXT("Untrusted extra generic entity metadata cannot falsely block grounded text"),
        NoisyMetadataAllowed.bOk && !NoisyMetadataAllowed.bBlocked
        && NoisyMetadataAllowed.SafeText == NoisyEntityMetadata.Text);

    FKnowledgeDialogueClaim NoisyNegationMetadata = PartialGrounded;
    NoisyNegationMetadata.ClaimId = TEXT("claim.dragon.noisy_negation_metadata");
    NoisyNegationMetadata.bNegated = true;
    const FKnowledgeDialogueGuardResult NoisyNegationAllowed =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, OriginView, FGameplayTagContainer(), {NoisyNegationMetadata}, MakeGuardPolicy());
    TestTrue(TEXT("Untrusted negation metadata cannot falsely reverse positive grounded text"),
        NoisyNegationAllowed.bOk && !NoisyNegationAllowed.bBlocked);

    UKnowledgePack* Numeric = MakeWorldEventPack(
        TEXT("measurements.observed"), TEXT("event.measurements.observed"), TEXT("temperature_reading"),
        TEXT("温度降至-5度。"));
    Numeric->Documents[0].Facts[0].Numbers = {TEXT("-5")};
    FKnowledgeDocument Ratio;
    Ratio.DocumentId = TEXT("ratio_reading");
    Ratio.Content = TEXT("比例为1.5。 ");
    AddDefaultFact(Ratio);
    Ratio.Facts[0].Numbers = {TEXT("1.5")};
    Numeric->Documents.Add(Ratio);
    FKnowledgeDocument WrittenNumber;
    WrittenNumber.DocumentId = TEXT("written_number_reading");
    WrittenNumber.Content = TEXT("现场有三枚封印。 ");
    AddDefaultFact(WrittenNumber);
    WrittenNumber.Facts[0].Numbers = {TEXT("三")};
    Numeric->Documents.Add(WrittenNumber);
    Packs.Add(Numeric);
    TestTrue(TEXT("Measurement evidence is granted"), Player->GrantKnowledgeFromWorldEvent(
        TEXT("world:measurements:1"), Numeric->SourceWorldEventId, Numeric).bOk);
    const FDialogueKnowledgeView NumericView =
        UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(Player, Packs, FGameplayTagContainer());
    UKnowledgeDialogueGuardPolicy* NumericPolicy = MakeGuardPolicy();
    NumericPolicy->ProtectedNumberLexicon = {TEXT("三")};

    FKnowledgeDialogueClaim NegativeNumber;
    NegativeNumber.ClaimId = TEXT("claim.temperature.negative");
    NegativeNumber.Text = TEXT("温度降至-5度。");
    NegativeNumber.EvidenceFactIds = {FactIdForDocument(TEXT("temperature_reading"))};
    NegativeNumber.ClaimedNumbers = {TEXT("-5")};
    TestTrue(TEXT("A signed number is preserved and accepted"),
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, NumericView, FGameplayTagContainer(), {NegativeNumber}, NumericPolicy).bOk);
    FKnowledgeDialogueClaim NoisyNumberMetadata = NegativeNumber;
    NoisyNumberMetadata.ClaimId = TEXT("claim.temperature.noisy_metadata");
    NoisyNumberMetadata.ClaimedNumbers = {TEXT("999")};
    TestTrue(TEXT("Untrusted number metadata cannot falsely block an authorized number in text"),
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, NumericView, FGameplayTagContainer(), {NoisyNumberMetadata}, NumericPolicy).bOk);
    FKnowledgeDialogueClaim SignReversal = NegativeNumber;
    SignReversal.ClaimId = TEXT("claim.temperature.sign.reversal");
    SignReversal.Text = TEXT("温度升至5度。");
    SignReversal.ClaimedNumbers = {TEXT("5")};
    TestTrue(TEXT("A numeric sign reversal is deferred to the semantic relation gate"),
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, NumericView, FGameplayTagContainer(), {SignReversal}, NumericPolicy).bOk);

    FKnowledgeDialogueClaim DecimalNumber;
    DecimalNumber.ClaimId = TEXT("claim.ratio.decimal");
    DecimalNumber.Text = TEXT("比例为1.5。");
    DecimalNumber.EvidenceFactIds = {FactIdForDocument(TEXT("ratio_reading"))};
    DecimalNumber.ClaimedNumbers = {TEXT("1.5")};
    TestTrue(TEXT("A decimal is treated as one numeric token"),
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, NumericView, FGameplayTagContainer(), {DecimalNumber}, NumericPolicy).bOk);

    FKnowledgeDialogueClaim WrittenNumberClaim;
    WrittenNumberClaim.ClaimId = TEXT("claim.written.number");
    WrittenNumberClaim.Text = TEXT("现场有三枚封印。");
    WrittenNumberClaim.EvidenceFactIds = {FactIdForDocument(TEXT("written_number_reading"))};
    WrittenNumberClaim.ClaimedNumbers = {TEXT("三")};
    TestTrue(TEXT("A host-declared Chinese written number is accepted"),
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, NumericView, FGameplayTagContainer(), {WrittenNumberClaim}, NumericPolicy).bOk);

    UKnowledgeDialogueGuardPolicy* SurfaceNumberPolicy = MakeGuardPolicy();
    SurfaceNumberPolicy->ProtectedNumberLexicon = {TEXT("2"), TEXT("二"), TEXT("3"), TEXT("三")};
    FKnowledgeDialogueClaim ArabicEquivalent = WrittenNumberClaim;
    ArabicEquivalent.ClaimId = TEXT("claim.written.number.arabic.equivalent");
    ArabicEquivalent.Text = TEXT("现场有3枚封印。");
    ArabicEquivalent.ClaimedNumbers = {TEXT("3")};
    const FKnowledgeDialogueGuardResult ArabicEquivalentResult =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, NumericView, FGameplayTagContainer(), {ArabicEquivalent}, SurfaceNumberPolicy);
    TestTrue(TEXT("Arabic and Chinese surface forms of the same integer are equivalent"),
        ArabicEquivalentResult.bOk && !ArabicEquivalentResult.bBlocked);

    FKnowledgeDialogueClaim DifferentQuantity = ArabicEquivalent;
    DifferentQuantity.ClaimId = TEXT("claim.written.number.different.quantity");
    DifferentQuantity.Text = TEXT("现场有二枚封印。");
    DifferentQuantity.ClaimedNumbers = {TEXT("二")};
    const FKnowledgeDialogueGuardResult DifferentQuantityResult =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, NumericView, FGameplayTagContainer(), {DifferentQuantity}, SurfaceNumberPolicy);
    TestTrue(TEXT("A different quantity remains blocked after surface normalization"),
        DifferentQuantityResult.bBlocked
        && DifferentQuantityResult.Error == TEXT("dialogue_claim_number_mismatch"));

    FKnowledgeDialogueClaim ChineseRange = WrittenNumberClaim;
    ChineseRange.ClaimId = TEXT("claim.written.number.range");
    ChineseRange.Text = TEXT("现场约有三百枚封印。");
    TestTrue(TEXT("A shorter Chinese protected number does not match inside a larger number"),
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, NumericView, FGameplayTagContainer(), {ChineseRange}, NumericPolicy).bOk);

    UKnowledgeDialogueGuardPolicy* CompatibilityPolicy = MakeGuardPolicy();
    CompatibilityPolicy->ProtectedNumberLexicon = {TEXT("3000")};
    FKnowledgeDialogueClaim FullWidthNumber = NegativeNumber;
    FullWidthNumber.ClaimId = TEXT("claim.number.fullwidth");
    FullWidthNumber.Text = TEXT("库存为３，０００枚。 ");
    const FKnowledgeDialogueGuardResult FullWidthBlocked =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, NumericView, FGameplayTagContainer(), {FullWidthNumber}, CompatibilityPolicy);
    TestTrue(TEXT("Full-width grouped digits normalize before protected-number matching"),
        FullWidthBlocked.bBlocked
        && FullWidthBlocked.Error == TEXT("dialogue_claim_number_mismatch"));

    FKnowledgeDialogueClaim OrdinaryDuration = NegativeNumber;
    OrdinaryDuration.ClaimId = TEXT("claim.number.ordinary.duration");
    OrdinaryDuration.Text = TEXT("先观察五分钟再行动。");
    TestTrue(TEXT("An unprotected ordinary duration is not mechanically rejected"),
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, NumericView, FGameplayTagContainer(), {OrdinaryDuration}, NumericPolicy).bOk);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKnowledgeModelAdapterContractTest,
    "MyAiTown.KnowledgeGrant.Runtime.ModelAdapterContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnowledgeModelAdapterContractTest::RunTest(const FString& Parameters)
{
    UKnowledgePack* Seal = MakeWorldEventPack(
        TEXT("adapter.seal"), TEXT("event.adapter.seal"), TEXT("adapter_seal_document"),
        TEXT("现场发现了1枚王廷秘印。"));
    Seal->Documents[0].Facts[0].FactId = TEXT("fact.adapter.royal_seal");
    Seal->Documents[0].Facts[0].Statement = TEXT("现场发现了1枚王廷秘印。");
    Seal->Documents[0].Facts[0].Entities = {TEXT("王廷秘印")};
    Seal->Documents[0].Facts[0].Numbers = {TEXT("1")};
    UKnowledgeGrantComponent* Player = MakeGrantComponent();
    TestTrue(TEXT("Adapter fixture knowledge is granted"), Player->GrantKnowledgeFromWorldEvent(
        TEXT("world:adapter:seal:1"), TEXT("event.adapter.seal"), Seal).bOk);
    const FDialogueKnowledgeView View = UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(
        Player, {Seal}, FGameplayTagContainer());
    UKnowledgeDialogueGuardPolicy* Policy = MakeGuardPolicy();

    const FString ValidJson = TEXT("{\"claims\":[{\"claimId\":\"claim.adapter.seal\",\"text\":\"现场发现了1枚王廷秘印。\",\"evidenceFactIds\":[\"fact.adapter.royal_seal\"],\"claimedEntities\":[\"王廷秘印\"],\"claimedNumbers\":[\"1\"],\"negated\":false}]}");
    const FKnowledgeDialogueGuardResult Valid = UKnowledgeModelAdapterLibrary::GuardKnowledgeModelJsonResponse(
        Player, View, FGameplayTagContainer(), ValidJson, Policy);
    TestTrue(TEXT("Strict JSON adapter passes one fully grounded claim"), Valid.bOk && !Valid.bBlocked);
    TestEqual(TEXT("Adapter exposes only guarded SafeText"), Valid.SafeText, FString(TEXT("现场发现了1枚王廷秘印。")));
    TestEqual(TEXT("The adapter replaces model claim IDs with plugin-assigned IDs"),
        Valid.AllowedClaimIds[0], FName(TEXT("claim-0001")));

    const FKnowledgeDialogueGuardResult Malformed = UKnowledgeModelAdapterLibrary::GuardKnowledgeModelJsonResponse(
        Player, View, FGameplayTagContainer(), TEXT("{not-json"), Policy);
    TestTrue(TEXT("Malformed JSON fails closed"),
        Malformed.bBlocked && Malformed.Error == TEXT("model_json_malformed"));
    TestFalse(TEXT("Malformed raw payload never reaches SafeText"), Malformed.SafeText.Contains(TEXT("not-json")));

    const FKnowledgeDialogueGuardResult UnknownRoot = UKnowledgeModelAdapterLibrary::GuardKnowledgeModelJsonResponse(
        Player, View, FGameplayTagContainer(), TEXT("{\"claims\":[],\"rawText\":\"secret raw leak\"}"), Policy);
    TestTrue(TEXT("Unknown root fields fail the exact schema"),
        UnknownRoot.bBlocked && UnknownRoot.Error == TEXT("model_json_schema_invalid"));
    TestFalse(TEXT("Unknown rawText never reaches SafeText"), UnknownRoot.SafeText.Contains(TEXT("secret raw leak")));

    const FString EmptyEvidenceJson = TEXT("{\"claims\":[{\"claimId\":\"claim.empty\",\"text\":\"现场发现了1枚王廷秘印。\",\"evidenceFactIds\":[],\"claimedEntities\":[\"王廷秘印\"],\"claimedNumbers\":[\"1\"],\"negated\":false}]}");
    const FKnowledgeDialogueGuardResult EmptyEvidence = UKnowledgeModelAdapterLibrary::GuardKnowledgeModelJsonResponse(
        Player, View, FGameplayTagContainer(), EmptyEvidenceJson, Policy);
    TestTrue(TEXT("Empty FactId evidence is rejected by the adapter contract"),
        EmptyEvidence.bBlocked && EmptyEvidence.Error == TEXT("model_json_evidence_invalid"));

    Policy->bAllowUncitedRoleplay = true;
    const FString RoleplayJson = TEXT("{\"claims\":[{\"claimId\":\"claim.roleplay\",\"text\":\"先看看周围，你想从哪里开始？\",\"evidenceFactIds\":[],\"claimedEntities\":[],\"claimedNumbers\":[],\"negated\":false}]}");
    const FKnowledgeDialogueGuardResult ParsedRoleplay =
        UKnowledgeModelAdapterLibrary::GuardKnowledgeModelJsonResponse(
            Player, View, FGameplayTagContainer(), RoleplayJson, Policy);
    TestTrue(TEXT("The adapter accepts opted-in non-factual roleplay JSON"),
        ParsedRoleplay.bOk && !ParsedRoleplay.bBlocked);
    Policy->bAllowUncitedRoleplay = false;

    const FString DuplicateEvidenceJson = TEXT("{\"claims\":[{\"claimId\":\"claim.duplicate.evidence\",\"text\":\"现场发现了1枚王廷秘印。\",\"evidenceFactIds\":[\"fact.adapter.royal_seal\",\"fact.adapter.royal_seal\"],\"claimedEntities\":[\"王廷秘印\"],\"claimedNumbers\":[\"1\"],\"negated\":false}]}");
    TestTrue(TEXT("Duplicate FactId evidence is rejected"),
        UKnowledgeModelAdapterLibrary::GuardKnowledgeModelJsonResponse(
            Player, View, FGameplayTagContainer(), DuplicateEvidenceJson, Policy).Error == TEXT("model_json_evidence_invalid"));

    const FString MissingEntityJson = TEXT("{\"claims\":[{\"claimId\":\"claim.missing.entity.json\",\"text\":\"现场发现了1枚王廷秘印。\",\"evidenceFactIds\":[\"fact.adapter.royal_seal\"],\"claimedEntities\":[],\"claimedNumbers\":[\"1\"],\"negated\":false}]}");
    const FKnowledgeDialogueGuardResult DerivedEntity =
        UKnowledgeModelAdapterLibrary::GuardKnowledgeModelJsonResponse(
            Player, View, FGameplayTagContainer(), MissingEntityJson, Policy);
    TestTrue(TEXT("A protected authorized entity is derived locally when model metadata omits it"),
        DerivedEntity.bOk && !DerivedEntity.bBlocked
        && DerivedEntity.SafeText == TEXT("现场发现了1枚王廷秘印。"));

    const FString WrongNumberJson = TEXT("{\"claims\":[{\"claimId\":\"claim.wrong.number.json\",\"text\":\"现场发现了2枚王廷秘印。\",\"evidenceFactIds\":[\"fact.adapter.royal_seal\"],\"claimedEntities\":[\"王廷秘印\"],\"claimedNumbers\":[\"2\"],\"negated\":false}]}");
    TestTrue(TEXT("Parsed ordinary numbers are passed to the independent semantic reviewer"),
        UKnowledgeModelAdapterLibrary::GuardKnowledgeModelJsonResponse(
            Player, View, FGameplayTagContainer(), WrongNumberJson, Policy).bOk);

    const FString WrongNegationJson = TEXT("{\"claims\":[{\"claimId\":\"claim.wrong.negation.json\",\"text\":\"现场没有发现1枚王廷秘印。\",\"evidenceFactIds\":[\"fact.adapter.royal_seal\"],\"claimedEntities\":[\"王廷秘印\"],\"claimedNumbers\":[\"1\"],\"negated\":true}]}");
    TestTrue(TEXT("Parsed negation is passed to the independent semantic reviewer"),
        UKnowledgeModelAdapterLibrary::GuardKnowledgeModelJsonResponse(
            Player, View, FGameplayTagContainer(), WrongNegationJson, Policy).bOk);

    const FString LongText = FString::ChrN(2049, TEXT('x'));
    const FString LongTextJson = FString::Printf(TEXT("{\"claims\":[{\"claimId\":\"claim.long\",\"text\":\"%s\",\"evidenceFactIds\":[\"fact.adapter.royal_seal\"],\"claimedEntities\":[],\"claimedNumbers\":[],\"negated\":false}]}"), *LongText);
    TestTrue(TEXT("Overlong claim text is rejected before the fact guard"),
        UKnowledgeModelAdapterLibrary::GuardKnowledgeModelJsonResponse(
            Player, View, FGameplayTagContainer(), LongTextJson, Policy).Error == TEXT("model_json_claim_text_invalid"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKnowledgeGrantQuestFlowTest,
    "MyAiTown.KnowledgeGrant.Runtime.QuestFlow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnowledgeGrantQuestFlowTest::RunTest(const FString& Parameters)
{
    UKnowledgePack* Pack = MakeTestPack();
    Pack->UnlockTag = KnowledgeContextTags::ItemBronzeControlBell;
    TArray<UKnowledgePack*> Packs{Pack};
    UKnowledgeGrantComponent* PlayerOne = MakeGrantComponent();
    UKnowledgeGrantComponent* PlayerTwo = MakeGrantComponent();

    UKnowledgePack* LockedMalformedPack = NewObject<UKnowledgePack>();
    LockedMalformedPack->PackId = TEXT("locked.malformed.pack");
    const FDialogueKnowledgeView LockedWithMalformedCandidate =
        UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(
            PlayerOne, {Pack, LockedMalformedPack}, FGameplayTagContainer());
    TestTrue(TEXT("An invalid but ungranted candidate does not fail the entire dialogue view"),
        LockedWithMalformedCandidate.bOk && LockedWithMalformedCandidate.AllowedDocuments.IsEmpty());

    const FDialogueKnowledgeView Locked = UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(PlayerOne, Packs, FGameplayTagContainer());
    TestTrue(TEXT("Locked resolution succeeds without intervention"), Locked.bOk);
    TestEqual(TEXT("Locked documents never enter the prompt payload"), Locked.AllowedDocuments.Num(), 0);

    const FKnowledgeGrantResult Granted = PlayerOne->GrantKnowledgeFromQuest(
        TEXT("player-1:quest.ancient_dragon:complete"), TEXT("quest.ancient_dragon"), Pack);
    TestTrue(TEXT("Quest completion grants knowledge"), Granted.bOk && Granted.bApplied);

    const FDialogueKnowledgeView Unlocked = UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(PlayerOne, Packs, FGameplayTagContainer());
    TestTrue(TEXT("Unlocked resolution succeeds"), Unlocked.bOk);
    TestEqual(TEXT("Every document in the granted pack is available"), Unlocked.AllowedDocuments.Num(), 2);

    const FDialogueKnowledgeView OtherPlayer = UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(PlayerTwo, Packs, FGameplayTagContainer());
    TestEqual(TEXT("Another player remains locked"), OtherPlayer.AllowedDocuments.Num(), 0);

    const FKnowledgeGrantResult Duplicate = PlayerOne->GrantKnowledgeFromQuest(
        TEXT("player-1:quest.ancient_dragon:complete"), TEXT("quest.ancient_dragon"), Pack);
    TestTrue(TEXT("Repeated completion is idempotent"), Duplicate.bOk && Duplicate.bDuplicate && !Duplicate.bApplied);
    TestEqual(TEXT("Duplicate does not advance revision"), Duplicate.Revision, Granted.Revision);

    const FKnowledgeGrantResult TrimmedDuplicate = PlayerOne->GrantKnowledgeFromQuest(
        TEXT("  player-1:quest.ancient_dragon:complete  "), TEXT("quest.ancient_dragon"), Pack);
    TestTrue(TEXT("Grant event identifiers are normalized before idempotence checks"),
        TrimmedDuplicate.bOk && TrimmedDuplicate.bDuplicate && TrimmedDuplicate.Revision == Granted.Revision);

    UKnowledgePack* ConflictingPack = MakeTestPack();
    ConflictingPack->PackId = TEXT("quest.ancient_dragon.conflicting_reward");
    for (int32 Index = 0; Index < ConflictingPack->Documents.Num(); ++Index)
    {
        ConflictingPack->Documents[Index].DocumentId = FName(*FString::Printf(TEXT("conflict_doc_%d"), Index));
        ConflictingPack->Documents[Index].Facts[0].FactId = FactIdForDocument(ConflictingPack->Documents[Index].DocumentId);
    }
    const FKnowledgeGrantResult EventConflict = PlayerOne->GrantKnowledgeFromQuest(
        TEXT("player-1:quest.ancient_dragon:complete"), TEXT("quest.ancient_dragon"), ConflictingPack);
    TestTrue(TEXT("Reusing one grant event identifier for a different pack fails explicitly"),
        !EventConflict.bOk && EventConflict.Error == TEXT("grant_event_id_conflict")
        && !PlayerOne->HasKnowledgePack(ConflictingPack->PackId));

    UKnowledgeGrantComponent* RestoredPlayer = MakeGrantComponent();
    const FKnowledgeGrantResult Restored = RestoredPlayer->RestoreSnapshot(
        PlayerOne->CreateSnapshot(TestPackRegistryVersion), Packs, TestPackRegistryVersion);
    TestTrue(TEXT("Snapshot restores successfully"), Restored.bOk);
    const FDialogueKnowledgeView AfterRestore = UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(RestoredPlayer, Packs, FGameplayTagContainer());
    TestEqual(TEXT("Knowledge survives save and restore"), AfterRestore.AllowedDocuments.Num(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKnowledgeGrantValidationTest,
    "MyAiTown.KnowledgeGrant.Runtime.Validation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnowledgeGrantValidationTest::RunTest(const FString& Parameters)
{
    UKnowledgePack* Pack = MakeTestPack();
    UKnowledgeGrantComponent* Player = MakeGrantComponent();
    const FKnowledgeGrantResult Mismatch = Player->GrantKnowledgeFromQuest(
        TEXT("wrong-quest"), TEXT("quest.some_other_quest"), Pack);
    TestFalse(TEXT("A different quest cannot grant the pack"), Mismatch.bOk);
    TestEqual(TEXT("Mismatch is explicit"), Mismatch.Error, FString(TEXT("quest_pack_mismatch")));

    FKnowledgeGrantSnapshot InvalidSnapshot;
    InvalidSnapshot.SchemaVersion = 99;
    const FKnowledgeGrantResult InvalidRestore = Player->RestoreSnapshot(
        InvalidSnapshot, {Pack}, TestPackRegistryVersion);
    TestFalse(TEXT("Unsupported snapshot is rejected"), InvalidRestore.bOk);
    TestFalse(TEXT("Failed restore does not grant access"), Player->HasKnowledgePack(Pack->PackId));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKnowledgeSnapshotConsistencyTest,
    "MyAiTown.KnowledgeGrant.Runtime.SnapshotConsistency",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnowledgeSnapshotConsistencyTest::RunTest(const FString& Parameters)
{
    UKnowledgePack* Premise = MakeWorldEventPack(
        TEXT("snapshot.premise"), TEXT("snapshot.event.premise"), TEXT("snapshot_premise"),
        TEXT("The premise has happened."));
    Premise->UnlockTag = KnowledgeContextTags::SceneNorthGate;
    UKnowledgePack* Result = MakeWorldEventPack(
        TEXT("snapshot.result"), TEXT("snapshot.event.result"), TEXT("snapshot_result"),
        TEXT("The result has happened."), {Premise->PackId});
    Result->UnlockTag = KnowledgeContextTags::ItemBronzeControlBell;
    TArray<UKnowledgePack*> Registry{Premise, Result};

    UKnowledgeGrantComponent* Source = MakeGrantComponent();
    TestTrue(TEXT("Snapshot premise grant succeeds"), Source->GrantKnowledgeFromWorldEvent(
        TEXT("snapshot:premise:1"), Premise->SourceWorldEventId, Premise).bOk);
    TestTrue(TEXT("Snapshot dependent grant succeeds"), Source->GrantKnowledgeFromWorldEvent(
        TEXT("snapshot:result:1"), Result->SourceWorldEventId, Result).bOk);
    const FKnowledgeGrantSnapshot Snapshot = Source->CreateSnapshot(TestPackRegistryVersion);

    UKnowledgeGrantComponent* Restored = MakeGrantComponent();
    const FKnowledgeGrantResult ValidRestore = Restored->RestoreSnapshot(
        Snapshot, Registry, TestPackRegistryVersion);
    TestTrue(TEXT("A registry-matched snapshot restores successfully"), ValidRestore.bOk);
    TestTrue(TEXT("Restore derives both unlock tags from registered packs"),
        Restored->HasKnowledgeTag(KnowledgeContextTags::SceneNorthGate)
        && Restored->HasKnowledgeTag(KnowledgeContextTags::ItemBronzeControlBell));

    UKnowledgePack* InvalidButUngranted = NewObject<UKnowledgePack>();
    InvalidButUngranted->PackId = TEXT("snapshot.invalid.ungranted");
    TArray<UKnowledgePack*> RegistryWithInvalid = Registry;
    RegistryWithInvalid.Add(InvalidButUngranted);
    UKnowledgeGrantComponent* RestoredWithInvalid = MakeGrantComponent();
    const FKnowledgeGrantResult InvalidRegistryRestore = RestoredWithInvalid->RestoreSnapshot(
        Snapshot, RegistryWithInvalid, TestPackRegistryVersion);
    TestTrue(TEXT("An invalid but ungranted registry entry does not block restore"),
        InvalidRegistryRestore.bOk);

    UKnowledgePack* InvalidGranted = NewObject<UKnowledgePack>();
    InvalidGranted->PackId = Result->PackId;
    TArray<UKnowledgePack*> RegistryWithInvalidGranted = Registry;
    RegistryWithInvalidGranted.Remove(Result);
    RegistryWithInvalidGranted.Add(InvalidGranted);
    UKnowledgeGrantComponent* InvalidGrantedTarget = MakeGrantComponent();
    const FKnowledgeGrantResult InvalidGrantedRestore = InvalidGrantedTarget->RestoreSnapshot(
        Snapshot, RegistryWithInvalidGranted, TestPackRegistryVersion);
    TestTrue(TEXT("An invalid granted pack still blocks restore"),
        !InvalidGrantedRestore.bOk && InvalidGrantedRestore.Error == TEXT("snapshot_registry_invalid"));

    UKnowledgeGrantComponent* RevokeTarget = MakeGrantComponent();
    TestTrue(TEXT("Revoke fixture grants the premise"), RevokeTarget->GrantKnowledgeFromWorldEvent(
        TEXT("snapshot:revoke:premise"), Premise->SourceWorldEventId, Premise).bOk);
    TestTrue(TEXT("Revoke fixture grants the dependent"), RevokeTarget->GrantKnowledgeFromWorldEvent(
        TEXT("snapshot:revoke:result"), Result->SourceWorldEventId, Result).bOk);
    const FKnowledgeGrantResult Revoked = RevokeTarget->RevokeKnowledgePack(Result->PackId, Registry);
    TestTrue(TEXT("Revoking a leaf pack succeeds"), Revoked.bOk && Revoked.bApplied);
    TestTrue(TEXT("Revoking a pack re-derives the remaining unlock tags"),
        RevokeTarget->HasKnowledgeTag(KnowledgeContextTags::SceneNorthGate)
        && !RevokeTarget->HasKnowledgeTag(KnowledgeContextTags::ItemBronzeControlBell));
    const FKnowledgeGrantSnapshot RevokedSnapshot = RevokeTarget->CreateSnapshot(TestPackRegistryVersion);
    UKnowledgeGrantComponent* RevokedRestoreTarget = MakeGrantComponent();
    TestTrue(TEXT("A post-revoke snapshot restores without the revoked pack"),
        RevokedRestoreTarget->RestoreSnapshot(RevokedSnapshot, Registry, TestPackRegistryVersion).bOk);
    TestFalse(TEXT("The restored post-revoke state keeps the revoked pack locked"),
        RevokedRestoreTarget->HasKnowledgePack(Result->PackId));

    UKnowledgeGrantComponent* Rejected = MakeGrantComponent();
    TestTrue(TEXT("Rejected-restore fixture starts from a non-default state"),
        Rejected->GrantKnowledgeFromWorldEvent(
            TEXT("snapshot:rejected-baseline:1"), Premise->SourceWorldEventId, Premise).bOk);
    const FKnowledgeGrantState RejectedBaseline = Rejected->GetState();
    const FKnowledgeGrantResult WrongVersion = Rejected->RestoreSnapshot(
        Snapshot, Registry, TEXT("test.knowledge.registry.v2"));
    TestTrue(TEXT("A cross-version snapshot is rejected"),
        !WrongVersion.bOk && WrongVersion.Error == TEXT("snapshot_registry_version_mismatch"));

    FKnowledgeGrantSnapshot ForgedTags = Snapshot;
    ForgedTags.State.GrantedTags.AddTag(KnowledgeContextTags::SceneTavern);
    const FKnowledgeGrantResult ForgedTagRestore = Rejected->RestoreSnapshot(
        ForgedTags, Registry, TestPackRegistryVersion);
    TestTrue(TEXT("Forged snapshot tags are rejected instead of trusted"),
        !ForgedTagRestore.bOk && ForgedTagRestore.Error == TEXT("snapshot_granted_tags_mismatch"));

    FKnowledgeGrantSnapshot MissingPrerequisite = Snapshot;
    MissingPrerequisite.State.GrantedPackIds.Remove(Premise->PackId);
    MissingPrerequisite.State.GrantedTags.RemoveTag(Premise->UnlockTag);
    MissingPrerequisite.State.ProcessedGrants.RemoveAll(
        [Premise](const FKnowledgeGrantRecord& Record) { return Record.PackId == Premise->PackId; });
    const FKnowledgeGrantResult MissingRestore = Rejected->RestoreSnapshot(
        MissingPrerequisite, Registry, TestPackRegistryVersion);
    TestTrue(TEXT("A snapshot missing its prerequisite closure is rejected"),
        !MissingRestore.bOk && MissingRestore.Error == TEXT("snapshot_prerequisite_missing"));

    FKnowledgeGrantSnapshot UnknownPack = Snapshot;
    UnknownPack.State.GrantedPackIds.Add(TEXT("snapshot.unknown"));
    FKnowledgeGrantRecord UnknownRecord;
    UnknownRecord.GrantEventId = TEXT("snapshot:unknown:1");
    UnknownRecord.PackId = TEXT("snapshot.unknown");
    UnknownPack.State.ProcessedGrants.Add(UnknownRecord);
    const FKnowledgeGrantResult UnknownRestore = Rejected->RestoreSnapshot(
        UnknownPack, Registry, TestPackRegistryVersion);
    TestTrue(TEXT("An unregistered snapshot pack is rejected"),
        !UnknownRestore.bOk && UnknownRestore.Error == TEXT("snapshot_pack_unregistered"));

    UKnowledgePack* CycleA = MakeWorldEventPack(
        TEXT("snapshot.cycle.a"), TEXT("snapshot.event.cycle.a"), TEXT("snapshot_cycle_a"),
        TEXT("Cycle A."), {FName(TEXT("snapshot.cycle.b"))});
    UKnowledgePack* CycleB = MakeWorldEventPack(
        TEXT("snapshot.cycle.b"), TEXT("snapshot.event.cycle.b"), TEXT("snapshot_cycle_b"),
        TEXT("Cycle B."), {CycleA->PackId});
    FKnowledgeGrantSnapshot CycleSnapshot;
    CycleSnapshot.PackRegistryVersion = TestPackRegistryVersion;
    CycleSnapshot.State.GrantedPackIds = {CycleA->PackId, CycleB->PackId};
    FKnowledgeGrantRecord CycleARecord;
    CycleARecord.GrantEventId = TEXT("snapshot:cycle:a");
    CycleARecord.PackId = CycleA->PackId;
    FKnowledgeGrantRecord CycleBRecord;
    CycleBRecord.GrantEventId = TEXT("snapshot:cycle:b");
    CycleBRecord.PackId = CycleB->PackId;
    CycleSnapshot.State.ProcessedGrants = {CycleARecord, CycleBRecord};
    const FKnowledgeGrantResult CycleRestore = Rejected->RestoreSnapshot(
        CycleSnapshot, {CycleA, CycleB}, TestPackRegistryVersion);
    TestTrue(TEXT("A cyclic prerequisite registry is rejected during restore"),
        !CycleRestore.bOk && CycleRestore.Error == TEXT("snapshot_prerequisite_cycle"));
    FKnowledgeGrantSnapshot MissingGrantHistory = Snapshot;
    MissingGrantHistory.State.ProcessedGrants.RemoveAt(0);
    const FKnowledgeGrantResult MissingHistoryRestore = Rejected->RestoreSnapshot(
        MissingGrantHistory, Registry, TestPackRegistryVersion);
    TestTrue(TEXT("A granted pack without grant history is rejected"),
        !MissingHistoryRestore.bOk
        && MissingHistoryRestore.Error == TEXT("snapshot_grant_history_incomplete"));

    FKnowledgeGrantSnapshot OrphanGrantHistory = Snapshot;
    OrphanGrantHistory.State.ProcessedGrants[0].PackId = TEXT("snapshot.unknown");
    const FKnowledgeGrantResult OrphanHistoryRestore = Rejected->RestoreSnapshot(
        OrphanGrantHistory, Registry, TestPackRegistryVersion);
    TestTrue(TEXT("Grant history cannot refer to a pack outside the authorized state"),
        !OrphanHistoryRestore.bOk
        && OrphanHistoryRestore.Error == TEXT("snapshot_grant_pack_mismatch"));

    TestTrue(TEXT("Every failed restore preserves the pre-existing granted pack"),
        Rejected->HasKnowledgePack(Premise->PackId)
        && !Rejected->HasKnowledgePack(Result->PackId));
    TestEqual(TEXT("Every failed restore preserves revision"),
        Rejected->GetState().Revision, RejectedBaseline.Revision);
    TestEqual(TEXT("Every failed restore preserves grant history"),
        Rejected->GetState().ProcessedGrants.Num(), RejectedBaseline.ProcessedGrants.Num());

    UKnowledgeGrantComponent* EpochTarget = MakeGrantComponent();
    TestTrue(TEXT("Epoch fixture grants the premise"), EpochTarget->GrantKnowledgeFromWorldEvent(
        TEXT("snapshot:epoch-target:1"), Premise->SourceWorldEventId, Premise).bOk);
    const FDialogueKnowledgeView BeforeRestore = UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(
        EpochTarget, Registry, FGameplayTagContainer());
    FKnowledgeGrantSnapshot SameRevisionReplacement = Snapshot;
    SameRevisionReplacement.State.Revision = BeforeRestore.PlayerRevision;
    const FKnowledgeGrantResult Replacement = EpochTarget->RestoreSnapshot(
        SameRevisionReplacement, Registry, TestPackRegistryVersion);
    TestTrue(TEXT("Restore advances revision even when the snapshot carries the current revision"),
        Replacement.bOk && Replacement.Revision > BeforeRestore.PlayerRevision);
    const FKnowledgeDialogueGuardResult OldViewAfterRestore =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            EpochTarget, BeforeRestore, FGameplayTagContainer(), {}, MakeGuardPolicy());
    TestTrue(TEXT("A pre-restore view is rejected after same-revision authorization replacement"),
        !OldViewAfterRestore.bOk && OldViewAfterRestore.Error == TEXT("dialogue_view_stale"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKnowledgeCausalChainTest,
    "MyAiTown.KnowledgeGrant.Runtime.CausalChain",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnowledgeCausalChainTest::RunTest(const FString& Parameters)
{
    UKnowledgePack* Premise = MakeWorldEventPack(
        TEXT("caravan.public_report"), TEXT("event.caravan.reported"), TEXT("caravan_missing"),
        TEXT("A northern caravan is overdue. The town knows only that an investigation is available."));
    UKnowledgePack* Wreck = MakeWorldEventPack(
        TEXT("caravan.wreck_examined"), TEXT("event.wreck.examined"), TEXT("royal_seal_found"),
        TEXT("The player examined the burned wreck and found a royal seal."), {Premise->PackId});
    UKnowledgePack* Bell = MakeWorldEventPack(
        TEXT("caravan.bell_decoded"), TEXT("event.bell.decoded"), TEXT("control_bell_truth"),
        TEXT("The recovered bronze fragment belongs to a bell that can control the dragon."), {Wreck->PackId});
    TArray<UKnowledgePack*> AllPacks{Premise, Wreck, Bell};
    UKnowledgeGrantComponent* Player = MakeGrantComponent();

    const FKnowledgeGrantResult PremiseGrant = Player->GrantKnowledgeFromWorldEvent(
        TEXT("world:caravan-report:1"), TEXT("event.caravan.reported"), Premise);
    TestTrue(TEXT("NPC context can know the public task premise"), PremiseGrant.bOk && PremiseGrant.bApplied);
    TestEqual(TEXT("Knowing the premise exposes only one already-true fact"),
        UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(Player, AllPacks, FGameplayTagContainer()).AllowedDocuments.Num(), 1);

    const FKnowledgeGrantResult PrematureBell = Player->GrantKnowledgeFromWorldEvent(
        TEXT("world:bell-decode:premature"), TEXT("event.bell.decoded"), Bell);
    TestFalse(TEXT("A later domino cannot fall before its prerequisite"), PrematureBell.bOk);
    TestEqual(TEXT("Out-of-order rejection is explicit"), PrematureBell.Error, FString(TEXT("knowledge_prerequisite_missing")));
    TestEqual(TEXT("Rejected future event leaks no future document"),
        UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(Player, AllPacks, FGameplayTagContainer()).AllowedDocuments.Num(), 1);

    const FKnowledgeGrantResult WreckGrant = Player->GrantKnowledgeFromWorldEvent(
        TEXT("player:examined-wreck:1"), TEXT("event.wreck.examined"), Wreck);
    TestTrue(TEXT("Player action pushes the next factual domino"), WreckGrant.bOk && WreckGrant.bApplied);
    TestEqual(TEXT("Only the observed wreck fact is added"),
        UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(Player, AllPacks, FGameplayTagContainer()).AllowedDocuments.Num(), 2);

    const FKnowledgeGrantResult BellGrant = Player->GrantKnowledgeFromWorldEvent(
        TEXT("player:decoded-bell:1"), TEXT("event.bell.decoded"), Bell);
    TestTrue(TEXT("The final fact unlocks only after the prior event"), BellGrant.bOk && BellGrant.bApplied);
    TestEqual(TEXT("All three facts exist only after the causal chain completes"),
        UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(Player, AllPacks, FGameplayTagContainer()).AllowedDocuments.Num(), 3);

    const FKnowledgeGrantResult DependentRevoke = Player->RevokeKnowledgePack(Wreck->PackId, AllPacks);
    TestTrue(TEXT("A pack with a still-granted dependent cannot be revoked"),
        !DependentRevoke.bOk && DependentRevoke.Error == TEXT("pack_has_dependents"));
    const FKnowledgeGrantResult RevokeBell = Player->RevokeKnowledgePack(Bell->PackId, AllPacks);
    TestTrue(TEXT("A leaf pack can be revoked"), RevokeBell.bOk && RevokeBell.bApplied);
    TestEqual(TEXT("Revoking the leaf removes only its document"),
        UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(Player, AllPacks, FGameplayTagContainer()).AllowedDocuments.Num(), 2);
    const FKnowledgeGrantResult RevokeWreck = Player->RevokeKnowledgePack(Wreck->PackId, AllPacks);
    TestTrue(TEXT("The former dependent can be revoked after its child"), RevokeWreck.bOk);
    TestEqual(TEXT("Revoking both future packs returns to the premise"),
        UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(Player, AllPacks, FGameplayTagContainer()).AllowedDocuments.Num(), 1);
    const FKnowledgeGrantResult RegrantBell = Player->GrantKnowledgeFromWorldEvent(
        TEXT("player:decoded-bell:1"), Bell->SourceWorldEventId, Bell);
    TestFalse(TEXT("A revoked pack cannot be re-granted before its prerequisite"), RegrantBell.bOk);
    TestEqual(TEXT("Failed re-grant restores its explicit prerequisite error"),
        RegrantBell.Error, FString(TEXT("knowledge_prerequisite_missing")));

    UKnowledgeGrantComponent* Capped = MakeGrantComponent();
    Capped->MaxProcessedGrantRecords = 1;
    TestTrue(TEXT("Capped history accepts the first grant"), Capped->GrantKnowledgeFromWorldEvent(
        TEXT("capped:premise"), Premise->SourceWorldEventId, Premise).bOk);
    const FKnowledgeGrantResult CappedSecond = Capped->GrantKnowledgeFromWorldEvent(
        TEXT("capped:wreck"), Wreck->SourceWorldEventId, Wreck);
    TestTrue(TEXT("A full grant history fails closed instead of dropping history"),
        !CappedSecond.bOk && CappedSecond.Error == TEXT("grant_history_full"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKnowledgePersonalityReactionTest,
    "MyAiTown.KnowledgeGrant.Reaction.PersonalityAndAffect",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnowledgePersonalityReactionTest::RunTest(const FString& Parameters)
{
    UKnowledgePack* Pack = MakeTestPack();
    UNpcPersonalityProfile* Aggressive = MakeAggressivePersonality();
    UNpcPersonalityProfile* Empathic = MakeEmpathicPersonality();
    FNpcAffectContext Calm;
    Calm.Mood = TEXT("neutral");
    Calm.Activation = TEXT("calm");

    const int32 OriginalEmpathy = Empathic->Empathy;
    const FNpcKnowledgeReaction AggressiveReaction = UKnowledgeReactionLibrary::EvaluateKnowledgeReaction(Aggressive, Pack->Documents[0], Calm);
    const FNpcKnowledgeReaction EmpathicReaction = UKnowledgeReactionLibrary::EvaluateKnowledgeReaction(Empathic, Pack->Documents[0], Calm);
    TestTrue(TEXT("Both personality evaluations succeed"), AggressiveReaction.bOk && EmpathicReaction.bOk);
    TestNotEqual(TEXT("The same fact produces different dialogue tones"), AggressiveReaction.DialogueTone, EmpathicReaction.DialogueTone);
    TestNotEqual(TEXT("The same fact produces different behavior intents"), AggressiveReaction.BehaviorIntent, EmpathicReaction.BehaviorIntent);

    const FNpcKnowledgeReaction DifferentFact = UKnowledgeReactionLibrary::EvaluateKnowledgeReaction(Empathic, Pack->Documents[1], Calm);
    TestNotEqual(TEXT("Different facts produce different perceived threat"), EmpathicReaction.PerceivedThreat, DifferentFact.PerceivedThreat);

    FNpcAffectContext Tense = Calm;
    Tense.Activation = TEXT("tense");
    const FNpcKnowledgeReaction CalmReaction = UKnowledgeReactionLibrary::EvaluateKnowledgeReaction(Empathic, Pack->Documents[1], Calm);
    const FNpcKnowledgeReaction TenseReaction = UKnowledgeReactionLibrary::EvaluateKnowledgeReaction(Empathic, Pack->Documents[1], Tense);
    TestTrue(TEXT("Current tension raises reaction intensity"), TenseReaction.Intensity > CalmReaction.Intensity);
    TestEqual(TEXT("Evaluation never mutates stable personality"), Empathic->Empathy, OriginalEmpathy);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKnowledgeDelayedReceiptTest,
    "MyAiTown.KnowledgeGrant.Reaction.DelayedReceipt",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnowledgeDelayedReceiptTest::RunTest(const FString& Parameters)
{
    UKnowledgePack* Pack = MakeTestPack();
    UKnowledgeGrantComponent* PlayerA = MakeGrantComponent();
    UKnowledgeGrantComponent* PlayerB = MakeGrantComponent();
    UNpcKnowledgeReceiverComponent* Receiver = MakeReceiver(MakeEmpathicPersonality());
    FNpcAffectContext Context;
    const FString AudienceA(TEXT("player.account.a"));
    const FString AudienceB(TEXT("player.account.b"));
    const FString ReceiptEventId(TEXT("npc-guard:dragon-news"));

    UNpcKnowledgeReceiverComponent* NonAuthorityReceiver = NewObject<UNpcKnowledgeReceiverComponent>();
    NonAuthorityReceiver->PersonalityProfile = MakeEmpathicPersonality();
    const FNpcKnowledgeReceiptResult NonAuthority = NonAuthorityReceiver->ReceiveUnlockedKnowledgePack(
        AudienceA, ReceiptEventId, PlayerA, Pack, Context);
    TestTrue(TEXT("A receiver without an authoritative owner rejects mutation"),
        !NonAuthority.bOk && NonAuthority.Error == TEXT("authority_required"));

    const FNpcKnowledgeReceiptResult InvalidAudience = Receiver->ReceiveUnlockedKnowledgePack(
        TEXT(""), ReceiptEventId, PlayerA, Pack, Context);
    TestFalse(TEXT("An empty audience key is rejected"), InvalidAudience.bOk);
    TestEqual(TEXT("Invalid audience failure is explicit"),
        InvalidAudience.Error, FString(TEXT("audience_id_invalid")));

    const FNpcKnowledgeReceiptResult Locked = Receiver->ReceiveUnlockedKnowledgePack(
        AudienceA, ReceiptEventId, PlayerA, Pack, Context);
    TestFalse(TEXT("Locked knowledge cannot trigger a personality reaction"), Locked.bOk);
    TestEqual(TEXT("Locked receipt is explicit"), Locked.Error, FString(TEXT("knowledge_pack_locked")));
    TestFalse(TEXT("Locked receipt is not persisted"),
        Receiver->HasReceivedEventForAudience(AudienceA, ReceiptEventId));

    const FKnowledgeGrantResult GrantA = PlayerA->GrantKnowledgeFromQuest(
        TEXT("player-a:quest.ancient_dragon:complete"), TEXT("quest.ancient_dragon"), Pack);
    const FKnowledgeGrantResult GrantB = PlayerB->GrantKnowledgeFromQuest(
        TEXT("player-b:quest.ancient_dragon:complete"), TEXT("quest.ancient_dragon"), Pack);
    TestTrue(TEXT("Both players unlock before their delayed receipts"), GrantA.bOk && GrantB.bOk);
    const FNpcKnowledgeReceiptResult First = Receiver->ReceiveUnlockedKnowledgePack(
        AudienceA, ReceiptEventId, PlayerA, Pack, Context);
    TestTrue(TEXT("Unlocked pack triggers delayed reactions"), First.bOk);
    TestEqual(TEXT("Every document receives one reaction"), First.Reactions.Num(), 2);

    const FNpcKnowledgeReceiptResult OtherAudience = Receiver->ReceiveUnlockedKnowledgePack(
        AudienceB, ReceiptEventId, PlayerB, Pack, Context);
    TestTrue(TEXT("The same NPC records the same event independently for another audience"),
        OtherAudience.bOk && !OtherAudience.bDuplicate);
    TestTrue(TEXT("Both audience-scoped receipts are queryable"),
        Receiver->HasReceivedEventForAudience(AudienceA, ReceiptEventId)
        && Receiver->HasReceivedEventForAudience(AudienceB, ReceiptEventId));

    const FNpcKnowledgeReceiptResult Duplicate = Receiver->ReceiveUnlockedKnowledgePack(
        AudienceA, ReceiptEventId, PlayerA, Pack, Context);
    TestTrue(TEXT("Repeated delivery remains idempotent inside one audience"), Duplicate.bOk && Duplicate.bDuplicate);
    TestEqual(TEXT("Duplicate audience receipt does not advance revision"), Duplicate.Revision, OtherAudience.Revision);

    const FNpcKnowledgeReceiptResult TrimmedDuplicate = Receiver->ReceiveUnlockedKnowledgePack(
        TEXT("  player.account.a  "), TEXT("  npc-guard:dragon-news  "), PlayerA, Pack, Context);
    TestTrue(TEXT("Audience and receipt identifiers are normalized"),
        TrimmedDuplicate.bOk && TrimmedDuplicate.bDuplicate
        && TrimmedDuplicate.Revision == Duplicate.Revision);

    UKnowledgePack* ConflictingReceiptPack = MakeTestPack();
    ConflictingReceiptPack->PackId = TEXT("quest.ancient_dragon.receipt_conflict");
    const FNpcKnowledgeReceiptResult ReceiptConflict = Receiver->ReceiveUnlockedKnowledgePack(
        AudienceA, ReceiptEventId, PlayerA, ConflictingReceiptPack, Context);
    TestTrue(TEXT("One audience receipt event cannot silently change packs"),
        !ReceiptConflict.bOk && ReceiptConflict.Error == TEXT("receipt_event_id_conflict"));

    const FNpcKnowledgeReactionSnapshot Snapshot = Receiver->CreateReactionSnapshot();
    TestEqual(TEXT("Snapshot stores both audience-scoped receipt records"), Snapshot.Receipts.Num(), 2);
    TestEqual(TEXT("Audience A is persisted in the reaction snapshot"), Snapshot.Receipts[0].AudienceId, AudienceA);
    TestEqual(TEXT("Audience B is persisted in the reaction snapshot"), Snapshot.Receipts[1].AudienceId, AudienceB);

    UNpcKnowledgeReceiverComponent* Restored = MakeReceiver(MakeEmpathicPersonality());
    const FNpcKnowledgeReceiptResult Restore = Restored->RestoreReactionSnapshot(Snapshot);
    TestTrue(TEXT("NPC reaction memory snapshot restores"), Restore.bOk);
    TestTrue(TEXT("Restored NPC preserves both audience memories"),
        Restored->HasReceivedEventForAudience(AudienceA, ReceiptEventId)
        && Restored->HasReceivedEventForAudience(AudienceB, ReceiptEventId));

    FNpcKnowledgeReactionSnapshot LegacySnapshot = Snapshot;
    LegacySnapshot.SchemaVersion = 2;
    TestFalse(TEXT("Legacy unscoped reaction snapshots fail closed"),
        Restored->RestoreReactionSnapshot(LegacySnapshot).bOk);

    const int32 RevisionBeforeForgedRestore = Restored->CreateReactionSnapshot().Revision;
    FNpcKnowledgeReactionSnapshot ForgedReaction = Snapshot;
    ForgedReaction.Receipts[0].Reactions[0].Intensity = 1000;
    const FNpcKnowledgeReceiptResult ForgedReactionRestore = Restored->RestoreReactionSnapshot(ForgedReaction);
    TestTrue(TEXT("Out-of-range reaction values in a snapshot are rejected"),
        !ForgedReactionRestore.bOk
        && ForgedReactionRestore.Error == TEXT("reaction_snapshot_reaction_invalid"));
    TestEqual(TEXT("A failed reaction restore preserves existing state"),
        Restored->CreateReactionSnapshot().Revision, RevisionBeforeForgedRestore);

    Receiver->MaxReceiptRecords = Snapshot.Receipts.Num();
    const FNpcKnowledgeReceiptResult CappedReceipt = Receiver->ReceiveUnlockedKnowledgePack(
        TEXT("player.account.c"), TEXT("npc-guard:dragon-news-c"), PlayerA, Pack, Context);
    TestTrue(TEXT("A full receipt history fails closed instead of dropping memory"),
        !CappedReceipt.bOk && CappedReceipt.Error == TEXT("receipt_history_full"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKnowledgeSemanticReviewContractTest,
    "MyAiTown.KnowledgeGrant.Runtime.SemanticReviewContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnowledgeSemanticReviewContractTest::RunTest(const FString& Parameters)
{
    UKnowledgePack* Pack = MakeTestPack();
    UKnowledgeGrantComponent* Player = MakeGrantComponent();
    TestTrue(TEXT("Semantic review fixture knowledge is granted"),
        Player->GrantKnowledgeFromQuest(TEXT("semantic:quest:1"), Pack->SourceQuestId, Pack).bOk);
    const FDialogueKnowledgeView View = UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(
        Player, {Pack}, FGameplayTagContainer());

    FKnowledgeDialogueClaim Claim;
    Claim.ClaimId = TEXT("claim.semantic.origin");
    Claim.Text = Pack->Documents[0].Content;
    Claim.EvidenceFactIds = {FactIdForDocument(Pack->Documents[0].DocumentId)};
    const FKnowledgeDialogueGuardResult Mechanical =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, View, FGameplayTagContainer(), {Claim}, MakeGuardPolicy());
    TestTrue(TEXT("Semantic review starts only after the mechanical gate"), Mechanical.bOk);

    AActor* ReviewOwner = NewObject<AActor>();
    UKnowledgeSemanticReviewComponent* Coordinator =
        NewObject<UKnowledgeSemanticReviewComponent>(ReviewOwner);
    UKnowledgeContextCollectorComponent* Collector =
        NewObject<UKnowledgeContextCollectorComponent>(ReviewOwner);
    UKnowledgeSemanticReviewPolicy* Policy = NewObject<UKnowledgeSemanticReviewPolicy>();
    Policy->PolicyVersion = TEXT("semantic.policy.v1");
    Policy->TimeoutSeconds = 5.0f;
    Policy->BlockedText = FText::FromString(TEXT("语义审查未通过。"));
    Policy->ErrorText = FText::FromString(TEXT("语义审查暂不可用。"));

    FKnowledgeDialogueGuardResult ForeignMechanicalResult = Mechanical;
    ++ForeignMechanicalResult.ValidatedScopeFingerprint;
    TestTrue(TEXT("A mechanical result from another dialogue view cannot enter semantic review"),
        !Coordinator->BeginSemanticReview(
            Player, View, {Collector}, ForeignMechanicalResult, Policy).bOk);

    const FKnowledgeSemanticReviewBeginResult Begin = Coordinator->BeginSemanticReview(
        Player, View, {Collector}, Mechanical, Policy);
    TestTrue(TEXT("A provider-neutral semantic request is created"), Begin.bOk);
    TestEqual(TEXT("The request carries only mechanically allowed claims"),
        Begin.Request.AllowedClaimIds.Num(), 1);
    TestEqual(TEXT("The request carries plugin-assigned claim objects"),
        Begin.Request.AllowedClaims.Num(), 1);
    TestEqual(TEXT("The request carries the allowed atomic facts"),
        Begin.Request.AllowedFacts.Num(), 2);

    FKnowledgeSemanticReviewResponse ApprovedResponse;
    ApprovedResponse.RequestId = Begin.Request.RequestId;
    ApprovedResponse.ClaimAssessments.Add(MakePassingAssessment(
        Begin.Request.AllowedClaimIds[0], Claim.EvidenceFactIds));
    ApprovedResponse.ReviewerName = TEXT("test-reviewer");
    ApprovedResponse.ReviewerVersion = TEXT("1");
    ApprovedResponse.ReviewDurationMs = 37;
    const FKnowledgeSemanticReviewResult Approved =
        Coordinator->CompleteSemanticReview(ApprovedResponse);
    TestTrue(TEXT("A complete external approval releases FinalApprovedText"),
        Approved.bCompleted && Approved.bApproved
        && Approved.FinalApprovedText == Mechanical.SafeText
        && Approved.ClaimAssessments.Num() == 1
        && Approved.ReviewedClaims.Num() == 1
        && Approved.WorstClaimId == Begin.Request.AllowedClaimIds[0]
        && Approved.ReviewerName == ApprovedResponse.ReviewerName
        && Approved.ReviewerVersion == ApprovedResponse.ReviewerVersion
        && Approved.PolicyVersion == Policy->PolicyVersion
        && Approved.AuthorizationEpoch == Begin.Request.AuthorizationEpoch
        && Approved.ScopeFingerprint == Begin.Request.ScopeFingerprint
        && Approved.ReviewDurationMs == ApprovedResponse.ReviewDurationMs);
    TestEqual(TEXT("A completed request is consumed exactly once"),
        Coordinator->GetPendingReviewCount(), 0);

    const FKnowledgeSemanticReviewBeginResult MismatchBegin = Coordinator->BeginSemanticReview(
        Player, View, {Collector}, Mechanical, Policy);
    FKnowledgeSemanticReviewResponse MismatchResponse;
    MismatchResponse.RequestId = MismatchBegin.Request.RequestId;
    MismatchResponse.ReviewerName = TEXT("test-reviewer");
    MismatchResponse.ReviewerVersion = TEXT("1");
    MismatchResponse.ClaimAssessments.Add(MakePassingAssessment(
        MismatchBegin.Request.AllowedClaimIds[0],
        {FactIdForDocument(TEXT("ruined_temple_location"))}));
    const FKnowledgeSemanticReviewResult Mismatch =
        Coordinator->CompleteSemanticReview(MismatchResponse);
    TestTrue(TEXT("Reviewer evidence must remain bound to the source claim"),
        !Mismatch.bApproved
        && Mismatch.Outcome == EKnowledgeSemanticReviewOutcome::Error
        && Mismatch.Error == TEXT("semantic_review_assessment_invalid"));

    const FKnowledgeSemanticReviewBeginResult StaleBegin = Coordinator->BeginSemanticReview(
        Player, View, {Collector}, Mechanical, Policy);
    TestTrue(TEXT("A second semantic request can wait asynchronously"), StaleBegin.bOk);
    TestTrue(TEXT("A later server grant changes the authorization revision"),
        Player->GrantKnowledgeFromQuest(TEXT("semantic:quest:2"), Pack->SourceQuestId, Pack).bOk);
    FKnowledgeSemanticReviewResponse StaleResponse = ApprovedResponse;
    StaleResponse.RequestId = StaleBegin.Request.RequestId;
    const FKnowledgeSemanticReviewResult Stale =
        Coordinator->CompleteSemanticReview(StaleResponse);
    TestTrue(TEXT("An API response is rejected after authorization changes while in flight"),
        !Stale.bApproved && Stale.Error == TEXT("dialogue_view_stale")
        && Stale.FinalApprovedText == Policy->ErrorText.ToString());

    const FDialogueKnowledgeView FreshView = UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(
        Player, {Pack}, FGameplayTagContainer());
    const FKnowledgeDialogueGuardResult FreshMechanical =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, FreshView, FGameplayTagContainer(), {Claim}, MakeGuardPolicy());
    const FKnowledgeSemanticReviewBeginResult BlockBegin = Coordinator->BeginSemanticReview(
        Player, FreshView, {Collector}, FreshMechanical, Policy);
    FKnowledgeSemanticReviewResponse BlockResponse;
    BlockResponse.RequestId = BlockBegin.Request.RequestId;
    BlockResponse.ReviewerName = TEXT("test-reviewer");
    BlockResponse.ReviewerVersion = TEXT("1");
    FKnowledgeSemanticClaimAssessment LeakingAssessment = MakePassingAssessment(
        BlockBegin.Request.AllowedClaimIds[0], Claim.EvidenceFactIds);
    LeakingAssessment.TaskLeak = EKnowledgeClaimRisk::Possible;
    LeakingAssessment.TaskLeakRiskScore = 0.30f;
    BlockResponse.ClaimAssessments.Add(LeakingAssessment);
    const FKnowledgeSemanticReviewResult Blocked =
        Coordinator->CompleteSemanticReview(BlockResponse);
    TestTrue(TEXT("A semantic rejection fails closed to the trusted policy text"),
        Blocked.Outcome == EKnowledgeSemanticReviewOutcome::Blocked
        && !Blocked.bApproved
        && Blocked.FinalApprovedText == Policy->BlockedText.ToString()
        && Blocked.RejectedCandidateText == FreshMechanical.SafeText
        && Blocked.ClaimAssessments.Num() == 1
        && Blocked.ClaimAssessments[0].TaskLeak == EKnowledgeClaimRisk::Possible
        && Blocked.ReviewedClaims.Num() == 1
        && Blocked.ReviewedClaims[0].Text == Claim.Text
        && Blocked.WorstClaimId == BlockBegin.Request.AllowedClaimIds[0]);

    UKnowledgeDialogueGuardPolicy* RoleplayGuard = MakeGuardPolicy();
    RoleplayGuard->bAllowUncitedRoleplay = true;
    FKnowledgeDialogueClaim Advice;
    Advice.ClaimId = TEXT("claim.semantic.advice");
    Advice.Text = TEXT("先检查脚印，再决定从哪里查起。");
    const FKnowledgeDialogueGuardResult AdviceMechanical =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, FreshView, FGameplayTagContainer(), {Advice}, RoleplayGuard);
    const FKnowledgeSemanticReviewBeginResult AdviceBegin = Coordinator->BeginSemanticReview(
        Player, FreshView, {Collector}, AdviceMechanical, Policy);
    FKnowledgeSemanticReviewResponse AdviceResponse;
    AdviceResponse.RequestId = AdviceBegin.Request.RequestId;
    AdviceResponse.ReviewerName = TEXT("test-reviewer");
    AdviceResponse.ReviewerVersion = TEXT("1");
    FKnowledgeSemanticClaimAssessment AdviceAssessment = MakePassingAssessment(
        AdviceBegin.Request.AllowedClaimIds[0], {});
    AdviceAssessment.Support = EKnowledgeClaimSupport::Unsupported;
    AdviceAssessment.FactSupportScore = 0.0f;
    AdviceResponse.ClaimAssessments.Add(AdviceAssessment);
    const FKnowledgeSemanticReviewResult AdviceApproved =
        Coordinator->CompleteSemanticReview(AdviceResponse);
    TestTrue(TEXT("Safe non-factual advice does not require factual support"),
        AdviceApproved.bApproved
        && AdviceApproved.FinalApprovedText == AdviceMechanical.SafeText);

    const FKnowledgeSemanticReviewBeginResult AdviceLeakBegin = Coordinator->BeginSemanticReview(
        Player, FreshView, {Collector}, AdviceMechanical, Policy);
    FKnowledgeSemanticReviewResponse AdviceLeakResponse = AdviceResponse;
    AdviceLeakResponse.RequestId = AdviceLeakBegin.Request.RequestId;
    AdviceLeakResponse.ClaimAssessments[0].TaskLeak = EKnowledgeClaimRisk::Possible;
    AdviceLeakResponse.ClaimAssessments[0].TaskLeakRiskScore = 0.3f;
    const FKnowledgeSemanticReviewResult AdviceLeakBlocked =
        Coordinator->CompleteSemanticReview(AdviceLeakResponse);
    TestTrue(TEXT("Non-factual advice still cannot leak future task information"),
        !AdviceLeakBlocked.bApproved
        && AdviceLeakBlocked.Outcome == EKnowledgeSemanticReviewOutcome::Blocked);

    const FKnowledgeSemanticReviewBeginResult TypeMismatchBegin = Coordinator->BeginSemanticReview(
        Player, FreshView, {Collector}, FreshMechanical, Policy);
    FKnowledgeSemanticReviewResponse TypeMismatchResponse = ApprovedResponse;
    TypeMismatchResponse.RequestId = TypeMismatchBegin.Request.RequestId;
    TypeMismatchResponse.ClaimAssessments[0].ClaimType = EKnowledgeClaimType::NonFactual;
    TypeMismatchResponse.ClaimAssessments[0].EvidenceFactIds.Reset();
    const FKnowledgeSemanticReviewResult TypeMismatch =
        Coordinator->CompleteSemanticReview(TypeMismatchResponse);
    TestTrue(TEXT("A factual claim cannot bypass evidence by being relabeled non-factual"),
        !TypeMismatch.bApproved
        && TypeMismatch.Error == TEXT("semantic_review_assessment_invalid"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAiNpcProviderContractTest,
    "MyAiTown.KnowledgeGrant.Runtime.AiNpcProviderContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAiNpcProviderContractTest::RunTest(const FString& Parameters)
{
    FAiNpcProviderConfiguration Configuration;
    Configuration.ProviderName = TEXT("test-provider");
    Configuration.ChatCompletionsUrl = TEXT("https://example.invalid/v1/chat/completions");
    Configuration.Model = TEXT("test-model");
    FString Error;
    TestFalse(TEXT("Remote transmission requires explicit player consent"),
        UAiNpcProviderSubsystem::ValidateConfiguration(Configuration, Error));
    TestEqual(TEXT("Consent failure is explicit"), Error,
        FString(TEXT("provider_transmission_consent_required")));
    Configuration.bPlayerConsentedToProviderTransmission = true;
    TestTrue(TEXT("A consented HTTPS provider configuration is accepted"),
        UAiNpcProviderSubsystem::ValidateConfiguration(Configuration, Error));

    FAiNpcProviderConfiguration Unsafe = Configuration;
    Unsafe.ChatCompletionsUrl = TEXT("http://vendor.example/v1/chat/completions");
    TestFalse(TEXT("Plain HTTP to a remote vendor is rejected"),
        UAiNpcProviderSubsystem::ValidateConfiguration(Unsafe, Error));
    Unsafe.ChatCompletionsUrl = TEXT("http://localhost.com/v1/chat/completions");
    TestFalse(TEXT("A localhost prefix on a remote hostname cannot bypass HTTPS"),
        UAiNpcProviderSubsystem::ValidateConfiguration(Unsafe, Error));
    Unsafe.ChatCompletionsUrl = TEXT("http://localhost@vendor.example/v1/chat/completions");
    TestFalse(TEXT("URL user-info cannot disguise a remote hostname as loopback"),
        UAiNpcProviderSubsystem::ValidateConfiguration(Unsafe, Error));
    Unsafe.ChatCompletionsUrl = TEXT("http://127.42.0.7:1234/v1/chat/completions");
    TestTrue(TEXT("The explicit IPv4 loopback range remains available for local models"),
        UAiNpcProviderSubsystem::ValidateConfiguration(Unsafe, Error));

    UGameInstance* TestGameInstance = NewObject<UGameInstance>();
    UAiNpcProviderSubsystem* ProviderGateway = NewObject<UAiNpcProviderSubsystem>(TestGameInstance);
    TestTrue(TEXT("The default dialogue provider can be configured in memory"),
        ProviderGateway->ConfigureProvider(EAiNpcProviderChannel::DialogueGeneration,
            Configuration, TEXT("dialogue-key"), Error));
    TestFalse(TEXT("The default policy rejects a correlated reviewer endpoint and model"),
        ProviderGateway->ConfigureProvider(EAiNpcProviderChannel::SemanticReview,
            Configuration, TEXT("review-key"), Error));
    TestEqual(TEXT("The independent-review failure is explicit"), Error,
        FString(TEXT("review_provider_not_independent")));

    FAiNpcProviderConfiguration DemoJson = Configuration;
    DemoJson.bRequestJsonObjectResponse = true;
    FAiNpcPersona DemoJsonPersona;
    DemoJsonPersona.NpcId = TEXT("npc.demo-json");
    DemoJsonPersona.DisplayName = TEXT("Demo JSON NPC");
    DemoJsonPersona.RoleInstructions = TEXT("Return authorized facts only.");
    FString DemoJsonBody;
    TestTrue(TEXT("The integrity demo can request JSON object response mode"),
        UAiNpcProviderSubsystem::BuildDialogueRequestBody(DemoJson, DemoJsonPersona,
            TEXT("hello"), TEXT("facts"), DemoJsonBody, Error));
    TestTrue(TEXT("The demo JSON request includes an explicit response format"),
        DemoJsonBody.Contains(TEXT("\"response_format\""))
        && DemoJsonBody.Contains(TEXT("\"json_object\"")));
    TestFalse(TEXT("Generic OpenAI-compatible JSON requests do not assume vendor thinking controls"),
        DemoJsonBody.Contains(TEXT("\"thinking\"")));
    DemoJson.bDisableThinkingForJsonContract = true;
    TestTrue(TEXT("Strict JSON providers can explicitly disable thinking mode"),
        UAiNpcProviderSubsystem::BuildDialogueRequestBody(DemoJson, DemoJsonPersona,
            TEXT("hello"), TEXT("facts"), DemoJsonBody, Error)
        && DemoJsonBody.Contains(TEXT("\"thinking\""))
        && DemoJsonBody.Contains(TEXT("\"disabled\"")));
    DemoJsonPersona.bAllowUncitedRoleplay = true;
    TestTrue(TEXT("An opted-in persona receives the bounded roleplay fallback contract"),
        UAiNpcProviderSubsystem::BuildDialogueRequestBody(DemoJson, DemoJsonPersona,
            TEXT("What should we do?"), TEXT("facts"), DemoJsonBody, Error)
        && DemoJsonBody.Contains(TEXT("evidenceFactIds=[]"))
        && DemoJsonBody.Contains(TEXT("instead of saying that evidence is insufficient")));
    FAiNpcProviderConfiguration IndependentReview = Configuration;
    IndependentReview.ProviderName = TEXT("independent-reviewer");
    IndependentReview.ChatCompletionsUrl = TEXT("https://review.example.invalid/v1/chat/completions");
    IndependentReview.Model = TEXT("review-model");
    TestTrue(TEXT("A distinct review endpoint and model are accepted"),
        ProviderGateway->ConfigureProvider(EAiNpcProviderChannel::SemanticReview,
            IndependentReview, TEXT("review-key"), Error));

    FAiNpcProviderConfiguration NpcProvider = Configuration;
    NpcProvider.ProviderName = TEXT("rowan-provider");
    NpcProvider.ChatCompletionsUrl = TEXT("https://rowan.example.invalid/v1/chat/completions");
    TestTrue(TEXT("An NPC can own an isolated in-memory provider profile"),
        ProviderGateway->ConfigureNpcProvider(TEXT("npc.rowan"), NpcProvider,
            TEXT("rowan-key"), Error));
    TestTrue(TEXT("The isolated NPC provider reports readiness without exposing its key"),
        ProviderGateway->IsNpcProviderReady(TEXT("npc.rowan")));
    FAiNpcProviderConfiguration MiraProvider = Configuration;
    MiraProvider.ProviderName = TEXT("mira-provider");
    MiraProvider.ChatCompletionsUrl = TEXT("https://mira.example.invalid/v1/chat/completions");
    TestTrue(TEXT("A second group member can own another provider profile"),
        ProviderGateway->ConfigureNpcProvider(TEXT("npc.mira"), MiraProvider,
            TEXT("mira-key"), Error));
    TestTrue(TEXT("One group member credential can be cleared independently"),
        ProviderGateway->ClearNpcProviderCredentials(TEXT("npc.rowan")));
    TestFalse(TEXT("The cleared group member becomes unavailable"),
        ProviderGateway->IsNpcProviderReady(TEXT("npc.rowan")));
    TestTrue(TEXT("Clearing one group member leaves the other route ready"),
        ProviderGateway->IsNpcProviderReady(TEXT("npc.mira")));
    TestTrue(TEXT("The shared reviewer can be cleared without disconnecting NPC routes"),
        ProviderGateway->ClearProviderCredentials(EAiNpcProviderChannel::SemanticReview));
    TestFalse(TEXT("The cleared reviewer becomes unavailable"),
        ProviderGateway->IsProviderReady(EAiNpcProviderChannel::SemanticReview));
    TestTrue(TEXT("Clearing the reviewer leaves the remaining NPC route ready"),
        ProviderGateway->IsNpcProviderReady(TEXT("npc.mira")));

    FAiNpcPersona Persona;
    Persona.NpcId = TEXT("npc.rowan");
    Persona.DisplayName = TEXT("Rowan");
    Persona.RoleInstructions = TEXT("Remain terse and protective.");
    FString Body;
    TestTrue(TEXT("The provider request is deterministic and contains only current-turn inputs"),
        UAiNpcProviderSubsystem::BuildDialogueRequestBody(
            Configuration, Persona, TEXT("What happened?"),
            TEXT("FactId=fact.bell | The bell was found."), Body, Error));
    TestTrue(TEXT("Role instructions are disclosed to the selected provider"),
        Body.Contains(Persona.RoleInstructions));
    TestTrue(TEXT("Authorized turn facts are disclosed to the selected provider"),
        Body.Contains(TEXT("fact.bell")));
    TestTrue(TEXT("The current player message is disclosed to the selected provider"),
        Body.Contains(TEXT("What happened?")));
    TestFalse(TEXT("No API key field exists in the serialized body"),
        Body.Contains(TEXT("apiKey"), ESearchCase::IgnoreCase));

    FString RepairBody;
    TestTrue(TEXT("A malformed candidate can receive one deterministic JSON-repair request"),
        UAiNpcProviderSubsystem::BuildDialogueRepairRequestBody(
            Configuration, Persona, TEXT("```json { broken } ```"),
            TEXT("FactId=fact.bell | The bell was found."), RepairBody, Error));
    TestTrue(TEXT("The repair pass is temperature zero and retains authorized facts"),
        RepairBody.Contains(TEXT("temperature"))
        && RepairBody.Contains(TEXT("fact.bell"))
        && RepairBody.Contains(TEXT("dialogue_claim_number_mismatch"))
        && RepairBody.Contains(TEXT("dialogue_claim_uncited_roleplay_unsafe")));

    const FString ProviderResponse = TEXT(
        "{\"choices\":[{\"message\":{\"content\":\"{\\\"claims\\\":[]}\"}}]}");
    FString Payload;
    TestTrue(TEXT("An OpenAI-compatible response yields only its untrusted content"),
        UAiNpcProviderSubsystem::ParseDialogueResponse(ProviderResponse, Payload, Error));
    TestEqual(TEXT("The strict JSON candidate is preserved for downstream guards"),
        Payload, FString(TEXT("{\"claims\":[]}")));
    TestFalse(TEXT("A non-JSON provider response is rejected deterministically"),
        UAiNpcProviderSubsystem::ParseDialogueResponse(TEXT("not-json"), Payload, Error));
    TestEqual(TEXT("Non-JSON dialogue failure has a stable diagnostic code"),
        Error, FString(TEXT("provider_response_not_json")));
    TestFalse(TEXT("A truncated provider envelope is rejected deterministically"),
        UAiNpcProviderSubsystem::ParseDialogueResponse(
            TEXT("{\"choices\":[{\"message\":{"), Payload, Error));
    TestEqual(TEXT("Truncated dialogue JSON uses the same bounded diagnostic"),
        Error, FString(TEXT("provider_response_not_json")));

    FKnowledgeSemanticReviewRequest ReviewRequest;
    ReviewRequest.RequestId = TEXT("review-1");
    ReviewRequest.MechanicallyValidatedText = TEXT("The bell was found.");
    ReviewRequest.AllowedClaimIds.Add(TEXT("claim-1"));
    FKnowledgeDialogueClaim ReviewClaim;
    ReviewClaim.ClaimId = TEXT("claim-1");
    ReviewClaim.Text = ReviewRequest.MechanicallyValidatedText;
    ReviewClaim.EvidenceFactIds.Add(TEXT("fact.bell"));
    ReviewRequest.AllowedClaims.Add(ReviewClaim);
    FKnowledgeFact Fact;
    Fact.FactId = TEXT("fact.bell");
    Fact.Statement = TEXT("The bell was found.");
    ReviewRequest.AllowedFacts.Add(Fact);
    FString ReviewRetryBody;
    TestTrue(TEXT("A reviewer format retry receives its trusted local validation code"),
        UAiNpcProviderSubsystem::BuildReviewRequestBody(
            Configuration, ReviewRequest, ReviewRetryBody, Error,
            TEXT("review_claim_coverage_invalid"))
        && ReviewRetryBody.Contains(TEXT("review_claim_coverage_invalid")));
    FKnowledgeSemanticReviewResponse ParsedReview;
    const FString ReviewResponse = TEXT(
        "{\"choices\":[{\"message\":{\"content\":\"{\\\"claimAssessments\\\":[{\\\"claimId\\\":\\\"claim-1\\\",\\\"claimType\\\":\\\"factual\\\",\\\"support\\\":\\\"supported\\\",\\\"contradiction\\\":\\\"none\\\",\\\"overreach\\\":\\\"none\\\",\\\"taskLeak\\\":\\\"none\\\",\\\"roleFit\\\":\\\"good\\\",\\\"confidence\\\":\\\"high\\\",\\\"evidenceFactIds\\\":[\\\"fact.bell\\\"],\\\"factSupportScore\\\":0.95,\\\"contradictionRiskScore\\\":0.01,\\\"overreachRiskScore\\\":0.01,\\\"taskLeakRiskScore\\\":0.01,\\\"roleFitScore\\\":0.9,\\\"confidenceScore\\\":0.95}]}\"}}]}");
    TestTrue(TEXT("The reviewer response accepts only the per-claim gate contract"),
        UAiNpcProviderSubsystem::ParseReviewResponse(
            ReviewResponse, ReviewRequest, Configuration, ParsedReview, Error));
    TestTrue(TEXT("Reviewer identity is supplied by trusted local configuration"),
        ParsedReview.ClaimAssessments.Num() == 1
        && ParsedReview.ReviewerName == Configuration.ProviderName
        && ParsedReview.ReviewerVersion == Configuration.Model);

    FKnowledgeSemanticReviewResponse MissingCoverage;
    const FString MissingCoverageResponse = TEXT(
        "{\"choices\":[{\"message\":{\"content\":\"{\\\"claimAssessments\\\":[]}\"}}]}");
    TestFalse(TEXT("A reviewer response that omits an input claim ID is rejected for bounded retry"),
        UAiNpcProviderSubsystem::ParseReviewResponse(
            MissingCoverageResponse, ReviewRequest, Configuration, MissingCoverage, Error));
    TestEqual(TEXT("Review coverage failure has a stable diagnostic code"),
        Error, FString(TEXT("review_claim_coverage_invalid")));

    FKnowledgeSemanticReviewResponse InvalidScore;
    const FString InvalidScoreResponse = ReviewResponse.Replace(TEXT("0.95}"), TEXT("2.0}"));
    TestFalse(TEXT("An out-of-range reviewer score is rejected for bounded retry"),
        UAiNpcProviderSubsystem::ParseReviewResponse(
            InvalidScoreResponse, ReviewRequest, Configuration, InvalidScore, Error));
    TestEqual(TEXT("Invalid reviewer score has a stable diagnostic code"),
        Error, FString(TEXT("review_score_invalid")));
    FKnowledgeSemanticReviewResponse TruncatedReview;
    TestFalse(TEXT("A truncated reviewer response is rejected before any gate decision"),
        UAiNpcProviderSubsystem::ParseReviewResponse(
            TEXT("{\"choices\":[{\"message\":{"), ReviewRequest,
            Configuration, TruncatedReview, Error));
    TestEqual(TEXT("Truncated reviewer JSON has a stable diagnostic code"),
        Error, FString(TEXT("provider_response_not_json")));
    TestTrue(TEXT("Timeout or disconnect failures receive one bounded retry"),
        UAiNpcProviderSubsystem::IsRetryableReviewErrorCode(TEXT("review_connection_failed")));
    TestTrue(TEXT("HTTP 429 receives one bounded retry"),
        UAiNpcProviderSubsystem::IsRetryableReviewErrorCode(TEXT("review_http_error_429")));
    TestTrue(TEXT("Invalid reviewer JSON receives one bounded format retry"),
        UAiNpcProviderSubsystem::IsRetryableReviewErrorCode(TEXT("provider_response_not_json")));
    TestFalse(TEXT("Permanent HTTP 400 failures are not retried"),
        UAiNpcProviderSubsystem::IsRetryableReviewErrorCode(TEXT("review_http_error_400")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FQuestAuthorityBlueprintDispatchTest,
    "MyAiTown.KnowledgeGrant.Runtime.QuestAuthorityBlueprintDispatch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FQuestAuthorityBlueprintDispatchTest::RunTest(const FString& Parameters)
{
    UKnowledgeQuestAuthorityDispatchProbe* Probe =
        NewObject<UKnowledgeQuestAuthorityDispatchProbe>();
    FString StablePlayerId;
    int32 CompletionRevision = 0;
    bool bDebugEvent = true;
    const bool bCompleted = UArpgKnowledgeGrantAdapterComponent::ResolveQuestAuthority(
        Probe, nullptr, TEXT("quest.dispatch.probe"),
        StablePlayerId, CompletionRevision, bDebugEvent);

    TestTrue(TEXT("Quest authority dispatch succeeds through the interface contract"), bCompleted);
    TestEqual(TEXT("The generated Execute_ thunk routes through ProcessEvent"),
        Probe->ProcessEventCallCount, 1);
    TestEqual(TEXT("The native implementation is invoked exactly once"),
        Probe->ImplementationCallCount, 1);
    TestEqual(TEXT("Dispatch preserves provider outputs"),
        StablePlayerId, FString(TEXT("dispatch-probe-player")));
    TestEqual(TEXT("Dispatch preserves the completion revision"), CompletionRevision, 7);
    TestFalse(TEXT("Dispatch preserves the debug-event result"), bDebugEvent);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKnowledgeSemanticRequestCorrelationTest,
    "MyAiTown.KnowledgeGrant.Runtime.SemanticRequestCorrelation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnowledgeSemanticRequestCorrelationTest::RunTest(const FString& Parameters)
{
    UAiNpcDialogueComponent* First = NewObject<UAiNpcDialogueComponent>();
    UAiNpcDialogueComponent* Second = NewObject<UAiNpcDialogueComponent>();
    const FGuid FirstDialogue = FGuid::NewGuid();
    const FGuid SecondDialogue = FGuid::NewGuid();
    const FString FirstReview = FGuid::NewGuid().ToString();
    const FString SecondReview = FGuid::NewGuid().ToString();

    TestTrue(TEXT("The first component records its review at the call site"),
        First->CorrelateSemanticRequest(FirstReview, FirstDialogue));
    TestTrue(TEXT("The second component records an interleaved review independently"),
        Second->CorrelateSemanticRequest(SecondReview, SecondDialogue));
    TestEqual(TEXT("The first review remains paired with the first dialogue"),
        First->SemanticToProviderRequest.FindRef(FirstReview), FirstDialogue);
    TestEqual(TEXT("The second review remains paired with the second dialogue"),
        Second->SemanticToProviderRequest.FindRef(SecondReview), SecondDialogue);
    TestFalse(TEXT("A review id cannot be rebound after correlation"),
        First->CorrelateSemanticRequest(FirstReview, SecondDialogue));
    TestFalse(TEXT("One component cannot observe another component's review mapping"),
        First->SemanticToProviderRequest.Contains(SecondReview));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKnowledgeRequestIdempotencyTest,
    "MyAiTown.KnowledgeGrant.Runtime.RequestIdempotency",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnowledgeRequestIdempotencyTest::RunTest(const FString& Parameters)
{
    UAiNpcDialogueComponent* Dialogue = NewObject<UAiNpcDialogueComponent>();
    const FGuid ClientRequestId = FGuid::NewGuid();
    const FGuid ProviderRequestId = FGuid::NewGuid();
    FDialogueKnowledgeView View;
    View.AuthorizationEpoch = FGuid::NewGuid();
    View.ScopeFingerprint = 4242;

    UAiNpcDialogueComponent::FClientRequestRecord Record;
    Record.InputHash = TEXT("input-hash");
    Record.AuthorizationEpoch = View.AuthorizationEpoch;
    Record.ScopeFingerprint = View.ScopeFingerprint;
    Record.OriginalRequestId = ProviderRequestId;
    Dialogue->ClientRequests.Add(ClientRequestId, Record);

    FGuid ResolvedRequestId;
    FString Error;
    TestTrue(TEXT("A repeated in-flight request is intercepted before provider dispatch"),
        Dialogue->TryResolveExistingClientRequest(
            ClientRequestId, TEXT("input-hash"), View, ResolvedRequestId, Error));
    TestEqual(TEXT("Pending duplicates reuse the original provider request"),
        ResolvedRequestId, ProviderRequestId);
    TestEqual(TEXT("Pending duplicate status is explicit"),
        Error, FString(TEXT("request_duplicate_pending")));

    Dialogue->ClientRequests[ClientRequestId].bCompleted = true;
    TestTrue(TEXT("A repeated completed request is served from the request record"),
        Dialogue->TryResolveExistingClientRequest(
            ClientRequestId, TEXT("input-hash"), View, ResolvedRequestId, Error));
    TestEqual(TEXT("Completed duplicate status is explicit"),
        Error, FString(TEXT("request_duplicate_completed")));

    TestTrue(TEXT("Reusing an id with another input is handled as a terminal conflict"),
        Dialogue->TryResolveExistingClientRequest(
            ClientRequestId, TEXT("other-input"), View, ResolvedRequestId, Error));
    TestEqual(TEXT("Conflicting id reuse cannot inherit an old result"),
        Error, FString(TEXT("client_request_conflict")));
    TestFalse(TEXT("A conflict does not expose a reusable provider request id"),
        ResolvedRequestId.IsValid());

    Dialogue->MaxClientRequestRecords = 2;
    Dialogue->ClientRequests.Reset();
    TArray<FGuid> CachedIds;
    for (int32 Index = 0; Index < 3; ++Index)
    {
        const FGuid Id = FGuid::NewGuid();
        UAiNpcDialogueComponent::FClientRequestRecord Cached;
        Cached.bCompleted = true;
        Cached.CompletedAtSeconds = static_cast<double>(Index);
        Cached.CompletedReply.FinalApprovedText = FString::Printf(TEXT("cached-%d"), Index);
        Dialogue->ClientRequests.Add(Id, Cached);
        CachedIds.Add(Id);
    }
    Dialogue->PruneCompletedClientRequests(2);
    TestEqual(TEXT("Completed idempotency records are capped"), Dialogue->ClientRequests.Num(), 2);
    TestFalse(TEXT("The oldest completed record is pruned first"),
        Dialogue->ClientRequests.Contains(CachedIds[0]));
    TestTrue(TEXT("The newest completed record remains available"),
        Dialogue->ClientRequests.Contains(CachedIds[2]));

    FString ReplayError;
    TestTrue(TEXT("A cached completed reply can be replayed"),
        Dialogue->ReplayClientRequestResult(CachedIds[2], ReplayError)
        && ReplayError.IsEmpty());
    TestFalse(TEXT("Replaying a missing or pending result fails closed"),
        Dialogue->ReplayClientRequestResult(FGuid::NewGuid(), ReplayError)
        && ReplayError.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKnowledgeStaleResponseRejectionTest,
    "MyAiTown.KnowledgeGrant.Runtime.StaleResponseRejection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnowledgeStaleResponseRejectionTest::RunTest(const FString& Parameters)
{
    UKnowledgePack* InitialPack = MakeTestPack();
    UKnowledgeGrantComponent* Player = MakeGrantComponent();
    TestTrue(TEXT("The stale-response fixture grants its initial pack"),
        Player->GrantKnowledgeFromQuest(
            TEXT("stale:quest:1"), InitialPack->SourceQuestId, InitialPack).bOk);
    const FDialogueKnowledgeView View = UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(
        Player, {InitialPack}, FGameplayTagContainer());

    UAiNpcDialogueComponent* Dialogue = NewObject<UAiNpcDialogueComponent>();
    Dialogue->PlayerKnowledge = Player;
    UAiNpcDialogueComponent::FPendingTurn Turn;
    Turn.KnowledgeView = View;
    Turn.PersonaRevision = Dialogue->PersonaRevision;
    FString Error;
    TestTrue(TEXT("A response remains current while authorization and persona are unchanged"),
        Dialogue->IsPendingTurnCurrent(Turn, Error));

    UKnowledgePack* LaterPack = MakeWorldEventPack(
        TEXT("event.after.request"), TEXT("event.after.request"),
        TEXT("later_fact"), TEXT("A later event changed the authorized view."));
    TestTrue(TEXT("A later grant changes the authorization epoch"),
        Player->GrantKnowledgeFromWorldEvent(
            TEXT("stale:event:2"), LaterPack->SourceWorldEventId, LaterPack).bOk);
    TestFalse(TEXT("A response captured before the later grant is rejected"),
        Dialogue->IsPendingTurnCurrent(Turn, Error));
    TestEqual(TEXT("Authorization changes use the stable stale-view error"),
        Error, FString(TEXT("dialogue_view_stale")));

    Turn.KnowledgeView = UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(
        Player, {InitialPack, LaterPack}, FGameplayTagContainer());
    ++Turn.PersonaRevision;
    TestFalse(TEXT("A response from a previous persona revision is rejected"),
        Dialogue->IsPendingTurnCurrent(Turn, Error));
    TestEqual(TEXT("Persona changes have an explicit stale-response error"),
        Error, FString(TEXT("dialogue_persona_changed")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKnowledgeMultiProviderRoutingTest,
    "MyAiTown.KnowledgeGrant.Runtime.MultiProviderRouting",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnowledgeMultiProviderRoutingTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    UAiNpcProviderSubsystem* Gateway = NewObject<UAiNpcProviderSubsystem>(GameInstance);
    FAiNpcProviderConfiguration Rowan;
    Rowan.ProviderName = TEXT("rowan-provider");
    Rowan.ChatCompletionsUrl = TEXT("https://rowan.example.invalid/v1/chat/completions");
    Rowan.Model = TEXT("rowan-model");
    Rowan.bPlayerConsentedToProviderTransmission = true;
    FAiNpcProviderConfiguration Mira = Rowan;
    Mira.ProviderName = TEXT("mira-provider");
    Mira.ChatCompletionsUrl = TEXT("https://mira.example.invalid/v1/chat/completions");
    Mira.Model = TEXT("mira-model");
    FString Error;
    TestTrue(TEXT("The first NPC provider profile is configured"),
        Gateway->ConfigureNpcProvider(TEXT("npc.rowan"), Rowan, TEXT("rowan-key"), Error));
    TestTrue(TEXT("The second NPC provider profile is configured"),
        Gateway->ConfigureNpcProvider(TEXT("npc.mira"), Mira, TEXT("mira-key"), Error));

    FAiNpcPersona RowanPersona;
    RowanPersona.NpcId = TEXT("rowan");
    RowanPersona.DisplayName = TEXT("Rowan");
    RowanPersona.RoleInstructions = TEXT("Guard the gate.");
    RowanPersona.ProviderProfileId = TEXT("npc.rowan");
    FAiNpcPersona MiraPersona = RowanPersona;
    MiraPersona.NpcId = TEXT("mira");
    MiraPersona.DisplayName = TEXT("Mira");
    MiraPersona.ProviderProfileId = TEXT("npc.mira");
    const FGuid RowanSession = Gateway->OpenNpcSession(RowanPersona);
    const FGuid MiraSession = Gateway->OpenNpcSession(MiraPersona);
    const UAiNpcProviderSubsystem::FRuntimeProvider* RowanRoute =
        Gateway->DialogueProviderForSession(Gateway->Sessions.FindChecked(RowanSession));
    const UAiNpcProviderSubsystem::FRuntimeProvider* MiraRoute =
        Gateway->DialogueProviderForSession(Gateway->Sessions.FindChecked(MiraSession));
    TestTrue(TEXT("Each session resolves a configured route"),
        RowanRoute != nullptr && MiraRoute != nullptr);
    TestEqual(TEXT("Rowan resolves only Rowan's model"),
        RowanRoute->Configuration.Model, FString(TEXT("rowan-model")));
    TestEqual(TEXT("Mira resolves only Mira's model"),
        MiraRoute->Configuration.Model, FString(TEXT("mira-model")));
    TestNotEqual(TEXT("The NPC sessions do not collapse onto one provider profile"),
        RowanRoute->Configuration.ChatCompletionsUrl,
        MiraRoute->Configuration.ChatCompletionsUrl);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKnowledgeDialogueFormatRepairTest,
    "MyAiTown.KnowledgeGrant.Runtime.DialogueFormatRepair",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnowledgeDialogueFormatRepairTest::RunTest(const FString& Parameters)
{
    FAiNpcProviderConfiguration Configuration;
    Configuration.ProviderName = TEXT("repair-provider");
    Configuration.ChatCompletionsUrl = TEXT("https://repair.example.invalid/v1/chat/completions");
    Configuration.Model = TEXT("repair-model");
    Configuration.bPlayerConsentedToProviderTransmission = true;
    FAiNpcPersona Persona;
    Persona.NpcId = TEXT("repair-npc");
    Persona.DisplayName = TEXT("Repair NPC");
    Persona.RoleInstructions = TEXT("Use only authorized facts.");
    FString Body;
    FString Error;
    TestTrue(TEXT("A malformed dialogue candidate produces one bounded repair request body"),
        UAiNpcProviderSubsystem::BuildDialogueRepairRequestBody(
            Configuration, Persona, TEXT("```json { broken } ```"),
            TEXT("FactId=fact.safe | The gate is closed."), Body, Error));
    TestTrue(TEXT("The repair body keeps the authorized context and strict claim schema"),
        Body.Contains(TEXT("fact.safe"))
        && Body.Contains(TEXT("evidenceFactIds"))
        && Body.Contains(TEXT("temperature")));
    TestFalse(TEXT("Permanent HTTP 400 failures are not retryable"),
        UAiNpcProviderSubsystem::IsRetryableReviewErrorCode(TEXT("review_http_error_400")));
    TestTrue(TEXT("HTTP 429 receives only the bounded retry path"),
        UAiNpcProviderSubsystem::IsRetryableReviewErrorCode(TEXT("review_http_error_429")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKnowledgeDeterministicAuthorizedFallbackTest,
    "MyAiTown.KnowledgeGrant.Runtime.DeterministicAuthorizedFallback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnowledgeDeterministicAuthorizedFallbackTest::RunTest(const FString& Parameters)
{
    FDialogueKnowledgeView View;
    FKnowledgeDocument GateDocument;
    FKnowledgeFact GateFact;
    GateFact.FactId = TEXT("fact.gate");
    GateFact.Statement = TEXT("The northern gate is closed.");
    GateFact.Entities = {TEXT("gate")};
    GateDocument.Facts.Add(GateFact);
    FKnowledgeDocument BellDocument;
    FKnowledgeFact BellFact;
    BellFact.FactId = TEXT("fact.bell");
    BellFact.Statement = TEXT("The bell was found near the tower.");
    BellFact.Entities = {TEXT("bell"), TEXT("tower")};
    BellDocument.Facts.Add(BellFact);
    View.AllowedDocuments = {GateDocument, BellDocument};
    TArray<FName> Evidence;
    const FString Result = UAiNpcDialogueComponent::BuildAuthorizedFactFallback(
        TEXT("Where was the bell found?"), View,
        FText::FromString(TEXT("No authorized fact.")), Evidence);
    TestEqual(TEXT("The fallback quotes one authorized fact rather than generating a replacement"),
        Result, BellFact.Statement);
    TestEqual(TEXT("The fallback exposes exactly the selected FactId"), Evidence.Num(), 1);
    TestEqual(TEXT("The selected evidence is deterministic"), Evidence[0], BellFact.FactId);
    const FString NoMatchResult = UAiNpcDialogueComponent::BuildAuthorizedFactFallback(
        TEXT("Tell me something unrelated."), View,
        FText::FromString(TEXT("No authorized fact.")), Evidence);
    TestEqual(TEXT("A question with no entity match uses the trusted default instead of an unrelated fact"),
        NoMatchResult, FString(TEXT("No authorized fact.")));
    TestTrue(TEXT("A no-match fallback exposes no evidence"), Evidence.IsEmpty());

    View.AllowedDocuments.Reset();
    TestEqual(TEXT("An empty authorized view uses only the trusted default"),
        UAiNpcDialogueComponent::BuildAuthorizedFactFallback(
            TEXT("Anything?"), View, FText::FromString(TEXT("No authorized fact.")), Evidence),
        FString(TEXT("No authorized fact.")));
    TestTrue(TEXT("The trusted default carries no invented evidence"), Evidence.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKnowledgeSemanticClaimGatesTest,
    "MyAiTown.KnowledgeGrant.Runtime.SemanticClaimGates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnowledgeSemanticClaimGatesTest::RunTest(const FString& Parameters)
{
    UKnowledgePack* Pack = MakeTestPack();
    UKnowledgeGrantComponent* Player = MakeGrantComponent();
    TestTrue(TEXT("The claim-gate fixture grants its source fact"),
        Player->GrantKnowledgeFromQuest(TEXT("claim-gate:1"), Pack->SourceQuestId, Pack).bOk);
    const FDialogueKnowledgeView View = UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(
        Player, {Pack}, FGameplayTagContainer());
    FKnowledgeDialogueClaim Claim;
    Claim.ClaimId = TEXT("claim.gate.origin");
    Claim.Text = Pack->Documents[0].Content;
    Claim.EvidenceFactIds = {FactIdForDocument(Pack->Documents[0].DocumentId)};
    const FKnowledgeDialogueGuardResult Mechanical =
        UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
            Player, View, FGameplayTagContainer(), {Claim}, MakeGuardPolicy());
    if (!TestTrue(TEXT("The claim-gate fixture passes the mechanical gate"),
            Mechanical.bOk && Mechanical.AllowedClaims.Num() == 1))
    {
        return false;
    }
    AActor* Owner = NewObject<AActor>();
    UKnowledgeSemanticReviewComponent* Coordinator =
        NewObject<UKnowledgeSemanticReviewComponent>(Owner);
    UKnowledgeContextCollectorComponent* Collector =
        NewObject<UKnowledgeContextCollectorComponent>(Owner);
    UKnowledgeSemanticReviewPolicy* Policy = NewObject<UKnowledgeSemanticReviewPolicy>();
    Policy->PolicyVersion = TEXT("semantic.claim-gates.v1");
    Policy->TimeoutSeconds = 5.0f;
    Policy->BlockedText = FText::FromString(TEXT("blocked"));
    Policy->ErrorText = FText::FromString(TEXT("error"));

    const FKnowledgeSemanticReviewBeginResult Begin = Coordinator->BeginSemanticReview(
        Player, View, {Collector}, Mechanical, Policy);
    if (!TestTrue(TEXT("The claim-gate fixture creates one reviewable claim"),
            Begin.bOk && Begin.Request.AllowedClaimIds.Num() == 1))
    {
        return false;
    }
    FKnowledgeSemanticReviewResponse Response;
    Response.RequestId = Begin.Request.RequestId;
    Response.ReviewerName = TEXT("claim-gate-reviewer");
    Response.ReviewerVersion = TEXT("1");
    Response.ClaimAssessments.Add(MakePassingAssessment(
        Begin.Request.AllowedClaimIds[0], Claim.EvidenceFactIds));
    Response.ClaimAssessments[0].TaskLeak = EKnowledgeClaimRisk::Possible;
    Response.ClaimAssessments[0].TaskLeakRiskScore = 0.3f;
    const FKnowledgeSemanticReviewResult Blocked =
        Coordinator->CompleteSemanticReview(Response);
    TestTrue(TEXT("One failed safety enum blocks the whole candidate without score compensation"),
        !Blocked.bApproved
        && Blocked.Outcome == EKnowledgeSemanticReviewOutcome::Blocked
        && Blocked.RejectedCandidateText == Mechanical.SafeText);

    const FKnowledgeSemanticReviewBeginResult TypeBegin = Coordinator->BeginSemanticReview(
        Player, View, {Collector}, Mechanical, Policy);
    if (!TestTrue(TEXT("The claim-type gate creates a fresh review request"),
            TypeBegin.bOk && TypeBegin.Request.AllowedClaimIds.Num() == 1))
    {
        return false;
    }
    Response.RequestId = TypeBegin.Request.RequestId;
    Response.ClaimAssessments[0] = MakePassingAssessment(
        TypeBegin.Request.AllowedClaimIds[0], {});
    const FKnowledgeSemanticReviewResult TypeMismatch =
        Coordinator->CompleteSemanticReview(Response);
    TestTrue(TEXT("A factual claim cannot be relabeled non-factual to discard its evidence"),
        !TypeMismatch.bApproved
        && TypeMismatch.Error == TEXT("semantic_review_assessment_invalid"));
    return true;
}

#endif
