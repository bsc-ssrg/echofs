/* * Copyright (C) 2017 Barcelona Supercomputing Center
 *                    Centro Nacional de Supercomputacion
 *
 * This file is part of the Data Scheduler, a daemon for tracking and managing
 * requests for asynchronous data transfer in a hierarchical storage environment.
 *
 * See AUTHORS file in the top level directory for information
 * regarding developers and contributors.
 *
 * The Data Scheduler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Data Scheduler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Data Scheduler.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/un.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <efs-api.h>

#include "defaults.h"
#include "xmalloc.h"
#include "xstring.h"
#include "communication.h"

static int connect_to_daemon(void);
static int send_message(int conn, const msgbuffer_t* buffer);
static int recv_message(int conn, msgbuffer_t* buffer);
static ssize_t recv_data(int conn, void* data, size_t size);
static ssize_t send_data(int conn, const void* data, size_t size);
static void print_hex(void* buffer, size_t bytes);

static inline uint64_t htonll(uint64_t x) {
    return ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32));
}

static inline uint64_t ntohll(uint64_t x) {
    return ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32));
}

int
send_load_request(EFS_FILE* handle) {

    msgbuffer_t req_buf = MSGBUFFER_INIT();
    msgbuffer_t resp_buf = MSGBUFFER_INIT();
    response_t resp;
    int res;

    if((res = pack_to_buffer(EFS_LOAD_PATH, &req_buf, handle)) != EFS_API_SUCCESS) {
        return res;
    }

    int conn = connect_to_daemon();

    if(conn == -1) {
        res = EFS_API_ECONNFAILED;
        goto cleanup_on_error;
    }

	// connection established, send the message
	if(send_message(conn, &req_buf) < 0) {
	    res = EFS_API_ERPCSENDFAILED;
	    goto cleanup_on_error;
    }

    // wait for the daemon's response
    if(recv_message(conn, &resp_buf) < 0) {
        res = EFS_API_ERPCRECVFAILED;
	    goto cleanup_on_error;
    }

    if((res = unpack_from_buffer(&resp_buf, &resp)) != EFS_API_SUCCESS) {
	    goto cleanup_on_error;
    }

    if(resp.r_type == EFS_REQUEST_ACCEPTED) {
        return EFS_API_SUCCESS;
    }


cleanup_on_error:
    return res;
}

int 
send_unload_request(EFS_FILE* handle) {

    msgbuffer_t buffer;
    int res;

    if((res = pack_to_buffer(EFS_UNLOAD_PATH, &buffer, handle)) != EFS_API_SUCCESS) {
        return res;
    }

    return EFS_API_SUCCESS;
}

static int 
connect_to_daemon(void) {

	struct sockaddr_un server;

	int sfd = socket(AF_UNIX, SOCK_STREAM, 0);

	if (sfd < 0) {
	    return -1;
	}

	server.sun_family = AF_UNIX;
	strncpy(server.sun_path, api_sockfile, sizeof(server.sun_path));
	server.sun_path[sizeof(server.sun_path)-1] = '\0';

	if (connect(sfd, (struct sockaddr *) &server, sizeof(struct sockaddr_un)) < 0) {
	    return -1;
    }

    return sfd;
}

static int 
send_message(int conn, const msgbuffer_t* buffer) {

    if(buffer == NULL || buffer->b_data == NULL || buffer->b_size == 0) {
        return -1;
    }

    const void* msg_data = buffer->b_data;
    size_t msg_length = buffer->b_size;


    // transform the message size into network order and send it 
    // before the actual data

    uint64_t prefix = htonll(msg_length);

    assert(sizeof(prefix) == API_MESSAGE_HEADER_LENGTH); 

	if(send_data(conn, &prefix, sizeof(prefix)) < 0) {
        return -1;
    }

	if(send_data(conn, msg_data, msg_length) < 0) {
        return -1;
    }

    return 0;
}

static int 
recv_message(int conn, msgbuffer_t* buffer) {

    // first of all read the message prefix and decode it 
    // so that we know how much data to receive
    uint64_t prefix = 0;

    if(recv_data(conn, &prefix, sizeof(prefix)) < 0) {
        goto recv_error;
    }

    print_hex(&prefix, sizeof(prefix));

    size_t msg_size = ntohll(prefix);

    if(msg_size == 0) {
        goto recv_error;
    }

    void* msg_data = xmalloc_nz(msg_size);

    if(msg_data == NULL) {
        goto recv_error;
    }

    if(recv_data(conn, msg_data, msg_size) < 0) {
        xfree(msg_data);
        goto recv_error;
    }

    print_hex(msg_data, msg_size);

    buffer->b_data = msg_data;
    buffer->b_size = msg_size;

    return 0;

recv_error:
    buffer->b_data = NULL;
    buffer->b_size = 0;

    return -1;
}

static ssize_t 
recv_data(int conn, void* data, size_t size) {

    size_t brecvd = 0; // bytes received
    size_t bleft = size; // bytes left to receive
    ssize_t n = 0;

	while(brecvd < size) {
		n = read(conn, data + brecvd, bleft);

		fprintf(stdout, "read %zd\n", n);

		if(n == -1 || n == 0) {
		    if(errno == EINTR) {
		        continue;
            }
			break;
		}

        brecvd += n;
        bleft -= n;
	}

	return (n == -1 ? n : (ssize_t) brecvd);
}


static ssize_t 
send_data(int conn, const void* data, size_t size) {

	size_t bsent = 0;	// bytes sent
	size_t bleft = size;// bytes left to send
	ssize_t n = 0;

	// send() might not send all the bytes we ask it to,
	// because the kernel can decide not to send all the
	// data out in one chunk
	while(bsent < size) {
		n = write(conn, data + bsent, bleft);

		if(n == -1) {
		    if(errno == EINTR) {
		        continue;
            }
			break;
		}

        bsent += n;
        bleft -= n;
	}

	return (n == -1 ? n : (ssize_t) bsent);
}

static void print_hex(void* buffer, size_t bytes) {

    unsigned char* p = (unsigned char*) buffer;

    fprintf(stdout, "<< ");

    for(size_t i = 0; i < bytes; ++i) {
        fprintf(stdout, "%02x ", (int) p[i]);
    }

    fprintf(stdout, " >>\n");
}
