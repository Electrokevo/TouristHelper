#ifndef ENCODERS_H
#define ENCODERS_H

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

// --- Robust Label Encoder ---
class LabelEncoder {
private:
  std::unordered_map<std::string, int> forward_map;
  std::vector<std::string> reverse_map;
  bool frozen = false;

public:
  float encode(std::string s);
  std::string decode(int id) const;
  void freeze();
  void unfreeze();
  void save(const std::string &filename);
  void load(const std::string &filename);
  size_t numClasses() const;
};

// --- Hierarchical Encoder (Province -> Canton) ---
class HierarchicalCantonEncoder {
public:
  std::map<int, std::map<std::string, int>> hierarchy;
  const int OFFSET_MULTIPLIER = 1000;

  float encode(int provId, std::string cantonName);
};

#endif // ENCODERS_H
