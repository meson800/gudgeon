#pragma once

#include <iostream>
#include <string>
#include <map>
#include <vector>

/**
 * This represents a parsed configuration file.
 *
 * An enclosing multimap stores top-level constructs by name.
 *
 * Each top-level construct itself stores a multi-map of key/value pairs,
 * where each value is a vector of tokens.
 */
using ParseKVStore = std::multimap<std::string, std::vector<std::string>>;
using ParseResult = std::multimap<std::string,ParseKVStore>;

/**
 * This class handles reading and tokenizing a simple file-based
 * configuration file. Top level constructs begin with a single token
 * "BEGIN CONSTRUCT_NAME" and terminate with "END CONSTRUCT_NAME"
 *
 * This is followed by a set of "key = value1 value2 ..." entries.
 * Keys do not have to be unique, with each set of values stored in a
 * single multimap entry.
 *
 * Empty lines are ignored. All characters after and including a '#'
 * character are also ignored.
 *
 * Example config file:
 *
 * # General configuration
 * BEGIN CONFIG
 * verbosity = 3
 * END CONFIG
 *
 * BEGIN TEAM # Team Dabney
 * station = helm
 * station = hydrophone
 * station = weapons
 * station = observer optional
 * station = communication optional
 * END TEAM
 */
class GenericParser
{
public:
    /// Opens the specified filename and returns a ParseResult. Throws GenericParseException upon parse error
    static ParseResult parse(const std::string& filename);
    /// Given an already-open file, parses and returns a ParseResult. Throws GenericParseException upon parse error
    static ParseResult parse(std::istream& instream);
};
