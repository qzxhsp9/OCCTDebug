# DRAW Smoke CTest

This document describes the minimal DRAW smoke tests used by OCCTDebug.
The tests verify that the local OCCT DRAWEXE runtime can start, load DRAW
plugins, execute tiny Tcl scripts, and run a minimal in-memory `checkshape`
command without any CAD data or network access.

## Files

- `tests/draw_smoke.tcl`: minimal DRAW startup Tcl script.
- `tests/draw_checkshape_smoke.tcl`: minimal box creation and `checkshape`
  Tcl script.
- `scripts/run_draw_smoke.ps1`: CTest wrapper that finds DRAWEXE, prepares the
  runtime environment, captures logs, and checks the output.
- `scripts/parse_draw_log.ps1`: standalone parser for DRAW logs and
  `checkshape` output.
- `scripts/register_temp_draw_ctest.ps1`: creates a temporary CTest entry for a
  user-provided repro Tcl script.
- `scripts/export_repro_pack.ps1`: exports a minimal DRAW Repro Pack.
- `tests/CMakeLists.txt`: registers the `draw_smoke` and
  `draw_checkshape_smoke` CTests.

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

Run only the startup smoke test:

```powershell
ctest --test-dir out\build\debug -R draw_smoke --output-on-failure
```

Run the startup and `checkshape` smoke tests:

```powershell
ctest --test-dir out\build\debug -R "draw_.*smoke" --output-on-failure
```

Run the environment collector and include the latest DRAW CTest result paths:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_env.ps1 -BuildDir out\build\debug
```

## Expected Success Output

The startup smoke test passes only when DRAW stdout contains:

```text
DRAW_SMOKE_OK
```

The `checkshape` smoke test passes only when DRAW stdout contains:

```text
DRAW_CHECKSHAPE_OK
```

The `checkshape` output should also include:

```text
This shape seems to be valid
```

CTest assigns these labels:

```text
draw;smoke;occt
draw;smoke;occt;checkshape
```

`draw_smoke` provides the CTest fixture `draw_ready`. Future testgrid/testdiff
CTest entries should require this fixture so DRAW startup remains the
environment gate before heavier regression work.

## Failure Logs

On failure the wrapper prints a clear error and the log path:

```text
DRAW_SMOKE_FAILED: <reason>
DRAW_SMOKE_LOG: <build>/Testing/draw_smoke/draw_smoke.log
```

Captured stdout and stderr are written next to the main log:

- `<build>/Testing/draw_smoke/draw_smoke.stdout.log`
- `<build>/Testing/draw_smoke/draw_smoke.stderr.log`

Each successful DRAW wrapper run also writes a structured result file:

- `<build>/Testing/<test>/<test>.result.json`

The result JSON contains the DRAWEXE path, source Tcl path, exit code, success
token status, error-line summary, and a minimal `checkshape` status when present.
`scripts/verify_env.ps1` reads these files into the `draw_tests` section of the
environment report.

## Temporary Repro CTest

A user-provided `repro.tcl` can be registered as a temporary local CTest without
editing the generated build tree by hand:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\register_temp_draw_ctest.ps1 `
  -ReproTcl path\to\repro.tcl `
  -BuildDir out\build\debug `
  -TestName draw_temp_repro

ctest --test-dir out\build\debug\TemporaryDrawCTest -R draw_temp_repro --output-on-failure
```

The temporary test uses the same DRAW lookup and runtime setup as
`draw_smoke`. By default it checks only the DRAW process exit code; pass
`-SuccessToken TOKEN` when the repro script should emit a specific success
marker.

## Log Parsing

Parse a DRAW stdout, stderr, or combined log file:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\parse_draw_log.ps1 `
  -LogPath out\build\debug\Testing\draw_checkshape_smoke\draw_checkshape_smoke.stdout.log
```

The parser emits JSON with success tokens, error lines, and structured
`checkshape` status. It is intentionally small and dependency-free so it can be
reused by the UI, report generation, and later regression tooling.

## Repro Pack

Package a failed DRAW case and its logs:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\export_repro_pack.ps1 `
  -TclScript path\to\repro.tcl `
  -DrawLogDir out\build\debug\Testing\draw_temp_repro `
  -CaseId DRAW-FAILED-001
```

The pack contains `scripts/repro.tcl`, copied logs, and `manifest.json`. No CAD
data is copied implicitly; callers must explicitly add small test data when a
future repro requires it.

## Limitations

- These are environment smoke tests, not full geometry regression tests.
- It does not load CAD data.
- It does not run full testgrid or testdiff yet.
- It does not validate Qt UI behavior through CTest.
- It does not modify OCCT source code.
