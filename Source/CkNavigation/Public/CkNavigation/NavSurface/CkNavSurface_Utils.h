#pragma once

#include "CkNavigation/NavSurface/CkNavSurface_Fragment.h"
#include "CkNavigation/NavSurface/CkNavSurface_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"
#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkNavSurface_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// The provider-neutral navigation-surface surface. Every capability an entity or a debugger needs
// from the world's walkable geometry, with no engine type in the signature. One provider answers
// per world, resolved per query through its registered capability table.
// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_NavSurfaceMarkup"))
class CKNAVIGATION_API UCk_Utils_NavSurface_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_NavSurface_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_NavSurfaceMarkup);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|NavSurface",
              DisplayName = "[Ck][NavSurface] Has")
    static bool
    Has(
        const FCk_Handle& InHandle);

public:
    /**
     * Names the provider that answers this world's navigation-surface queries from here on.
     *
     * Written onto the world's transient entity, so it outlives nothing and is asked for on every
     * query rather than cached anywhere.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|NavSurface",
              DisplayName = "[Ck][NavSurface] Request Set Provider",
              meta = (WorldContext = "InWorldContext"))
    static void
    Request_SetProvider(
        const UObject* InWorldContext,
        ECk_NavSurface_Provider InProvider);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|NavSurface",
              DisplayName = "[Ck][NavSurface] Get Provider",
              meta = (WorldContext = "InWorldContext"))
    static ECk_NavSurface_Provider
    Get_Provider(
        const UObject* InWorldContext);

    /**
     * Names whether a second provider answers this world's queries alongside the installing one
     * from here on. The shadowing provider's result is compared and discarded.
     *
     * Written onto the world's transient entity AND into the per-world mirror the boundary query
     * reads; the entity copy is the record, the mirror is what a query off the game thread can reach.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|NavSurface",
              DisplayName = "[Ck][NavSurface] Request Set Shadow Mode",
              meta = (WorldContext = "InWorldContext"))
    static void
    Request_SetShadowMode(
        const UObject* InWorldContext,
        ECk_NavSurface_ShadowMode InMode);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|NavSurface",
              DisplayName = "[Ck][NavSurface] Get Shadow Mode",
              meta = (WorldContext = "InWorldContext"))
    static ECk_NavSurface_ShadowMode
    Get_ShadowMode(
        const UObject* InWorldContext);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|NavSurface",
              DisplayName = "[Ck][NavSurface] Try Project Point",
              meta = (WorldContext = "InWorldContext"))
    static FCk_NavSurface_ProjectionResult
    Try_ProjectPoint(
        const UObject* InWorldContext,
        const FCk_NavSurface_ProjectionQuery& InQuery);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|NavSurface",
              DisplayName = "[Ck][NavSurface] Try Move Along Surface",
              meta = (WorldContext = "InWorldContext"))
    static FCk_NavSurface_MoveAlongSurfaceResult
    Try_MoveAlongSurface(
        const UObject* InWorldContext,
        const FCk_NavSurface_MoveAlongSurfaceQuery& InQuery);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|NavSurface",
              DisplayName = "[Ck][NavSurface] Try Surface Raycast",
              meta = (WorldContext = "InWorldContext"))
    static FCk_NavSurface_RaycastResult
    Try_SurfaceRaycast(
        const UObject* InWorldContext,
        const FCk_NavSurface_RaycastQuery& InQuery);

    // THREAD CONTRACT: callable off the game thread against an immutable field snapshot. C++-only
    // by contract — no UFUNCTION here; Blueprint and AngelScript reach the capability through a
    // separate game-thread wrapper. Results are written into caller-provided storage.
    static auto
    Get_BoundarySegments(
        const UObject* InWorldContext,
        const FCk_NavSurface_BoundaryQuery& InQuery,
        TArray<FCk_NavSurface_BoundarySegment>& OutSegments) -> ECk_NavSurface_QueryStatus;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|NavSurface",
              DisplayName = "[Ck][NavSurface] Get Is Reachable",
              meta = (WorldContext = "InWorldContext"))
    static ECk_NavSurface_Reachability
    Get_IsReachable(
        const UObject* InWorldContext,
        const FCk_NavSurface_ReachabilityQuery& InQuery);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|NavSurface",
              DisplayName = "[Ck][NavSurface] Request Area Markup",
              meta = (WorldContext = "InWorldContext", AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_NavSurfaceMarkup
    Request_AreaMarkup(
        const UObject* InWorldContext,
        const FCk_Request_NavSurface_AreaMarkup& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    /**
     * Paints the request's shape as the well-known impassable area, whatever provider answers for
     * this world. The request's own area tag is NOT read - naming an area is exactly what this
     * convenience exists to spare the caller - so callers construct it with a default tag.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|NavSurface",
              DisplayName = "[Ck][NavSurface] Request Impassable Box",
              meta = (WorldContext = "InWorldContext", AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_NavSurfaceMarkup
    Request_ImpassableBox(
        const UObject* InWorldContext,
        const FCk_Request_NavSurface_AreaMarkup& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|NavSurface",
              DisplayName = "[Ck][NavSurface] Get Is Markup Live")
    static bool
    Get_IsMarkupLive(
        const FCk_Handle_NavSurfaceMarkup& InMarkup);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|NavSurface",
              DisplayName = "[Ck][NavSurface] Get Surface Revision",
              meta = (WorldContext = "InWorldContext"))
    static int64
    Get_SurfaceRevision(
        const UObject* InWorldContext);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|NavSurface",
              DisplayName = "[Ck][NavSurface] Get Surface Bounds",
              meta = (WorldContext = "InWorldContext"))
    static FBox
    Get_SurfaceBounds(
        const UObject* InWorldContext);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|NavSurface",
              DisplayName = "[Ck][NavSurface] Get Provider Health",
              meta = (WorldContext = "InWorldContext"))
    static ECk_NavSurface_ProviderHealth
    Get_ProviderHealth(
        const UObject* InWorldContext);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|NavSurface",
              DisplayName = "[Ck][NavSurface] Get Is Build In Progress",
              meta = (WorldContext = "InWorldContext"))
    static bool
    Get_IsBuildInProgress(
        const UObject* InWorldContext);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|NavSurface",
              DisplayName = "[Ck][NavSurface] Request Surface Rebuild For Testing",
              meta = (WorldContext = "InWorldContext", AutoCreateRefTerm = "InDelegate"))
    static void
    Request_SurfaceRebuild_ForTesting(
        const UObject* InWorldContext,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|NavSurface",
              DisplayName = "[Ck][NavSurface] Bind To OnSurfaceRebuilt",
              meta = (WorldContext = "InWorldContext"))
    static void
    BindTo_OnSurfaceRebuilt(
        const UObject* InWorldContext,
        const FCk_Delegate_NavSurface_OnSurfaceRebuilt& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|NavSurface",
              DisplayName = "[Ck][NavSurface] Unbind From OnSurfaceRebuilt",
              meta = (WorldContext = "InWorldContext"))
    static void
    UnbindFrom_OnSurfaceRebuilt(
        const UObject* InWorldContext,
        const FCk_Delegate_NavSurface_OnSurfaceRebuilt& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
