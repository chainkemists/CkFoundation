#include "CkJoltCook_AssetSave.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkJolt/CkJolt_Log.h"

#include <AssetRegistry/AssetRegistryModule.h>
#include <HAL/PlatformFileManager.h>
#include <ISourceControlModule.h>
#include <Misc/PackageName.h>
#include <SourceControlHelpers.h>
#include <UObject/Package.h>
#include <UObject/SavePackage.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_cook_asset_save
{
    static auto DoMake_FileWritable(
        const FString& InFileName)
        -> void
    {
        auto& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

        if (NOT PlatformFile.FileExists(*InFileName))
        { return; }

        if (NOT PlatformFile.IsReadOnly(*InFileName))
        { return; }

        // Checking out is the correct path under LFS locking: the Git provider takes the lock AND clears
        // the read-only flag, so the write we are about to make is one we actually own.
        constexpr auto Silent = true;
        if (ISourceControlModule::Get().IsEnabled() && USourceControlHelpers::CheckOutFile(InFileName, Silent))
        { return; }

        // Cooked data is generated, never hand-authored, so the cook is authoritative for it and clearing
        // the flag locally beats aborting the bake. Loud on purpose: if someone else holds the LFS lock,
        // this local overwrite collides when it is committed.
        ck::jolt::Warning(TEXT("JoltCook: [{}] is READ-ONLY and could not be checked out - clearing the "
            "read-only flag to overwrite it. Under Git LFS locking that means you do not hold its lock; "
            "check the cooked-data folder out before cooking or this write will conflict on commit."),
            InFileName);

        constexpr auto ReadOnly = false;
        PlatformFile.SetReadOnly(*InFileName, ReadOnly);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::cook
{
    auto
        Save_CookedAsset(
            UObject& InAsset)
        -> bool
    {
        using namespace ck_jolt_cook_asset_save;

        auto* Package = InAsset.GetOutermost();

        const auto PackageIsValid = ck::IsValid(Package);
        CK_ENSURE_IF_NOT(PackageIsValid, TEXT("Cooked asset [{}] has no outermost package"), InAsset.GetName())
        { return false; }

        FAssetRegistryModule::AssetCreated(&InAsset);
        Package->MarkPackageDirty();

        const auto FileName = FPackageName::LongPackageNameToFilename(
            Package->GetName(), FPackageName::GetAssetPackageExtension());

        DoMake_FileWritable(FileName);

        auto SaveArgs = FSavePackageArgs{};
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;

        // SAVE_NoError downgrades SavePackage's failure path from Error+GError to a plain Warning, and
        // pointing Error at GLog keeps every other failure path off the fatal device too. Without both, a
        // read-only target (or a locked handle, or a full disk) kills the editor mid-cook via appError.
        SaveArgs.SaveFlags = SAVE_NoError;
        SaveArgs.Error = GLog;

        return UPackage::SavePackage(Package, &InAsset, *FileName, SaveArgs);
    }
}

// --------------------------------------------------------------------------------------------------------------------
