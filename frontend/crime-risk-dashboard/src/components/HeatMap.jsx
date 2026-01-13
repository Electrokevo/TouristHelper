import { useEffect, useRef } from "react";
import L from "leaflet";
import "leaflet/dist/leaflet.css";
import "leaflet.heat";

export default function HeatMap({ provincia }) {
  const mapRef = useRef(null);
  const heatLayerRef = useRef(null);

  // Coordenadas y zoom por provincia
  const provinceCenter = {
    Pichincha: { coords: [-0.1807, -78.4678], zoom: 10 },
    Guayas: { coords: [-2.1700, -79.9000], zoom: 10 },
    Manabí: { coords: [-0.9500, -80.7300], zoom: 9 },
  };

  // Datos de calor
  const heatDataByProvince = {
    Pichincha: [
      [-0.1807, -78.4678, 0.9],
      [-0.2000, -78.4800, 0.6],
    ],
    Guayas: [
      [-2.1700, -79.9000, 0.7],
      [-2.1900, -79.9300, 0.8],
    ],
    Manabí: [
      [-0.9500, -80.7300, 0.5],
      [-1.0500, -80.4500, 0.9],
    ],
  };

  // Inicializar mapa solo una vez
  useEffect(() => {
    if (!mapRef.current) {
      mapRef.current = L.map("map").setView([-1.8312, -78.1834], 6);

      L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", {
        attribution: "© OpenStreetMap contributors",
      }).addTo(mapRef.current);
    }
  }, []);

  // Actualizar heatmap y zoom cuando cambia la provincia
  useEffect(() => {
    const map = mapRef.current;

    if (!map) return;

    // Eliminar capa anterior si existe
    if (heatLayerRef.current) {
      map.removeLayer(heatLayerRef.current);
    }

    const heatData = heatDataByProvince[provincia] || [];

    if (heatData.length > 0) {
      heatLayerRef.current = L.heatLayer(heatData, {
        radius: 50,
        blur: 20,
        maxZoom: 10
      }).addTo(map);

      // Centrar y hacer zoom en la provincia
      const centerInfo = provinceCenter[provincia];
      if (centerInfo) {
        map.setView(centerInfo.coords, centerInfo.zoom);
      }
    } else {
      // Si no hay datos, mostrar todo Ecuador
      map.setView([-1.8312, -78.1834], 6);
    }
  }, [provincia]);

  return <div id="map" style={{ width: "100%", height: "100vh" }} />;
}
