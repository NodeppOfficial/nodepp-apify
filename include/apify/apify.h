/*
 * Copyright 2023 The Nodepp Project Authors. All Rights Reserved.
 *
 * Licensed under the MIT (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://github.com/NodeppOfficial/nodepp/blob/main/LICENSE
 */

/*────────────────────────────────────────────────────────────────────────────*/

#ifndef NODEPP_APIFY
#define NODEPP_APIFY

/*────────────────────────────────────────────────────────────────────────────*/

#define MIDDL function_t<void,apify_t<T>&,function_t<void>>
#define CALBK function_t<void,apify_t<T>&>
#define ROUTR apify_host_t
#define APIFY apify_t<T>

/*────────────────────────────────────────────────────────────────────────────*/

#include <nodepp/optional.h>
#include <nodepp/encoder.h>
#include <nodepp/path.h>
#include <nodepp/json.h>
#include <nodepp/url.h>
#include <nodepp/fs.h>

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { template< class T > class apify_t { public:

    /*.......................................................................*/

    struct NODE { bool state=0; T ctx; }; ptr_t<NODE> obj;
    string_t message; string_t method; string_t path; query_t params;

    /*.......................................................................*/

    int emit( string_t method, string_t path, string_t data ) const noexcept { done(); return send(method,path,data); }
    int emit( string_t path, string_t data ) /*------------*/ const noexcept { return emit( nullptr, path   , data ); }
    int emit( string_t data ) /*---------------------------*/ const noexcept { return emit( nullptr, nullptr, data ); }

    /*.......................................................................*/

    string_t format( string_t method, string_t path, string_t data ) const noexcept {
    return regex::format( ".${0}.${1}.${2}.",
        !method.empty() ? encoder::base64::atob(method) : "",
        !path  .empty() ? encoder::base64::atob(path)   : "",
        !data  .empty() ? encoder::base64::atob(data)   : ""
    );}

    /*.......................................................................*/

    int send( string_t method, string_t path, string_t data ) const noexcept {
        return obj->ctx.write( format( method, path, data ) );
    }

    /*.......................................................................*/

    bool is_available()  const noexcept { return obj->ctx.is_available(); }
    bool is_closed()     const noexcept { return obj->ctx.is_closed(); }
    bool is_done()       const noexcept { return obj->state == 1; }

    /*.......................................................................*/

    void close()         const noexcept { obj->ctx.close(); }

    /*.......................................................................*/

    T& operator->()      const noexcept { return obj->ctx; }
    T&     get_fd()      const noexcept { return obj->ctx; }
    T& get_socket()      const noexcept { return get_fd(); }
    void set_fd( T& fd ) const noexcept { obj->ctx  =fd; }
    void done()          const noexcept { obj->state= 1; }

    /*.......................................................................*/

    apify_t( T fd ) : obj( new NODE() ) { set_fd(fd); }
    apify_t()       : obj( new NODE() ) {}

};}

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { template< class T > class apify_host_t {
protected:

    struct apify_item_t {
        optional_t<MIDDL> middleware;
        optional_t<CALBK> callback;
        optional_t<ROUTR> router;
        string_t /*----*/ method;
        string_t /*----*/ path;
    };

    /*.......................................................................*/

    struct NODE {
         queue_t<apify_item_t> list;
         queue_t<apify_item_t> mddl;
         string_t path = nullptr;
    };   ptr_t<NODE> obj;

    /*.......................................................................*/

    void execute( string_t path, apify_item_t& data, APIFY& cli, function_t<void>& next ) const noexcept {
        if  ( data.middleware.has_value() ){ data.middleware.value()( cli, next ); return; }
        elif( data.callback  .has_value() ){ data.callback  .value() /*----*/ ( cli ); }
        elif( data.router    .has_value() ){ data.router    .value().run( path, cli ); }
    next(); }

    /*.......................................................................*/

    void insert( string_t path, const apify_item_t& item ) const noexcept {
        auto x= obj->list.first(); while( x != nullptr ){
        if( path.size() > x->data.path.size() ){ break; }
        x = x->next; } obj->list.insert( x, item );
    }

    /*.......................................................................*/

    string_t format( string_t method, string_t path, string_t data ) const noexcept {
    return regex::format( ".${0}.${1}.${2}.",
        !method.empty() ? encoder::base64::atob(method) : "",
        !path  .empty() ? encoder::base64::atob(path)   : "",
        !data  .empty() ? encoder::base64::atob(data)   : ""
    );}

    /*.......................................................................*/

    int path_match( APIFY& cli, string_t base, string_t path ) const noexcept {
        
        string_t pathname = normalize( base, path );

        if  ( regex::test( cli.path, "^"+pathname ) ){ return 1; }
        
            array_t<string_t> _path[2]= { 
            string::split(cli.path,'/'), 
            string::split(pathname,'/') };

        for ( int x=2; x-->0; ){
        if  ( _path[x][_path[x].first()].empty() ){ _path[x].shift(); }
        if  ( _path[x][_path[x].last ()].empty() ){ _path[x].pop  (); }}

        if  ( _path[0].empty()&& _path[1].empty()){ return  1; }
    //  if  ( _path[0].empty()|| _path[1].empty()){ return -1; }
    //  if  ( _path[0][0]     != _path[1][0]     ){ return -1; }
        if  ( _path[0].size() <  _path[1].size() ){ return  0; }

        for ( ulong x=0; x<_path[1].size(); ++x ){
        if  ( _path[1][x] ==nullptr ){ continue; }
        elif( _path[1][x][0] == ':' ){ if( _path[0][x].empty() )/**/{ return 0; }
              cli.params[_path[1][x].slice(1)] = url::normalize( _path[0][x] ); }
        elif( _path[1][x].empty() /*--*/ ){ continue; }
        elif( _path[1][x] != _path[0][x] ){ return 0; }}

    return 1; }

    /*.......................................................................*/

    void run( string_t path, APIFY& cli ) const noexcept { do {

        auto     n = obj->mddl.first();
        auto _base = normalize( path, obj->path );
        function_t<void> next = [&](){ n = n->next; };

        while( n!=nullptr && !cli.is_done() ) 
             { execute( _base, n->data, cli, next ); }

    } while(0); if( cli.is_done() ){ return; } do {

        auto     n = obj->list.first();
        auto _base = normalize( path, obj->path );
        function_t<void> next = [&](){ n = n->next; };

        while ( n!=nullptr && !cli.is_done() ){ int c=0;
            if(( n->data.path.empty() && obj->path.empty() )
            || ((c=path_match(cli,_base,n->data.path))==1 )){
            if ( n->data.method.empty() 
            || ( n->data.method==cli.method ) )
               { execute( _base, n->data, cli, next ); continue; }}
        if(c==-1) { break; } next(); }

    } while(0); }

    /*.......................................................................*/

    string_t normalize( string_t base, string_t path ) const noexcept {
        auto new_path =  base.empty() ? ("/"+path) : path.empty() ? /*------*/
        /*---------------------------*/ ("/"+base) : path::join( base, path );
        return path::normalize( new_path );
    }

public:

    apify_host_t() noexcept : obj( new NODE() ) {}

    /*.........................................................................*/

    void     set_path( string_t path ) const noexcept { obj->path = path; }
    string_t get_path() /*----------*/ const noexcept { return obj->path; }

    /*.........................................................................*/

    const apify_host_t& on( string_t _method, string_t _path, CALBK cb ) const noexcept {
        apify_item_t item; // memset( (void*) &item, 0, sizeof(item) );
        item.path   = _path.empty() ? nullptr : _path;
        item.method = _method; item.callback = cb;
        insert( _path, item ); return (*this);
    }

    const apify_host_t& on( string_t _path, CALBK cb ) const noexcept {
        return on( nullptr, _path, cb );
    }

    const apify_host_t& on( CALBK cb ) const noexcept {
        return on( nullptr, nullptr, cb );
    }

    /*.........................................................................*/

    void emit( T cli, string_t method, string_t path, string_t data ) const noexcept {
         next( cli, format( method, path, data ) );
    }

    /*.........................................................................*/

    const apify_host_t& add( string_t _path, apify_host_t cb ) const noexcept {
        apify_item_t item; // memset( (void*) &item, 0, sizeof(item) );
        cb.set_path( normalize( obj->path, _path ) );
        item.router = optional_t<ROUTR>(cb);
        item.method = nullptr;
        item.path   = nullptr;
        insert( _path, item ); return (*this);
    }

    const apify_host_t& add( apify_host_t cb ) const noexcept {
        return add( nullptr, cb );
    }

    /*.........................................................................*/

    const apify_host_t& add( string_t _path, MIDDL cb ) const noexcept {
        apify_item_t item; // memset( (void*) &item, 0, sizeof(item) );
        item.path       = _path.empty() ? nullptr : _path;
        item.middleware = optional_t<MIDDL>(cb);
        item.method     = nullptr;
        if( _path.empty() ){ obj->mddl.push( item ); }
        else /*---------*/ { insert ( _path, item ); }
    return (*this); }

    const apify_host_t& add( MIDDL cb ) const noexcept {
        return add( nullptr, cb );
    }

    /*.........................................................................*/

    void next( T cli, string_t message ) const noexcept {
    
        queue_t<ulong> raw; auto app=APIFY(cli); 

        for( ulong x=0; x<message.size(); x++ ){
        if ( message[x] != '.' ){ continue; } raw.push( x ); }

        auto idx = array_t<ulong>( raw.data() );

        while( !idx.empty()  ){ auto tmp = idx.splice( 0, 4 );
        if   (  tmp.size()<4 ){ break; }
            
            app.method  = encoder::base64::btoa( message.slice_view( tmp[0]+1, tmp[1] ) );
            app.path    = encoder::base64::btoa( message.slice_view( tmp[1]+1, tmp[2] ) );
            app.message = encoder::base64::btoa( message.slice_view( tmp[2]+1, tmp[3] ) );

            run( nullptr, app );

        }

    }

};}

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { namespace apify {
    template< class T > apify_host_t<T> add()          { return apify_host_t<T>(); }
    template< class T > apify_t<T> add( const T& cli ) { return apify_t<T>( cli ); }
    template< class T > apify_t<T> get( const T& cli ) { return apify_t<T>( cli ); }
}}

/*────────────────────────────────────────────────────────────────────────────*/

#undef MIDDL
#undef CALBK
#undef ROUTR
#undef APIFY
#endif

/*────────────────────────────────────────────────────────────────────────────*/