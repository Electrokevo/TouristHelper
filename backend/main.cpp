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

// --- GPU / LIBTORCH INCLUDES ---
#include <cuda_runtime.h>
#include <torch/torch.h>

using namespace mlpack;
using namespace mlpack::tree;
using namespace mlpack::ann;
using namespace std;
using namespace arma;

// ==========================================
// 1. LIBTORCH GPU LSTM MODULE (RESTORED)
// ==========================================
// Note: This struct is available for use if you switch to a custom LibTorch loop.
// The current main() uses MLPack's RNN, which runs on GPU via NVBLAS.
struct ContextualLSTMImpl : torch::nn::Module
{
    torch::nn::Embedding cantonEmb{nullptr};
    torch::nn::Embedding serviceEmb{nullptr};
    torch::nn::LSTM lstm{nullptr};
    torch::nn::Linear fc{nullptr};
    int64_t hiddenSize;

    ContextualLSTMImpl(int64_t numCantones, int64_t numServicios,
                       int64_t embCantonDim = 16, int64_t embServiceDim = 32,
                       int64_t hidden = 64)
        : hiddenSize(hidden)
    {
        cantonEmb = register_module(
            "cantonEmb", torch::nn::Embedding(numCantones, embCantonDim));
        serviceEmb = register_module(
            "serviceEmb", torch::nn::Embedding(numServicios, embServiceDim));

        lstm = register_module(
            "lstm", torch::nn::LSTM(
                torch::nn::LSTMOptions(embCantonDim + embServiceDim, hidden)
                .batch_first(true)));

        fc = register_module("fc", torch::nn::Linear(hidden, numServicios));
    }

    torch::Tensor forward(torch::Tensor cantonIds, torch::Tensor serviceSeq)
    {
        auto cEmb = cantonEmb(cantonIds);
        auto sEmb = serviceEmb(serviceSeq);
        auto cEmbRep = cEmb.unsqueeze(1).repeat({1, sEmb.size(1), 1});
        auto x = torch::cat({cEmbRep, sEmb}, 2);
        auto lstmOut = std::get<0>(lstm(x));
        auto last = lstmOut.select(1, lstmOut.size(1) - 1);
        return fc(last);
    }
};

TORCH_MODULE(ContextualLSTM);

// ==========================================
// 2. MAIN APPLICATION
// ==========================================
int main()
{
    // --- A. GPU & LIBTORCH CHECKS (RESTORED) ---
    cout << "--- System Check ---" << endl;

    // 1. Raw CUDA Check
    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);
    if (err == cudaSuccess && deviceCount > 0)
    {
        cout << "[CUDA] Detected " << deviceCount << " GPU(s)." << endl;
        cout << "[MLPack] NVBLAS should automatically offload matrix ops." << endl;
    }
    else
    {
        cout << "[CUDA] No device found (or driver error). Running on CPU." << endl;
    }

    // 2. LibTorch Check
    if (torch::cuda::is_available())
    {
        cout << "[LibTorch] CUDA is available. (Device: " << torch::kCUDA << ")" << endl;
    }
    else
    {
        cout << "[LibTorch] CUDA not available. Using CPU." << endl;
    }


    // --- B. DATA LOADING ---
    cout << "\n--- 1. Data Loading ---" << endl;
    string filename = "../Datasets/join_2022_05_v2.csv";

    LabelEncoder encProv, encSubtipo, encServ;
    HierarchicalCantonEncoder encCanton;

    // --- PASS 1: SCAN METADATA ---
    cout << "Pass 1: Scanning file for metadata..." << endl;
    ifstream file(filename);
    if (!file.is_open())
    {
        cerr << "Error: " << filename << endl;
        return -1;
    }

    string line, cell, header;
    getline(file, header);

    // Count columns dynamically
    stringstream ss(header);
    int totalCsvCols = 0;
    while (getline(ss, cell, ',')) totalCsvCols++;
    int numStatsCols = max(0, totalCsvCols - 5);

    int n_feats = 1 + 24 + 1 + 1 + numStatsCols; // Date + Prov(24) + Canton + Subtype + Stats
    size_t totalRows = 0;

    while (getline(file, line))
    {
        stringstream lineStream(line);
        vector<string> row;
        row.reserve(totalCsvCols);
        while (getline(lineStream, cell, ',')) row.push_back(cell);
        if (row.size() < 5) continue;

        encProv.encode(row[0]);
        encCanton.encode((int)encProv.encode(row[0]), row[1]);
        encServ.encode(row[2]);
        encSubtipo.encode(row[3]);
        totalRows++;
    }
    encProv.freeze();
    encSubtipo.freeze();

    // --- PASS 2: LOAD DATA ---
    cout << "Pass 2: Loading " << totalRows << " rows directly to RAM..." << endl;
    fmat dataMat(n_feats, totalRows);
    Row<size_t> labelMat(totalRows);
    map<int, vector<float>> canton_histories;

    file.clear();
    file.seekg(0);
    getline(file, header);
    size_t idx = 0;
    while (getline(file, line) && idx < totalRows)
    {
        stringstream lineStream(line);
        vector<string> row;
        row.reserve(totalCsvCols);
        while (getline(lineStream, cell, ',')) row.push_back(cell);
        if (row.size() < 5) continue;

        float targetVal = (float)encServ.encode(row[2]);
        if (targetVal < 0) continue;
        labelMat(idx) = (size_t)targetVal;

        // Features
        dataMat(0, idx) = parseNumeric(row[4]); // Date
        int provId = (int)encProv.encode(row[0]);
        for (int p = 0; p < 24; ++p) dataMat(1 + p, idx) = (p == provId) ? 1.0f : 0.0f; // OneHot Prov

        int cantonId = (int)encCanton.encode(provId, row[1]);
        dataMat(25, idx) = (float)cantonId;
        dataMat(26, idx) = (float)encSubtipo.encode(row[3]);

        for (int k = 0; k < numStatsCols; ++k)
        {
            if (5 + k < row.size()) dataMat(27 + k, idx) = parseNumeric(row[5 + k]);
        }

        // History
        canton_histories[cantonId].push_back(targetVal);
        idx++;
    }
    file.close();

    // --- C. RANDOM FOREST ---
    cout << "\n--- 2. Random Forest ---" << endl;
    fmat trainData, testData;
    Row<size_t> trainLabels, testLabels;
    data::Split(dataMat, labelMat, trainData, testData, trainLabels, testLabels, 0.3);
    dataMat.clear(); // Free RAM

    RandomForest<GiniGain, RandomDimensionSelect> rf;
    cout << "Training RF (MinLeaf=50)..." << endl;
    rf.Train(trainData, trainLabels, encServ.numClasses(), 20, 50);

    mlpack::data::Save("rf_model.bin", "rf_model", rf, false);
    cout << "RF Saved." << endl;

    // --- D. LSTM ---
    cout << "\n--- 3. Contextual LSTM (Probabilistic) ---" << endl;

    size_t totalLstmSamples = 0;
    for (auto const& [cantonId, history] : canton_histories)
    {
        if (history.size() > 1) totalLstmSamples += (history.size() - 1);
    }

    // 1. Setup Dimensions
    int numClasses = (int)encServ.numClasses();
    float maxCant = 25000.0f;

    cout << "LSTM Configuration:" << endl;
    cout << "  Output Neurons: " << numClasses << endl;
    cout << "  Target Labels: 0 to " << (numClasses - 1) << endl;

    // 2. Prepare Training Data
    fcube inputCube(2, totalLstmSamples, 1);
    fcube targetCube(1, totalLstmSamples, 1);

    size_t c_idx = 0;
    float maxLabelFound = -1.0f;

    for (auto const& [cantonId, history] : canton_histories)
    {
        if (history.size() < 2) continue;

        for (size_t i = 0; i < history.size() - 1; ++i)
        {
            inputCube(0, c_idx, 0) = history[i] / (float)numClasses;
            inputCube(1, c_idx, 0) = (float)cantonId / maxCant;

            float label = history[i + 1];
            // Safety Clamp
            if (label >= numClasses) label = (float)(numClasses - 1);
            if (label < 0) label = 0.0f;

            targetCube(0, c_idx, 0) = label;
            if (label > maxLabelFound) maxLabelFound = label;
            c_idx++;
        }
    }
    canton_histories.clear(); // Free RAM

    cout << "  Max Label in Data: " << maxLabelFound << endl;

    // 3. Define Network
    using FloatOutputLayer = NegativeLogLikelihoodType<arma::fmat>;
    RNN<FloatOutputLayer, RandomInitialization, arma::fmat> rnn;

    rnn.Add<LinearType<arma::fmat>>(32);
    rnn.Add<LSTMType<arma::fmat>>(32);
    rnn.Add<LinearType<arma::fmat>>(numClasses);
    rnn.Add<LogSoftMaxType<arma::fmat>>();

    // 4. Train
    const size_t BATCH_SIZE = 128;
    const size_t EPOCHS = 10;
    size_t iterPerEpoch = totalLstmSamples / BATCH_SIZE;
    if (iterPerEpoch == 0) iterPerEpoch = 1;

    ens::Adam optimizer(0.001, BATCH_SIZE, 0.9, 0.999, 1e-8, iterPerEpoch * EPOCHS, 1e-9);

    cout << "Training LSTM Classifier..." << endl;
    rnn.Train(inputCube, targetCube, optimizer, ens::PrintLoss(), ens::ProgressBar());

    // --- 4. PREDICTION DEMO (DO THIS BEFORE SAVING) ---
    // Moving this up prevents memory corruption from the Save() function
    cout << "\n--- Prediction Demo ---" << endl;

    float lastCantonID = 1000.0f;
    string demoService = "Seguridad Ciudadana";
    float lastServiceID = (float)encServ.encode(demoService);

    // Prepare Query
    fcube query(2, 1, 1);
    query(0, 0, 0) = lastServiceID / (float)numClasses;
    query(1, 0, 0) = lastCantonID / maxCant;

    // FIX: Pre-allocate result container to prevent resizing crash
    // Dimensions: [NumClasses, BatchSize(1), TimeSteps(1)]
    fcube predLogProbs(numClasses, 1, 1);

    // Predict
    rnn.Predict(query, predLogProbs);

    cout << "Context: " << demoService << endl;
    cout << "Probabilities:" << endl;

    float maxProb = -1.0f;
    int bestClass = -1;

    for (int c = 0; c < numClasses; ++c)
    {
        float prob = exp(predLogProbs(c, 0, 0));

        if (prob > maxProb)
        {
            maxProb = prob;
            bestClass = c;
        }
        if (prob > 0.01f)
        {
            cout << "  " << encServ.decode(c) << ": " << (prob * 100.0f) << "%" << endl;
        }
    }
    cout << "Best Guess: " << encServ.decode(bestClass) << endl;

    // --- 5. SAVE MODELS (DO THIS LAST) ---
    cout << "\n--- Saving Models ---" << endl;
    mlpack::data::Save("rf_model.bin", "rf_model", rf, false);
    mlpack::data::Save("lstm_model.bin", "lstm_model", rnn, false);
    cout << "Models Saved." << endl;

    exit(0);
}
