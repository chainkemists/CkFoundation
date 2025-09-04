#include "CkRichTextDecorator_Widget.h"

#include "CkCore/Game/CkGame_Utils.h"
#include "CkCore/Algorithms/CkAlgorithms.h"

#include <Components/RichTextBlock.h>

#include "Kismet/GameplayStatics.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    /**
     * Add a widget inline with the text.
     * Usage: Before widget <widget id="MyId"/>, after widget.
     */
    class FRichTextWidgetDecorator : public FRichTextDecorator
    {
    public:
        CK_GENERATED_BODY(FRichTextWidgetDecorator);

    public:
        FRichTextWidgetDecorator(
            URichTextBlock* InOwner,
            UCk_RichTextBlockWidgetDecorator* InDecorator,
            const FString& InFallbackID);

        FRichTextWidgetDecorator(
            URichTextBlock* InOwner,
            UCk_RichTextBlockWidgetDecorator* InDecorator,
            const FCk_RichTextDecorator_CustomParams& InDecoratorCustomParams,
            const FString& InFallbackID);

    public:
        auto
        Supports(
            const FTextRunParseResults& InRunParseResult,
            const FString& InText) const-> bool override;

    private:
        auto
        CreateDecoratorWidget(
            const FTextRunInfo& InRunInfo,
            const FTextBlockStyle& InTextStyle) const -> TSharedPtr<SWidget> override;

    private:
        TWeakObjectPtr<UCk_RichTextBlockWidgetDecorator> _Decorator;
        TOptional<FCk_RichTextDecorator_CustomParams> _DecoratorCustomParams;
        FString _FallbackID;
    };
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_RichTextDecorator_UserWidget::
    InjectDecoratorMetadata(
        const FCk_RichTextDecorator_Metadata& InMetadata) -> void
{
    _Metadata = InMetadata;
    OnInjectDecoratorMetadata(InMetadata);
}

auto
    UCk_RichTextDecorator_UserWidget::
    InjectDecoratorCustomParams(
        const FCk_RichTextDecorator_CustomParams& InCustomParams)
    -> void
{
    _CustomParams = InCustomParams;
    OnInjectDecoratorCustomParams(InCustomParams);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_RichTextBlockWidgetDecorator::
    CreateDecorator(
        URichTextBlock* InOwner)
    -> TSharedPtr<ITextDecorator>
{
    if (ck::IsValid(_LastInjectedCustomParams))
    {
        return MakeShareable(new ck::FRichTextWidgetDecorator{InOwner, this, *_LastInjectedCustomParams, _FallbackID});
    }

    return MakeShareable(new ck::FRichTextWidgetDecorator{InOwner, this, _FallbackID});
}

auto
    UCk_RichTextBlockWidgetDecorator::
    FindUserWidget_DataRow(
        FName InTagOrId) const
    -> FCk_RichWidgetRow*
{
    if (ck::Is_NOT_Valid(_UserWidgetsTable))
    { return {}; }

    const auto ContextString = FString{};
    constexpr auto WarnIfRowMissing = true;

    return _UserWidgetsTable->FindRow<FCk_RichWidgetRow>(InTagOrId, ContextString, WarnIfRowMissing);
}

auto
    UCk_RichTextBlockWidgetDecorator::
    InjectDecoratorCustomParams(
        const FCk_RichTextDecorator_CustomParams& InCustomParams)
    -> void
{
    _LastInjectedCustomParams = InCustomParams;

    ck::algo::ForEachIsValid(_CreatedWidgets, [&](UCk_RichTextDecorator_UserWidget* InDecoratorWidget)
    {
        InDecoratorWidget->InjectDecoratorCustomParams(InCustomParams);
    });
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FRichTextWidgetDecorator::
        FRichTextWidgetDecorator(
            URichTextBlock* InOwner,
            UCk_RichTextBlockWidgetDecorator* InDecorator,
            const FString& InFallbackID)
        : FRichTextDecorator(InOwner)
        , _Decorator(InDecorator)
        , _FallbackID(InFallbackID)
    {
    }

    FRichTextWidgetDecorator::
        FRichTextWidgetDecorator(
            URichTextBlock* InOwner,
            UCk_RichTextBlockWidgetDecorator* InDecorator,
            const FCk_RichTextDecorator_CustomParams& InDecoratorCustomParams,
            const FString& InFallbackID)
        : FRichTextDecorator(InOwner)
        , _Decorator(InDecorator)
        , _DecoratorCustomParams(InDecoratorCustomParams)
        , _FallbackID(InFallbackID)
    {
    }

    auto
        FRichTextWidgetDecorator::
        Supports(
            const FTextRunParseResults& InRunParseResult,
            const FString& InText) const
        -> bool
    {
        if (InRunParseResult.Name != TEXT("widget"))
        { return {}; }

        if (ck::Is_NOT_Valid(_Decorator))
        { return {}; }

        FString RowName = _FallbackID;

        if (InRunParseResult.MetaData.Contains(TEXT("id")))
        {
            const auto& IdRange = InRunParseResult.MetaData[TEXT("id")];
            const auto& TagId = InText.Mid(IdRange.BeginIndex, IdRange.EndIndex - IdRange.BeginIndex);
            RowName = TagId;
        }

        const auto& FoundUserWidget = _Decorator->FindUserWidget_DataRow(*RowName);

        return ck::IsValid(FoundUserWidget, ck::IsValid_Policy_NullptrOnly{});
    }

    auto
        FRichTextWidgetDecorator::
        CreateDecoratorWidget(
            const FTextRunInfo& InRunInfo,
            const FTextBlockStyle& InTextStyle) const
        -> TSharedPtr<SWidget>
    {
        // TODO: Cache decorator widget to avoid re-creating widget decorators over and over again
        if (ck::Is_NOT_Valid(_Decorator))
        { return {}; }

        FString RowName = _FallbackID;

        if (InRunInfo.MetaData.Contains(TEXT("id")))
        {
            RowName = InRunInfo.MetaData[TEXT("id")];
        }

        const auto& FoundUserWidget = _Decorator->FindUserWidget_DataRow(*RowName);

        if (ck::Is_NOT_Valid(FoundUserWidget, ck::IsValid_Policy_NullptrOnly{}))
        { return {}; }

        const auto& WidgetClass = FoundUserWidget->Get_RichTextDecoratorWidgetClass();
        if (ck::Is_NOT_Valid(WidgetClass))
        { return {}; }

        const auto& NewUserWidget = CreateWidget<UCk_RichTextDecorator_UserWidget>(_Decorator->GetWorld(), WidgetClass);

        if (ck::Is_NOT_Valid(NewUserWidget))
        { return {}; }

        NewUserWidget->InjectDecoratorMetadata(FCk_RichTextDecorator_Metadata{}
                                                        .Set_TagMetaData(InRunInfo.MetaData));

        if (ck::IsValid(_DecoratorCustomParams))
        {
            NewUserWidget->InjectDecoratorCustomParams(*_DecoratorCustomParams);
        }
        else if (ck::IsValid(_Decorator->_LastInjectedCustomParams))
        {
            NewUserWidget->InjectDecoratorCustomParams(*_Decorator->_LastInjectedCustomParams);
        }

        _Decorator->_CreatedWidgets.Add(NewUserWidget);
        const auto& SlateWidget = NewUserWidget->TakeWidget();

        return SlateWidget;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_RichTextBlock::
    InjectCustomParamsToAllDecorators(
        const FCk_RichTextDecorator_CustomParams& InCustomParams) const
    -> void
{
    ck::algo::ForEachIsValid(InstanceDecorators, [&](TObjectPtr<URichTextBlockDecorator> InDecorator)
    {
        if (const auto& DecoratorInterface = Cast<ICk_RichTextBlockDecocator_Interface>(InDecorator);
            ck::IsValid(DecoratorInterface, ck::IsValid_Policy_NullptrOnly{}))
        {
            DecoratorInterface->InjectDecoratorCustomParams(InCustomParams);
        }
    });
}

#if WITH_EDITOR
auto
    UCk_RichTextBlock::
    GetPaletteCategory()
    -> const FText
{
    return ck::widget_palette_categories::Default;
}
#endif

// --------------------------------------------------------------------------------------------------------------------
