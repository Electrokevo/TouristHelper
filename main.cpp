#include "Encoders.h"
#include "Helpers.h"
#include <armadillo>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <mlpack.hpp>
#include <sstream>
#include <vector>

// Only include if you need specific CUDA API calls, otherwise NVBLAS handles it
#include <cuda_runtime.h>

using namespace mlpack;
using namespace mlpack::tree;
using namespace mlpack::ann;
using namespace std;
using namespace arma;

int main() {
  // --- 0. GPU CHECK ---
  // We use standard CUDA API to check, avoiding heavy Torch dependency just for
  // a print
  int deviceCount = 0;
  cudaError_t error_id = cudaGetDeviceCount(&deviceCount);
  if (error_id == cudaSuccess && deviceCount > 0) {
    cout << "CUDA Device Detected! (" << deviceCount << " GPU(s) found)"
         << endl;
    cout << "NVBLAS should automatically offload matrix ops." << endl;
  } else {
    cout << "No CUDA Device found or CUDA driver error. Running on CPU."
         << endl;
  }

  // --- 1. DATA LOADING ---
  cout << "\n--- 1. Data Loading ---" << endl;
  string filename = "../join_2025_01.csv";
  ifstream file(filename);
  if (!file.is_open()) {
    cerr << "Error opening " << filename << endl;
    return -1;
  }

  // Encoders
  LabelEncoder encProv, encParr, encSubtipo, encServ;
  HierarchicalCantonEncoder encCanton;

  // Feature Headers
  vector<string> featureNames;
  string header, feature;
  getline(file, header);
  stringstream headerStream(header);
  int h_idx = 0;
  while (getline(headerStream, feature, ',')) {
    if (h_idx++ == 5)
      continue; // Skip Target column in features list
    featureNames.push_back(feature);
  }

  int n_feats = featureNames.size();

  // Storage
  map<int, vector<double>> canton_histories; // For Contextual LSTM
  vector<vector<double>> features(n_feats);  // For RF
  vector<double> targets;

  string line, cell;
  int validRows = 0;

  cout << "Parsing CSV..." << endl;
  while (getline(file, line)) {
    stringstream lineStream(line);
    vector<string> row;
    while (getline(lineStream, cell, ','))
      row.push_back(cell);

    if (row.size() < n_feats)
      continue;

    // Target: Service
    double targetVal = encServ.encode(row[5]);
    if (targetVal < 0)
      continue;

    targets.push_back(targetVal);

    // Feature 0: Date
    features[0].push_back(parseDate(row[0]));

    // Feature 1: Prov (Hierarchical Base)
    int provId = (int)encProv.encode(row[1]);
    features[1].push_back((double)provId);

    // Feature 2: Canton (Hierarchical Child)
    int cantonId = (int)encCanton.encode(provId, row[2]);
    features[2].push_back((double)cantonId);

    // Feature 3: Numeric
    features[3].push_back(parseNumeric(row[3]));

    // Feature 4 & 5: Categorical
    features[4].push_back(encParr.encode(row[4]));
    features[5].push_back(encSubtipo.encode(row[6]));

    // Rest: Demographics
    for (int i = 0; i < n_feats - 6; ++i) {
      int colIndex = 7 + i;
      if (colIndex < row.size())
        features[6 + i].push_back(parseNumeric(row[colIndex]));
      else
        features[6 + i].push_back(0.0);
    }

    // Store history for LSTM (Context: Canton)
    canton_histories[cantonId].push_back(targetVal);

    validRows++;
  }

  if (validRows == 0) {
    cerr << "No valid data loaded." << endl;
    return -1;
  }
  cout << "Data Loaded: " << validRows << " samples." << endl;

  // Build Matrix
  mat dataMat(n_feats, validRows);
  Row<size_t> labelMat(validRows);

  for (size_t i = 0; i < validRows; ++i) {
    for (size_t f = 0; f < n_feats; ++f) {
      dataMat(f, i) = features[f][i];
    }
    labelMat(i) = (size_t)targets[i];
  }

  // Check Orthogonality
  CheckOrthogonality(dataMat, featureNames);

  // --- 2. RANDOM FOREST ---
  cout << "\n--- 2. Random Forest (Classification) ---" << endl;

  mat trainData, testData;
  Row<size_t> trainLabels, testLabels;
  data::Split(dataMat, labelMat, trainData, testData, trainLabels, testLabels,
              0.2);

  RandomForest<GiniGain, RandomDimensionSelect> rf;

  cout << "Training Random Forest..." << endl;
  auto start = chrono::high_resolution_clock::now();

  // 200 Trees, MinLeaf 1
  rf.Train(trainData, trainLabels, encServ.numClasses(), 200, 1);

  auto stop = chrono::high_resolution_clock::now();
  cout << "RF Training finished in "
       << chrono::duration_cast<chrono::milliseconds>(stop - start).count()
       << " ms." << endl;

  Row<size_t> predictions;
  rf.Classify(testData, predictions);
  size_t correct = 0;
  for (size_t i = 0; i < testLabels.n_elem; ++i) {
    if (predictions[i] == testLabels[i])
      correct++;
  }
  cout << "RF Accuracy: " << (double)correct / testLabels.n_elem * 100.0 << "%"
       << endl;

  // --- 3. CONTEXTUAL LSTM ---
  cout << "\n--- 3. Contextual LSTM (Predict Next Service in Canton) ---"
       << endl;

  // Prepare Cubes
  size_t totalLstmSamples = 0;
  for (auto const &[cantonId, history] : canton_histories) {
    if (history.size() > 1)
      totalLstmSamples += (history.size() - 1);
  }

  double maxServ = (double)encServ.numClasses();
  double maxCant = 25000.0; // Approx max for hierarchical

  cube inputCube(2, totalLstmSamples, 1);
  cube targetCube(1, totalLstmSamples, 1);

  size_t idx = 0;
  for (auto const &[cantonId, history] : canton_histories) {
    if (history.size() < 2)
      continue;

    for (size_t i = 0; i < history.size() - 1; ++i) {
      // Feature 0: Current Service
      inputCube(0, idx, 0) = history[i] / maxServ;
      // Feature 1: Context (Canton)
      inputCube(1, idx, 0) = (double)cantonId / maxCant;

      // Target
      targetCube(0, idx, 0) = history[i + 1] / maxServ;
      idx++;
    }
  }

  cout << "LSTM Training Samples: " << totalLstmSamples << endl;

  // Define Network
  RNN<MeanSquaredError> rnn;
  rnn.Add<Linear>(32); // Increased hidden size for GPU
  rnn.Add<LSTM>(32);
  rnn.Add<Linear>(1);

  // Optimizer (Corrected for Epochs)
  const size_t BATCH_SIZE = 64; // Larger batch for GPU efficiency
  const size_t EPOCHS = 50;
  size_t iterPerEpoch = totalLstmSamples / BATCH_SIZE;
  if (iterPerEpoch == 0)
    iterPerEpoch = 1; // Safety
  size_t totalIterations = iterPerEpoch * EPOCHS;

  cout << "Total Iterations: " << totalIterations << " (" << EPOCHS
       << " Epochs)" << endl;

  ens::Adam optimizer(0.005, BATCH_SIZE, 0.9, 0.999, 1e-8, totalIterations,
                      1e-15);

  cout << "Training LSTM..." << endl;
  rnn.Train(inputCube, targetCube, optimizer, ens::PrintLoss(),
            ens::ProgressBar());

  // --- 4. PREDICTION DEMO ---
  cout << "\n--- Prediction Demo ---" << endl;
  // Context: "Seguridad Ciudadana" (ID Lookup) in "Cuenca" (Hierarchical
  // Lookup)

  // For demo, we just pick the very last event from our data to be safe
  // In a real app, you would use encCanton.encode(provID, "Cuenca") to find the
  // ID.

  double lastCantonID = 1000.0; // Hardcoded example for Cuenca
  string demoService = "Seguridad Ciudadana";
  double lastServiceID = encServ.encode(demoService);
  if (lastServiceID < 0)
    lastServiceID = 0; // Fallback

  cube query(2, 1, 1);
  query(0, 0, 0) = lastServiceID / maxServ;
  query(1, 0, 0) = lastCantonID / maxCant;

  cube predCube;
  rnn.Predict(query, predCube);

  int nextID = (int)(predCube(0, 0, 0) * maxServ);
  if (nextID < 0)
    nextID = 0;
  if (nextID >= encServ.numClasses())
    nextID = encServ.numClasses() - 1;

  cout << "Context: " << demoService << " (ID " << lastServiceID
       << ") in Canton " << lastCantonID << endl;
  cout << "Predicted Next Service: " << encServ.decode(nextID) << endl;

  return 0;
}
