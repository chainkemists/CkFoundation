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
     * Editor-only behavior: outside WITH_EDITOR the body compiles to a plain pass-through (the UFUNCTION
     * itself must exist in every build for BP/AS call sites to cook).
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|LiveTune",
              DisplayName="[Ck][LiveTune] Link")
    static FCk_Handle
    Link(
        UPARAM(ref) FCk_Handle& InHandle,
        const UObject* InTuningAsset,
        FName InMemberName);
};

// --------------------------------------------------------------------------------------------------------------------
