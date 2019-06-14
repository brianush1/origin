#pragma once
#include <stddef.h>
#include <istream>
#include <string>
#include <vector>

namespace origin {
	enum class token_type {
		invalid_token,
		eof,
		identifier,
		keyword,
		string,
		number,
		symbol,
	};

	struct token {
		token_type type = token_type::invalid_token;
		std::istream* stream;
		std::string value;
		size_t start;
		size_t end;
	};

	struct diagnostic {
		std::string message;
		std::istream* stream;
		size_t start;
		size_t end;
		std::string template_str;
		bool warning = false;
	};

	class lexer {
	private:
		struct lexer_state {
			size_t diagnostics;
			std::streampos input;
			bool has_peeked;
			token peeked;
		};

		std::vector<lexer_state> state;
		std::istream& input;
		std::vector<diagnostic>& diagnostics;
		bool has_peeked = false;
		token peeked;
		token last_token;
		token ln_token;
		bool has_ln;

		token next_internal();
		token next_internal(bool, token);
	public:
		lexer(std::istream& input, std::vector<diagnostic>& diagnostics);
		bool eof();
		bool is_next(token_type type);
		bool is_next(token_type type, const std::string& value);
		bool has_newline();
		token newline_token();
		bool try_to_close(token_type type, const std::string& open, const std::string& close);
		token last();
		token next();
		token peek();
		token consume(token_type type);
		token consume(token_type type, const std::string& value);
		token consume_msg(token_type type, const std::string& message);
		token consume_msg(token_type type, const std::string& value, const std::string& message);
		token read(token_type type);
		token read(token_type type, const std::string& value);
		token read_msg(token_type type, const std::string& message);
		token read_msg(token_type type, const std::string& value, const std::string& message);
		void save();
		void restore();
		void discard();
	};

	std::string encode_token(token token, bool include_value);
}