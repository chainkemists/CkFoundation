#include "CkGameWorldSubsystem.h"

#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Game_WorldSubsystem_Base_UE::
    Deinitialize()
    -> void
{
    OnDeinitialize();
}

auto
    UCk_Game_WorldSubsystem_Base_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);

    OnInitialize();
}

auto
    UCk_Game_WorldSubsystem_Base_UE::
    PostInitialize()
    -> void
{
    Super::PostInitialize();

    OnAllWorldSubsystemsInitialized();
}

auto
    UCk_Game_WorldSubsystem_Base_UE::
    ShouldCreateSubsystem(
        UObject* InOuter) const
    -> bool
{
    const auto& ShouldCreateSubsystem = Super::ShouldCreateSubsystem(InOuter);

    if (NOT ShouldCreateSubsystem)
    { return false; }

    if (ck::Is_NOT_Valid(InOuter))
    { return true; }

    const auto& World = InOuter->GetWorld();

    if (ck::Is_NOT_Valid(World))
    { return true; }

    return DoesSupportWorldType(World->WorldType);
}

auto
    UCk_Game_WorldSubsystem_Base_UE::
    DoesSupportWorldType(
        const EWorldType::Type WorldType) const
    -> bool
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

// --------------------------------------------------------------------------------------------------------------------
