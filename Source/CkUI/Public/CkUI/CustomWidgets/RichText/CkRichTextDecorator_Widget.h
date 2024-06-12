#pragma once

#include "CkUI/UserWidget/CkUserWidget.h"

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>
#include <Components/RichTextBlockDecorator.h>
#include <Engine/DataTable.h>
#include <Styling/SlateBrush.h>
#include <Templates/SubclassOf.h>
#include <CommonRichTextBlock.h>
#include <InstancedStruct.h>

#include "CkRichTextDecorator_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// This allows the RichText to spawn Inlined UserWidgets
// Inspired from Runtime/UMG/Private/Components/RichTextBlockImageDecorator.cpp

// --------------------------------------------------------------------------------------------------------------------

UINTERFACE(MinimalAPI)
class UCk_RichTextBlockDecocator_Interface : public UInterface { GENERATED_BODY() };
class CKUI_API ICk_RichTextBlockDecocator_Interface
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ICk_RichTextBlockDecocator_Interface);

public:
    UFUNCTION(BlueprintImplementableEvent)
    void
    OnDecoratorCustomParamsInjected(
        const FInstancedStruct& InCustomParams);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKUI_API UCk_RichTextBlock_UE : public UCommonRichTextBlock
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_RichTextBlock_UE);

public:
    UFUNCTION(BlueprintCallable)
    void InjectCustomParamsToAllDecorators(
        const FInstancedStruct& InCustomParams) const;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(Blueprintable, BlueprintType)
struct CKUI_API FCk_RichTextDecorator_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RichTextDecorator_Params);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TMap<FString, FString> _TagMetaData;

public:
    CK_PROPERTY(_TagMetaData);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class CKUI_API UCk_RichTextDecorator_UserWidget_UE : public UCk_UserWidget_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_RichTextDecorator_UserWidget_UE);

public:
    auto InitializeFromRichTextDecorator(
        const FCk_RichTextDecorator_Params& InParams) -> void;

protected:
    UFUNCTION(BlueprintImplementableEvent)
    void
    OnInitializeFromRichTextDecorator(
        const FCk_RichTextDecorator_Params& InParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_RichTextDecorator_Params _Params;
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

UCLASS(Abstract, Blueprintable, DisplayName = "RichTextBlockWidgetDecorator")
class CKUI_API UCk_RichTextBlockWidgetDecorator_UE : public URichTextBlockDecorator
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_RichTextBlockWidgetDecorator_UE);

public:
    auto
    CreateDecorator(
        URichTextBlock* InOwner) -> TSharedPtr<ITextDecorator> override;

    auto
    FindUserWidget_DataRow(
        FName InTagOrId) const-> FCk_RichTextDecorator_UserWidget_DataRow*;

    auto
    InsertNewCreatedWidget(
        UCk_RichTextDecorator_UserWidget_UE* InWidgetToInsert) -> void;

private:
    UPROPERTY(EditAnywhere, meta=(RequiredAssetDataTags = "RowStructure=/Script/CkUI.Ck_RichTextDecorator_UserWidget_DataRow"))
    TWeakObjectPtr<class UDataTable> _UserWidgetsTable;

    UPROPERTY(Transient)
    TArray<UCk_RichTextDecorator_UserWidget_UE*> _CreatedWidgets;
};

// --------------------------------------------------------------------------------------------------------------------

