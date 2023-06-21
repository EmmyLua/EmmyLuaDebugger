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
#include "emmy_debugger/proto/proto.h"
#include "emmy_debugger/api/lua_api.h"

JsonProtocol::~JsonProtocol() {
}

nlohmann::json JsonProtocol::Serialize() {
	return nlohmann::json::object();
}

void JsonProtocol::Deserialize(nlohmann::json json) {
}

nlohmann::json BreakPoint::Serialize() {
	return JsonProtocol::Serialize();
}

void BreakPoint::Deserialize(nlohmann::json json) {
	if (json.count("file") != 0) {
		file = json["file"];
	}
	if (json.count("line") != 0) {
		line = json["line"];
	}
	if (json.count("condition") != 0) {
		condition = json["condition"];
	}
	if (json.count("hitCondition") != 0) {
		hitCondition = json["hitCondition"];
	}
	if (json.count("logMessage") != 0) {
		logMessage = json["logMessage"];
	}
}

Variable::Variable()
	: nameType(LUA_TSTRING),
	  valueType(0),
	  cacheId(0) {
}

nlohmann::json Variable::Serialize() {
	auto obj = nlohmann::json::object();
	obj["name"] = name;
	obj["nameType"] = nameType;
	obj["value"] = value;
	obj["valueType"] = valueType;
	obj["valueTypeName"] = valueTypeName;
	obj["cacheId"] = cacheId;

	// children
	if (!children.empty()) {
		auto arr = nlohmann::json::array();
		for (auto idx: children) {
			arr.push_back(idx->Serialize());
		}
		obj["children"] = arr;
	}
	return obj;
}

void Variable::Deserialize(nlohmann::json json) {
	JsonProtocol::Deserialize(json);
}

Stack::Stack()
	: level(0), line(0), variableArena(std::make_shared<Arena<Variable>>()) {
}

nlohmann::json Stack::Serialize() {

	auto stackJson = nlohmann::json::object();
	stackJson["file"] = file;
	stackJson["functionName"] = functionName;
	stackJson["line"] = line;
	stackJson["level"] = level;
	{
		auto arr = nlohmann::json::array();
		for (auto idx: localVariables) {
			arr.push_back(idx->Serialize());
		}
		stackJson["localVariables"] = arr;
	}

	{
		auto arr = nlohmann::json::array();
		for (auto idx: upvalueVariables) {
			arr.push_back(idx->Serialize());
		}
		stackJson["upvalueVariables"] = arr;
	}

	return stackJson;
}

void Stack::Deserialize(nlohmann::json json) {
	JsonProtocol::Deserialize(json);
}

EvalContext::EvalContext()
	: variableArena(std::make_shared<Arena<Variable>>()) {
	result = variableArena->Alloc();
}

nlohmann::json EvalContext::Serialize() {
	auto obj = nlohmann::json::object();
	obj["seq"] = seq;
	obj["success"] = success;

	if (success) {
		obj["value"] = result->Serialize();
	} else {
		obj["error"] = error;
	}
	return obj;
}

void EvalContext::Deserialize(nlohmann::json json) {
	if (json.count("seq") != 0 && json["seq"].is_number_integer()) {
		seq = json["seq"];
	}
	if (json.count("expr") != 0 && json["expr"].is_string()) {
		expr = json["expr"];
	}

	if (json.count("stackLevel") != 0 && json["stackLevel"].is_number_integer()) {
		stackLevel = json["stackLevel"];
	}
	if (json.count("depth") != 0 && json["depth"].is_number_integer()) {
		depth = json["depth"];
	}
	if (json.count("cacheId") != 0 && json["cacheId"].is_number_integer()) {
		cacheId = json["cacheId"];
	}

}
