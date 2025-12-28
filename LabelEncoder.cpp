#include <armadillo>
#include <map>
#include <mlpack.hpp>
#include <string>
#include <vector>

class LabelEncoder {
public:
  std::map<std::string, int> mapping;
  std::vector<std::string> reverse_mapping;
  int next_id = 0;

  double encode(std::string s) {
    const auto strBegin = s.find_first_not_of(" \t\r\n");
    if (strBegin == std::string::npos)
      return -1.0;
    const auto strEnd = s.find_last_not_of(" \t\r\n");
    const auto strRange = strEnd - strBegin + 1;
    std::string trimmed = s.substr(strBegin, strRange);

    if (mapping.find(trimmed) == mapping.end()) {
      mapping[trimmed] = next_id++;
      reverse_mapping.push_back(trimmed);
    }
    return (double)mapping[trimmed];
  }

  // NEW: Convert ID back to String
  std::string decode(int id) {
    if (id >= 0 && id < reverse_mapping.size()) {
      return reverse_mapping[id];
    }
    return "Unknown Service";
  }

  size_t numClasses() const { return next_id; }
};
