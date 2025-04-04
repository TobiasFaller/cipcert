#include "cip.hpp"
#include <cassert>

#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <string_view>

#include "cnf.hpp"

inline std::string trim(const std::string &s) {
   auto wsfront = std::find_if_not(s.begin(), s.end(), [](int c){ return std::isspace(c); });
   auto wsback = std::find_if_not(s.rbegin(), s.rend(), [](int c){ return std::isspace(c); }).base();
   return (wsback <= wsfront ? std::string() : std::string(wsfront, wsback));
}

Cip::Cip(const char *path): size(0u) {
	const std::regex innerGroups { "\\((.*)\\)" };
	const std::regex specificGroups { "(-?[0-9]+:[0-9]+)" };

	auto parse_clause = [&](const std::string& line, int64_t vars) -> std::vector<int64_t> {
		std::smatch match;
		if (!std::regex_match(line, match, innerGroups)) {
			std::cerr << "Error: Could not parse line \"" << line << "\"" << std::endl;
			exit(2);
		}
		std::vector<int64_t> clause { };
		std::string fullClause = match[1].str();
		for (auto it = std::sregex_iterator(fullClause.begin(), fullClause.end(), specificGroups);
			it != std::sregex_iterator(); it++) {
			match = *it;
			std::stringstream stream(match.str(0));
			std::string token;
			std::getline(stream, token, ':');
			int64_t literalId = std::stol(token);
			std::getline(stream, token, ':');
			int64_t timeframe = std::stol(token);
			clause.push_back(literalId + ((literalId < 0) ? (-vars * timeframe) : (vars * timeframe)));
		}
		return clause;
	};

	std::string line;
	std::ifstream file(path);
	CNF *current;
	while (file.good() && !file.eof()) {
		std::getline(file, line);
		line = trim(line);
		if (line.empty()) { continue; }

		if (line.rfind("DECL", 0u) == 0u) {
			while(file.good() && !file.eof()) {
				std::getline(file, line);
				line = trim(line);
				if (line.empty()) { break; }

				std::string variableType;
				size_t variableIndex;
				std::stringstream stream(line);
				stream >> variableType;
				stream >> variableIndex;
				size++;
				assert(variableType == "AND_VAR" || variableType == "AUX_VAR"
					|| variableType == "LATCH_VAR" || variableType == "INPUT_VAR"
					|| variableType == "OUTPUT_VAR");
				assert (size == variableIndex);
			}
		} else if (line.rfind("INIT", 0u) == 0u) {
			current = &(init = CNF(size, 0));
			while(file.good() && !file.eof()) {
				std::getline(file, line);
				line = trim(line);
				if (line.empty()) { break; }
				current->clauses.push_back(parse_clause(line, size));
			}
			current->m = current->clauses.size();
		} else if (line.rfind("TRANS", 0u) == 0u) {
			current = &(trans = CNF(size, 0));
			while(file.good() && !file.eof()) {
				std::getline(file, line);
				line = trim(line);
				if (line.empty()) { break; }
				current->clauses.push_back(parse_clause(line, size));
			}
			current->m = current->clauses.size();
		} else if (line.rfind("TARGET", 0u) == 0u) {
			current = &(target = CNF(size, 0));
			while(file.good() && !file.eof()) {
				std::getline(file, line);
				line = trim(line);
				if (line.empty()) { break; }
				current->clauses.push_back(parse_clause(line, size));
			}
			current->m = current->clauses.size();
		} else if (line.rfind("--", 0u) == 0u) {
			line = line.substr(2);
			line = trim(line);
			if (line.empty()) { continue; }
			static const std::regex pattern(R"(\s*(-?\d+)\s*=\s*(-?\d+)\s*)");
			std::smatch matches;
			if (!std::regex_match(line, matches, pattern)) { continue; }
			int64_t witness = std::stoi(matches[1].str());
			int64_t model = std::stoi(matches[2].str());
			simulation.push_back({witness, model});
		} else if (line.rfind("OFFSET: ", 0u) == 0u
				|| line.rfind("USE_PROPERTY: ", 0u) == 0u
				|| line.rfind("SIMPLIFY_INTERPOLANTS: ", 0u) == 0u
				|| line.rfind("TIMEOUT: ", 0u) == 0u
				|| line.rfind("MAXDEPTH: ", 0u) == 0u) {
			continue;
		}
	}
}

std::ostream &operator<<(std::ostream &os, const Cip &dimspec) {
  os << "init " << dimspec.init;
  os << "trans " << dimspec.trans;
  os << "target " << dimspec.target;
  return os;
}
