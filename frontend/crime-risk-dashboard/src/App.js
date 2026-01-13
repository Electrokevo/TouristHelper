import React, { useState, useEffect } from "react";
import './App.css';
import ProbabilityChart from "./components/Probabilities";
import HeatMap from "./components/HeatMap";
import LocationSelector from "./components/SelectProvince"; // Asegúrate de importar el nuevo selector

function App() {
    // Estados para guardar la selección del usuario
    const [provincia, setProvincia] = useState("");
    const [canton, setCanton] = useState("");
    const [currentStats, setCurrentStats] = useState([]);
    // Estado para guardar los puntos que vienen del backend C++
    const [mapData, setMapData] = useState([]);

    // ESTA ES LA FUNCIÓN QUE FALTABA (La que corrige el error "is not a function")
    const handleLocationChange = (nuevaProv, nuevoCant) => {
        console.log("Ubicación cambiada:", nuevaProv, nuevoCant);
        setProvincia(nuevaProv);
        setCanton(nuevoCant);
        setCurrentStats([]);
    };

    // Efecto: Se ejecuta cada vez que cambia la provincia o el cantón
    useEffect(() => {
        // Solo hacemos la petición si tenemos datos válidos
        if (provincia && canton) {
            const url = `http://localhost:8000/riesgo?provincia=${encodeURIComponent(provincia)}&canton=${encodeURIComponent(canton)}`;

            console.log("Consultando backend C++:", url);

            fetch(url)
                .then((res) => res.json())
                .then((json) => {
                    const heatPoints = json.map((p) => [
                        p.latitud, p.longitud, p.riesgo
                    ]);
                    setMapData(heatPoints);

                    // 2. Datos para la barra lateral
                    // Como filtramos por cantón, tomamos el primer resultado (o el único)
                    if (json.length > 0 && json[0].desglose) {
                        setCurrentStats(json[0].desglose);
                    }
                    // Transformamos la respuesta del backend al formato que necesita el mapa [lat, lon, intensidad]
                    console.log(heatPoints)
                })
                .catch((err) => console.error("Error cargando datos del backend:", err));
        }
    }, [provincia, canton]); // <- Dependencias del useEffect

    return (
        <div style={{ display: "flex", height: "100vh", overflow: "hidden" }}>

            {/* --- PANEL LATERAL --- */}
            <div
                style={{
                    width: "300px",
                    background: "#f5f5f5",
                    borderRight: "1px solid #ddd",
                    padding: "20px",
                    display: "flex",
                    flexDirection: "column",
                    gap: "20px"
                }}
            >
                <h2 style={{ margin: 0, color: "#333" }}>TouristHelper 🇪🇨</h2>
                <p style={{ fontSize: "14px", color: "#666" }}>
                    Sistema predictivo de riesgo delictivo basado en IA.
                </p>

                {/* Aquí pasamos la función handleLocationChange al hijo */}
                <LocationSelector onLocationChange={handleLocationChange} />

                {/* GRÁFICA DE BARRAS (Solo aparece si hay datos) */}
                {currentStats.length > 0 ? (
                    <ProbabilityChart data={currentStats} />
                ) : (
                    canton && <p style={{fontSize: "12px", color: "#999", textAlign: "center"}}>Cargando análisis...</p>
                )}

                {/* Debug visual para confirmar que el estado cambia */}
                <div style={{ fontSize: "12px", color: "#888", marginTop: "auto" }}>
                    Estado Actual:<br/>
                    Prov: {provincia || "-"}<br/>
                    Cant: {canton || "-"}
                </div>
            </div>

            {/* --- CONTENIDO PRINCIPAL (MAPA) --- */}
            <div style={{ flexGrow: 1, position: "relative" }}>
                {/* Pasamos los datos (puntos) al mapa, no solo el nombre de la provincia */}
                <HeatMap data={mapData} center={mapData.length > 0 ? [mapData[0][0], mapData[0][1]] : null} />

                {/* Mensaje flotante si no hay datos */}
                {mapData.length === 0 && (
                    <div style={{
                        position: "absolute",
                        top: "50%",
                        left: "50%",
                        transform: "translate(-50%, -50%)",
                        background: "rgba(255,255,255,0.9)",
                        padding: "20px",
                        borderRadius: "10px",
                        boxShadow: "0 4px 6px rgba(0,0,0,0.1)",
                        textAlign: "center"
                    }}>
                        <h3>Selecciona una zona</h3>
                        <p>Elige una provincia y cantón para ver el análisis de riesgo.</p>
                    </div>
                )}
            </div>
        </div>
    );
}

export default App;