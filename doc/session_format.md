# `.occtdbg` session format

JSON UTF-8 files used by **OCCTDebug** (Milestone 4). Extension suggestion: `.occtdbg`.

## Top-level object

| Field | Type | Meaning |
|-------|------|---------|
| `format` | string | Must be `"occtdbg"`. |
| `version` | number | Currently `1`. |
| `createdAt` | string | UTC timestamp (ISO 8601, e.g. from `QDateTime::ISODateWithMs`). |
| `problem` | object | `ProblemContext`: title, category, description, build metadata, parameters. |
| `inputs` | array | Input files with portable paths (often relative to the session file). |
| `operations` | array | Reserved for future algorithm steps (currently empty `[]`). |
| `ui` | object | UI hints, e.g. `selectedShapeId`. |
| `diagnostics` | array | Snapshot of `DiagnosticFinding` objects from the last run (optional). |

## `problem`

- `title`, `description` — strings.
- `category` — one of: `Unknown`, `Boolean`, `Projection`, `Classification`, `Topology`, `Tolerance`, `Meshing`, `HLR`, `Performance`, `Crash`.
- `occtVersion`, `compiler`, `buildType` — captured when the session was saved.
- `inputFiles` — array of strings (legacy mirror of paths; loaders should prefer `inputs`).
- `parameters` — object of string key/value pairs.

## `inputs[]`

Each element:

- `path` — absolute or relative to the directory containing the `.occtdbg` file.
- `type` — e.g. `"brep"`.
- `role` — e.g. `"primary"`.

## `diagnostics[]`

Each element mirrors `DiagnosticFinding`:

- `ruleId`, `severity` (`Info` / `Warning` / `Error` / `Critical`), `title`, `description`
- `relatedShapeIds` — integers matching `ShapeDocument` node ids
- `evidence`, `possibleCauses`, `suggestions` — arrays of strings

## Versioning

Bump `version` when removing or renaming fields. Older writers should keep unknown fields if implementing round-trip in the future.

Implementation: `io/SessionSerializer.cpp`, model: `core/DebugSession.h`.

## Minimal repro folder (Export)

**Export → Minimal repro folder…** writes into a user-chosen directory:

- `case/input.brep` — copy of the primary loaded model  
- `debug.occtdbg` — session v1 with `inputs[0].path` = `case/input.brep` (portable)  
- `README.txt` — short open instructions  

Opening `debug.occtdbg` resolves `case/input.brep` relative to the session file directory. Implementation: `io/ReproPackageExporter.cpp`.
