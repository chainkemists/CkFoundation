#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkGroundNav/Volume/CkGroundNavVolume_Fragment.h"
#include "CkGroundNav/Volume/CkGroundNavVolume_Fragment_Data.h"

#include "CkGroundNavVolume_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_GroundNavVolume"))
class CKGROUNDNAV_API UCk_Utils_GroundNavVolume_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_GroundNavVolume_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_GroundNavVolume);

public:
    /** Add the grounded-navigation feature to InOwner as a child entity carrying the bake params.
     *  FProcessor_GroundNavVolume_Setup consumes its NeedsSetup tag on the next tick and, unless the
     *  params opted out, arms the first build from there. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Add Volume Feature")
    static FCk_Handle_GroundNavVolume
    Add(
        UPARAM(ref) FCk_Handle& InOwner,
        const FCk_Fragment_GroundNavVolume_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Has Volume Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

public:
    /** Bakes the volume, resuming across as many ticks as the probe budget needs. A request arriving
     *  while a build is already underway is an idempotent no-op unless it forces a restart.
     *
     *  The completion delegate fires when the BUILD ends, not when the request is accepted — that is
     *  ticks later, and it is the only outcome a caller can act on. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Request Build",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_GroundNavVolume
    Request_Build(
        UPARAM(ref) FCk_Handle_GroundNavVolume& InVolume,
        const FCk_Request_GroundNavVolume_Build& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Get Is Built")
    static bool
    Get_IsBuilt(
        const FCk_Handle_GroundNavVolume& InVolume);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Get Is Building")
    static bool
    Get_IsBuilding(
        const FCk_Handle_GroundNavVolume& InVolume);

    /** Bumps on every completed build. A consumer holding a field compares against this to learn it is
     *  behind — staleness is derived here rather than stored anywhere. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Get Build Epoch")
    static int64
    Get_BuildEpoch(
        const FCk_Handle_GroundNavVolume& InVolume);

public:
    /**
     * The published field, or an invalid pointer if nothing has been built yet.
     *
     * C++ only, and deliberately so: the caller takes a shared reference to an immutable structure and
     * is guaranteed a self-consistent field for as long as it holds it. There is no Blueprint or
     * AngelScript shape for that guarantee, and one that copied the field per call would cost more than
     * every query made through it.
     */
    static auto
    Get_Field(
        const FCk_Handle_GroundNavVolume& InVolume) -> ck::groundnav::FCk_GroundNav_FieldPtr;
};

// --------------------------------------------------------------------------------------------------------------------
