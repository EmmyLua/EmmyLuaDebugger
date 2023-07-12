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
#pragma once

#include "emmy_debugger/arena/arena.h"
#include "nlohmann/json.hpp"
#include <string>
#include <vector>


enum class DebugAction {
	None = -1,
	Break = 0,
	Continue,
	StepOver,
	StepIn,
	StepOut,
	Stop,
};


class JsonProtocol {
public:
	template<class T>
	static nlohmann::json SerializeArray(std::vector<T> &objs) {
		auto arr = nlohmann::json::array();
		for (auto &obj: objs) {
			arr.push_back(obj.Serialize());
		}
		return arr;
	}

	virtual ~JsonProtocol();

	virtual nlohmann::json Serialize();

	virtual void Deserialize(nlohmann::json json);
};

class InitParams : public JsonProtocol {
public:
	std::string emmyHelper;
	std::vector<std::string> ext;

	virtual nlohmann::json Serialize();

	virtual void Deserialize(nlohmann::json json);
};

class BreakPoint : public JsonProtocol {
public:
	std::string file;
	std::string condition;
	std::string hitCondition;
	std::string logMessage;
	int hitCount = 0;
	int line = 0;

	nlohmann::json Serialize() override;

	void Deserialize(nlohmann::json json) override;
};

class AddBreakpointParams : public JsonProtocol {
public:
	bool clear = false;
	std::vector<std::shared_ptr<BreakPoint>> breakPoints;

	nlohmann::json Serialize() override;

	void Deserialize(nlohmann::json json) override;
};

class RemoveBreakpointParams : public JsonProtocol {
public:
	std::vector<std::shared_ptr<BreakPoint>> breakPoints;

	nlohmann::json Serialize() override;

	void Deserialize(nlohmann::json json) override;
};

class ActionParams : public JsonProtocol {
public:
	DebugAction action = DebugAction::None;

	nlohmann::json Serialize() override;

	void Deserialize(nlohmann::json json) override;
};


class Variable : public JsonProtocol {
public:
	Variable();

	std::string name;
	int nameType = 0;
	std::string value;
	int valueType = 0;
	std::string valueTypeName;
	std::vector<Idx<Variable>> children;
	int cacheId = 0;

	nlohmann::json Serialize() override;

	void Deserialize(nlohmann::json json) override;
};

class Stack : public JsonProtocol {
public:
	Stack();

	std::string file;
	std::string functionName;
	int level = 0;
	int line = 0;
	std::vector<Idx<Variable>> localVariables;
	std::vector<Idx<Variable>> upvalueVariables;

	std::shared_ptr<Arena<Variable>> variableArena;

	nlohmann::json Serialize() override;

	void Deserialize(nlohmann::json json) override;
};

class EvalContext : public JsonProtocol {
public:
	EvalContext();

	std::string expr;
	std::string value;
	std::string error;
	int seq = 0;
	int stackLevel = 0;
	int depth = 0;
	int cacheId = 0;
	Idx<Variable> result;
	bool success = false;
	bool setValue = false;

	nlohmann::json Serialize() override;

	void Deserialize(nlohmann::json json) override;
private:
	Arena<Variable> _arena;
};

class EvalParams : public JsonProtocol {
public:
	std::shared_ptr<EvalContext> ctx;

	nlohmann::json Serialize() override;

	void Deserialize(nlohmann::json json) override;
};
