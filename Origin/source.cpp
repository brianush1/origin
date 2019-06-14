#include <sstream>
#include <iostream>
#include "lexer.h"
#include "parser.h"
#include "type_analysis.h"
#include "rang.h"

using namespace std::string_literals;

std::string replaceAll(std::string str, std::string from, std::string to) {
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos) {
		return str;
	}
	return replaceAll(str.replace(start_pos, from.length(), to), from, to);
}

int main() {
	std::istringstream prog(replaceAll(R":<(namespace asd::def::ghi;
import stdlib::core;

class clessy<T...> {
	int a;
	int b;
public:
	string x;
	function<int, int, T> cool;

	clessy() {
	}

	int asd(int x) {
		return x + 2;
	}

	~clessy() {
	}
}

int fib(int y) {
	return fib(y - 1) + fib(y - 2);
}

void main(int x) {
	clessy<int, string> lol;
	char[] mi;
	lol.a; // works in the same file
	lol.cool(2 <<~ 1, 3, mi);
	lol.asd(1);
	int[] asd;
	asd[4] = 3;
	//asd[8] = lol;
	asd[6];
	clessy<int, string>[] def;
	//def[4] = 3;
	def[8] = lol;
	def[6];
	int @return;
	@return = -2;
	return 23;
}
):<"s, "\t"s, "    "s));
	std::istringstream stdprog(replaceAll(R":<(namespace stdlib::core;

// Common aliases
alias sbyte = int8;
alias short = int16;
alias half = int32;
alias int = int64;
alias long = int128;

alias byte = uint8;
alias ushort = uint16;
alias uhalf = uint32;
alias uint = uint64;
alias ulong = uint128;

alias char = ushort;
alias string = char[];

alias void = null; // unlike other languages, we don't distinguish between null and void

struct null {}

// Integral structures
struct int8 {
public:
	int8 operator+() {}
	int8 operator-() {}
	int8 operator+(int8 x) {}
	int8 operator-(int8 x) {}
	int8 operator*(int8 x) {}
	int8 operator/(int8 x) {}
	int8 operator%(int8 x) {}
	int8 operator<<~(int8 x) {}
	int8 operator~>>(int8 x) {}
}
struct int16 {
public:
	int16 operator+() {}
	int16 operator-() {}
	int16 operator+(int16 x) {}
	int16 operator-(int16 x) {}
	int16 operator*(int16 x) {}
	int16 operator/(int16 x) {}
	int16 operator%(int16 x) {}
	int16 operator<<~(int16 x) {}
	int16 operator~>>(int16 x) {}
}
struct int32 {
public:
	int32 operator+() {}
	int32 operator-() {}
	int32 operator+(int32 x) {}
	int32 operator-(int32 x) {}
	int32 operator*(int32 x) {}
	int32 operator/(int32 x) {}
	int32 operator%(int32 x) {}
	int32 operator<<~(int32 x) {}
	int32 operator~>>(int32 x) {}
}
struct int64 {
public:
	int64 operator+() {}
	int64 operator-() {}
	int64 operator+(int64 x) {}
	int64 operator-(int64 x) {}
	int64 operator*(int64 x) {}
	int64 operator/(int64 x) {}
	int64 operator%(int64 x) {}
	int64 operator<<~(int64 x) {}
	int64 operator~>>(int64 x) {}
}
struct int128 {
public:
	int128 operator+() {}
	int128 operator-() {}
	int128 operator+(int128 x) {}
	int128 operator-(int128 x) {}
	int128 operator*(int128 x) {}
	int128 operator/(int128 x) {}
	int128 operator%(int128 x) {}
	int128 operator<<~(int128 x) {}
	int128 operator~>>(int128 x) {}
}
struct uint8 {
public:
	uint8 operator+() {}
	uint8 operator-() {}
	uint8 operator+(uint8 x) {}
	uint8 operator-(uint8 x) {}
	uint8 operator*(uint8 x) {}
	uint8 operator/(uint8 x) {}
	uint8 operator%(uint8 x) {}
	uint8 operator<<~(uint8 x) {}
	uint8 operator~>>(uint8 x) {}
}
struct uint16 {
public:
	uint16 operator+() {}
	uint16 operator-() {}
	uint16 operator+(uint16 x) {}
	uint16 operator-(uint16 x) {}
	uint16 operator*(uint16 x) {}
	uint16 operator/(uint16 x) {}
	uint16 operator%(uint16 x) {}
	uint16 operator<<~(uint16 x) {}
	uint16 operator~>>(uint16 x) {}
}
struct uint32 {
public:
	uint32 operator+() {}
	uint32 operator-() {}
	uint32 operator+(uint32 x) {}
	uint32 operator-(uint32 x) {}
	uint32 operator*(uint32 x) {}
	uint32 operator/(uint32 x) {}
	uint32 operator%(uint32 x) {}
	uint32 operator<<~(uint32 x) {}
	uint32 operator~>>(uint32 x) {}
}
struct uint64 {
public:
	uint64 operator+() {}
	uint64 operator-() {}
	uint64 operator+(uint64 x) {}
	uint64 operator-(uint64 x) {}
	uint64 operator*(uint64 x) {}
	uint64 operator/(uint64 x) {}
	uint64 operator%(uint64 x) {}
	uint64 operator<<~(uint64 x) {}
	uint64 operator~>>(uint64 x) {}
}
struct uint128 {
public:
	uint128 operator+() {}
	uint128 operator-() {}
	uint128 operator+(uint128 x) {}
	uint128 operator-(uint128 x) {}
	uint128 operator*(uint128 x) {}
	uint128 operator/(uint128 x) {}
	uint128 operator%(uint128 x) {}
	uint128 operator<<~(uint128 x) {}
	uint128 operator~>>(uint128 x) {}
}

struct function<T, Args...> {
public:
	T operator()(Args args) {}
}

// not valid syntax yet, but this is how function overloads will be done:
// (with the necessary syntax sugar, of course, cuz this is just ugly)
/*struct function_combinator<A : function<ARet, AArgs...>, B : function<BRet, BArgs...>> {
private:
	A fn1;
	B fn2;
public:
	// we make use of the fact that operators can be overloaded
	// but functions are just objects, so we can't overload them as easily
	ARet operator()(AArgs args) {}
	BRet operator()(BArgs args) {}
}*/

struct array<T> {
public:
	int length;

	array(int length) {}

	T operator[](int index) {}
	T operator[]=(int index, T value) {}

	T[] slice(int start, int end) {
		// TODO: (and I can't believe this isn't done) if statements
		//if (end < 0) {
			end = self.length + end + 1;
		//}
	}
}
):<"s, "\t"s, "    "s));
	std::unordered_map<std::istream*, std::string> files = {
		{&prog, "file.og"},
		{&stdprog, "stdlib/core.og"}
	};
	std::vector<origin::diagnostic> diagnostics;
	origin::lexer lex1(prog, diagnostics);
	origin::parser pr1(lex1, diagnostics);
	origin::lexer lex2(stdprog, diagnostics);
	origin::parser pr2(lex2, diagnostics);
	origin::compilation_unit unit;
	unit.push_back(pr1.read_program());
	unit.push_back(pr2.read_program());
	auto assigner = origin::type_assigner(diagnostics);
	assigner.walk(&unit);
	//origin::type_checker(diagnostics).walk(&unit);
	for (origin::diagnostic d : diagnostics) {
		std::istream& prog = *d.stream;
		if (d.stream == nullptr) continue;
		prog.seekg(0);
		std::string str;
		size_t start_line = (size_t)-1;
		size_t end_line;
		std::string lines;
		size_t index = 0, end_index = 0;
		size_t ifirst;
		size_t isecond;
		size_t line = 0;
		while (getline(prog, str)) {
			line++;
			index = end_index;
			end_index += str.length() + 1;
			if (d.start >= index && d.start < end_index) {
				start_line = line;
				ifirst = d.start - index;
			}
			if (start_line != (size_t)-1) lines += str + "\n";
			if (d.end >= index && d.end < end_index) {
				end_line = line;
				isecond = d.end - index;
				break;
			}
		}
		std::istringstream strs(lines);
		size_t i = start_line;
		std::cout << rang::style::bold << (d.warning ? "warning" : "error") << "@"
			<< files[d.stream] << " on line" << (start_line == end_line ? " " : "s ")
			<< start_line;
		if (start_line != end_line) {
			std::cout << "-" << end_line;
		}
		if (d.template_str != "") {
			std::cout << " while evaluating template " << d.template_str;
		}
		std::cout << ": " << rang::style::reset << d.message << std::endl;
		for (std::string line; getline(strs, line); i++) {
			std::cout << line << std::endl;
			std::cout << (d.warning ? rang::fgB::green : rang::fgB::red);
			if (i == start_line) {
				size_t end = start_line == end_line ? isecond : line.length() - ifirst;
				for (size_t i = 0; i < ifirst; ++i) {
					std::cout << " ";
				}
				for (size_t i = ifirst; i <= end; ++i) {
					std::cout << "~";
				}
			}
			else if (i == end_line) {
				for (size_t i = 0; i <= isecond; ++i) {
					std::cout << "~";
				}
			}
			else {
				for (size_t i = 0; i < line.length(); ++i) {
					std::cout << "~";
				}
			}
			std::cout << rang::style::reset << std::endl;
		}
	}
	return 0;
}