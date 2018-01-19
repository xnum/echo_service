#include <uv.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Our tcp server object.
 */
uv_tcp_t server;

/**
 * Shared reference to our event loop.
 */
uv_loop_t * loop;

/**
 * Function declarations.
 */
void alloc_buffer(uv_handle_t * handle, size_t size, uv_buf_t*);
void connection_cb(uv_stream_t * server, int status);
void read_cb(uv_stream_t * stream, ssize_t nread, const uv_buf_t *buf);

int main() {
    loop = uv_default_loop();
    
    /* convert a humanreadable ip address to a c struct */
    struct sockaddr_in addr;
    uv_ip4_addr("127.0.0.1", 3000, &addr);

    /* initialize the server */
    uv_tcp_init(loop, &server);
    /* bind the server to the address above */
    uv_tcp_bind(&server, &addr, 0);
    
    /* let the server listen on the address for new connections */
    int r = uv_listen((uv_stream_t *) &server, 128, connection_cb);

    if (r < 0) {
        return fprintf(stderr, "Error on listening: %s.\n", 
                uv_strerror(r));
    }

    /* execute all tasks in queue */
    return uv_run(loop, UV_RUN_DEFAULT);
}

/**
 * Callback which is executed on each new connection.
 */
void connection_cb(uv_stream_t * server, int status) {
    /* dynamically allocate a new client stream object on conn */
    uv_tcp_t * client = malloc(sizeof(uv_tcp_t));
    
    /* if status not zero there was an error */
    if (status < 0) {
        fprintf(stderr, "Error on listening: %s.\n", 
            uv_strerror(status));
    }

    /* initialize the new client */
    uv_tcp_init(loop, client);

    /* now let bind the client to the server to be used for incomings */
    if (uv_accept(server, (uv_stream_t *) client) == 0) {
        /* start reading from stream */
        int r = uv_read_start((uv_stream_t *) client, alloc_buffer, read_cb);

        if (r < 0) {
            fprintf(stderr, "Error on reading client stream: %s.\n", 
                    uv_strerror(r));
        }
    } else {
        /* close client stream on error */
        uv_close((uv_handle_t *) client, NULL);
    }
}

/**
 * Callback which is executed on each readable state.
 */
void read_cb(uv_stream_t * stream, ssize_t nread, const uv_buf_t *buf) {
    /* dynamically allocate memory for a new write task */
    uv_write_t req;
    
    /* if read bytes counter -1 there is an error or EOF */
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "Error on reading client stream: %s.\n", 
                    uv_strerror(nread));
        }

        uv_close((uv_handle_t *) stream, NULL);
    }

    uv_buf_t b = uv_buf_init(buf->base, nread);

    /* write sync the incoming buffer to the socket */
    int r = uv_write(&req, stream, &b, 1, NULL);

    /* free the remaining memory */
    free(buf->base);

    if (r < 0) {
        fprintf(stderr, "Error on writing client stream: %s.\n", 
                uv_strerror(r));
    }
}

/**
 * Allocates a buffer which we can use for reading.
 */
void alloc_buffer(uv_handle_t * handle, size_t size, uv_buf_t *buf) {
        buf->base = (char *) malloc(size);
	buf->len = size;
}
