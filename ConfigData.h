/*
	Copyright(C) 2026-2026 Arthur L (Quales)

	This program is free software : you can redistribute it and /or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.If not, see < https://www.gnu.org/licenses/>.
*/

#pragma once
#ifndef _CONFIGDATA_H_
#define _CONFIGDATA_H_

#include <string>
#include <vector>

namespace FPLV
{
	typedef struct
	{
		std::string callsign;
		std::string icao;
		std::string frequency;
		std::string radio_callsign;
	} RadioCallsignElement_t;

	typedef std::vector<RadioCallsignElement_t> RadioCallsigns_t;

	enum class DirectionAxis
	{
		EastWest,
		NorthSouth
	};

	enum class AltitudeParity
	{
		Any,
		Odd,
		Even
	};

	struct ValidationColumn
	{
		std::string code;
		std::string title;
		int width = 8;
		bool centered = false;
	};

	struct ValidationSide
	{
		std::string name;
		std::vector<std::string> route_markers;
		std::vector<std::string> airport_markers;
		AltitudeParity parity = AltitudeParity::Any;
	};

	struct ValidationRule
	{
		std::string name;
		DirectionAxis axis = DirectionAxis::EastWest;
		bool require_rvsm = false;
		int min_cleared_altitude = 0;
		int max_cleared_altitude = 0;
		ValidationSide first_side;
		ValidationSide second_side;
		bool enabled = true;
	};

	struct ValidationIssue
	{
		std::string code;
		std::string message;
	};

	struct ValidationResult
	{
		bool valid = true;
		bool fl_issue = false;
		std::string summary;
		std::string direction;
		std::string details;
		std::string debug;
		std::vector<ValidationIssue> issues;
	};

	class ConfigData
	{
	public:
		std::string plugin_name;
		std::string list_name;
		bool debug_enabled = false;
		std::vector<ValidationColumn> columns;
		std::vector<ValidationRule> rules;
		std::vector<ValidationRule> vfr_rules;
		RadioCallsigns_t RadioCallsigns;
		bool loaded_from_ese = false;

		ConfigData();
		void Cleanup();
	};
}

#endif
