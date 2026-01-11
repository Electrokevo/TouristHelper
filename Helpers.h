#ifndef HELPERS_H
#define HELPERS_H

#include <armadillo>
#include <string>
#include <vector>

double parseDate(const std::string &dateStr);
double parseNumeric(const std::string &s);
void CheckOrthogonality(const arma::mat &features,
                        const std::vector<std::string> &featureNames);

#endif // HELPERS_H
