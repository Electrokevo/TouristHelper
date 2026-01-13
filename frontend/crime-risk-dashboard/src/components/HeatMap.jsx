import { useEffect } from "react";
import L from "leaflet";
import "leaflet/dist/leaflet.css";
import "leaflet.heat";

export default function HeatMap() {
  useEffect(() => {
    const map = L.map("map").setView([-0.1807, -78.4678], 13);

    L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", {
      attribution: "© OpenStreetMap contributors",
    }).addTo(map);

    const heatData = [
      [-0.1807, -78.4678, 0.2],
      [-0.1900, -78.4700, 0.5],
      [-0.1700, -78.4600, 0.8],
    ];

    L.heatLayer(heatData, { radius: 25 }).addTo(map);

    return () => map.remove();
  }, []);

  return (
    <div
      id="map"
      style={{
        width: "100%",
        height: "100vh"
      }}
    />
  );
}
