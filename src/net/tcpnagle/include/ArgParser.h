#ifndef NET_TCP_NAGLE_ARG_PARSER_H
#define NET_TCP_NAGLE_ARG_PARSER_H

#include "ConfigArgs.h"
#include "argparse/argparse.hpp"

namespace net::tcpNagle {

void cmdParser(argparse::ArgumentParser &parser, ConfigArgs &config);

} // namespace net::tcpNagle

#endif // NET_TCP_NAGLE_ARG_PARSER_H
