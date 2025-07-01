#[macro_use] extern crate rocket;
use rocket::{tokio::net::TcpStream, tokio::io::{AsyncReadExt, AsyncWriteExt}, futures::{StreamExt, SinkExt}};
use ws::{Message, WebSocket};

#[get("/")]
fn index() -> &'static str {
    "Hello, world!"
}

#[get("/connect")]
async fn connect(ws: WebSocket) -> ws::Channel<'static> {
    ws.channel(move |mut websocket_stream| Box::pin(async move {
        let mut serverhost = TcpStream::connect("localhost:25565").await?;
        let mut buffer = vec![0; 1024];

        websocket_stream.send("hello, world!".into()).await?;

        loop {
            let serverhost_bytes = serverhost.read(&mut buffer);
            let serverhost_bytes = serverhost_bytes.await?;

            if serverhost_bytes > 0 {
                websocket_stream.send(Message::binary(&buffer[0..serverhost_bytes])).await?;
            } else {
                websocket_stream.close(None).await?
            }

            if let Some(websocket_message) = websocket_stream.next().await {
                let websocket_message = websocket_message?;

                if websocket_message.is_binary() {
                    serverhost.write(&websocket_message.into_data()).await?;
                } else if websocket_message.is_close() {
                    websocket_stream.close(None).await?;
                    // drop(serverhost);
                }
            } else {
                error!("No packet received from websocket");
            }
        }
        
        // Ok(())
    }))
}

#[launch]
fn rocket() -> _ {
    info!("Hello, world!");

    rocket::build()
        .mount("/", routes![index, connect])
}