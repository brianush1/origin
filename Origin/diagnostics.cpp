#include "diagnostics.h"

namespace origin {
	std::string template_str;

	diagnostic error(const std::string& message, token start) {
		return { message, start.stream, start.start, start.end, template_str };
	}

	diagnostic error(const std::string& message, token start, token end) {
		return { message, start.stream, start.start, end.end, template_str };
	}

	diagnostic warn(const std::string& message, token start) {
		return { message, start.stream, start.start, start.end, template_str, true };
	}

	diagnostic warn(const std::string& message, token start, token end) {
		return { message, start.stream, start.start, end.end, template_str, true };
	}
}