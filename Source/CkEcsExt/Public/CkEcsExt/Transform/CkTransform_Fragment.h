#pragma once

#include "CkCore/Time/CkTime.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkSignal/CkSignal_Macros.h"

#include "CkCore/TypeTraits/CkTypeTraits.h"

#include <variant>
#include <Engine/SkeletalMeshSocket.h>
#include <Engine/StaticMeshSocket.h>

#include "CkTransform_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Transform_UE;
class UCk_Fragment_Transform_Rep;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_Transform_Updated);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECSEXT_API FFragment_Transform_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Transform_Params);

    public:
        using ParamsType = FCk_Transform_ParamsData;

    private:
        ParamsType _Data;

    public:
        CK_PROPERTY_GET(_Data);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Transform_Params, _Data);
    };

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

    struct CKECSEXT_API FFragment_Transform_SkeletalMeshSocket
    {
    public:
        CK_GENERATED_BODY(FFragment_Transform_SkeletalMeshSocket);

    public:
        FFragment_Transform_SkeletalMeshSocket() = default;

        explicit
        FFragment_Transform_SkeletalMeshSocket(
            const USkeletalMeshComponent* InComponent,
            const USkeletalMeshSocket* InSocket);

    private:
        TWeakObjectPtr<const USkeletalMeshComponent> _Component;
        TWeakObjectPtr<const USkeletalMeshSocket> _Socket;

    public:
        CK_PROPERTY_GET(_Component);
        CK_PROPERTY_GET(_Socket);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECSEXT_API FFragment_Transform_StaticMeshSocket
    {
    public:
        CK_GENERATED_BODY(FFragment_Transform_StaticMeshSocket);

    public:
        FFragment_Transform_StaticMeshSocket() = default;

        explicit
            FFragment_Transform_StaticMeshSocket(
                const UStaticMeshComponent* InComponent,
                const UStaticMeshSocket* InSocket);

    private:
        TWeakObjectPtr<const UStaticMeshComponent> _Component;
        TWeakObjectPtr<const UStaticMeshSocket> _Socket;

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

    struct FFragment_Transform
    {
    public:
        CK_GENERATED_BODY(FFragment_Transform);

    public:
        friend class FProcessor_Transform_HandleRequests;
        friend class FProcessor_Transform_Replicate;
        friend class FProcessor_Transform_SyncFromActor;
        friend class FProcessor_Transform_SyncFromSkeletalMeshSocket;
        friend class FProcessor_Transform_SyncFromStaticMeshSocket;
        friend class FProcessor_Transform_SyncFromActor;
        friend class UCk_Fragment_Transform_Rep;
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

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKECSEXT_API, TransformUpdate, FCk_Delegate_Transform_OnUpdate_MC, FCk_Handle_Transform, FTransform);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID(ck::FFragment_Transform_RootComponent, ck::IsValid_Policy_Default,
[&](const ck::FFragment_Transform_RootComponent& InRootComponentFragment)
{
    return ck::IsValid(InRootComponentFragment.Get_RootComponent());
});

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_Transform_Replicate; }

UCLASS(Blueprintable)
class CKECSEXT_API UCk_Fragment_Transform_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_Transform_Rep);

public:
    auto
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>&) const -> void override;

protected:
    auto
    PostLink() -> void override;

public:
    UFUNCTION()
    void
    OnRep_Location();

    UFUNCTION()
    void
    OnRep_Rotation();

    UFUNCTION()
    void
    OnRep_Scale();

private:
    UPROPERTY(ReplicatedUsing = OnRep_Location)
    FVector _Location;

    UPROPERTY(ReplicatedUsing = OnRep_Rotation)
    FQuat _Rotation;

    UPROPERTY(ReplicatedUsing = OnRep_Scale)
    FVector _Scale;

public:
    auto
    Set_Location(
        const FVector& OutLocation) -> void;

    auto
    Set_Rotation(
        const FQuat& OutRotation) -> void;

    auto
    Set_Scale(
        const FVector& OutScale) -> void;

public:
    CK_PROPERTY_GET(_Location);
    CK_PROPERTY_GET(_Rotation);
    CK_PROPERTY_GET(_Scale);
};

// --------------------------------------------------------------------------------------------------------------------