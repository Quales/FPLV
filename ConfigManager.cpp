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

#include "stdafx.h"
#include "config.h"
#include "ConfigManager.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>

const char default_file_content[] = DEFAULT_FILE_CONTENT;

namespace
{
	std::string ToUpperCopy(const std::string& value)
	{
		std::string out = value;
		for (size_t i = 0; i < out.size(); ++i)
		{
			out[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[i])));
		}
		return out;
	}

	std::string ToLowerCopy(const std::string& value)
	{
		std::string out = value;
		for (size_t i = 0; i < out.size(); ++i)
		{
			out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
		}
		return out;
	}

	std::vector<std::string> ReadStringArray(const rapidjson::Value& value)
	{
		std::vector<std::string> result;
		if (!value.IsArray())
		{
			return result;
		}

		const auto& arr = value.GetArray();
		for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
		{
			if (arr[i].IsString())
			{
				result.push_back(arr[i].GetString());
			}
		}
		return result;
	}

	std::vector<int> ReadIntArray(const rapidjson::Value& value)
	{
		std::vector<int> result;
		if (!value.IsArray())
		{
			return result;
		}

		const auto& arr = value.GetArray();
		for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
		{
			if (arr[i].IsInt())
			{
				result.push_back(arr[i].GetInt());
			}
		}
		return result;
	}

	FPLV::AltitudeParity ParseParity(const rapidjson::Value& value)
	{
		if (!value.IsString())
		{
			return FPLV::AltitudeParity::Any;
		}

		std::string parity = ToLowerCopy(value.GetString());
		if (parity == "odd")
			return FPLV::AltitudeParity::Odd;
		if (parity == "even")
			return FPLV::AltitudeParity::Even;
		return FPLV::AltitudeParity::Any;
	}

	FPLV::DirectionAxis ParseAxis(const rapidjson::Value& value)
	{
		if (!value.IsString())
		{
			return FPLV::DirectionAxis::EastWest;
		}

		std::string axis = ToLowerCopy(value.GetString());
		if (axis == "north_south" || axis == "northsouth" || axis == "ns")
			return FPLV::DirectionAxis::NorthSouth;
		return FPLV::DirectionAxis::EastWest;
	}

	FPLV::ValidationColumn ParseColumn(const rapidjson::Value& value)
	{
		FPLV::ValidationColumn column;
		if (value.IsObject())
		{
			if (value.HasMember("code") && value["code"].IsString())
				column.code = value["code"].GetString();
			if (value.HasMember("title") && value["title"].IsString())
				column.title = value["title"].GetString();
			if (value.HasMember("width") && value["width"].IsInt())
				column.width = value["width"].GetInt();
			if (value.HasMember("centered") && value["centered"].IsBool())
				column.centered = value["centered"].GetBool();
		}
		if (column.code.empty())
			column.code = "status";
		if (column.title.empty())
			column.title = column.code;
		if (column.width <= 0)
			column.width = 8;
		return column;
	}

	FPLV::ValidationSide ParseSide(const rapidjson::Value& value, const char* default_name)
	{
		FPLV::ValidationSide side;
		side.name = default_name;
		if (!value.IsObject())
		{
			return side;
		}

		if (value.HasMember("name") && value["name"].IsString())
			side.name = value["name"].GetString();
		if (value.HasMember("route_markers"))
			side.route_markers = ReadStringArray(value["route_markers"]);
		if (value.HasMember("airport_markers"))
			side.airport_markers = ReadStringArray(value["airport_markers"]);
		if (value.HasMember("parity"))
			side.parity = ParseParity(value["parity"]);
		if (value.HasMember("parity_exceptions"))
			side.parity_exceptions = ReadIntArray(value["parity_exceptions"]);
		else if (value.HasMember("allowed_flight_levels"))
			side.parity_exceptions = ReadIntArray(value["allowed_flight_levels"]);
		return side;
	}

	FPLV::ValidationRule ParseRule(const rapidjson::Value& value)
	{
		FPLV::ValidationRule rule;
		if (!value.IsObject())
		{
			return rule;
		}

		if (value.HasMember("name") && value["name"].IsString())
			rule.name = value["name"].GetString();
		if (value.HasMember("axis"))
			rule.axis = ParseAxis(value["axis"]);
		if (value.HasMember("require_rvsm") && value["require_rvsm"].IsBool())
			rule.require_rvsm = value["require_rvsm"].GetBool();
		if (value.HasMember("min_cleared_altitude") && value["min_cleared_altitude"].IsInt())
			rule.min_cleared_altitude = value["min_cleared_altitude"].GetInt();
		if (value.HasMember("max_cleared_altitude") && value["max_cleared_altitude"].IsInt())
			rule.max_cleared_altitude = value["max_cleared_altitude"].GetInt();
		if (value.HasMember("enabled") && value["enabled"].IsBool())
			rule.enabled = value["enabled"].GetBool();

		const char* first_key = rule.axis == FPLV::DirectionAxis::NorthSouth ? "north" : "west";
		const char* second_key = rule.axis == FPLV::DirectionAxis::NorthSouth ? "south" : "east";
		rule.first_side = value.HasMember(first_key) ? ParseSide(value[first_key], first_key) : ParseSide(rapidjson::Value(), first_key);
		rule.second_side = value.HasMember(second_key) ? ParseSide(value[second_key], second_key) : ParseSide(rapidjson::Value(), second_key);

		if (rule.name.empty())
			rule.name = "validation-rule";
		return rule;
	}

	FPLV::RVSMRule ParseRvsmRule(const rapidjson::Value& value, FPLV::ConfigData& data)
	{
		FPLV::RVSMRule rule;
		if (!value.IsObject())
			return rule;

		if (value.HasMember("name") && value["name"].IsString())
			rule.name = value["name"].GetString();
		if (value.HasMember("description") && value["description"].IsString())
			rule.description = value["description"].GetString();
		if (value.HasMember("enabled") && value["enabled"].IsBool())
			rule.enabled = value["enabled"].GetBool();
		if (value.HasMember("default") && value["default"].IsBool())
			rule.is_default = value["default"].GetBool();

		if (value.HasMember("filters") && value["filters"].IsObject())
		{
			const rapidjson::Value& filters = value["filters"];
			if (filters.HasMember("airports"))
				rule.filters.airports = ReadStringArray(filters["airports"]);
			if (filters.HasMember("regex"))
				rule.filters.regex = ReadStringArray(filters["regex"]);
		}

		// Accept top-level filters for compatibility.
		if (value.HasMember("airports"))
			rule.filters.airports = ReadStringArray(value["airports"]);
		if (value.HasMember("regex"))
			rule.filters.regex = ReadStringArray(value["regex"]);

		FPLV::DirectionAxis axis = FPLV::DirectionAxis::EastWest;
		bool hasNorthSouth = false;
		bool hasEastWest = false;
		if (value.HasMember("direction") && value["direction"].IsObject())
		{
			const rapidjson::Value& direction = value["direction"];
			if (direction.HasMember("north") || direction.HasMember("south"))
			{
				axis = FPLV::DirectionAxis::NorthSouth;
				hasNorthSouth = true;
			}
			else if (direction.HasMember("east") || direction.HasMember("west"))
			{
				axis = FPLV::DirectionAxis::EastWest;
				hasEastWest = true;
			}

			if (direction.HasMember("north"))
			{
				rule.first_side.name = "North";
				if (direction["north"].IsString())
				{
					rule.first_side.parity = ParseParity(direction["north"]);
				}
				else if (direction["north"].IsObject())
				{
					const rapidjson::Value& north = direction["north"];
					if (north.HasMember("parity"))
						rule.first_side.parity = ParseParity(north["parity"]);
					if (north.HasMember("parity_exceptions"))
						rule.first_side.parity_exceptions = ReadIntArray(north["parity_exceptions"]);
					else if (north.HasMember("allowed_flight_levels"))
						rule.first_side.parity_exceptions = ReadIntArray(north["allowed_flight_levels"]);
				}
			}
			if (direction.HasMember("south"))
			{
				rule.second_side.name = "South";
				if (direction["south"].IsString())
				{
					rule.second_side.parity = ParseParity(direction["south"]);
				}
				else if (direction["south"].IsObject())
				{
					const rapidjson::Value& south = direction["south"];
					if (south.HasMember("parity"))
						rule.second_side.parity = ParseParity(south["parity"]);
					if (south.HasMember("parity_exceptions"))
						rule.second_side.parity_exceptions = ReadIntArray(south["parity_exceptions"]);
					else if (south.HasMember("allowed_flight_levels"))
						rule.second_side.parity_exceptions = ReadIntArray(south["allowed_flight_levels"]);
				}
			}
			if (direction.HasMember("west"))
			{
				rule.first_side.name = "West";
				if (direction["west"].IsString())
				{
					rule.first_side.parity = ParseParity(direction["west"]);
				}
				else if (direction["west"].IsObject())
				{
					const rapidjson::Value& west = direction["west"];
					if (west.HasMember("parity"))
						rule.first_side.parity = ParseParity(west["parity"]);
					if (west.HasMember("parity_exceptions"))
						rule.first_side.parity_exceptions = ReadIntArray(west["parity_exceptions"]);
					else if (west.HasMember("allowed_flight_levels"))
						rule.first_side.parity_exceptions = ReadIntArray(west["allowed_flight_levels"]);
				}
			}
			if (direction.HasMember("east"))
			{
				rule.second_side.name = "East";
				if (direction["east"].IsString())
				{
					rule.second_side.parity = ParseParity(direction["east"]);
				}
				else if (direction["east"].IsObject())
				{
					const rapidjson::Value& east = direction["east"];
					if (east.HasMember("parity"))
						rule.second_side.parity = ParseParity(east["parity"]);
					if (east.HasMember("parity_exceptions"))
						rule.second_side.parity_exceptions = ReadIntArray(east["parity_exceptions"]);
					else if (east.HasMember("allowed_flight_levels"))
						rule.second_side.parity_exceptions = ReadIntArray(east["allowed_flight_levels"]);
				}
			}

			if (direction.HasMember("north_parity_exceptions"))
				rule.first_side.parity_exceptions = ReadIntArray(direction["north_parity_exceptions"]);
			else if (direction.HasMember("north_allowed_flight_levels"))
				rule.first_side.parity_exceptions = ReadIntArray(direction["north_allowed_flight_levels"]);
			if (direction.HasMember("south_parity_exceptions"))
				rule.second_side.parity_exceptions = ReadIntArray(direction["south_parity_exceptions"]);
			else if (direction.HasMember("south_allowed_flight_levels"))
				rule.second_side.parity_exceptions = ReadIntArray(direction["south_allowed_flight_levels"]);
			if (direction.HasMember("west_parity_exceptions"))
				rule.first_side.parity_exceptions = ReadIntArray(direction["west_parity_exceptions"]);
			else if (direction.HasMember("west_allowed_flight_levels"))
				rule.first_side.parity_exceptions = ReadIntArray(direction["west_allowed_flight_levels"]);
			if (direction.HasMember("east_parity_exceptions"))
				rule.second_side.parity_exceptions = ReadIntArray(direction["east_parity_exceptions"]);
			else if (direction.HasMember("east_allowed_flight_levels"))
				rule.second_side.parity_exceptions = ReadIntArray(direction["east_allowed_flight_levels"]);
		}

		if (!hasNorthSouth && !hasEastWest)
		{
			// Default to east/west if direction map is incomplete.
			rule.first_side.name = "West";
			rule.second_side.name = "East";
		}
		else if (axis == FPLV::DirectionAxis::NorthSouth)
		{
			if (rule.first_side.name.empty())
				rule.first_side.name = "North";
			if (rule.second_side.name.empty())
				rule.second_side.name = "South";
		}
		else
		{
			if (rule.first_side.name.empty())
				rule.first_side.name = "West";
			if (rule.second_side.name.empty())
				rule.second_side.name = "East";
		}
		rule.axis = axis;

		if (rule.name.empty())
			rule.name = "rvsm-rule";

		for (size_t i = 0; i < rule.filters.regex.size(); ++i)
		{
			try
			{
				rule.compiled_regex.push_back(std::regex(rule.filters.regex[i], std::regex_constants::ECMAScript | std::regex_constants::icase));
			}
			catch (const std::regex_error& ex)
			{
				rule.enabled = false;
				rule.regex_valid = false;
				data.load_messages.push_back("Disabled rule '" + rule.name + "': invalid regex '" + rule.filters.regex[i] + "' (" + ex.what() + ")");
			}
		}

		return rule;
	}

	void ParseRulesArray(const rapidjson::Value& value, std::vector<FPLV::ValidationRule>& rules)
	{
		if (!value.IsArray())
			return;

		for (rapidjson::SizeType i = 0; i < value.Size(); ++i)
		{
			FPLV::ValidationRule rule = ParseRule(value[i]);
			if (!rule.name.empty())
				rules.push_back(rule);
		}
	}

	void ParseRvsmRulesArray(const rapidjson::Value& value, FPLV::ConfigData& data)
	{
		if (!value.IsArray())
			return;

		for (rapidjson::SizeType i = 0; i < value.Size(); ++i)
		{
			const rapidjson::Value& item = value[i];
			if (!item.IsObject())
				continue;

			if (item.HasMember("filters") || item.HasMember("direction") || item.HasMember("default") || item.HasMember("airports") || item.HasMember("regex"))
			{
				FPLV::RVSMRule rule = ParseRvsmRule(item, data);
				if (!rule.name.empty())
					data.rvsm_rules.push_back(rule);
			}
		}
	}
}

namespace FPLV
{
	ConfigManager::ConfigManager()
	{
	}

	ConfigManager::~ConfigManager()
	{
		this->Cleanup();
	}

	void ConfigManager::Init(std::string filepath)
	{
		file_path = filepath;
		this->Init();
	}

	void ConfigManager::Init(void)
	{
		std::ifstream file_stream(file_path.c_str(), std::ios::in);
		if (file_stream.fail())
		{
			throw std::runtime_error("Failed to open JSON file.");
		}

		std::stringstream file_sstream;
		file_sstream << file_stream.rdbuf();
		file_stream.close();

		json_document.Parse(file_sstream.str().c_str());
		if (json_document.HasParseError())
		{
			std::string error_str = "Failed to parse the JSON file. Error code: ";
			error_str += std::to_string(json_document.GetParseError());
			throw std::runtime_error(error_str);
		}

		if (!json_document.IsObject())
		{
			throw std::runtime_error("Config root must be an object.");
		}

		data.Cleanup();

		if (json_document.HasMember("plugin_name") && json_document["plugin_name"].IsString())
			data.plugin_name = json_document["plugin_name"].GetString();
		if (json_document.HasMember("list_name") && json_document["list_name"].IsString())
			data.list_name = json_document["list_name"].GetString();
		if (json_document.HasMember("debug_enabled") && json_document["debug_enabled"].IsBool())
			data.debug_enabled = json_document["debug_enabled"].GetBool();

		if (json_document.HasMember("columns"))
		{
			const rapidjson::Value& columns = json_document["columns"];
			if (columns.IsArray())
			{
				for (rapidjson::SizeType i = 0; i < columns.Size(); ++i)
				{
					data.columns.push_back(ParseColumn(columns[i]));
				}
			}
		}

		if (data.columns.empty())
		{
			ValidationColumn column;
			column.code = "status";
			column.title = "VAL";
			column.width = 6;
			column.centered = true;
			data.columns.push_back(column);
		}

		if (json_document.HasMember("rules") && json_document["rules"].IsArray())
		{
			const rapidjson::Value& rules = json_document["rules"];
			for (rapidjson::SizeType i = 0; i < rules.Size(); ++i)
			{
				const rapidjson::Value& item = rules[i];
				if (!item.IsObject())
					continue;

				if (item.HasMember("filters") || item.HasMember("direction") || item.HasMember("default") || item.HasMember("airports") || item.HasMember("regex"))
				{
					FPLV::RVSMRule rule = ParseRvsmRule(item, data);
					if (!rule.name.empty())
						data.rvsm_rules.push_back(rule);
					continue;
				}

				ValidationRule rule = ParseRule(item);
				if (!rule.name.empty())
					data.rules.push_back(rule);
			}
		}
		if (json_document.HasMember("vfr_rules"))
			ParseRulesArray(json_document["vfr_rules"], data.vfr_rules);

		if (data.rules.empty())
		{
			ValidationRule fallback;
			fallback.name = "West odd / East even";
			fallback.axis = DirectionAxis::EastWest;
			fallback.require_rvsm = true;
			fallback.first_side.name = "West";
			fallback.first_side.route_markers.push_back("WEST");
			fallback.first_side.parity = AltitudeParity::Odd;
			fallback.second_side.name = "East";
			fallback.second_side.route_markers.push_back("EAST");
			fallback.second_side.parity = AltitudeParity::Even;
			data.rules.push_back(fallback);
		}

		_ready = true;
	}

	void ConfigManager::Cleanup(void)
	{
		_ready = false;
		json_document.SetNull();
		data.Cleanup();
	}

	void ConfigManager::GenerateConfigFile(std::string filepath)
	{
		std::ofstream o_fs(filepath.c_str(), std::ios::out | std::ios::trunc);
		o_fs << default_file_content;
		o_fs.close();
	}
}
