#pragma once

#include "GameplayTagContainer.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkLabel/CkLabel_Utils.h"
#include "CkOverlapBody/Marker/CkMarker_Fragment.h"

#include "CkOverlapBody/Marker/CkMarker_Fragment_Params.h"
#include "CkOverlapBody/MarkerAndSensor/CkMarkerAndSensor_Utils.h"
#include "CkRecord/Record/CkRecord_Fragment_Params.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkMarker_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FCk_Processor_Marker_Setup;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKOVERLAPBODY_API UCk_Utils_Marker_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Marker_UE);

private:
    struct RecordOfMarkers_Utils : public ck::TCk_Utils_Record<ck::FCk_Fragment_RecordOfMarkers> {};

public:
    friend class ck::FCk_Processor_Marker_Setup;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName="Add New Marker", meta = (GameplayTagFilter = "Marker"))
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Marker_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              meta = (DevelopmentOnly, DefaultToSelf = "InOuter"))
    static void
    PreviewAllMarkers(
        UObject* InOuter,
        FCk_Handle InHandle);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName="Has Marker")
    static bool
    DoHas(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName="Ensure Has Marker")
    static bool
    DoEnsure(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "Get Marker Physics Info")
    static FCk_Marker_PhysicsInfo
    DoGet_PhysicsInfo(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "Get Marker Shape Info")
    static FCk_Marker_ShapeInfo
    DoGet_ShapeInfo(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "Get Marker Debug Info")
    static FCk_Marker_DebugInfo
    DoGet_DebugInfo(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "Get Marker Enable Disable")
    static ECk_EnableDisable
    DoGet_EnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "Get Marker Shape Component")
    static UShapeComponent*
    DoGet_ShapeComponent(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "Get Marker Attached Entity And Actor")
    static FCk_EntityOwningActor_BasicDetails
    DoGet_AttachedEntityAndActor(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName = "Request Enable Disable Marker")
    static void
    DoRequest_EnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName,
        const FCk_Request_Marker_EnableDisable& InRequest);

public:
    // TODO: Add functions to Bind/Unbind OnEnableDisable

public:
    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Has(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName) -> bool;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Ensure(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName) -> bool;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Get_PhysicsInfo(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName) -> FCk_Marker_PhysicsInfo;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Get_ShapeInfo(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName) -> FCk_Marker_ShapeInfo;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Get_DebugInfo(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName) -> FCk_Marker_DebugInfo;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Get_EnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName) -> ECk_EnableDisable;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Get_ShapeComponent(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName) -> UShapeComponent*;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    PreviewSingleMarker(
        UObject* InOuter,
        FCk_Handle InHandle,
        FGameplayTag InMarkerName) -> void;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Get_AttachedEntityAndActor(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName) -> FCk_EntityOwningActor_BasicDetails;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Request_EnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName,
        const FCk_Request_Marker_EnableDisable& InRequest) -> void;

private:
    static auto
    Request_MarkMarker_AsNeedToUpdateTransform(
        FCk_Handle InMarkerHandle) -> void;

private:
    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy, typename T_Func>
    static auto
    DoPerformCommonMarkerUtilFunc(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName,
        T_Func InFunc) -> decltype(InFunc(InHandle, InMarkerName));

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy, typename T_Func>
    static auto
    DoPerformCommonMarkerUtilVoidFunc(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName,
        T_Func InFunc) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
// Definitions

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Marker_UE::
    Has(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> bool
{
    return DoPerformCommonMarkerUtilFunc<T_FragmentQueryPolicy>(InHandle, InMarkerName, [&](FCk_Handle InMarkerEntity, FGameplayTag InMarkerNameToUse)
    {
        return InMarkerEntity.Has_All<ck::FCk_Fragment_Marker_Params, ck::FCk_Fragment_Marker_Current>() &&
               UCk_Utils_GameplayLabel_UE::Has(InMarkerEntity) &&
               UCk_Utils_GameplayLabel_UE::Get_Label(InMarkerEntity) == InMarkerNameToUse;
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Marker_UE::
    Ensure(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has<T_FragmentQueryPolicy>(InHandle ,InMarkerName), TEXT("Handle [{}] does NOT have a Marker with Name [{}]"), InHandle, InMarkerName)
    { return false; }

    return true;
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Marker_UE::
    Get_PhysicsInfo(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> FCk_Marker_PhysicsInfo
{
    if (NOT Ensure<T_FragmentQueryPolicy>(InHandle, InMarkerName))
    { return {}; }

    return DoPerformCommonMarkerUtilFunc<T_FragmentQueryPolicy>(InHandle, InMarkerName, [&](FCk_Handle InMarkerEntity, FGameplayTag)
    {
        return InMarkerEntity.Get<ck::FCk_Fragment_Marker_Params>().Get_Params().Get_PhysicsParams();
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Marker_UE::
    Get_ShapeInfo(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> FCk_Marker_ShapeInfo
{
    if (NOT Ensure<T_FragmentQueryPolicy>(InHandle, InMarkerName))
    { return {}; }

    return DoPerformCommonMarkerUtilFunc<T_FragmentQueryPolicy>(InHandle, InMarkerName, [&](FCk_Handle InMarkerEntity, FGameplayTag)
    {
        return InMarkerEntity.Get<ck::FCk_Fragment_Marker_Params>().Get_Params().Get_ShapeParams();
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Marker_UE::
    Get_DebugInfo(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> FCk_Marker_DebugInfo
{
    if (NOT Ensure<T_FragmentQueryPolicy>(InHandle, InMarkerName))
    { return {}; }

    return DoPerformCommonMarkerUtilFunc<T_FragmentQueryPolicy>(InHandle, InMarkerName, [&](FCk_Handle InMarkerEntity, FGameplayTag)
    {
        return InMarkerEntity.Get<ck::FCk_Fragment_Marker_Params>().Get_Params().Get_DebugParams();
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Marker_UE::
    Get_EnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> ECk_EnableDisable
{
    if (NOT Ensure<T_FragmentQueryPolicy>(InHandle, InMarkerName))
    { return {}; }

    return DoPerformCommonMarkerUtilFunc<T_FragmentQueryPolicy>(InHandle, InMarkerName, [&](FCk_Handle InMarkerEntity, FGameplayTag)
    {
        return InMarkerEntity.Get<ck::FCk_Fragment_Marker_Current>().Get_EnableDisable();
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Marker_UE::
    Get_ShapeComponent(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> UShapeComponent*
{
    if (NOT Ensure<T_FragmentQueryPolicy>(InHandle, InMarkerName))
    { return {}; }

    return DoPerformCommonMarkerUtilFunc<T_FragmentQueryPolicy>(InHandle, InMarkerName, [&](FCk_Handle InMarkerEntity, FGameplayTag)
    {
        return InMarkerEntity.Get<ck::FCk_Fragment_Marker_Current>().Get_Marker().Get();
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Marker_UE::
    PreviewSingleMarker(
        UObject* InOuter,
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> void
{
    if (NOT Ensure<T_FragmentQueryPolicy>(InHandle, InMarkerName))
    { return; }

    DoPerformCommonMarkerUtilVoidFunc<T_FragmentQueryPolicy>(InHandle, InMarkerName, [&](FCk_Handle InMarkerEntity, FGameplayTag)
    {
        const auto& markerParams = InMarkerEntity.Get<ck::FCk_Fragment_Marker_Params>().Get_Params();
        const auto& markerCurrent = InMarkerEntity.Get<ck::FCk_Fragment_Marker_Current>();

        UCk_Utils_MarkerAndSensor_UE::Draw_Marker_DebugLines(InOuter, markerCurrent, markerParams);
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Marker_UE::
    Get_AttachedEntityAndActor(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> FCk_EntityOwningActor_BasicDetails
{
    if (NOT Ensure<T_FragmentQueryPolicy>(InHandle, InMarkerName))
    { return {}; }

    return DoPerformCommonMarkerUtilFunc<T_FragmentQueryPolicy>(InHandle, InMarkerName, [&](FCk_Handle InMarkerEntity, FGameplayTag)
    {
        return InMarkerEntity.Get<ck::FCk_Fragment_Marker_Current>().Get_AttachedEntityAndActor();
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Marker_UE::
    Request_EnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName,
        const FCk_Request_Marker_EnableDisable& InRequest)
    -> void
{
    if (NOT Ensure<T_FragmentQueryPolicy>(InHandle, InMarkerName))
    { return; }

    DoPerformCommonMarkerUtilVoidFunc<T_FragmentQueryPolicy>(InHandle, InMarkerName, [&](FCk_Handle InMarkerEntity, FGameplayTag)
    {
        InMarkerEntity.AddOrGet<ck::FCk_Fragment_Marker_Requests>()._Requests = InRequest;
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy, typename T_Func>
auto
    UCk_Utils_Marker_UE::
    DoPerformCommonMarkerUtilFunc(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName,
        T_Func InFunc)
    -> decltype(InFunc(InHandle, InMarkerName))
{
    if constexpr(T_FragmentQueryPolicy == ECk_FragmentQuery_Policy::CurrentEntity)
    {
        return InFunc(InHandle, InMarkerName);
    }
    else if constexpr(T_FragmentQueryPolicy == ECk_FragmentQuery_Policy::EntityInRecord)
    {
        const auto& MaybeMarker = RecordOfMarkers_Utils::Get_RecordEntryIf(InHandle, [&](FCk_Handle InRecordEntry) -> bool
        {
            return Has<ECk_FragmentQuery_Policy::CurrentEntity>(InRecordEntry, InMarkerName);
        });

        CK_ENSURE_IF_NOT(ck::IsValid(MaybeMarker), TEXT("Entity [{}] does NOT have any Marker with Name [{}]"), InHandle, InMarkerName)
        { return {}; }

        return InFunc(MaybeMarker, InMarkerName);
    }
    else
    {
        static_assert("Invalid Fragment Query policy");
        return {};
    }
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy, typename T_Func>
auto
    UCk_Utils_Marker_UE::
    DoPerformCommonMarkerUtilVoidFunc(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName,
        T_Func InFunc)
    -> void
{
    if constexpr(T_FragmentQueryPolicy == ECk_FragmentQuery_Policy::CurrentEntity)
    {
        InFunc(InHandle, InMarkerName);
    }
    else if constexpr(T_FragmentQueryPolicy == ECk_FragmentQuery_Policy::EntityInRecord)
    {
        const auto& MaybeMarker = RecordOfMarkers_Utils::Get_RecordEntryIf(InHandle, [&](FCk_Handle InRecordEntry) -> bool
        {
            return Has<ECk_FragmentQuery_Policy::CurrentEntity>(InRecordEntry, InMarkerName);
        });

        CK_ENSURE_IF_NOT(ck::IsValid(MaybeMarker), TEXT("Entity [{}] does NOT have any Marker with Name [{}]"), InHandle, InMarkerName)
        { return; }

        InFunc(MaybeMarker, InMarkerName);
    }
    else
    {
        static_assert("Invalid Fragment Query policy");
    }
}

// --------------------------------------------------------------------------------------------------------------------
