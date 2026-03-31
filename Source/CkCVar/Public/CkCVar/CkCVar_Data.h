#pragma once

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

USTRUCT(BlueprintType, meta = (HasNativeMake))
struct CKCVAR_API FCk_CVarRef
{
    GENERATED_BODY()

public:
    friend class ck::layout::FCVarRef_Details;

public:
    FCk_CVarRef() = default;

    explicit FCk_CVarRef(
        FName InName);

public:
    auto IsValid() const -> bool;

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "CVar", meta = (AllowPrivateAccess))
    FName _Name;

public:
    auto Get_Name() const -> FName;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCVAR_API FCk_CVarDefinition
{
    GENERATED_BODY()

public:
    FCk_CVarDefinition() = default;

    FCk_CVarDefinition(
        FName InName,
        ECk_CVarType InType,
        const FString& InDefaultValue,
        const FString& InHelpText);

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
    auto Get_Name() const -> FName;
    auto Get_Type() const -> ECk_CVarType;
    auto Get_DefaultValue() const -> const FString&;
    auto Get_HelpText() const -> const FString&;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCVAR_API FCk_CVarCallbackHandle
{
    GENERATED_BODY()

public:
    FCk_CVarCallbackHandle() = default;

    explicit FCk_CVarCallbackHandle(
        int32 InID);

public:
    auto IsValid() const -> bool;

private:
    UPROPERTY(BlueprintReadOnly,
              Category = "CVar", meta = (AllowPrivateAccess))
    int32 _ID = INDEX_NONE;

public:
    auto Get_ID() const -> int32;
};

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
