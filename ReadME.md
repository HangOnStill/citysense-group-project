# CitySense — Urban Sensor Simulator

## Overview
CitySense simulates smart-city sensor data (traffic, air quality, and noise) using modern C++20.  
The simulator reads CSV/JSON inputs, aggregates readings in rolling windows, detects anomalies,  
and outputs console and JSON summaries.

The project focuses on modular design, RAII, exception safety, concurrency, and testing.

---

## Quick Start

### Build
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j