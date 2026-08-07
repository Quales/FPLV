#pragma once
#include "config.h"

#define DEFAULT_FILE_CONTENT R"json(
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
    },
    {
      "name": "North odd / South even",
      "axis": "north_south",
      "require_rvsm": true,
      "north": {
        "name": "North",
        "route_markers": ["NORTH", "N"],
        "airport_markers": ["N"],
        "parity": "odd"
      },
      "south": {
        "name": "South",
        "route_markers": ["SOUTH", "S"],
        "airport_markers": ["S"],
        "parity": "even"
      }
    }
  ],
  "vfr_rules": [
    {
      "name": "VFR east/west",
      "axis": "east_west",
      "require_rvsm": false,
      "west": {
        "name": "West",
        "route_markers": ["WEST", "W"],
        "airport_markers": ["L"],
        "parity": "any"
      },
      "east": {
        "name": "East",
        "route_markers": ["EAST", "E"],
        "airport_markers": ["E"],
        "parity": "any"
      }
    },
    {
      "name": "VFR north/south",
      "axis": "north_south",
      "require_rvsm": false,
      "north": {
        "name": "North",
        "route_markers": ["NORTH", "N"],
        "airport_markers": ["N"],
        "parity": "any"
      },
      "south": {
        "name": "South",
        "route_markers": ["SOUTH", "S"],
        "airport_markers": ["S"],
        "parity": "any"
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
)json"
