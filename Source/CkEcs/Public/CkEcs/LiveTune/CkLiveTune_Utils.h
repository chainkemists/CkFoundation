#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkLiveTune_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECS_API UCk_Utils_LiveTune_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_LiveTune_UE);

public:
    /**
     * Links the feature params just Added on InHandle to a member property of a tuning asset, so an
     * editor-time edit of that member is re-applied to this entity live (through the handler registered
     * for the member's params type — FCk_LiveTuneHandlerRegistry). Call after any normal feature Add, on
     * the handle Add returned:
     *
     *     auto Health = UCk_Utils_FloatAttribute_UE::Add(InHandle, Tuning->Get_Health(), ECk_Replication::Replicates);
     *     UCk_Utils_LiveTune_UE::Link(Health, Tuning, GET_MEMBER_NAME_CHECKED(UMy_Tuning_PDA, _Health));
     *
     * The tuning asset is ANY UObject with params-struct UPROPERTYs (plain PDA, AS asset literal) — no
     * base class, no registration. The member must be a TOP-LEVEL struct UPROPERTY of the asset's class.
     * Editor-only: outside WITH_EDITOR this is an empty inline.
     */
#if WITH_EDITOR
    static auto
    Link(
        FCk_Handle& InHandle,
        const UObject* InTuningAsset,
        FName InMemberName) -> FCk_Handle;
#else
    static auto
    Link(
        FCk_Handle& InHandle,
        const UObject* InTuningAsset,
        FName InMemberName) -> FCk_Handle
    {
        return InHandle;
    }
#endif
};

// --------------------------------------------------------------------------------------------------------------------
