#pragma once
#include <vector>
#include <functional>

namespace origin {
	class allocator {
	private:
		std::vector<std::function<void()>> cleanup;
	public:
		template<class T>
		T* allocate() {
			T* result = new T();
			cleanup.push_back([=] {
				delete result;
			});
			return result;
		}

		~allocator();
	};
}