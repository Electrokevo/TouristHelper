import './App.css';
import HeatMap from "./components/HeatMap";
import SelectProvince from "./components/SelectProvince";
import { useState } from "react";

function App() {
  const [provincia, setProvincia] = useState("");
  return (
    <div style={{ display: "flex" }}>
      {/* Panel lateral */}
      <div
        style={{
          width: "250px",
          background: "#f5f5f5",
          borderRight: "1px solid #ddd",
          height: "100vh",
          padding: "15px"
        }}
      >
        <SelectProvince value={provincia} onChange={setProvincia} />
      </div>
      {/* Contenido principal */}
      <div style={{ flexGrow: 1 }}>
        <HeatMap provincia={provincia} />
      </div>
    </div>
  );
}

export default App;
