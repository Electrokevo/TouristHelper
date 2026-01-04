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

void CheckOrthogonality(const arma::mat& features, const std::vector<std::string>& featureNames) {
  std::cout << "\n--- Orthogonality & Correlation Analysis ---" << std::endl;

  // 1. Calculate Correlation Matrix
  // We transpose (.t()) because Armadillo expects (Samples x Features)
  // to calculate correlation between columns.
  arma::mat correlationMatrix = cor(features.t());

  // 2. Scan for High Correlations (Lack of Orthogonality)
  std::cout << "Checking for redundant features (Correlation > 0.7 or < -0.7)..." << std::endl;
  bool foundHighCorr = false;

  for (arma::uword i = 0; i < correlationMatrix.n_rows; ++i) {
    for (arma::uword j = i + 1; j < correlationMatrix.n_cols; ++j) {
      double val = correlationMatrix(i, j);

      // If correlation is high, the features are NOT orthogonal (they are redundant)
      if (abs(val) > 0.7) {
        std::cout << "  [!] High Correlation (" << val << ") between: "
             << featureNames[i] << " <--> " << featureNames[j] << std::endl;
        foundHighCorr = true;
      }
    }
  }

  if (!foundHighCorr) {
    std::cout << "  [OK] No highly correlated features found. Dataset is relatively orthogonal." << std::endl;
  }

  // 3. Overall Orthogonality Score (Condition Number)
  // A value close to 1.0 means perfect orthogonality.
  // A very high value (e.g., > 100) means high multicollinearity (bad).
  double conditionNumber = cond(correlationMatrix);
  std::cout << "\nGlobal Orthogonality Score (Condition Number): " << conditionNumber << std::endl;
  if (conditionNumber < 10) std::cout << "  -> Excellent: Features are very independent." << std::endl;
  else if (conditionNumber < 100) std::cout << "  -> Moderate: Some features overlap." << std::endl;
  else std::cout << "  -> Poor: Heavy redundancy in data." << std::endl;
}
