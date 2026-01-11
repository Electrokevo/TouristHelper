#include "Encoders.h"
#include <algorithm>
#include <fstream>
#include <iostream>

using namespace std;

// ==========================================
// LabelEncoder Implementation
// ==========================================
double LabelEncoder::encode(string s) {
  // Trim whitespace
  const auto strBegin = s.find_first_not_of(" \t\r\n");
  if (strBegin == string::npos)
    return -1.0;
  const auto strEnd = s.find_last_not_of(" \t\r\n");
  const auto strRange = strEnd - strBegin + 1;
  string trimmed = s.substr(strBegin, strRange);

  if (forward_map.find(trimmed) != forward_map.end()) {
    return (double)forward_map[trimmed];
  }

  if (frozen)
    return -1.0;

  int new_id = reverse_map.size();
  forward_map[trimmed] = new_id;
  reverse_map.push_back(trimmed);

  return (double)new_id;
}

string LabelEncoder::decode(int id) const {
  if (id >= 0 && id < reverse_map.size()) {
    return reverse_map[id];
  }
  return "UNKNOWN";
}

void LabelEncoder::freeze() { frozen = true; }
void LabelEncoder::unfreeze() { frozen = false; }

void LabelEncoder::save(const string &filename) {
  ofstream out(filename);
  if (!out.is_open()) {
    cerr << "Error saving encoder: " << filename << endl;
    return;
  }
  for (size_t i = 0; i < reverse_map.size(); ++i) {
    out << i << "," << reverse_map[i] << "\n";
  }
  cout << "Encoder saved to " << filename << " (" << reverse_map.size()
       << " classes)" << endl;
}

void LabelEncoder::load(const string &filename) {
  ifstream in(filename);
  if (!in.is_open()) {
    cerr << "Error loading encoder: " << filename << endl;
    return;
  }

  forward_map.clear();
  reverse_map.clear();
  frozen = false;

  string line;
  while (getline(in, line)) {
    size_t commaPos = line.find(',');
    if (commaPos == string::npos)
      continue;
    try {
      int id = stoi(line.substr(0, commaPos));
      string name = line.substr(commaPos + 1);
      if (reverse_map.size() <= id)
        reverse_map.resize(id + 1);
      reverse_map[id] = name;
      forward_map[name] = id;
    } catch (...) {
      continue;
    }
  }
  frozen = true;
  cout << "Encoder loaded from " << filename << " (" << reverse_map.size()
       << " classes)" << endl;
}

size_t LabelEncoder::numClasses() const { return reverse_map.size(); }

// ==========================================
// HierarchicalCantonEncoder Implementation
// ==========================================
double HierarchicalCantonEncoder::encode(int provId, string cantonName) {
  const auto strBegin = cantonName.find_first_not_of(" \t\r\n");
  if (strBegin == string::npos)
    return -1.0;
  const auto strEnd = cantonName.find_last_not_of(" \t\r\n");
  string trimmed = cantonName.substr(strBegin, strEnd - strBegin + 1);

  if (hierarchy[provId].find(trimmed) == hierarchy[provId].end()) {
    int nextLocalId = hierarchy[provId].size();
    hierarchy[provId][trimmed] = nextLocalId;
  }

  return (double)((provId * OFFSET_MULTIPLIER) + hierarchy[provId][trimmed]);
}
