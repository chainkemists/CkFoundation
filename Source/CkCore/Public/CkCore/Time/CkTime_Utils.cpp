#include "CkTime_Utils.h"

#include "AngelscriptBinds.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

#define ANGELSCRIPT_BIND_OPERATOR_EQUALS(_Type_)                                                                                        \
AS_FORCE_LINK const FAngelscriptBinds::FBind opEquals_##_Type_ (FAngelscriptBinds::EOrder::Early, []                                    \
{                                                                                                                                       \
    const FBindFlags Flags;                                                                                                             \
    auto Bind = FAngelscriptBinds::ValueClass<_Type_>(#_Type_, Flags);                                                                  \
    Bind.Method("bool opEquals(const "#_Type_"& Other) const",                                                                          \
    	METHODPR_TRIVIAL(bool, _Type_, operator==, (const _Type_&) const));                                                             \
});

#define ANGELSCRIPT_BIND_OPERATOR_COMPARISON(_Type_)                                                                                    \
AS_FORCE_LINK const FAngelscriptBinds::FBind opCmp_##_Type_ (FAngelscriptBinds::EOrder::Early, []                                       \
{                                                                                                                                       \
    const FBindFlags Flags;                                                                                                             \
    auto Bind = FAngelscriptBinds::ValueClass<_Type_>(#_Type_, Flags);                                                                  \
    Bind.Method("int opCmp(const "#_Type_"& Other) const",                                                                              \
    	METHODPR_TRIVIAL(int, _Type_, OpCmp, (const _Type_&) const));                                                                   \
});

#define ANGELSCRIPT_BIND_OPERATOR_ASSIGNMENT(_Type_, _Operator_, _AS_Op_Name_)                                                          \
AS_FORCE_LINK const FAngelscriptBinds::FBind _AS_Op_Name_##_##_Type_ (FAngelscriptBinds::EOrder::Early, []                              \
{                                                                                                                                       \
    const FBindFlags Flags;                                                                                                             \
    auto Bind = FAngelscriptBinds::ValueClass<_Type_>(#_Type_, Flags);                                                                  \
    Bind.Method(#_Type_" "#_AS_Op_Name_"(const "#_Type_"& Other) const",                                                                \
    	METHODPR_TRIVIAL(_Type_, _Type_, operator _Operator_, (const _Type_&) const));                                                  \
});

#define ANGELSCRIPT_BIND_OPERATOR_UNARY(_Type_, _Ret_Type_, _Ret_Type__AsParam_, _Operator_, _AS_Op_Name_)                       \
AS_FORCE_LINK const FAngelscriptBinds::FBind _AS_Op_Name_##_##_Type_##_Ret_Type_ (FAngelscriptBinds::EOrder::Early, []           \
{                                                                                                                                \
    const FBindFlags Flags;                                                                                                      \
    auto Bind = FAngelscriptBinds::ValueClass<_Type_>(#_Type_, Flags);                                                           \
    Bind.Method(#_Ret_Type_" "#_AS_Op_Name_"() const",                                                                           \
    	METHODPR_TRIVIAL(_Ret_Type__AsParam_, _Type_, operator _Operator_, () const));                                           \
});

// The "_Ret_Type__AsParam_" is needed because float in the bind needs to be "float32" for some weird reason, but it returns a float in c++
#define ANGELSCRIPT_BIND_OPERATOR_BINARY(_Type_, _Other_Type_, _Other_Type_AsParam_, _Ret_Type_, _Ret_Type__AsParam_, _Operator_, _AS_Op_Name_)      \
AS_FORCE_LINK const FAngelscriptBinds::FBind _AS_Op_Name_##_##_Type_##_Other_Type_##_Ret_Type_ (FAngelscriptBinds::EOrder::Early, []                 \
{                                                                                                                                                    \
    const FBindFlags Flags;                                                                                                                          \
    auto Bind = FAngelscriptBinds::ValueClass<_Type_>(#_Type_, Flags);                                                                               \
    Bind.Method(#_Ret_Type_" "#_AS_Op_Name_"("#_Other_Type_AsParam_" Other) const",                                                                  \
    	METHODPR_TRIVIAL(_Ret_Type__AsParam_, _Type_, operator _Operator_, (_Other_Type_AsParam_) const));                                           \
});

ANGELSCRIPT_BIND_OPERATOR_EQUALS(FCk_Time)
ANGELSCRIPT_BIND_OPERATOR_COMPARISON(FCk_Time)

ANGELSCRIPT_BIND_OPERATOR_ASSIGNMENT(FCk_Time, +=, opAddAssign)
ANGELSCRIPT_BIND_OPERATOR_ASSIGNMENT(FCk_Time, -=, opSubAssign)

ANGELSCRIPT_BIND_OPERATOR_UNARY(FCk_Time, FCk_Time, FCk_Time, -, opNeg)

ANGELSCRIPT_BIND_OPERATOR_BINARY(FCk_Time, FCk_Time, const FCk_Time&, FCk_Time, FCk_Time, +, opAdd)
ANGELSCRIPT_BIND_OPERATOR_BINARY(FCk_Time, FCk_Time, const FCk_Time&, FCk_Time, FCk_Time, -, opSub)
ANGELSCRIPT_BIND_OPERATOR_BINARY(FCk_Time, float, float, FCk_Time, FCk_Time, *, opMul)
ANGELSCRIPT_BIND_OPERATOR_BINARY(FCk_Time, int32, int32, FCk_Time, FCk_Time, *, opMul)
ANGELSCRIPT_BIND_OPERATOR_BINARY(FCk_Time, FCk_Time, const FCk_Time&, float32, float, /, opDiv)
ANGELSCRIPT_BIND_OPERATOR_BINARY(FCk_Time, float, float, FCk_Time, FCk_Time, /, opDiv)
ANGELSCRIPT_BIND_OPERATOR_BINARY(FCk_Time, int32, int32, FCk_Time, FCk_Time, /, opDiv)

// --------------------------------------------------------------------------------------------------------------------

FCk_Utils_Time_GetWorldTime_Params::
    FCk_Utils_Time_GetWorldTime_Params(
        UObject* InObject)
    : _Object(InObject)
{
}

FCk_Utils_Time_GetWorldTime_Params::
    FCk_Utils_Time_GetWorldTime_Params(
        UWorld* InWorld)
    : _World(InWorld)
{
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Time_UE::
    Conv_TimeToText(
        const FCk_Time& InTime)
    -> FText
{
    return FText::FromString(Conv_TimeToString(InTime));
}

auto
    UCk_Utils_Time_UE::
    Conv_TimeToString(
        const FCk_Time& InTime)
    -> FString
{
    return ck::Format_UE(TEXT("{}"), InTime);
}

auto
    UCk_Utils_Time_UE::
    Make_FromSeconds(
        float InSeconds)
    -> FCk_Time
{
    return FCk_Time{InSeconds};
}

auto
    UCk_Utils_Time_UE::
    Make_FromMilliseconds(
        float InMilliSeconds)
    -> FCk_Time
{
    return Make_FromSeconds(InMilliSeconds / 1000.0f);
}

auto
    UCk_Utils_Time_UE::
    Make_WorldTime(
        const UObject* InWorldContextObject)
    -> FCk_WorldTime
{
    return FCk_WorldTime{InWorldContextObject};
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
    Get_FrameCounter()
    -> int64
{
    return GFrameCounter;
}

auto
    UCk_Utils_Time_UE::
    Get_WorldTime(
        const FCk_Utils_Time_GetWorldTime_Params& InParams)
    -> FCk_Utils_Time_GetWorldTime_Result
{
    const auto& IsParamsWorldValid  = ck::IsValid(InParams.Get_World());
    const auto& IsParamsObjectValid = ck::IsValid(InParams.Get_Object());

    CK_ENSURE_IF_NOT(IsParamsWorldValid || IsParamsObjectValid, TEXT("The UObject or the UWorld must be valid to get the correct time"))
    { return {}; }

    const auto& World = [&]() -> const UWorld*
    {
        if (IsParamsWorldValid)
        { return InParams.Get_World().Get(); }

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
                    FCk_Time{static_cast<float>(World->GetTimeSeconds())},
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
                    FCk_Time{static_cast<float>(World->GetUnpausedTimeSeconds())},
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
                    FCk_Time{static_cast<float>(World->GetRealTimeSeconds())},
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
                    FCk_Time{static_cast<float>(World->GetAudioTimeSeconds())},
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
                    FCk_Time{World->GetDeltaSeconds()},
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
    Get_Milliseconds(
        const FCk_Time& InTime)
    -> float
{
    return InTime.Get_Milliseconds();
}

auto
    UCk_Utils_Time_UE::
    Get_IsZero(
        const FCk_Time& InTime)
    -> bool
{
    return InTime == FCk_Time::ZeroSecond();
}

auto
    UCk_Utils_Time_UE::
    Add(
        const FCk_Time& InA,
        const FCk_Time& InB)
    -> FCk_Time
{
    return InA + InB;
}

auto
    UCk_Utils_Time_UE::
    Subtract(
        const FCk_Time& InA,
        const FCk_Time& InB)
    -> FCk_Time
{
    return InA - InB;
}

auto
    UCk_Utils_Time_UE::
    Divide(
        const FCk_Time& InA,
        const FCk_Time& InB)
    -> float
{
    return InA / InB;
}

auto
    UCk_Utils_Time_UE::
    Divide_TimeFloat(
        const FCk_Time& InA,
        float InB)
    -> FCk_Time
{
    return InA / InB;
}

auto
    UCk_Utils_Time_UE::
    Divide_TimeInt(
        const FCk_Time& InA,
        int32 InB)
    -> FCk_Time
{
    return InA / InB;
}

auto
    UCk_Utils_Time_UE::
    Multiply_TimeFloat(
        const FCk_Time& InA,
        float InB)
    -> FCk_Time
{
    return InA * InB;
}

auto
    UCk_Utils_Time_UE::
    Multiply_TimeInt(
        const FCk_Time& InA,
        int32 InB)
    -> FCk_Time
{
    return InA * InB;
}

auto
    UCk_Utils_Time_UE::
    IsEqual(
        const FCk_Time& InA,
        const FCk_Time& InB)
    -> bool
{
    return InA == InB;
}

auto
    UCk_Utils_Time_UE::
    IsNotEqual(
        const FCk_Time& InA,
        const FCk_Time& InB)
    -> bool
{
    return InA != InB;
}

auto
	UCk_Utils_Time_UE::
	Greater(
		const FCk_Time& InA,
		const FCk_Time& InB)
	-> bool
{
	return InA > InB;
}

auto
	UCk_Utils_Time_UE::
	GreaterEqual(
		const FCk_Time& InA,
		const FCk_Time& InB)
	-> bool
{
	return InA >= InB;
}

auto
	UCk_Utils_Time_UE::
	Less(
		const FCk_Time& InA,
		const FCk_Time& InB)
	-> bool
{
	return InA < InB;
}

auto
	UCk_Utils_Time_UE::
	LessEqual(
		const FCk_Time& InA,
		const FCk_Time& InB)
	-> bool
{
	return InA <= InB;
}

// --------------------------------------------------------------------------------------------------------------------

