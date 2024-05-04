#include "enetfix.h"
#include <iostream>

class LocalClient {
public:
	LocalClient(size_t connections = 1, size_t channels = 2, enet_uint32 limit_in = 0, enet_uint32 limit_out = 0) {
		localAddress.host = ENET_HOST_ANY;
		localAddress.port = 7777;

		localHost = enet_host_create(NULL, connections, channels, limit_in, limit_out);
	}

	~LocalClient() {
		enet_host_destroy(localHost);
	}

	bool tryConnect(const char* address = "127.0.0.1", enet_uint16 port = 7777, int ms = 5000) {
		enet_address_set_host(&serverAddress, address);
		serverAddress.port = port;

		serverPeer = enet_host_connect(localHost, &serverAddress, 2, 0);
		if (serverPeer == nullptr) {
			fprintf(stderr, "No available peers for initiating an ENET connection.\n");
			exit(EXIT_FAILURE);
		}

		ENetEvent connectEvent{};
		if (enet_host_service(localHost, &connectEvent, ms) > 0 && connectEvent.type == ENET_EVENT_TYPE_CONNECT) {
			return true;
		}
		else {
			enet_peer_reset(serverPeer);
			return false;
		}
	}

	bool tryDisconnect(int ms = 3000) {
		ENetEvent disconnectEvent;

		enet_peer_disconnect(serverPeer, 0);

		while (enet_host_service(localHost, &disconnectEvent, ms) > 0) {
			switch (disconnectEvent.type) {
			case ENET_EVENT_TYPE_RECEIVE:
				enet_packet_destroy(disconnectEvent.packet);
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				puts("Disconnection succeeded.");
				return true;
			}
		}

		/* We've arrived here, so the disconnect attempt didn't */
		/* succeed yet.  Force the connection down.             */
		forceDisconnect();
		return true;
	}

	void forceDisconnect() {
		enet_peer_reset(serverPeer);
	}

	void sendToServer(ENetPacket* packet) {
		enet_peer_send(serverPeer, 0, packet);
	}

	ENetHost* get_localHost() {
		return localHost;
	}
private:
	ENetAddress localAddress{};
	ENetHost* localHost{};

	ENetAddress serverAddress{};
	ENetPeer* serverPeer{};
};