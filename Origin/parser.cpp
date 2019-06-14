#include "parser.h"
#include <unordered_set>

using namespace std::string_literals;

namespace origin {
	static void register_prefix_op(parser* parser,
		std::unordered_map<std::string, prefix_parselet>& parselets,
		std::unordered_map<std::string, int>& op_precedence, allocator& memory,
		std::string op, int precedence) {
		op_precedence["un" + op] = precedence;
		parselets[op] = [&memory, op, parser](token start) {
			auto result = memory.allocate<un_expr>();
			result->op = op;
			result->expr = parser->read_expr("un" + op);
			result->start = start;
			result->end = parser->lexer.last();
			return result;
		};
	}

	static void register_infix_op(parser* parser,
		std::unordered_map<std::string, infix_parselet>& parselets,
		std::unordered_map<std::string, int>& op_precedence, allocator& memory,
		std::string op, int precedence, bool ltr) {
		op_precedence[op] = precedence;
		parselets[op] = [&memory, op, parser, precedence, ltr](token op_token, expr* left) {
			auto result = memory.allocate<bin_expr>();
			result->op = op;
			result->left = left;
			result->right = parser->read_expr(precedence - (ltr ? 0 : 1));
			result->start = left->start;
			result->op_token = op_token;
			result->end = parser->lexer.last();
			return result;
		};
	}

	parser::parser(origin::lexer& lexer, std::vector<diagnostic>& diagnostics)
		: diagnostics(diagnostics), lexer(lexer) {
		std::unordered_map<std::string, int> linfix_ops;
		std::unordered_map<std::string, int> rinfix_ops;
		std::unordered_map<std::string, int> prefix_ops;

		//linfix_ops[","] = 1;

		rinfix_ops["="] = 2;
		rinfix_ops["*="] = 2;
		rinfix_ops["/="] = 2;
		rinfix_ops["%="] = 2;
		rinfix_ops["+="] = 2;
		rinfix_ops["-="] = 2;
		rinfix_ops["<<~="] = 2;
		rinfix_ops["~>>="] = 2;
		rinfix_ops["&="] = 2;
		rinfix_ops["|="] = 2;
		rinfix_ops["^="] = 2;
		
		//linfix_ops["?"] = 3;
		linfix_ops["||"] = 4;
		linfix_ops["&&"] = 5;
		linfix_ops["|"] = 6;
		linfix_ops["^"] = 7;
		linfix_ops["&"] = 8;

		linfix_ops["=="] = 9;
		linfix_ops["!="] = 9;

		linfix_ops["<="] = 10;
		linfix_ops[">="] = 10;
		linfix_ops["<"] = 10;
		linfix_ops[">"] = 10;

		linfix_ops["<<~"] = 11;
		linfix_ops["~>>"] = 11;

		linfix_ops["+"] = 12;
		linfix_ops["-"] = 12;

		linfix_ops["*"] = 13;
		linfix_ops["/"] = 13;
		linfix_ops["%"] = 13;

		prefix_ops["++"] = 14;
		prefix_ops["--"] = 14;
		prefix_ops["~"] = 14;
		prefix_ops["!"] = 14;
		prefix_ops["-"] = 14;
		prefix_ops["+"] = 14;

		//linfix_ops["::"] = 16;

		for (auto curr : prefix_ops) {
			register_prefix_op(this, prefix_parselets, op_precedence,
				memory, curr.first, curr.second);
		}

		for (auto curr : linfix_ops) {
			register_infix_op(this, infix_parselets, op_precedence,
				memory, curr.first, curr.second, true);
		}

		for (auto curr : rinfix_ops) {
			register_infix_op(this, infix_parselets, op_precedence,
				memory, curr.first, curr.second, false);
		}

		op_precedence["("] = 15;
		infix_parselets["("] = [&](token start, expr* left) {
			auto result = memory.allocate<call_expr>();
			result->start = left->start;
			result->function = left;
			while (!lexer.is_next(token_type::symbol, ")"s) && !lexer.eof()) {
				result->args.push_back(read_expr());
				if (lexer.is_next(token_type::symbol, ","s)) {
					token comma = lexer.next();
					if (lexer.is_next(token_type::symbol, ")"s)) {
						diagnostics.push_back(error("trailing comma in argument list"s, comma));
					}
				}
				else {
					break;
				}
			}
			if (!lexer.is_next(token_type::symbol, ")"s)
				&& !lexer.try_to_close(token_type::symbol, "("s, ")"s)) {
				diagnostics.push_back(error("unclosed function call"s, start));
			}
			lexer.read_msg(token_type::symbol, ")"s, "to close function call"s);
			result->end = lexer.last();
			return result;
		};

		op_precedence["["] = 15;
		infix_parselets["["] = [&](token start, expr* left) {
			auto result = memory.allocate<subscript>();
			result->start = left->start;
			result->left = left;
			result->right = read_expr();
			if (!lexer.is_next(token_type::symbol, "]"s)
				&& !lexer.try_to_close(token_type::symbol, "["s, "]"s)) {
				diagnostics.push_back(error("unclosed subscript"s, start));
			}
			lexer.read_msg(token_type::symbol, "]"s, "to close subscript"s);
			result->end = lexer.last();
			return result;
		};

		op_precedence["."] = 15;
		infix_parselets["."] = [&](token start, expr* left) {
			auto result = memory.allocate<member>();
			result->start = left->start;
			result->object = left;
			result->name = (result->name_token = lexer.consume(token_type::identifier)).value;
			result->end = lexer.last();
			return result;
		};

		op_precedence["::"] = 16;
		infix_parselets["::"] = [&](token start, expr* left) {
			auto result = memory.allocate<variable>();
			auto ns = dynamic_cast<variable*>(left);
			if (!ns) {
				diagnostics.push_back(error("expected namespace"s, left->start, left->end));
				ns = memory.allocate<variable>();
				ns->name = "<error namespace>";
			}
			result->start = left->start;
			result->name = ns->name + "::" + lexer.consume(token_type::identifier).value;
			result->end = lexer.last();
			return result;
		};

		op_precedence["un("] = 17;
		prefix_parselets["("] = [&](token token) {
			auto result = read_expr();
			if (lexer.is_next(token_type::symbol, ")"s)) {
				lexer.next();
			}
			else if (!lexer.try_to_close(token_type::symbol, "("s, ")"s)) {
				diagnostics.push_back(error("unclosed parenthesis"s, token));
				lexer.read_msg(token_type::symbol, ")"s, "to close parenthesis"s);
			}
			auto real_result = memory.allocate<parenthetical>();
			real_result->expr = result;
			real_result->start = token;
			real_result->end = lexer.last();
			return real_result;
		};
	}

	static expr* atom(allocator& memory, lexer& lexer, std::vector<diagnostic>& diagnostics) {
		if (lexer.is_next(token_type::number)) {
			auto result = memory.allocate<int_literal>();
			result->value = lexer.next().value;
			result->end = result->start = lexer.last();
			return result;
		}
		else if (lexer.is_next(token_type::identifier)) {
			auto result = memory.allocate<variable>();
			result->name = lexer.next().value;
			result->end = result->start = lexer.last();
			return result;
		}
		else {
			return nullptr;
		}
	}

	typing* parser::read_typing() {
		auto result = memory.allocate<typing>();
		result->start = lexer.peek();
		result->alias_name = result->name = read_variable()->name;
		if (lexer.is_next(token_type::symbol, "<"s)) {
			token start = lexer.next();
			result->generic_token = start;
			while (lexer.is_next(token_type::identifier)) {
				result->templates.push_back(read_typing());
				if (lexer.is_next(token_type::symbol, ","s)) {
					token comma = lexer.next();
					if (lexer.is_next(token_type::symbol, ">"s)) {
						diagnostics.push_back(error("trailing comma on template types"s, comma));
					}
				}
				else {
					break;
				}
			}
			if (!lexer.is_next(token_type::symbol, ">"s)
				&& !lexer.try_to_close(token_type::symbol, "<"s, ">"s)) {
				diagnostics.push_back(error("unclosed template"s, start));
			}
			lexer.read_msg(token_type::symbol, ">"s, "to close template"s);
		}
		else {
			result->generic_token = result->start;
		}
		result->end = lexer.last();
		while (lexer.is_next(token_type::symbol, "["s)) {
			auto array_result = memory.allocate<typing>();
			array_result->generic_token = lexer.next();
			lexer.read_msg(token_type::symbol, "]"s, "to close array modifier"s);
			array_result->end = lexer.last();
			array_result->start = result->start;
			array_result->alias_name = array_result->name = "stdlib::core::array";
			array_result->templates.push_back(result);
			result = array_result;
		}
		return result;
	}

	expr* parser::read_expr(const std::string& op) {
		int precedence = 0;
		if (op_precedence.find(op) != op_precedence.end()) {
			precedence = op_precedence[op];
		}
		return read_expr(precedence);
	}

	expr* parser::read_expr(int precedence) {
		std::function<expr*()> fail = [&] {
			return read_expr(precedence, fail);
		};
		return read_expr(precedence, fail);
	}

	expr* parser::read_expr(int precedence, std::function<expr*()> fail) {
		expr* left = nullptr;
		if (lexer.is_next(token_type::symbol)) {
			token tok = lexer.peek();
			if (prefix_parselets.find(tok.value) != prefix_parselets.end()) {
				left = prefix_parselets[tok.value](lexer.next());
			}
		}

		if (left == nullptr) {
			left = atom(memory, lexer, diagnostics);
		}

		while (true) {
			if (left == nullptr) break;
			int tprec = -1;
			if (lexer.is_next(token_type::symbol)) {
				token tok = lexer.peek();
				if (infix_parselets.find(tok.value) != infix_parselets.end()) {
					int prec = op_precedence[tok.value];
					if (prec > precedence) {
						left = infix_parselets[tok.value](lexer.next(), left);
						continue;
					}
				}
			}
			break;
		}

		if (left == nullptr) {
			if (lexer.eof()) {
				token tok = lexer.next();
				diagnostics.push_back(error("unexpected <eof>"s, tok));
				return memory.allocate<error_expr>();
			}
			token tok = lexer.next();
			diagnostics.push_back(error("unexpected "s + encode_token(tok, true), tok));
			return fail();
		}
		else {
			return left;
		}
	}

	void parser::semi() {
		if (!lexer.is_next(token_type::symbol, ";"s)) {
			if (lexer.has_newline()) {
				token newline = lexer.newline_token();
				diagnostics.push_back(error("missing semicolon"s, newline));
			}
			else {
				lexer.read_msg(token_type::symbol, ";"s, "to end statement"s);
			}
		}
		else {
			lexer.next();
		}
	}

	variable* parser::read_variable() {
		auto varv = memory.allocate<variable>();
		varv->start = lexer.peek();
		varv->name = lexer.consume(token_type::identifier).value;
		while (lexer.is_next(token_type::symbol, "::"s)) {
			lexer.next();
			varv->name += "::" + lexer.consume(token_type::identifier).value;
		}
		varv->end = lexer.last();
		return varv;
	}

	stat* parser::read_stat() {
		if (lexer.is_next(token_type::keyword, "return"s)) {
			auto result = memory.allocate<return_stat>();
			result->start = lexer.peek();
			lexer.next();
			result->expr = read_expr();
			semi();
			result->end = lexer.last();
			return result;
		}
		else if (lexer.is_next(token_type::keyword, "do"s)) {
			token start = lexer.next();
			auto result = read_block();
			result->start = start;
			return result;
		}
		else {
			lexer.save();
			token begin = lexer.peek();
			if (lexer.is_next(token_type::identifier)) {
				typing* typing = read_typing();
				if (lexer.is_next(token_type::identifier)) {
					lexer.restore();
					return read_vardecl();
				}
				else {
					lexer.restore();
				}
			}
			expr* expr = read_expr(0, [] {
				return nullptr;
			});
			if (expr == nullptr) return read_stat();
			semi();
			auto result = memory.allocate<expr_stat>();
			result->start = expr->start;
			result->end = expr->end;
			result->expr = expr;
			return result;
		}
	}

	block* parser::read_block() {
		auto result = memory.allocate<block>();
		token start = lexer.read_msg(token_type::symbol, "{"s, "to open block"s);
		result->start = start;
		while (lexer.is_next(token_type::symbol, ";"s)) lexer.next();
		while (!lexer.is_next(token_type::symbol, "}"s) && !lexer.eof()) {
			result->stats.push_back(read_stat());
			while (lexer.is_next(token_type::symbol, ";"s)) lexer.next();
		}
		if (!lexer.is_next(token_type::symbol, "}"s)) {
			diagnostics.push_back(error("unclosed block"s, result->start));
		}
		lexer.read_msg(token_type::symbol, "}"s, "to close block"s);
		result->end = lexer.last();
		return result;
	}

	vardecl* parser::read_vardecl() {
		auto result = memory.allocate<vardecl>();
		result->start = lexer.peek();
		result->typing = read_typing();
		result->var_token = lexer.read(token_type::identifier);
		result->variable = result->var_token.value;
		if (lexer.is_next(token_type::symbol, "("s)) {
			auto lambda = memory.allocate<origin::lambda>();
			lambda->start = result->start;

			token start = lexer.next();
			while (!lexer.is_next(token_type::symbol, ")"s) && !lexer.eof()) {
				lambda->param_types.push_back(read_typing());
				lambda->param_names.push_back(lexer.consume(token_type::identifier).value);
				if (lexer.is_next(token_type::symbol, ","s)) {
					token comma = lexer.next();
					if (lexer.is_next(token_type::symbol, ")"s)) {
						diagnostics.push_back(error("trailing comma in parameter list"s, comma));
					}
				}
				else {
					break;
				}
			}
			if (lexer.is_next(token_type::symbol, ")"s)) {
				lexer.next();
			}
			else if (!lexer.try_to_close(token_type::symbol, "("s, ")"s)) {
				diagnostics.push_back(error("unclosed parenthesis"s, start));
				lexer.read_msg(token_type::symbol, ")"s, "to close parenthesis"s);
			}
			token tend = lexer.last();
			lambda->return_type = result->typing;
			lambda->block = read_block();
			lambda->end = lexer.last();
			result->init_value = lambda;
			result->typing = memory.allocate<typing>();
			result->typing->start = lambda->return_type->start;
			result->typing->generic_token = lambda->return_type->start;
			result->typing->end = tend;
			result->typing->alias_name = result->typing->name = "stdlib::core::function";
			result->typing->templates.push_back(lambda->return_type);
			for (auto type : lambda->param_types) {
				result->typing->templates.push_back(type);
			}
		}
		else if (lexer.is_next(token_type::symbol, "="s)) {
			lexer.next();
			result->init_value = read_expr();
			semi();
		}
		else {
			semi();
		}
		result->end = lexer.last();
		return result;
	}

	lambda* parser::read_func_part(typing* return_type) {
		auto lambda = memory.allocate<origin::lambda>();
		lambda->start = lexer.peek();

		token start = lexer.consume(token_type::symbol, "("s);
		while (!lexer.is_next(token_type::symbol, ")"s) && !lexer.eof()) {
			lambda->param_types.push_back(read_typing());
			lambda->param_names.push_back(lexer.consume(token_type::identifier).value);
			if (lexer.is_next(token_type::symbol, ","s)) {
				token comma = lexer.next();
				if (lexer.is_next(token_type::symbol, ")"s)) {
					diagnostics.push_back(error("trailing comma in parameter list"s, comma));
				}
			}
			else {
				break;
			}
		}
		if (lexer.is_next(token_type::symbol, ")"s)) {
			lexer.next();
		}
		else if (!lexer.try_to_close(token_type::symbol, "("s, ")"s)) {
			diagnostics.push_back(error("unclosed parenthesis"s, start));
			lexer.read_msg(token_type::symbol, ")"s, "to close parenthesis"s);
		}

		lambda->return_type = return_type;
		lambda->block = read_block();
		lambda->end = lexer.last();

		auto typing = memory.allocate<origin::typing>();
		typing->alias_name = typing->name = "stdlib::core::function";
		typing->templates.push_back(lambda->return_type);
		for (auto type : lambda->param_types) {
			typing->templates.push_back(type);
		}
		lambda->typing = typing;
		return lambda;
	}

	classdef* parser::read_classdef() {
		auto result = memory.allocate<classdef>();
		result->is_struct = lexer.is_next(token_type::keyword, "struct"s);
		lexer.next();
		result->name = (result->name_token = lexer.consume(token_type::identifier)).value;
		if (lexer.is_next(token_type::symbol, "<"s)) {
			token start = lexer.next();
			while (lexer.is_next(token_type::identifier)) {
				result->generics.push_back(lexer.next().value);
				if (lexer.is_next(token_type::symbol, "..."s)) {
					lexer.next();
					result->variadic = true;
					break;
				}
				if (lexer.is_next(token_type::symbol, ","s)) {
					token comma = lexer.next();
					if (lexer.is_next(token_type::symbol, ">"s)) {
						diagnostics.push_back(error("trailing comma on template types"s, comma));
					}
				}
				else {
					break;
				}
			}
			if (!lexer.is_next(token_type::symbol, ">"s)
				&& !lexer.try_to_close(token_type::symbol, "<"s, ">"s)) {
				diagnostics.push_back(error("unclosed template"s, start));
			}
			lexer.read_msg(token_type::symbol, ">"s, "to close template"s);
		}
		access current_access = access::private_access;
		token start = lexer.read_msg(token_type::symbol, "{"s, "to open class definition"s);
		while (!lexer.is_next(token_type::symbol, "}"s) && !lexer.eof()) {
			if (lexer.is_next(token_type::keyword, "private"s)) {
				lexer.next();
				lexer.read_msg(token_type::symbol, ":"s, "after access modifier"s);
				current_access = access::private_access;
			}
			else if (lexer.is_next(token_type::keyword, "public"s)) {
				lexer.next();
				lexer.read_msg(token_type::symbol, ":"s, "after access modifier"s);
				current_access = access::public_access;
			}
			else if (lexer.is_next(token_type::symbol, "~"s)) {
				auto vardecl = memory.allocate<origin::vardecl>();
				vardecl->start = lexer.peek();
				lexer.next();
				lexer.read_msg(token_type::identifier, result->name, "in destructor"s);
				vardecl->variable = ".dtor";
				lambda* lambda = read_func_part(nullptr);
				vardecl->init_value = lambda;
				vardecl->end = lexer.last();
				result->vardecls.push_back(vardecl);
				result->accesses.push_back(current_access);
			}
			else {
				lexer.save();
				if (lexer.is_next(token_type::identifier, result->name)) {
					auto vardecl = memory.allocate<origin::vardecl>();
					vardecl->start = lexer.peek();
					lexer.next();
					if (lexer.is_next(token_type::symbol, "("s)) {
						lexer.discard();
						vardecl->variable = ".ctor";
						lambda* lambda = read_func_part(nullptr);
						vardecl->init_value = lambda;
						vardecl->end = lexer.last();
						result->vardecls.push_back(vardecl);
						result->accesses.push_back(current_access);
						continue;
					}
					else {
						lexer.restore();
					}
				}
				lexer.save();
				typing* typing = read_typing();
				if (lexer.is_next(token_type::keyword, "operator"s)) {
					lexer.discard();
					auto vardecl = memory.allocate<origin::vardecl>();
					vardecl->start = lexer.peek();
					lexer.next();
					token sym1 = lexer.consume(token_type::symbol);
					std::string op = sym1.value;
					if (sym1.value == "("s) {
						lexer.read_msg(token_type::symbol, ")"s, "to close parenthesis"s);
					}
					else if (sym1.value == "["s) {
						lexer.read_msg(token_type::symbol, "]"s, "to close bracket"s);
						if (lexer.is_next(token_type::symbol, "="s)) {
							lexer.next();
							op += "=";
						}
					}
					bool can_override = infix_parselets.find(op) != infix_parselets.end()
						|| op == "[=";
					std::unordered_set<std::string> no_list;
					no_list.insert("=");
					no_list.insert("*=");
					no_list.insert("/=");
					no_list.insert("%=");
					no_list.insert("+=");
					no_list.insert("-=");
					no_list.insert("<<~=");
					no_list.insert("~>>=");
					no_list.insert("&=");
					no_list.insert("|=");
					no_list.insert("^=");
					no_list.insert(".");
					no_list.insert("::");
					if (!can_override || no_list.find(op) != no_list.end()) {
						diagnostics.push_back(error("cannot override operator"s,
							vardecl->start, lexer.last()));
					}
					vardecl->variable = "operator"s + op;
					lambda* lambda = read_func_part(typing);
					vardecl->init_value = lambda;
					vardecl->typing = lambda->typing;
					vardecl->end = lexer.last();
					result->vardecls.push_back(vardecl);
					result->accesses.push_back(current_access);
				}
				else {
					lexer.restore();
					result->vardecls.push_back(read_vardecl());
					result->accesses.push_back(current_access);
				}
			}
		}
		if (!lexer.is_next(token_type::symbol, "}"s)) {
			diagnostics.push_back(error("unclosed class definition"s, start));
		}
		lexer.read_msg(token_type::symbol, "}"s, "to close class definition"s);
		return result;
	}

	program* parser::read_program() {
		auto result = memory.allocate<program>();
		if (lexer.is_next(token_type::keyword, "namespace"s)) {
			lexer.next();
			result->namespace_name = read_variable()->name;
			semi();
		}
		while (lexer.is_next(token_type::keyword, "import"s)) {
			lexer.next();
			result->imports.push_back(read_variable());
			semi();
		}
		while (!lexer.eof()) {
			if (lexer.is_next(token_type::keyword, "class"s)) {
				result->classes.push_back(read_classdef());
			}
			else if (lexer.is_next(token_type::keyword, "struct"s)) {
				result->classes.push_back(read_classdef());
			}
			else if (lexer.is_next(token_type::keyword, "alias"s)) {
				lexer.next();
				auto name = lexer.consume_msg(token_type::identifier, "in type alias"s).value;
				lexer.read(token_type::symbol, "="s);
				auto typing = read_typing();
				semi();
				result->typedefs[name] = typing;
			}
			else if (lexer.is_next(token_type::identifier)) {
				result->vardecls.push_back(read_vardecl());
			}
			else {
				lexer.consume(token_type::identifier);
			}
		}
		return result;
	}
}