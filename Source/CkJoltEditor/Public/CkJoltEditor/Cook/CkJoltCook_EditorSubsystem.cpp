#include "CkJoltCook_EditorSubsystem.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkJoltEditor/Cook/CkJoltCook_MeshShapeCooker.h"
#include "CkJoltEditor/Cook/CkJoltCook_WorldCooker.h"

#include <Editor.h>
#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_cook_editor_subsystem
{
    static auto Get_EditorWorld() -> UWorld*
    {
        if (ck::Is_NOT_Valid(GEditor))
        { return nullptr; }

        return GEditor->GetEditorWorldContext().World();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Cook_CurrentWorld()
    -> bool
{
    auto* World = ck_jolt_cook_editor_subsystem::Get_EditorWorld();

    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("No editor world to cook"))
    { return false; }

    constexpr auto DryRun = false;
    return FCk_Jolt_WorldCooker::Cook_World(*World, DryRun)._Success;
}

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Cook_CurrentWorld_DryRun()
    -> bool
{
    auto* World = ck_jolt_cook_editor_subsystem::Get_EditorWorld();

    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("No editor world to cook"))
    { return false; }

    constexpr auto DryRun = true;
    return FCk_Jolt_WorldCooker::Cook_World(*World, DryRun)._Success;
}

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Validate_CurrentWorld()
    -> bool
{
    auto* World = ck_jolt_cook_editor_subsystem::Get_EditorWorld();

    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("No editor world to validate"))
    { return false; }

    return FCk_Jolt_WorldCooker::Validate_World(*World)._Success;
}

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Cook_MeshShapes()
    -> bool
{
    constexpr auto DryRun = false;
    return FCk_Jolt_MeshShapeCooker::Cook_MeshShapes(DryRun)._Success;
}

// --------------------------------------------------------------------------------------------------------------------
