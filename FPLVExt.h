#pragma once

#include "inc\\3.2\\EuroScopePlugIn.h"
#include "ConfigManager.h"
#include <map>
#include <string>
#include <vector>

class FPLVExt : public EuroScopePlugIn::CPlugIn
{
public:
	FPLVExt();
	virtual ~FPLVExt();

	virtual void OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan,
		EuroScopePlugIn::CRadarTarget RadarTarget,
		int ItemCode,
		int TagData,
		char sItemString[16],
		int* pColorCode,
		COLORREF* pRGB,
		double* pFontSize);
	virtual void OnRefreshFpListContent(EuroScopePlugIn::CFlightPlanList AcList);
	virtual bool OnCompileCommand(const char* sCommandLine);
	virtual void OnFlightPlanFlightPlanDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan);
	virtual void OnFlightPlanControllerAssignedDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan, int DataType);
	virtual void OnFunctionCall(int FunctionId, const char* sItemString, POINT Pt, RECT Area);

private:
	void LoadConfigOrCreateDefault();
	FPLV::ValidationResult EvaluateFlightPlan(EuroScopePlugIn::CFlightPlan FlightPlan) const;
	FPLV::ValidationResult EvaluateFlightPlanForRule(EuroScopePlugIn::CFlightPlan FlightPlan, const FPLV::ValidationRule& rule) const;
	std::string BuildCompactSummary(const FPLV::ValidationResult& result) const;
	std::string BuildIssueText(const FPLV::ValidationResult& result) const;
	std::string BuildDirectionText(const FPLV::ValidationResult& result) const;
	std::string GetRouteText(EuroScopePlugIn::CFlightPlan FlightPlan) const;
	void ApplyTagValue(const FPLV::ValidationResult& result,
		const std::string& value,
		char sItemString[16],
		int* pColorCode,
		COLORREF* pRGB) const;
	void LogDebugMessage(const std::string& message);
	bool IsDebugEnabled() const;
	void RegisterValidationColumns();

	FPLV::ConfigManager config;
	EuroScopePlugIn::CFlightPlanList validationList;
	bool verboseDetails = false;
	bool runtimeDebug = false;
	mutable std::map<std::string, std::string> lastDebugMessages;
};

extern FPLVExt* pMyPlugIn;