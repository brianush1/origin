#pragma once
#include <unordered_map>
#include "ast.h"
#include "walker.h"
#include "allocator.h"
#include "diagnostics.h"
#include "lexer.h"

namespace origin {
	class scope {
	public:
		scope* parent;
		std::unordered_map<std::string, typing*> variables;

		scope();
		scope(scope* parent);

		bool has(const std::string& name);
		typing* get(const std::string& name);
		void declare(const std::string& name, typing* typing);
	};

	class type_assigner : public walker<void> {
	private:
		allocator memory;
		compilation_unit* unit;
		scope* current_scope;
		std::vector<diagnostic>& diagnostics;
		program* current_program;
		std::unordered_map<std::string, classdef*> classes;
		std::unordered_map<typing*, classdef*> generic_classes;
		std::unordered_map<std::string, typing*> aliases;
		std::vector<typing*> current_template;
	public:
		type_assigner(std::vector<diagnostic>& diagnostics);
		~type_assigner();

		void downscope();
		void upscope();

		classdef* find_class(typing* typing);
		typing* patch(typing*);

		virtual void walk(vardecl* stat);
		virtual void walk(expr_stat* stat);
		virtual void walk(if_stat* stat);
		virtual void walk(block* stat);
		virtual void walk(return_stat* stat);

		virtual void walk(error_expr* expr);
		virtual void walk(lambda* expr);
		virtual void walk(parenthetical* expr);
		virtual void walk(int_literal* expr);
		virtual void walk(variable* expr);
		virtual void walk(member* expr);

		vardecl* find_overload(const std::string& name, typing* typing, const std::vector<origin::typing*>& expected_params);

		virtual void walk(subscript* expr);
		virtual void walk(call_expr* expr);
		virtual void walk(bin_expr* expr);
		virtual void walk(un_expr* expr);

		virtual void walk(compilation_unit* unit);
	};

	/*class type_checker : public walker<void> {
	private:
		allocator memory;
		compilation_unit* unit;
		scope* current_scope;
		std::vector<diagnostic>& diagnostics;
	public:
		type_checker(std::vector<diagnostic>& diagnostics);
		~type_checker();

		void downscope();
		void upscope();

		virtual void walk(vardecl* stat);
		virtual void walk(expr_stat* stat);
		virtual void walk(block* stat);
		virtual void walk(return_stat* stat);

		virtual void walk(error_expr* expr);
		virtual void walk(lambda* expr);
		virtual void walk(parenthetical* expr);
		virtual void walk(int_literal* expr);
		virtual void walk(variable* expr);
		virtual void walk(member* expr);
		virtual void walk(subscript* expr);
		virtual void walk(call_expr* expr);
		virtual void walk(bin_expr* expr);
		virtual void walk(un_expr* expr);

		virtual void walk(classdef* classdef);
		virtual void walk(program* program);
		virtual void walk(compilation_unit* unit);
	};*/
}