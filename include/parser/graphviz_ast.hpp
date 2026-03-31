#include <parser/parser.hpp>
#include <utility>

#include "parser/nodes.hpp"

namespace parser {

class ASTGraphVizDumper {
 public:
  // Dumper needs to know, where it would generate new pictures
  explicit ASTGraphVizDumper(std::string output_dir_name);

  std::string generateFileName() const;

  void dumpToDotFile(const std::pair<Program, Positions>& ast);
  void dumpToDotFile(const std::pair<Program, Positions>& ast,
                     const std::string& dot_file_name);

  void dumpToPng(const std::pair<Program, Positions>& ast);
  void dumpToPng(const std::pair<Program, Positions>& ast,
                 const std::string& png_file_name);

 private:
  std::string output_dir_name_;
};

}  // namespace parser