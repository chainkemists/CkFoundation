#include "CkIskmSubsystem.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Utils.h"

#include "CkIskmRenderer/CkIskmRenderer_Log.h"
#include "CkIskmRenderer/Renderer/CkIskmRenderer_Fragment_Data.h"
#include "CkIskmRenderer/Renderer/CkIskm_BatchedCrowd_Actor.h"

#include <Engine/Engine.h>
#include <Engine/World.h>

ACk_IskmRenderer_Actor_UE::ACk_IskmRenderer_Actor_UE()
{
    PrimaryActorTick.bCanEverTick = false;
    _RootNode = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(_RootNode);

#if WITH_EDITORONLY_DATA
    // Runtime cache actors derived from UCk_IskmRenderer_Data — never user-facing (ISM parity).
    bListedInSceneOutliner = false;
#endif
}

auto
    ACk_IskmRenderer_Actor_UE::
    BeginPlay() -> void
{
    Super::BeginPlay();
}

#if WITH_EDITOR
auto
    ACk_IskmRenderer_Actor_UE::
    IsSelectionChild() const
    -> bool
{
    return _EditorSelectionOwner.IsValid();
}

auto
    ACk_IskmRenderer_Actor_UE::
    GetSelectionParent() const
    -> AActor*
{
    return _EditorSelectionOwner.Get();
}
#endif

auto
    ACk_IskmRenderer_Actor_UE::
    DoInitialize(UCk_IskmRenderer_Data* InRendererData) -> void
{
    if (_Initialized)
    { return; }
    _RendererData = InRendererData;
    _Initialized = true;
}

#if WITH_EDITOR
auto
    ACk_IskmRenderer_Actor_UE::
    EditorOnly_EnableAnimationTicking(USkeletalMeshComponent* InComp) -> void
{
    if (ck::Is_NOT_Valid(InComp))
    { return; }

    const auto* World = InComp->GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return; }

    if (World->WorldType != EWorldType::Editor && World->WorldType != EWorldType::EditorPreview)
    { return; }

    // Editor worlds one-shot-tick a SKMC at registration then freeze it behind bUpdateAnimationInEditor
    // (the RefreshBoneTransforms gate); VisibilityBasedAnimTickOption alone does nothing there.
    // Runtime worlds ignore the flag entirely.
    InComp->SetUpdateAnimationInEditor(true);
}
#endif

auto
    ACk_IskmRenderer_Actor_UE::
    Acquire_BaseSKMC() -> USkeletalMeshComponent*
{
    // A pooled component destroyed externally (editor level cleanup, owner teardown ordering) is nulled
    // in this UPROPERTY array after the next GC — skip stale entries rather than handing one out.
    while (_Pool_FreeSKMCs.Num() > 0)
    {
        auto Pooled = _Pool_FreeSKMCs.Pop(EAllowShrinking::No).Get();
        if (ck::Is_NOT_Valid(Pooled))
        { continue; }
        Pooled->SetVisibility(true);
        _LiveSKMCs.Add(Pooled);
#if WITH_EDITOR
        EditorOnly_EnableAnimationTicking(Pooled);
#endif
        return Pooled;
    }

    auto NewComp = NewObject<USkeletalMeshComponent>(this, USkeletalMeshComponent::StaticClass(), NAME_None, RF_Transient);
    NewComp->SetupAttachment(_RootNode);
    NewComp->RegisterComponent();
    NewComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    NewComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    _LiveSKMCs.Add(NewComp);
#if WITH_EDITOR
    EditorOnly_EnableAnimationTicking(NewComp);
#endif
    return NewComp;
}

auto
    ACk_IskmRenderer_Actor_UE::
    Release_BaseSKMC(USkeletalMeshComponent* InComp) -> void
{
    if (ck::Is_NOT_Valid(InComp))
    { return; }
    InComp->SetVisibility(false);
    // Never Stop() here: on an AnimBP / notify-bridge instance it logs the "Currently in Animation
    // Blueprint mode" Warning, which the AutoTest harness escalates to a test failure. Clearing the
    // class destroys the live instance outright, halting playback either way.
    InComp->SetAnimInstanceClass(nullptr);
    InComp->SetSkeletalMesh(nullptr);
    InComp->SetSimulatePhysics(false);
    // Outline state (CkUsf custom depth/stencil) is component-level and survives the mesh clear — strip it
    // unconditionally so it can never leak to the next borrower, whatever the EndPlay bookkeeping order.
    InComp->SetRenderCustomDepth(false);
    InComp->SetCustomDepthStencilValue(0);
    const auto NumRemoved = _LiveSKMCs.RemoveSwap(InComp);
    CK_ENSURE_IF_NOT(NumRemoved > 0,
        TEXT("Release_BaseSKMC: SKMC [{}] is not live on renderer [{}] (double release or foreign component)"), InComp, this)
    { return; }
    _Pool_FreeSKMCs.Add(InComp);

#if WITH_EDITORONLY_DATA
    // Reclaiming any earlier would race the owner's entity-destroy cascade (~4 ticks) — EndPlay fires a
    // loud ensure if the SKMC vanished before it ran. IsExplicitlyNull distinguishes "never a per-owner
    // renderer" (shared runtime renderer — must never self-destroy) from "owner set, then destroyed".
    if (NOT _EditorSelectionOwner.IsExplicitlyNull() &&
        NOT _EditorSelectionOwner.IsValid() &&
        _LiveSKMCs.IsEmpty())
    {
        Destroy();
    }
#endif
}

#if WITH_EDITOR
auto
    UCk_IskmRenderer_Subsystem_UE::
    Initialize(FSubsystemCollectionBase& Collection) -> void
{
    Super::Initialize(Collection);

    // Backstop for per-owner editor renderers whose pool is ALREADY quiescent: with no live SKMCs, no
    // future Release_BaseSKMC will ever run the self-reclaim that handles the normal delete path.
    if (const auto* World = GetWorld();
        ck::IsValid(World) && World->WorldType == EWorldType::Editor && ck::IsValid(GEngine))
    {
        _OnLevelActorDeletedHandle = GEngine->OnLevelActorDeleted().AddUObject(
            this, &UCk_IskmRenderer_Subsystem_UE::EditorOnly_OnLevelActorDeleted);
    }
}

auto
    UCk_IskmRenderer_Subsystem_UE::
    EditorOnly_OnLevelActorDeleted(AActor* InActor) -> void
{
    if ((_PerOwnerRendererActors.IsEmpty() && _PerOwnerPreviewCrowds.IsEmpty()) ||
        ck::Is_NOT_Valid(InActor, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    constexpr auto EvenIfPendingKill = true;

    // Collect first, destroy after — Destroy() re-broadcasts OnLevelActorDeleted.
    auto ToDestroy = TArray<ACk_IskmRenderer_Actor_UE*>{};

    for (auto It = _PerOwnerRendererActors.CreateIterator(); It; ++It)
    {
        if (It->Key.Value.Get(EvenIfPendingKill) != InActor)
        { continue; }

        if (auto* Renderer = It->Value.Get();
            ck::IsValid(Renderer) && Renderer->Get_LiveSKMCs().IsEmpty())
        { ToDestroy.Add(Renderer); }

        // Live SKMCs still out → the destroy cascade is in flight and Release_BaseSKMC's self-reclaim
        // finishes the job. Drop the map entry either way: the owner is gone.
        It.RemoveCurrent();
    }

    for (auto* Renderer : ToDestroy)
    { Renderer->Destroy(); }

    // Preview crowds have no in-flight pool cascade to wait out (members are instances inside the
    // crowd's own tile components), so they can be destroyed immediately.
    auto CrowdsToDestroy = TArray<ACk_Iskm_BatchedCrowd_Actor*>{};

    for (auto It = _PerOwnerPreviewCrowds.CreateIterator(); It; ++It)
    {
        if (It->Key.Value.Get(EvenIfPendingKill) != InActor)
        { continue; }

        if (auto* Crowd = It->Value.Get();
            ck::IsValid(Crowd))
        { CrowdsToDestroy.Add(Crowd); }

        It.RemoveCurrent();
    }

    for (auto* Crowd : CrowdsToDestroy)
    { Crowd->Destroy(); }
}
#endif

auto
    UCk_IskmRenderer_Subsystem_UE::
    Deinitialize() -> void
{
    for (auto& Pair : _RendererActors)
    {
        if (ck::IsValid(Pair.Value))
        { Pair.Value->Destroy(); }
    }
    _RendererActors.Reset();

#if WITH_EDITOR
    if (ck::IsValid(GEngine) && _OnLevelActorDeletedHandle.IsValid())
    {
        GEngine->OnLevelActorDeleted().Remove(_OnLevelActorDeletedHandle);
        _OnLevelActorDeletedHandle.Reset();
    }

    for (auto& Pair : _PerOwnerRendererActors)
    {
        if (auto* Renderer = Pair.Value.Get();
            ck::IsValid(Renderer))
        { Renderer->Destroy(); }
    }
    _PerOwnerRendererActors.Reset();

    for (auto& Pair : _PerOwnerPreviewCrowds)
    {
        if (auto* Crowd = Pair.Value.Get();
            ck::IsValid(Crowd))
        { Crowd->Destroy(); }
    }
    _PerOwnerPreviewCrowds.Reset();
#endif

    Super::Deinitialize();
}

auto
    UCk_IskmRenderer_Subsystem_UE::
    DoesSupportWorldType(const EWorldType::Type WorldType) const -> bool
{
    switch (WorldType)
    {
        case EWorldType::Game:
        case EWorldType::PIE:
        case EWorldType::Editor:
        case EWorldType::EditorPreview:
            return true;
        default:
            return false;
    }
}

auto
    UCk_IskmRenderer_Subsystem_UE::
    DoSpawn_RendererActor(UCk_IskmRenderer_Data* InRendererData)
    -> ACk_IskmRenderer_Actor_UE*
{
    auto World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return nullptr; }
    auto SpawnInfo = FActorSpawnParameters{};
    SpawnInfo.ObjectFlags |= RF_Transient;
    SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    auto* NewActor = World->SpawnActor<ACk_IskmRenderer_Actor_UE>(
        ACk_IskmRenderer_Actor_UE::StaticClass(), FTransform::Identity, SpawnInfo);

    if (ck::IsValid(NewActor))
    {
        NewActor->DoInitialize(InRendererData);
    }
    return NewActor;
}

auto
    UCk_IskmRenderer_Subsystem_UE::
    GetOrCreate_RendererActor(UCk_IskmRenderer_Data* InRendererData)
    -> ACk_IskmRenderer_Actor_UE*
{
    if (ck::Is_NOT_Valid(InRendererData))
    { return nullptr; }
    if (auto* Existing = _RendererActors.Find(InRendererData))
    {
        return *Existing;
    }

    auto* NewActor = DoSpawn_RendererActor(InRendererData);

    if (ck::IsValid(NewActor))
    {
        _RendererActors.Add(InRendererData, NewActor);
    }
    return NewActor;
}

#if WITH_EDITOR
auto
    UCk_IskmRenderer_Subsystem_UE::
    GetOrCreate_RendererActor_ForEditorSelectionOwner(
        UCk_IskmRenderer_Data* InRendererData,
        AActor* InSelectionOwner)
    -> ACk_IskmRenderer_Actor_UE*
{
    if (ck::Is_NOT_Valid(InRendererData))
    { return nullptr; }

    CK_ENSURE_IF_NOT(ck::IsValid(InSelectionOwner),
        TEXT("Trying to GetOrCreate a per-owner IskmRenderer for an INVALID SelectionOwner (Data Asset [{}])"), InRendererData)
    { return nullptr; }

    const auto Key = FPerOwnerRendererKey{InRendererData, InSelectionOwner};

    if (const auto* MaybeFound = _PerOwnerRendererActors.Find(Key);
        MaybeFound != nullptr && MaybeFound->IsValid())
    { return MaybeFound->Get(); }

    auto* NewActor = DoSpawn_RendererActor(InRendererData);

    if (ck::Is_NOT_Valid(NewActor))
    { return nullptr; }

    NewActor->_EditorSelectionOwner = InSelectionOwner;

    UCk_Utils_EditorSelectionOwner_UE::RegisterProxyActor(InSelectionOwner, NewActor);

    _PerOwnerRendererActors.Add(Key, NewActor);

    return NewActor;
}

auto
    UCk_IskmRenderer_Subsystem_UE::
    GetOrCreate_PreviewCrowd_ForEditorSelectionOwner(
        UCk_IskmAnimCollection_Data* InCollection,
        float InTileSize,
        AActor* InSelectionOwner)
    -> ACk_Iskm_BatchedCrowd_Actor*
{
    if (ck::Is_NOT_Valid(InCollection))
    { return nullptr; }

    CK_ENSURE_IF_NOT(ck::IsValid(InSelectionOwner),
        TEXT("Trying to GetOrCreate a per-owner preview crowd for an INVALID SelectionOwner (AnimCollection [{}])"), InCollection)
    { return nullptr; }

    const auto Key = FPerOwnerCrowdKey{InCollection, InSelectionOwner};

    if (const auto* MaybeFound = _PerOwnerPreviewCrowds.Find(Key);
        MaybeFound != nullptr && MaybeFound->IsValid())
    { return MaybeFound->Get(); }

    auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return nullptr; }

    auto SpawnInfo = FActorSpawnParameters{};
    SpawnInfo.ObjectFlags |= RF_Transient;
    SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    auto* Crowd = World->SpawnActor<ACk_Iskm_BatchedCrowd_Actor>(
        ACk_Iskm_BatchedCrowd_Actor::StaticClass(), FTransform::Identity, SpawnInfo);

    if (ck::Is_NOT_Valid(Crowd))
    { return nullptr; }

    Crowd->Initialize(InCollection, InTileSize);
    Crowd->_EditorSelectionOwner = InSelectionOwner;

    UCk_Utils_EditorSelectionOwner_UE::RegisterProxyActor(InSelectionOwner, Crowd);

    _PerOwnerPreviewCrowds.Add(Key, Crowd);

    return Crowd;
}
#endif

auto
    UCk_Utils_IskmRenderer_Subsystem_UE::
    GetOrCreate_RendererActor(const UWorld* InWorld, UCk_IskmRenderer_Data* InRendererData)
    -> ACk_IskmRenderer_Actor_UE*
{
    if (ck::Is_NOT_Valid(InWorld))
    { return nullptr; }
    auto* Sub = InWorld->GetSubsystem<UCk_IskmRenderer_Subsystem_UE>();
    if (ck::Is_NOT_Valid(Sub))
    { return nullptr; }
    return Sub->GetOrCreate_RendererActor(InRendererData);
}
