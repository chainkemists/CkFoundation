#pragma once

#include "CkUI/UserWidget/CkUserWidget.h"

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>
#include <Components/RichTextBlockDecorator.h>
#include <Engine/DataTable.h>
#include <Templates/SubclassOf.h>
#include <CommonRichTextBlock.h>
#include <InstancedStruct.h>

#include "CkRichTextDecorator_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// This allows the RichText to spawn Inlined UserWidgets
// Inspired from Runtime/UMG/Private/Components/RichTextBlockImageDecorator.cpp

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(Blueprintable, BlueprintType)
struct CKUI_API FCk_RichTextDecorator_Metadata
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RichTextDecorator_Metadata);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TMap<FString, FString> _TagMetaData;

public:
    CK_PROPERTY(_TagMetaData);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(Blueprintable, BlueprintType)
struct CKUI_API FCk_RichTextDecorator_CustomParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RichTextDecorator_CustomParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FInstancedStruct _Params;

public:
    CK_PROPERTY(_Params);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class CKUI_API UCk_RichTextDecorator_UserWidget_UE : public UCk_UserWidget_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_RichTextDecorator_UserWidget_UE);

public:
    auto
    InjectDecoratorMetadata(
        const FCk_RichTextDecorator_Metadata& InMetadata) -> void;

    auto
    InjectDecoratorCustomParams(
        const FCk_RichTextDecorator_CustomParams& InCustomParams) -> void;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Ck|UI")
    void
    OnInjectDecoratorMetadata(
        const FCk_RichTextDecorator_Metadata& InMetadata);

    UFUNCTION(BlueprintImplementableEvent, Category = "Ck|UI")
    void
    OnInjectDecoratorCustomParams(
        const FCk_RichTextDecorator_CustomParams& InCustomParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_RichTextDecorator_Metadata _Metadata;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_RichTextDecorator_CustomParams _CustomParams;
};

// --------------------------------------------------------------------------------------------------------------------


USTRUCT(Blueprintable, BlueprintType)
struct CKUI_API FCk_RichTextDecorator_UserWidget_DataRow : public FTableRowBase
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RichTextDecorator_UserWidget_DataRow);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_RichTextDecorator_UserWidget_UE> _RichTextDecoratorWidgetClass;

public:
    CK_PROPERTY_GET(_RichTextDecoratorWidgetClass);
};

// --------------------------------------------------------------------------------------------------------------------

UINTERFACE(MinimalAPI)
class UCk_RichTextBlockDecocator_Interface : public UInterface { GENERATED_BODY() };
class CKUI_API ICk_RichTextBlockDecocator_Interface
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ICk_RichTextBlockDecocator_Interface);

public:
    virtual auto
    InjectDecoratorCustomParams(
        const FCk_RichTextDecorator_CustomParams& InCustomParams) -> void
    PURE_VIRTUAL(ICk_RichTextBlockDecocator_Interface::InjectDecoratorCustomParams, return;);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FRichTextWidgetDecorator; }

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, DisplayName = "RichTextBlockWidgetDecorator")
class CKUI_API UCk_RichTextBlockWidgetDecorator_UE : public URichTextBlockDecorator, public ICk_RichTextBlockDecocator_Interface
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_RichTextBlockWidgetDecorator_UE);

public:
    friend class ck::FRichTextWidgetDecorator;

public:
    auto
    CreateDecorator(
        URichTextBlock* InOwner) -> TSharedPtr<ITextDecorator> override;

    auto
    FindUserWidget_DataRow(
        FName InTagOrId) const-> FCk_RichTextDecorator_UserWidget_DataRow*;

protected:
    auto
    InjectDecoratorCustomParams(
        const FCk_RichTextDecorator_CustomParams& InCustomParams) -> void override;

private:
    UPROPERTY(EditAnywhere, meta=(RequiredAssetDataTags = "RowStructure=/Script/CkUI.Ck_RichTextDecorator_UserWidget_DataRow"))
    TObjectPtr<class UDataTable> _UserWidgetsTable;

    UPROPERTY(Transient)
    TArray<UCk_RichTextDecorator_UserWidget_UE*> _CreatedWidgets;

private:
    TOptional<FCk_RichTextDecorator_CustomParams> _LastInjectedCustomParams;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKUI_API UCk_RichTextBlock_UE : public UCommonRichTextBlock
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_RichTextBlock_UE);

public:
    UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Ck|UI")
    void InjectCustomParamsToAllDecorators(
        const FCk_RichTextDecorator_CustomParams& InCustomParams) const;

protected:
#if WITH_EDITOR
    auto GetPaletteCategory() -> const FText override;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
