use axum::{routing::get, Json, Router};
use serde::Serialize;
use std::net::SocketAddr;

#[derive(Serialize)]
struct HelloResponse {
    message: String,
}

async fn hello_world() -> Json<HelloResponse> {
    Json(HelloResponse {
        message: "Hello World!".to_string(),
    })
}

#[tokio::main]
async fn main() {
    let app = Router::new().route("/", get(hello_world));

    let addr = SocketAddr::from(([127, 0, 0, 1], 3000));
    println!("Server running at http://{}", addr);

    axum::serve(tokio::net::TcpListener::bind(addr).await.unwrap(), app.into_make_service())
        .await
        .unwrap();
}
