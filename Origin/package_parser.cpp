#include "package_parser.h"

namespace origin {
	package_parser::package_parser(lexer& input, std::vector<diagnostic>& diagnostics)
		: diagnostics(diagnostics), input(input) {
	}

	bool package_parser::is_category() {
		return input.is_next(token_type::symbol, "[");
	}

	category package_parser::next_category() {
		if (is_category()) {
			category result;
			input.next();
			result.name = (result.name_token = input.consume_msg(token_type::identifier, "in category")).value;
			input.read_msg(token_type::symbol, "]", "to close category");
			return result;
		}
	}

	pair package_parser::next_pair() {
		pair result;
		if (input.is_next(token_type::string)) {
			result.key = (result.key_token = input.next()).value;
		}
		else {
			result.key = (result.key_token = input.consume_msg(token_type::identifier, "as key")).value;
		}
		input.read_msg(token_type::symbol, "=", "to separate key from value");
		if (input.is_next(token_type::string)) {
			result.value = (result.value_token = input.next()).value;
		}
		else {
			result.value = (result.value_token = input.consume_msg(token_type::identifier, "as value")).value;
		}
		return result;
	}
}