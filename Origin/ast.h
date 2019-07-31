#pragma once
#include <string>
#include <variant>
#include <unordered_map>
#include "lexer.h"

namespace origin {
	enum access {
		private_access,
		public_access,
	};

	class block;
	class classdef;

	class typing {
	public:
		std::string name;
		std::string alias_name;
		bool alias;
		std::vector<typing*> templates;
		token start;
		token generic_token;
		token end;
	};

	class expr {
	public:
		class typing* typing;
		token start;
		token end;

		virtual ~expr();
	};

	class un_expr : public expr {
	public:
		std::string op;
		class expr* expr;
	};

	class bin_expr : public expr {
	public:
		token op_token;
		std::string op;
		expr* left;
		expr* right;
	};

	class call_expr : public expr {
	public:
		expr* function;
		std::vector<expr*> args;
	};

	class subscript : public expr {
	public:
		expr* left;
		expr* right;
	};

	class member : public expr {
	public:
		expr* object;
		std::string name;
		token name_token;
	};

	class variable : public expr {
	public:
		std::string name;
	};

	class int_literal : public expr {
	public:
		std::string value;
	};

	class parenthetical : public expr {
	public:
		class expr* expr;
	};

	class lambda : public expr {
	public:
		class block* block;
		origin::typing* return_type;
		std::vector<origin::typing*> param_types;
		std::vector<std::string> param_names;
	};

	class error_expr : public expr {
	public:
	};

	class stat {
	public:
		token start;
		token end;

		virtual ~stat();
	};

	class return_stat : public stat {
	public:
		class expr* expr;
	};

	class block : public stat {
	public:
		std::vector<stat*> stats;
	};

	class expr_stat : public stat {
	public:
		class expr* expr;
	};

	class if_stat : public stat {
	public:
		expr* cond;
		stat* body;
		stat* else_body;
	};

	class vardecl : public stat {
	public:
		token var_token;
		std::string variable;
		class typing* typing;
		expr* init_value;
	};

	class program {
	public:
		std::string namespace_name;
		std::vector<variable*> imports;
		std::vector<vardecl*> vardecls;
		std::vector<classdef*> classes;
		std::unordered_map<std::string, typing*> aliases;
	};

	typedef std::vector<program*> compilation_unit;

	class classdef {
	public:
		std::string name;
		token name_token;
		std::vector<vardecl*> vardecls;
		std::vector<access> accesses;
		bool is_struct;
		std::vector<std::string> generics;
		bool variadic;
		class program* program;
	};
}