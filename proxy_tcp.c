#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <syslog.h>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/event.h>

#include "debug.h"
#include "uthash.h"
#include "common.h"
#include "proxy.h"
#include "config.h"
#include "tcpmux.h"

#define	BUF_LEN	4098

// read data from local service
void tcp_proxy_c2s_cb(struct bufferevent *bev, void *ctx)
{
	struct common_conf  *c_conf = get_common_config();
	struct proxy_client *client = (struct proxy_client *)ctx;
	assert(client);
	struct bufferevent *partner = client->ctl_bev;
	assert(partner);
	struct evbuffer *src, *dst;
	src = bufferevent_get_input(bev);
	size_t len = evbuffer_get_length(src);
	assert(len > 0);
	dst = bufferevent_get_output(partner);
	if (!c_conf->tcp_mux) {
		evbuffer_add_buffer(dst, src);
		return;
	}
	
	tcp_mux_send_data(partner, client->stream_id, len);
	uint8_t buf[BUF_LEN];
	while(len > 0) {
		memset(buf, 0, BUF_LEN);
		int nread = bufferevent_read(bev, buf, BUF_LEN);
		assert(nread >= 0);
		bufferevent_write(partner, buf, nread);
		len -= nread;
	}
}

// read data from frps
// when tcp mux enable this function will not be used
void tcp_proxy_s2c_cb(struct bufferevent *bev, void *ctx)
{
	struct proxy_client *client = (struct proxy_client *)ctx;
	assert(client);
	struct bufferevent *partner = client->local_proxy_bev;
	assert(partner);
	struct evbuffer *src, *dst;
	src = bufferevent_get_input(bev);
	size_t len = evbuffer_get_length(src);
	assert(len > 0);
	dst = bufferevent_get_output(partner);
	evbuffer_add_buffer(dst, src);
}
