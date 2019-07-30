#pragma once
#include <string>
#include <istream>
#include <unordered_map>
#include "diagnostics.h"
#include "lexer.h"
#include "allocator.h"

namespace origin {
	struct category {
		std::string name;
		token name_token;
	};

	struct pair {
		std::string key;
		std::string value;
		token key_token;
		token value_token;
	};

	class package_parser {
	private:
		std::vector<diagnostic>& diagnostics;

		lexer& input;
	public:
		package_parser(lexer& input, std::vector<diagnostic>& diagnostics);
		
		bool is_category();
		category next_category();
		pair next_pair();
	};
}