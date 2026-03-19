#include "CkStructTypeSelector.h"

// --------------------------------------------------------------------------------------------------------------------

auto FCk_StructTypeSelector::Get_StructType() const -> UScriptStruct*
{
    return _StructType;
}

auto FCk_StructTypeSelector::Set_StructType(UScriptStruct* InStructType) -> void
{
    _StructType = InStructType;
}

auto FCk_StructTypeSelector::IsValid() const -> bool
{
    return _StructType != nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
