use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize)]
pub struct TelemetryPayload {
    pub device_id: String,
    pub timestamp: i64, // Unix epoch from ESP32
    pub temperature: f32,
    pub humidity: f32,
    pub light_level: f32,
    pub light_lux: f32,
    pub status_led_temp: bool,
}
