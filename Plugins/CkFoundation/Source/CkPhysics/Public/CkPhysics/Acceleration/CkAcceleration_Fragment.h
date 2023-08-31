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
    struct FCk_Tag_Acceleration_Setup {};
    struct FCk_Tag_AccelerationModifier_SingleTarget {};
    struct FCk_Tag_AccelerationModifier_SingleTarget_Setup {};

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FCk_Fragment_Acceleration_Params
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_Acceleration_Params);

    public:
        using ParamsType = FCk_Fragment_Acceleration_ParamsData;

    public:
        FCk_Fragment_Acceleration_Params() = default;
        explicit FCk_Fragment_Acceleration_Params(ParamsType InParams);

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FCk_Fragment_Acceleration_Current
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_Acceleration_Current);

    public:
        friend class FCk_Processor_Acceleration_Setup;
        friend class UCk_Utils_Acceleration_UE;
        friend class FCk_Processor_AccelerationModifier_SingleTarget_Setup;
        friend class FCk_Processor_AccelerationModifier_SingleTarget_Teardown;

    public:
        FCk_Fragment_Acceleration_Current() = default;
        explicit FCk_Fragment_Acceleration_Current(
            FVector InAcceleration);

    private:
        FVector _CurrentAcceleration = FVector::ZeroVector;

    public:
        CK_PROPERTY_GET(_CurrentAcceleration);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FCk_Fragment_Acceleration_Target : public FCk_Fragment_EntityHolder
    {
        using FCk_Fragment_EntityHolder::FCk_Fragment_EntityHolder;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FCk_Fragment_Acceleration_Channel : public FCk_Fragment_GameplayLabel
    {
        using FCk_Fragment_GameplayLabel::FCk_Fragment_GameplayLabel;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FCk_Fragment_RecordOfAccelerationModifiers : public FCk_Fragment_Record {};
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FCk_Processor_Acceleration_Replicate; }


UCLASS(Blueprintable)
class CKPHYSICS_API UCk_Fragment_Acceleration_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Fragment_Acceleration_Rep);

public:
    friend class ck::FCk_Processor_Acceleration_Replicate;

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
