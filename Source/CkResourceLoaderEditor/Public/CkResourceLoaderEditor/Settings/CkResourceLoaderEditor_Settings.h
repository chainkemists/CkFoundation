#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include <IDetailCustomization.h>

#include "CkResourceLoaderEditor_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    class FResourceLoaderEditor_ProjectSettings_Details : public IDetailCustomization
{
    public:
        /** Makes a new instance of this detail layout class for a specific detail view requesting it */
        static auto MakeInstance() -> TSharedRef<IDetailCustomization>;

        /** IDetailCustomization interface */
        auto CustomizeDetails(
            IDetailLayoutBuilder& DetailBuilder) -> void override;
    };
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Resource Loader Editor"))
class CKRESOURCELOADEREDITOR_API UCk_ResourceLoaderEditor_ProjectSettings_UE : public UCk_Editor_ProjectSettings_UE
{
    GENERATED_BODY()

    friend class UCk_Utils_ResourceLoaderEditor_ProjectSettings_UE;

public:
    CK_GENERATED_BODY(UCk_ResourceLoaderEditor_ProjectSettings_UE);

protected:
    auto PostEditChangeProperty(
        FPropertyChangedEvent& InPropertyChangedEvent) -> void override;

private:
    UPROPERTY(Config, EditDefaultsOnly, Category = "Asset Loading",
              meta = (AllowPrivateAccess = true, AllowAbstract = true))
    TArray<FSoftClassPath> _ClassesToLoad;

    UPROPERTY(Transient, EditInstanceOnly, Category = "Asset Loading - DEBUG",
              meta = (AllowPrivateAccess = true, DisplayThumbnail="false"))
    TArray<FSoftObjectPath> _LoadedObjects;

public:
    CK_PROPERTY_GET(_ClassesToLoad);
};

// --------------------------------------------------------------------------------------------------------------------

class CKRESOURCELOADEREDITOR_API UCk_Utils_ResourceLoaderEditor_ProjectSettings_UE
{
public:
    static auto
    Get_ClassesToLoad() -> const TArray<FSoftClassPath>&;

    static auto
    Request_AddLoadedObject(
        const FSoftObjectPath& InLoadedObject) -> void;

    static auto
    Request_AddLoadedObjects(
        const TArray<FSoftObjectPath>& InLoadedObjects) -> void;

    static auto
    Request_ClearAllLoadedObjects() -> void;
};

// --------------------------------------------------------------------------------------------------------------------
