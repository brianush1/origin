#include "allocator.h"

namespace origin {
	allocator::~allocator() {
		for (auto fn : cleanup) {
			fn();
		}
	}
}