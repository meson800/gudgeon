#pragma once

#include <stdexcept>

/**
 * Exception representing an event system error
 */
class EventError : public std::runtime_error
{
public:
    EventError(const std::string& err) : std::runtime_error(err) {}
};

/**
 * Exception representing a generic parse failure.
 */
class GenericParseError : public std::runtime_error
{
public:
    GenericParseError(const std::string& err) : std::runtime_error(err) {}
};

/**
 * Exception representing a TeamParser error.
 */
class TeamParseError : public GenericParseError
{
public:
    TeamParseError(const std::string& err) : GenericParseError(err) {}
};

/**
 * Exception representing a ConfigParser error.
 */
class ConfigParseError : public GenericParseError
{
public:
    ConfigParseError(const std::string& err) : GenericParseError(err) {}
};

/**
 * Exception representing a lobby error
 */
class LobbyError : public std::runtime_error
{
public:
    LobbyError(const std::string& err) : std::runtime_error(err) {}
};

/**
 * Exception representing a Envelope delivery error
 */
class EnvelopeError : public std::runtime_error
{
public:
    EnvelopeError(const std::string& err) : std::runtime_error(err) {}
};

/**
 * Exception representing an SDL error
 */
class SDLError : public std::runtime_error
{
public:
    SDLError(const std::string& err) : std::runtime_error(err) {}
};

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

/**
 * Exception representing a failure to decode a network message.
 */
class NetworkMessageError : public NetworkError
{
public:
    NetworkMessageError(const std::string &err) : NetworkError(err) {}
};

/**
 * Exception representing a failure to handle an incoming message.
 */
class NetworkMessageUnhandledError : public NetworkMessageError
{
public:
    NetworkMessageUnhandledError(const std::string& err) : NetworkMessageError(err) {}
};

/**
 * Exception representing a message sent to an invalid destination
 */
class InvalidDestinationError : public NetworkMessageError
{
public:
    InvalidDestinationError(const std::string& err) : NetworkMessageError(err) {}
};
