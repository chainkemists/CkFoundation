#include "CkGrid_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_grid_settings
{
    namespace cvar
    {
        static bool PreviewAllGrids = false;
        static FAutoConsoleVariableRef CVar_PreviewAllGrids(TEXT("ck.Grid.PreviewAllGrids"),
            PreviewAllGrids,
            TEXT("Draw the debug information of all existing 2D Grid Systems"));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Grid_Settings::
    Get_DebugPreviewAllGrids()
    -> bool
{
    return ck_grid_settings::cvar::PreviewAllGrids;
}

// --------------------------------------------------------------------------------------------------------------------