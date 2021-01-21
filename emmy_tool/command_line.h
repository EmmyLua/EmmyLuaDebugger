#pragma once

#include <string>
#include <map>
#include <set>
#include <vector>

struct Option
{
	std::string Value = "";
	bool RestOfAll = false;
};

/**
 * \brief 特定语法结构的命令行解析工具
 */
class CommandLine
{
public:
	/**
	 * \brief 添加解析的参数
	 * \param restOfAll 为true 表示此参数之后的所有参数共同构成该参数的内容
	 */
	void AddArg(const std::string& name, bool restOfAll = false);

	/**
	 * \brief 添加解析目标例如 attach
	 */
	void AddTarget(const std::string& name, bool isParse = true);

	std::string GetTarget() const noexcept;

	std::string GetArg(const std::string& name) const noexcept;

	std::string GetArg(int index) const noexcept;
	
	bool Parse(int argc, char** argv);

private:
	std::map<std::string, Option> _args;
	std::vector<std::string> _argvs;
	std::map<std::string, bool> _targets;
	std::string _currentTarget;
};
