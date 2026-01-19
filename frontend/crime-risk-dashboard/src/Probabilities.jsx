import React from "react";

export default function ProbabilityChart({ data }) {
    if (!data || data.length === 0) return null;

    // Ordenamos de mayor a menor riesgo para que se vea mejor
    const sortedData = [...data].sort((a, b) => b.probabilidad - a.probabilidad);

    return (
        <div style={{ marginTop: "20px", padding: "10px", background: "white", borderRadius: "8px", boxShadow: "0 1px 3px rgba(0,0,0,0.1)" }}>
            <h3 style={{ fontSize: "14px", marginBottom: "10px", borderBottom: "1px solid #eee", paddingBottom: "5px" }}>
                Probabilidad por Tipo de Incidente
            </h3>

            <div style={{ display: "flex", flexDirection: "column", gap: "8px" }}>
                {sortedData.map((item, idx) => (
                    <div key={idx} style={{ fontSize: "12px" }}>
                        <div style={{ display: "flex", justifyContent: "space-between", marginBottom: "2px" }}>
                            <span style={{ fontWeight: "500" }}>{item.servicio}</span>
                            <span style={{ color: "#666" }}>{item.probabilidad.toFixed(1)}%</span>
                        </div>

                        {/* Barra de fondo */}
                        <div style={{ width: "100%", height: "6px", background: "#f0f0f0", borderRadius: "3px", overflow: "hidden" }}>
                            {/* Barra de progreso */}
                            <div
                                style={{
                                    width: `${item.probabilidad}%`,
                                    height: "100%",
                                    // Color dinámico: Rojo si es alto, Azul si es bajo
                                    background: item.probabilidad > 50 ? "#ff4d4f" : item.probabilidad > 20 ? "#faad14" : "#1890ff",
                                    borderRadius: "3px"
                                }}
                            />
                        </div>
                    </div>
                ))}
            </div>
        </div>
    );
}