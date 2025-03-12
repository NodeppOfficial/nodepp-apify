# NODEPP-EXPRESS
Real-Time API in C++ with Nodepp

## Build & Run
```bash
time; g++ -o main main.cpp -I ./include; ./main
```

## Usage

```cpp
#include <nodepp/nodepp.h>
#include <apify/apify.h>
#include <nodepp/ws.h>

using namespace nodepp;

void onMain() {

    auto app = apify::add<ws_t>();
    auto srv = ws::server();

    app.ADD("/:id",[=]( apify_t<ws_t> cli ){
        console::log( cli.params["id"] );
        cli.done();
    });

    app.ADD([=]( apify_t<ws_t> cli ){
        console::log( cli.message );
        cli.done();
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
        console::log( "<>", "ws:/localhost:8000" );
    }); console::log("Started");

}
```

## License

**Nodepp** is distributed under the MIT License. See the LICENSE file for more details.
