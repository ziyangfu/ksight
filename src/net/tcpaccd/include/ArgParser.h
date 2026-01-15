#ifndef NET_TCP_ACCD_ARG_PARSER_H
#define NET_TCP_ACCD_ARG_PARSER_H

#include "ConfigArgs.h"
#include "argparse/argparse.hpp"

namespace net::tcpAccd {

void cmdParser(argparse::ArgumentParser &parser, ConfigArgs &config);

} // namespace net::tcpAccd

#endif // NET_TCP_ACCD_ARG_PARSER_H
