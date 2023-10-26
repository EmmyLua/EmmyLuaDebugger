#include "emmy_debugger/util.h"
#include <cstring>
#include <algorithm>

// glibc 函数
int __strncasecmp(const char *s1, const char *s2, int n) {
	if (n && s1 != s2) {
		do {
			int d = ::tolower(*s1) - ::tolower(*s2);
			if (d || *s1 == '\0' || *s2 == '\0')
				return d;
			s1++;
			s2++;
		} while (--n);
	}
	return 0;
}

bool CompareIgnoreCase(const std::string &lh, const std::string &rh) {
	std::size_t llen = lh.size();
	std::size_t rlen = rh.size();
	int ret = __strncasecmp(lh.data(), rh.data(), static_cast<int>((std::min)(llen, rlen)));
	return ret;
}

bool CaseInsensitiveLess::operator()(const std::string &lhs, const std::string &rhs) const {
	std::size_t llen = lhs.size();
	std::size_t rlen = rhs.size();

	int ret = CompareIgnoreCase(lhs, rhs);

	if (ret < 0) {
		return true;
	}
	if (ret == 0 && llen < rlen) {
		return true;
	}
	return false;
}

bool EndWith(const std::string &source, const std::string &end) {
	auto endSize = end.size();
	auto sourceSize = source.size();

	if (endSize > sourceSize) {
		return false;
	}

	return strncmp(end.data(), source.data() + (sourceSize - endSize), endSize) == 0;
}


