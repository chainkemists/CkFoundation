#pragma once

#include <CkCore/Macros/CkMacros.h>
#include <CkCore/Validation/CkIsValid.h>

#include "CkCVar_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    class FCVarRef_Details;
}

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_CVarType : uint8
{
    Int32,
    Float,
    Bool,
    String,
    Command     // Parameterless console command (no value, just executes)
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_CVar_InitialCallbackPolicy : uint8
{
    DoNotFire,
    FireImmediately
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake = "/Script/CkCVar.Ck_Utils_CVar_UE.Make_CVarRef"))
struct CKCVAR_API FCk_CVarRef
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_CVarRef);

    friend class ck::layout::FCVarRef_Details;

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "CVar", meta = (AllowPrivateAccess))
    FName _Name;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "CVar", meta = (AllowPrivateAccess))
    ECk_CVarType _Type = ECk_CVarType::Float;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_Type);

public:
    auto IsValid() const -> bool;

public:
    CK_DEFINE_CONSTRUCTORS(FCk_CVarRef, _Name, _Type);
};

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_CVarRef, IsValid_Policy_Default,
    [=](const FCk_CVarRef& InRef)
    {
        return InRef.IsValid();
    });

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCVAR_API FCk_CVarDefinition
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_CVarDefinition);

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "CVar", meta = (AllowPrivateAccess))
    FName _Name;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "CVar", meta = (AllowPrivateAccess))
    ECk_CVarType _Type = ECk_CVarType::Float;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "CVar", meta = (AllowPrivateAccess))
    FString _DefaultValue;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "CVar", meta = (AllowPrivateAccess))
    FString _HelpText;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_Type);
    CK_PROPERTY_GET(_DefaultValue);
    CK_PROPERTY_GET(_HelpText);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_CVarDefinition, _Name, _Type, _DefaultValue, _HelpText);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCVAR_API FCk_CVarCallbackHandle
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_CVarCallbackHandle);

private:
    UPROPERTY(BlueprintReadOnly,
              Category = "CVar", meta = (AllowPrivateAccess))
    int32 _ID = INDEX_NONE;

public:
    CK_PROPERTY_GET(_ID);

public:
    auto IsValid() const -> bool;

public:
    CK_DEFINE_CONSTRUCTORS(FCk_CVarCallbackHandle, _ID);
};

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_CVarCallbackHandle, IsValid_Policy_Default,
    [=](const FCk_CVarCallbackHandle& InHandle)
    {
        return InHandle.IsValid();
    });

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(FCk_Delegate_CVar_OnChanged_Int32, int32, NewValue);
DECLARE_DYNAMIC_DELEGATE_OneParam(FCk_Delegate_CVar_OnChanged_Float, float, NewValue);
DECLARE_DYNAMIC_DELEGATE_OneParam(FCk_Delegate_CVar_OnChanged_Bool, bool, NewValue);
DECLARE_DYNAMIC_DELEGATE_OneParam(FCk_Delegate_CVar_OnChanged_String, const FString&, NewValue);
DECLARE_DYNAMIC_DELEGATE(FCk_Delegate_CVar_OnCommand);

// --------------------------------------------------------------------------------------------------------------------

// Internal holder struct — UPROPERTY on each delegate type forces UHT to generate
// FDelegateProperty with the correct SignatureFunction. Must live in the same
// module as the delegate declarations for UHT to resolve them properly.
USTRUCT()
struct FCk_CVar_DelegateSignatureHolder
{
    GENERATED_BODY()

    UPROPERTY()
    FCk_Delegate_CVar_OnChanged_Int32 _OnChanged_Int32;

    UPROPERTY()
    FCk_Delegate_CVar_OnChanged_Float _OnChanged_Float;

    UPROPERTY()
    FCk_Delegate_CVar_OnChanged_Bool _OnChanged_Bool;

    UPROPERTY()
    FCk_Delegate_CVar_OnChanged_String _OnChanged_String;

    UPROPERTY()
    FCk_Delegate_CVar_OnCommand _OnCommand;

    CKCVAR_API static auto
    GetSignatureFunctionForType(
        ECk_CVarType InType) -> UFunction*;
};

// --------------------------------------------------------------------------------------------------------------------
