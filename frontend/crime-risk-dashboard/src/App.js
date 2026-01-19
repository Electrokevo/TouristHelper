import React, { useState } from 'react';
import SelectProvince from './SelectProvince'; // Tu componente selector
import HeatMap from './HeatMap';             // Tu componente mapa
import zonasData from './zonas_mapeadas.json'; // Tu JSON de coordenadas
import './App.css';

function App() {
    // Estado para el mapa
    const [mapCenter, setMapCenter] = useState([-1.8312, -78.1834]); // Centro Ecuador
    const [heatData, setHeatData] = useState([]); // Puntos de calor [[lat, lon, intensidad]]

    // Estado para resultados y UI
    const [resultado, setResultado] = useState(null);
    const [loading, setLoading] = useState(false);
    const [error, setError] = useState("");

    // Función que llama SelectProvince cuando el usuario elige un cantón
    const handleLocationChange = async (provincia, canton) => {
        // Si se resetea el cantón, limpiamos resultados
        if (!canton) {
            setResultado(null);
            setHeatData([]);
            return;
        }

        setLoading(true);
        setError("");

        try {
            // 1. Buscar coordenadas en tu JSON local
            const zonaInfo = zonasData.find(z =>
                z.nombreCanton === canton && z.nombreProvincia === provincia
            );

            if (!zonaInfo) {
                throw new Error("Coordenadas no encontradas para esta zona.");
            }

            // 2. Consultar al Backend
            const zonaQuery = canton.toUpperCase();
            const response = await fetch(`http://localhost:8080/predict?zona=${zonaQuery}`);

            if (!response.ok) throw new Error("Error conectando con el servidor.");

            const data = await response.json();
            setResultado(data); // Guardamos la respuesta del server (nivel, mensaje, etc)

            // 3. Configurar el Mapa y el Heatmap
            // Intensidad: ALTO = 1.0 (Rojo), BAJO = 0.2 (Azul/Verde)
            const intensidad = data.nivel === "ALTO" ? 1.0 : 0.2;

            // Actualizamos centro del mapa
            setMapCenter([zonaInfo.lat, zonaInfo.lon]);

            // Ponemos el punto de calor
            setHeatData([
                [zonaInfo.lat, zonaInfo.lon, intensidad]
            ]);

        } catch (err) {
            console.error(err);
            setError("No se pudo analizar la zona. Intenta con otra.");
            setHeatData([]);
        } finally {
            setLoading(false);
        }
    };

    return (
        <div className="app-layout">
            {/* PANEL LATERAL (IZQUIERDA) */}
            <div className="sidebar">
                <header className="sidebar-header">
                    <h1>🛡️ TouristHelper</h1>
                    <p>Predicción de Seguridad</p>
                </header>

                <div className="control-panel">
                    <h3>Selecciona tu Destino</h3>
                    {/* Usamos tu componente SelectProvince */}
                    <SelectProvince onLocationChange={handleLocationChange} />

                    {error && <div className="error-msg">{error}</div>}
                    {loading && <div className="loading-msg">Analizando datos históricos...</div>}
                </div>

                {/* TARJETA DE RESULTADOS */}
                {resultado && !loading && (
                    <div className={`result-card ${resultado.nivel === 'ALTO' ? 'risk-high' : 'risk-low'}`}>
                        <h3>{resultado.zona}</h3>

                        <div className="badge-container">
                            <span className="risk-badge">RIESGO {resultado.nivel}</span>
                        </div>

                        <p className="prediction-msg">
                            {resultado.mensaje}
                        </p>

                        <div className="meta-info">
                            <small>📅 Datos actualizados al: {resultado.fecha_datos}</small>
                            <br/>
                            <small>🤖 Probabilidad del modelo: {resultado.riesgo_predicho}</small>
                        </div>
                    </div>
                )}
            </div>

            {/* MAPA (DERECHA) */}
            <div className="map-container">
                {/* Usamos tu componente HeatMap */}
                <HeatMap data={heatData} center={mapCenter} />
            </div>
        </div>
    );
}
export default App;