#ifndef LABEL_ENCODER_H
#define LABEL_ENCODER_H

#include <string>
#include <unordered_map>
#include <vector>

class LabelEncoder {
public:
    int encode(const std::string& label);
    std::string decode(int id) const;
    size_t numClasses() const;

private:
    std::unordered_map<std::string, int> labelToId;
    std::vector<std::string> idToLabel;
};

#endif
