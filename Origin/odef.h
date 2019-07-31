#pragma once
#include "ast.h"
#include "allocator.h"
#include <istream>
#include <ostream>

namespace origin {
	void encode(std::ostream&, program*);
	program* decode(std::istream&, allocator&);
}