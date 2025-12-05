//! glyphd - Glyph OS Node Daemon
//! Main service for persisting and querying glyphs.

use axum::{
    extract::{Path, State},
    http::StatusCode,
    response::Json,
    routing::{get, post},
    Router,
};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::sync::{Arc, RwLock};
use tracing::{info, warn};

#[derive(Clone, Debug, Serialize, Deserialize)]
struct Glyph {
    id: String,
    content: String,
    metadata: serde_json::Value,
    commit_id: Option<String>,
}

type GlyphStore = Arc<RwLock<HashMap<String, Glyph>>>;

#[derive(Clone)]
struct AppState {
    store: GlyphStore,
}

#[derive(Deserialize)]
struct CreateGlyphRequest {
    content: String,
    metadata: Option<serde_json::Value>,
}

#[derive(Serialize)]
struct CreateGlyphResponse {
    id: String,
    commit_id: String,
}

async fn create_glyph(
    State(state): State<AppState>,
    Json(req): Json<CreateGlyphRequest>,
) -> Result<Json<CreateGlyphResponse>, StatusCode> {
    use std::time::{SystemTime, UNIX_EPOCH};

    let timestamp = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap()
        .as_nanos();
    let id = format!("{:064x}", timestamp);
    let commit_id = format!("commit_{:016x}", timestamp);

    let glyph = Glyph {
        id: id.clone(),
        content: req.content,
        metadata: req.metadata.unwrap_or(serde_json::json!({})),
        commit_id: Some(commit_id.clone()),
    };

    state.store.write().unwrap().insert(id.clone(), glyph);
    info!("Created glyph: {}", id);

    Ok(Json(CreateGlyphResponse { id, commit_id }))
}

async fn query_glyph(
    State(state): State<AppState>,
    Path(id): Path<String>,
) -> Result<Json<Glyph>, StatusCode> {
    let store = state.store.read().unwrap();
    match store.get(&id) {
        Some(glyph) => Ok(Json(glyph.clone())),
        None => Err(StatusCode::NOT_FOUND),
    }
}

async fn health() -> &'static str {
    "glyphd OK"
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    tracing_subscriber::fmt::init();
    info!("Starting glyphd v0.1.0");

    let state = AppState {
        store: Arc::new(RwLock::new(HashMap::new())),
    };

    let app = Router::new()
        .route("/health", get(health))
        .route("/glyphs", post(create_glyph))
        .route("/glyphs/:id", get(query_glyph))
        .with_state(state);

    let listener = tokio::net::TcpListener::bind("0.0.0.0:8080").await?;
    axum::serve(listener, app).await?;
    Ok(())
}
