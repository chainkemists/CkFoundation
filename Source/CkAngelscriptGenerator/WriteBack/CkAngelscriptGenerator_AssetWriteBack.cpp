#include "CkAngelscriptGenerator/WriteBack/CkAngelscriptGenerator_AssetWriteBack.h"

#include "CkAngelscriptGenerator/Assets/CkAssetRegistryConfig.h"
#include "CkAngelscriptGenerator/Assets/CkAssetRegistrySubsystem.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"
#include "CkAngelscriptGenerator/WriteBack/CkAngelscriptGenerator_AccessorResolver.h"
#include "CkAngelscriptGenerator/WriteBack/CkAngelscriptGenerator_AssetBlockPatcher.h"
#include "CkAngelscriptGenerator/WriteBack/CkAngelscriptGenerator_AssetValueDiff.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Reference/CkAssetReferenceProvider.h"

#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Textures/SlateIcon.h"
#include "ToolMenu.h"
#include "ToolMenuEntry.h"
#include "ToolMenuSection.h"
#include "ToolMenus.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/AssetEditorToolkitMenuContext.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptManager.h>
#include <ClassGenerator/ASClass.h>
#endif

#define LOCTEXT_NAMESPACE "CkAngelscriptGeneratorWriteBack"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::angelscriptgenerator::write_back
{
    namespace ck_angelscript_generator_asset_write_back
    {
        const auto MenuOwnerName = FName{TEXT("CkAngelscriptGenerator.WriteBack")};
        const auto SectionName   = FName{TEXT("CkAngelscriptWriteBack")};
        const auto EntryName     = FName{TEXT("CkAngelscriptWriteBackButton")};

        // Spelled out rather than taken from FAssetEditorToolkit::DefaultAssetEditorToolBarName,
        // which is PRIVATE in this fork (AssetEditorToolkit.h:522-523) and so unreachable from a
        // plugin. The literal is the symbol's own definition (AssetEditorToolkit.cpp:52), and every
        // asset-editor toolbar registers against it as its parent (:1450-1453), which is what makes
        // one entry here reach every asset editor.
        const auto DefaultAssetEditorToolBarName = FName{TEXT("AssetEditor.DefaultToolBar")};

        FDelegateHandle GStartupCallbackHandle;
        FDelegateHandle GPropertyChangedHandle;

        // A toolbar's enabled-attribute is polled every frame per open editor, so the answer has to
        // be cached. `OnObjectPropertyChanged` is what makes the cache correct rather than merely
        // cheap: it is the one signal that a details-panel edit happened.
        TMap<TWeakObjectPtr<const UObject>, bool> GDiffersFromDefaultsCache;

        auto Handle_ObjectPropertyChanged(
            UObject*                   InObject,
            FPropertyChangedEvent&     /*InEvent*/) -> void
        {
            GDiffersFromDefaultsCache.Remove(InObject);
        }

        auto Get_DiffersFromDefaults_Cached(
            const UObject* InAsset) -> bool
        {
            if (ck::Is_NOT_Valid(InAsset, ck::IsValid_Policy_NullptrOnly{}))
            { return false; }

            if (const auto* Cached = GDiffersFromDefaultsCache.Find(InAsset))
            { return *Cached; }

            const auto Differs = FCkAsAssetValueDiff::Get_DiffersFromClassDefaults(InAsset);
            GDiffersFromDefaultsCache.Add(InAsset, Differs);
            return Differs;
        }

        auto Get_EditedLiteralAsset(
            const FToolMenuContext& InContext) -> UObject*
        {
            const auto* ToolkitContext = InContext.FindContext<UAssetEditorToolkitMenuContext>();
            if (ToolkitContext == nullptr)
            { return nullptr; }

            const auto Editing = ToolkitContext->GetEditingObjects();
            if (Editing.Num() != 1)
            { return nullptr; }

            auto* Asset = Editing[0];
            return FCkAsAssetWriteBack::Get_IsWriteBackCandidate(Asset) ? Asset : nullptr;
        }

        auto Show_Error(
            const FText& InTitle,
            const FText& InMessage) -> void
        {
            ck::angelscriptgenerator::Warning(TEXT("[WriteBack] {}"), InMessage.ToString());
            FMessageDialog::Open(EAppMsgType::Ok, InMessage, InTitle);
        }

        // Package metadata first, then the module's own code sections. Never a guessed path: writing
        // to the wrong .as would destroy an unrelated file.
        auto Try_ResolveSourceFile(
            const UObject* InAsset,
            const FString& InAssetName,
            FString&       OutPath,
            FString&       OutFailure) -> bool
        {
#if WITH_EDITOR
            // Non-const: FMetaData::GetValue is a non-const member on the package.
            if (auto* Package = InAsset->GetPackage();
                ck::IsValid(Package, ck::IsValid_Policy_NullptrOnly{}))
            {
                const auto Recorded = Package->GetMetaData().GetValue(InAsset, TEXT("ScriptAssetFilename"));
                if (NOT Recorded.IsEmpty() && FPaths::FileExists(Recorded))
                { OutPath = Recorded; return true; }
            }
#endif

#if WITH_ANGELSCRIPT_CK
            // The preprocessor rewrites the declaration into `void __Init_<Name>(<Type>)`, and the
            // module keeps the PROCESSED source per file — so the section containing that token is
            // the declaring file.
            if (const auto Module = FAngelscriptManager::Get().GetModuleContainingLiteralAsset(InAssetName);
                Module.IsValid())
            {
                const auto Needle = FString::Printf(TEXT("__Init_%s("), *InAssetName);

                for (const auto& Section : Module->Code)
                {
                    if (NOT Section.Code.Contains(Needle, ESearchCase::CaseSensitive))
                    { continue; }

                    if (FPaths::FileExists(Section.AbsoluteFilename))
                    { OutPath = Section.AbsoluteFilename; return true; }
                }
            }
#endif

            OutFailure = FString::Printf(
                TEXT("Could not find the .as file declaring '%s'. Its package metadata records no readable path, and ")
                TEXT("no loaded AngelScript module contains its `__Init_` body. Nothing was written."), *InAssetName);
            return false;
        }

        auto Gather_AccessorIndex() -> TMap<FString, FCk_ScriptAccessorEntry>
        {
            auto Files = TArray<FCk_GeneratedAccessorFile>{};

            for (const auto* Config : UCkAssetRegistrySubsystem::Request_DiscoverAllConfigs())
            {
                if (ck::Is_NOT_Valid(Config, ck::IsValid_Policy_NullptrOnly{}))
                { continue; }

                const auto OutputPath = UCkAssetRegistrySubsystem::Get_OutputDirectoryForRootPath(
                    Config->AssetDiscoveryRoot) / Config->OutputFileName;

                auto Contents = FString{};
                if (NOT FFileHelper::LoadFileToString(Contents, *OutputPath))
                { continue; }

                Files.Add(FCk_GeneratedAccessorFile{OutputPath, MoveTemp(Contents), Config->Namespace});
            }

            // Sorted so "first entry wins" on a duplicate object path is the same answer every run.
            Files.Sort([](const FCk_GeneratedAccessorFile& InA, const FCk_GeneratedAccessorFile& InB)
            {
                return InA.AbsolutePath < InB.AbsolutePath;
            });

            auto Entries = TArray<FCk_ScriptAccessorEntry>{};
            for (const auto& File : Files)
            { Entries.Append(FCkAsAccessorResolver::Parse_GeneratedAccessorFile(File)); }

            return FCkAsAccessorResolver::Build_Index(Entries);
        }

        auto Build_UnresolvedText(
            const FString&                        InAssetName,
            const TArray<FCk_WriteBackUnresolved>& InUnresolved) -> FText
        {
            auto Body = FString::Printf(
                TEXT("'%s' cannot be written back yet.\n\n")
                TEXT("Nothing was written. A partial write would be worse than none: saving the .as file makes ")
                TEXT("AngelScript re-run the asset's initializer, which resets every property it does not assign ")
                TEXT("back to the class default — so writing only the resolvable half would destroy the edits below ")
                TEXT("in the same gesture.\n\n")
                TEXT("Unresolved:\n"), *InAssetName);

            for (const auto& Unresolved : InUnresolved)
            { Body += FString::Printf(TEXT("\n  \x2022 %s\n      %s\n"), *Unresolved.PropertyPath, *Unresolved.Message); }

            return FText::FromString(Body);
        }

        auto Build_ConfirmationText(
            const FString&                        InAssetName,
            const FString&                        InFilePath,
            const TArray<FCk_AssetBlockLineDiff>& InDiff,
            bool                                  InHasLossyText,
            const TArray<FString>&                InClearedReferences) -> FText
        {
            auto Body = FString::Printf(TEXT("Write %d change(s) back to:\n%s\n\n"), InDiff.Num(), *InFilePath);

            for (const auto& Line : InDiff)
            {
                switch (Line.Op)
                {
                    case ECk_AssetBlockPatch_Op::ReplaceValue:
                        Body += FString::Printf(TEXT("  %d  - %s\n     +%s\n\n"),
                            Line.LineNumber, *Line.Before.TrimStart(), *Line.After);
                        break;
                    case ECk_AssetBlockPatch_Op::InsertLine:
                        Body += FString::Printf(TEXT("  %d  +%s\n\n"), Line.LineNumber, *Line.After);
                        break;
                    case ECk_AssetBlockPatch_Op::DeleteLine:
                        Body += FString::Printf(TEXT("  %d  - %s\n\n"), Line.LineNumber, *Line.Before.TrimStart());
                        break;
                }
            }

            if (NOT InClearedReferences.IsEmpty())
            {
                Body += FString::Printf(
                    TEXT("\nWARNING - %d reference(s) below are being CLEARED: %s\n")
                    TEXT("The .as file assigns each of these, but the loaded asset holds none. If you did not ")
                    TEXT("clear them yourself, this asset failed to resolve them at load time (an initializer ")
                    TEXT("that reaches the asset registry before it is scanned leaves the value null), and ")
                    TEXT("writing now would erase a working assignment from the source. Cancel and reopen the ")
                    TEXT("asset if you are not sure.\n"),
                    InClearedReferences.Num(), *FString::Join(InClearedReferences, TEXT(", ")));
            }

            if (InHasLossyText)
            {
                Body += TEXT("\nNote: an FText is being written as FText::FromString(...), which drops its ")
                        TEXT("localization namespace and key.\n");
            }

            Body += TEXT("\nIf this file is open with unsaved changes in another editor, that editor will report a ")
                    TEXT("conflict — the version on disk wins.\n");

            return FText::FromString(Body);
        }

        auto Execute_WriteBack(
            const FToolMenuContext& InContext) -> void
        {
            static const auto ErrorTitle   = LOCTEXT("WriteBackFailedTitle", "Write Back to Script");
            static const auto ConfirmTitle = LOCTEXT("WriteBackConfirmTitle", "Write Back to Script");

            auto* Asset = Get_EditedLiteralAsset(InContext);
            if (ck::Is_NOT_Valid(Asset, ck::IsValid_Policy_NullptrOnly{}))
            { return; }

            const auto AssetName = Asset->GetName();

            auto FilePath = FString{};
            if (auto Failure = FString{};
                NOT Try_ResolveSourceFile(Asset, AssetName, FilePath, Failure))
            {
                Show_Error(ErrorTitle, FText::FromString(Failure));
                return;
            }

            auto Snapshot = FCk_AsFileSnapshot{};
            if (NOT FCkAsAssetBlockPatcher::Try_ReadSnapshot(FilePath, Snapshot))
            {
                Show_Error(ErrorTitle, FText::FromString(Snapshot.ErrorMessage));
                return;
            }

            const auto Location = FCkAsAssetBlockPatcher::Find_AssetBlock(Snapshot.Contents, AssetName);
            if (NOT Location.Found)
            {
                Show_Error(ErrorTitle, FText::FromString(FString::Printf(
                    TEXT("Found '%s' but could not locate its `asset %s of ...` block inside it. Nothing was written."),
                    *FilePath, *AssetName)));
                return;
            }

            const auto Baseline = FCkAsAssetValueDiff::Build_ScratchBaseline(Asset->GetClass(), AssetName);
            if (ck::Is_NOT_Valid(Baseline.Get()))
            {
                Show_Error(ErrorTitle, FText::FromString(FString::Printf(
                    TEXT("Could not re-run `__Init_%s` to work out what the current file text produces, so there is ")
                    TEXT("no safe way to tell which values you changed. Nothing was written."), *AssetName)));
                return;
            }

            const auto Accessors = Gather_AccessorIndex();

            auto DiffContext = FCk_AssetValueDiffContext{};
            DiffContext.Accessors               = &Accessors;
            DiffContext.AnyProviderRegistered   = FCk_AssetReferenceProviderRegistry::Get().Get_HasAnyProvider();
            DiffContext.TargetBlockIsEditorOnly = FCkAsAssetBlockPatcher::Get_IsInsideEditorGuard(
                Snapshot.Contents, Location.DeclStart);

            for (const auto& Declared : FCkAsAssetBlockPatcher::Find_AllAssetDeclarations(Snapshot.Contents))
            { DiffContext.SameFileLiteralAssetNames.Add(Declared); }

            const auto Diff = FCkAsAssetValueDiff::Compute(
                Asset, Baseline.Get(), Asset->GetClass()->GetDefaultObject(/*bCreateIfNeeded=*/false), DiffContext);

            if (NOT Diff.Success)
            {
                Show_Error(ErrorTitle, Build_UnresolvedText(AssetName, Diff.Unresolved));
                return;
            }

            if (Diff.Entries.IsEmpty())
            {
                Show_Error(ErrorTitle, FText::FromString(FString::Printf(
                    TEXT("'%s' already matches what its .as file produces — there is nothing to write."), *AssetName)));
                return;
            }

            const auto Patch = FCkAsAssetBlockPatcher::Apply_Patch(Snapshot.Contents, AssetName, Diff.Entries);
            if (NOT Patch.Success)
            {
                Show_Error(ErrorTitle, FText::FromString(Patch.ErrorMessage));
                return;
            }

            // Reverting a property whose value is not produced by a top-level assignment cannot be
            // written: there is no line to remove. Writing the file anyway would change nothing but
            // its timestamp, and the reload that follows would put the old value straight back over
            // the revert — the same destruction a partial write causes.
            if (NOT Patch.UnmatchedDeletes.IsEmpty())
            {
                Show_Error(ErrorTitle, FText::FromString(FString::Printf(
                    TEXT("Cannot clear %s in '%s': nothing at the top level of the asset block assigns it, so ")
                    TEXT("the value comes from somewhere write-back does not edit (inside an `if`, a loop, or a ")
                    TEXT("helper call). Nothing was written — remove or change that code by hand."),
                    *FString::Join(Patch.UnmatchedDeletes, TEXT(", ")), *AssetName)));
                return;
            }

            if (Patch.Diff.IsEmpty())
            {
                Show_Error(ErrorTitle, FText::FromString(FString::Printf(
                    TEXT("'%s' produced no line changes, so there is nothing to write. Writing anyway would ")
                    TEXT("reload the script and overwrite your edit with the file's own value."), *AssetName)));
                return;
            }

            const auto Choice = FMessageDialog::Open(EAppMsgType::YesNo,
                Build_ConfirmationText(AssetName, FilePath, Patch.Diff, Diff.HasLossyText,
                    Diff.ClearedObjectReferences), ConfirmTitle);

            if (Choice != EAppReturnType::Yes)
            { return; }

            // The dialog can sit open for minutes while the file changes underneath — a save from
            // another editor, or a watcher-driven reload. The write is a whole-file replace, so a
            // stale snapshot would silently revert whatever landed in that window.
            auto Fresh = FCk_AsFileSnapshot{};
            if (NOT FCkAsAssetBlockPatcher::Try_ReadSnapshot(FilePath, Fresh) || Fresh.Contents != Snapshot.Contents)
            {
                Show_Error(ErrorTitle, FText::FromString(FString::Printf(
                    TEXT("'%s' changed on disk while the confirmation was open, so the patch was computed against ")
                    TEXT("stale text. Nothing was written — press the button again to recompute."), *FilePath)));
                return;
            }

            if (NOT FCkAsAssetBlockPatcher::Try_AtomicWrite(Snapshot, Patch.PatchedContents))
            {
                Show_Error(ErrorTitle, FText::FromString(FString::Printf(
                    TEXT("Failed to write '%s'. Check that it is not read-only or locked by another process."),
                    *FilePath)));
                return;
            }

            GDiffersFromDefaultsCache.Remove(Asset);

            ck::angelscriptgenerator::Log(
                TEXT("[WriteBack] Wrote {} change(s) for literal asset [{}] into [{}]"),
                Patch.Diff.Num(), AssetName, FilePath);
        }

        auto Construct_Section(
            UToolMenu* InMenu) -> void
        {
            if (ck::Is_NOT_Valid(InMenu, ck::IsValid_Policy_NullptrOnly{}))
            { return; }

            auto* Asset = Get_EditedLiteralAsset(InMenu->Context);
            if (ck::Is_NOT_Valid(Asset, ck::IsValid_Policy_NullptrOnly{}))
            { return; }

            auto Action = FToolUIAction{};
            Action.ExecuteAction    = FToolMenuExecuteAction::CreateStatic(&Execute_WriteBack);
            Action.CanExecuteAction = FToolMenuCanExecuteAction::CreateLambda(
                [](const FToolMenuContext& InContext)
                {
                    return Get_DiffersFromDefaults_Cached(Get_EditedLiteralAsset(InContext));
                });

            auto& Section = InMenu->FindOrAddSection(SectionName);

            auto Entry = FToolMenuEntry::InitToolBarButton(
                EntryName,
                FToolUIActionChoice{Action},
                LOCTEXT("WriteBackLabel", "Write Back"),
                LOCTEXT("WriteBackTooltip",
                    "Patch the edited property values back into the `asset ... of ... { }` block in this asset's .as "
                    "source. Literal assets have no .uasset, so this is the only way to save them."),
                FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("AssetEditor.SaveAsset")));

            Section.AddEntry(Entry);
        }

        auto Register_Section() -> void
        {
            // Owner-scoped so ShutdownModule can withdraw exactly this section and nothing else.
            const auto OwnerScope = FToolMenuOwnerScoped{MenuOwnerName};

            auto* Menu = UToolMenus::Get()->ExtendMenu(DefaultAssetEditorToolBarName);
            if (ck::Is_NOT_Valid(Menu, ck::IsValid_Policy_NullptrOnly{}))
            { return; }

            Menu->AddDynamicSection(SectionName,
                FNewSectionConstructChoice{FNewToolMenuDelegate::CreateStatic(&Construct_Section)});
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAssetWriteBack::
        Get_IsWriteBackCandidate(
            const UObject* InAsset)
        -> bool
    {
#if WITH_ANGELSCRIPT_CK
        if (ck::Is_NOT_Valid(InAsset, ck::IsValid_Policy_NullptrOnly{}))
        { return false; }

        if (InAsset->GetOuter() != FAngelscriptManager::Get().AssetsPackage)
        { return false; }

        // A script-declared class has no stable C++ identity to write against, and its own defaults
        // already round-trip through the .as file. A generated Blueprint class is excluded for the
        // same reason — the requester's rule is that the declared parent must be native.
        const auto* Class = InAsset->GetClass();
        return Cast<UASClass>(Class) == nullptr
            && Cast<UBlueprintGeneratedClass>(Class) == nullptr;
#else
        return false;
#endif
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAssetWriteBack::
        Register_ToolbarExtension()
        -> void
    {
        namespace detail = ck_angelscript_generator_asset_write_back;

        if (detail::GStartupCallbackHandle.IsValid())
        { return; }

        detail::GStartupCallbackHandle = UToolMenus::RegisterStartupCallback(
            FSimpleMulticastDelegate::FDelegate::CreateStatic(&detail::Register_Section));

        detail::GPropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddStatic(
            &detail::Handle_ObjectPropertyChanged);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAssetWriteBack::
        Unregister_ToolbarExtension()
        -> void
    {
        namespace detail = ck_angelscript_generator_asset_write_back;

        if (detail::GPropertyChangedHandle.IsValid())
        {
            FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(detail::GPropertyChangedHandle);
            detail::GPropertyChangedHandle.Reset();
        }

        if (detail::GStartupCallbackHandle.IsValid())
        {
            UToolMenus::UnRegisterStartupCallback(detail::GStartupCallbackHandle);
            detail::GStartupCallbackHandle.Reset();
        }

        if (UObjectInitialized())
        { UToolMenus::UnregisterOwner(detail::MenuOwnerName); }

        detail::GDiffersFromDefaultsCache.Empty();
    }
}

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
