#[macro_use] extern crate rocket;
use rocket::futures::{SinkExt, StreamExt};
use ws::{frame::Frame, Message};

#[get("/")]
fn index() -> &'static str {
    "Hello, world!"
}

#[get("/connect")]
async fn connect(ws: ws::WebSocket) -> ws::Channel<'static> {
    ws.channel(move |mut stream| Box::pin(async move {
        while let Some(message) = stream.next().await {
            let message = message?;
            info!("payload: {}, payload (size): {}, binary: {}, text: {}", message, message.len(), message.is_binary(), message.is_text());

            let opcode;

             if message.is_text() {
                opcode = 1;
            } else {
                opcode = 2;
            }

            // let _ = stream.send(Message::Binary("hello, world!".into())).await;

            let _ = stream.send(Message::Frame(Frame::message(message.to_string()[0..message.len() / 2].into(), opcode.into(), false))).await;
            let _ = stream.send(Message::Frame(Frame::message(message.to_string()[message.len() / 2..message.len()].into(), opcode.into(), true))).await;

            // ok(());
        }
        
        let _ = stream.send(Message::Close(None)).await;
        
        Ok(())
    }))
}

#[launch]
fn rocket() -> _ {
    info!("Hello, world!");

    rocket::build()
        .mount("/", routes![index, connect])
}