#include "CkWidgetPasteArtifacts.h"

#include "CkAssetExporter_Log.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditorUtils.h"

#include <Animation/WidgetAnimation.h>
#include <Blueprint/WidgetTree.h>
#include <Components/Widget.h>
#include <Exporters/Exporter.h>
#include <HAL/FileManager.h>
#include <Misc/FileHelper.h>
#include <Misc/Paths.h>
#include <Misc/StringOutputDevice.h>
#include <UnrealExporter.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_widget_paste_artifacts
{
    // Mirrors the flag set FWidgetBlueprintEditorUtils::ExportWidgetsToText uses for the designer clipboard, so the
    // animation dumps read the same as the (pasteable) hierarchy text.
    constexpr auto CopyPortFlags = PPF_ExportsNotFullyQualified | PPF_Copy | PPF_Delimited;

    static auto
    Export_HierarchyCopyText(
        UWidgetBlueprint* InWidgetBlueprint,
        FString& OutText)
        -> bool
    {
        if (ck::Is_NOT_Valid(InWidgetBlueprint->WidgetTree) || ck::Is_NOT_Valid(InWidgetBlueprint->WidgetTree->RootWidget))
        { return false; }

        auto Widgets = TArray<UWidget*>{};
        Widgets.Add(InWidgetBlueprint->WidgetTree->RootWidget);
        UWidgetTree::GetChildWidgets(InWidgetBlueprint->WidgetTree->RootWidget, Widgets);

        FWidgetBlueprintEditorUtils::ExportWidgetsToText(Widgets, OutText);
        return NOT OutText.IsEmpty();
    }

    static auto
    Export_AnimationT3D(
        UWidgetAnimation* InAnimation)
        -> FString
    {
        auto Archive = FStringOutputDevice{};
        const auto Context = FExportObjectInnerContext{};

        constexpr auto Indent = 0;
        constexpr auto SelectedOnly = false;
        UExporter::ExportToOutputDevice(
            &Context, InAnimation, nullptr, Archive, TEXT("copy"), Indent, CopyPortFlags, SelectedOnly, nullptr);

        return MoveTemp(Archive);
    }

    static auto
    Delete_StaleAnimationDumps(
        const FString& InBasePathNoExt)
        -> void
    {
        auto& FileManager = IFileManager::Get();

        const auto Dir = FPaths::GetPath(InBasePathNoExt);
        const auto Wildcard = FPaths::GetCleanFilename(InBasePathNoExt) + TEXT(".animation.*.t3d.txt");

        auto FoundFiles = TArray<FString>{};
        constexpr auto FindFiles = true;
        constexpr auto FindDirectories = false;
        FileManager.FindFiles(FoundFiles, *FPaths::Combine(Dir, Wildcard), FindFiles, FindDirectories);

        for (const auto& FileName : FoundFiles)
        {
            constexpr auto RequireExists = false;
            FileManager.Delete(*FPaths::Combine(Dir, FileName), RequireExists);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_WidgetPasteArtifacts::
    ExportFor(
        UBlueprint* InBlueprint,
        const FString& InBasePathNoExt)
    -> FCk_WidgetPasteArtifactsResult
{
    using namespace ck_widget_paste_artifacts;

    auto Result = FCk_WidgetPasteArtifactsResult{};

    auto* WidgetBlueprint = Cast<UWidgetBlueprint>(InBlueprint);
    if (ck::Is_NOT_Valid(WidgetBlueprint, ck::IsValid_Policy_NullptrOnly{}))
    {
        Result.Succeeded = true;
        return Result;
    }

    // ---- Hierarchy (paste-ready) ----
    auto HierarchyText = FString{};
    if (NOT Export_HierarchyCopyText(WidgetBlueprint, HierarchyText))
    {
        Result.ErrorMessage = TEXT("WidgetBlueprint has no exportable widget tree (missing WidgetTree/RootWidget)");
        return Result;
    }

    const auto HierarchyPath = InBasePathNoExt + TEXT(".hierarchy.copy.txt");
    if (NOT FFileHelper::SaveStringToFile(HierarchyText, *HierarchyPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        Result.ErrorMessage = ck::Format_UE(TEXT("Failed to write [{}]"), HierarchyPath);
        return Result;
    }
    Result.WrittenFiles.Add(HierarchyPath);

    // ---- Animations (reference-grade, not pasteable) ----
    Delete_StaleAnimationDumps(InBasePathNoExt);

    for (const auto& Animation : WidgetBlueprint->Animations)
    {
        if (ck::Is_NOT_Valid(Animation))
        { continue; }

        const auto AnimationText = Export_AnimationT3D(Animation);
        if (AnimationText.IsEmpty())
        {
            Result.ErrorMessage = ck::Format_UE(TEXT("Animation [{}] produced empty T3D text"), Animation->GetName());
            return Result;
        }

        const auto AnimationPath = InBasePathNoExt + TEXT(".animation.") + Animation->GetName() + TEXT(".t3d.txt");
        if (NOT FFileHelper::SaveStringToFile(AnimationText, *AnimationPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        {
            Result.ErrorMessage = ck::Format_UE(TEXT("Failed to write [{}]"), AnimationPath);
            return Result;
        }
        Result.WrittenFiles.Add(AnimationPath);
    }

    Result.Succeeded = true;
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------
