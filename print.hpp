#pragma once

#include <iosfwd>

#include "ast.hpp"

namespace minic {

void printAST(std::ostream& os, const Program& p);

} // namespace minic

