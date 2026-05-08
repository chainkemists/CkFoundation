#include "CkIskmSubsystem.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkIskmRenderer/CkIskmRenderer_Log.h"

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
    _LiveSKMCs.Add(NewComp);
    return NewComp;
}

auto
    ACk_IskmRenderer_Actor_UE::
    Release_BaseSKMC(USkeletalMeshComponent* InComp) -> void
{
    if (ck::Is_NOT_Valid(InComp)) { return; }
    InComp->SetVisibility(false);
    InComp->Stop();
    InComp->SetAnimInstanceClass(nullptr);
    InComp->SetSkeletalMesh(nullptr);
    InComp->SetSimulatePhysics(false);
    _LiveSKMCs.RemoveSwap(InComp);
    _Pool_FreeSKMCs.Add(InComp);
}
