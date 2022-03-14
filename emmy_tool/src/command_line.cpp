#include "command_line.h"

void CommandLine::AddTarget(const std::string& name, bool isParse)
{
	_targets.insert({name, isParse});
}

std::string CommandLine::GetTarget() const noexcept
{
	return _currentTarget;
}

std::string CommandLine::GetArg(int index) const noexcept
{
	if (static_cast<std::size_t>(index) < _argvs.size())
	{
		return _argvs[index];
	}
	else
	{
		return "";
	}
}

bool CommandLine::Parse(int argc, char** argv)
{
	if (argc < 2)
	{
		return false;
	}
	_currentTarget = argv[1];

	if (_targets.count(_currentTarget) == 0)
	{
		return false;
	}

	bool isParse = _targets.at(_currentTarget);
	_argvs.reserve(argc);
	for (int i = 0; i != argc; i++)
	{
		_argvs.emplace_back(argv[i]);
	}

	if (!isParse)
	{
		return true;
	}

	// index = 0 的参数是程序名
	for (int index = 1; index < argc; index++)
	{
		std::string current = argv[index];
		if (current.empty())
		{
			continue;
		}
		// not empty
		if (current[0] == '-')
		{
			// 仅仅支持-dir这种形式
			std::string optionName = current.substr(1);
			// 如果该参数不存在
			if (_args.count(optionName) == 0)
			{
				return false;
			}
			CommandLineOption& option = _args[optionName];

			if (option.Type == CommandLineValueType::Boolean)
			{
				option.Value = "true";
				continue;
			}

			std::string optionValue;
			optionValue.reserve(128);

			// 该选项之后没有接参数
			// 目前没有支持bool选项的必要
			if (argc <= (index + 1) && !option.RestOfAll)
			{
				return false;
			}

			do
			{
				if (argc <= (index + 1))
				{
					break;
				}

				std::string value = argv[++index];
				if (value.empty())
				{
					continue;
				}
				if (option.RestOfAll)
				{
					// 剩余参数通常会被传递到子进程再处理
					// 如果剩余参数中存在路径，且路径存在空格，那么传递到子进程的创建就会失效
					// 所以这里要特别的处理
					if (!value.empty())
					{
						// 认为该参数可能是选项
						if (value[0] == '-')
						{
							optionValue.append(" ").append(value);
						}
						else //认为该参数是值，所以用引号包含起来
						{
							optionValue.append(" ").append("\"" + value + "\"");
						}
					}
				}
				else
				{
					// 认为值是被一对引号包起来的
					// windows下引号已经被自动处理了
					if (value[0] == '\"' || value[0] == '\'')
					{
						optionValue = value.substr(1, value.size() - 2);
					}
					else
					{
						optionValue = value;
					}
					break;
				}
			}
			while (true);

			option.Value = std::move(optionValue);
		}
	}
	return true;
}
