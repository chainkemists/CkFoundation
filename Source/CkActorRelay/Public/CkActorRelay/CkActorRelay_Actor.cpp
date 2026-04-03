#include "CkActorRelay_Actor.h"

#include "CkActorRelay_GroupSubsystem.h"

#include "CkActorRelay/CkActorRelay_Log.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
#include "CkEntityBridge/Public/CkEntityBridge/CkEntityBridge_ConstructionScript.h"

#include <Net/UnrealNetwork.h>
#include <Net/Core/PushModel/PushModel.h>

// --------------------------------------------------------------------------------------------------------------------

ACk_ActorRelay_UE::
    ACk_ActorRelay_UE()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    PrimaryActorTick.bCanEverTick = false;
    PrimaryActorTick.bTickEvenWhenPaused = false;

    _EntityBridge = CreateDefaultSubobject<UCk_EntityBridge_ActorComponent_UE>(TEXT("EntityBridge"));
    _EntityBridge->_ConstructionScript = UCk_Entity_ConstructionScript_WithTransform_PDA::StaticClass();
}

auto
    ACk_ActorRelay_UE::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    OnRep_GroupSubsystemClass();
}

auto
    ACk_ActorRelay_UE::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    constexpr auto Params = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _GroupSubsystemClass, Params);
}

auto
    ACk_ActorRelay_UE::
    InjectGroupSubsystemClass(
        TSubclassOf<UCk_ActorRelay_Group_Subsystem_Base_UE> InGroupSubsystemClass)
    -> void
{
    _GroupSubsystemClass = InGroupSubsystemClass;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _GroupSubsystemClass, this);
}

auto
    ACk_ActorRelay_UE::
    OnRep_GroupSubsystemClass()
    -> void
{
    if (ck::Is_NOT_Valid(_GroupSubsystemClass))
    { return; }

    if (ck::IsValid(_GroupSubsystem))
    { return; }

    if (DoTryRegisterWithGroupSubsystem())
    { return; }

    ck::actorrelay::Verbose(TEXT("GroupSubsystem not yet resolved for ActorRelay. Will retry every 100ms"));

    auto WeakThis = TWeakObjectPtr(this);
    GetWorld()->GetTimerManager().SetTimer(_RegistrationRetryTimerHandle, [WeakThis]()
    {
        if (ck::Is_NOT_Valid(WeakThis))
        { return; }

        if (WeakThis->DoTryRegisterWithGroupSubsystem())
        {
            ck::actorrelay::Verbose(TEXT("Successfully registered ActorRelay with GroupSubsystem"));
            WeakThis->GetWorld()->GetTimerManager().ClearTimer(WeakThis->_RegistrationRetryTimerHandle);
        }
    }, 0.1f, true);
}

auto
    ACk_ActorRelay_UE::
    DoTryRegisterWithGroupSubsystem()
    -> bool
{
    auto GroupSubsystem = Cast<UCk_ActorRelay_Group_Subsystem_Base_UE>(GetWorld()->GetSubsystemBase(_GroupSubsystemClass));

    if (ck::Is_NOT_Valid(GroupSubsystem))
    { return false; }

    _GroupSubsystem = GroupSubsystem;
    GroupSubsystem->DoRegisterChannelActor(this);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
