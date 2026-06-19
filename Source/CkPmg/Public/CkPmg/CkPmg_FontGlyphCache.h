#pragma once
#include "CoreMinimal.h"

namespace ck::pmg
{
    // Task 1 link smoke (kept).
    CKPMG_API auto FontGlyph_SelfTest_FreeTypeInitDone() -> bool;

    struct CKPMG_API FCachedGlyph
    {
        TArray<TArray<FVector2D>> Contours;   // EM units, FreeType orientation preserved
        TArray<FVector2D>         TessVerts;   // EM units
        TArray<FIntVector>        TessTris;    // indices into TessVerts
        float                     AdvanceEm = 0.0f;
        bool                      bHasGeometry = false;
    };

    class CKPMG_API FFontGlyphCache
    {
    public:
        static FFontGlyphCache& Get();

        // Singleton: non-copyable. Out-of-line dtor (FFaceEntry is incomplete in this header).
        FFontGlyphCache(const FFontGlyphCache&) = delete;
        FFontGlyphCache& operator=(const FFontGlyphCache&) = delete;
        ~FFontGlyphCache();

        int32              EnsureFace(const TArray<uint8>& InFontBytes);
        const FCachedGlyph& GetOrBuildGlyph(int32 InFaceKey, uint32 InCodepoint);
        float              Get_LineHeightEm(int32 InFaceKey) const;
        void               Shutdown();

    private:
        FFontGlyphCache() = default;

        void*                          _FtLibrary = nullptr; // FT_Library (opaque — keeps FreeType out of this public header)

        struct FFaceEntry;
        TArray<TUniquePtr<FFaceEntry>> _Faces;
        TMap<uint32, int32>            _FaceKeyByHash;  // font-bytes CRC -> index into _Faces
        FCachedGlyph                   _EmptyGlyph;     // returned on hard failure
    };
}
