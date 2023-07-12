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

nlohmann::json InitParams::Serialize() {
	return JsonProtocol::Serialize();
}

void InitParams::Deserialize(nlohmann::json json) {
	if (json["emmyHelper"].is_string()) {
		emmyHelper = json["emmyHelper"];
	}

	if (json["ext"].is_array()) {
		for (auto &e: json["ext"]) {
			std::string ex(e);
			// C++ can not use this only
			ext.emplace_back(ex);
		}
	}
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

nlohmann::json AddBreakpointParams::Serialize() {
	return JsonProtocol::Serialize();
}

void AddBreakpointParams::Deserialize(nlohmann::json json) {
	if (json["clear"].is_boolean()) {
		clear = json["clear"];
	}

	if (json["breakPoints"].is_array()) {
		for (auto breakPoint: json["breakPoints"]) {
			auto bp = std::make_shared<BreakPoint>();
			bp->Deserialize(breakPoint);
			breakPoints.push_back(bp);
		}
	}
}

nlohmann::json RemoveBreakpointParams::Serialize() {
	return JsonProtocol::Serialize();
}

void RemoveBreakpointParams::Deserialize(nlohmann::json json) {
	if (json["breakPoints"].is_array()) {
		for (auto breakPoint: json["breakPoints"]) {
			auto bp = std::make_shared<BreakPoint>();
			bp->Deserialize(breakPoint);
			breakPoints.push_back(bp);
		}
	}
}

nlohmann::json ActionParams::Serialize() {
	return JsonProtocol::Serialize();
}

void ActionParams::Deserialize(nlohmann::json json) {
	if (json.count("action") != 0 && json["action"].is_number_integer()) {
		action = json["action"].get<DebugAction>();
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

EvalContext::EvalContext() {
	result = _arena.Alloc();
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

	if (json.count("value") != 0 && json["value"].is_string()) {
		value = json["value"];
	}

	if (json.count("setValue") != 0 && json["setValue"].is_boolean()) {
		setValue = json["setValue"];
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

nlohmann::json EvalParams::Serialize() {
	return JsonProtocol::Serialize();
}

void EvalParams::Deserialize(nlohmann::json json) {
	ctx = std::make_shared<EvalContext>();
	ctx->Deserialize(json);
}
