#pragma once
#include "config.h"

#define DEFAULT_FILE_CONTENT R"json(
{
  "plugin_name": "FPLV",
  "list_name": "Flight plan validation",
  "debug_enabled": false,
  "columns": [
    { "code": "status", "title": "VAL", "width": 6, "centered": true },
    { "code": "direction", "title": "DIR", "width": 8, "centered": true },
    { "code": "issues", "title": "ISSUES", "width": 18, "centered": false }
  ],
  "rules": [
    {
      "name": "Europe",
      "description": "Europe north/south RVSM rule",
      "filters": {
        "airports": ["LFPG", "LFBO", "EHAM"],
        "regex": ["^LF[A-Z]{2}$", "^EH[A-Z]{2}$"]
      },
      "direction": {
        "north": "odd",
        "south": "even"
      },
      "enabled": true
    },
    {
      "name": "Caribbean",
      "description": "Caribbean east/west RVSM rule",
      "filters": {
        "airports": ["TNCC", "TNCM"],
        "regex": ["^TN[A-Z]{2}$"]
      },
      "direction": {
        "east": "odd",
        "west": "even",
        "west_allowed_flight_levels": [430, 470, 510]
      },
      "enabled": true
    },
    {
      "name": "USA",
      "description": "United States east/west RVSM rule",
      "filters": {
        "regex": ["^K[A-Z]{3}$"]
      },
      "direction": {
        "east": "odd",
        "west": "even",
        "west_allowed_flight_levels": [430, 470, 510]
      },
      "enabled": true
    },
    {
      "name": "Default",
      "description": "Fallback rule",
      "filters": {
        "regex": [".*"]
      },
      "direction": {
        "east": "odd",
        "west": "even",
        "west_allowed_flight_levels": [430, 470, 510]
      },
      "default": true,
      "enabled": true
    }
  ],
  "vfr_rules": []
}
)json"
