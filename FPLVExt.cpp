#include "stdafx.h"
#include "config.h"
#include "FPLVExt.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sstream>
#include <stdexcept>

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace
{
	const int TAG_ITEM_STATUS = 7001;
	const int TAG_ITEM_DIRECTION = 7002;
	const int TAG_ITEM_ISSUES = 7003;
	const int TAG_FUNCTION_TOGGLE_VERBOSE = 7101;

	std::string ToUpperCopy(const std::string& value)
	{
		std::string out = value;
		for (size_t i = 0; i < out.size(); ++i)
			out[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[i])));
		return out;
	}

	std::vector<std::string> TokenizeRoute(const std::string& text)
	{
		std::vector<std::string> tokens;
		std::string current;
		for (size_t i = 0; i < text.size(); ++i)
		{
			const unsigned char ch = static_cast<unsigned char>(text[i]);
			if (std::isalnum(ch))
			{
				current.push_back(static_cast<char>(std::toupper(ch)));
				continue;
			}
			if (!current.empty())
			{
				tokens.push_back(current);
				current.clear();
			}
		}
		if (!current.empty())
			tokens.push_back(current);
		return tokens;
	}

	bool MarkerMatchesToken(const std::string& token, const std::string& marker)
	{
		if (token == marker)
			return true;
		if (marker.size() > 2 && token.find(marker) != std::string::npos)
			return true;
		return false;
	}

	int CountMatches(const std::vector<std::string>& tokens, const std::vector<std::string>& markers, const std::string& text)
	{
		int count = 0;
		for (size_t i = 0; i < markers.size(); ++i)
		{
			const std::string marker = ToUpperCopy(markers[i]);
			bool found = false;
			for (size_t j = 0; j < tokens.size(); ++j)
			{
				if (MarkerMatchesToken(tokens[j], marker))
				{
					found = true;
					break;
				}
			}
			if (!found && !marker.empty() && text.find(marker) != std::string::npos)
				found = true;
			if (found)
				++count;
		}
		return count;
	}

	int AltitudeToFlightLevel(int finalAltitude, bool& isSpecial)
	{
		isSpecial = false;
		if (finalAltitude <= 0 || finalAltitude == 1 || finalAltitude == 2)
		{
			isSpecial = true;
			return 0;
		}

		if (finalAltitude >= 1000)
			return finalAltitude / 1000;
		if (finalAltitude >= 100)
			return finalAltitude / 100;
		return finalAltitude;
	}

	std::string TrimToTag(const std::string& value)
	{
		return value.size() <= 15 ? value : value.substr(0, 15);
	}

	const FPLV::ValidationSide* DetermineSideFromCoordinates(
		EuroScopePlugIn::CFlightPlan FlightPlan,
		const FPLV::ValidationRule& rule,
		std::ostringstream& debugLine)
	{
		EuroScopePlugIn::CFlightPlanExtractedRoute extractedRoute = FlightPlan.GetExtractedRoute();
		const int points = extractedRoute.GetPointsNumber();
		if (points < 2)
		{
			debugLine << " source=markers";
			return NULL;
		}

		const EuroScopePlugIn::CPosition start = extractedRoute.GetPointPosition(0);
		const EuroScopePlugIn::CPosition end = extractedRoute.GetPointPosition(points - 1);
		const double deltaLat = end.m_Latitude - start.m_Latitude;
		const double deltaLon = end.m_Longitude - start.m_Longitude;

		debugLine << " source=coords";
		debugLine << " points=" << points;
		debugLine << " dlat=" << deltaLat;
		debugLine << " dlon=" << deltaLon;

		if (rule.axis == FPLV::DirectionAxis::EastWest)
		{
			if (deltaLon > 0.0)
				return &rule.second_side;
			if (deltaLon < 0.0)
				return &rule.first_side;
		}
		else
		{
			if (deltaLat > 0.0)
				return &rule.first_side;
			if (deltaLat < 0.0)
				return &rule.second_side;
		}

		debugLine << " coord=flat";
		return NULL;
	}

	std::string JoinIssues(const std::vector<FPLV::ValidationIssue>& issues)
	{
		std::ostringstream builder;
		for (size_t i = 0; i < issues.size(); ++i)
		{
			if (i > 0)
				builder << "; ";
			builder << issues[i].code << "=" << issues[i].message;
		}
		return builder.str();
	}
}

FPLVExt::FPLVExt() : EuroScopePlugIn::CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, PLUGIN_NAME, PLUGIN_VERSION, "Arthur L", "GNU GPLv3")
{
	LoadConfigOrCreateDefault();
	RegisterValidationColumns();
}

FPLVExt::~FPLVExt()
{
	config.Cleanup();
}

void FPLVExt::LoadConfigOrCreateDefault()
{
	char modulePath[MAX_PATH] = { 0 };
	if (GetModuleFileNameA((HMODULE)&__ImageBase, modulePath, MAX_PATH) == 0)
	{
		throw std::runtime_error("Failed to locate plugin module path.");
	}

	std::filesystem::path configPath = std::filesystem::path(modulePath).parent_path() / CONFIG_FILENAME;
	try
	{
		config.Init(configPath.string());
	}
	catch (const std::runtime_error&)
	{
		config.GenerateConfigFile(configPath.string());
		config.Init(configPath.string());
	}

	config.LoadRadioCallsigns();
	lastDebugMessages.clear();
}

bool FPLVExt::IsDebugEnabled() const
{
	return runtimeDebug || config.Data().debug_enabled;
}

void FPLVExt::LogDebugMessage(const std::string& message)
{
	if (!IsDebugEnabled())
		return;
	DisplayUserMessage("Message", PLUGIN_NAME, message.c_str(), true, true, false, true, false);
}

std::string FPLVExt::GetRouteText(EuroScopePlugIn::CFlightPlan FlightPlan) const
{
	if (!FlightPlan.IsValid())
		return "";

	EuroScopePlugIn::CFlightPlanData fpData = FlightPlan.GetFlightPlanData();
	std::string route = fpData.GetRoute() != NULL ? fpData.GetRoute() : "";
	std::string origin = fpData.GetOrigin() != NULL ? fpData.GetOrigin() : "";
	std::string destination = fpData.GetDestination() != NULL ? fpData.GetDestination() : "";
	std::string combined = route;
	if (!origin.empty())
		combined += " " + origin;
	if (!destination.empty())
		combined += " " + destination;
	return ToUpperCopy(combined);
}

FPLV::ValidationResult FPLVExt::EvaluateFlightPlanForRule(EuroScopePlugIn::CFlightPlan FlightPlan, const FPLV::ValidationRule& rule) const
{
	FPLV::ValidationResult result;
	if (!FlightPlan.IsValid() || !rule.enabled)
		return result;

	EuroScopePlugIn::CFlightPlanData fpData = FlightPlan.GetFlightPlanData();
	const std::string routeText = GetRouteText(FlightPlan);
	const std::vector<std::string> tokens = TokenizeRoute(routeText);
	const std::string origin = fpData.GetOrigin() != NULL ? ToUpperCopy(fpData.GetOrigin()) : "";
	const std::string destination = fpData.GetDestination() != NULL ? ToUpperCopy(fpData.GetDestination()) : "";
	const int finalAltitude = fpData.GetFinalAltitude();
	bool specialAltitude = false;
	const int flightLevel = AltitudeToFlightLevel(finalAltitude, specialAltitude);

	std::ostringstream debugLine;
	debugLine << "rule=" << rule.name;
	debugLine << " route=" << routeText;
	debugLine << " final=" << finalAltitude;
	if (finalAltitude >= 100)
		debugLine << " fl=" << flightLevel;
	debugLine << " rvsm=" << (fpData.IsRvsm() ? "yes" : "no");

	if (rule.require_rvsm && !fpData.IsRvsm())
		result.issues.push_back(FPLV::ValidationIssue{ "RVSM", "RVSM required" });

	if (rule.min_cleared_altitude > 0 && finalAltitude > 0 && finalAltitude < rule.min_cleared_altitude)
		result.issues.push_back(FPLV::ValidationIssue{ "ALT", "Altitude too low" });

	if (rule.max_cleared_altitude > 0 && finalAltitude > rule.max_cleared_altitude)
		result.issues.push_back(FPLV::ValidationIssue{ "ALT", "Altitude too high" });

	const int firstScore = CountMatches(tokens, rule.first_side.route_markers, routeText) + CountMatches(tokens, rule.first_side.airport_markers, origin + " " + destination);
	const int secondScore = CountMatches(tokens, rule.second_side.route_markers, routeText) + CountMatches(tokens, rule.second_side.airport_markers, origin + " " + destination);
	debugLine << " first=" << firstScore;
	debugLine << " second=" << secondScore;

	const FPLV::ValidationSide* activeSide = DetermineSideFromCoordinates(FlightPlan, rule, debugLine);
	if (activeSide == NULL)
	{
		if (firstScore > secondScore && firstScore > 0)
		{
			activeSide = &rule.first_side;
			debugLine << " source=markers side=first";
		}
		else if (secondScore > firstScore && secondScore > 0)
		{
			activeSide = &rule.second_side;
			debugLine << " source=markers side=second";
		}
	}

	if (activeSide == NULL)
	{
		result.issues.push_back(FPLV::ValidationIssue{ "DIR", "Direction unknown" });
		debugLine << " side=?";
	}
	else
	{
		result.direction = activeSide->name;
		debugLine << " side=" << activeSide->name;
		if (!specialAltitude && activeSide->parity != FPLV::AltitudeParity::Any)
		{
			if ((activeSide->parity == FPLV::AltitudeParity::Odd && (flightLevel % 2) == 0) ||
				(activeSide->parity == FPLV::AltitudeParity::Even && (flightLevel % 2) != 0))
			{
				result.fl_issue = true;
				result.issues.push_back(FPLV::ValidationIssue{ "FL", "Level parity mismatch" });
			}
		}
		else if (specialAltitude)
		{
			result.fl_issue = true;
			result.issues.push_back(FPLV::ValidationIssue{ "FL", "Flight level missing" });
		}
	}

	result.valid = result.issues.empty();
	if (result.valid)
	{
		result.summary = "OK";
		result.details = activeSide != NULL ? activeSide->name : "";
		debugLine << " result=OK";
		result.debug = debugLine.str();
		return result;
	}

	result.summary = result.fl_issue && result.issues.size() == 1 ? "FL" : "ERR";
	std::ostringstream details;
	for (size_t i = 0; i < result.issues.size(); ++i)
	{
		if (i > 0)
			details << ",";
		details << result.issues[i].code;
	}
	result.details = details.str();
	debugLine << " result=" << result.summary;
	debugLine << " issues=" << JoinIssues(result.issues);
	result.debug = debugLine.str();
	return result;
}

FPLV::ValidationResult FPLVExt::EvaluateFlightPlan(EuroScopePlugIn::CFlightPlan FlightPlan) const
{
	FPLV::ValidationResult combined;
	if (!FlightPlan.IsValid())
	{
		combined.valid = false;
		combined.summary = "N/A";
		return combined;
	}

	const char* callsign = FlightPlan.GetCallsign();
	std::ostringstream debugStream;
	debugStream << "FPL=" << (callsign != NULL ? callsign : "?");
	debugStream << " final=" << FlightPlan.GetFlightPlanData().GetFinalAltitude();
	debugStream << " debug=" << (IsDebugEnabled() ? "on" : "off");

	const std::vector<FPLV::ValidationRule>& rules = config.Data().rules;
	for (size_t i = 0; i < rules.size(); ++i)
	{
		FPLV::ValidationResult current = EvaluateFlightPlanForRule(FlightPlan, rules[i]);
		debugStream << " || " << current.debug;
		if (!current.direction.empty() && combined.direction.empty())
			combined.direction = current.direction;
		if (!current.valid)
		{
			combined.valid = false;
			if (current.fl_issue)
				combined.fl_issue = true;
			combined.issues.insert(combined.issues.end(), current.issues.begin(), current.issues.end());
		}
	}

	if (combined.issues.empty())
	{
		combined.summary = "OK";
		combined.debug = debugStream.str();
		return combined;
	}

	combined.summary = combined.fl_issue && combined.issues.size() == 1 ? "FL" : "ERR";
	std::ostringstream details;
	for (size_t i = 0; i < combined.issues.size(); ++i)
	{
		if (i > 0)
			details << ",";
		details << combined.issues[i].code;
	}
	combined.details = details.str();
	combined.debug = debugStream.str();
	return combined;
}

std::string FPLVExt::BuildCompactSummary(const FPLV::ValidationResult& result) const
{
	return result.summary.empty() ? "N/A" : result.summary;
}

std::string FPLVExt::BuildIssueText(const FPLV::ValidationResult& result) const
{
	if (result.valid)
		return "OK";

	if (verboseDetails)
	{
		std::ostringstream builder;
		for (size_t i = 0; i < result.issues.size(); ++i)
		{
			if (i > 0)
				builder << "; ";
			builder << result.issues[i].code << ": " << result.issues[i].message;
		}
		return builder.str();
	}

	if (result.issues.empty())
		return result.details;
	if (result.issues.size() == 1)
		return result.issues[0].code + std::string(": ") + result.issues[0].message;

	std::ostringstream builder;
	for (size_t i = 0; i < result.issues.size(); ++i)
	{
		if (i > 0)
			builder << "; ";
		builder << result.issues[i].code << ": " << result.issues[i].message;
	}
	return builder.str();
}

std::string FPLVExt::BuildDirectionText(const FPLV::ValidationResult& result) const
{
	if (result.direction.empty())
		return "??";
	std::string dir = ToUpperCopy(result.direction);
	return dir.substr(0, 1);
}

void FPLVExt::ApplyTagValue(const FPLV::ValidationResult& result, const std::string& value, char sItemString[16], int* pColorCode, COLORREF* pRGB) const
{
	strncpy_s(sItemString, 16, TrimToTag(value).c_str(), _TRUNCATE);
	if (pColorCode == NULL || pRGB == NULL)
		return;

	if (result.valid)
	{
		*pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
		*pRGB = RGB(0, 170, 0);
		return;
	}

	if (result.fl_issue && result.issues.size() == 1)
	{
		*pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
		*pRGB = RGB(255, 140, 0);
		return;
	}

	*pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
	*pRGB = RGB(210, 40, 40);
}

void FPLVExt::RegisterValidationColumns()
{
	RegisterTagItemType("FPLV status", TAG_ITEM_STATUS);
	RegisterTagItemType("FPLV direction", TAG_ITEM_DIRECTION);
	RegisterTagItemType("FPLV issues", TAG_ITEM_ISSUES);
	RegisterTagItemFunction("Toggle validation detail", TAG_FUNCTION_TOGGLE_VERBOSE);

	validationList = RegisterFpList(config.Data().list_name.c_str());
	if (!validationList.IsValid())
		return;

	validationList.DeleteAllColumns();
	const std::vector<FPLV::ValidationColumn>& columns = config.Data().columns;
	for (size_t i = 0; i < columns.size(); ++i)
	{
		const FPLV::ValidationColumn& column = columns[i];
		int itemCode = TAG_ITEM_STATUS;
		int leftFunction = 0;
		if (column.code == "direction")
			itemCode = TAG_ITEM_DIRECTION;
		else if (column.code == "issues")
			itemCode = TAG_ITEM_ISSUES;
		else if (column.code == "status")
			itemCode = TAG_ITEM_STATUS;
		if (column.code == "status")
			leftFunction = TAG_FUNCTION_TOGGLE_VERBOSE;

		validationList.AddColumnDefinition(
			column.title.c_str(),
			column.width,
			column.centered,
			PLUGIN_NAME,
			itemCode,
			PLUGIN_NAME,
			leftFunction,
			NULL,
			0);
	}
}

void FPLVExt::OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan,
	EuroScopePlugIn::CRadarTarget RadarTarget,
	int ItemCode,
	int TagData,
	char sItemString[16],
	int* pColorCode,
	COLORREF* pRGB,
	double* pFontSize)
{
	if (pFontSize != NULL)
		*pFontSize = 0.0;

	if (!FlightPlan.IsValid() && RadarTarget.IsValid())
		FlightPlan = RadarTarget.GetCorrelatedFlightPlan();

	if (!FlightPlan.IsValid())
	{
		strncpy_s(sItemString, 16, "N/A", _TRUNCATE);
		if (pColorCode != NULL && pRGB != NULL)
		{
			*pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
			*pRGB = RGB(140, 140, 140);
		}
		return;
	}

	FPLV::ValidationResult result = EvaluateFlightPlan(FlightPlan);
	if (ItemCode == TAG_ITEM_STATUS)
	{
		const char* callsign = FlightPlan.GetCallsign();
		const std::string key = callsign != NULL ? callsign : "";
		std::map<std::string, std::string>::const_iterator it = lastDebugMessages.find(key);
		if (IsDebugEnabled() && (it == lastDebugMessages.end() || it->second != result.debug))
		{
			lastDebugMessages[key] = result.debug;
			LogDebugMessage(result.debug);
		}
	}

	if (TagData == EuroScopePlugIn::TAG_DATA_UNCORRELATED_RADAR)
	{
		strncpy_s(sItemString, 16, "NO FPL", _TRUNCATE);
		if (pColorCode != NULL && pRGB != NULL)
		{
			*pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
			*pRGB = RGB(140, 140, 140);
		}
		return;
	}

	switch (ItemCode)
	{
	case TAG_ITEM_STATUS:
		ApplyTagValue(result, BuildCompactSummary(result), sItemString, pColorCode, pRGB);
		return;
	case TAG_ITEM_DIRECTION:
		ApplyTagValue(result, BuildDirectionText(result), sItemString, pColorCode, pRGB);
		return;
	case TAG_ITEM_ISSUES:
		ApplyTagValue(result, BuildIssueText(result), sItemString, pColorCode, pRGB);
		return;
	default:
		strncpy_s(sItemString, 16, "", _TRUNCATE);
		if (pColorCode != NULL)
			*pColorCode = EuroScopePlugIn::TAG_COLOR_DEFAULT;
		return;
	}
}

void FPLVExt::OnRefreshFpListContent(EuroScopePlugIn::CFlightPlanList AcList)
{
	EuroScopePlugIn::CRadarTarget target = RadarTargetSelectFirst();
	while (target.IsValid())
	{
		EuroScopePlugIn::CFlightPlan flightPlan = target.GetCorrelatedFlightPlan();
		if (flightPlan.IsValid())
			AcList.AddFpToTheList(flightPlan);
		target = RadarTargetSelectNext(target);
	}
}

bool FPLVExt::OnCompileCommand(const char* sCommandLine)
{
	if (sCommandLine == NULL)
		return false;

	if (strcmp(sCommandLine, ".fplv reload") == 0)
	{
		LoadConfigOrCreateDefault();
		DisplayUserMessage("Message", PLUGIN_NAME, "Validation rules reloaded.", true, true, false, true, false);
		return true;
	}
	if (strcmp(sCommandLine, ".fplv debug") == 0)
	{
		runtimeDebug = !runtimeDebug;
		DisplayUserMessage("Message", PLUGIN_NAME, runtimeDebug ? "Debug enabled." : "Debug disabled.", true, true, false, true, false);
		return true;
	}
	if (strcmp(sCommandLine, ".fplv debug on") == 0)
	{
		runtimeDebug = true;
		DisplayUserMessage("Message", PLUGIN_NAME, "Debug enabled.", true, true, false, true, false);
		return true;
	}
	if (strcmp(sCommandLine, ".fplv debug off") == 0)
	{
		runtimeDebug = false;
		DisplayUserMessage("Message", PLUGIN_NAME, "Debug disabled.", true, true, false, true, false);
		return true;
	}

	return false;
}

void FPLVExt::OnFlightPlanFlightPlanDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan)
{
	(void)FlightPlan;
}

void FPLVExt::OnFlightPlanControllerAssignedDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan, int DataType)
{
	(void)FlightPlan;
	(void)DataType;
}

void FPLVExt::OnFunctionCall(int FunctionId, const char* sItemString, POINT Pt, RECT Area)
{
	(void)sItemString;
	(void)Pt;
	(void)Area;

	if (FunctionId == TAG_FUNCTION_TOGGLE_VERBOSE)
	{
		verboseDetails = !verboseDetails;
		DisplayUserMessage("Message", PLUGIN_NAME, verboseDetails ? "Validation detail enabled." : "Validation detail disabled.", true, true, false, true, false);
	}
}