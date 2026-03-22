#include <parser/parser.hpp>
#include <utility>

#include "parser/nodes.hpp"

namespace parser {

void printAST(const std::pair<Program, Positions>& ast);

}  // namespace parser