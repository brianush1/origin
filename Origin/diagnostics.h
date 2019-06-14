#pragma once
#include <stddef.h>
#include <string>
#include "lexer.h"

namespace origin {
	extern std::string template_str;
	diagnostic error(const std::string& message, token start);
	diagnostic error(const std::string& message, token start, token end);
	diagnostic warn(const std::string& message, token start);
	diagnostic warn(const std::string& message, token start, token end);
}