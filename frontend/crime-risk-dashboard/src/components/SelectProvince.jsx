import React, { useState, useEffect, useMemo } from "react";

// Asegúrate de que la ruta sea correcta
import zonasData from "./zonas_mapeadas.json";

export default function LocationSelector({ onLocationChange }) {
    const [selectedProvincia, setSelectedProvincia] = useState("");
    const [selectedCanton, setSelectedCanton] = useState("");

    // 1. Extraer lista única de Provincias del JSON
    const provincias = useMemo(() => {
        // Usamos Set para eliminar duplicados y sort para ordenar alfabéticamente
        const uniqueProvs = [...new Set(zonasData.map((z) => z.nombreProvincia))];
        return uniqueProvs.sort();
    }, []);

    // 2. Calcular Cantones disponibles basados en la Provincia seleccionada
    const cantonesDisponibles = useMemo(() => {
        if (!selectedProvincia) return [];

        // Filtramos el JSON por la provincia seleccionada
        const filtrados = zonasData.filter(
            (z) => z.nombreProvincia === selectedProvincia
        );

        // Extraemos solo los nombres de los cantones y quitamos duplicados
        const uniqueCantons = [...new Set(filtrados.map((z) => z.nombreCanton))];
        return uniqueCantons.sort();
    }, [selectedProvincia]);

    // Manejador de cambio de Provincia
    const handleProvinciaChange = (e) => {
        const nuevaProvincia = e.target.value;
        setSelectedProvincia(nuevaProvincia);

        // Importante: Reiniciar el cantón cuando cambia la provincia
        setSelectedCanton("");

        // Notificar al padre (solo provincia por ahora)
        onLocationChange(nuevaProvincia, "");
    };

    // Manejador de cambio de Cantón
    const handleCantonChange = (e) => {
        const nuevoCanton = e.target.value;
        setSelectedCanton(nuevoCanton);

        // Notificar al padre la selección completa
        onLocationChange(selectedProvincia, nuevoCanton);
    };

    // Estilos compartidos
    const labelStyle = { fontSize: "16px", fontWeight: "bold", display: "block", marginBottom: "5px" };
    const selectStyle = {
        padding: "8px",
        width: "100%",
        fontSize: "16px",
        borderRadius: "6px",
        border: "1px solid #ccc",
        marginBottom: "15px"
    };

    return (
        <div style={{ padding: "10px", backgroundColor: "#f9f9f9", borderRadius: "8px" }}>

            {/* SELECCIONAR PROVINCIA */}
            <div>
                <label style={labelStyle}>Provincia:</label>
                <select
                    value={selectedProvincia}
                    onChange={handleProvinciaChange}
                    style={selectStyle}
                >
                    <option value="">-- Selecciona Provincia --</option>
                    {provincias.map((prov) => (
                        <option key={prov} value={prov}>
                            {prov} {/* Puedes usar una función capitalize si vienen en MAYÚSCULAS */}
                        </option>
                    ))}
                </select>
            </div>

            {/* SELECCIONAR CANTÓN (Deshabilitado si no hay provincia) */}
            <div>
                <label style={labelStyle}>Cantón:</label>
                <select
                    value={selectedCanton}
                    onChange={handleCantonChange}
                    style={selectStyle}
                    disabled={!selectedProvincia} // Bloqueado hasta elegir provincia
                >
                    <option value="">
                        {selectedProvincia ? "-- Selecciona Cantón --" : "-- Primero elige Provincia --"}
                    </option>

                    {cantonesDisponibles.map((cant) => (
                        <option key={cant} value={cant}>
                            {cant}
                        </option>
                    ))}
                </select>
            </div>
        </div>
    );
}