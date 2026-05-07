-- Enable the UUID extension
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- Create the main telemetry table
CREATE TABLE IF NOT EXISTS telemetry (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    device_id VARCHAR(50) NOT NULL,
    recorded_at TIMESTAMPTZ NOT NULL, 
    temperature REAL,
    humidity REAL,
    light_level REAL,
    light_lux REAL,
    status_led_temp BOOLEAN,
    inserted_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Index for per-device history
CREATE INDEX IF NOT EXISTS idx_telemetry_device_time ON telemetry (device_id, recorded_at DESC);
-- Index for global "latest" queries
CREATE INDEX IF NOT EXISTS idx_telemetry_recorded_at ON telemetry (recorded_at DESC);

-- RLS Configuration
ALTER TABLE telemetry ENABLE ROW LEVEL SECURITY;

-- Allow reading
CREATE POLICY "Allow public read-only access" 
ON telemetry FOR SELECT 
USING (true);

-- Allow backend to insert (required if RLS is enabled and backend isn't the table owner)
CREATE POLICY "Allow all inserts" 
ON telemetry FOR INSERT 
WITH CHECK (true);