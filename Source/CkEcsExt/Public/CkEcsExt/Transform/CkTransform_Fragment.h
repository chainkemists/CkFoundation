#pragma once

#include "CkCore/Time/CkTime.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include <variant>
#include <Components/MeshComponent.h>
#include <Engine/EngineTypes.h>

#include "CkTransform_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Transform_UE;
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_Transform_Updated);
    CK_DEFINE_ECS_TAG(FTag_Transform_Movable);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_Transform_Params = FCk_Transform_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECSEXT_API FFragment_Transform_RootComponent
    {
    public:
        CK_GENERATED_BODY(FFragment_Transform_RootComponent);

    private:
        TWeakObjectPtr<USceneComponent> _RootComponent;

    public:
        CK_PROPERTY_GET(_RootComponent);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Transform_RootComponent, _RootComponent);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECSEXT_API FFragment_Transform_RootComponentTeleportType
    {
    public:
        CK_GENERATED_BODY(FFragment_Transform_RootComponentTeleportType);

    private:
        ETeleportType _TeleportType = ETeleportType::None;

    public:
        CK_PROPERTY_GET(_TeleportType);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Transform_RootComponentTeleportType, _TeleportType);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECSEXT_API FFragment_Transform_MeshSocket
    {
    public:
        CK_GENERATED_BODY(FFragment_Transform_MeshSocket);

    public:
        FFragment_Transform_MeshSocket() = default;

        explicit
        FFragment_Transform_MeshSocket(
            const UMeshComponent* InComponent,
            FName InSocket);

    private:
        TWeakObjectPtr<const UMeshComponent> _Component;
        FName _Socket;

    public:
        CK_PROPERTY_GET(_Component);
        CK_PROPERTY_GET(_Socket);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECSEXT_API FFragment_TransformInterpolation_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_TransformInterpolation_Params);

    public:
        using ParamsType = FCk_TransformInterpolation_ParamsData;

    private:
        ParamsType _Data;

    public:
        CK_PROPERTY_GET(_Data);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_TransformInterpolation_Params, _Data);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_TransformInterpolation_NewGoal_Location
    {
        friend class FProcessor_Transform_InterpolateToGoal_Location;

    public:
        CK_GENERATED_BODY(FFragment_TransformInterpolation_NewGoal_Location);

    private:
        FVector _InterpolationOffset;
        FCk_Time _DeltaT;

    public:
        CK_PROPERTY_GET(_InterpolationOffset);
        CK_PROPERTY(_DeltaT);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_TransformInterpolation_NewGoal_Location, _InterpolationOffset);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_TransformInterpolation_NewGoal_Rotation
    {
        friend class FProcessor_Transform_InterpolateToGoal_Rotation;

    public:
        CK_GENERATED_BODY(FFragment_TransformInterpolation_NewGoal_Rotation);

    private:
        FRotator _InterpolationOffset;
        FCk_Time _DeltaT;

    public:
        CK_PROPERTY_GET(_InterpolationOffset);
        CK_PROPERTY(_DeltaT);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_TransformInterpolation_NewGoal_Rotation, _InterpolationOffset);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_Transform_Previous
    {
    public:
        CK_GENERATED_BODY(FFragment_Transform_Previous);

    private:
        FTransform _Transform;

    public:
        CK_PROPERTY_GET(_Transform);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Transform_Previous, _Transform);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_Transform
    {
    public:
        CK_GENERATED_BODY(FFragment_Transform);

    public:
        friend class FProcessor_Transform_HandleRequests;
        friend class FProcessor_Transform_Replicate;
        friend class FProcessor_Transform_SyncFromActor;
        friend class FProcessor_Transform_SyncFromMeshSocket;
        friend class FProcessor_Transform_SyncFromMeshSocket_SceneNode;
        friend UCk_Utils_Transform_UE;

    private:
        FTransform _Transform;
        ECk_TransformComponents _ComponentsModified = ECk_TransformComponents::None;

    public:
        CK_PROPERTY_GET(_Transform);
        CK_PROPERTY(_ComponentsModified);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Transform, _Transform);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECSEXT_API FFragment_Transform_Requests
    {
        CK_GENERATED_BODY(FFragment_Transform_Requests);

    public:
        friend class FProcessor_Transform_HandleRequests;
        friend class UCk_Utils_Transform_UE;

    public:
        using RotationRequestType = std::variant<FCk_Request_Transform_SetRotation, FCk_Request_Transform_AddRotationOffset>;
        using RotationRequestList = TArray<RotationRequestType>;

        using LocationRequestType = std::variant<FCk_Request_Transform_SetLocation, FCk_Request_Transform_AddLocationOffset>;
        using LocationRequestList = TArray<LocationRequestType>;

        using ScaleRequestType    = FCk_Request_Transform_SetScale;
        using ScaleRequestList    = TOptional<ScaleRequestType>;

    private:
        RotationRequestList _RotationRequests;
        LocationRequestList _LocationRequests;
        ScaleRequestList _ScaleRequests;

    public:
        CK_PROPERTY_GET(_RotationRequests);
        CK_PROPERTY_GET(_LocationRequests);
        CK_PROPERTY_GET(_ScaleRequests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKECSEXT_API, TransformUpdate, FCk_Delegate_Transform_OnUpdate, FCk_Handle_Transform, FTransform);
}

// --------------------------------------------------------------------------------------------------------------------
// Replicated Data USTRUCTs — one per transform component for granular replication

USTRUCT()
struct CKECSEXT_API FCk_RepData_Location
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RepData_Location);

public:
    UPROPERTY()
    FVector Value = FVector::ZeroVector;

public:
    CK_DEFINE_CONSTRUCTORS(FCk_RepData_Location, Value);
};

USTRUCT()
struct CKECSEXT_API FCk_RepData_Rotation
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RepData_Rotation);

public:
    UPROPERTY()
    FQuat Value = FQuat::Identity;

public:
    CK_DEFINE_CONSTRUCTORS(FCk_RepData_Rotation, Value);
};

USTRUCT()
struct CKECSEXT_API FCk_RepData_Scale
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RepData_Scale);

public:
    UPROPERTY()
    FVector Value = FVector::OneVector;

public:
    CK_DEFINE_CONSTRUCTORS(FCk_RepData_Scale, Value);
};

// --------------------------------------------------------------------------------------------------------------------
// Entity-side container entry references

namespace ck
{
    using FFragment_ContainerRef_Location = TFragment_ContainerEntryRef<FCk_RepData_Location>;
    using FFragment_ContainerRef_Rotation = TFragment_ContainerEntryRef<FCk_RepData_Rotation>;
    using FFragment_ContainerRef_Scale    = TFragment_ContainerEntryRef<FCk_RepData_Scale>;
}

// --------------------------------------------------------------------------------------------------------------------
