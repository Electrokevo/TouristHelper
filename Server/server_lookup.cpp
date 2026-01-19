#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// Solo necesitamos OpenCV para el Random Forest
#include <opencv2/ml.hpp>
#include <opencv2/opencv.hpp>

// Librería del servidor HTTP
#include "httplib.h"
#include "json.hpp"

using json = nlohmann::json;
using namespace cv;
using namespace cv::ml;

// ==========================================
// CONFIGURACIÓN
// ==========================================
const std::string MODEL_RF_PATH = "random_forest_model.xml";
const std::string CSV_EMBEDDINGS = "embeddings_lstm_gpu.csv";
const std::string CSV_INEC = "datos_202510_ciudades_unicas_RF.csv";
const int EMB_DIM = 32;

// ==========================================
// ESTRUCTURAS DE DATOS EN MEMORIA
// ==========================================

// Mapa 1: Datos Estáticos (INEC) -> [Zona] : [Vector Features]
std::map<std::string, std::vector<float>> inec_cache;

// Mapa 2: Embeddings (LSTM Pre-calculados) -> [Zona] : [Vector Embedding]
// Guardaremos solo el embedding MÁS RECIENTE encontrado en el CSV para cada
// zona.
std::map<std::string, std::vector<float>> embedding_cache;
std::map<std::string, long>
    latest_date_cache; // Para asegurar que usamos la fecha más nueva

Ptr<RTrees> rf_model;
bool system_ready = false;

// ==========================================
// FUNCIONES DE CARGA
// ==========================================

void load_inec(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "[WARN] No se pudo abrir INEC: " << path << std::endl;
    return;
  }
  std::string line, cell;
  std::getline(file, line); // Header
  while (std::getline(file, line)) {
    std::stringstream ss(line);
    std::vector<std::string> row;
    while (std::getline(ss, cell, ','))
      row.push_back(cell);
    if (row.size() < 3)
      continue;

    std::vector<float> feats;
    // Asumiendo formato: ID, Nombre, Feat1, Feat2...
    for (size_t i = 2; i < row.size(); ++i) {
      try {
        feats.push_back(std::stof(row[i]));
      } catch (...) {
        feats.push_back(0.0f);
      }
    }
    inec_cache[row[0]] = feats; // row[0] es la Zona ID/Nombre
  }
  std::cout << "[INFO] INEC Cache cargado: " << inec_cache.size() << " zonas."
            << std::endl;
}

void load_embeddings_lookup(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "[ERROR] No se pudo abrir Embeddings CSV: " << path
              << std::endl;
    return;
  }
  std::string line, cell;
  std::getline(file, line); // Header: zona,fecha,emb_1...emb_32,target

  int loaded_count = 0;
  while (std::getline(file, line)) {
    std::stringstream ss(line);
    std::vector<std::string> row;
    while (std::getline(ss, cell, ','))
      row.push_back(cell);

    // Validación mínima: Zona + Fecha + 32 Embs + Target = 35 cols
    if (row.size() < (2 + EMB_DIM))
      continue;

    std::string zona = row[0];
    long fecha = 0;
    try {
      fecha = std::stol(row[1]);
    } catch (...) {
    }

    // Lógica: Solo guardamos si es la fecha más reciente que hemos visto para
    // esta zona
    if (latest_date_cache.find(zona) == latest_date_cache.end() ||
        fecha >= latest_date_cache[zona]) {

      std::vector<float> embs;
      for (int i = 0; i < EMB_DIM; ++i) {
        try {
          embs.push_back(std::stof(row[2 + i]));
        } catch (...) {
          embs.push_back(0.0f);
        }
      }

      embedding_cache[zona] = embs;
      latest_date_cache[zona] = fecha;
      loaded_count++;
    }
  }
  std::cout << "[INFO] Embeddings Lookup cargado: " << embedding_cache.size()
            << " zonas únicas actualizadas." << std::endl;
}

// ==========================================
// MAIN SERVER
// ==========================================

int main() {
  std::cout << "--- Iniciando Servidor (Modo Lookup/Clasificación) ---"
            << std::endl;

  // 1. Cargar Random Forest (Cerebro de decisión)
  try {
    rf_model = RTrees::load(MODEL_RF_PATH);
    if (rf_model.empty())
      throw std::runtime_error("Modelo RF vacio o no encontrado");
    std::cout << "[INFO] Modelo Random Forest cargado." << std::endl;
  } catch (const cv::Exception &e) {
    std::cerr << "[CRITICAL] Error cargando XML: " << e.what() << std::endl;
    return -1;
  }

  // 2. Cargar Datos en Memoria
  load_inec(CSV_INEC);
  load_embeddings_lookup(CSV_EMBEDDINGS);

  if (embedding_cache.empty()) {
    std::cerr << "[CRITICAL] No se cargaron embeddings. El servidor no puede "
                 "funcionar."
              << std::endl;
    return -1;
  }

  system_ready = true;
  httplib::Server svr;

  // Endpoint: /predict?zona=GUAYAQUIL
  svr.Get("/predict", [&](const httplib::Request &req, httplib::Response &res) {
    if (!system_ready) {
      res.status = 500;
      res.set_content("Sistema no inicializado", "text/plain");
      return;
    }

    std::string zona = req.get_param_value("zona");

    // Normalización básica (Upper case) si es necesario,
    // pero asumimos que el usuario o el front envían igual que el CSV.

    if (zona.empty()) {
      res.status = 400;
      res.set_content("Falta 'zona'", "text/plain");
      return;
    }

    // --- PASO A: BUSCAR EMBEDDING (Lookup) ---
    if (embedding_cache.find(zona) == embedding_cache.end()) {
      // Zona no encontrada en el histórico LSTM
      json err;
      err["error"] = "Zona desconocida o sin datos historicos recientes.";
      res.status = 404;
      res.set_content(err.dump(), "application/json");
      return;
    }

    std::vector<float> lstm_feats = embedding_cache[zona];
    long fecha_dato = latest_date_cache[zona];

    // --- PASO B: BUSCAR INEC ---
    std::vector<float> inec_feats;
    if (inec_cache.count(zona)) {
      inec_feats = inec_cache[zona];
    } else {
      // Relleno seguro si falta INEC
      if (!inec_cache.empty())
        inec_feats.resize(inec_cache.begin()->second.size(), 0.0f);
      else
        inec_feats.resize(5, 0.0f);
    }

    // --- PASO C: FUSIÓN ---
    std::vector<float> final_features = lstm_feats;
    final_features.insert(final_features.end(), inec_feats.begin(),
                          inec_feats.end());

    // --- PASO D: CLASIFICACIÓN RF ---
    cv::Mat sample(1, final_features.size(), CV_32F);
    for (size_t i = 0; i < final_features.size(); ++i) {
      sample.at<float>(0, i) = final_features[i];
    }

    float prediccion = rf_model->predict(sample);

    // --- RESPUESTA ---
    json response;
    response["zona"] = zona;
    response["riesgo_predicho"] = (int)prediccion;
    response["nivel"] = (prediccion > 0.5) ? "ALTO" : "BAJO";
    response["mensaje"] =
        (prediccion > 0.5) ? "Zona de Alto Riesgo basada en historial reciente."
                           : "Zona segura basada en historial reciente.";
    response["fecha_datos"] =
        fecha_dato; // Para que el usuario sepa de cuándo es la info

    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_content(response.dump(), "application/json");
  });

  std::cout << "Servidor escuchando en http://localhost:8080" << std::endl;
  svr.listen("0.0.0.0", 8080);

  return 0;
}
