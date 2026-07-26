#include "CkLevelStreaming_Utils.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Algorithms/CkAlgorithms.h"

#include <Engine/World.h>
#include <Engine/Level.h>
#include <Misc/PackageName.h>
#include <Components/SceneComponent.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_LevelTransformParams::
    Get_Transform(
        ECk_TransformOriginPolicy InPolicy) const
    -> FTransform
{
    if (InPolicy == ECk_TransformOriginPolicy::WorldOrigin || NOT _UseCustomOrigin)
    { return FTransform(_Rotation, _Location, _Scale); }

    const auto RelativeLocation = _Location - _CustomOrigin;
    const auto RotatedLocation = _Rotation.RotateVector(RelativeLocation) + _CustomOrigin;

    return FTransform(_Rotation, RotatedLocation, _Scale);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_LevelStreaming_UE::
    Request_LoadStreamingLevel(
        const UObject* InWorldContextObject,
        const FCk_LoadLevelParams& InParams,
        ULevelStreamingDynamic*& OutStreamingLevel)
    -> bool
{
    OutStreamingLevel = nullptr;

    if (ck::Is_NOT_Valid(GEngine))
    { return false; }

    const auto World = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("Invalid World for loading streaming level"))
    { return false; }

    auto LongPackageName = FString{};
    CK_ENSURE_IF_NOT(FPackageName::SearchForPackageOnDisk(InParams.Get_LevelPath(), &LongPackageName), TEXT("Failed to find level package: {}"), InParams.Get_LevelPath())
    { return false; }

    const auto UniquePackageName = Internal_CreateUniquePackageName(World, LongPackageName);

    auto* StreamingLevel = NewObject<ULevelStreamingDynamic>(
        World,
        ULevelStreamingDynamic::StaticClass(),
        NAME_None,
        RF_Transient);

    CK_ENSURE_IF_NOT(ck::IsValid(StreamingLevel), TEXT("Failed to create streaming level object"))
    { return false; }

    StreamingLevel->SetWorldAssetByPackageName(FName(*UniquePackageName));
    StreamingLevel->LevelColor = FColor::MakeRandomColor();
    StreamingLevel->SetShouldBeLoaded(true);
    StreamingLevel->SetShouldBeVisible(InParams.Get_InitiallyVisible());
    StreamingLevel->bShouldBlockOnLoad = InParams.Get_BlockOnLoad();
    StreamingLevel->bInitiallyLoaded = true;
    StreamingLevel->bInitiallyVisible = InParams.Get_InitiallyVisible();

    const auto& TransformParams = InParams.Get_TransformParams();
    StreamingLevel->LevelTransform = TransformParams.Get_UseCustomOrigin()
        ? TransformParams.Get_Transform(ECk_TransformOriginPolicy::CustomOrigin)
        : TransformParams.Get_Transform(ECk_TransformOriginPolicy::WorldOrigin);

    StreamingLevel->PackageNameToLoad = FName(*LongPackageName);

    World->AddStreamingLevel(StreamingLevel);

    OutStreamingLevel = StreamingLevel;
    return true;
}

auto
    UCk_Utils_LevelStreaming_UE::
    Request_UpdateLevelTransform(
        ULevelStreaming* InStreamingLevel,
        const FCk_LevelTransformParams& InTransformParams)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InStreamingLevel), TEXT("Invalid streaming level for transform update"))
    { return; }

    const auto NewTransform = InTransformParams.Get_Transform(ECk_TransformOriginPolicy::WorldOrigin);

    if (InStreamingLevel->LevelTransform.Equals(NewTransform))
    { return; }

    InStreamingLevel->Modify();

    Internal_RemoveLevelTransform(InStreamingLevel);

    InStreamingLevel->LevelTransform = NewTransform;
    Internal_ApplyLevelTransform(InStreamingLevel);
}

auto
    UCk_Utils_LevelStreaming_UE::
    Request_UpdateLevelTransformAndOrigin(
        ULevelStreaming* InStreamingLevel,
        const FCk_LevelTransformParams& InTransformParams)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InStreamingLevel), TEXT("Invalid streaming level for transform update"))
    { return; }

    const auto NewTransform = InTransformParams.Get_UseCustomOrigin()
        ? InTransformParams.Get_Transform(ECk_TransformOriginPolicy::CustomOrigin)
        : InTransformParams.Get_Transform(ECk_TransformOriginPolicy::WorldOrigin);

    if (InStreamingLevel->LevelTransform.Equals(NewTransform))
    { return; }

    const auto LoadedLevel = InStreamingLevel->GetLoadedLevel();
    if (ck::Is_NOT_Valid(LoadedLevel))
    {
        InStreamingLevel->LevelTransform = NewTransform;
        return;
    }

    InStreamingLevel->Modify();

    const auto OldTransform = InStreamingLevel->LevelTransform;
    const auto DeltaTransform = NewTransform.GetRelativeTransform(OldTransform);

    Request_ApplyTransformToLevelActors(LoadedLevel, DeltaTransform, true);

    InStreamingLevel->LevelTransform = NewTransform;
}

auto
    UCk_Utils_LevelStreaming_UE::
    Request_UnloadStreamingLevel(
        ULevelStreamingDynamic* InStreamingLevel)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InStreamingLevel), TEXT("Invalid streaming level to unload"))
    { return; }

    InStreamingLevel->SetIsRequestingUnloadAndRemoval(true);
}

auto
    UCk_Utils_LevelStreaming_UE::
    Request_ApplyTransformToLevelActors(
        ULevel* InLevel,
        const FTransform& InTransform,
        bool InPreserveComponentMobility)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InLevel), TEXT("Invalid level for transform application"))
    { return; }

    if (InTransform.Equals(FTransform::Identity))
    { return; }

    if (NOT InTransform.GetRotation().IsIdentity())
    {
        InLevel->bTextureStreamingRotationChanged = true;
    }

    for (const auto& Actors = InLevel->Actors; auto Actor : Actors)
    {
        if (ck::Is_NOT_Valid(Actor))
        { continue; }

        const auto ActorIsAttachedToParent = ck::IsValid(Actor->GetAttachParentActor());
        if (ActorIsAttachedToParent)
        { continue; }

        auto* RootComponent = Actor->GetRootComponent();
        if (ck::Is_NOT_Valid(RootComponent))
        { continue; }

        const auto OriginalMobility = RootComponent->Mobility;
        if (InPreserveComponentMobility)
        {
            RootComponent->Mobility = EComponentMobility::Movable;
        }

        const auto CurrentLocation = RootComponent->GetRelativeLocation();
        const auto CurrentRotation = RootComponent->GetRelativeRotation().Quaternion();

        const auto NewLocation = InTransform.TransformPosition(CurrentLocation);
        const auto NewRotation = InTransform.TransformRotation(CurrentRotation);

        RootComponent->SetRelativeLocationAndRotation(NewLocation, NewRotation);

        if (InPreserveComponentMobility)
        {
            RootComponent->Mobility = OriginalMobility;
        }
    }

    InLevel->OnApplyLevelTransform.Broadcast(InTransform);
}

auto
    UCk_Utils_LevelStreaming_UE::
    Get_AllStreamingLevels(
        const UObject* InWorldContextObject)
    -> TArray<ULevelStreaming*>
{
    if (ck::Is_NOT_Valid(GEngine))
    { return {}; }

    const auto World = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

    if (ck::Is_NOT_Valid(World))
    { return {}; }

    return World->GetStreamingLevels();
}

auto
    UCk_Utils_LevelStreaming_UE::
    Get_LevelTransform(
        const ULevelStreaming* InStreamingLevel)
    -> FTransform
{
    if (ck::Is_NOT_Valid(InStreamingLevel))
    { return FTransform::Identity; }

    return InStreamingLevel->LevelTransform;
}

auto
    UCk_Utils_LevelStreaming_UE::
    Get_IsLevelLoaded(
        const ULevelStreaming* InStreamingLevel)
    -> bool
{
    if (ck::Is_NOT_Valid(InStreamingLevel))
    { return false; }

    return ck::IsValid(InStreamingLevel->GetLoadedLevel());
}

auto
    UCk_Utils_LevelStreaming_UE::
    Internal_RemoveLevelTransform(
        const ULevelStreaming* InStreamingLevel)
    -> void
{
    if (ck::Is_NOT_Valid(InStreamingLevel))
    { return; }

    const auto LoadedLevel = InStreamingLevel->GetLoadedLevel();
    if (ck::Is_NOT_Valid(LoadedLevel))
    { return; }

    Request_ApplyTransformToLevelActors(LoadedLevel, InStreamingLevel->LevelTransform.Inverse(), true);
}

auto
    UCk_Utils_LevelStreaming_UE::
    Internal_ApplyLevelTransform(
        const ULevelStreaming* InStreamingLevel)
    -> void
{
    if (ck::Is_NOT_Valid(InStreamingLevel))
    { return; }

    const auto LoadedLevel = InStreamingLevel->GetLoadedLevel();
    if (ck::Is_NOT_Valid(LoadedLevel))
    { return; }

    Request_ApplyTransformToLevelActors(LoadedLevel, InStreamingLevel->LevelTransform, true);
}

auto
    UCk_Utils_LevelStreaming_UE::
    Internal_CreateUniquePackageName(
        const UWorld* InWorld,
        const FString& InLongPackageName)
    -> FString
{
    const auto ShortPackageName = FPackageName::GetShortName(InLongPackageName);
    const auto PackagePath = FPackageName::GetLongPackagePath(InLongPackageName);

    auto UniquePackageName = ck::Format_UE(TEXT("{}/{}{}"),
        PackagePath,
        InWorld->StreamingLevelsPrefix,
        ShortPackageName);

    UniquePackageName += ck::Format_UE(TEXT("_LevelInstance_{}"), ++UniqueInstanceIdCounter);

    return UniquePackageName;
}

// --------------------------------------------------------------------------------------------------------------------
