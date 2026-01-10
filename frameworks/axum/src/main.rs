mod server;

use axum::{routing::get, Json, Router};
use serde::Serialize;

#[derive(Serialize)]
struct HelloResponse {
    message: String,
}

async fn hello_world() -> Json<HelloResponse> {
    Json(HelloResponse {
        message: "Hello World!".to_string(),
    })
}

fn main() {
    server::start_tokio(serve_app);
}

async fn serve_app() {
    let app = Router::new().route("/", get(hello_world));
    
    server::serve_hyper(app, Some(3000)).await;
}
