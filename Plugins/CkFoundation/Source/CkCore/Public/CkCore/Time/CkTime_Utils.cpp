#include "CkTime_Utils.h"

#include "CkEnsure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Time_UE::
    Make_FromSeconds(float InSeconds)
    -> FCk_Time
{
    return FCk_Time{InSeconds};
}

auto
    UCk_Utils_Time_UE::
    Get_FrameNumber()
    -> int64
{
    return GFrameNumber;
}

auto
    UCk_Utils_Time_UE::
    Get_WorldTime(const FCk_Utils_Time_GetWorldTime_Params& InParams)
    -> FCk_Utils_Time_GetWorldTime_Result
{
    const auto& IsParamsWorldValid  = IsValid(InParams.Get_World());
    const auto& IsParamsObjectValid = IsValid(InParams.Get_Object());

    CK_ENSURE_IF_NOT(IsParamsWorldValid || IsParamsObjectValid, TEXT("The UObject or the UWorld must be valid to get the correct time"))
    { return {};}

    const auto& World = [&]() -> const UWorld*
    {
        if (IsParamsWorldValid)
        { return InParams.Get_World(); }

        return InParams.Get_Object()->GetWorld();
    }();

    const auto& IsWorldValid = IsValid(World);

    CK_ENSURE_IF_NOT(IsWorldValid, TEXT("Could not get a valid world"))
    { return {};}

    switch(const auto& WorldTimeType = InParams.Get_TimeType())
    {
        case ECk_Time_WorldTimeType::PausedAndDilatedAndClamped:
        {
            return FCk_Utils_Time_GetWorldTime_Result
            {
                FCk_Time_Unreal
                {
                    FCk_Time(World->GetTimeSeconds()),
                    WorldTimeType
                }
            };
        }
        case ECk_Time_WorldTimeType::DilatedAndClampedOnly:
        {
            return FCk_Utils_Time_GetWorldTime_Result
            {
                FCk_Time_Unreal
                {
                    FCk_Time(World->GetUnpausedTimeSeconds()),
                    WorldTimeType
                }
            };
        }
        case ECk_Time_WorldTimeType::RealTime:
        {
            return FCk_Utils_Time_GetWorldTime_Result
            {
                FCk_Time_Unreal
                {
                    FCk_Time(World->GetRealTimeSeconds()),
                    WorldTimeType
                }
            };
        }
        case ECk_Time_WorldTimeType::AudioTime:
        {
            return FCk_Utils_Time_GetWorldTime_Result
            {
                FCk_Time_Unreal
                {
                    FCk_Time(World->GetAudioTimeSeconds()),
                    WorldTimeType
                }
            };
        }
        case ECk_Time_WorldTimeType::DeltaTime:
        {
            return FCk_Utils_Time_GetWorldTime_Result
            {
                FCk_Time_Unreal
                {
                    FCk_Time(World->GetDeltaSeconds()),
                    WorldTimeType
                }
            };
        }
        default:
        {
            CK_INVALID_ENUM(WorldTimeType);
            return {};
        }
    };
}

auto
    UCk_Utils_Time_UE::
    Get_Milliseconds(const FCk_Time& InTime)
    -> float
{
    return InTime.Get_Milliseconds();
}

auto
    UCk_Utils_Time_UE::
    Add
    (
        const FCk_Time& InA,
        const FCk_Time& InB
    )
    -> FCk_Time
{
    return InA + InB;
}

auto
    UCk_Utils_Time_UE::
    Subtract
    (
        const FCk_Time& InA,
        const FCk_Time& InB
    )
    -> FCk_Time
{
    return InA - InB;
}

auto
    UCk_Utils_Time_UE::
    Divide
    (
        const FCk_Time& InA,
        const FCk_Time& InB
    )
    -> FCk_Time
{
    return InA / InB;
}

auto
    UCk_Utils_Time_UE::
    Multiply
    (
        const FCk_Time& InA,
        const FCk_Time& InB
    )
    -> FCk_Time
{
    return InA * InB;
}

auto
    UCk_Utils_Time_UE::
    Equal
    (
        const FCk_Time& InA,
        const FCk_Time& InB
    )
    -> bool
{
    return InA == InB;
}

auto
    UCk_Utils_Time_UE::
    NotEqual
    (
        const FCk_Time& InA,
        const FCk_Time& InB
    )
    -> bool
{
    return InA != InB;
}

// --------------------------------------------------------------------------------------------------------------------

