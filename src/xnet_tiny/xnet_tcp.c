#include "include/xnet_tcp.h"
#include "include/xnet_sys.h"

#include <string.h>

void xtcp_init() {
	memset(tcp_socket, 0, sizeof(tcp_socket));
}


void xtcp_in(xipaddr_t* remote_ip, xnet_packet_t* packet) {
	xtcp_hdr_t* tcp_hdr = (xtcp_hdr_t*)packet->data;

	if (packet->size < sizeof(xtcp_hdr_t)) {
		return;
	}

	uint16_t pre_checksum;

	pre_checksum = tcp_hdr->checksum;
	tcp_hdr->checksum = 0;

	// 如果接收的包 checksum 为 0，则不要检查。
	// 非 0 才是需要检查
	if (pre_checksum != 0) {
		uint16_t checksum = checksum_peso(remote_ip, &netif_ipaddr, XNET_PROTOCOL_TCP, (uint16_t*)tcp_hdr, packet->size);
		checksum = (checksum == 0) ? 0xffff : checksum;
		if (pre_checksum != checksum) {
			printf("error: TCP checksum invalid\n");
			return;
		}
	}

	printf("received TCP packet\n");
}


xtcp_t* xtcp_alloc() {
	xtcp_t* tcp;
	xtcp_t* end;

	for (tcp = tcp_socket, end = tcp_socket + XTCP_CFG_MAX_TCP; tcp < end; tcp++) {
		if (tcp->state == XTCP_STATE_FREE) {
			tcp->local_port  = 0;
			tcp->remote_port = 0;
			tcp->remote_ip.addr = 0;
			tcp->handler = (xtcp_handler_t)0;
			return tcp;
		}
	}

	return (xtcp_t*)0;
}

void xtcp_free(xtcp_t* tcp) {
	tcp->state = XTCP_STATE_FREE;
}


xtcp_t* xtcp_find(xipaddr_t* remote_ip, uint16_t remote_port, uint16_t local_port) {
	xtcp_t* tcp;
	xtcp_t* end;
	xtcp_t* founded_tcp = (xtcp_t*)0;

	for (tcp = tcp_socket, end = &tcp_socket[XTCP_CFG_MAX_TCP]; tcp < end; tcp++) {
		if (tcp->state == XTCP_STATE_FREE) {
			continue;
		}

		if (tcp->local_port != local_port) {
			continue;
		}

		if (xipaddr_is_equal(remote_ip, &tcp->remote_ip) && (remote_port == tcp->remote_port)) {
			return tcp;
		}

		if (tcp->state == XTCP_STATE_LISTEN) {
			founded_tcp = tcp;
		}
	}

	return founded_tcp;
}



xtcp_t* xtcp_open(xtcp_handler_t handler) {
	xtcp_t* tcp = xtcp_alloc();
	if (!tcp) return (xtcp_t*)0;

	tcp->state   = XTCP_STATE_CLOSED;
	tcp->handler = handler;
	return tcp;
}


xnet_err_t xtcp_bind(xtcp_t* tcp, uint16_t local_port) {
	xtcp_t* curr;
	xtcp_t* end;

	for (curr = tcp_socket, end = &tcp_socket[XTCP_CFG_MAX_TCP]; tcp < end; tcp++) {
		if ((curr != tcp) && (curr->local_port == local_port)) {
			return XNET_ERR_BINDED;
		}
	}

	tcp->local_port = local_port;
	return XNET_ERR_OK;
}


xnet_err_t xtcp_listen(xtcp_t* tcp) {
	tcp->state = XTCP_STATE_LISTEN;
	return XNET_ERR_OK;
}


xnet_err_t xtcp_close(xtcp_t* tcp) {
	xtcp_free(tcp);
	return XNET_ERR_OK;
}