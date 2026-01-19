import { useEffect, useRef } from "react";
import L from "leaflet";
import "leaflet/dist/leaflet.css";
import "leaflet.heat";

// Icono fix para Leaflet en React (a veces los marcadores no cargan sin esto)
import icon from 'leaflet/dist/images/marker-icon.png';
import iconShadow from 'leaflet/dist/images/marker-shadow.png';

let DefaultIcon = L.icon({
  iconUrl: icon,
  shadowUrl: iconShadow,
  iconSize: [25, 41],
  iconAnchor: [12, 41]
});
L.Marker.prototype.options.icon = DefaultIcon;

export default function HeatMap({ data, center }) {
  const mapRef = useRef(null);
  const heatLayerRef = useRef(null);

  // 1. Inicializar mapa (Solo una vez)
  useEffect(() => {
    if (!mapRef.current) {
      // Coordenadas iniciales (Centro de Ecuador)
      mapRef.current = L.map("map").setView([-1.8312, -78.1834], 6);

      L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", {
        attribution: "© OpenStreetMap contributors",
      }).addTo(mapRef.current);
    }

    // Cleanup al desmontar
    return () => {
      if (mapRef.current) {
        mapRef.current.remove();
        mapRef.current = null;
      }
    };
  }, []);

  // 2. Actualizar Heatmap cuando cambia 'data'
  useEffect(() => {
    const map = mapRef.current;
    if (!map) return;

    // A. Limpiar capa anterior
    if (heatLayerRef.current) {
      map.removeLayer(heatLayerRef.current);
      heatLayerRef.current = null;
    }

    // B. Si hay datos, crear nueva capa
    if (data && data.length > 0) {
      console.log(`Renderizando ${data.length} puntos de calor en el mapa.`);

      heatLayerRef.current = L.heatLayer(data, {
        radius: 35,      // Radio del punto de calor
        blur: 20,        // Difuminado
        maxZoom: 14,     // Zoom máximo para intensidad máxima
        max: 1.0,        // Intensidad máxima esperada (nuestro backend manda 0.0 a 1.0)
        gradient: {      // Gradiente de colores personalizado (Verde -> Rojo)
          0.1: 'blue',
          0.4: 'lime',
          0.6: 'yellow',
          0.9: 'red'
        }
      }).addTo(map);
    }
  }, [data]);

  // 3. Actualizar Vista (Centro) cuando cambia 'center'
  useEffect(() => {
    const map = mapRef.current;
    if (!map || !center) return;

    // Animación suave hacia la nueva zona seleccionada
    // Zoom 12 es un buen nivel para ver un cantón/ciudad
    map.flyTo(center, 12, {
      duration: 1.5 // segundos de animación
    });

  }, [center]);

  return <div id="map" style={{ width: "100%", height: "100%" }} />;
}