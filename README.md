# Nodepp APIfy

High-Performance, Real-Time API and Structured Messaging in C++

APIfy is a protocol-agnostic routing framework for C++ applications within the Nodepp ecosystem. It injects the clarity, scalability, and expressive syntax of modern web routing frameworks (like Express.js) into any bidirectional stream—be it WebSockets, TCP, or TLS.

Forget manual message parsing, payload decoding, and brittle conditional logic. APIfy shifts your focus from handling raw bytes to defining high-level, addressable API endpoints immediately.

🔗: [Mastering APIfy: A Routing Protocol for Structured C++ Messaging](https://medium.com/p/400ac5e023d6)

## Dependencies & Cmake Integration
```bash
# Openssl
    🪟: pacman -S mingw-w64-ucrt-x86_64-openssl
    🐧: sudo apt install libssl-dev
# Zlib
    🪟: pacman -S mingw-w64-ucrt-x86_64-zlib
    🐧: sudo apt install zlib1g-dev
```
```bash
include(FetchContent)

FetchContent_Declare(
	nodepp
	GIT_REPOSITORY   https://github.com/NodeppOfficial/nodepp
	GIT_TAG          origin/main
	GIT_PROGRESS     ON
)
FetchContent_MakeAvailable(nodepp)

FetchContent_Declare(
	nodepp-apify
	GIT_REPOSITORY   https://github.com/NodeppOfficial/nodepp-apify
	GIT_TAG          origin/main
	GIT_PROGRESS     ON
)
FetchContent_MakeAvailable(nodepp-apify)

#[...]

target_link_libraries( #[...]
	PUBLIC nodepp nodepp-apify #[...]
)
```

## Key Features
- **Protocol-Agnostic:** Runs seamlessly over any established, full-duplex connection (WebSockets, TCP, TLS).
- **Structured Messaging:** Enforces a simple, self-contained message format: `Base64(METHOD).Base64(PATH).Base64(MESSAGE)`
- **Expressive Routing:** Uses a familiar syntax to define endpoints, including dynamic route parameters `(e.g., app.on("GET", "/users/:id", handler))`.
- **Dynamic Parameters:** Automatically extracts route parameters `(like :id)` into the client context `(cli.params)`.
- **Middleware and Nested Routing:** Native support for middleware chains `(for auth, logging, etc.)` and API modularization via nested routers.
- **Decoupled Architecture:** Employs an event/promise-based model that allows true bidirectional communication and low-latency multiplexing over a single socket connection.

## Usage
APIfy centers on two core classes: the Router Host `(apify_host_t)` which defines the routes, and the Client Context `(apify_t)` which carries the message and enables responses.

### Server Side (Defining Routes)
On the server, you use `apify_host_t` to register routes against the underlying connection type (here using `ws_t` for WebSockets):

```cpp
#include <nodepp/nodepp.h>
#include <apify/apify.h>
#include <nodepp/ws.h>

using namespace nodepp;

void onMain() {
    // 1. Initialize the APIfy Router for WebSocket connections
    auto app = apify::add<ws_t>();
    auto srv = ws::server();

    // 2. Specific Handler Definition
    // Only matches if METHOD="AUTH" and PATH="/login"
    app.on("AUTH", "/login", [=]( apify_t<ws_t> cli ) {
        console::log("-> Login message received:", cli.message );
        // Respond to the client with a "SUCCESS" method
        cli.emit("SUCCESS", nullptr, "Authentication successful.");
    });

    // 3. Dynamic Parameter Handler
    app.on("GET", "/user/:id", [=]( apify_t<ws_t> cli ) {
        // 'id' is automatically extracted from the path into cli.params
        string_t user_id = cli.params["id"]; 
        console::log("-> Request for user:", user_id );
        cli.emit("USER_DATA", nullptr, json::stringify({ {"id", user_id} }));
    });

    // 4. Fallback/Catch-All Handler
    // Catches any message that didn't match the specific routes above
    app.on(nullptr, nullptr, [=]( apify_t<ws_t> cli ) {
        console::warn("-> Unmatched Route:", cli.method, cli.path );
        cli.emit("ERROR", nullptr, "Route or Method not supported.");
    });

    // 5. Connect the Router to the Data Stream
    srv.onConnect([=]( ws_t cli ) {
        // Pass raw incoming data directly to the APIfy router
        cli.onData([=]( string_t data ) {
            app.next( cli, data );
        });
        console::log("Client Connected.");
    });

    srv.listen("localhost", 8000, [=](...){ 
        console::log("WebSocket Server started at ws:/localhost:8000");
    });
}
```

### Client Side (Sending & Receiving)
Any module sending data over the stream uses the `apify_t` object to format and emit the structured message.

```cpp
#include <nodepp/nodepp.h>
#include <apify/apify.h>
#include <nodepp/ws.h>

using namespace nodepp;

void onMain() {
    auto client = ws::client();

    client.onConnect([=]( ws_t cli ) {
        // 1. Get the APIfy context for the client socket
        auto app_cli = apify::add( cli ); 

        // 2. Register Response Handlers (Listeners)
        // Client listens for a "SUCCESS" method response from the server
        app_cli.on("SUCCESS", nullptr, [=]( apify_t<ws_t> res ) {
            console::done("Login Successful:", res.message );
        });
        
        // Client listens for an "ERROR" response
        app_cli.on("ERROR", nullptr, [=]( apify_t<ws_t> res ) {
            console::error("Error:", res.message );
        });

        // 3. Send a Message (Request)
        // Send a message with METHOD="AUTH", PATH="/login" and payload
        app_cli.emit("AUTH", "/login", "user:pass");
        
        // Send a GET request for a specific user ID
        app_cli.emit("GET", "/user/42", "fetch");

        // Note: The client must also process incoming server messages 
        // through app_cli.next() if it expects to route structured events 
        // from the server to its own internal listeners.
    });

    client.connect("localhost", 8000);
}
```

## Middleware and Nested Routing
APIfy fully supports the middleware pattern (functions that execute before the final handler and must call next()) and modular routing.

### Middleware
Middleware is added using the add() method and is perfect for cross-cutting concerns like authentication or logging.
```cpp
// Logging Middleware executes before any subsequent route logic
auto logger_middleware = [=]( apify_t<ws_t> cli, function_t<void> next ) {
    console::log("[LOG]", cli.method, cli.path);
    next(); // CRITICAL: Call next() to pass control to the next handler/route
};

app.add( logger_middleware );

// The logger runs before this handler:
app.on("GET", "/status", [=]( apify_t<ws_t> cli ) {
    cli.emit("STATUS", nullptr, "OK");
});
```

### Nested Routing
Modularize your API by creating dedicated routers (e.g., for /admin endpoints).
```cpp
// 1. Create a dedicated Router for Admin logic
auto admin_router = apify::add<ws_t>();

// Internal routes for the admin router
admin_router.on("POST", "/create", [=]( apify_t<ws_t> cli ) {
    cli.emit("SUCCESS", nullptr, "Admin: User created.");
});

// 2. Mount the Admin Router onto the Main Router
// All routes in 'admin_router' will be prefixed with '/admin'
app.add("/admin", admin_router); 

// A request with PATH="/admin/create" is now handled by 'admin_router'.
```

## Contributing
Contributions are welcome! If you'd like to improve APIfy, report a bug, or add a new feature, please refer to the Contributing Guidelines or open a Pull Request.

## License
**Nodepp-apify** is distributed under the MIT License. See the LICENSE file for more details.