#pragma once
#include <string>
#include <vector>
#include <cstdlib>

//helper functions
inline float _stringToFloat(const std::string &source) {
	return std::strtof(source.c_str(), NULL);
}

inline unsigned int _stringToUint(const std::string &source) {
	return (unsigned int)std::strtoul(source.c_str(), NULL, 10);
}

inline int _stringToInt(const std::string &source) {
	return (int)std::strtol(source.c_str(), NULL, 10);
}

inline void _stringTokenize(const std::string &source, std::vector<std::string> &tokens) {
	tokens.clear();
	const char* str = source.c_str();
	
	while (*str) {
		// Skip whitespace
		while (*str && (*str == ' ' || *str == '\t' || *str == '\n')) str++;
		
		if (!*str) break;
		
		const char* start = str;
		// Find end of token
		while (*str && *str != ' ' && *str != '\t' && *str != '\n') str++;
		
		tokens.emplace_back(start, str - start);
	}
}

inline void _faceTokenize(const std::string &source, std::vector<std::string> &tokens) {
	tokens.clear();
	const char* str = source.c_str();
	
	// Treat / and \ as separators
	while (*str) {
		// Skip separators
		while (*str && (*str == '/' || *str == '\\')) str++;
		
		if (!*str) break;
		
		const char* start = str;
		// Find end of token
		while (*str && *str != '/' && *str != '\\') str++;
		
		tokens.emplace_back(start, str - start);
	}
}
