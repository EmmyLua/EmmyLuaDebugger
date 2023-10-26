#pragma once
#include <string>

// 等于0表示相等
bool CompareIgnoreCase(const std::string &lh, const std::string &rh);

struct CaseInsensitiveLess final {
	bool operator()(const std::string &lhs, const std::string &rhs) const;
};

// 直到C++ 20才有endwith，这里自己写一个
bool EndWith(const std::string &source, const std::string &end);


