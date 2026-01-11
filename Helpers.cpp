#include "Helpers.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

using namespace std;

double parseDate(const string &dateStr) {
  if (dateStr.empty())
    return 0.0;
  string s = dateStr;
  replace(s.begin(), s.end(), '/', ' ');
  stringstream ss(s);
  int d, m, y;
  ss >> d >> m >> y;
  return (double)(y * 10000 + m * 100 + d);
}

double parseNumeric(const string &s) {
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

void CheckOrthogonality(const arma::mat &features,
                        const vector<string> &featureNames) {
  cout << "\n--- Orthogonality & Correlation Analysis ---" << endl;

  // Transpose because cor() expects Samples x Features
  arma::mat correlationMatrix = arma::cor(features.t());
  bool foundHighCorr = false;

  cout << "Checking for redundant features (|Correlation| > 0.7)..." << endl;
  for (arma::uword i = 0; i < correlationMatrix.n_rows; ++i) {
    for (arma::uword j = i + 1; j < correlationMatrix.n_cols; ++j) {
      double val = correlationMatrix(i, j);
      if (abs(val) > 0.7) {
        // Bounds check for feature names
        string n1 = (i < featureNames.size()) ? featureNames[i] : to_string(i);
        string n2 = (j < featureNames.size()) ? featureNames[j] : to_string(j);

        cout << "  [!] High Correlation (" << val << ") between: " << n1
             << " <--> " << n2 << endl;
        foundHighCorr = true;
      }
    }
  }

  if (!foundHighCorr)
    cout << "  [OK] No highly correlated features found." << endl;

  // Condition number check logic
  // Using simple fallback if calculation fails (common with singular matrices)
  double conditionNumber = 0.0;
  try {
    conditionNumber = arma::cond(correlationMatrix);
    cout << "Global Orthogonality Score (Condition Number): " << conditionNumber
         << endl;
  } catch (...) {
    cout << "Global Orthogonality Score: Calculation failed (likely singular "
            "matrix)."
         << endl;
    conditionNumber = 999.0; // Assume bad
  }
}
