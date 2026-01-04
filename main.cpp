#include "Helpers.cpp"
#include "LabelEncoder.cpp"
#include <armadillo>
#include <chrono>
#include <fstream>
#include <iostream>
#include <mlpack.hpp>
#include <sstream>
#include <string>
#include <vector>

// Namespaces
using namespace mlpack;

int main()
{
    // 1. DATA LOADING
    std::cout << "--- 1. Data Loading ---" << std::endl;
    std::string filename = "../small_data.csv";
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Error opening " << filename << std::endl;
        return -1;
    }

    std::vector<std::vector<double>> features(19);
    std::vector<double> targets;
    LabelEncoder encProv, encCanton, encCodParr, encParr, encSubtipo, encServ;

    std::string line, cell;
    getline(file, line);

    int validRows = 0;
    while (getline(file, line))
    {
        std::stringstream lineStream(line);
        std::vector<std::string> row;
        while (getline(lineStream, cell, ';'))
            row.push_back(cell);

        if (row.size() < 7)
            continue;

        double targetVal = encServ.encode(row[5]);
        if (targetVal < 0)
            continue;

        features[0].push_back(parseDate(row[0]));
        features[1].push_back(encProv.encode(row[1]));
        features[2].push_back(encCanton.encode(row[2]));
        features[3].push_back(encCodParr.encode(row[3]));
        features[4].push_back(encParr.encode(row[4]));
        features[5].push_back(encSubtipo.encode(row[6]));
        targets.push_back(targetVal);

        for (int i = 0; i < 13; ++i)
        {
            int colIndex = 7 + i;
            if (colIndex < row.size())
                features[6 + i].push_back(parseNumeric(row[colIndex]));
            else
                features[6 + i].push_back(0.0);
        }
        validRows++;
    }

    if (validRows == 0)
        return -1;
    std::cout << "Data Loaded: " << validRows << " samples." << std::endl;
    arma::mat dataMat(19, validRows);
    arma::Row<size_t> labelMat(validRows);

    for (size_t i = 0; i < validRows; ++i)
    {
        for (size_t f = 0; f < 19; ++f)
            dataMat(f, i) = features[f][i];
        labelMat(i) = (size_t)targets[i];
    }
    // Define names for your 19 features so the output is readable
    std::vector<std::string> featureNames = {
        "Fecha", "Provincia", "Canton", "Cod_Parroquia", "Parroquia", "Subtipo", // 0-5
        "p02", "p03", "empleo", "desempleo", "ingpc", "nnivins",
        "vi12", "vi01", "vi05b", "vi03b", "vi141", "c01", "c09" // 6-18
    };

    // CALL THE NEW FUNCTION
    CheckOrthogonality(dataMat, featureNames);

    // 2. RANDOM FOREST
    std::cout << "\n--- 2. Random Forest (Classification) ---" << std::endl;

    arma::mat trainData, testData;
    arma::Row<size_t> trainLabels, testLabels;
    data::Split(dataMat, labelMat, trainData, testData, trainLabels, testLabels,
                0.2);

    RandomForest<GiniGain, RandomDimensionSelect> rf;

    // Using 100 trees and leaf size 20 for large dataset
    std::cout << "Training Random Forest..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    rf.Train(trainData, trainLabels, encServ.numClasses(), 500, 1);

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "RF Training finished in " << duration.count() << " ms."
        << std::endl;

    arma::Row<size_t> predictions;
    rf.Classify(testData, predictions);
    size_t correct = 0;
    for (size_t i = 0; i < testLabels.n_elem; ++i)
        if (predictions[i] == testLabels[i])
            correct++;
    std::cout << "RF Accuracy: " << (double)correct / testLabels.n_elem * 100.0
        << "%" << std::endl;

    // LSTM (Sequence Prediction)
    std::cout << "\n--- 3. LSTM (Sequence Prediction) ---" << std::endl;

    // Prepare Data
    arma::mat seqData = arma::conv_to<arma::mat>::from(labelMat);
    double maxVal = (double)(encServ.numClasses() > 0 ? encServ.numClasses() : 1);
    seqData = seqData / maxVal;

    // Get input/target matrices (2D)
    arma::mat inputMat = seqData.submat(0, 0, 0, seqData.n_cols - 2);
    arma::mat targetMat = seqData.submat(0, 1, 0, seqData.n_cols - 1);

    // Dimensions: [Features=1, Samples=N, TimeSteps=1]
    arma::cube inputCube(1, inputMat.n_cols, 1);
    arma::cube targetCube(1, targetMat.n_cols, 1);

    // Copy data into the first slice
    inputCube.slice(0) = inputMat;
    targetCube.slice(0) = targetMat;

    // Define Network
    RNN<MeanSquaredError> rnn;
    rnn.Add<Linear>(10); // Input -> Hidden Linear
    rnn.Add<LSTM>(10); // LSTM Layer
    rnn.Add<Linear>(1); // Output Layer

    // Optimizer
    ens::Adam optimizer(0.01, 32, 0.9, 0.999, 1e-8, 50, 1e-5);

    std::cout << "Training LSTM..." << std::endl;

    // Train using Cubes
    rnn.Train(inputCube, targetCube, optimizer, ens::PrintLoss(),
              ens::ProgressBar());

    std::cout << "\nLSTM Training Complete." << std::endl;

    arma::mat lastEventMat(1, 1);
    lastEventMat(0, 0) = seqData(0, seqData.n_cols - 1);

    // Convert single sample to Cube for prediction
    arma::cube lastEventCube(1, 1, 1);
    lastEventCube.slice(0) = lastEventMat;

    arma::cube nextPredCube;
    rnn.Predict(lastEventCube, nextPredCube);

    // Extract result
    double predVal = nextPredCube(0, 0, 0);
    int nextClassID = (int)(predVal * maxVal);

    // Clamp ID to valid range (safety check)
    if (nextClassID < 0)
        nextClassID = 0;
    if (nextClassID >= encServ.numClasses())
        nextClassID = encServ.numClasses() - 1;

    // --- FINAL OUTPUT ---
    std::cout << "Previous Service ID: " << (int)(lastEventMat(0, 0) * maxVal)
        << std::endl;
    std::cout << "Predicted Next Service ID: " << nextClassID << std::endl;
    std::cout << "PREDICTED SERVICE NAME: " << encServ.decode(nextClassID)
        << std::endl;

    return 0;
}
