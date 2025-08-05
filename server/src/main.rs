#[macro_use] extern crate rocket;
use rocket::futures::SinkExt;
use ws::Message;

#[get("/")]
fn index() -> &'static str {
    "Hello, world!"
}

// #[get("/connect")]
// async fn connect(ws: WebSocket) -> ws::Channel<'static> {
//     ws.channel(move |mut websocket_stream| Box::pin(async move {
//         let mut serverhost = TcpStream::connect("localhost:25565").await?;
//         let mut buffer = vec![0; 1024];

//         loop {
//             info!("loop");
//             rocket::tokio::select! {
//                 message = websocket_stream.next() => {
//                     if let Some(message) = message {
//                         let message = message?;
//                         info!("GOT MESSAGE!");
//                         if message.is_binary() {
//                             serverhost.write_all(&Message::binary(message).into_data()).await?;
//                         } else if message.is_close() {
//                             websocket_stream.close(None).await?;
//                             return Ok(());
//                         }
//                     } else {
//                         error!("No packet received from websocket");
//                     }
//                 },
//                 data_bytes = serverhost.read(&mut buffer) => {
//                     let data_bytes = data_bytes?;
//                     if data_bytes > 0 {
//                         websocket_stream.send(Message::binary(&buffer[0..data_bytes])).await?;
//                     } else {
//                         info!("TCP/Unix stream closed");
//                         websocket_stream.close(None).await?;
//                     }
//                 }
//             }
//         }
//     }))
// }

#[get("/connect")]
async fn connect(ws: ws::WebSocket) -> ws::Channel<'static> {
    ws.channel(move |mut stream| Box::pin(async move {
        for _ in 0..1000 {
            let message = format!("{} hello world {}\0", rand::random::<u64>(), rand::random::<u64>());
            info!("message: {}, message length: {}", message, message.len());

            let _ = stream.send(Message::Text(message)).await;
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