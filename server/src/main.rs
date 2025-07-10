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

        // websocket_stream.send("hello, world!".into()).await?;

        loop {
            info!("loop");
            rocket::tokio::select! {
                message = websocket_stream.next() => {
                    if let Some(message) = message {
                        let message = message?;
                        info!("GOT MESSAGE!");
                        if message.is_binary() {
                            serverhost.write_all(&Message::binary(message).into_data()).await?;
                        } else if message.is_close() {
                            websocket_stream.close(None).await?;
                            return Ok(());
                        }
                    } else {
                        error!("No packet received from websocket");
                    }
                },
                data_bytes = serverhost.read(&mut buffer) => {
                    let data_bytes = data_bytes?;
                    if data_bytes > 0 {
                        websocket_stream.send(Message::binary(&buffer[0..data_bytes])).await?;
                    } else {
                        info!("TCP/Unix stream closed");
                        websocket_stream.close(None).await?;
                    }
                }
            }
        }
    }))
}

#[launch]
fn rocket() -> _ {
    info!("Hello, world!");

    rocket::build()
        .mount("/", routes![index, connect])
}