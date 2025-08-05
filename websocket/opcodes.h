// https://en.wikipedia.org/wiki/WebSocket#Opcodes

enum opcodes {
    CONTINUATION = 0,
    TEXT = 1,
    BINARY = 2,
    // 3–7 Reserved...
    CLOSE = 8,
    PING = 9,
    PONG = 10,
    // 11–15 Reserved...
};