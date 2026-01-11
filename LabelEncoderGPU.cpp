#include "LabelEncoder.h"

int LabelEncoder::encode(const std::string& label)
{
  auto it = labelToId.find(label);
  if (it != labelToId.end()) return it->second;

  int id = static_cast<int>(idToLabel.size());
  labelToId[label] = id;
  idToLabel.push_back(label);
  return id;
}

std::string LabelEncoder::decode(int id) const
{
  if (id < 0 || static_cast<size_t>(id) >= idToLabel.size()) return "";
  return idToLabel[id];
}

size_t LabelEncoder::numClasses() const
{
  return idToLabel.size();
}
