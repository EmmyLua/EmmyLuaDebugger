#pragma once

#include <cinttypes>
#include <vector>
#include <exception>
#include <stdexcept>

template<class T>
class Arena;



template<class T>
struct Idx {
	using ArenaClass = Arena<T>;

	Idx()
		: Raw(0), _arena(nullptr) {
	}

	Idx(uint32_t raw, ArenaClass *arena)
		: Raw(raw),
		  _arena(arena) {
	}

	T &operator*() {
		return _arena->Index(*this);
	}

	T *operator->() {
		return &(_arena->Index(*this));
	}


	ArenaClass *GetArena() {
		return _arena;
	}

	uint32_t Raw;

private:
	ArenaClass *_arena;
};

template<class T>
class Arena {
public:
	T &Index(Idx<T> id) {
		if (id.Raw < _data.size()) {
			return _data[id.Raw];
		}
		throw std::runtime_error("index out of range");
	}

	Idx<T> Alloc() {
		uint32_t rawId = static_cast<uint32_t>(_data.size());
		_data.emplace_back();
		return Idx<T>(rawId, this);
	}

	void Clear() {
		_data.clear();
	}

private:
	std::vector<T> _data;
};
