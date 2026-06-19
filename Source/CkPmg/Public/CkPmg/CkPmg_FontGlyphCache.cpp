#include "CkPmg/CkPmg_FontGlyphCache.h"

#if CK_PMG_WITH_FREETYPE
#include "ft2build.h"
#include FT_FREETYPE_H
#endif

namespace ck::pmg
{
    auto FontGlyph_SelfTest_FreeTypeInitDone() -> bool
    {
#if CK_PMG_WITH_FREETYPE
        FT_Library Library = nullptr;
        if (FT_Init_FreeType(&Library) != 0) { return false; }
        FT_Done_FreeType(Library);
        return true;
#else
        return false;
#endif
    }
}
