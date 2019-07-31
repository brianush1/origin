#include "odef.h"
#include <sstream>

namespace origin {
	static inline void write_uint8(std::ostream& str, uint8_t data) {
		str.put((char)data);
	}

	static inline void write_uint16(std::ostream& str, uint16_t data) {
		write_uint8(str, data & 0xFF);
		write_uint8(str, data >> 8);
	}

	static inline void write_uint32(std::ostream& str, uint32_t data) {
		write_uint16(str, data & 0xFFFF);
		write_uint16(str, data >> 16);
	}

	static inline void write_uint64(std::ostream& str, uint64_t data) {
		write_uint32(str, data & 0xFFFFFFFF);
		write_uint32(str, data >> 32);
	}

	static inline void write_str(std::ostream& str, const std::string& s) {
		write_uint64(str, s.size());
		str << s;
	}

	static inline void encode_typing(std::ostream& str, typing* type) {
		write_str(str, type->name);
		write_uint64(str, type->templates.size());
		for (auto& t : type->templates) {
			encode_typing(str, t);
		}
	}

	static inline void encode_vardecl(std::ostream& str, vardecl* var) {
		write_str(str, var->variable);
		if (var->type != nullptr) { // constructors and destructors are nullptr types
			write_uint8(str, 1);
			encode_typing(str, var->type);
		}
		else {
			write_uint8(str, 0);
		}
	}

	void encode(std::ostream& ostr, program* prog) {
		std::ostringstream str;
		write_str(str, prog->namespace_name);
		write_uint64(str, prog->imports.size());
		for (auto& i : prog->imports) write_str(str, i->name);
		write_uint64(str, prog->vardecls.size());
		for (auto& i : prog->vardecls) encode_vardecl(str, i);
		write_uint64(str, prog->classes.size());
		for (auto& i : prog->classes) {
			write_str(str, i->name);
			write_uint8(str, i->is_struct ? 1 : 0);
			write_uint8(str, i->variadic ? 1 : 0);
			write_uint64(str, i->generics.size());
			for (auto& g : i->generics) write_str(str, g);
			write_uint64(str, i->vardecls.size());
			for (size_t k = 0; k < i->vardecls.size(); ++k) {
				encode_vardecl(str, i->vardecls[k]);
				switch (i->accesses[k]) {
				case access::private_access:
					write_uint8(str, 0);
					break;
				case access::public_access:
					write_uint8(str, 1);
					break;
				}
			}
		}
		write_uint64(str, prog->aliases.size());
		for (auto& i : prog->aliases) {
			write_str(str, i.first);
			encode_typing(str, i.second);
		}
		ostr << "Org" << (char)0x92;
		std::string final = str.str();
		write_uint64(ostr, final.size());
		ostr << final;
	}

	program* decode(std::istream& str, allocator& alloc) {

	}
}