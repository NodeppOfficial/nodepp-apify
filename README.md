# NODEPP-APIFY
Real-Time API in C++ with Nodepp

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
