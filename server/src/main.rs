#[macro_use] extern crate rocket;
use rocket::futures::{SinkExt, StreamExt};
use ws::Message;

#[get("/")]
fn index() -> &'static str {
    "Hello, world!"
}

#[get("/connect")]
async fn connect(ws: ws::WebSocket) -> ws::Channel<'static> {
    ws.channel(move |mut stream| Box::pin(async move {
        while let Some(message) = stream.next().await {
            let message = message?;
            info!("payload: {}, payload (size): {}", message, message.len());
            let _ = stream.send(message);
        }
        
        let _ = stream.send(Message::Close(None));
        
        Ok(())
    }))
}

#[launch]
fn rocket() -> _ {
    info!("Hello, world!");

    rocket::build()
        .mount("/", routes![index, connect])
}