#include "CkSnapshot/Inspection/CkSnapshot_Inspection_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_SnapshotInspection_Document::
    Get_ErrorCount() const
    -> int32
{
    auto Count = 0;
    for (const auto& Diagnostic : _Diagnostics)
    {
        if (Diagnostic.Get_Severity() == ECk_SnapshotInspection_Severity::Error)
        { ++Count; }
    }
    return Count;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_SnapshotInspection_Document::
    Get_WarningCount() const
    -> int32
{
    auto Count = 0;
    for (const auto& Diagnostic : _Diagnostics)
    {
        if (Diagnostic.Get_Severity() == ECk_SnapshotInspection_Severity::Warning)
        { ++Count; }
    }
    return Count;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_SnapshotInspection_Document::
    TryGet_EntityIndexForSavedId(
        uint32 InSavedId) const
    -> int32
{
    const auto* Found = _SavedIdToEntityIndex.Find(InSavedId);
    return Found != nullptr ? *Found : INDEX_NONE;
}

// --------------------------------------------------------------------------------------------------------------------
