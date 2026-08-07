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
#include "ESEHandler.h"
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
				ValidationRule rule = ParseRule(rules[i]);
				if (!rule.name.empty())
				{
					data.rules.push_back(rule);
				}
			}
		}

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

	void ConfigManager::LoadRadioCallsigns(void)
	{
		data.RadioCallsigns.clear();
		if (!json_document.IsObject() || !json_document.HasMember("radio_callsigns"))
			return;

		const rapidjson::Value& radio = json_document["radio_callsigns"];
		if (!radio.IsObject())
			return;

		if (radio.HasMember("config") && radio["config"].IsObject())
		{
			const rapidjson::Value& cfg = radio["config"];
			if (cfg.HasMember("load_from_ese") && cfg["load_from_ese"].IsBool() && cfg["load_from_ese"].GetBool())
			{
				std::string relative_path = ".\\";
				if (cfg.HasMember("path_to_ese") && cfg["path_to_ese"].IsString())
					relative_path = cfg["path_to_ese"].GetString();

				std::string absolute_path;
				if (relative_path.size() > 1 && relative_path[1] == ':')
					absolute_path = relative_path;
				else
					absolute_path = this->file_path.substr(0, this->file_path.find_last_of('\\') + 1) + relative_path;

				if (ESEHandler::LocateESEFile(absolute_path) && ESEHandler::ParsePositions() > 0)
				{
					data.loaded_from_ese = true;
					ESEHandler::GetRadioCallsigns(data.RadioCallsigns);
					return;
				}
			}
		}

		if (!radio.HasMember("custom_callsigns") || !radio["custom_callsigns"].IsObject())
			return;

		const rapidjson::Value& obj = radio["custom_callsigns"];
		for (rapidjson::Value::ConstMemberIterator it = obj.MemberBegin(); it != obj.MemberEnd(); ++it)
		{
			if (!it->name.IsString() || !it->value.IsString())
				continue;

			RadioCallsignElement_t element;
			element.callsign = it->name.GetString();
			element.radio_callsign = it->value.GetString();
			element.icao = element.callsign.substr(0, element.callsign.find_first_of('_'));
			data.RadioCallsigns.push_back(element);
		}
	}

	void ConfigManager::FindRadioCallsign(std::string callsign, std::string frequency, std::string& radio_callsign)
	{
		radio_callsign = callsign;
		for (const auto& it : data.RadioCallsigns)
		{
			if (data.loaded_from_ese)
			{
				if (it.icao == callsign.substr(0, callsign.find_first_of('_')) && it.frequency == frequency)
				{
					radio_callsign = it.radio_callsign;
					return;
				}
				continue;
			}

			try
			{
				std::regex rgx(it.callsign);
				if (!std::regex_search(callsign, rgx))
					continue;
				if (!it.frequency.empty() && it.frequency != frequency)
					continue;
				radio_callsign = it.radio_callsign;
				return;
			}
			catch (...)
			{
				if (ToUpperCopy(it.callsign) == ToUpperCopy(callsign))
				{
					radio_callsign = it.radio_callsign;
					return;
				}
			}
		}
	}

	void ConfigManager::GenerateConfigFile(std::string filepath)
	{
		std::ofstream o_fs(filepath.c_str(), std::ios::out | std::ios::trunc);
		o_fs << default_file_content;
		o_fs.close();
	}
}
