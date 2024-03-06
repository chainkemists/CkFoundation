#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkUI/UserWidget/CkUserWidget.h"

#include <Components/WidgetComponent.h>

#include "CkWidgetComponent.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_WidgetComponent_WidgetSpacePolicy : uint8
{
    Default,
    AlwaysFaceCamera,
    SwitchToScreenSpaceAtRuntime
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_WidgetComponent_WidgetSpacePolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUI_API FCk_WidgetComponent_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_WidgetComponent_Params);

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_WidgetComponent_WidgetSpacePolicy _WidgetSpacePolicy = ECk_WidgetComponent_WidgetSpacePolicy::Default;

public:
    CK_PROPERTY_GET(_WidgetSpacePolicy);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_WidgetComponent_Params, _WidgetSpacePolicy);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract,
       HideCategories("Replication", "ComponentTick", "Activation", "Tags", "ComponentReplication", "Mobile", "RayTracing",
                      "Collision", "AssetUserData", "Cooking", "Sockets", "Variable", "Navigation", "HLOD", "Physics")  )
class CKUI_API UCk_WidgetComponent_UE : public UWidgetComponent
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_WidgetComponent_UE);

public:
    using ParamsType = FCk_WidgetComponent_Params;

public:
    UCk_WidgetComponent_UE();

protected:
    auto TickComponent(float InDeltaTime, enum ELevelTick InTickType, FActorComponentTickFunction* InThisTickFunction) -> void override;
    auto InitializeComponent() -> void override;
    auto InitWidget() -> void override;

protected:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|WidgetComponent")
    void Request_SetWidgetAndBind(
        UCk_UserWidget_UE* InNewWidget);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|WidgetComponent")
    void Request_SetWidgetSpacePolicy(
        ECk_WidgetComponent_WidgetSpacePolicy InNewPolicy);


    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|WidgetComponent")
    void OnPostInitWidget();

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "UCk_WidgetComponent_UE",
              meta = (AllowPrivateAccess = true))
    FCk_WidgetComponent_Params _Params;

private:
    UPROPERTY(Transient)
    ECk_WidgetComponent_WidgetSpacePolicy _CurrentWidgetSpacePolicy = ECk_WidgetComponent_WidgetSpacePolicy::Default;

public:
    CK_PROPERTY_GET(_Params);
    CK_PROPERTY_GET(_CurrentWidgetSpacePolicy);
};

// --------------------------------------------------------------------------------------------------------------------
