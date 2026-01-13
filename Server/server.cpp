#include <algorithm> // Para transform (uppercase)
#include <armadillo>
#include <iostream>
#include <mlpack.hpp>
#include <string>
#include <vector>

#include "httplib.h"
#include "json.hpp"

// Tus headers
#include "Encoders.h"
#include "Helpers.h"

using namespace mlpack;
using namespace mlpack::tree;
using namespace std;
using namespace arma;
using json = nlohmann::json;

// Nombres de servicios
std::vector<std::string> SERVICE_NAMES = {
    "Gestión de Siniestros",  "Gestión Sanitaria",     "Seguridad Ciudadana",
    "Servicio Militar",       "Servicios Municipales", "Tránsito y Movilidad",
    "Violencia Intrafamiliar" // Ejemplo, ajusta según tus clases reales
};
// 1. Estructura mejorada para soportar filtros
struct ZoneInfo {
  int idCanton;           // ID numérico para el modelo (Encoder)
  int idProvincia;        // ID numérico para el modelo (Encoder)
  string nombreProvincia; // Texto para filtrar (Ej: "GUAYAS")
  string nombreCanton;    // Texto para filtrar (Ej: "GUAYAQUIL")
  double lat;
  double lon;
};

// 2. Base de datos simulada de zonas (Coordenadas reales aproximadas)
// NOTA: Los IDs deben coincidir con los que usaste en el entrenamiento (Pass 1
// de main.cpp)

vector<ZoneInfo> ZONES = {
    {23, 0, "AZUAY", "CAMILO PONCE ENRIQUEZ", -2.96407, -79.56304},
    {39, 0, "AZUAY", "CHORDELEG", -2.98447, -78.73072},
    {45, 0, "AZUAY", "CUENCA", -2.89741, -79.00417},
    {55, 0, "AZUAY", "EL PAN", -2.83903, -78.65403},
    {66, 0, "AZUAY", "GIRON", -3.15898, -79.11025},
    {70, 0, "AZUAY", "GUACHAPALA", -2.76681, -78.71135},
    {71, 0, "AZUAY", "GUALACEO", -2.91802, -78.84616},
    {116, 0, "AZUAY", "NABON", -3.33283, -79.09966},
    {124, 0, "AZUAY", "OÑA", -3.49631, -79.10928},
    {138, 0, "AZUAY", "PAUTE", -2.74685, -78.73624},
    {151, 0, "AZUAY", "PUCARA", -3.17489, -79.51471},
    {174, 0, "AZUAY", "SAN FERNANDO", -3.13987, -79.27948},
    {188, 0, "AZUAY", "SANTA ISABEL", -3.18394, -79.35941},
    {196, 0, "AZUAY", "SEVILLA DE ORO", -2.70516, -78.60144},
    {200, 0, "AZUAY", "SIGSIG", -3.13317, -78.84544},
    {21, 1, "BOLIVAR", "CALUMA", -1.59767, -79.19633},
    {35, 1, "BOLIVAR", "CHILLANES", -1.94327, -79.06607},
    {36, 1, "BOLIVAR", "CHIMBO", -1.67650, -79.13106},
    {51, 1, "BOLIVAR", "ECHEANDIA", -1.44695, -79.27359},
    {75, 1, "BOLIVAR", "GUARANDA", -1.43986, -79.05767},
    {94, 1, "BOLIVAR", "LAS NAVES", -1.28132, -79.30134},
    {178, 1, "BOLIVAR", "SAN MIGUEL", -1.81206, -79.12726},
    {19, 2, "CARCHI", "BOLIVAR", 0.47264, -77.93585},
    {63, 2, "CARCHI", "ESPEJO", 0.71393, -77.96623},
    {108, 2, "CARCHI", "MIRA", 0.71701, -78.19183},
    {113, 2, "CARCHI", "MONTUFAR", 0.56843, -77.80878},
    {181, 2, "CARCHI", "SAN PEDRO DE HUACA", 0.62336, -77.72064},
    {212, 2, "CARCHI", "TULCAN", 0.81157, -77.71722},
    {11, 3, "CAÑAR", "AZOGUES", -2.63173, -78.70089},
    {18, 3, "CAÑAR", "BIBLIAN", -2.67483, -78.98028},
    {28, 3, "CAÑAR", "CAÑAR", -2.47493, -79.18008},
    {49, 3, "CAÑAR", "DELEG", -2.76828, -78.92792},
    {58, 3, "CAÑAR", "EL TAMBO", -2.48281, -78.91340},
    {90, 3, "CAÑAR", "LA TRONCAL", -2.43369, -79.37516},
    {206, 3, "CAÑAR", "SUSCAL", -2.46363, -79.07064},
    {2, 4, "CHIMBORAZO", "ALAUSI", -2.32505, -78.70357},
    {33, 4, "CHIMBORAZO", "CHAMBO", -1.74403, -78.54260},
    {40, 4, "CHIMBORAZO", "CHUNCHI", -2.32857, -78.89328},
    {42, 4, "CHIMBORAZO", "COLTA", -1.79709, -78.84944},
    {46, 4, "CHIMBORAZO", "CUMANDA", -2.21087, -79.10358},
    {73, 4, "CHIMBORAZO", "GUAMOTE", -2.04933, -78.64267},
    {74, 4, "CHIMBORAZO", "GUANO", -1.53992, -78.65784},
    {130, 4, "CHIMBORAZO", "PALLATANGA", -2.01673, -78.93023},
    {143, 4, "CHIMBORAZO", "PENIPE", -1.55404, -78.45692},
    {165, 4, "CHIMBORAZO", "RIOBAMBA", -1.67328, -78.64825},
    {89, 5, "COTOPAXI", "LA MANA", -0.78419, -79.10106},
    {95, 5, "COTOPAXI", "LATACUNGA", -0.93403, -78.61458},
    {133, 5, "COTOPAXI", "PANGUA", -1.10105, -79.16454},
    {155, 5, "COTOPAXI", "PUJILI", -1.00770, -78.85592},
    {169, 5, "COTOPAXI", "SALCEDO", -1.05700, -78.54111},
    {194, 5, "COTOPAXI", "SAQUISILI", -0.83598, -78.73126},
    {199, 5, "COTOPAXI", "SIGCHOS", -0.61302, -78.88186},
    {8, 6, "EL ORO", "ARENILLAS", -3.58592, -80.09743},
    {10, 6, "EL ORO", "ATAHUALPA", -3.55716, -79.70977},
    {15, 6, "EL ORO", "BALSAS", -3.77422, -79.83110},
    {34, 6, "EL ORO", "CHILLA", -3.43762, -79.62833},
    {54, 6, "EL ORO", "EL GUABO", -3.16290, -79.75845},
    {78, 6, "EL ORO", "HUAQUILLAS", -3.47562, -80.18445},
    {93, 6, "EL ORO", "LAS LAJAS", -3.80072, -80.08015},
    {102, 6, "EL ORO", "MACHALA", -3.31489, -79.95128},
    {104, 6, "EL ORO", "MARCABELI", -3.77334, -79.92234},
    {135, 6, "EL ORO", "PASAJE", -3.32349, -79.76260},
    {147, 6, "EL ORO", "PIÑAS", -3.71068, -79.78322},
    {149, 6, "EL ORO", "PORTOVELO", -3.71523, -79.61720},
    {190, 6, "EL ORO", "SANTA ROSA", -3.41455, -80.15490},
    {221, 6, "EL ORO", "ZARUMA", -3.51119, -79.51317},
    {9, 7, "ESMERALDAS", "ATACAMES", 0.79711, -79.87908},
    {60, 7, "ESMERALDAS", "ELOY ALFARO", 0.87404, -78.99503},
    {62, 7, "ESMERALDAS", "ESMERALDAS", 0.96682, -79.65238},
    {115, 7, "ESMERALDAS", "MUISNE", 0.53872, -79.88172},
    {162, 7, "ESMERALDAS", "QUININDE", 0.32872, -79.47264},
    {166, 7, "ESMERALDAS", "RIOVERDE", 1.07431, -79.41229},
    {177, 7, "ESMERALDAS", "SAN LORENZO", 0.98506, -78.68693},
    {80, 8, "GALAPAGOS", "ISABELA", -0.96210, -90.95895},
    {173, 8, "GALAPAGOS", "SAN CRISTOBAL", -0.91512, -89.56630},
    {186, 8, "GALAPAGOS", "SANTA CRUZ", -0.62882, -90.36388},
    {3, 9, "GUAYAS", "ALFREDO BAQUERIZO MORENO", -1.94943, -79.54953},
    {14, 9, "GUAYAS", "BALAO", -2.88525, -79.71805},
    {16, 9, "GUAYAS", "BALZAR", -1.30630, -79.93671},
    {41, 9, "GUAYAS", "COLIMES", -1.51616, -80.08188},
    {-1, -1, "GUAYAS", "CRNEL. MARCELINO MARIDUEÑA", 0.00000, 0.00000},
    {48, 9, "GUAYAS", "DAULE", -1.92001, -79.91293},
    {50, 9, "GUAYAS", "DURAN", -2.21270, -79.78959},
    {59, 9, "GUAYAS", "EL TRIUNFO", -2.29078, -79.36606},
    {61, 9, "GUAYAS", "EMPALME", -0.99119, -79.66441},
    {-1, -1, "GUAYAS", "GNRAL. ANTONIO ELIZALDE", 0.00000, 0.00000},
    {76, 9, "GUAYAS", "GUAYAQUIL", -2.19006, -79.88687},
    {81, 9, "GUAYAS", "ISIDRO AYORA", -1.87968, -80.14631},
    {99, 9, "GUAYAS", "LOMAS DE SARGENTILLO", -1.83484, -80.07895},
    {107, 9, "GUAYAS", "MILAGRO", -2.11796, -79.55520},
    {118, 9, "GUAYAS", "NARANJAL", -2.57313, -79.53256},
    {119, 9, "GUAYAS", "NARANJITO", -2.15904, -79.39046},
    {120, 9, "GUAYAS", "NOBOL", -1.98178, -80.06707},
    {129, 9, "GUAYAS", "PALESTINA", -1.63190, -79.92733},
    {140, 9, "GUAYAS", "PEDRO CARBO", -1.86727, -80.30098},
    {148, 9, "GUAYAS", "PLAYAS", -2.59379, -80.42615},
    {171, 9, "GUAYAS", "SALITRE", -1.82892, -79.81873},
    {172, 9, "GUAYAS", "SAMBORONDON", -2.01557, -79.75627},
    {175, 9, "GUAYAS", "SAN JACINTO DE YAGUACHI", -2.11785, -79.73594},
    {189, 9, "GUAYAS", "SANTA LUCIA", -1.71459, -80.04263},
    {201, 9, "GUAYAS", "SIMON BOLIVAR", -2.04241, -79.42186},
    {5, 10, "IMBABURA", "ANTONIO ANTE", 0.33160, -78.21962},
    {43, 10, "IMBABURA", "COTACACHI", 0.39552, -78.45070},
    {79, 10, "IMBABURA", "IBARRA", 0.51594, -78.13294},
    {123, 10, "IMBABURA", "OTAVALO", 0.22276, -78.24543},
    {145, 10, "IMBABURA", "PIMAMPIRO", 0.39330, -77.94027},
    {180, 10, "IMBABURA", "SAN MIGUEL DE URCUQUI", 0.57772, -78.33491},
    {22, 11, "LOJA", "CALVAS", -4.34030, -79.58639},
    {26, 11, "LOJA", "CATAMAYO", -3.99121, -79.40379},
    {29, 11, "LOJA", "CELICA", -4.16481, -79.98518},
    {32, 11, "LOJA", "CHAGUARPAMBA", -3.87654, -79.64466},
    {64, 11, "LOJA", "ESPINDOLA", -4.57331, -79.40462},
    {69, 11, "LOJA", "GONZANAMA", -4.15003, -79.46889},
    {98, 11, "LOJA", "LOJA", -3.99684, -79.20167},
    {101, 11, "LOJA", "MACARA", -4.34904, -79.91545},
    {121, 11, "LOJA", "OLMEDO", -3.93984, -79.60492},
    {132, 11, "LOJA", "PALTAS", -4.00262, -79.70066},
    {146, 11, "LOJA", "PINDAL", -4.10051, -80.13881},
    {157, 11, "LOJA", "PUYANGO", -3.96535, -80.07354},
    {161, 11, "LOJA", "QUILANGA", -4.35195, -79.38234},
    {195, 11, "LOJA", "SARAGURO", -3.52695, -79.30639},
    {202, 11, "LOJA", "SOZORANGA", -4.30046, -79.79473},
    {220, 11, "LOJA", "ZAPOTILLO", -4.23413, -80.28909},
    {12, 12, "LOS RIOS", "BABA", -1.67277, -79.66158},
    {13, 12, "LOS RIOS", "BABAHOYO", -1.87626, -79.50689},
    {20, 12, "LOS RIOS", "BUENA FE", -0.74887, -79.52970},
    {109, 12, "LOS RIOS", "MOCACHE", -1.19790, -79.55127},
    {111, 12, "LOS RIOS", "MONTALVO", -1.80259, -79.34813},
    {128, 12, "LOS RIOS", "PALENQUE", -1.33533, -79.71826},
    {152, 12, "LOS RIOS", "PUEBLOVIEJO", -1.53685, -79.54741},
    {159, 12, "LOS RIOS", "QUEVEDO", -1.06697, -79.47768},
    {163, 12, "LOS RIOS", "QUINSALOMA", -1.14606, -79.34690},
    {213, 12, "LOS RIOS", "URDANETA", -1.56802, -79.37697},
    {214, 12, "LOS RIOS", "VALENCIA", -0.76965, -79.32114},
    {215, 12, "LOS RIOS", "VENTANAS", -1.32539, -79.33516},
    {216, 12, "LOS RIOS", "VINCES", -1.52353, -79.76605},
    {0, 13, "MANABI", "24 DE MAYO", -1.38427, -80.33915},
    {19, 13, "MANABI", "BOLIVAR", -0.92577, -80.01740},
    {38, 13, "MANABI", "CHONE", -0.38281, -80.07216},
    {52, 13, "MANABI", "EL CARMEN", -0.48095, -79.57415},
    {65, 13, "MANABI", "FLAVIO ALFARO", -0.32429, -79.84454},
    {82, 13, "MANABI", "JAMA", -0.21495, -80.22958},
    {83, 13, "MANABI", "JARAMIJO", -0.97439, -80.61346},
    {84, 13, "MANABI", "JIPIJAPA", -1.51582, -80.61250},
    {85, 13, "MANABI", "JUNIN", -0.93947, -80.20541},
    {103, 13, "MANABI", "MANTA", -0.94864, -80.71908},
    {112, 13, "MANABI", "MONTECRISTI", -1.11496, -80.67758},
    {121, 13, "MANABI", "OLMEDO", -1.36952, -80.21449},
    {126, 13, "MANABI", "PAJAN", -1.70408, -80.42130},
    {139, 13, "MANABI", "PEDERNALES", 0.06933, -79.84517},
    {144, 13, "MANABI", "PICHINCHA", -0.89612, -79.80540},
    {150, 13, "MANABI", "PORTOVIEJO", -1.05282, -80.45341},
    {153, 13, "MANABI", "PUERTO LOPEZ", -1.54408, -80.74452},
    {167, 13, "MANABI", "ROCAFUERTE", -0.90324, -80.40121},
    {183, 13, "MANABI", "SAN VICENTE", -0.43739, -80.37614},
    {184, 13, "MANABI", "SANTA ANA", -1.19447, -80.26313},
    {203, 13, "MANABI", "SUCRE", -0.73268, -80.41991},
    {211, 13, "MANABI", "TOSAGUA", -0.77552, -80.25772},
    {72, 14, "MORONA SANTIAGO", "GUALAQUIZA", -3.32771, -78.70649},
    {77, 14, "MORONA SANTIAGO", "HUAMBOYA", -1.98408, -77.98209},
    {96, 14, "MORONA SANTIAGO", "LIMON INDANZA", -3.05243, -78.31800},
    {-1, -1, "MORONA SANTIAGO", "LOGROÐO", 0.00000, 0.00000},
    {114, 14, "MORONA SANTIAGO", "MORONA", -2.43030, -77.88039},
    {125, 14, "MORONA SANTIAGO", "PABLO SEXTO", -1.84088, -78.29135},
    {131, 14, "MORONA SANTIAGO", "PALORA", -1.69814, -77.96753},
    {176, 14, "MORONA SANTIAGO", "SAN JUAN BOSCO", -3.25938, -78.38519},
    {191, 14, "MORONA SANTIAGO", "SANTIAGO", -3.04894, -78.00696},
    {197, 14, "MORONA SANTIAGO", "SEVILLA DON BOSCO", -2.31506, -78.10372},
    {204, 14, "MORONA SANTIAGO", "SUCUA", -2.45601, -78.17274},
    {207, 14, "MORONA SANTIAGO", "TAISHA", -2.44064, -77.38273},
    {210, 14, "MORONA SANTIAGO", "TIWINTZA", -2.98133, -77.93304},
    {7, 15, "NAPO", "ARCHIDONA", -0.71607, -77.95504},
    {24, 15, "NAPO", "CARLOS JULIO AROSEMENA TOLA", -1.16676, -77.93987},
    {53, 15, "NAPO", "EL CHACO", -0.24428, -77.67596},
    {160, 15, "NAPO", "QUIJOS", -0.45465, -77.93030},
    {208, 15, "NAPO", "TENA", -0.94387, -78.10840},
    {1, 16, "ORELLANA", "AGUARICO", -0.98462, -75.98915},
    {87, 16, "ORELLANA", "LA JOYA DE LOS SACHAS", -0.31614, -76.86115},
    {100, 16, "ORELLANA", "LORETO", -0.64719, -77.32527},
    {122, 16, "ORELLANA", "ORELLANA", -0.29358, -76.85094},
    {6, 17, "PASTAZA", "ARAJUNO", -1.35374, -76.84411},
    {106, 17, "PASTAZA", "MERA", -1.44629, -78.11619},
    {136, 17, "PASTAZA", "PASTAZA", -1.95149, -76.84976},
    {185, 17, "PASTAZA", "SANTA CLARA", -1.25618, -77.87013},
    {27, 18, "PICHINCHA", "CAYAMBE", 0.02516, -77.98896},
    {105, 18, "PICHINCHA", "MEJIA", -0.21088, -78.51872},
    {141, 18, "PICHINCHA", "PEDRO MONCAYO", 0.05946, -78.26509},
    {142, 18, "PICHINCHA", "PEDRO VICENTE MALDONADO", 0.16121, -79.04006},
    {154, 18, "PICHINCHA", "PUERTO QUITO", 0.15530, -79.23307},
    {164, 18, "PICHINCHA", "QUITO", -0.22016, -78.51233},
    {168, 18, "PICHINCHA", "RUMIÑAHUI", -0.58595, -78.50798},
    {179, 18, "PICHINCHA", "SAN MIGUEL DE LOS BANCOS", -0.01884, -78.92147},
    {88, 19, "SANTA ELENA", "LA LIBERTAD", -2.22081, -80.90776},
    {170, 19, "SANTA ELENA", "SALINAS", -2.26125, -80.92380},
    {187, 19, "SANTA ELENA", "SANTA ELENA", -2.22701, -80.85769},
    {86, 20, "SANTO DOMINGO DE LOS TSACHILAS", "LA CONCORDIA", -0.04411,
     -79.46298},
    {193, 20, "SANTO DOMINGO DE LOS TSACHILAS", "SANTO DOMINGO", -0.24750,
     -79.17132},
    {25, 21, "SUCUMBIOS", "CASCALES", 0.15083, -77.23195},
    {47, 21, "SUCUMBIOS", "CUYABENO", -0.33393, -75.72785},
    {68, 21, "SUCUMBIOS", "GONZALO PIZARRO", 0.11169, -77.63988},
    {91, 21, "SUCUMBIOS", "LAGO AGRIO", 0.12663, -76.75759},
    {156, 21, "SUCUMBIOS", "PUTUMAYO", 0.13403, -76.13079},
    {198, 21, "SUCUMBIOS", "SHUSHUFINDI", -0.28851, -76.59069},
    {205, 21, "SUCUMBIOS", "SUCUMBIOS", 0.39739, -77.63468},
    {4, 22, "TUNGURAHUA", "AMBATO", -1.24224, -78.62876},
    {17, 22, "TUNGURAHUA", "BAÑOS DE AGUA SANTA", -1.41741, -78.44023},
    {31, 22, "TUNGURAHUA", "CEVALLOS", -1.35494, -78.61640},
    {110, 22, "TUNGURAHUA", "MOCHA", -1.41459, -78.70056},
    {137, 22, "TUNGURAHUA", "PATATE", -1.28317, -78.42976},
    {158, 22, "TUNGURAHUA", "QUERO", -1.43031, -78.60832},
    {182, 22, "TUNGURAHUA", "SAN PEDRO DE PELILEO", -1.33013, -78.54507},
    {192, 22, "TUNGURAHUA", "SANTIAGO DE PILLARO", -1.11636, -78.45284},
    {209, 22, "TUNGURAHUA", "TISALEO", -1.36252, -78.67879},
    {30, 23, "ZAMORA CHINCHIPE", "CENTINELA DEL CONDOR", -3.94493, -78.73347},
    {37, 23, "ZAMORA CHINCHIPE", "CHINCHIPE", -4.85399, -79.13739},
    {56, 23, "ZAMORA CHINCHIPE", "EL PANGUI", -3.62535, -78.54730},
    {117, 23, "ZAMORA CHINCHIPE", "NANGARITZA", -4.31168, -78.79657},
    {127, 23, "ZAMORA CHINCHIPE", "PALANDA", -4.52105, -79.12679},
    {134, 23, "ZAMORA CHINCHIPE", "PAQUISHA", -3.96234, -78.60938},
    {217, 23, "ZAMORA CHINCHIPE", "YACUAMBI", -3.58488, -78.93912},
    {218, 23, "ZAMORA CHINCHIPE", "YANTZAZA", -3.71468, -78.73763},
    {219, 23, "ZAMORA CHINCHIPE", "ZAMORA", -4.00420, -78.95778},
    {-1, -1, "ZONA NO DELIMITADA", "EL PIEDRERO", 0.00000, 0.00000},
    {92, 24, "ZONA NO DELIMITADA", "LAS GOLONDRINAS", 0.32266, -79.21218},
};

// Helper para convertir a mayúsculas (para comparar sin importar si envían
// "Guayas" o "GUAYAS")
string toUpper(string s) {
  transform(s.begin(), s.end(), s.begin(), ::toupper);
  return s;
}

int main() {
  cout << "--- Iniciando Backend TouristHelper ---" << endl;
  LabelEncoder encProv;
  HierarchicalCantonEncoder encCanton;

  // Poblar encoders con las zonas conocidas para que tengan IDs válidos
  // (Esto simula lo que hizo el Pass 1 de tu main.cpp)
  for (const auto &z : ZONES) {
    encProv.encode(z.nombreProvincia);
    int idProv = (int)encProv.encode(z.nombreProvincia);
    encCanton.encode(idProv, z.nombreCanton);
  }
  // Congelar para evitar IDs nuevos accidentales
  encProv.freeze();

  // 3. Cargar Modelo
  RandomForest<GiniGain, RandomDimensionSelect> rf;
  cout << "Cargando modelo Random Forest..." << endl;
  try {
    mlpack::data::Load("rf_model.bin", "rf_model", rf);
    cout << "Modelo cargado OK." << endl;
  } catch (std::exception &e) {
    cerr << "Error crítico: No se encontró rf_model.bin. Asegúrate de ejecutar "
            "el entrenamiento primero."
         << endl;
    return -1;
  }

  httplib::Server svr;

  // 4. Endpoint Inteligente con Filtros
  // Uso: http://localhost:8000/riesgo?provincia=GUAYAS&canton=GUAYAQUIL
  svr.Get("/riesgo", [&](const httplib::Request &req, httplib::Response &res) {
    // A. Leer Parámetros
    string reqProv = req.get_param_value("provincia");
    string reqCant = req.get_param_value("canton");

    cout << "Peticion recibida. Filtros -> Prov: " << reqProv
         << ", Canton: " << reqCant << endl;

    json response_json = json::array();
    string targetProv = toUpper(reqProv);
    string targetCant = toUpper(reqCant);

    // B. Iterar zonas y filtrar
    for (const auto &zone : ZONES) {

      // Lógica de filtrado:
      // Si el usuario no envía nada, devolvemos todo (opcional).
      // Si envía provincia, debe coincidir.
      // Si envía cantón, debe coincidir.
      bool matchProv =
          (reqProv.empty() || toUpper(zone.nombreProvincia) == targetProv);
      bool matchCant =
          (reqCant.empty() || toUpper(zone.nombreCanton) == targetCant);

      if (matchProv && matchCant) {
        // --- 4. OBTENER IDs DINÁMICAMENTE ---
        // Aquí usamos el Encoder real, garantizando que el ID sea el correcto
        // para la combinación Provincia + Cantón.

        float idProvincia = (float)encProv.encode(zone.nombreProvincia);

        // HierarchicalCantonEncoder requiere el ID de la provincia padre
        float idCanton =
            (float)encCanton.encode((int)idProvincia, zone.nombreCanton);

        // Validación: Si devuelve -1 es que no conoce la zona (no debería pasar
        // con ZONES fijo)
        if (idProvincia < 0 || idCanton < 0)
          continue;

        // Construir Vector de Características
        int n_feats = 42;
        fmat queryPoint(n_feats, 1, fill::zeros);

        queryPoint(0, 0) = 202510.0; // Fecha

        // Feature Provincia (One-Hot o Label encoding según tu entrenamiento)
        // Si usaste One-Hot de 24 provincias:
        int pIdx = (int)idProvincia;
        if (pIdx >= 0 && pIdx < 24)
          queryPoint(1 + pIdx, 0) = 1.0f;

        // Feature Cantón (Posición 25 en tu diseño original)
        queryPoint(25, 0) = idCanton;

        // Predecir
        Row<size_t> prediction;
        mat probabilities;
        rf.Classify(queryPoint, prediction, probabilities);

        // Generar Desglose
        json desglose = json::array();
        for (size_t i = 0; i < probabilities.n_rows; ++i) {
          string nombre =
              (i < SERVICE_NAMES.size()) ? SERVICE_NAMES[i] : to_string(i);
          desglose.push_back({{"servicio", nombre},
                              {"probabilidad", probabilities(i, 0) * 100.0}});
        }

        response_json.push_back({{"latitud", zone.lat},
                                 {"longitud", zone.lon},
                                 {"riesgo", (double)probabilities.max()},
                                 {"zona", zone.nombreCanton},
                                 {"provincia", zone.nombreProvincia},
                                 {"desglose", desglose}});
      }
    }
    // Si no hubo coincidencias
    if (response_json.empty()) {
      cout << "No se encontraron zonas para los filtros dados." << endl;
    }

    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_content(response_json.dump(), "application/json");
  });

  cout << "Servidor listo en http://localhost:8000" << endl;
  svr.listen("0.0.0.0", 8000);

  return 0;
}
