# Boolean half-space shell cut

## Summary
Shell cut by half-space produces no cap face.

## Environment
- OCCT Version: 7.9.3
- Compiler: MSVC 19.39
- Build Type: Debug
- Category: Boolean
- FuzzyValue: 0.01

## Input Files
- case/input.brep
- case/tool.step

## Reproduction Steps
1. Load input shell.
2. Run BRepAlgoAPI_Cut with half-space tool.

## Expected Behavior
Result has a section cap face.

## Actual Behavior
Result remains an open shell.

## Notes / Suspected Area
Likely BOPAlgo behavior for sheet bodies.