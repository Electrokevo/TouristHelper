#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// LIBRERÍAS
#include "httplib.h"
#include "json.hpp"
#include <opencv2/ml.hpp>
#include <opencv2/opencv.hpp>
#include <torch/torch.h>

using json = nlohmann::json;
using namespace cv;
using namespace cv::ml;

// ==========================================
// 1. DEFINICIÓN DEL MODELO LSTM (Debe ser idéntica al training)
// ==========================================
struct CrimeLSTMImpl : torch::nn::Module {
  torch::nn::LSTM lstm1{nullptr}, lstm2{nullptr};
  torch::nn::Linear embedding_layer{nullptr};
  torch::nn::Linear output_layer{nullptr};
  torch::nn::Dropout dropout{nullptr};

  CrimeLSTMImpl(int input_dim, int emb_dim) {
    lstm1 = register_module(
        "lstm1", torch::nn::LSTM(
                     torch::nn::LSTMOptions(input_dim, 64).batch_first(true)));
    lstm2 = register_module(
        "lstm2",
        torch::nn::LSTM(torch::nn::LSTMOptions(64, 32).batch_first(true)));
    dropout = register_module("dropout", torch::nn::Dropout(0.2));
    embedding_layer =
        register_module("embedding", torch::nn::Linear(32, emb_dim));
    output_layer = register_module("out", torch::nn::Linear(emb_dim, 1));
  }

  torch::Tensor forward(torch::Tensor x) {
    auto out = std::get<0>(lstm1->forward(x));
    out = dropout->forward(out);
    auto out2 = std::get<0>(lstm2->forward(out));
    auto last_step =
        out2.index({torch::indexing::Slice(), -1, torch::indexing::Slice()});
    last_step = dropout->forward(last_step);
    auto emb = torch::relu(embedding_layer->forward(last_step));
    return torch::sigmoid(output_layer->forward(emb));
  }

  // Función auxiliar para sacar solo los embeddings
  torch::Tensor get_embedding(torch::Tensor x) {
    auto out = std::get<0>(lstm1->forward(x));
    out = dropout->forward(out);
    auto out2 = std::get<0>(lstm2->forward(out));
    auto last_step =
        out2.index({torch::indexing::Slice(), -1, torch::indexing::Slice()});
    last_step = dropout->forward(last_step);
    return torch::relu(embedding_layer->forward(last_step));
  }
};
TORCH_MODULE(CrimeLSTM);

// ==========================================
// 2. VARIABLES GLOBALES Y CARGA DE DATOS
// ==========================================
std::map<std::string, std::vector<float>> inec_map;
Ptr<RTrees> rf_model;
CrimeLSTM lstm_model(nullptr); // Se inicializa luego
bool models_loaded = false;

// Cargar datos estáticos (INEC)
void load_inec_data(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "Error cargando INEC: " << path << std::endl;
    return;
  }
  std::string line, cell;
  std::getline(file, line); // header
  while (std::getline(file, line)) {
    std::stringstream ss(line);
    std::vector<std::string> row;
    while (std::getline(ss, cell, ','))
      row.push_back(cell);
    if (row.size() < 3)
      continue;

    std::vector<float> feats;
    // Asumiendo col 0 es ID, col 2 en adelante son features
    for (size_t i = 2; i < row.size(); ++i) {
      try {
        feats.push_back(std::stof(row[i]));
      } catch (...) {
        feats.push_back(0.0f);
      }
    }
    inec_map[row[0]] = feats;
  }
  std::cout << "INEC Data cargada: " << inec_map.size() << " zonas."
            << std::endl;
}

int main() {
  std::cout << "--- Iniciando Servidor TouristHelper (Híbrido) ---"
            << std::endl;

  // A. CARGAR MODELOS
  try {
    // 1. Cargar RF (OpenCV)
    rf_model = RTrees::load("random_forest_model.xml");
    if (rf_model.empty())
      throw std::runtime_error("No se pudo cargar XML de RF");
    std::cout << "Random Forest (OpenCV) cargado." << std::endl;

    // 2. Cargar LSTM (LibTorch)
    // Nota: Asumimos input_dim=7 (servicios) y emb_dim=32 como en main.cpp
    lstm_model = CrimeLSTM(7, 32);
    torch::load(lstm_model, "lstm_checkpoint.pt");
    lstm_model->eval(); // Modo inferencia
    std::cout << "LSTM (LibTorch) cargado." << std::endl;

    // 3. Cargar Mapa INEC
    load_inec_data("datos_202510_ciudades_unicas_rf.csv");

    models_loaded = true;
  } catch (const std::exception &e) {
    std::cerr << "CRITICAL ERROR: " << e.what() << std::endl;
    return -1;
  }

  // B. CONFIGURAR SERVIDOR
  httplib::Server svr;

  // Endpoint: /predict?zona=GUAYAQUIL
  svr.Get("/predict", [&](const httplib::Request &req, httplib::Response &res) {
    if (!models_loaded) {
      res.status = 500;
      res.set_content("Modelos no cargados", "text/plain");
      return;
    }

    std::string zona = req.get_param_value("zona");
    if (zona.empty()) {
      res.status = 400;
      res.set_content("Falta parametro 'zona'", "text/plain");
      return;
    }

    // --- PASO 1: INFERENCIA LSTM ---
    // En un caso real, aquí consultarías a tu DB los últimos 30 días de esa
    // zona. Como este es un demo, generaremos un tensor "dummy" o aleatorio
    // simulando la historia. Input: [Batch=1, Timesteps=30, Features=7]
    auto input_tensor = torch::rand({1, 30, 7});

    // Obtenemos el embedding latente (lo que "piensa" el LSTM sobre el futuro)
    auto embedding_tensor = lstm_model->get_embedding(input_tensor); // [1, 32]

    // Convertir tensor a vector std::vector
    std::vector<float> lstm_feats(32);
    float *ptr = embedding_tensor.data_ptr<float>();
    for (int i = 0; i < 32; ++i)
      lstm_feats[i] = ptr[i];

    // --- PASO 2: BUSCAR DATOS INEC ---
    std::vector<float> inec_feats;
    if (inec_map.count(zona)) {
      inec_feats = inec_map[zona];
    } else {
      // Zona desconocida: rellenar con ceros
      inec_feats.resize(inec_map.begin()->second.size(), 0.0f);
    }

    // --- PASO 3: FUSIÓN DE VECTORES ---
    std::vector<float> final_features = lstm_feats;
    final_features.insert(final_features.end(), inec_feats.begin(),
                          inec_feats.end());

    // --- PASO 4: INFERENCIA RANDOM FOREST ---
    // OpenCV espera una Matriz CV_32F
    cv::Mat sample(1, final_features.size(), CV_32F);
    for (size_t i = 0; i < final_features.size(); ++i) {
      sample.at<float>(0, i) = final_features[i];
    }

    float prediccion = rf_model->predict(sample);

    // --- PASO 5: RESPUESTA JSON ---
    json response;
    response["zona"] = zona;
    response["riesgo_predicho"] = (int)prediccion; // 0 o 1
    response["nivel"] = (prediccion > 0.5) ? "ALTO" : "BAJO";
    response["mensaje"] = (prediccion > 0.5) ? "Se recomienda precaucion."
                                             : "Zona segura segun historial.";

    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_content(response.dump(), "application/json");
  });

  std::cout << "Servidor corriendo en http://localhost:8080" << std::endl;
  svr.listen("0.0.0.0", 8080);

  return 0;
}
