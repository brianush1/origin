#include <sstream>
#include <iostream>
#include <fstream>
#include "lexer.h"
#include "parser.h"
#include "odef.h"
#include "type_analysis.h"
#include "rang.h"

using namespace std::string_literals;

void help() {
	std::cout << "Usage: originc {option} project\n";
	std::cout << "       originc {option} (--file|-f) file\n";
	std::cout << "       originc {option} (--make-def|-d) file\n";
	std::cout << "Info Options:\n";
	std::cout << "  --help, -h                         Display this information.\n";
	std::cout << "  --version, -v                      Display the compiler's version.\n";
	std::cout << "  --target-list, -l                  List available targets.\n";
	//std::cout << "  --format-list, -L                  List available output formats.\n";
	std::cout << "Options:\n";
	std::cout << "  --target=<arg>, -t <arg>           Specify the compilation target.\n";
	std::cout << "  --output=<file>, -o <file>         Write the output into <file>.\n";
	//std::cout << "  --format=<arg>, -f <arg>           Specify the output format.\n";
	std::cout << "  --include=<arg>, -I <arg>          Semicolon-separated list of paths to\n";
	std::cout << "                                     search for packages.\n";
	std::cout << "  --file, -f                         Compile a single file instead of a project.\n";
	std::cout << "  --make-def, -d                     Generate a definition file for a single\n";
	std::cout << "                                     source file.\n";
	/*d::cout << "================================================================================" */
	std::cout << "\nTo submit a bug report, see:\n";
	std::cout << "<https://github.com/brianush1/origin/issues>.\n";
}

void version() {
	std::cerr << "originc version 0.1.0\n";
}

void target_list() {
	std::cout << "Targets:\n";
	std::cout << "  c\n";
	/*d::cout << "================================================================================" */
	std::cout << "\nTo submit a bug report, see:\n";
	std::cout << "<https://github.com/brianush1/origin/issues>.\n";
}

bool starts_with(const std::string& haystack, const std::string& needle) {
	if (haystack.size() < needle.size()) return false;
	return memcmp(haystack.data(), needle.data(), needle.size()) == 0;
}

std::string prog;

void single_char_arg(std::string& result, std::vector<std::string>& args, size_t& i) {
	std::string& arg = args[i];
	if (arg.size() > 2) {
		result = arg.substr(2);
	}
	else if (args.size() > i + 1) {
		result = args[++i];
	}
	else {
		std::cerr << rang::style::bold << prog << ": " << rang::fgB::red
			<< "error: " << rang::style::reset << "expected argument to command line option "
			<< rang::style::bold << "'" << arg << "'\n";
	}
}

void multichar_arg(std::string& result, std::string& arg, size_t sub, const std::string& name) {
	if (arg.size() == sub) {
		std::cerr << rang::style::bold << prog << ": " << rang::fgB::red
			<< "error: " << rang::style::reset << "expected argument to command line option "
			<< rang::style::bold << "'" << arg << "'\n";
	}
	else if (result == "") {
		result = arg.substr(sub);
	} else {
		std::cerr << rang::style::bold << prog << ": " << rang::fgB::yellow
			<< "warning: " << rang::style::reset << "extra " << name << " parameter "
			<< rang::style::bold << "'" << arg.substr(sub) << "'"
			<< rang::style::reset << " is ignored\n";
	}
}

using namespace origin;

int main(int argc, char** argv) {
	prog = "originc";
	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i) {
		if (argv[i] == "--"s) {
			break;
		}
		else if (argv[i] != ""s) {
			args.push_back(argv[i]);
		}
	}
	size_t size = args.size();
	if (size == 0) {
		help();
		return 0;
	}
	bool requires_input = true;
	std::string target_str = "c";
	std::string output_str = "a.out";
	std::string include_str;
	std::string input_str;
	bool compile_file = false;
	bool make_def = false;
	FILE* x = stdout;
	for (size_t i = 0; i < args.size(); ++i) {
		std::string arg = args[i];
		if (arg == "--help" || arg == "-h") {
			help();
			requires_input = false;
		}
		else if (arg == "--version" || arg == "-v") {
			version();
			requires_input = false;
		}
		else if (arg == "--target-list" || arg == "-l") {
			target_list();
			requires_input = false;
		}
		else if (starts_with(arg, "--target=")) multichar_arg(target_str, arg, 9, "target");
		else if (starts_with(arg, "--output=")) multichar_arg(output_str, arg, 9, "output");
		else if (starts_with(arg, "--include=")) multichar_arg(include_str, arg, 10, "include");
		else if (starts_with(arg, "-t")) single_char_arg(target_str, args, i);
		else if (starts_with(arg, "-o")) single_char_arg(output_str, args, i);
		else if (starts_with(arg, "-I")) single_char_arg(include_str, args, i);
		else if (arg == "--file" || arg == "-f") compile_file = true;
		else if (arg == "--make-def" || arg == "-d") make_def = true;
		else if (starts_with(arg, "-")) {
			std::cerr << rang::style::bold << prog << ": " << rang::fgB::red
				<< "error: " << rang::style::reset << "unrecognized command line option "
				<< rang::style::bold << "'" << arg << "'\n" << rang::style::reset;
		}
		else {
			if (input_str != "") {
				std::cerr << rang::style::bold << prog << ": " << rang::fgB::yellow
					<< "warning: " << rang::style::reset << "extra input source "
					<< rang::style::bold << "'" << arg << "'"
					<< rang::style::reset << " is ignored\n";
			}
			else {
				input_str = arg;
			}
		}
	}
	if (requires_input && input_str == "") {
		std::cerr << rang::style::bold << prog << ": " << rang::fgB::red
			<< "fatal error: " << rang::style::reset << "no input files\n" << rang::style::reset;
		std::cerr << "compilation terminated.\n";
		return 1;
	}
	if (make_def) {
		std::ifstream stream(input_str);
		std::vector<diagnostic> diagnostics;
		lexer lex(stream, diagnostics);
		parser parse(lex, diagnostics);
		program* prog = parse.read_program();
		std::ofstream ostream(output_str, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
		encode(ostream, prog);
	}
	else {
		std::cerr << rang::style::bold << prog << ": " << rang::fgB::red
			<< "fatal error: " << rang::style::reset << "compilation type is not yet supported\n" << rang::style::reset;
		std::cerr << "compilation terminated.\n";
		return 1;
	}
	return 0;
}