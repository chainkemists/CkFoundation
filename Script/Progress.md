# AngelScript Handle Conversion Progress

Generated: 2026-01-17 17:44:01

## Summary

| Metric | Count |
|--------|-------|
| Total Files Scanned | 27 |
| Files Modified | 0 |
| Simple Renames (To_FCk_Handle_XYZ → As_XYZ) | 0 |
| Pattern Optimizations (convert-check → Is_/As_) | 0 |

## Files Processed

| File | Simple Renames | Pattern Optimizations |
|------|----------------|----------------------|
| *(No files needed modification)* | - | - |

## Conversion Rules Applied

### Rule 1: Simple Rename
`ngelscript
// Before
Handle.To_FCk_Handle_Probe()

// After  
Handle.As_Probe()
`

### Rule 2: Convert-Check Optimization
`ngelscript
// Before
auto MaybeProbe = KilledEntity.To_FCk_Handle_Probe();
if (ck::IsValid(MaybeProbe))
{
    MaybeProbe.Request_EnableDisable(...);
}

// After
if (KilledEntity.Is_Probe())
{
    KilledEntity.As_Probe().Request_EnableDisable(...);
}
`

**Note:** Pattern optimization only applies when the converted handle is used exactly once inside the if-block.