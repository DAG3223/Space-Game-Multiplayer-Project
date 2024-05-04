#include "enetfix.h"

class LocalServer {
public:
	LocalServer(size_t connections = 1, size_t channels = 2, enet_uint32 limit_in = 0, enet_uint32 limit_out = 0) {
		localAddress.host = ENET_HOST_ANY;
		localAddress.port = 7777;

		localHost = enet_host_create(&localAddress, connections, channels, limit_in, limit_out);
	}

	~LocalServer() {
		enet_host_destroy(localHost);
	}

	void sendAll(ENetPacket* packet) {
		enet_host_broadcast(localHost, 0, packet);
	}

	ENetHost* get_localHost() {
		return localHost;
	}
private:
	ENetAddress localAddress{};
	ENetHost* localHost{};
};