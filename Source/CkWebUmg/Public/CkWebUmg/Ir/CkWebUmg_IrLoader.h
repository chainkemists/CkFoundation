#pragma once

#include "CkWebUmg/Ir/CkWebUmg_Ir.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::webumg
{
    /**
     * Parse a *.ckui.json document (schema v1). Fails loudly (ensure + unset optional) on a
     * schema-version mismatch or a structurally invalid document — never a partial tree.
     */
    CKWEBUMG_API auto
    LoadIrDocument(
        const FString& InJsonText)
        -> TOptional<FCkWebUmg_IrDocument>;

    CKWEBUMG_API auto
    LoadIrDocumentFromFile(
        const FString& InFilePath)
        -> TOptional<FCkWebUmg_IrDocument>;
}

// --------------------------------------------------------------------------------------------------------------------
