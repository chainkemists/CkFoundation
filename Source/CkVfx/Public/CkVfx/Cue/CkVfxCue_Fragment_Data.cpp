#include "CkVfxCue_Fragment_Data.h"

#if WITH_ANGELSCRIPT_CK
#include "CkCore/Macros/CkMacros_AngelScript.h"
#endif

// --------------------------------------------------------------------------------------------------------------------

FCk_VfxCue_UserParameter::
    FCk_VfxCue_UserParameter(
        FName InParameterName,
        const float InValue)
    : _ParameterName(InParameterName)
    , _ParameterType(ECk_VfxCue_ParameterType::Float)
    , _FloatValue(InValue)
{
}

FCk_VfxCue_UserParameter::
    FCk_VfxCue_UserParameter(
        FName InParameterName,
        const FVector& InValue)
    : _ParameterName(InParameterName)
    , _ParameterType(ECk_VfxCue_ParameterType::Vector)
    , _VectorValue(InValue)
{
}

FCk_VfxCue_UserParameter::
    FCk_VfxCue_UserParameter(
        FName InParameterName,
        const FLinearColor InValue)
    : _ParameterName (InParameterName)
    , _ParameterType (ECk_VfxCue_ParameterType::Color)
    , _ColorValue (InValue)
{
}

FCk_VfxCue_UserParameter::
    FCk_VfxCue_UserParameter(
        FName InParameterName,
        const bool InValue)
    : _ParameterName (InParameterName)
    , _ParameterType (ECk_VfxCue_ParameterType::Bool)
    , _BoolValue (InValue)
{
}

FCk_VfxCue_UserParameter::
    FCk_VfxCue_UserParameter(
        FName InParameterName,
        const int32 InValue)
    : _ParameterName (InParameterName)
    , _ParameterType (ECk_VfxCue_ParameterType::Int)
    , _IntValue (InValue)
{
}

// --------------------------------------------------------------------------------------------------------------------
