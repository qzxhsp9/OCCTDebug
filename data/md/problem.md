# Boolean half-space shell cut

## Summary

Shell cut by half-space produces no cap face.

## Environment

- Category: Boolean
- OCCT Version: 7.9.3
- Compiler: MSVC 19.39
- Build Type: Debug

## Input Files

- D:\data\workspace\github\OCCTDebug\data\bug\boolean\0001\plane.step
- D:\data\workspace\github\OCCTDebug\data\bug\boolean\0001\sphere.step

## Reproduction Steps

1. Load input shell.
2. Run BRepAlgoAPI_Cut with half-space tool.

## Expected Behavior

Result status: has a section cap face.

## Actual Behavior

Observed result: remains an open shell.

## Notes / Suspected Area

Likely BOPAlgo behavior for sheet bodies.
FuzzyValue: 0.000001
