import { useEffect, useState } from "react";
import HeatMapView from "./components/HeatMap";

function App() {
  const [data, setData] = useState([]);

  useEffect(() => {
    fetch("http://localhost:8000/riesgo")  // URL de tu API
      .then((res) => res.json())
      .then((json) => {
        const heatPoints = json.map((p) => [
          p.latitud,
          p.longitud,
          p.riesgo, // intensidad del color
        ]);
        setData(heatPoints);
      })
      .catch((err) => console.error("Error cargando datos", err));
  }, []);

  return (
    <div className="w-full h-screen">
      <HeatMapView />
    </div>
  );
}

export default App;
