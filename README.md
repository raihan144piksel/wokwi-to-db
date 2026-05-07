# Smart Greenhouse IoT: Wokwi to PostgreSQL

A robust IoT monitoring system that captures telemetry data from an ESP32 (simulated in Wokwi) and stores it in a PostgreSQL database (Supabase) via a high-performance Rust backend.

## 🚀 Project Overview

This project demonstrates a full-stack IoT data pipeline:
1.  **Edge (ESP32)**: Collects sensor data (Temperature, Humidity, Light) and sends it over MQTT.
2.  **Broker (HiveMQ Cloud)**: A secure cloud-based MQTT broker for message passing.
3.  **Backend (Rust/Axum)**: A multi-threaded subscriber that processes incoming telemetry and manages a database pool.
4.  **Database (PostgreSQL/Supabase)**: Stores time-series data with optimized indices and Row Level Security (RLS).

## 🛠 Tech Stack

-   **Hardware Simulation**: [Wokwi](https://wokwi.com/) (ESP32, DHT22, LDR).
-   **Firmware**: Arduino/C++ with `PubSubClient` and `ArduinoJson`.
-   **Backend**: Rust using [Axum](https://github.com/tokio-rs/axum) (HTTP Server), [rumqttc](https://github.com/bytebeamio/rumqttc) (MQTT), and [sqlx](https://github.com/launchbadge/sqlx) (SQL Toolkit).
-   **Database**: PostgreSQL hosted on [Supabase](https://supabase.com/).
-   **Connectivity**: MQTT over TLS (Port 8883).

## 📂 Project Structure

```text
.
├── backend/                # Rust Axum Backend
│   ├── src/                # Source code (main.rs, models.rs)
│   ├── migrations/         # SQLX Migration scripts
│   └── Cargo.toml          # Rust dependencies
└── wokwi-esp32/            # ESP32 Firmware
    └── wokwi-esp32.ino     # Arduino source code
```

## ⚙️ Setup & Installation

### 1. Database Setup
- Create a new project on **Supabase**.
- Set up the schema using the migrations found in `backend/migrations/`.
- Copy your `DATABASE_URL`.

### 2. MQTT Setup
- Create a free cluster on **HiveMQ Cloud**.
- Add a set of credentials (username/password).
- Copy the Cluster URL.

### 3. Backend Configuration
Create a `backend/.env` file:
```env
DATABASE_URL=postgresql://postgres:[password]@[host]:5432/postgres
MQTT_HOST=[your-cluster-id].s1.eu.hivemq.cloud
MQTT_USER=[your-username]
MQTT_PASS=[your-password]
```

### 4. Running the Backend
```bash
cd backend
cargo run
```

### 5. Running the ESP32
- Copy the code from `wokwi-esp32/wokwi-esp32.ino` into a Wokwi project.
- Update the MQTT credentials in the `.ino` file to match your HiveMQ setup.
- Start the simulation.

## 📊 API Endpoints

-   `GET /health`: Checks if the backend is alive.
-   `GET /latest`: Returns the most recent telemetry record from the database.

## 📝 Database Schema

The `telemetry` table includes:
- `id`: UUID (Primary Key)
- `device_id`: MAC Address of the ESP32
- `recorded_at`: Time of measurement (UTC)
- `temperature`: Celsius (REAL)
- `humidity`: Percentage (REAL)
- `light_lux`: Calculated Lux (REAL)
- `status_led_temp`: Local actuator state (BOOLEAN)
- `inserted_at`: Server-side arrival time

---