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
#include "ConfigData.h"

namespace FPLV
{
	ConfigData::ConfigData()
	{
		this->Cleanup();
	}

	void ConfigData::Cleanup()
	{
		plugin_name = "FPLV";
		list_name = "Flight plan validation";
		debug_enabled = false;
		columns.clear();
		rules.clear();
		vfr_rules.clear();
		rvsm_rules.clear();
		load_messages.clear();
	}
}
