#pragma once
#include <functional>
#include <vector>
#include <unordered_map>
#include <string>
#include "diagnostics.h"
#include "lexer.h"
#include "allocator.h"
#include "ast.h"

namespace origin {
	typedef std::function<expr*(token op)> prefix_parselet;
	typedef std::function<expr*(token op, expr* left)> infix_parselet;

	class parser {
	private:
		allocator memory;
		std::vector<diagnostic>& diagnostics;
		std::unordered_map<std::string, int> op_precedence;
		std::unordered_map<std::string, prefix_parselet> prefix_parselets;
		std::unordered_map<std::string, infix_parselet> infix_parselets;
		void semi();
	public:
		class lexer& lexer;

		parser(origin::lexer& lexer, std::vector<diagnostic>& diagnostics);
		typing* read_typing();
		expr* read_expr(const std::string& op);
		expr* read_expr(int precedence = -1);
		expr* read_expr(int precedence, std::function<expr*()> fail);
		variable* read_variable();
		stat* read_stat();
		block* read_block();
		vardecl* read_vardecl();
		lambda* read_func_part(typing* return_type);
		classdef* read_classdef();
		program* read_program();
	};
}