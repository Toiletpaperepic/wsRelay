#ifndef WEBSOCKET_STATUS_CODES_H
#define WEBSOCKET_STATUS_CODES_H

enum status_code {
    // non standard 
    none = 0,

    // 0–999, unused...
    normal_closure = 1000,
    going_away = 1001,
    protocol_error = 1002,
    unsupported_data = 1003,
    // 1004, Reserved
    no_code_received = 1005,
    connection_closed_abnormally = 1006,
    invalid_payload_data = 1007, 
    policy_violated = 1008,
    message_too_big = 1009,
    unsupported_extension = 1010,
    internal_server_error = 1011,
    // 10012–1014, unused?
    TLS_handshake_failure = 1015,
    // 3000–3999, Reserved...
    // 4000–4999, Private use...
};
#endif
