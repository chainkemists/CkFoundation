#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Fragment.h"
#include "CkLabel/CkLabel_Fragment.h"
#include "CkMacros/CkMacros.h"

#include "CkPhysics/Acceleration/CkAcceleration_Fragment_Params.h"
#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkAcceleration_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Acceleration_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FTag_Acceleration_Setup {};
    struct FTag_AccelerationModifier_SingleTarget {};
    struct FTag_AccelerationModifier_SingleTarget_Setup {};

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_Acceleration_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Acceleration_Params);

    public:
        using ParamsType = FCk_Fragment_Acceleration_ParamsData;

    public:
        FFragment_Acceleration_Params() = default;
        explicit FFragment_Acceleration_Params(ParamsType InParams);

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_Acceleration_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Acceleration_Current);

    public:
        friend class FProcessor_Acceleration_Setup;
        friend class UCk_Utils_Acceleration_UE;
        friend class FProcessor_AccelerationModifier_SingleTarget_Setup;
        friend class FProcessor_AccelerationModifier_SingleTarget_Teardown;

    public:
        FFragment_Acceleration_Current() = default;
        explicit FFragment_Acceleration_Current(
            FVector InAcceleration);

    private:
        FVector _CurrentAcceleration = FVector::ZeroVector;

    public:
        CK_PROPERTY_GET(_CurrentAcceleration);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_Acceleration_Target : public FFragment_EntityHolder
    {
        using FFragment_EntityHolder::FFragment_EntityHolder;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_Acceleration_Channel : public FFragment_GameplayLabel
    {
        using FFragment_GameplayLabel::FFragment_GameplayLabel;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_RecordOfAccelerationModifiers : public FFragment_Record {};

    // --------------------------------------------------------------------------------------------------------------------

    class FProcessor_Acceleration_Replicate;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable)
class CKPHYSICS_API UCk_Fragment_Acceleration_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Fragment_Acceleration_Rep);

public:
    friend class ck::FProcessor_Acceleration_Replicate;

public:
    virtual auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const -> void override;

public:
    UFUNCTION()
    void OnRep_Acceleration();

private:
    UPROPERTY(ReplicatedUsing = OnRep_Acceleration)
    FVector _Acceleration = FVector::ZeroVector;
};

// --------------------------------------------------------------------------------------------------------------------
