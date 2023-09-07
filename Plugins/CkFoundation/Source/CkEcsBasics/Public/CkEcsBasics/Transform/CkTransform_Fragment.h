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

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    struct FTag_Transform_Updated {};

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECSBASICS_API FFragment_Transform_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Transform_Params);

    public:
        using SettingsType = FCk_Transform_Interpolation_Settings;

    private:
        FCk_Transform_ParamsData _Data;

    public:
        CK_PROPERTY_GET(_Data);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Transform_Params, _Data);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_Transform_NewGoal_Location
    {
        friend class FProcessor_Transform_InterpolateToGoal;

    public:
        CK_GENERATED_BODY(FFragment_Transform_NewGoal_Location);

    private:
        FVector _InterpolationOffset;
        FCk_Time _DeltaT;

    public:
        CK_PROPERTY_GET(_InterpolationOffset);
        CK_PROPERTY(_DeltaT);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Transform_NewGoal_Location, _InterpolationOffset);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_ConstOrNonConst>
    struct TFragment_Transform
    {
    public:
        using ThisType = TFragment_Transform<T_ConstOrNonConst>;
        using MutabilityPolicy = policy::TMutability<T_ConstOrNonConst>;

    public:
        friend class FProcessor_Transform_HandleRequests;
        friend class UCk_Fragment_Transform_Rep;
        friend UCk_Utils_Transform_UE;

    private:
        FTransform _Transform;
        ECk_TransformComponents _ComponentsModified = ECk_TransformComponents::None;

    public:
        CK_PROPERTY_GET(_Transform);
        CK_PROPERTY(_ComponentsModified);

    public:
        CK_DEFINE_CONSTRUCTORS(TFragment_Transform<T_ConstOrNonConst>, _Transform);
    };

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_Transform_Current = TFragment_Transform<type_traits::NonConst>;

    using FFragment_ImmutableTransform_Current = TFragment_Transform<type_traits::Const>;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECSBASICS_API FFragment_Transform_Requests
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

    class FProcessor_Transform_Replicate;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable)
class CKECSBASICS_API UCk_Fragment_Transform_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_Transform_Rep);

public:
    friend class ck::FProcessor_Transform_Replicate;

public:
    virtual auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const -> void override;

public:
    UFUNCTION()
    void OnRep_Location();

    UFUNCTION()
    void OnRep_Rotation();

    UFUNCTION()
    void OnRep_Scale();

private:
    UPROPERTY(ReplicatedUsing = OnRep_Location)
    FVector _Location;

    UPROPERTY(ReplicatedUsing = OnRep_Rotation)
    FQuat _Rotation;

    UPROPERTY(ReplicatedUsing = OnRep_Scale)
    FVector _Scale;
};

// --------------------------------------------------------------------------------------------------------------------
