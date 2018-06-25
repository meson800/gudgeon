#pragma once

#include <stdexcept>

/**
 * Exception representing a network failure of some kind.
 */
class NetworkError : public std::runtime_error
{
public:
    NetworkError(const std::string &err) : std::runtime_error(err) {}
};

/**
 * Exception representing a network startup failure of some kind.
 */
class NetworkStartupError : public NetworkError
{
public:
    NetworkStartupError(const std::string &err) : NetworkError(err) {}
};

/**
 * Exception representing a failure to connect to another computer.
 */
class NetworkConnectionError : public NetworkError
{
public:
    NetworkConnectionError(const std::string &err) : NetworkError(err) {}
};
