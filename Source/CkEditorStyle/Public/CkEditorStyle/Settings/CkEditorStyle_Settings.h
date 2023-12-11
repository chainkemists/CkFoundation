#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"
#include "CkEditorStyle/CkEditorStyle_Utils.h"

#include <IDetailCustomization.h>

#include "CkEditorStyle_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

enum class ECk_AssetStyleType : uint8;

namespace ck::details
{
    class FEditorStyle_ProjectSettings_Details : public IDetailCustomization
{
    public:
        /** Makes a new instance of this detail layout class for a specific detail view requesting it */
        static TSharedRef<IDetailCustomization> MakeInstance();

        /** IDetailCustomization interface */
        virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
    };
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Editor Style"))
class CKEDITORSTYLE_API UCk_EditorStyle_ProjectSettings_UE : public UCk_Editor_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EditorStyle_ProjectSettings_UE);

public:
    auto
    AssignCustomIconOrThumbnailToAsset(
        UClass* InAssetClass,
        ECk_AssetStyleType InAssetStyle) -> void;

public:
    auto PostInitProperties() -> void override;

#if WITH_EDITOR
    auto PostEditChangeProperty(
        struct FPropertyChangedEvent& PropertyChangedEvent) -> void override;
#endif

private:
    auto DoUpdateCustomIconsAndThumbnail() const -> void;

private:
    // Asset icon size should be 16x16
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Asset Icons",
              meta = (AllowPrivateAccess = true, ConfigRestartRequired = true))
    TMap<TSoftClassPtr<UObject>, FCk_CustomAssetStyle_Params> _CustomAssetIcons;

    // Asset thumbnail size should be 64x64
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Asset Thumbnails",
              meta = (AllowPrivateAccess = true, ConfigRestartRequired = true))
    TMap<TSoftClassPtr<UObject>, FCk_CustomAssetStyle_Params> _CustomAssetThumbnails;

public:
    CK_PROPERTY_GET(_CustomAssetIcons);
    CK_PROPERTY_GET(_CustomAssetThumbnails);
};

// --------------------------------------------------------------------------------------------------------------------
