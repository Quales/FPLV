# FPLV (EuroScope Flight Plan Validator)

[Download latest FPLV.dll built](https://github.com/Quales/FPLV/releases/latest)
[Download latest FPLV.dll release](https://github.com/Quales/FPLV/releases/latest/download/FPLV.dll)

FPLV is a EuroScope plugin that validates flight plans against configurable rules (RVSM, direction, flight level parity, altitude range) and displays results directly in a custom flight plan list.

## What it does

- Loads rules from `FPLV_rules.json` located next to the plugin DLL.
- Evaluates each flight plan and outputs:
  - **Status** (`OK`, `FL`, `ERR`, `N/A`)
  - **Direction** (e.g. `W`, `E`, `N`, `S`, `??`)
  - **Issues** (compact or detailed)
- Colors list values:
  - Green: valid (`OK`)
  - Orange: single FL-related issue
  - Red: multiple/other errors
- Supports runtime debug messages for per-rule evaluation tracing.

## Rule engine summary

For each enabled rule, FPLV checks:

1. RVSM requirement (`require_rvsm`)
2. Optional altitude limits (`min_cleared_altitude`, `max_cleared_altitude`)
3. Direction side detection:
   - First from extracted route coordinates (`GetExtractedRoute`)
   - Fallback to route/airport markers
4. Flight level parity on normalized level:
   - `35000 -> 35`, `36000 -> 36`, etc.
   - then odd/even check based on the selected side

## Configuration file

File name is fixed by `config.h`:

- `FPLV_rules.json`

If missing, the plugin generates a default config automatically.

### Example config

```json
{
  "plugin_name": "FPLV",
  "list_name": "FPLV Validation",
  "debug_enabled": false,
  "columns": [
    { "code": "status", "title": "VAL", "width": 6, "centered": true },
    { "code": "direction", "title": "DIR", "width": 8, "centered": true },
    { "code": "issues", "title": "ISSUES", "width": 18, "centered": false }
  ],
  "rules": [
    {
      "name": "West even / East odd",
      "axis": "east_west",
      "require_rvsm": true,
      "west": {
        "name": "West",
        "route_markers": ["WEST", "W"],
        "airport_markers": ["L"],
        "parity": "even"
      },
      "east": {
        "name": "East",
        "route_markers": ["EAST", "E"],
        "airport_markers": ["E"],
        "parity": "odd"
      }
    }
  ],
  "radio_callsigns": {
    "config": {
      "load_from_ese": false,
      "path_to_ese": ".\\"
    },
    "custom_callsigns": {}
  }
}
```

### Supported fields

- Root:
  - `plugin_name` (string)
  - `list_name` (string)
  - `debug_enabled` (bool)
  - `columns` (array)
  - `rules` (array)
  - `radio_callsigns` (object)
- Rule:
  - `name` (string)
  - `axis`: `east_west` or `north_south`
  - `require_rvsm` (bool)
  - `min_cleared_altitude` (int, optional)
  - `max_cleared_altitude` (int, optional)
  - `enabled` (bool, optional)
  - side blocks:
    - East/West rules: `west`, `east`
    - North/South rules: `north`, `south`
- Side:
  - `name` (string)
  - `route_markers` (string array)
  - `airport_markers` (string array)
  - `parity`: `odd`, `even`, `any`

## EuroScope commands

- `.fplv reload` - reload JSON config
- `.fplv debug` - toggle runtime debug
- `.fplv debug on`
- `.fplv debug off`

Clicking the **status** column function toggles verbose issue text.

## License

GNU GPLv3 (see file headers in source).
