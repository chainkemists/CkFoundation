#include "CkRichTextDecorator_Widget.h"

#include "CkCore/Game/CkGame_Utils.h"
#include "CkCore/Algorithms/CkAlgorithms.h"

#include <Components/RichTextBlock.h>

#include "Kismet/GameplayStatics.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FRichTextWidgetDecorator : public FRichTextDecorator
    {
    public:
        CK_GENERATED_BODY(FRichTextWidgetDecorator);

    public:
        FRichTextWidgetDecorator(
            URichTextBlock* InOwner,
            UCk_RichTextBlockWidgetDecorator_UE* InDecorator);

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
        TWeakObjectPtr<UCk_RichTextBlockWidgetDecorator_UE> _Decorator;
    };
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_RichTextDecorator_UserWidget_UE::
    InjectDecoratorMetadata(
        const FCk_RichTextDecorator_Metadata& InMetadata) -> void
{
    _Metadata = InMetadata;
    OnInjectDecoratorMetadata(InMetadata);
}

auto
    UCk_RichTextDecorator_UserWidget_UE::
    InjectDecoratorCustomParams(
        const FCk_RichTextDecorator_CustomParams& InCustomParams)
    -> void
{
    _CustomParams = InCustomParams;
    OnInjectDecoratorCustomParams(InCustomParams);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_RichTextBlockWidgetDecorator_UE::
    CreateDecorator(
        URichTextBlock* InOwner)
    -> TSharedPtr<ITextDecorator>
{
    return MakeShareable(new ck::FRichTextWidgetDecorator(InOwner, this));
}

auto
    UCk_RichTextBlockWidgetDecorator_UE::
    FindUserWidget_DataRow(
        FName InTagOrId) const
    -> FCk_RichTextDecorator_UserWidget_DataRow*
{
    if (ck::Is_NOT_Valid(_UserWidgetsTable))
    { return {}; }

    const auto ContextString = FString{};
    constexpr auto WarnIfRowMissing = true;

    return _UserWidgetsTable->FindRow<FCk_RichTextDecorator_UserWidget_DataRow>(InTagOrId, ContextString, WarnIfRowMissing);
}

auto
    UCk_RichTextBlockWidgetDecorator_UE::
    InjectDecoratorCustomParams(
        const FCk_RichTextDecorator_CustomParams& InCustomParams)
    -> void
{
    ck::algo::ForEachIsValid(_CreatedWidgets, [&](UCk_RichTextDecorator_UserWidget_UE* InDecoratorWidget)
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
            UCk_RichTextBlockWidgetDecorator_UE* InDecorator)
        : FRichTextDecorator(InOwner)
        , _Decorator(InDecorator)
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

        if (NOT InRunParseResult.MetaData.Contains(TEXT("id")))
        { return {}; }

        const auto& IdRange = InRunParseResult.MetaData[TEXT("id")];
        const auto& TagId = InText.Mid(IdRange.BeginIndex, IdRange.EndIndex - IdRange.BeginIndex);

        const auto& FoundUserWidget = _Decorator->FindUserWidget_DataRow(*TagId);

        return ck::IsValid(FoundUserWidget, ck::IsValid_Policy_NullptrOnly{});
    }

    auto
        FRichTextWidgetDecorator::
        CreateDecoratorWidget(
            const FTextRunInfo& InRunInfo,
            const FTextBlockStyle& InTextStyle) const
        -> TSharedPtr<SWidget>
    {
        if (ck::Is_NOT_Valid(_Decorator))
        { return {}; }

        if (NOT InRunInfo.MetaData.Contains(TEXT("id")))
        { return {}; }

        const auto& StrId = InRunInfo.MetaData[TEXT("id")];
        const auto& FoundUserWidget = _Decorator->FindUserWidget_DataRow(*StrId);

        if (ck::Is_NOT_Valid(FoundUserWidget, ck::IsValid_Policy_NullptrOnly{}))
        { return {}; }

        const auto& WidgetClass = FoundUserWidget->Get_RichTextDecoratorWidgetClass();
        if (ck::Is_NOT_Valid(WidgetClass))
        { return {}; }

        const auto& NewUserWidget = CreateWidget<UCk_RichTextDecorator_UserWidget_UE>(_Decorator->GetWorld(), WidgetClass);

        if (ck::Is_NOT_Valid(NewUserWidget))
        { return {}; }

        NewUserWidget->InjectDecoratorMetadata(FCk_RichTextDecorator_Metadata{}
                                                        .Set_TagMetaData(InRunInfo.MetaData));

        _Decorator->_CreatedWidgets.Add(NewUserWidget);
        const auto& SlateWidget = NewUserWidget->TakeWidget();

        return SlateWidget;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_RichTextBlock_UE::
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
    UCk_RichTextBlock_UE::
    GetPaletteCategory()
    -> const FText
{
    return ck::widget_palette_categories::Default;
}
#endif

// --------------------------------------------------------------------------------------------------------------------
