#pragma once
#include <iostream>
#include <vector>
#include "enetfix.h"
#include "DAGPacket.h"
#include "Player.h"
#include "GameNetworkGlobals.h"
#include "NetTypes.h"

class Client {
public:
	Client(size_t connections = 1, size_t channels = 2, enet_uint32 limit_in = 0, enet_uint32 limit_out = 0) {
		localAddress.host = ENET_HOST_ANY;
		localAddress.port = 7777;

		localHost = enet_host_create(NULL, connections, channels, limit_in, limit_out);
	}

	~Client() {
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

		ENetEvent netEvent{};
		//if service failed or had no message, or connectEvent is not a connection event
		if (enet_host_service(localHost, &netEvent, ms) <= 0 || netEvent.type != ENET_EVENT_TYPE_CONNECT) {
			enet_peer_reset(serverPeer);
			return false;
		}

		//sets client id for this client
		if (enet_host_service(localHost, &netEvent, ms) <= 0 || netEvent.type != ENET_EVENT_TYPE_RECEIVE) {
			enet_peer_reset(serverPeer);
			return false;
		}
		
		DAGPacket received(netEvent.packet);

		if (received.get_int(0) != SERVER_ID &&
			received.get_int(1) != (int)NetworkMessage::INFORM_PLAYER) {
			enet_peer_reset(serverPeer);
			return false;
		}

		clientID = received.get_int(2);
		printf("my client id is %d!", clientID);

		playerLimit = received.get_int(3);

		connected = true;

		enet_packet_destroy(netEvent.packet);
		return true;
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
				connected = false;
				return true;
			}
		}

		/* We've arrived here, so the disconnect attempt didn't */
		/* succeed yet.  Force the connection down.             */
		forceDisconnect();
		connected = false;
		return true;
	}

	void forceDisconnect() {
		enet_peer_reset(serverPeer);
		connected = false;
	}

	void sendToServer(ENetPacket* packet) {
		enet_peer_send(serverPeer, 0, packet);
	}

	ENetHost* get_localHost() {
		return localHost;
	}

	int get_clientID() {
		return clientID;
	}

	int get_playerLimit() {
		return playerLimit;
	}

	bool isConnected(){
		return connected;
	}
	/*bool set_clientID(int ms) {

	}*/
private:
	ENetAddress localAddress{};
	ENetHost* localHost{};

	ENetAddress serverAddress{};
	ENetPeer* serverPeer{};

	int clientID{};
	unsigned int playerLimit{};

	bool connected{};
	//std::vector<Player> players{};
};