// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkUI/Types/CkUI_Types.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkUI_Context_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class APlayerController;
class UUserWidget;
class UCk_UI_Context_Subsystem_UE;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Utility functions for UI context injection and retrieval.
 */
UCLASS(NotBlueprintable)
class CKUI_API UCk_Utils_UI_Context_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_UI_Context_UE);

    // ----------------------------------------------------------------------------------------------------------------
    // Subsystem Access
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Get Context Subsystem",
        meta = (DefaultToSelf = "InPlayerController"))
    static UCk_UI_Context_Subsystem_UE* Get_ContextSubsystem(const APlayerController* InPlayerController);

    static UCk_UI_Context_Subsystem_UE* Get_ContextSubsystem(const ULocalPlayer* InLocalPlayer);

    // ----------------------------------------------------------------------------------------------------------------
    // Context Injection
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Inject Context",
        meta = (DefaultToSelf = "InWidget"))
    static void InjectContext(UUserWidget* InWidget, const FCk_UI_Context& InContext);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Inject Context (From Entity)",
        meta = (DefaultToSelf = "InWidget"))
    static void InjectEntityContext(UUserWidget* InWidget, FCk_Handle InEntity);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Inject Context (From Actor)",
        meta = (DefaultToSelf = "InWidget"))
    static void InjectActorContext(UUserWidget* InWidget, AActor* InActor);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Inject Context (From Object)",
        meta = (DefaultToSelf = "InWidget"))
    static void InjectObjectContext(UUserWidget* InWidget, UObject* InObj);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Clear Context",
        meta = (DefaultToSelf = "InWidget"))
    static void ClearContext(UUserWidget* InWidget);

    // ----------------------------------------------------------------------------------------------------------------
    // Context Query
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Get Context For Widget",
        meta = (DefaultToSelf = "InWidget"))
    static FCk_UI_Context Get_ContextForWidget(const UUserWidget* InWidget);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Has Valid Context",
        meta = (DefaultToSelf = "InWidget"))
    static bool Has_ValidContextForWidget(const UUserWidget* InWidget);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Get Context Entity",
        meta = (DefaultToSelf = "InWidget"))
    static FCk_Handle Get_ContextEntity(const UUserWidget* InWidget);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Get Context Actor",
        meta = (DefaultToSelf = "InWidget"))
    static AActor* Get_ContextActor(const UUserWidget* InWidget);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Get Context Payload",
        meta = (DefaultToSelf = "InWidget"))
    static UObject* Get_ContextPayload(const UUserWidget* InWidget);

    // ----------------------------------------------------------------------------------------------------------------
    // Context Creation Helpers
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Make Context (Entity)")
    static FCk_UI_Context MakeContext_Entity(FCk_Handle InEntity);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Make Context (Actor)")
    static FCk_UI_Context MakeContext_Actor(AActor* InActor);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Make Context (Object)")
    static FCk_UI_Context MakeContext_Object(UObject* InObject);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context", DisplayName = "[Ck][UI] Get Is Valid (UI Context)")
    static bool IsValid_Context(const FCk_UI_Context& InContext);
};

// --------------------------------------------------------------------------------------------------------------------