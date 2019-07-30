#include "lexer.h"
#include <sstream>
#include <unordered_map>

using namespace std::string_literals;

namespace origin {
	constexpr const char* keywords[] = {
		"if", "else", "true", "false",
		"return", "do",
		"namespace", "import",
		"class", "struct", "public", "private",
		"operator",
		"alias",
		nullptr
	};

	constexpr const char* symbols[] = {
		"<<~=", "~>>=",

		"<=>", "...", "<<~", "~>>",

		"<=", ">=", "++", "--", "&&", "||", "::",
		"*=", "/=", "%=", "+=", "-=", "&=", "|=", "^=",
		"==", "!=",
		
		".", "[", "]", "(", ")", "~", "!", "-", "+", "*", "/",
		"%", "+", "-", "<", ">", "&", "^", "|", "?", ":", "=", ",",

		";", "{", "}",
		nullptr
	};

	static bool next_is(std::istream& stream, std::string value) {
		std::streampos pos = stream.tellg();
		stream.seekg(0, std::ios_base::end);
		std::streampos size = stream.tellg();
		stream.seekg(pos);
		if (size - pos < (std::streampos)value.length()) return false;
		for (char c : value) {
			if (stream.get() != c) {
				stream.seekg(pos);
				return false;
			}
		}
		return true;
	}

	token lexer::next_internal() {
		return next_internal(false, {});
	}

	token lexer::next_internal(bool local_ln, token local_ln_token) {
		int ws_char = input.peek();
		while (ws_char == ' ' || ws_char == '\t' || ws_char == '\n'
			|| ws_char == '\r') {
			if (ws_char == '\n') {
				size_t ln = (size_t)input.tellg();
				if (!local_ln) {
					local_ln = true;
					local_ln_token = { token_type::invalid_token, &input, "\n", ln, ln };
				}
			}
			input.get();
			ws_char = input.peek();
		}
		has_ln = local_ln;
		ln_token = local_ln_token;
		if (input.eof()) {
			input.seekg(0, std::ios_base::end);
			size_t index = (size_t)input.tellg();
			return { token_type::eof, &input, "", index, index };
		}
		if (input.get() == '/' && input.peek() == '/') {
			std::string _;
			getline(input, _);
			return next_internal(local_ln, local_ln_token);
		}
		else {
			input.unget();
			if (input.get() == '/' && input.peek() == '*') {
				input.get();
				while (!input.eof() && !(input.get() == '*' && input.peek() == '/'));
				input.get();
				return next_internal(local_ln, local_ln_token);
			}
			else {
				input.unget();
			}
		}
		char c = input.peek();
		auto read_while = [this](bool cond(char c)) {
			std::ostringstream res;
			while (!input.eof() && cond(input.peek())) {
				res << (char)input.get();
			}
			return res.str();
		};
		auto is_digit = [](char c) {
			return c >= '0' && c <= '9';
		};
		auto is_identifier = [](char c) {
			return (c >= 'a' && c <= 'z')
				|| (c >= 'A' && c <= 'Z')
				|| (c >= '0' && c <= '9')
				|| c == '_' || c == '$';
		};
		size_t begin = (size_t)input.tellg();
		if (is_digit(c)) {
			std::string data = read_while(is_digit);
			return { token_type::number, &input, data, begin, (size_t)input.tellg() - 1 };
		}
		else if (is_identifier(c) || c == '@') {
			bool is_raw = false;
			if (c == '@') {
				is_raw = true;
				input.get();
			}
			std::string data = read_while(is_identifier);
			bool is_keyword = false;
			if (!is_raw) for (const char* const* kw = keywords; *kw != nullptr; kw++) {
				if (data == *kw) {
					is_keyword = true;
					break;
				}
			}
			return { is_keyword ? token_type::keyword
				: token_type::identifier, &input, data, begin, (size_t)input.tellg() - 1 };
		}
		else if (c == '"' || c == '\'') {
			input.get();
			std::string data;
			bool escape = false;
			while (input.peek() != c || escape) {
				if (escape) {
					if (input.peek() >= '0' && input.peek() <= '9') {
						int value = input.get() - '0';
						if (input.peek() >= '0' && input.peek() <= '9') {
							value *= 10;
							value += input.get() - '0';
						}
						if (input.peek() >= '0' && input.peek() <= '9') {
							value *= 10;
							value += input.get() - '0';
						}
						data.push_back((char)value);
					}
					else if (input.peek() == 'a') data.push_back('\a');
					else if (input.peek() == 'b') data.push_back('\b');
					else if (input.peek() == 'f') data.push_back('\f');
					else if (input.peek() == 'n') data.push_back('\n');
					else if (input.peek() == 'r') data.push_back('\r');
					else if (input.peek() == 't') data.push_back('\t');
					else if (input.peek() == 'v') data.push_back('\v');
					else {
						data.push_back(input.get());
					}
					escape = false;
				}
				else if (input.eof() || input.peek() == '\n' || input.peek() == '\r') {
					diagnostics.push_back({ "unfinished string", &input, begin, (size_t)input.tellg() - 1 });
					return next_internal(local_ln, local_ln_token);
				}
				else if (input.peek() == '\\') {
					escape = true;
				}
				else {
					data.push_back(input.get());
				}
			}
			input.get(); // this is guaranteed to be ==c or error
			return { token_type::string, &input, data, begin, (size_t)input.tellg() - 1 };
		}
		else {
			for (const char* const* op = symbols; *op != nullptr; op++) {
				std::string s(*op);
				if (next_is(input, s)) {
					return { token_type::symbol, &input, s, begin, (size_t)input.tellg() - 1 };
				}
			}
			input.get();
			diagnostics.push_back({ "unexpected character '"s + c + "'", &input, begin, begin });
			return next_internal(local_ln, local_ln_token);
		}
	}

	lexer::lexer(std::istream& input,
		std::vector<diagnostic>& diagnostics)
		: input(input), diagnostics(diagnostics) {
	}

	bool lexer::eof() {
		return peek().type == token_type::eof;
	}

	bool lexer::is_next(token_type type) {
		return peek().type == type;
	}

	bool lexer::is_next(token_type type, const std::string& value) {
		return peek().type == type && peek().value == value;
	}

	bool lexer::has_newline() {
		return has_ln;
	}

	token lexer::newline_token() {
		return ln_token;
	}

	bool lexer::try_to_close(token_type type, const std::string& open, const std::string& close) {
		save();
		int level = 1;
		token start = peek();
		token end = start;
		while (true) {
			token token = peek();
			if (token.type == token_type::eof || has_ln) {
				restore();
				return false;
			}
			else {
				next();
				if (token.type == type && token.value == open) {
					level++;
				}
				else if (token.type == type && token.value == close) {
					level--;
				}
				if (level == 0) {
					discard();
					diagnostics.push_back({ "unexpected token(s)", &input, start.start, end.end });
					return true;
				}
				else {
					end = token;
				}
			}
		}
	}

	token lexer::last() {
		return last_token;
	}

	token lexer::next() {
		if (has_peeked) {
			has_peeked = false;
			last_token = peeked;
			return peeked;
		}
		else {
			return last_token = next_internal();
		}
	}

	token lexer::peek() {
		if (has_peeked) {
			return peeked;
		}
		else {
			has_peeked = true;
			return peeked = next_internal();
		}
	}

	token lexer::consume(token_type type) {
		token next_token = next();
		if (next_token.type != type) {
			std::ostringstream s;
			s << "expected " << encode_token({ type }, false) << ", found "
				<< encode_token(next_token, false);
			diagnostics.push_back({ s.str(), &input, next_token.start, next_token.end });
			next_token.type = type;
			next_token.value = "";
		}
		return next_token;
	}

	token lexer::consume(token_type type, const std::string& value) {
		token next_token = next();
		if (next_token.type != type || next_token.value != value) {
			std::ostringstream s;
			s << "expected " << encode_token({ type, &input, value }, true) << ", found "
				<< encode_token(next_token, true);
			diagnostics.push_back({ s.str(), &input, next_token.start, next_token.end });
			next_token.type = type;
			next_token.value = value;
		}
		return next_token;
	}

	token lexer::consume_msg(token_type type, const std::string& message) {
		token next_token = next();
		if (next_token.type != type) {
			std::ostringstream s;
			s << "expected " << encode_token({ type }, false) << " " << message
				<< ", found " << encode_token(next_token, false);
			diagnostics.push_back({ s.str(), &input, next_token.start, next_token.end });
			next_token.type = type;
			next_token.value = "";
		}
		return next_token;
	}

	token lexer::consume_msg(token_type type, const std::string& value, const std::string& message) {
		token next_token = next();
		if (next_token.type != type || next_token.value != value) {
			std::ostringstream s;
			s << "expected " << encode_token({ type, &input, value }, true) << " " << message
				<< ", found " << encode_token(next_token, true);
			diagnostics.push_back({ s.str(), &input, next_token.start, next_token.end });
			next_token.type = type;
			next_token.value = value;
		}
		return next_token;
	}

	token lexer::read(token_type type) {
		token next_token = peek();
		if (next_token.type != type) {
			std::ostringstream s;
			s << "expected " << encode_token({ type }, false) << ", found "
				<< encode_token(next_token, false);
			diagnostics.push_back({ s.str(), &input, next_token.start, next_token.end });
			next_token.type = type;
			next_token.value = "";
		}
		else {
			next();
		}
		return next_token;
	}

	token lexer::read(token_type type, const std::string& value) {
		token next_token = peek();
		if (next_token.type != type || next_token.value != value) {
			std::ostringstream s;
			s << "expected " << encode_token({ type, &input, value }, true) << ", found "
				<< encode_token(next_token, true);
			diagnostics.push_back({ s.str(), &input, next_token.start, next_token.end });
			next_token.type = type;
			next_token.value = value;
		}
		else {
			next();
		}
		return next_token;
	}

	token lexer::read_msg(token_type type, const std::string& message) {
		token next_token = peek();
		if (next_token.type != type) {
			std::ostringstream s;
			s << "expected " << encode_token({ type }, false) << " " << message
				<< ", found " << encode_token(next_token, false);
			diagnostics.push_back({ s.str(), &input, next_token.start, next_token.end });
			next_token.type = type;
			next_token.value = "";
		}
		else {
			next();
		}
		return next_token;
	}

	token lexer::read_msg(token_type type, const std::string& value, const std::string& message) {
		token next_token = peek();
		if (next_token.type != type || next_token.value != value) {
			std::ostringstream s;
			s << "expected " << encode_token({ type, &input, value }, true) << " " << message
				<< ", found " << encode_token(next_token, true);
			diagnostics.push_back({ s.str(), &input, next_token.start, next_token.end });
			next_token.type = type;
			next_token.value = value;
		}
		else {
			next();
		}
		return next_token;
	}

	void lexer::save() {
		state.push_back({ diagnostics.size(), input.tellg(), has_peeked, peeked });
	}

	void lexer::restore() {
		lexer_state s = state.back();
		state.pop_back();
		while (diagnostics.size() != s.diagnostics) diagnostics.pop_back();
		input.seekg(s.input);
		has_peeked = s.has_peeked;
		peeked = s.peeked;
	}

	void lexer::discard() {
		state.pop_back();
	}

	std::string encode_token(token token, bool include_value) {
		std::ostringstream result;
		switch (token.type) {
		case token_type::eof:
			result << "<eof>";
			break;
		case token_type::identifier:
			result << "identifier";
			break;
		case token_type::keyword:
			result << "keyword";
			break;
		case token_type::number:
			result << "number";
			break;
		case token_type::string:
			result << "string";
			break;
		case token_type::symbol:
			result << "symbol";
			break;
		default:
			result << "<unknown>";
			break;
		}
		if (include_value && token.type != token_type::eof) {
			result << " '" << token.value << "'";
		}
		return result.str();
	}
}