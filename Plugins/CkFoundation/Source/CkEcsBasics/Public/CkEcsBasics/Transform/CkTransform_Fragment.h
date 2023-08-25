#pragma once

#include <CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h>

#include <CkEcsBasics/Transform/CkTransform_Fragment_Params.h>

#include <CkMacros/CkMacros.h>

#include <CkTypeTraits/CkTypeTraits.h>

#include <variant>

#include "CkCore/Time/CkTime.h"

#include "CkTransform_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Transform_UE;
class UCk_Fragment_Transform_Rep;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSBASICS_API FCk_Fragment_Interpolation_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Interpolation_Params);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess, ValidEnumValues="Linear"))
    ECk_Interpolation_Strategy _Strategy = ECk_Interpolation_Strategy::Linear;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    float _MaxSmoothUpdateDistance = 256.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    float _NoSmoothUpdateDistance = 384.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    FCk_Time _SmoothLocationTime = FCk_Time{0.1f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    FCk_Time _SmoothRotationTime = FCk_Time{0.05f};

public:
    CK_PROPERTY(_Strategy);
    CK_PROPERTY(_MaxSmoothUpdateDistance);
    CK_PROPERTY(_NoSmoothUpdateDistance);
    CK_PROPERTY(_SmoothLocationTime);
    CK_PROPERTY(_SmoothRotationTime);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSBASICS_API FCk_Fragment_Transform_NewGoal_Location
{
    GENERATED_BODY()

    friend class FCk_Processor_Transform_InterpolateToGoal;

public:
    CK_GENERATED_BODY(FCk_Fragment_Transform_NewGoal_Location);

private:
    UPROPERTY()
    FVector _InterpolationOffset;

    UPROPERTY()
    FCk_Time _DeltaT;

public:
    CK_PROPERTY(_InterpolationOffset);
    CK_PROPERTY(_DeltaT);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    struct FTag_Transform_Updated {};

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_ConstOrNonConst>
    struct TFragment_Transform
    {
    public:
        using ThisType = TFragment_Transform<T_ConstOrNonConst>;
        using MutabilityPolicy = policy::TMutability<T_ConstOrNonConst>;

    public:
        friend class FCk_Processor_Transform_HandleRequests;
        friend class UCk_Fragment_Transform_Rep;
        friend UCk_Utils_Transform_UE;

    public:
        TFragment_Transform() = default;
        explicit TFragment_Transform(FTransform InTransform);

    private:
        FTransform _Transform;
        ECk_TransformComponents _ComponentsModified = ECk_TransformComponents::None;

    public:
        CK_PROPERTY(_Transform);
        CK_PROPERTY(_ComponentsModified);
    };

    // --------------------------------------------------------------------------------------------------------------------

    using FCk_Fragment_Transform_Current = TFragment_Transform<type_traits::NonConst>;

    using FCk_Fragment_ImmutableTransform_Current = TFragment_Transform<type_traits::Const>;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECSBASICS_API FCk_Fragment_Transform_Requests
    {
        CK_GENERATED_BODY(FCk_Fragment_Transform_Requests);

    public:
        friend class FCk_Processor_Transform_HandleRequests;
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
}

// --------------------------------------------------------------------------------------------------------------------
// Definitions

namespace ck
{
    template <typename T_ConstOrNonConst>
    TFragment_Transform<T_ConstOrNonConst>::
        TFragment_Transform(
            FTransform InTransform)
        : _Transform(std::move(InTransform))
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FCk_Processor_Transform_Replicate; }

UCLASS(Blueprintable)
class CKECSBASICS_API UCk_Fragment_Transform_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Fragment_Transform_Rep);

public:
    friend class ck::FCk_Processor_Transform_Replicate;

public:
    virtual auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const -> void override;

public:
    UFUNCTION()
    void OnRep_Transform();

    UFUNCTION()
    void OnRep_Rotation();

    UFUNCTION()
    void OnRep_Scale();

private:
    UPROPERTY(ReplicatedUsing = OnRep_Transform)
    FVector _Location;

    UPROPERTY(ReplicatedUsing = OnRep_Rotation)
    FQuat _Rotation;

    UPROPERTY(ReplicatedUsing = OnRep_Scale)
    FVector _Scale;
};

// --------------------------------------------------------------------------------------------------------------------
