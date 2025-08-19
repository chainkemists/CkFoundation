#include "CkCommonButton.h"

#include "CkUI/Styles/CkCommonButtonStyle_Danger.h"
#include "CkUI/Styles/CkCommonButtonStyle_Dropdown.h"
#include "CkUI/Styles/CkCommonButtonStyle_Primary.h"
#include "CkUI/Styles/CkCommonButtonStyle_Secondary.h"
#include "CkUI/Styles/CkCommonButtonStyle_Tab.h"
#include "CkUI/Styles/CkCommonTextStyle_Body.h"
#include "CkUI/Styles/CkCommonTextStyle_Dropdown.h"
#include "CkUI/Styles/CkCommonTextStyle_Secondary.h"

#include <Components/TextBlock.h>
#include <Blueprint/WidgetTree.h>

UCkCommonButton::UCkCommonButton()
{
    ButtonText = FText::FromString(TEXT("Button"));
    TextStyleClass = nullptr;
}

void UCkCommonButton::NativePreConstruct()
{
    Super::NativePreConstruct();

    if (ck::Is_NOT_Valid(TextBlock))
    {
        TextBlock = WidgetTree->ConstructWidget<UCommonTextBlock>();
        if (ck::IsValid(TextBlock))
        {
            WidgetTree->RootWidget = TextBlock;
            TextBlock->SetJustification(ETextJustify::Center);

            // Set minimum desired size for button
            TextBlock->SetMinDesiredWidth(100.0f);
        }
    }
}

void UCkCommonButton::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (NOT ck::IsValid(TextBlock))
    {
        return;
    }

    if (NOT ButtonText.IsEmpty())
    {
        TextBlock->SetText(ButtonText);
    }

    Request_ApplyDefaultTextStyle();
}

void UCkCommonButton::Request_SetText(const FText& InText)
{
    ButtonText = InText;

    if (ck::IsValid(TextBlock))
    {
        TextBlock->SetText(ButtonText);
    }
}

FText UCkCommonButton::Get_ButtonText() const
{
    return ButtonText;
}

void UCkCommonButton::Request_SetTextStyle(TSubclassOf<UCommonTextStyle> InTextStyle)
{
    TextStyleClass = InTextStyle;

    if (ck::IsValid(TextBlock) && ck::IsValid(TextStyleClass))
    {
        TextBlock->SetStyle(TextStyleClass);
    }
}

UCommonTextBlock* UCkCommonButton::Get_TextBlock() const
{
    return TextBlock;
}

TSubclassOf<UCommonTextStyle> UCkCommonButton::Get_DefaultTextStyle() const
{
    return nullptr;
}

void UCkCommonButton::Request_ApplyDefaultTextStyle()
{
    if (ck::IsValid(TextStyleClass))
    {
        Request_SetTextStyle(TextStyleClass);
        return;
    }

    if (auto DefaultStyle = Get_DefaultTextStyle(); 
        ck::IsValid(DefaultStyle))
    {
        Request_SetTextStyle(DefaultStyle);
    }
}

// --------------------------------------------------------------------------------------------------------------------

UCkCommonButton_Primary::UCkCommonButton_Primary()
{
    SetStyle(UCkCommonButtonStyle_Primary::StaticClass());
}

TSubclassOf<UCommonTextStyle> UCkCommonButton_Primary::Get_DefaultTextStyle() const
{
    return UCkCommonTextStyle_Body::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------

UCkCommonButton_Secondary::UCkCommonButton_Secondary()
{
    SetStyle(UCkCommonButtonStyle_Secondary::StaticClass());
}

TSubclassOf<UCommonTextStyle> UCkCommonButton_Secondary::Get_DefaultTextStyle() const
{
    return UCkCommonTextStyle_Body::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------

UCkCommonButton_Danger::UCkCommonButton_Danger()
{
    SetStyle(UCkCommonButtonStyle_Danger::StaticClass());
}

TSubclassOf<UCommonTextStyle> UCkCommonButton_Danger::Get_DefaultTextStyle() const
{
    return UCkCommonTextStyle_Body::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------

UCkCommonButton_Tab::UCkCommonButton_Tab()
{
    SetStyle(UCkCommonButtonStyle_Tab::StaticClass());
}

TSubclassOf<UCommonTextStyle> UCkCommonButton_Tab::Get_DefaultTextStyle() const
{
    return UCkCommonTextStyle_Secondary::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------

UCkCommonButton_Dropdown::UCkCommonButton_Dropdown()
{
    SetStyle(UCkCommonButtonStyle_Dropdown::StaticClass());
}

TSubclassOf<UCommonTextStyle> UCkCommonButton_Dropdown::Get_DefaultTextStyle() const
{
    return UCkCommonTextStyle_Dropdown::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------
