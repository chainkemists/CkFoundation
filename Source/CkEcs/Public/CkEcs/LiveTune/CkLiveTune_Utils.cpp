#include "CkLiveTune_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/LiveTune/CkLiveTune_HandlerRegistry.h"
#include "CkEcs/Subsystem/CkLiveTune_Subsystem.h"

#include <UObject/UnrealType.h>

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR

auto
    UCk_Utils_LiveTune_UE::
    Link(
        FCk_Handle& InHandle,
        const UObject* InTuningAsset,
        FName InMemberName)
    -> FCk_Handle
{
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("LiveTune Link: invalid Handle — cannot link to [{}].[{}]"), InTuningAsset, InMemberName)
    {}
    if (NOT HandleIsValid)
    { return InHandle; }

    const auto AssetIsValid = ck::IsValid(InTuningAsset);
    CK_ENSURE_IF_NOT(AssetIsValid,
        TEXT("LiveTune Link: invalid Tuning Asset — cannot link Entity [{}] member [{}]"), InHandle, InMemberName)
    {}
    if (NOT AssetIsValid)
    { return InHandle; }

    const auto* Property = InTuningAsset->GetClass()->FindPropertyByName(InMemberName);
    const auto PropertyExists = Property != nullptr;
    CK_ENSURE_IF_NOT(PropertyExists,
        TEXT("LiveTune Link: [{}] has no member property named [{}]"), InTuningAsset, InMemberName)
    {}
    if (NOT PropertyExists)
    { return InHandle; }

    const auto* StructProperty = CastField<FStructProperty>(Property);
    const auto PropertyIsStruct = StructProperty != nullptr;
    CK_ENSURE_IF_NOT(PropertyIsStruct,
        TEXT("LiveTune Link: [{}].[{}] is not a struct property — a link targets a params-struct member"),
        InTuningAsset, InMemberName)
    {}
    if (NOT PropertyIsStruct)
    { return InHandle; }

    if (const auto* Handler = FCk_LiveTuneHandlerRegistry::Find(StructProperty->Struct);
        Handler != nullptr && Handler->HasFragment)
    {
        const auto EntityHasParams = Handler->HasFragment(InHandle);
        CK_ENSURE_IF_NOT(EntityHasParams,
            TEXT("LiveTune Link: Entity [{}] does not carry params fragment [{}] — Link AFTER the feature's Add, "
                 "on the handle Add returned"),
            InHandle, StructProperty->Struct->GetName())
        {}
        if (NOT EntityHasParams)
        { return InHandle; }
    }

    const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
    const auto WorldIsValid = ck::IsValid(World);
    CK_ENSURE_IF_NOT(WorldIsValid,
        TEXT("LiveTune Link: Entity [{}] has no World — cannot resolve the LiveTune subsystem"), InHandle)
    {}
    if (NOT WorldIsValid)
    { return InHandle; }

    auto* Subsystem = World->GetSubsystem<UCk_LiveTune_Subsystem_UE>();
    const auto SubsystemIsValid = ck::IsValid(Subsystem);
    CK_ENSURE_IF_NOT(SubsystemIsValid,
        TEXT("LiveTune Link: no LiveTune subsystem on World [{}] — Link is only valid in game/PIE worlds"), World)
    {}
    if (NOT SubsystemIsValid)
    { return InHandle; }

    Subsystem->Request_RegisterLink(InHandle, InTuningAsset, InMemberName);
    return InHandle;
}

#endif

// --------------------------------------------------------------------------------------------------------------------
