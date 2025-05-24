# NODEPP-APIFY
To develop a high-performance, real-time API in C++ leveraging the [NodePP](https://github.com/NodeppOfficial/nodepp) library. This API will offer a development experience similar to Socket.IO but within the C++ ecosystem.

# Features
- **Real-time Bidirectional Communication:** Enable persistent, low-latency communication between clients and the server, allowing for instant data exchange.
- **Event-Driven Architecture:** Design the API around the concept of emitting and handling events, providing a flexible and reactive programming model.
- **Asynchronous I/O:** Utilize an asynchronous I/O model to handle concurrent connections efficiently without blocking, ensuring scalability and responsiveness.
- **C++ Performance:** Capitalize on the inherent performance advantages of C++ for demanding real-time applications.

## Build & Run
```bash
time; g++ -o main main.cpp -I ./include; ./main
```

## Usage
### Server

```cpp
#include <nodepp/nodepp.h>
#include <apify/apify.h>
#include <nodepp/ws.h>

using namespace nodepp;

void onMain() {
    auto app = apify::add<ws_t>();
    auto srv = ws::server();

    app.on("METHOD","/PATH",[=]( apify_t<ws_t> cli ){
        console::log( cli.message );
        cli.emit( "DONE", "/PATH", "done" );
    });

    srv.onConnect([=]( ws_t cli ){

        cli.onData([=]( string_t data ){
            app.next( cli, data );
        });

        cli.onClose([=](){
            console::log("Disconnected");
        }); console::log("Connected");

    });

    srv.listen("localhost",8000,[=](...){
        console::log( "ws:/localhost:8000" );
    }); console::log( "Started" );

}
```

### Client
```cpp
#include <nodepp/nodepp.h>
#include <apify/apify.h>
#include <nodepp/ws.h>

using namespace nodepp;

void onMain(){
    auto srv = ws::client( "ws://localhost:8000" );
    auto app = apify::add<ws_t>();

    app.on("DONE","/PATH",[=]( apify_t<ws_t> cli ){
        console::log( cli.message );
    });

    srv.onConnect([=]( ws_t cli ){

        cli.onData([=]( string_t data ){
            app.next( cli, data );
        });

        srv.onClose([=](){
            console::log("Disconnected");
        }); console::log("Connected");

        apify::add(cli).emit("METHOD","/PATH","hello world!");

    });

}
```

## License

**Nodepp** is distributed under the MIT License. See the LICENSE file for more details.
