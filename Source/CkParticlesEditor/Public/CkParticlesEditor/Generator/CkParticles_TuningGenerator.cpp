#include "CkParticlesEditor/Generator/CkParticles_TuningGenerator.h"

#include "CkParticles/ScriptDefinition/CkParticles_ScriptDefinition_Naming.h"
#include "CkParticles/ScriptDefinition/CkParticles_TuningDefinition.h"
#include "CkParticlesEditor_Log.h"

#include "CkCore/Macros/CkMacros.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

// --------------------------------------------------------------------------------------------------------------------
// Per-behavior tuning assets: one UCkParticles_TuningDefinition per roster id, created ONLY where none exists. A
// present asset is user data — the whole point of the convention is that a designer edits it in the editor and every
// later regeneration leaves the edit alone.
//
// The part-row ROSTER is the exception, and it is not user data: it is derived from the behavior's renderer band, so
// a band that gains or loses a renderer must move the rows with it. Reconciling adds and removes rows and refreshes
// their generator-owned names; it never reads or writes a value a designer typed.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::particles_editor::TuningGenLocal
{
    static auto Make_PartRow(const ck::particles::FCk_ParticlesPartInfo& InPart) -> FCkParticles_PartTuning_AssetRow
    {
        auto Row = FCkParticles_PartTuning_AssetRow{};
        Row._PartName = InPart.DisplayName;
        Row._VisTag   = InPart.VisTag;

        return Row;
    }

    // Answers whether anything about the roster moved. Rows are matched by VisTag, which is a part's identity; the
    // name is a label the generator owns and refreshes in place.
    static auto Reconcile_PartRows(UCkParticles_TuningDefinition* InTuning, const int32 InBehaviorId) -> bool
    {
        const auto Parts = ck::particles::Get_BehaviorTunableParts(InBehaviorId);

        auto Reconciled = TArray<FCkParticles_PartTuning_AssetRow>{};
        Reconciled.Reserve(Parts.Num());

        auto NumAdded   = 0;
        auto NumRenamed = 0;

        for (const auto& Part : Parts)
        {
            const auto* Existing = InTuning->_Parts.FindByPredicate(
                [&](const FCkParticles_PartTuning_AssetRow& InRow) -> bool
                { return InRow._VisTag == Part.VisTag; });

            if (Existing == nullptr)
            {
                Reconciled.Add(Make_PartRow(Part));
                ++NumAdded;
                continue;
            }

            auto Row = *Existing;

            if (Row._PartName != Part.DisplayName)
            {
                Row._PartName = Part.DisplayName;
                ++NumRenamed;
            }

            Reconciled.Add(Row);
        }

        auto NumRemoved = 0;

        for (const auto& Row : InTuning->_Parts)
        {
            const auto StillDeclared = Parts.ContainsByPredicate(
                [&](const ck::particles::FCk_ParticlesPartInfo& InPart) -> bool
                { return InPart.VisTag == Row._VisTag; });

            if (StillDeclared)
            { continue; }

            ++NumRemoved;

            ck::particles_editor::Log(TEXT("Behavior {}: dropping tuning row [{}] on VisTag {} — the behavior's "
                "renderer band no longer declares that part"), InBehaviorId, Row._PartName, Row._VisTag);
        }

        auto RosterMoved = NumAdded > 0 || NumRemoved > 0 || NumRenamed > 0;

        // Order is the band's, so the asset reads the way the effect draws. A pure reorder still has to be written,
        // and so does a length mismatch — which is what a hand-duplicated VisTag row looks like from here.
        if (NOT RosterMoved && Reconciled.Num() != InTuning->_Parts.Num())
        { RosterMoved = true; }

        if (NOT RosterMoved)
        {
            for (auto Index = 0; Index < Reconciled.Num(); ++Index)
            {
                if (Reconciled[Index]._VisTag == InTuning->_Parts[Index]._VisTag)
                { continue; }

                RosterMoved = true;
                break;
            }
        }

        if (NOT RosterMoved)
        { return false; }

        InTuning->_Parts = MoveTemp(Reconciled);

        return true;
    }

    static auto Save_TuningAsset(UCkParticles_TuningDefinition* InTuning, const FString& InPkgPath) -> bool
    {
        auto* Package = InTuning->GetPackage();
        if (Package == nullptr)
        { return false; }

        InTuning->MarkPackageDirty();

        const auto FileName = FPackageName::LongPackageNameToFilename(
            InPkgPath, FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        return UPackage::SavePackage(Package, InTuning, *FileName, SaveArgs);
    }

    static auto Create_TuningAsset(const int32 InBehaviorId) -> bool
    {
        const auto PkgPath   = ck::particles::Get_BehaviorTuningAssetPackagePath(InBehaviorId);
        const auto AssetName = FPackageName::GetShortName(PkgPath);

        auto* Package = CreatePackage(*PkgPath);
        if (Package == nullptr)
        { return false; }

        auto* Tuning = NewObject<UCkParticles_TuningDefinition>(Package, *AssetName, RF_Public | RF_Standalone);
        if (Tuning == nullptr)
        { return false; }

        for (const auto& Part : ck::particles::Get_BehaviorTunableParts(InBehaviorId))
        { Tuning->_Parts.Add(Make_PartRow(Part)); }

        FAssetRegistryModule::AssetCreated(Tuning);

        return Save_TuningAsset(Tuning, PkgPath);
    }
}

namespace ck::particles_editor
{
    auto Generate_AllTuningAssets() -> void
    {
        using namespace TuningGenLocal;

        auto NumCreated    = 0;
        auto NumKept       = 0;
        auto NumReconciled = 0;

        for (auto BehaviorId = 0; BehaviorId < ck::particles::NumBehaviors; ++BehaviorId)
        {
            const auto PkgPath = ck::particles::Get_BehaviorTuningAssetPackagePath(BehaviorId);
            if (PkgPath.IsEmpty())
            { continue; }

            if (NOT FPackageName::DoesPackageExist(PkgPath))
            {
                if (Create_TuningAsset(BehaviorId))
                {
                    ++NumCreated;
                    continue;
                }

                ck::particles_editor::Error(TEXT("Failed to create the tuning asset for behavior {} at [{}]"),
                    BehaviorId, PkgPath);
                continue;
            }

            ++NumKept;

            const auto ObjectPath = ck::particles::Get_BehaviorTuningAssetObjectPath(BehaviorId);
            auto*      Tuning     = LoadObject<UCkParticles_TuningDefinition>(nullptr, *ObjectPath);

            if (Tuning == nullptr)
            {
                ck::particles_editor::Error(TEXT("Failed to load the existing tuning asset for behavior {} at [{}] "
                    "— its part roster is left unreconciled"), BehaviorId, ObjectPath);
                continue;
            }

            if (NOT Reconcile_PartRows(Tuning, BehaviorId))
            { continue; }

            if (Save_TuningAsset(Tuning, PkgPath))
            {
                ++NumReconciled;
                continue;
            }

            ck::particles_editor::Error(TEXT("Failed to save the reconciled part roster of behavior {}'s tuning "
                "asset at [{}]"), BehaviorId, PkgPath);
        }

        ck::particles_editor::Log(TEXT("CkParticles tuning assets: {} created, {} kept, {} reconciled (existing "
            "tuning VALUES are never overwritten)"), NumCreated, NumKept, NumReconciled);
    }
}

// --------------------------------------------------------------------------------------------------------------------
