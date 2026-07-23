#include "CkDialogEmitter_Fragment.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

CK_ECS_DEFINE_CALLSTACK_ANGELSCRIPT_UTILS(CKDIALOG_API, dialog, ck::FFragment_DialogEmitter_Requests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FDialog_CooldownSentinels::
        Forever()
        -> const FCk_Time&
    {
        // An expiry no world-time ever reaches, so the (Now < Expiry) active test always holds.
        static const auto Sentinel = FCk_Time{TNumericLimits<double>::Max()};
        return Sentinel;
    }
}

// --------------------------------------------------------------------------------------------------------------------
