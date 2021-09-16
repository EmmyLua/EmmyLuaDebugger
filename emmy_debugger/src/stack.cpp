/*
* Copyright (c) 2019. tangzx(love.tangzx@qq.com)
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <memory>
#include "emmy_debugger/types.h"
#include "emmy_debugger/api/lua_api.h"

Variable::Variable()
	: nameType(LUA_TSTRING),
	  valueType(0),
	  cacheId(0)
{
}

Variable::~Variable()
{
}

std::shared_ptr<Variable> Variable::Clone()
{
	auto to = std::make_shared<Variable>();
	to->name = name;
	to->nameType = nameType;
	to->value = value;
	to->valueType = valueType;
	to->valueTypeName = valueTypeName;
	for (auto child : children)
	{
		auto c = std::make_shared<Variable>();
		to->children.push_back(child->Clone());
	}
	return to;
}

std::shared_ptr<Variable> Variable::CreateChildNode()
{
	auto child = std::make_shared<Variable>();
	children.push_back(child);
	return child;
}

Stack::Stack(): level(0), line(0)
{
}

Stack::~Stack()
{
}

std::shared_ptr<Variable> Stack::CreateVariable()
{
	return std::make_shared<Variable>();
}
