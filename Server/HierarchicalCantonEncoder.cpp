#include <map>
#include <mlpack.hpp>
#include <string>

using namespace mlpack;
using namespace mlpack::tree;
using namespace mlpack::ann;
using namespace std;

class HierarchicalCantonEncoder {
public:
  // Maps: ProvID -> (CantonName -> LocalID)
  // Example: Prov 1 -> { "Cuenca": 0, "Gualaceo": 1 }
  map<int, map<string, int>> hierarchy;

  // We assume no province has more than 1000 cantons (safe assumption)
  const int OFFSET_MULTIPLIER = 1000;

  double encode(int provId, string cantonName) {
    // Trim whitespace
    const auto strBegin = cantonName.find_first_not_of(" \t\r\n");
    if (strBegin == string::npos)
      return -1.0;
    const auto strEnd = cantonName.find_last_not_of(" \t\r\n");
    string trimmed = cantonName.substr(strBegin, strEnd - strBegin + 1);

    // Check if this specific province already has this canton
    if (hierarchy[provId].find(trimmed) == hierarchy[provId].end()) {
      // Assign next available local ID for this province
      int nextLocalId = hierarchy[provId].size();
      hierarchy[provId][trimmed] = nextLocalId;
    }

    // Formula: (ProvID * 1000) + LocalID
    // Example: Prov 5 (Guayas), Canton 2 (Milagro) -> 5002
    return (double)((provId * OFFSET_MULTIPLIER) + hierarchy[provId][trimmed]);
  }
};
