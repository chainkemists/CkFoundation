#include "CkIsmSubsystem.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkEcsExt/EntityScript/CkEntityScript_WithActor_Data.h"

#include "CkIsmRenderer/CkIsmRenderer_EntityScript.h"
#include "CkIsmRenderer/Renderer/CkIsmRenderer_TransientFactory.h"
#include "CkIsmRenderer/Renderer/CkIsmRenderer_Utils.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"

// --------------------------------------------------------------------------------------------------------------------

ACk_IsmRenderer_Actor_UE::
    ACk_IsmRenderer_Actor_UE()
{
    _RootNode = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
    RootComponent = _RootNode;

#if WITH_EDITORONLY_DATA
    bListedInSceneOutliner = false;
#endif
}

auto
    ACk_IsmRenderer_Actor_UE::
    DoInitialize()
    -> void
{
    if (_Initialized)
    { return; }

    auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return; }

    auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(World);
    if (ck::Is_NOT_Valid(TransientEntity))
    { return; }

    auto SpawnParams = FInstancedStruct::Make<FCk_EntityScript_WithActor_SpawnParams>(this);
    UCk_Utils_EntityScript_UE::Request_SpawnEntity(
        TransientEntity, UCk_EntityScript_IsmRenderer_UE::StaticClass(), SpawnParams);

    _Initialized = true;
}

auto
    ACk_IsmRenderer_Actor_UE::
    BeginPlay()
    -> void
{
    Super::BeginPlay();
    DoInitialize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_IsmRenderer_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);
    DoSweepLeakedRenderers();
}

auto
    UCk_IsmRenderer_Subsystem_UE::
    Deinitialize()
    -> void
{
    UCk_Utils_IsmRenderer_TransientFactory_UE::ClearCache();

    Super::Deinitialize();
}

auto
    UCk_IsmRenderer_Subsystem_UE::
    DoSweepLeakedRenderers()
    -> void
{
    auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    auto Leaked = TArray<ACk_IsmRenderer_Actor_UE*>{};
    for (TActorIterator<ACk_IsmRenderer_Actor_UE> It(World); It; ++It)
    {
        Leaked.Add(*It);
    }

    for (auto* Actor : Leaked)
    {
        if (ck::IsValid(Actor))
        {
            Actor->Destroy();
        }
    }

    if (Leaked.Num() > 0 && World->WorldType == EWorldType::Editor)
    {
        // Bake the cleanup into the level package on next save. The entity-spawner rebuild
        // path will repopulate fresh transient renderers on demand.
        if (auto* Level = World->PersistentLevel.Get())
        {
            Level->MarkPackageDirty();
        }
    }
}

auto
    UCk_IsmRenderer_Subsystem_UE::
    DoesSupportWorldType(
        const EWorldType::Type WorldType) const
    -> bool
{
    return Super::DoesSupportWorldType(WorldType) || WorldType == EWorldType::Editor;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_IsmRenderer_Subsystem_UE::
    GetOrCreate_IsmRenderer(
        const UCk_IsmRenderer_Data* InDataAsset)
    -> ACk_IsmRenderer_Actor_UE*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InDataAsset),
        TEXT("Trying to GetOrCreate an IsmRenderer from an INVALID Data Asset"))
    { return nullptr; }

    CK_ENSURE_IF_NOT(ck::IsValid(InDataAsset->Get_Mesh()),
        TEXT("Trying to GetOrCreate an IsmRenderer with an INVALID Mesh on Data Asset [{}]"), InDataAsset)
    { return nullptr; }

    if (auto Found = _IsmRenderers.Find(InDataAsset);
        Found != nullptr && ck::IsValid(*Found))
    { return *Found; }

    // Identify the renderer via SpawnInfo.Name (passed through .Set_NonUniqueName) rather than
    // .Set_Label(). SetActorLabel calls AActor::Modify() → MarkPackageDirty() on the actor's
    // package; in editor worlds the only package is the persistent level, so every spawn would
    // dirty the level. RF_Transient (applied in the post-spawn callback) prevents serialization
    // on save but does NOT undo the dirty flag — by the time it's set, the dirty has already
    // fired. SpawnInfo.Name is the construction-time identifier, doesn't call Modify(), and
    // surfaces as the actor's FName (visible in outliner when bListedInSceneOutliner is true,
    // and in `obj list class=Ck_IsmRenderer_Actor_UE` regardless).
    const auto& DescriptiveName = FName{*ck::Format_UE(TEXT("IsmRenderer [{}][{}][Shadows:{}][{}][Collision:{}]"),
        InDataAsset->Get_Mesh(),
        InDataAsset->Get_Mobility(),
        InDataAsset->Get_LightingInfo().Get_CastShadows(),
        InDataAsset->Get_RenderPolicy(),
        InDataAsset->Get_PhysicsInfo().Get_Collision())};

    const auto& SpawnedIsmRendererActor = Cast<ACk_IsmRenderer_Actor_UE>(UCk_Utils_Actor_UE::Request_SpawnActor
    (
        FCk_Utils_Actor_SpawnActor_Params{GetWorld(), ACk_IsmRenderer_Actor_UE::StaticClass()}
        .Set_NonUniqueName(DescriptiveName)
        .Set_SpawnPolicy(ECk_Utils_Actor_SpawnActorPolicy::CannotSpawnInPersistentLevel),
        [&](AActor* InActor)
        {
            const auto& NewIsmRendererActor = Cast<ACk_IsmRenderer_Actor_UE>(InActor);
            NewIsmRendererActor->_RenderData = InDataAsset;

            // ISM renderer actors are a runtime cache derived from UCk_IsmRenderer_Data; they
            // must never be saved into the level package. Without RF_Transient, every editor
            // open re-spawns one and dirties the level, then a save bakes the duplicate in.
            NewIsmRendererActor->SetFlags(RF_Transient);

            // Editor worlds never fire BeginPlay; explicit init also covers runtime without
            // double-spawning (DoInitialize is idempotent).
            NewIsmRendererActor->DoInitialize();
        }
    ));

    const auto IsmRenderer = _IsmRenderers.Add(InDataAsset, SpawnedIsmRendererActor);

    return IsmRenderer;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_IsmRenderer_Subsystem_UE::
    FindOrCache_IsmComponent(
        const UCk_IsmRenderer_Data* InRendererData)
    -> TWeakObjectPtr<UInstancedStaticMeshComponent>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InRendererData),
        TEXT("InRendererData [{}] is NOT valid"), InRendererData)
    { return {}; }

    if (const auto& MaybeFound = _IsmComponentCache.Find(InRendererData);
        ck::IsValid(MaybeFound, ck::IsValid_Policy_NullptrOnly{}) && ck::IsValid(*MaybeFound))
    { return *MaybeFound; }

    const auto NewRenderer = GetOrCreate_IsmRenderer(InRendererData);

    CK_ENSURE_IF_NOT(ck::IsValid(NewRenderer),
        TEXT("Failed to GetOrCreate ISM Renderer Actor for [{}]"), InRendererData)
    { return {}; }

    auto StaticMeshComponent = [&]() -> UInstancedStaticMeshComponent*
    {
        if (InRendererData->Get_RenderPolicy() == ECk_Ism_RenderPolicy::ISM)
        { return NewRenderer->FindComponentByClass<UInstancedStaticMeshComponent>(); }

        return NewRenderer->FindComponentByClass<UHierarchicalInstancedStaticMeshComponent>();
    }();

    if (ck::Is_NOT_Valid(StaticMeshComponent))
    { return {}; }

    return _IsmComponentCache.Add(InRendererData, StaticMeshComponent);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IsmRenderer_Subsystem_UE::
    GetOrCreate_IsmRenderer(
        const UWorld* InWorld,
        const UCk_IsmRenderer_Data* InDataAsset)
    -> ACk_IsmRenderer_Actor_UE*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorld),
        TEXT("Trying to get Ism Renderer from an INVALID World"))
    { return nullptr; }

    CK_ENSURE_IF_NOT(ck::IsValid(InDataAsset),
        TEXT("Trying to get Ism Renderer from an INVALID Data Asset"))
    { return nullptr; }

    const auto& IsmRendererSubsystem = InWorld->GetSubsystem<UCk_IsmRenderer_Subsystem_UE>(InWorld);

    CK_ENSURE_IF_NOT(ck::IsValid(IsmRendererSubsystem),
        TEXT("Could NOT find the IsmRender_Subsystem for the World [{}]"), InWorld)
    { return nullptr; }

    return IsmRendererSubsystem->GetOrCreate_IsmRenderer(InDataAsset);
}

// --------------------------------------------------------------------------------------------------------------------
