#include <algorithm>
#include <armadillo>
#include <iostream>
#include <mlpack.hpp>
#include <sstream>
#include <string>

// --- Helper: Label Encoder ---
// --- Helper: Date Parser ---
double parseDate(const std::string &dateStr) {
  if (dateStr.empty())
    return 0.0;
  std::string s = dateStr;
  replace(s.begin(), s.end(), '/', ' ');
  std::stringstream ss(s);
  int d, m, y;
  ss >> d >> m >> y;
  return (double)(y * 10000 + m * 100 + d);
}

// --- Helper: Numeric Parser ---
double parseNumeric(const std::string &s) {
  if (s.empty())
    return 0.0;
  if (all_of(s.begin(), s.end(), [](unsigned char c) { return isspace(c); }))
    return 0.0;
  try {
    return stod(s);
  } catch (...) {
    return 0.0;
  }
}
