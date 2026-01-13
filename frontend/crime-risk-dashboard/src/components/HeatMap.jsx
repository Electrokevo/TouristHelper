import { useEffect } from "react";
import L from "leaflet";
import "leaflet/dist/leaflet.css";
import "leaflet.heat";

export default function HeatMap({ provincia }) {
  useEffect(() => {
    const map = L.map("map").setView([-1.8312, -78.1834], 6); // Ecuador

    L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", {
      attribution: "© OpenStreetMap contributors",
    }).addTo(map);

    // Datos de ejemplo por provincia
    const heatDataByProvince = {
      Pichincha: [
        [-0.1807, -78.4678, 1.0],
        [-0.2000, -78.4800, 1.0]
      ],
      Guayas: [
        [-2.1700, -79.9000, 2.0],
        [-2.1900, -79.9300, 2.0],
      ],
      Manabí: [
        [-0.9500, -80.7300, 0.5],
        [-1.0500, -80.4500, 0.9]
      ],
    };

    // Obtener heatData según provincia seleccionada
    const heatData = heatDataByProvince[provincia] || [];

    // Mostrar en consola para verificar
    console.log("heatData actual:", heatData);

    if (heatData.length > 0) {
      L.heatLayer(heatData, { radius: 100 }).addTo(map);
    }

    return () => map.remove();
  }, [provincia]);

  return <div id="map" style={{ width: "100%", height: "100vh" }} />;
}
