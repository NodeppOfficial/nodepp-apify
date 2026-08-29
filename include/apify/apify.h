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

    T* operator->()      const noexcept { return &obj->ctx; }
    T&     get_fd()      const noexcept { return  obj->ctx; }
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
        string_t method, path;
        int /*--------*/ mode;
        any_t /*------*/ data;
    };

    /*.......................................................................*/

    struct NODE {
         queue_t<apify_item_t> list;
         queue_t<apify_item_t> mddl;
         string_t path = nullptr;
    };   ptr_t<NODE> obj;

    /*.......................................................................*/

    void execute( const string_t& path, apify_item_t& data, APIFY& cli, function_t<void>& next ) const noexcept {
        if  ( data.data.has_value() && data.mode==0 ){ data.data.template as<MIDDL>()( cli, next ); return; }
        elif( data.data.has_value() && data.mode==1 ){ data.data.template as<CALBK>() /*----*/ ( cli ); }
        elif( data.data.has_value() && data.mode==2 ){ data.data.template as<ROUTR>().run( path, cli ); }
    next(); }

    /*.......................................................................*/

    string_t format( const string_t& method, const string_t& path, const string_t& data ) const noexcept {
    return regex::format( ".${0}.${1}.${2}.",
        !method.empty() ? encoder::base64::atob(method) : "",
        !path  .empty() ? encoder::base64::atob(path)   : "",
        !data  .empty() ? encoder::base64::atob(data)   : ""
    );}

    /*.......................................................................*/

    void insert( const string_t& path, const apify_item_t& item ) const noexcept {
        auto x= obj->list.first(); while( x != nullptr ){
        if( path.size() > x->data.path.size() ){ break; }
        x = x->next; } obj->list.insert( x, item );
    }

    /*.......................................................................*/

    int path_match( APIFY& cli, const string_t& base, const string_t& path ) const noexcept {
        
        string_t pathname = normalize( base, path );
        if( cli.path.starts_with( pathname ) ){ return 1; }
        
            array_t<string_t> _path[2] = { 
            string::split_view(cli.path,'/'), 
            string::split_view(pathname,'/') };

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

    void run( const string_t& path, APIFY& cli ) const noexcept { do {

        auto     n = obj->mddl.first();
        auto _base = normalize( path, obj->path );
        function_t<void> next = [&](){ n = n->next; };

        while( n!=nullptr && !cli.is_done() ) 
             { execute( _base, n->data, cli, next ); }

    } while(0); if( cli.is_done() ){ return; } do {

        auto     n = obj->list.first();
        auto _base = normalize( path, obj->path );
        function_t<void> next = [&](){ n = n->next; };

        while( n!=nullptr && !cli.is_done() ){ int c=0;
        if ( path_match(cli,_base,n->data.path) ){
        if ( n->data.method==cli.method
        || ( n->data.method.empty() )){ 
             execute( _base, n->data, cli, next ); continue; 
        }  } next(); }

    } while(0); }

    /*.......................................................................*/

    string_t normalize( const string_t& base, const string_t& path ) const noexcept {
        auto new_path =  base.empty() ? ("/"+path) : path.empty() ? /*------*/
        /*---------------------------*/ ("/"+base) : path::join( base, path );
        return path::normalize( new_path );
    }

public:

    apify_host_t() noexcept : obj( new NODE() ) {}

    /*.........................................................................*/

    void     set_path( const string_t& path ) const noexcept { obj->path = path; }
    string_t get_path() /*-----------------*/ const noexcept { return obj->path; }

    /*.........................................................................*/

    const ROUTR& on( const string_t& _method, const string_t& _path, const CALBK& cb ) const noexcept {
        apify_item_t item; // memset( (void*) &item, 0, sizeof(item) );
        item.method = _method; item.data = cb;
        item.path   = _path  ; item.mode = 1 ;
        insert( _path, item ); return (*this);
    }

    const ROUTR& on( const string_t& _path, const CALBK& cb ) const noexcept {
        return on( nullptr, _path, cb );
    }

    const ROUTR& on( const CALBK& cb ) const noexcept {
        return on( nullptr, nullptr, cb );
    }

    /*.........................................................................*/

    void emit( const T& cli, const string_t& method, const string_t& path, const string_t& data ) const noexcept {
         next( cli, format( method, path, data ) );
    }

    /*.........................................................................*/

    const ROUTR& add( const string_t& _path, const ROUTR& cb ) const noexcept {
        apify_item_t item; // memset( (void*) &item, 0, sizeof(item) );
        cb.set_path( normalize( obj->path, _path ) );
        item.data   = cb; item.path   = cb.get_path();
        item.mode   = 2 ; item.method = nullptr;
        insert( _path, item ); return (*this);
    }

    const ROUTR& add( const ROUTR& cb ) const noexcept {
        return add( nullptr, cb );
    }

    /*.........................................................................*/

    const ROUTR& add( const string_t& _path, const MIDDL& cb ) const noexcept {
        apify_item_t item; // memset( (void*) &item, 0, sizeof(item) );
        item.path   = _path; item.method = nullptr;
        item.data   = cb   ; item.mode   = 0;
        if( _path.empty() ){ obj->mddl.push( item ); }
        else /*---------*/ { insert ( _path, item ); }
    return (*this); }

    const ROUTR& add( const MIDDL& cb ) const noexcept {
        return add( nullptr, cb );
    }

    /*.........................................................................*/

    int next( const T& cli, const string_t& message ) const noexcept {
    
        queue_t<ulong> raw; auto app=APIFY(cli); int c=0;

        for( ulong x=0; x<message.size(); x++ ){
        if ( message[x] != '.' ){ continue; } raw.push( x ); }

        auto idx = array_t<ulong>( raw.data() );

        while( !idx.empty()  ){ auto tmp = idx.slice_view( 0, 4 );
        if   (  tmp.size()<4 ){ break; }
            
            app.method  = encoder::base64::btoa( message.slice_view( tmp[0]+1, tmp[1] ) );
            app.path    = encoder::base64::btoa( message.slice_view( tmp[1]+1, tmp[2] ) );
            app.message = encoder::base64::btoa( message.slice_view( tmp[2]+1, tmp[3] ) );

            run( nullptr, app ); c++;

        idx.ptr().slice( 4, (ulong) -1 ); }

    return c; }

};}

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { namespace apify {
    
    template< class T > ROUTR<T>   add()               { return ROUTR<T>(); }
    template< class T > apify_t<T> add( const T& cli ) { return apify_t<T>( cli ); }
    template< class T > apify_t<T> get( const T& cli ) { return apify_t<T>( cli ); }

    string_t format( const string_t& method, const string_t& path, const string_t& data ){
    return regex::format( ".${0}.${1}.${2}.",
        !method.empty() ? encoder::base64::atob(method) : "",
        !path  .empty() ? encoder::base64::atob(path)   : "",
        !data  .empty() ? encoder::base64::atob(data)   : ""
    );}

}}

/*────────────────────────────────────────────────────────────────────────────*/

#undef MIDDL
#undef CALBK
#undef ROUTR
#undef APIFY
#endif

/*────────────────────────────────────────────────────────────────────────────*/