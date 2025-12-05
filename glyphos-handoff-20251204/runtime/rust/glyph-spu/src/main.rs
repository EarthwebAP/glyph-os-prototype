//! glyph-spu - Glyph SPU Offload Service
//! Provides hardware-accelerated merge operations.

use axum::{
    extract::State,
    http::StatusCode,
    response::Json,
    routing::{get, post},
    Router,
};
use serde::{Deserialize, Serialize};
use std::sync::Arc;
use tracing::info;

#[derive(Clone, Debug, Serialize, Deserialize)]
struct Glyph {
    id: String,
    content: String,
    energy: f64,
    activation_count: u32,
    last_update_time: u64,
}

#[derive(Deserialize)]
struct MergeRequest {
    glyph1: Glyph,
    glyph2: Glyph,
}

#[derive(Serialize)]
struct MergeResponse {
    merged_state: Glyph,
    parent1_id: String,
    parent2_id: String,
}

#[derive(Clone)]
struct AppState {}

/// Merge two glyphs (software reference implementation)
fn merge_glyphs(g1: &Glyph, g2: &Glyph) -> Glyph {
    use std::time::{SystemTime, UNIX_EPOCH};

    // Determine precedence
    let (primary, secondary) = if g1.energy >= g2.energy {
        (g1, g2)
    } else {
        (g2, g1)
    };

    // Concatenate content
    let merged_content = format!("{} + {}", primary.content, secondary.content);

    // Generate ID (simplified)
    let timestamp = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap()
        .as_nanos();
    let merged_id = format!("{:064x}", timestamp);

    // Sum energy
    let merged_energy = primary.energy + secondary.energy;

    // Merge metadata (max operations)
    let activation_count = primary.activation_count.max(secondary.activation_count);
    let last_update_time = primary.last_update_time.max(secondary.last_update_time);

    Glyph {
        id: merged_id,
        content: merged_content,
        energy: merged_energy,
        activation_count,
        last_update_time,
    }
}

async fn offload_merge(
    State(_state): State<AppState>,
    Json(req): Json<MergeRequest>,
) -> Result<Json<MergeResponse>, StatusCode> {
    info!("Processing merge: {} + {}", req.glyph1.id, req.glyph2.id);

    let merged_state = merge_glyphs(&req.glyph1, &req.glyph2);

    let response = MergeResponse {
        merged_state,
        parent1_id: req.glyph1.id.clone(),
        parent2_id: req.glyph2.id.clone(),
    };

    Ok(Json(response))
}

async fn offload_status() -> Json<serde_json::Value> {
    Json(serde_json::json!({
        "service": "glyph-spu",
        "status": "ready",
        "accelerator": "software_reference",
        "version": "0.1.0"
    }))
}

async fn health() -> &'static str {
    "glyph-spu OK"
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    tracing_subscriber::fmt::init();
    info!("Starting glyph-spu v0.1.0");
    info!("SPU offload service (software reference)");

    let state = AppState {};

    let app = Router::new()
        .route("/health", get(health))
        .route("/offload/merge", post(offload_merge))
        .route("/offload/status", get(offload_status))
        .with_state(state);

    let listener = tokio::net::TcpListener::bind("0.0.0.0:8081").await?;
    info!("Listening on 0.0.0.0:8081");
    axum::serve(listener, app).await?;
    Ok(())
}
