#include "CkTabBar_Types.h"

#include <Components/ActorComponent.h>
#include <GameFramework/Actor.h>
#include <Styling/CoreStyle.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_TabBar_Style::
    Get_Default()
    -> const FCk_TabBar_Style&
{
    static const auto DefaultStyle = []
    {
        auto Style = FCk_TabBar_Style{};
        Style.Set_TabButtonStyle(FCoreStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("Button")));
        Style.Set_ActiveTabButtonStyle(FCoreStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("Button")));
        Style.Set_TabTextStyle(FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>(TEXT("NormalText")));
        Style.Set_ActiveTabTextStyle(FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>(TEXT("NormalText")));
        return Style;
    }();

    return DefaultStyle;
}

// --------------------------------------------------------------------------------------------------------------------
