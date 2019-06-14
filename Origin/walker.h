#pragma once
#include "ast.h"

namespace origin {
	template<class T>
	class walker {
	public:

		T walk_expr(expr* expr) {
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
		
		T walk_stat(stat* stat) {
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

		virtual T walk(vardecl* stat) = 0;
		virtual T walk(expr_stat* stat) = 0;
		virtual T walk(if_stat* stat) = 0;
		virtual T walk(block* stat) = 0;
		virtual T walk(return_stat* stat) = 0;

		virtual T walk(error_expr* expr) = 0;
		virtual T walk(lambda* expr) = 0;
		virtual T walk(parenthetical* expr) = 0;
		virtual T walk(int_literal* expr) = 0;
		virtual T walk(variable* expr) = 0;
		virtual T walk(member* expr) = 0;
		virtual T walk(subscript* expr) = 0;
		virtual T walk(call_expr* expr) = 0;
		virtual T walk(bin_expr* expr) = 0;
		virtual T walk(un_expr* expr) = 0;

		virtual T walk(compilation_unit* unit) = 0;
	};
}