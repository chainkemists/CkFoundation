#include "CkAssetExporter_OnSaveHook.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/Dispatch/CkAssetExporter_Dispatch.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include <CoreGlobals.h>
#include <HAL/IConsoleManager.h>
#include <Misc/PackageName.h>
#include <Misc/Paths.h>
#include <UObject/ObjectSaveContext.h>
#include <UObject/Package.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_asset_exporter_onsavehook
{
    static TAutoConsoleVariable<bool> CVarAutoSidecarOnSave(
        TEXT("ck.AssetExporter.AutoSidecarOnSave"),
        true,
        TEXT("Refresh the sibling sidecar export whenever a logic-bearing asset is saved in the editor."));

    static FDelegateHandle GPackageSavedHandle;

    static auto
    OnPackageSaved(
        const FString& InPackageFileName,
        UPackage* InPackage,
        FObjectPostSaveContext InContext)
        -> void
    {
        if (NOT CVarAutoSidecarOnSave.GetValueOnGameThread())
        { return; }

        // Cooks are commandlets and save thousands of packages; procedural saves (incl. autosave machinery) are not
        // user-authored content states.
        if (IsRunningCommandlet() || InContext.IsProceduralSave())
        { return; }

        if (ck::Is_NOT_Valid(InPackage))
        { return; }

        const auto PackageName = InPackage->GetName();
        if (NOT PackageName.StartsWith(TEXT("/Game")))
        { return; }

        // A sidecar must mirror the asset's CANONICAL on-disk state — a save routed anywhere else (autosaves under
        // Saved/Autosaves, cook targets) would stamp the sidecar with content the real .uasset doesn't have.
        const auto CanonicalFile = FPackageName::LongPackageNameToFilename(
            PackageName, FPackageName::GetAssetPackageExtension());
        if (NOT FPaths::IsSamePath(InPackageFileName, CanonicalFile))
        { return; }

        auto* Asset = InPackage->FindAssetInPackage();
        if (ck::Is_NOT_Valid(Asset))
        { return; }

        if (NOT FCk_AssetExporter_Dispatch::Get_IsExportableClass(Asset->GetClass()))
        { return; }

        // JsonOnly: the summary text is lossy and gitignored in consuming projects, so it is never shared and drifts
        // silently out of date. The structured sidecar is the artifact that must stay truthful on every save; the
        // manual right-click path still writes both for anyone who wants the readable companion.
        constexpr auto SkipFresh = false;
        const auto Summary = FCk_AssetExporter_Dispatch::ExportAssets(
            { Asset->GetPathName() }, SkipFresh, ECk_AssetExporter_SidecarFormats::JsonOnly);

        if (Summary.Failed > 0)
        {
            ck::asset_exporter::Warning(
                TEXT("[OnSaveHook] Sidecar refresh FAILED for [{}]: {}"),
                Asset->GetPathName(),
                Summary.Entries.Num() > 0 ? Summary.Entries[0].ErrorMessage : TEXT("unknown"));
            return;
        }

        ck::asset_exporter::Display(TEXT("[OnSaveHook] Sidecar refreshed for [{}]"), Asset->GetPathName());
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetExporter_OnSaveHook::
    Register()
    -> void
{
    using namespace ck_asset_exporter_onsavehook;

    if (IsRunningCommandlet())
    { return; }

    GPackageSavedHandle = UPackage::PackageSavedWithContextEvent.AddStatic(&OnPackageSaved);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetExporter_OnSaveHook::
    Unregister()
    -> void
{
    using namespace ck_asset_exporter_onsavehook;

    if (GPackageSavedHandle.IsValid())
    {
        UPackage::PackageSavedWithContextEvent.Remove(GPackageSavedHandle);
        GPackageSavedHandle.Reset();
    }
}

// --------------------------------------------------------------------------------------------------------------------
