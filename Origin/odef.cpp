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

	static inline uint8_t read_uint8(std::istream& str) {
		return str.get();
	}

	static inline uint16_t read_uint16(std::istream& str) {
		auto a = read_uint8(str);
		auto b = read_uint8(str);
		return a | (b << 8);
	}

	static inline uint32_t read_uint32(std::istream& str) {
		auto a = read_uint16(str);
		auto b = read_uint16(str);
		return a | (b << 16);
	}

	static inline uint64_t read_uint64(std::istream& str) {
		auto a = read_uint32(str);
		auto b = read_uint32(str);
		return a | (b << 32);
	}

	static inline std::string read_str(std::istream& str) {
		uint64_t length = read_uint64(str);
		char data[length + 1];
		str.read(data, length);
		data[length] = 0;
		return data;
	}

	static inline typing* decode_typing(std::istream& str, allocator& memory) {
		auto result = memory.allocate<typing>();
		result->alias_name = result->name = read_str(str);
		auto size = read_uint64(str);
		for (uint64_t i = 0; i < size; ++i) {
			result->templates.push_back(decode_typing(str, memory));
		}
		return result;
	}

	static inline vardecl* decode_vardecl(std::istream& str, allocator& memory) {
		auto result = memory.allocate<vardecl>();
		result->variable = read_str(str);
		if (read_uint8(str) == 1) {
			result->type = decode_typing(str, memory);
		}
		else {
			result->type = nullptr;
		}
		result->init_value = nullptr;
		return result;
	}

	program* decode(std::istream& str, allocator& memory) {
		str.seekg(0, std::ios_base::end);
		uint64_t length = str.tellg();
		str.seekg(0, std::ios_base::beg);
		if (length < 12) return nullptr;
		char sign[5];
		str.read(sign, 4);
		sign[4] = 0;
		std::ostringstream rsign;
		rsign << "Org" << (char)0x92;
		if (sign != rsign.str()) return nullptr;
		read_uint64(str);
		auto result = memory.allocate<program>();
		result->namespace_name = read_str(str);
		for (uint64_t size = read_uint64(str), i = 0; i < size; ++i) {
			auto import = memory.allocate<variable>();
			import->name = read_str(str);
			result->imports.push_back(import);
		}
		for (uint64_t size = read_uint64(str), i = 0; i < size; ++i) {
			result->vardecls.push_back(decode_vardecl(str, memory));
		}
		for (uint64_t size = read_uint64(str), i = 0; i < size; ++i) {
			auto def = memory.allocate<classdef>();
			def->name = read_str(str);
			def->is_struct = read_uint8(str) == 1;
			def->variadic = read_uint8(str) == 1;
			for (uint64_t size = read_uint64(str), i = 0; i < size; ++i) {
				def->generics.push_back(read_str(str));
			}
			for (uint64_t size = read_uint64(str), i = 0; i < size; ++i) {
				def->vardecls.push_back(decode_vardecl(str, memory));
				switch (read_uint8(str)) {
				case 0:
					def->accesses.push_back(access::private_access);
					break;
				case 1:
					def->accesses.push_back(access::public_access);
					break;
				}
			}
			result->classes.push_back(def);
		}
		for (uint64_t size = read_uint64(str), i = 0; i < size; ++i) {
			auto key = read_str(str);
			auto value = decode_typing(str, memory);
			result->aliases[key] = value;
		}
		return result;
	}
}