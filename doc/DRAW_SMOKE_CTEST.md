# DRAW Smoke CTest

This document describes the minimal DRAW smoke test used by OCCTDebug.
The test verifies that the local OCCT DRAWEXE runtime can start, load DRAW
plugins, and execute a tiny Tcl script without any CAD data or network access.

## Files

- `tests/draw_smoke.tcl`: minimal Tcl script.
- `scripts/run_draw_smoke.ps1`: CTest wrapper that finds DRAWEXE, prepares the
  runtime environment, captures logs, and checks the output.
- `tests/CMakeLists.txt`: registers the `draw_smoke` CTest.

## DRAWEXE Lookup Strategy

The wrapper searches in this order:

1. `-DrawExe <path>` argument passed to `scripts/run_draw_smoke.ps1`.
2. `OCCTDEBUG_DRAWEXE` environment variable.
3. Repository-local OCCT layouts under `depends/occt`:
   - `depends/occt/lib/<Config>/bind/DRAWEXE.exe`
   - `depends/occt/lib/<Config>/bin/DRAWEXE.exe`
   - `depends/occt/lib/<Config>/bini/DRAWEXE.exe`
   - `depends/occt/lib/Debug/bind/DRAWEXE.exe`
   - `depends/occt/lib/Release/bin/DRAWEXE.exe`
   - `depends/occt/lib/RelWithDebInfo/bini/DRAWEXE.exe`
4. Recursive fallback search under `depends/occt`.

No personal machine path is hardcoded.

## Runtime Environment

The wrapper sets these variables for the DRAW process only:

- `CASROOT=<repo>/depends/occt`
- `TCL_LIBRARY=<repo>/depends/occt_3rdparty/tcltk-8.6.15-x64/lib/tcl8.6`
- `TK_LIBRARY=<repo>/depends/occt_3rdparty/tcltk-8.6.15-x64/lib/tk8.6`
- `PATH` is prepended with:
  - DRAWEXE directory
  - `depends/occt_3rdparty/freetype-2.13.3-x64/bin`
  - `depends/occt_3rdparty/tcltk-8.6.15-x64/bin`
  - `depends/occt_3rdparty/tcltk-8.6.15-x64/lib`

The test also requires `depends/occt/src/DrawResources/DrawDefault`.

## Commands

Configure and build:

```powershell
cmake -S . -B out\build\debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build out\build\debug --config Debug
```

Run only the smoke test:

```powershell
ctest --test-dir out\build\debug -R draw_smoke --output-on-failure
```

## Expected Success Output

The test passes only when DRAW stdout contains:

```text
DRAW_SMOKE_OK
```

CTest also assigns these labels:

```text
draw;smoke;occt
```

## Failure Logs

On failure the wrapper prints a clear error and the log path:

```text
DRAW_SMOKE_FAILED: <reason>
DRAW_SMOKE_LOG: <build>/Testing/draw_smoke/draw_smoke.log
```

Captured stdout and stderr are written next to the main log:

- `<build>/Testing/draw_smoke/draw_smoke.stdout.log`
- `<build>/Testing/draw_smoke/draw_smoke.stderr.log`

## Limitations

- This is an environment smoke test, not a full geometry regression test.
- It does not load CAD data.
- It does not run testgrid or testdiff.
- It does not validate Qt UI behavior.
- It does not modify OCCT source code.
