#include "CkIskmSubsystem.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkIskmRenderer/CkIskmRenderer_Log.h"
#include "CkIskmRenderer/Renderer/CkIskmRenderer_Fragment_Data.h"

ACk_IskmRenderer_Actor_UE::ACk_IskmRenderer_Actor_UE()
{
    PrimaryActorTick.bCanEverTick = false;
    _RootNode = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(_RootNode);
}

auto
    ACk_IskmRenderer_Actor_UE::
    BeginPlay() -> void
{
    Super::BeginPlay();
}

auto
    ACk_IskmRenderer_Actor_UE::
    DoInitialize(UCk_IskmRenderer_Data* InRendererData) -> void
{
    if (_Initialized) { return; }
    _RendererData = InRendererData;
    _Initialized = true;
}

auto
    ACk_IskmRenderer_Actor_UE::
    Acquire_BaseSKMC() -> USkeletalMeshComponent*
{
    if (_Pool_FreeSKMCs.Num() > 0)
    {
        auto Pooled = _Pool_FreeSKMCs.Pop(EAllowShrinking::No).Get();
        Pooled->SetVisibility(true);
        _LiveSKMCs.Add(Pooled);
        return Pooled;
    }

    auto NewComp = NewObject<USkeletalMeshComponent>(this, USkeletalMeshComponent::StaticClass(), NAME_None, RF_Transient);
    NewComp->SetupAttachment(_RootNode);
    NewComp->RegisterComponent();
    NewComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    NewComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    // Update Rate Optimization: throttle full pose+bone evaluation by significance
    // (screen size / distance) with interpolation between, instead of evaluating every
    // pooled SKMC every frame. Pose still evaluates (AlwaysTickPose kept), so sockets /
    // leader-pose followers keep working — only distant instances update less often.
    NewComp->bEnableUpdateRateOptimizations = true;
    _LiveSKMCs.Add(NewComp);
    return NewComp;
}

auto
    ACk_IskmRenderer_Actor_UE::
    Release_BaseSKMC(USkeletalMeshComponent* InComp) -> void
{
    if (ck::Is_NOT_Valid(InComp)) { return; }
    InComp->SetVisibility(false);
    // No Stop() here: on a component whose AnimScriptInstance is an AnimBP (or
    // the notify bridge), Stop() logs the "Currently in Animation Blueprint
    // mode" Warning — which the AutoTest harness escalates to a failure for any
    // proxy destroyed mid-test. SetAnimInstanceClass(nullptr) destroys the live
    // instance outright (single-node or AnimBP), halting playback either way.
    InComp->SetAnimInstanceClass(nullptr);
    InComp->SetSkeletalMesh(nullptr);
    InComp->SetSimulatePhysics(false);
    _LiveSKMCs.RemoveSwap(InComp);
    _Pool_FreeSKMCs.Add(InComp);
}

auto
    UCk_IskmRenderer_Subsystem_UE::
    Deinitialize() -> void
{
    for (auto& Pair : _RendererActors)
    {
        if (ck::IsValid(Pair.Value)) { Pair.Value->Destroy(); }
    }
    _RendererActors.Reset();

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
    GetOrCreate_RendererActor(UCk_IskmRenderer_Data* InRendererData)
    -> ACk_IskmRenderer_Actor_UE*
{
    if (ck::Is_NOT_Valid(InRendererData)) { return nullptr; }

    if (auto* Existing = _RendererActors.Find(InRendererData))
    {
        return *Existing;
    }

    auto World = GetWorld();
    if (ck::Is_NOT_Valid(World, ck::IsValid_Policy_NullptrOnly{})) { return nullptr; }

    auto SpawnInfo = FActorSpawnParameters{};
    SpawnInfo.ObjectFlags |= RF_Transient;
    SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    auto* NewActor = World->SpawnActor<ACk_IskmRenderer_Actor_UE>(
        ACk_IskmRenderer_Actor_UE::StaticClass(), FTransform::Identity, SpawnInfo);

    if (ck::IsValid(NewActor))
    {
        NewActor->DoInitialize(InRendererData);
        _RendererActors.Add(InRendererData, NewActor);
    }
    return NewActor;
}

auto
    UCk_Utils_IskmRenderer_Subsystem_UE::
    GetOrCreate_RendererActor(const UWorld* InWorld, UCk_IskmRenderer_Data* InRendererData)
    -> ACk_IskmRenderer_Actor_UE*
{
    if (ck::Is_NOT_Valid(InWorld, ck::IsValid_Policy_NullptrOnly{})) { return nullptr; }
    auto* Sub = InWorld->GetSubsystem<UCk_IskmRenderer_Subsystem_UE>();
    if (ck::Is_NOT_Valid(Sub)) { return nullptr; }
    return Sub->GetOrCreate_RendererActor(InRendererData);
}
