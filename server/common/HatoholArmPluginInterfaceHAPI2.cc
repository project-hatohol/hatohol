/*
 * Copyright (C) 2015 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "HatoholArmPluginInterfaceHAPI2.h"

using namespace std;
using namespace mlpl;

struct HatoholArmPluginInterfaceHAPI2::Impl
{
	HatoholArmPluginInterfaceHAPI2 *hapi2;
	ProcedureHandlerMap procedureHandlerMap;

	Impl(HatoholArmPluginInterfaceHAPI2 *_hapi2)
	: hapi2(_hapi2)
	{
	}

	~Impl()
	{
	}
};

HatoholArmPluginInterfaceHAPI2::HatoholArmPluginInterfaceHAPI2()
: m_impl(new Impl(this))
{
}

HatoholArmPluginInterfaceHAPI2::~HatoholArmPluginInterfaceHAPI2()
{
}

void HatoholArmPluginInterfaceHAPI2::registerProcedureHandler(
  const HAPI2ProcedureType &type, ProcedureHandler handler)
{
	m_impl->procedureHandlerMap[type] = handler;
}

void HatoholArmPluginInterfaceHAPI2::interpretHandler(
  const HAPI2ProcedureType &type, const string params, JsonNode *root)
{
	ProcedureHandlerMapConstIterator it =
	  m_impl->procedureHandlerMap.find(type);
	if (it == m_impl->procedureHandlerMap.end()) {
		// TODO: reply error
		return;
	}
	ProcedureHandler handler = it->second;
	string replyJSON = (this->*handler)(&type, params);
	// TODO: send replyJSON
	onHandledCommand(type);
}

void HatoholArmPluginInterfaceHAPI2::onHandledCommand(const HAPI2ProcedureType &type)
{
}
