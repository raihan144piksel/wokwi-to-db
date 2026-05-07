use axum::{extract::State, routing::get, Json, Router};
use dotenvy::dotenv;
use rumqttc::{AsyncClient, MqttOptions, QoS, Transport};
use sqlx::postgres::PgPoolOptions;
use sqlx::PgPool;
use std::time::Duration;
mod models;
use models::TelemetryPayload;

#[tokio::main]
async fn main() {
    rustls::crypto::aws_lc_rs::default_provider()
        .install_default()
        .expect("Failed to install crypto provider");

    dotenv().ok();
    let database_url = std::env::var("DATABASE_URL").expect("DATABASE_URL must be set");
    let mqtt_host = std::env::var("MQTT_HOST").expect("MQTT_HOST must be set");
    let mqtt_user = std::env::var("MQTT_USER").expect("MQTT_USER must be set");
    let mqtt_pass = std::env::var("MQTT_PASS").expect("MQTT_PASS must be set");

    // 1. Setup Database Pool (Supabase)
    let pool = PgPoolOptions::new()
        .max_connections(5)
        .acquire_timeout(Duration::from_secs(3))
        .connect(&database_url)
        .await
        .expect("Failed to connect to Supabase");

    // 2. Setup MQTT Client (HiveMQ Cloud TLS)
    let mut mqtt_options = MqttOptions::new("rust_backend_subscriber", mqtt_host, 8883);
    mqtt_options.set_keep_alive(Duration::from_secs(5));
    mqtt_options.set_credentials(mqtt_user, mqtt_pass);

    // HiveMQ Cloud requires TLS
    let mut root_cert_store = rustls::RootCertStore::empty();
    root_cert_store.extend(webpki_roots::TLS_SERVER_ROOTS.iter().cloned());

    let tls_config = rustls::ClientConfig::builder()
        .with_root_certificates(root_cert_store)
        .with_no_client_auth();

    mqtt_options.set_transport(Transport::tls_with_config(tls_config.into()));

    let (client, mut eventloop) = AsyncClient::new(mqtt_options, 10);
    client
        .subscribe("wokwi/esp32/telemetry", QoS::AtMostOnce)
        .await
        .unwrap();

    // 3. Spawn MQTT Background Task
    let pool_clone = pool.clone();
    tokio::spawn(async move {
        loop {
            // Listen for notifications from the broker
            match eventloop.poll().await {
                Ok(notification) => {
                    if let rumqttc::Event::Incoming(rumqttc::Packet::Publish(publish)) =
                        notification
                    {
                        let pool = pool_clone.clone();
                        tokio::spawn(async move {
                            if let Ok(payload) =
                                serde_json::from_slice::<TelemetryPayload>(&publish.payload)
                            {
                                save_to_db(&pool, payload).await;
                            }
                        });
                    }
                }
                Err(e) => {
                    println!("MQTT Error: {:?}", e);
                    tokio::time::sleep(Duration::from_secs(5)).await;
                }
            }
        }
    });

    // 4. Setup Axum Server
    let app = Router::new()
        .route("/health", get(|| async { "Backend is running!" }))
        .route("/latest", get(get_latest_data))
        .with_state(pool);

    let listener = tokio::net::TcpListener::bind("0.0.0.0:3000").await.unwrap();
    println!("Axum server running on http://localhost:3000");
    axum::serve(listener, app).await.unwrap();
}

async fn save_to_db(pool: &PgPool, data: TelemetryPayload) {
    let res = sqlx::query!(
        r#"
        INSERT INTO telemetry (device_id, recorded_at, temperature, humidity, light_level, light_lux, status_led_temp)
        VALUES ($1, to_timestamp($2), $3, $4, $5, $6, $7)
        "#,
        data.device_id,
        data.timestamp as f64,
        data.temperature,
        data.humidity,
        data.light_level,
        data.light_lux,
        data.status_led_temp
    )
    .execute(pool)
    .await;

    match res {
        Ok(_) => println!("Stored data from device: {}", data.device_id),
        Err(e) => eprintln!("Database error: {}", e),
    }
}

async fn get_latest_data(State(pool): State<PgPool>) -> Json<serde_json::Value> {
    let row = sqlx::query!("SELECT * FROM telemetry ORDER BY recorded_at DESC LIMIT 1")
        .fetch_one(&pool)
        .await;

    match row {
        Ok(r) => Json(serde_json::json!({ "status": "success", "data": format!("{:?}", r) })),
        Err(_) => Json(serde_json::json!({ "status": "error", "message": "No data found" })),
    }
}
