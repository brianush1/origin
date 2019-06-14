#include "type_analysis.h"
#include <iostream>
#include <unordered_set>
#include <sstream>

using namespace std::string_literals;

namespace origin {
	static bool type_equals(typing* a, typing* b) {
		if (a == nullptr || b == nullptr) return false;
		if (a->templates.size() != b->templates.size()) return false;
		if (a->name != b->name) return false;
		for (size_t i = 0; i < a->templates.size(); ++i) {
			if (!type_equals(a->templates[i], b->templates[i])) return false;
		}
		return true;
	}

	scope::scope() : parent(nullptr) {
	}

	scope::scope(scope* parent) : parent(parent) {
	}

	bool scope::has(const std::string& name) {
		return variables.find(name) != variables.end();
	}

	typing* scope::get(const std::string& name) {
		if (variables.find(name) == variables.end()) {
			if (parent) {
				return parent->get(name);
			}
			else {
				return nullptr;
			}
		}
		else {
			return variables[name];
		}
	}

	void scope::declare(const std::string& name, typing* typing) {
		if (typing == nullptr) return;
		variables[name] = typing;
	}

	class templater : public walker<void> {
	private:
		allocator& memory;
		std::vector<diagnostic>& diagnostics;
		std::unordered_map<std::string, typing*>& map;
		std::string variadic;
		std::vector<typing*>& variadic_types;
	public:
		templater(allocator& memory, std::vector<diagnostic>& diagnostics,
			std::unordered_map<std::string, typing*>& map, const std::string& variadic, std::vector<typing*>& types)
			: memory(memory), diagnostics(diagnostics), map(map), variadic(variadic), variadic_types(types) {
		}

		void walk(typing* typing) {
			if (typing == nullptr) return;
			if (map.find(typing->name) != map.end()) {
				if (typing->templates.size() > 0) {
					diagnostics.push_back(error("template type cannot have templates of its own"s,
						typing->generic_token, typing->end));
				}
				else {
					typing->generic_token = typing->start;
				}
				auto change = map[typing->name];
				typing->alias_name = typing->name = change->name;
				typing->templates = change->templates;
			}
			else {
				bool can_have_more = true;
				std::vector<origin::typing*> templates;
				for (auto t : typing->templates) {
					if (!can_have_more) {
						diagnostics.push_back(error("cannot have more templates after variadic template"s, t->start, t->end));
					}
					else if (t->name == variadic) {
						if (t->templates.size() > 0) {
							diagnostics.push_back(error("template type cannot have templates of its own"s,
								t->generic_token, t->end));
						}
						else {
							t->generic_token = t->start;
						}
						can_have_more = false;
						for (auto s : variadic_types) {
							templates.push_back(s);
						}
					} else {
						walk(t);
						templates.push_back(t);
					}
				}
				typing->templates = templates;
			}
		}

		virtual void walk(vardecl* stat) {
			walk(stat->typing);
			if (stat->init_value != nullptr) {
				walk_expr(stat->init_value);
			}
		}

		virtual void walk(expr_stat* stat) {
			walk_expr(stat->expr);
		}

		virtual void walk(if_stat* stat) {
			walk_expr(stat->cond);
			walk_stat(stat->body);
			walk_stat(stat->else_body);
		}

		virtual void walk(block* stat) {
			for (auto s : stat->stats) {
				walk_stat(s);
			}
		}

		virtual void walk(return_stat* stat) {
			walk_expr(stat->expr);
		}

		virtual void walk(error_expr* expr) {
		}

		virtual void walk(lambda* expr) {
			walk(expr->return_type);
			bool can_have_more = true;
			std::vector<typing*> types;
			std::vector<std::string> names;
			size_t i = 0, k = 0;
			for (auto s : expr->param_types) {
				if (!can_have_more) {
					diagnostics.push_back(error("cannot have more parameters after variadic template"s, s->start, s->end));
				}
				else if (s->name == variadic) {
					can_have_more = false;
					for (auto t : variadic_types) {
						types.push_back(t);
						std::ostringstream str;
						str << expr->param_names[i] << "..." << k++;
						names.push_back(str.str());
					}
				} else {
					walk(s);
					types.push_back(s);
					names.push_back(expr->param_names[i++]);
				}
			}
			expr->param_names = names;
			expr->param_types = types;
			walk(expr->block);
			auto typing = memory.allocate<origin::typing>();
			typing->alias_name = typing->name = "stdlib::core::function";
			typing->templates.push_back(expr->return_type);
			for (auto type : expr->param_types) {
				typing->templates.push_back(type);
			}
			expr->typing = typing;
		}

		virtual void walk(parenthetical* expr) {
			walk_expr(expr->expr);
		}

		virtual void walk(int_literal* expr) {
		}

		virtual void walk(variable* expr) {
		}

		virtual void walk(member* expr) {
			walk_expr(expr->object);
		}

		virtual void walk(subscript* expr) {
			walk_expr(expr->left);
			walk_expr(expr->right);
		}

		virtual void walk(call_expr* expr) {
			walk_expr(expr->function);
			for (auto s : expr->args) {
				walk_expr(s);
			}
		}

		virtual void walk(bin_expr* expr) {
			walk_expr(expr->left);
			walk_expr(expr->right);
		}

		virtual void walk(un_expr* expr) {
			walk_expr(expr->expr);
		}

		virtual void walk(compilation_unit* unit) {
		}
	};

	class cloner {
	private:
		allocator& memory;
	public:
		cloner(allocator& memory) : memory(memory) {
		}

		typing* walk(typing* typing) {
			if (typing == nullptr) return nullptr;
			auto result = memory.allocate<origin::typing>();
			result->start = typing->start;
			result->end = typing->end;
			result->generic_token = typing->generic_token;
			result->alias_name = result->name = typing->name;
			std::vector<origin::typing*> clone;
			for (auto t : typing->templates) {
				clone.push_back(walk(t));
			}
			result->templates = clone;
			return result;
		}

		expr* walk_expr(expr* expr) {
			if (auto error_expr = dynamic_cast<origin::error_expr*>(expr)) {
				return walk(error_expr);
			}
			else if (auto lambda_expr = dynamic_cast<lambda*>(expr)) {
				return walk(lambda_expr);
			}
			else if (auto parenthetical_expr = dynamic_cast<parenthetical*>(expr)) {
				return walk(parenthetical_expr);
			}
			else if (auto int_literal_expr = dynamic_cast<int_literal*>(expr)) {
				return walk(int_literal_expr);
			}
			else if (auto variable_expr = dynamic_cast<variable*>(expr)) {
				return walk(variable_expr);
			}
			else if (auto member_expr = dynamic_cast<member*>(expr)) {
				return walk(member_expr);
			}
			else if (auto subscript_expr = dynamic_cast<subscript*>(expr)) {
				return walk(subscript_expr);
			}
			else if (auto call_expr = dynamic_cast<origin::call_expr*>(expr)) {
				return walk(call_expr);
			}
			else if (auto bin_expr = dynamic_cast<origin::bin_expr*>(expr)) {
				return walk(bin_expr);
			}
			else if (auto un_expr = dynamic_cast<origin::un_expr*>(expr)) {
				return walk(un_expr);
			}
			else {
				throw std::exception();
			}
		}

		stat* walk_stat(stat* stat) {
			if (auto vardecl_stat = dynamic_cast<vardecl*>(stat)) {
				return walk(vardecl_stat);
			}
			else if (auto expr_stat = dynamic_cast<origin::expr_stat*>(stat)) {
				return walk(expr_stat);
			}
			else if (auto if_stat = dynamic_cast<origin::if_stat*>(stat)) {
				return walk(if_stat);
			}
			else if (auto block_stat = dynamic_cast<block*>(stat)) {
				return walk(block_stat);
			}
			else if (auto return_stat = dynamic_cast<origin::return_stat*>(stat)) {
				return walk(return_stat);
			}
			else {
				throw std::exception();
			}
		};

		vardecl* walk(vardecl* stat) {
			auto result = memory.allocate<vardecl>();
			result->start = stat->start;
			result->end = stat->end;
			if (stat->init_value != nullptr) {
				result->init_value = walk_expr(stat->init_value);
			}
			result->typing = walk(stat->typing);
			result->variable = stat->variable;
			result->var_token = stat->var_token;
			return result;
		}

		expr_stat* walk(expr_stat* stat) {
			auto result = memory.allocate<expr_stat>();
			result->start = stat->start;
			result->end = stat->end;
			result->expr = walk_expr(stat->expr);
			return result;
		}

		if_stat* walk(if_stat* stat) {
			auto result = memory.allocate<if_stat>();
			result->start = stat->start;
			result->end = stat->end;
			result->cond = walk_expr(stat->cond);
			result->body = walk_stat(stat->body);
			result->else_body = walk_stat(stat->else_body);
			return result;
		}

		block* walk(block* stat) {
			auto result = memory.allocate<block>();
			result->start = stat->start;
			result->end = stat->end;
			std::vector<origin::stat*> clone;
			for (auto t : stat->stats) {
				clone.push_back(walk_stat(t));
			}
			result->stats = clone;
			return result;
		}

		return_stat* walk(return_stat* stat) {
			auto result = memory.allocate<return_stat>();
			result->start = stat->start;
			result->end = stat->end;
			result->expr = walk_expr(stat->expr);
			return result;
		}

		error_expr* walk(error_expr* expr) {
			auto result = memory.allocate<error_expr>();
			result->start = expr->start;
			result->end = expr->end;
			result->typing = walk(expr->typing);
			return result;
		}

		lambda* walk(lambda* expr) {
			auto result = memory.allocate<lambda>();
			result->start = expr->start;
			result->end = expr->end;
			result->block = walk(expr->block);
			result->return_type = walk(expr->return_type);
			result->param_names = expr->param_names;
			std::vector<typing*> clone;
			for (auto t : expr->param_types) {
				clone.push_back(walk(t));
			}
			result->param_types = clone;
			result->typing = walk(expr->typing);
			return result;
		}

		parenthetical* walk(parenthetical* expr) {
			auto result = memory.allocate<parenthetical>();
			result->start = expr->start;
			result->end = expr->end;
			result->expr = walk_expr(expr->expr);
			result->typing = walk(expr->typing);
			return result;
		}

		int_literal* walk(int_literal* expr) {
			auto result = memory.allocate<int_literal>();
			result->start = expr->start;
			result->end = expr->end;
			result->value = expr->value;
			result->typing = walk(expr->typing);
			return result;
		}

		variable* walk(variable* expr) {
			auto result = memory.allocate<variable>();
			result->start = expr->start;
			result->end = expr->end;
			result->name = expr->name;
			result->typing = walk(expr->typing);
			return result;
		}

		member* walk(member* expr) {
			auto result = memory.allocate<member>();
			result->start = expr->start;
			result->end = expr->end;
			result->object = walk_expr(expr->object);
			result->name = expr->name;
			result->name_token = expr->name_token;
			result->typing = walk(expr->typing);
			return result;
		}

		subscript* walk(subscript* expr) {
			auto result = memory.allocate<subscript>();
			result->start = expr->start;
			result->end = expr->end;
			result->left = walk_expr(expr->left);
			result->right = walk_expr(expr->right);
			result->typing = walk(expr->typing);
			return result;
		}

		call_expr* walk(call_expr* expr) {
			auto result = memory.allocate<call_expr>();
			result->start = expr->start;
			result->end = expr->end;
			result->typing = walk(expr->typing);
			result->function = walk_expr(expr->function);
			std::vector<origin::expr*> clone;
			for (auto t : expr->args) {
				clone.push_back(walk_expr(t));
			}
			result->args = clone;
			return result;
		}

		bin_expr* walk(bin_expr* expr) {
			auto result = memory.allocate<bin_expr>();
			result->start = expr->start;
			result->end = expr->end;
			result->left = walk_expr(expr->left);
			result->right = walk_expr(expr->right);
			result->op = expr->op;
			result->op_token = expr->op_token;
			result->typing = walk(expr->typing);
			return result;
		}

		un_expr* walk(un_expr* expr) {
			auto result = memory.allocate<un_expr>();
			result->start = expr->start;
			result->end = expr->end;
			result->op = expr->op;
			result->expr = walk_expr(expr->expr);
			result->typing = walk(expr->typing);
			return result;
		}

		void walk(compilation_unit* unit) {
		}
	};

	type_assigner::type_assigner(std::vector<diagnostic>& diagnostics)
		: current_scope(new scope()), diagnostics(diagnostics) {
	}

	type_assigner::~type_assigner() {
		while (current_scope != nullptr) upscope();
	}

	void type_assigner::downscope() {
		current_scope = new scope(current_scope);
	}

	void type_assigner::upscope() {
		auto old = current_scope;
		current_scope = old->parent;
		delete old;
	}
	
	static std::string typing2str(typing* typing) {
		if (typing->alias_name == typing->name && typing->name == "stdlib::core::array" && typing->templates.size() == 1) {
			return typing2str(typing->templates[0]) + "[]";
		}
		std::ostringstream result;
		result << typing->alias_name;
		if (typing->templates.size() > 0 && !typing->alias) {
			result << "<";
			bool first = true;
			for (auto t : typing->templates) {
				if (first) {
					first = false;
				}
				else {
					result << ", ";
				}
				result << typing2str(t);
			}
			result << ">";
		}
		return result.str();
	}

	classdef* type_assigner::find_class(typing* typing) {
		if (typing == nullptr) return nullptr;
		if (classes.find(typing->name) == classes.end()) return nullptr;
		auto result = classes[typing->name];
		if (result->generics.size() > 0) {
			if (generic_classes.find(typing) != generic_classes.end()) {
				return generic_classes[typing];
			}
			size_t size = result->generics.size() - (result->variadic ? 1 : 0);
			if (!(typing->templates.size() == result->generics.size() ||
				(result->variadic && typing->templates.size() >= result->generics.size() - 1))) {
				return nullptr;
			}
			auto old_program = current_program;
			auto old_scope = current_scope;
			current_scope = new scope();
			current_program = result->program;
			current_template.push_back(typing);
			template_str = typing2str(current_template.back());
			auto clone = memory.allocate<classdef>();
			cloner c(memory);
			clone->accesses = result->accesses;
			clone->generics = result->generics;
			clone->name = result->name;
			clone->name_token = result->name_token;
			clone->program = result->program;
			clone->variadic = result->variadic;
			for (auto stat : result->vardecls) {
				clone->vardecls.push_back(c.walk(stat));
			}
			std::unordered_map<std::string, origin::typing*> map;
			for (size_t i = 0; i < size; ++i) {
				auto generic_t = result->generics[i];
				auto template_t = typing->templates[i];
				map[generic_t] = template_t;
			}
			std::vector<origin::typing*> types;
			if (result->variadic) {
				for (size_t i = size; i < typing->templates.size(); ++i) {
					types.push_back(typing->templates[i]);
				}
			}
			templater t(memory, diagnostics, map, result->variadic ? result->generics.back() : ""s, types);
			downscope();
			generic_classes[typing] = clone;
			current_scope->declare("self", typing);
			for (auto stat : clone->vardecls) {
				if (stat->init_value) t.walk_expr(stat->init_value);
				if (stat->typing != nullptr) {
					t.walk(stat->typing);
				}
			}
			for (auto stat : clone->vardecls) {
				if (stat->typing != nullptr) {
					patch(stat->typing);
				}
			}
			for (auto stat : clone->vardecls) {
				if (stat->init_value) walk_expr(stat->init_value);
			}
			upscope();
			current_scope = old_scope;
			current_program = old_program;
			current_template.pop_back();
			if (current_template.size() == 0) {
				template_str = "";
			} else {
				template_str = typing2str(current_template.back());
			}
			return clone;
		}
		else {
			return result;
		}
	}

	typing* type_assigner::patch(typing* typing) {
		if (typing == nullptr) return typing;
		for (auto t : typing->templates) {
			patch(t);
		}
		std::vector<std::string> names;
		if (typing->name.find("::"s) != std::string::npos) {
			names.push_back(typing->name);
		}
		else {
			names.push_back(current_program->namespace_name + "::" + typing->name);
			for (auto s : current_program->imports) {
				names.push_back(s->name + "::" + typing->name);
			}
		}
		for (auto name : names) {
			if (typedefs.find(name) != typedefs.end()) {
				auto res = patch(typedefs[name]);
				typing->start = res->start;
				typing->end = res->end;
				typing->alias = true;
				typing->generic_token = res->generic_token;
				typing->name = res->name;
				typing->templates = res->templates;
				return typing;
			}
		}
		for (auto name : names) {
			if (classes.find(name) != classes.end()) {
				auto classdef = classes[name];
				if (classdef->generics.size() > 0) {
					if (!(typing->templates.size() == classdef->generics.size()
						|| (classdef->variadic && typing->templates.size() >= classdef->generics.size() - 1))) {
						diagnostics.push_back(error("incorrect number of template types"s, typing->generic_token, typing->end));
					}
				}
				else {
					if (typing->templates.size() > 0) {
						diagnostics.push_back(error("template types don't belong on a non-generic type"s,
							typing->generic_token, typing->end));
					}
				}
				typing->name = name;
				return typing;
			}
		}
		diagnostics.push_back(error("unknown type "s + typing->name, typing->start,
			typing->end));
		return typing;
	}

	void type_assigner::walk(vardecl* stat) {
		if (stat->init_value) walk_expr(stat->init_value);
		if (current_scope->has(stat->variable)) {
			diagnostics.push_back(warn("duplicate variable declaration"s, stat->var_token));
		}
		current_scope->declare(stat->variable, patch(stat->typing));
	}

	void type_assigner::walk(expr_stat* stat) {
		walk_expr(stat->expr);
	}

	void type_assigner::walk(if_stat* stat) {
		walk_expr(stat->cond);
		walk_stat(stat->body);
		walk_stat(stat->else_body);
	}

	void type_assigner::walk(block* stat) {
		downscope();
		for (auto s : stat->stats) {
			walk_stat(s);
		}
		upscope();
	}

	void type_assigner::walk(return_stat* stat) {
		walk_expr(stat->expr);
	}

	void type_assigner::walk(error_expr* expr) {
		auto typing = memory.allocate<origin::typing>();
		typing->alias_name = typing->name = "<error type>";
		expr->typing = typing;
	}

	void type_assigner::walk(lambda* expr) {
		downscope();
		for (size_t i = 0; i < expr->param_names.size(); ++i) {
			patch(expr->param_types[i]);
			current_scope->declare(expr->param_names[i], expr->param_types[i]);
		}
		walk(expr->block);
		upscope();
		auto typing = memory.allocate<origin::typing>();
		typing->alias_name = typing->name = "stdlib::core::function";
		typing->templates.push_back(patch(expr->return_type));
		for (auto type : expr->param_types) {
			typing->templates.push_back(type);
		}
		expr->typing = typing;
	}

	void type_assigner::walk(parenthetical* expr) {
		walk_expr(expr->expr);
		expr->typing = expr->expr->typing;
	}

	void type_assigner::walk(int_literal* expr) {
		auto int_type = memory.allocate<typing>();
		int_type->alias_name = int_type->name = "stdlib::core::int64";
		expr->typing = int_type;
	}

	void type_assigner::walk(variable* expr) {
		if (typing* typing = current_scope->get(expr->name)) {
			expr->typing = typing;
		} else {
			diagnostics.push_back(error("undefined variable"s, expr->start, expr->end));
			typing = memory.allocate<origin::typing>();
			typing->alias_name = typing->name = "<error type>";
			expr->typing = typing;
		}
	}

	void type_assigner::walk(member* expr) {
		walk_expr(expr->object);
		if (classdef* classdef = find_class(expr->object->typing)) {
			size_t i = 0;
			for (auto decl : classdef->vardecls) {
				auto access = classdef->accesses[i++];
				bool accessible = access == public_access || classdef->program == current_program;
				if (decl->variable == expr->name && accessible) {
					expr->typing = decl->typing;
					return;
				}
			}
		}
		diagnostics.push_back(error("undefined member '"s + expr->name + "'",
			expr->name_token));
	}

	static void* bad_ptr = (void*)(uintptr_t)(-1);

	vardecl* type_assigner::find_overload(const std::string& name, typing* typing,
		const std::vector<origin::typing*>& expected_params) {
		bool found_overload = false;
		if (classdef* classdef = find_class(typing)) {
			size_t i = 0;
			for (auto decl : classdef->vardecls) {
				auto access = classdef->accesses[i++];
				bool accessible = access == public_access || classdef->program == current_program;
				if (decl->variable == name && accessible && decl->typing->name == "stdlib::core::function") {
					found_overload = true;
					origin::typing* return_type = decl->typing->templates[0];
					std::vector<origin::typing*> param_types;
					auto list = decl->typing->templates;
					for (size_t i = 1; i < list.size(); ++i) param_types.push_back(list[i]);
					if (param_types.size() != expected_params.size()) continue;
					for (size_t i = 0; i < param_types.size(); ++i) {
						if (!type_equals(param_types[i], expected_params[i])) goto cont;
					}
					return decl;
				}
			cont:;
			}
		}
		return found_overload ? (vardecl*)bad_ptr : nullptr;
	}

	void type_assigner::walk(subscript* expr) {
		walk_expr(expr->left);
		walk_expr(expr->right);
		vardecl* overload = find_overload("operator["s, expr->left->typing, std::vector<origin::typing*>({ expr->right->typing }));
		if (overload == bad_ptr) {
			diagnostics.push_back(error("no overload of member operator[] matches the given parameters"s,
				expr->left->start, expr->left->end));
		}
		else if (overload == nullptr) {
			diagnostics.push_back(error("undefined member operator[]"s,
				expr->left->start, expr->left->end));
		}
		else {
			expr->typing = overload->typing->templates[0];
		}
	}

	void type_assigner::walk(call_expr* expr) {
		walk_expr(expr->function);
		for (auto s : expr->args) {
			walk_expr(s);
		}
		std::vector<typing*> types;
		for (auto arg : expr->args) {
			types.push_back(arg->typing);
		}
		vardecl* overload = find_overload("operator("s, expr->function->typing, types);
		if (overload == bad_ptr) {
			diagnostics.push_back(error("no overload of member operator() matches the given parameters"s,
				expr->function->start, expr->function->end));
		}
		else if (overload == nullptr) {
			diagnostics.push_back(error("undefined member operator()"s,
				expr->function->start, expr->function->end));
		}
		else {
			expr->typing = overload->typing->templates[0];
		}
	}

	void type_assigner::walk(bin_expr* expr) {
		if (expr->op == "=") {
			variable* var;
			member* mem;
			if ((var = dynamic_cast<variable*>(expr->left)) || (mem = dynamic_cast<member*>(expr->left))) {
				walk_expr(expr->left);
				walk_expr(expr->right);
				if (!type_equals(expr->left->typing, expr->right->typing)) {
					diagnostics.push_back(error("type mismatch"s,
						expr->left->start, expr->left->end));
				}
				expr->typing = expr->left->typing;
			}
			else if (auto subs = dynamic_cast<subscript*>(expr->left)) {
				walk_expr(subs->left);
				walk_expr(subs->right);
				walk_expr(expr->right);
				vardecl* overload = find_overload("operator[="s, subs->left->typing, std::vector<origin::typing*>({
					subs->right->typing, expr->right->typing }));
				if (overload == bad_ptr) {
					diagnostics.push_back(error("no overload of member operator[]= matches the given parameters"s,
						expr->left->start, expr->left->end));
				}
				else if (overload == nullptr) {
					diagnostics.push_back(error("undefined member operator[]="s,
						expr->left->start, expr->left->end));
				}
				else {
					expr->typing = overload->typing->templates[0];
				}
			}
			else {
				walk_expr(expr->right);
				diagnostics.push_back(error("expected lvalue"s,
					expr->left->start, expr->left->end));
			}
			return;
		}
		walk_expr(expr->left);
		walk_expr(expr->right);
		vardecl* overload = find_overload("operator"s + expr->op, expr->left->typing, std::vector<origin::typing*>({
			expr->right->typing }));
		if (overload == bad_ptr) {
			diagnostics.push_back(error("no overload of member operator"s + expr->op + " matches the given parameters"s,
				expr->left->start, expr->left->end));
		}
		else if (overload == nullptr) {
			diagnostics.push_back(error("undefined member operator"s + expr->op,
				expr->left->start, expr->left->end));
		}
		else {
			expr->typing = overload->typing->templates[0];
		}
	}

	void type_assigner::walk(un_expr* expr) {
		walk_expr(expr->expr);
		vardecl* overload = find_overload("operator"s + expr->op, expr->expr->typing, {});
		if (overload == bad_ptr) {
			diagnostics.push_back(error("no overload of member operator"s + expr->op + " matches the given parameters"s,
				expr->expr->start, expr->expr->end));
		}
		else if (overload == nullptr) {
			diagnostics.push_back(error("undefined member operator"s + expr->op,
				expr->expr->start, expr->expr->end));
		}
		else {
			expr->typing = overload->typing->templates[0];
		}
	}

	void type_assigner::walk(compilation_unit* unit) {
		this->unit = unit;
		std::unordered_set<std::string> namespaces;
		for (auto program : *unit) {
			namespaces.emplace(program->namespace_name);
			for (auto s : program->typedefs) {
				typedefs[program->namespace_name + "::" + s.first] = s.second;
			}
		}
		for (auto program : *unit) {
			for (auto s : program->imports) {
				if (namespaces.find(s->name) == namespaces.end()) {
					diagnostics.push_back(error("unknown namespace"s, s->start, s->end));
				}
			}
		}
		for (auto program : *unit) {
			current_program = program;
			for (auto classdef : program->classes) {
				classdef->program = program;
				std::string& ns = current_program->namespace_name;
				std::string name = ns + "::" + classdef->name;
				if (classes.find(name) == classes.end()) {
					classes[name] = classdef;
				}
				else {
					auto other = classes[name]->name_token;
					diagnostics.push_back(error("conflicting class definitions"s, other));
					diagnostics.push_back(error("conflicting class definitions"s,
						classdef->name_token));
				}
			}
		}
		for (auto program : *unit) {
			current_program = program;
			for (auto classdef : program->classes) {
				if (classdef->generics.size() == 0) {
					for (auto stat : classdef->vardecls) {
						if (stat->typing != nullptr) {
							patch(stat->typing);
						}
					}
				}
			}
		}
		for (auto program : *unit) {
			current_program = program;
			downscope();
			for (auto s : program->vardecls) {
				current_scope->declare(s->variable, s->typing);
			}
			for (auto classdef : program->classes) {
				if (classdef->generics.size() == 0) {
					for (auto stat : classdef->vardecls) {
						if (stat->init_value) walk_expr(stat->init_value);
					}
				}
			}
			downscope(); // we want to prevent "duplicate variable declaration" messages
			// so we define an outer scope for hoisting purposes, and an inner scope for actual definitions
			// "but won't that refer to an uninitialized variable?" you ask?
			// yes, but this is just a type analyzer; our compiler will take care of that
			for (auto s : program->vardecls) {
				walk(s);
			}
			upscope();
			upscope();
		}
	}
}