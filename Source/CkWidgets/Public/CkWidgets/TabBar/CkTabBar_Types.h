#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>
#include <Layout/Margin.h>
#include <Styling/SlateBrush.h>
#include <Styling/SlateTypes.h>

#include "CkTabBar_Types.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKWIDGETS_API FCk_TabBar_TabConfig
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_TabBar_TabConfig);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TabBar",
              meta = (AllowPrivateAccess = true))
    FText _TabName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TabBar",
              meta = (AllowPrivateAccess = true))
    FSlateBrush _TabIcon;

public:
    CK_PROPERTY(_TabName);
    CK_PROPERTY(_TabIcon);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKWIDGETS_API FCk_TabBar_Style
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_TabBar_Style);

public:
    static auto
    Get_Default() -> const FCk_TabBar_Style&;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TabBar",
              meta = (AllowPrivateAccess = true))
    FButtonStyle _TabButtonStyle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TabBar",
              meta = (AllowPrivateAccess = true))
    FButtonStyle _ActiveTabButtonStyle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TabBar",
              meta = (AllowPrivateAccess = true))
    FTextBlockStyle _TabTextStyle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TabBar",
              meta = (AllowPrivateAccess = true))
    FTextBlockStyle _ActiveTabTextStyle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TabBar",
              meta = (AllowPrivateAccess = true))
    FVector2D _IconSize = FVector2D(16.0, 16.0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TabBar",
              meta = (AllowPrivateAccess = true, ClampMin = "0"))
    float _TabSpacing = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TabBar",
              meta = (AllowPrivateAccess = true))
    FMargin _TabPadding = FMargin(8.0f, 4.0f, 8.0f, 4.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TabBar",
              meta = (AllowPrivateAccess = true))
    FSlateBrush _TabBarBackground;

public:
    CK_PROPERTY(_TabButtonStyle);
    CK_PROPERTY(_ActiveTabButtonStyle);
    CK_PROPERTY(_TabTextStyle);
    CK_PROPERTY(_ActiveTabTextStyle);
    CK_PROPERTY(_IconSize);
    CK_PROPERTY(_TabSpacing);
    CK_PROPERTY(_TabPadding);
    CK_PROPERTY(_TabBarBackground);
};

// --------------------------------------------------------------------------------------------------------------------
