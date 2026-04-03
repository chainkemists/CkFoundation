#pragma once

#include "CkCVar/CkCVar_Data.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkCVar_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKCVAR_API UCk_Utils_CVar_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // =================================================================================================================
    // Registration (K2Node expansion targets)
    // =================================================================================================================

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CVar|INTERNAL",
              meta = (BlueprintInternalUseOnly = true))
    static FCk_CVarCallbackHandle
    INTERNAL_Register_Int32(
        FName InName,
        int32 InDefaultValue,
        const FString& InHelp,
        const FCk_Delegate_CVar_OnChanged_Int32& InCallback,
        ECk_CVar_InitialCallbackPolicy InPolicy);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CVar|INTERNAL",
              meta = (BlueprintInternalUseOnly = true))
    static FCk_CVarCallbackHandle
    INTERNAL_Register_Float(
        FName InName,
        float InDefaultValue,
        const FString& InHelp,
        const FCk_Delegate_CVar_OnChanged_Float& InCallback,
        ECk_CVar_InitialCallbackPolicy InPolicy);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CVar|INTERNAL",
              meta = (BlueprintInternalUseOnly = true))
    static FCk_CVarCallbackHandle
    INTERNAL_Register_Bool(
        FName InName,
        bool InDefaultValue,
        const FString& InHelp,
        const FCk_Delegate_CVar_OnChanged_Bool& InCallback,
        ECk_CVar_InitialCallbackPolicy InPolicy);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CVar|INTERNAL",
              meta = (BlueprintInternalUseOnly = true))
    static FCk_CVarCallbackHandle
    INTERNAL_Register_String(
        FName InName,
        const FString& InDefaultValue,
        const FString& InHelp,
        const FCk_Delegate_CVar_OnChanged_String& InCallback,
        ECk_CVar_InitialCallbackPolicy InPolicy);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CVar|INTERNAL",
              meta = (BlueprintInternalUseOnly = true))
    static FCk_CVarCallbackHandle
    INTERNAL_Register_Command(
        FName InName,
        const FString& InHelp,
        const FCk_Delegate_CVar_OnCommand& InCallback);

    // =================================================================================================================
    // Binding (K2Node expansion targets)
    // =================================================================================================================

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CVar|INTERNAL",
              meta = (BlueprintInternalUseOnly = true))
    static FCk_CVarCallbackHandle
    INTERNAL_Bind_Int32(
        FCk_CVarRef InRef,
        const FCk_Delegate_CVar_OnChanged_Int32& InCallback,
        ECk_CVar_InitialCallbackPolicy InPolicy);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CVar|INTERNAL",
              meta = (BlueprintInternalUseOnly = true))
    static FCk_CVarCallbackHandle
    INTERNAL_Bind_Float(
        FCk_CVarRef InRef,
        const FCk_Delegate_CVar_OnChanged_Float& InCallback,
        ECk_CVar_InitialCallbackPolicy InPolicy);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CVar|INTERNAL",
              meta = (BlueprintInternalUseOnly = true))
    static FCk_CVarCallbackHandle
    INTERNAL_Bind_Bool(
        FCk_CVarRef InRef,
        const FCk_Delegate_CVar_OnChanged_Bool& InCallback,
        ECk_CVar_InitialCallbackPolicy InPolicy);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CVar|INTERNAL",
              meta = (BlueprintInternalUseOnly = true))
    static FCk_CVarCallbackHandle
    INTERNAL_Bind_String(
        FCk_CVarRef InRef,
        const FCk_Delegate_CVar_OnChanged_String& InCallback,
        ECk_CVar_InitialCallbackPolicy InPolicy);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CVar|INTERNAL",
              meta = (BlueprintInternalUseOnly = true))
    static FCk_CVarCallbackHandle
    INTERNAL_Bind_Command(
        FCk_CVarRef InRef,
        const FCk_Delegate_CVar_OnCommand& InCallback);

    // =================================================================================================================
    // Unbinding
    // =================================================================================================================

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CVar|INTERNAL",
              meta = (BlueprintInternalUseOnly = true))
    static void
    INTERNAL_Unbind(
        FCk_CVarCallbackHandle InHandle);

    // =================================================================================================================
    // Get (K2Node expansion targets)
    // =================================================================================================================

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CVar|INTERNAL",
              meta = (BlueprintInternalUseOnly = true))
    static int32
    INTERNAL_Get_Int32(
        FCk_CVarRef InRef);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CVar|INTERNAL",
              meta = (BlueprintInternalUseOnly = true))
    static float
    INTERNAL_Get_Float(
        FCk_CVarRef InRef);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CVar|INTERNAL",
              meta = (BlueprintInternalUseOnly = true))
    static bool
    INTERNAL_Get_Bool(
        FCk_CVarRef InRef);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CVar|INTERNAL",
              meta = (BlueprintInternalUseOnly = true))
    static FString
    INTERNAL_Get_String(
        FCk_CVarRef InRef);

    // =================================================================================================================
    // Set (K2Node expansion targets)
    // =================================================================================================================

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CVar|INTERNAL",
              meta = (BlueprintInternalUseOnly = true))
    static void
    INTERNAL_Set_Int32(
        FCk_CVarRef InRef,
        int32 InValue);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CVar|INTERNAL",
              meta = (BlueprintInternalUseOnly = true))
    static void
    INTERNAL_Set_Float(
        FCk_CVarRef InRef,
        float InValue);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CVar|INTERNAL",
              meta = (BlueprintInternalUseOnly = true))
    static void
    INTERNAL_Set_Bool(
        FCk_CVarRef InRef,
        bool InValue);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CVar|INTERNAL",
              meta = (BlueprintInternalUseOnly = true))
    static void
    INTERNAL_Set_String(
        FCk_CVarRef InRef,
        const FString& InValue);

    // =================================================================================================================
    // Construction
    // =================================================================================================================

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CVar",
              DisplayName = "[Ck][CVar] Make CVar Ref",
              meta = (NativeMakeFunc, Keywords = "construct build"))
    static FCk_CVarRef
    Make_CVarRef(
        FName InName,
        ECk_CVarType InType);

    // =================================================================================================================
    // Public Query
    // =================================================================================================================

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CVar",
              DisplayName = "[Ck][CVar] Is Registered")
    static bool
    IsRegistered(
        FCk_CVarRef InRef);
};

// --------------------------------------------------------------------------------------------------------------------
