// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include <GameplayTagContainer.h>

#include "CkUI_Extension_Types.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UUserWidget;
class UCk_UI_Extension_Subsystem_UE;

// --------------------------------------------------------------------------------------------------------------------
// Enums
// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_UI_ExtensionPointMatch : uint8
{
    ExactMatch,
    PartialMatch
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_UI_ExtensionPointMatch);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_UI_ExtensionAction : uint8
{
    Added,
    Removed
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_UI_ExtensionAction);

// --------------------------------------------------------------------------------------------------------------------
// Forward Declarations
// --------------------------------------------------------------------------------------------------------------------

struct FCk_UI_Extension;
struct FCk_UI_ExtensionPoint;
struct FCk_UI_ExtensionRequest;

// --------------------------------------------------------------------------------------------------------------------
// Delegates
// --------------------------------------------------------------------------------------------------------------------

DECLARE_DELEGATE_TwoParams(
    FCk_Delegate_ExtensionPoint_OnExtend,
    ECk_UI_ExtensionAction,
    const FCk_UI_ExtensionRequest&);

// --------------------------------------------------------------------------------------------------------------------
// FCk_UI_Extension
// --------------------------------------------------------------------------------------------------------------------

struct CKUI_API FCk_UI_Extension : TSharedFromThis<FCk_UI_Extension>
{
public:
    CK_GENERATED_BODY(FCk_UI_Extension);

private:
    FGameplayTag _ExtensionPointTag;
    TSubclassOf<UUserWidget> _WidgetClass;
    int32 _Priority = INDEX_NONE;

public:
    CK_PROPERTY_GET(_ExtensionPointTag);
    CK_PROPERTY_GET(_WidgetClass);
    CK_PROPERTY(_Priority);

    CK_DEFINE_CONSTRUCTORS(FCk_UI_Extension, _ExtensionPointTag, _WidgetClass);
};

// --------------------------------------------------------------------------------------------------------------------
// FCk_UI_ExtensionPoint
// --------------------------------------------------------------------------------------------------------------------

struct CKUI_API FCk_UI_ExtensionPoint : TSharedFromThis<FCk_UI_ExtensionPoint>
{
public:
    CK_GENERATED_BODY(FCk_UI_ExtensionPoint);

    auto DoesExtensionPassContract(const FCk_UI_Extension* InExtension) const -> bool;

private:
    FGameplayTag _ExtensionPointTag;
    FCk_Delegate_ExtensionPoint_OnExtend _Callback;
    ECk_UI_ExtensionPointMatch _MatchType = ECk_UI_ExtensionPointMatch::ExactMatch;

public:
    CK_PROPERTY_GET(_ExtensionPointTag);
    CK_PROPERTY_GET(_Callback);
    CK_PROPERTY(_MatchType);

    CK_DEFINE_CONSTRUCTORS(FCk_UI_ExtensionPoint, _ExtensionPointTag, _Callback);
};

// --------------------------------------------------------------------------------------------------------------------
// FCk_UI_ExtensionPointHandle
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUI_API FCk_UI_ExtensionPointHandle
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_UI_ExtensionPointHandle);

    FCk_UI_ExtensionPointHandle() = default;

    auto Unregister() -> void;
    auto IsValid() const -> bool;

    auto operator==(const ThisType& Other) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

    friend uint32 GetTypeHash(const FCk_UI_ExtensionPointHandle& InHandle);

private:
    TWeakObjectPtr<UCk_UI_Extension_Subsystem_UE> _ExtensionSource;
    TSharedPtr<FCk_UI_ExtensionPoint> _DataPtr;

    friend UCk_UI_Extension_Subsystem_UE;

    FCk_UI_ExtensionPointHandle(
        UCk_UI_Extension_Subsystem_UE* InSource,
        const TSharedPtr<FCk_UI_ExtensionPoint>& InDataPtr);

public:
    CK_PROPERTY_GET(_ExtensionSource);
    CK_PROPERTY_GET(_DataPtr);
};

template<>
struct TStructOpsTypeTraits<FCk_UI_ExtensionPointHandle> : public TStructOpsTypeTraitsBase2<FCk_UI_ExtensionPointHandle>
{
    enum
    {
        WithCopy = true,
        WithIdenticalViaEquality = true,
    };
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_UI_ExtensionPointHandle, IsValid_Policy_Default,
[](const FCk_UI_ExtensionPointHandle& InHandle)
{
    return InHandle.IsValid();
});

// --------------------------------------------------------------------------------------------------------------------
// FCk_UI_ExtensionHandle
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUI_API FCk_UI_ExtensionHandle
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_UI_ExtensionHandle);

    FCk_UI_ExtensionHandle() = default;

    auto Unregister() -> void;
    auto IsValid() const -> bool;

    auto operator==(const ThisType& Other) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

    friend uint32 GetTypeHash(const FCk_UI_ExtensionHandle& InHandle);

private:
    TWeakObjectPtr<UCk_UI_Extension_Subsystem_UE> _ExtensionSource;
    TSharedPtr<FCk_UI_Extension> _DataPtr;

    friend UCk_UI_Extension_Subsystem_UE;

    FCk_UI_ExtensionHandle(
        UCk_UI_Extension_Subsystem_UE* InSource,
        const TSharedPtr<FCk_UI_Extension>& InDataPtr);

public:
    CK_PROPERTY_GET(_ExtensionSource);
    CK_PROPERTY_GET(_DataPtr);
};

template<>
struct TStructOpsTypeTraits<FCk_UI_ExtensionHandle> : public TStructOpsTypeTraitsBase2<FCk_UI_ExtensionHandle>
{
    enum
    {
        WithCopy = true,
        WithIdenticalViaEquality = true,
    };
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_UI_ExtensionHandle, IsValid_Policy_Default,
[](const FCk_UI_ExtensionHandle& InHandle)
{
    return InHandle.IsValid();
});

// --------------------------------------------------------------------------------------------------------------------
// FCk_UI_ExtensionRequest
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUI_API FCk_UI_ExtensionRequest
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_UI_ExtensionRequest);

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_UI_ExtensionHandle _ExtensionHandle;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FGameplayTag _ExtensionPointTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TSubclassOf<UUserWidget> _WidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _Priority = INDEX_NONE;

public:
    CK_PROPERTY_GET(_ExtensionHandle);
    CK_PROPERTY_GET(_ExtensionPointTag);
    CK_PROPERTY_GET(_WidgetClass);
    CK_PROPERTY_GET(_Priority);

    CK_DEFINE_CONSTRUCTORS(FCk_UI_ExtensionRequest, _ExtensionHandle, _ExtensionPointTag, _WidgetClass, _Priority);
};

// --------------------------------------------------------------------------------------------------------------------
// FCk_UI_HUDElementEntry
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUI_API FCk_UI_HUDElementEntry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_UI_HUDElementEntry);


private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true, Categories = "UI.Slot"))
    FGameplayTag _SlotTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true, AssetBundles = "Client"))
    TSoftClassPtr<UUserWidget> _WidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _Priority = 0;

public:
    CK_PROPERTY_GET(_SlotTag);
    CK_PROPERTY_GET(_WidgetClass);
    CK_PROPERTY_GET(_Priority);

    CK_DEFINE_CONSTRUCTORS(FCk_UI_HUDElementEntry, _SlotTag, _WidgetClass, _Priority);
};

// --------------------------------------------------------------------------------------------------------------------