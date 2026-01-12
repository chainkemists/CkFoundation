// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/Context/CkUI_Context_Utils.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkUI/Context/CkUI_Context_Subsystem.h"

#include <Blueprint/UserWidget.h>
#include <GameFramework/PlayerController.h>

// --------------------------------------------------------------------------------------------------------------------
// Subsystem Access
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_Context_UE::
    Get_ContextSubsystem(
        const APlayerController* InPlayerController)
    -> UCk_UI_Context_Subsystem_UE*
{
    if (ck::Is_NOT_Valid(InPlayerController))
    { return nullptr; }

    return Get_ContextSubsystem(InPlayerController->GetLocalPlayer());
}

auto
    UCk_Utils_UI_Context_UE::
    Get_ContextSubsystem(
        const ULocalPlayer* InLocalPlayer)
    -> UCk_UI_Context_Subsystem_UE*
{
    if (ck::Is_NOT_Valid(InLocalPlayer))
    { return nullptr; }

    return InLocalPlayer->GetSubsystem<UCk_UI_Context_Subsystem_UE>();
}

// --------------------------------------------------------------------------------------------------------------------
// Context Injection
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_Context_UE::
    InjectContext(
        UUserWidget* InWidget,
        const FCk_UI_Context& InContext)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    auto* Subsystem = Get_ContextSubsystem(InWidget->GetOwningPlayer());

    if (ck::Is_NOT_Valid(Subsystem))
    { return; }

    Subsystem->InjectContextToWidget(InWidget, InContext);
}

auto
    UCk_Utils_UI_Context_UE::
    InjectEntityContext(
        UUserWidget* InWidget,
        FCk_Handle InEntity)
    -> void
{
    InjectContext(InWidget, FCk_UI_Context::MakeFromEntity(InEntity));
}

auto
    UCk_Utils_UI_Context_UE::
    InjectActorContext(
        UUserWidget* InWidget,
        AActor* InActor)
    -> void
{
    InjectContext(InWidget, FCk_UI_Context::MakeFromActor(InActor));
}

auto
    UCk_Utils_UI_Context_UE::
    ClearContext(
        UUserWidget* InWidget)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    auto* Subsystem = Get_ContextSubsystem(InWidget->GetOwningPlayer());

    if (ck::Is_NOT_Valid(Subsystem))
    { return; }

    Subsystem->ClearContextFromWidget(InWidget);
}

// --------------------------------------------------------------------------------------------------------------------
// Context Query
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_Context_UE::
    Get_ContextForWidget(
        const UUserWidget* InWidget)
    -> FCk_UI_Context
{
    if (ck::Is_NOT_Valid(InWidget))
    { return {}; }

    const auto* Subsystem = Get_ContextSubsystem(InWidget->GetOwningPlayer());

    if (ck::Is_NOT_Valid(Subsystem))
    { return {}; }

    return Subsystem->Get_ContextForWidget(InWidget);
}

auto
    UCk_Utils_UI_Context_UE::
    Has_ValidContextForWidget(
        const UUserWidget* InWidget)
    -> bool
{
    return Get_ContextForWidget(InWidget).IsValid();
}

auto
    UCk_Utils_UI_Context_UE::
    Get_ContextEntity(
        const UUserWidget* InWidget)
    -> FCk_Handle
{
    return Get_ContextForWidget(InWidget).Get_Entity();
}

auto
    UCk_Utils_UI_Context_UE::
    Get_ContextActor(
        const UUserWidget* InWidget)
    -> AActor*
{
    return Get_ContextForWidget(InWidget).Get_Actor().Get();
}

auto
    UCk_Utils_UI_Context_UE::
    Get_ContextPayload(
        const UUserWidget* InWidget)
    -> UObject*
{
    return Get_ContextForWidget(InWidget).Get_Payload().Get();
}

// --------------------------------------------------------------------------------------------------------------------
// Context Creation Helpers
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_Context_UE::
    MakeContext_Entity(
        FCk_Handle InEntity)
    -> FCk_UI_Context
{
    return FCk_UI_Context::MakeFromEntity(InEntity);
}

auto
    UCk_Utils_UI_Context_UE::
    MakeContext_Actor(
        AActor* InActor)
    -> FCk_UI_Context
{
    return FCk_UI_Context::MakeFromActor(InActor);
}

auto
    UCk_Utils_UI_Context_UE::
    MakeContext_Object(
        UObject* InObject)
    -> FCk_UI_Context
{
    return FCk_UI_Context::MakeFromObject(InObject);
}

auto
    UCk_Utils_UI_Context_UE::
    MakeContext_EntityAndActor(
        FCk_Handle InEntity,
        AActor* InActor)
    -> FCk_UI_Context
{
    return FCk_UI_Context::MakeFromEntityAndActor(InEntity, InActor);
}

// --------------------------------------------------------------------------------------------------------------------