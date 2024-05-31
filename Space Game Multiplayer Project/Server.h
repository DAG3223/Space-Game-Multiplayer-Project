#pragma once

#include "enetfix.h"
#include "DAGPacket.h"
#include "GameNetworkGlobals.h"
#include <vector>
#include "Player.h"
#include "NetTypes.h"

/*
* Server()
* ~Server()
* 
* kick(id)
* ban(id)
* close()
* 
* handleConnection()
* handleDisconnection()
* 
* addPeer()
* removePeer(id)
* assignIDToPeer()
* getFirstAvailableID()
* 
*/

class S {
public:
	S(unsigned int peerCount) {
		myAddress.host = ENET_HOST_ANY;
		myAddress.port = 7777;

		localHost = enet_host_create(&myAddress, peerCount, 2, 0, 0);
		
		maxPeers = peerCount;
		peers.resize(maxPeers, nullptr);
	}

	~S() {
		enet_host_destroy(localHost);
	}

	void handleConnection() {

	}

	void handleDisconnection() {

	}

	void addPeer() {

	}

	void removePeer() {

	}

	void giveIDToPeer() {

	}

	void kick(unsigned int id) {
		
	}

	void ban() {

	}

	void close() {

	}
private:
	ENetHost* localHost{};
	ENetAddress myAddress{};

	unsigned int maxPeers{};
	std::vector<ENetPeer*> peers{};
};




class Server {
public:
	Server(peerID_t extPlayers = 1, size_t channels = 2, enet_uint32 limit_in = 0, enet_uint32 limit_out = 0) {
		localAddress.host = ENET_HOST_ANY;
		localAddress.port = 7777;

		localHost = enet_host_create(&localAddress, extPlayers, channels, limit_in, limit_out);
		
		peerLimit = extPlayers + static_cast<peerID_t>(1);
		peers.resize(peerLimit, nullptr);
	}

	~Server() {
		enet_host_destroy(localHost);
	}

	void sendTo(peerID_t peer, ENetPacket* packet, enet_uint8 channel = 0) {
		sendTo(getPeer(peer), packet, channel);
	}

	void sendTo(ENetPeer* peer, ENetPacket* packet, enet_uint8 channel = 0) {
		enet_peer_send(peer, channel, packet);
	}

	void sendAll(ENetPacket* packet, enet_uint8 channel = 0) {
		enet_host_broadcast(localHost, channel, packet);
	}

	

	ENetHost* get_localHost() {
		return localHost;
	}

	unsigned short get_playerLimit() {
		return peerLimit;
	}

	//returns id for this peer
	peerID_t addPeer(ENetPeer* peer) {
		int id = getFirstAvailableID();
		if (id == -1) return -1;
		
		peers.at(id) = peer;

		return id;
	}

	int getFirstAvailableID() {
		for (int i = 1; i < peers.size(); i++) {
			if (peers.at(i) == nullptr) {
				return i;
			}
		}

		return -1;
	}

	ENetPeer* getPeer(unsigned int id) {
		return peers.at(id);
	}

	peerID_t getPeerID(ENetPeer* peer) {
		return reinterpret_cast<peerID_t>(peer->data);
	}

	void removePeer(peerID_t id) {
		enet_peer_disconnect_now(getPeer(id), 0);
		peers.at(id) = nullptr;
	}

	void closeServer() {
		for (int i = 1; i < peers.size(); i++) {
			if (peers.at(i) == nullptr) continue;
			enet_peer_disconnect_now(peers.at(i), 0);
		}
	}
	
private:
	ENetAddress localAddress{};
	ENetHost* localHost{};

	unsigned short peerLimit{};
	std::vector<ENetPeer*> peers; //0 reserved
};
