#ifndef HELPERS_H
#define HELPERS_H

#include <armadillo>
#include <string>
#include <vector>

float parseDate(const std::string &dateStr);
float parseNumeric(const std::string &s);
void CheckOrthogonality(const arma::fmat &features,
                        const std::vector<std::string> &featureNames);

#endif // HELPERS_H
