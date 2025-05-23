#[macro_use] extern crate rocket;
use rocket::futures::SinkExt;
use ws::WebSocket;

#[get("/")]
fn index() -> &'static str {
    "Hello, world!"
}

#[get("/connect")]
async fn connect(ws: WebSocket) -> ws::Channel<'static> {
    ws.channel(move |mut stream| Box::pin(async move {
        loop {
            let _ = stream.send("hello, world!".into()).await;

            // while let Some(message) = stream.next().await {
            //     let message = message?;

            //     if message.is_empty() || message.is_close() {
            //         break;
            //     }
            // }
        }
        
        Ok(())
    }))
}

#[launch]
fn rocket() -> _ {
    info!("Hello, world!");

    rocket::build()
        .mount("/", routes![index, connect])
}