#include "CkJoltMeshShapeCook_AssetAction.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkJolt/Settings/CkJolt_ProjectSettings.h"
#include "CkJolt/StaticWorld/CkJoltMeshShape_Utils.h"

#include "CkJoltEditor/Cook/CkJoltCook_EditorSubsystem.h"

#include <ContentBrowserModule.h>
#include <Editor.h>
#include <Engine/StaticMesh.h>
#include <IContentBrowserSingleton.h>
#include <Modules/ModuleManager.h>
#include <ToolMenus.h>

#define LOCTEXT_NAMESPACE "CkJoltMeshShapeCookAssetAction"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_mesh_shape_cook_asset_action
{
    static auto Get_SelectedStaticMeshes() -> TArray<FAssetData>
    {
        auto& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

        auto SelectedAssets = TArray<FAssetData>{};
        ContentBrowser.Get().GetSelectedAssets(SelectedAssets);

        // FAssetData only — right-click must never load the selection just to build a menu.
        return ck::algo::Filter(SelectedAssets, [](const FAssetData& InAssetData)
        {
            return InAssetData.IsInstanceOf(UStaticMesh::StaticClass());
        });
    }

    static auto Get_MeshesUnderBakedRoots(const TArray<FAssetData>& InMeshes) -> TArray<FAssetData>
    {
        return ck::algo::Filter(InMeshes, [](const FAssetData& InAssetData)
        {
            return ck::jolt::bake::mesh_shape_utils::Get_IsUnderBakedRoot(InAssetData.PackageName.ToString());
        });
    }

    static auto Get_DisabledTooltip(int32 InNumSelected) -> FText
    {
        if (UCk_Utils_Jolt_ProjectSettings::Get_BakedMeshShapeRoots().IsEmpty())
        {
            return LOCTEXT("CookMeshShape_NoRoots",
                "The per-mesh Jolt pre-bake is switched off: Project Settings > Jolt > Static World > "
                "Baked Mesh Shape Roots is empty, so no mesh has a cooked shape to keep up to date.");
        }

        return FText::Format(
            LOCTEXT("CookMeshShape_OutsideRoots",
                "{0}|plural(one=This mesh sits,other=These meshes sit) outside Baked Mesh Shape Roots "
                "(Project Settings > Jolt > Static World). The runtime builds their collision directly, "
                "so there is no cooked shape to generate."),
            InNumSelected);
    }

    static auto Get_EnabledTooltip(int32 InNumEligible, int32 InNumSelected) -> FText
    {
        if (InNumEligible == InNumSelected)
        {
            return LOCTEXT("CookMeshShape_All",
                "Rebuild the pre-baked Jolt collision shape for the selected mesh(es). Runs in the "
                "background with a progress notification. Meshes whose collision is primitive-only are "
                "skipped — the runtime rebuilds those more cheaply than loading a cooked shape.");
        }

        return FText::Format(
            LOCTEXT("CookMeshShape_Subset",
                "Rebuild the pre-baked Jolt collision shape for {0} of the {1} selected meshes. The rest "
                "sit outside Baked Mesh Shape Roots and have no cooked shape."),
            InNumEligible, InNumSelected);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::cook
{
    void RegisterMeshShapeCookContextMenu()
    {
        using namespace ck_jolt_mesh_shape_cook_asset_action;

        auto* ToolMenus = UToolMenus::Get();
        if (ToolMenus == nullptr)
        { return; }

        auto* Menu = ToolMenus->ExtendMenu("ContentBrowser.AssetContextMenu");
        if (Menu == nullptr)
        { return; }

        auto& Section = Menu->FindOrAddSection("CkJoltActions");

        Section.AddDynamicEntry("CookJoltMeshShape",
            FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
            {
                const auto SelectedMeshes = Get_SelectedStaticMeshes();

                if (SelectedMeshes.IsEmpty())
                { return; }

                const auto Eligible = Get_MeshesUnderBakedRoots(SelectedMeshes);
                const auto NumEligible = Eligible.Num();
                const auto NumSelected = SelectedMeshes.Num();

                auto Action = FToolUIAction{};

                Action.CanExecuteAction = FToolMenuCanExecuteAction::CreateLambda(
                    [NumEligible](const FToolMenuContext&)
                    {
                        return NumEligible > 0;
                    });

                Action.ExecuteAction = FToolMenuExecuteAction::CreateLambda(
                    [Eligible](const FToolMenuContext&)
                    {
                        if (ck::Is_NOT_Valid(GEditor))
                        { return; }

                        if (auto* Subsystem = GEditor->GetEditorSubsystem<UCk_JoltCook_EditorSubsystem_UE>())
                        { Subsystem->Request_CookMeshShapes_ForAssets(Eligible); }
                    });

                InSection.AddMenuEntry(
                    "CookJoltMeshShape",
                    FText::Format(LOCTEXT("CookMeshShape_Label", "Cook Jolt Mesh {0}|plural(one=Shape,other=Shapes)"),
                        NumSelected),
                    NumEligible > 0
                        ? Get_EnabledTooltip(NumEligible, NumSelected)
                        : Get_DisabledTooltip(NumSelected),
                    FSlateIcon{},
                    Action);
            }));
    }
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
