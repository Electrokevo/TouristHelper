import React from "react";

export default function SelectProvince({ value, onChange }) {
  const provincias = [
    "Azuay",
    "Bolívar",
    "Cañar",
    "Carchi",
    "Chimborazo",
    "Cotopaxi",
    "El Oro",
    "Esmeraldas",
    "Galápagos",
    "Guayas",
    "Imbabura",
    "Loja",
    "Los Ríos",
    "Manabí",
    "Morona Santiago",
    "Napo",
    "Orellana",
    "Pastaza",
    "Pichincha",
    "Santa Elena",
    "Santo Domingo",
    "Sucumbíos",
    "Tungurahua",
    "Zamora Chinchipe"
  ];

  return (
    <div style={{ padding: "10px" }}>
      <label style={{ fontSize: "16px", fontWeight: "bold" }}>
        Selecciona una provincia:
      </label>

      <select
        value={value}
        onChange={(e) => onChange(e.target.value)}
        style={{
          marginTop: "10px",
          padding: "8px",
          width: "100%",
          fontSize: "16px",
          borderRadius: "6px",
          border: "1px solid #ccc"
        }}
      >
        <option value="">-- Selecciona --</option>

        {provincias.map((prov) => (
          <option key={prov} value={prov}>
            {prov}
          </option>
        ))}
      </select>
    </div>
  );
}
