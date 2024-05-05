#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <list>
#include "raylib.h"
#include "raymath.h"
#include <sstream>

#include "enetfix.h"
#include "LocalClient.h"
#include "LocalServer.h"
#include "DAGPacket.h"

#include "I_GUI.h"
#include "GUIButton.h"
#include "GUITextField.h"

#include "Utils.h"
#include "Component.h"
#include "Player.h"


//Phase 1 Development:
/*
* Implement basic networking [DONE]
* predefined, non-component ships [DONE]
* gun points towards mouse pointer 
* movement is rotation and step based [DONE]
* combat between two players possible 
* repeat game until someone leaves 
* ships explode when out of health
*/

//used to align value changes to be done every second, non-frame-dependent




//ship controlled by a player
//class Ship {
//public:
//	Ship(int hp) {
//		this->hp = hp;
//	}
//
//	void control() {
//		component->control();
//	}
//
//	void draw() {
//		component->draw();
//	}
//
//	void drawHUD(int x, int y) {
//		DrawText(TextFormat("a: %3.1f px/s^2", acceleration), 10 + x, 10 + y, 10, WHITE);
//		DrawText(TextFormat("v: %4.1f / %4.1f px/s", velocity, maxVelocity), 10 + x, 30 + y, 10, WHITE);
//		DrawText(TextFormat("x: %.1f, y: %.1f", component->get_hitbox().x, component->get_hitbox().y), 10 + x, 50 + y, 10, WHITE);
//		DrawText(TextFormat("angle: %.2f", angle), 10 + x, 70 + y, 10, WHITE);
//	}
//
//	//void update() //general update function
//	//void component_update() // to be called when a component is destroyed, when ships are multi-component.
//private:
//	Component* component{};
//	//DAG::SMatrix<Component*>
//
//	float angle{};
//	float velocity{};
//	float maxVelocity{}; //arbitrary
//	float acceleration{};
//
//	int hp{};
//	//std::list<Component> components;
//};



enum class NetworkMode {
	SINGLEPLAYER = 0,
	CLIENT,
	SERVER
};

void initENet() {
	if (enet_initialize() != 0) {
		fprintf(stderr, "uh oh! enet didn't initialize!\n");
		exit(EXIT_FAILURE);
	}
	atexit(enet_deinitialize);
	std::cout << "ENET init successful.\n";
}

//
/*NETWORK LOOP
* 
* client connects to server
* server adds client to vector, returns id to client
* 
* loop:
*	client sends packet to server
*	server's for client is updated
*	server sends its data to all clients
*/

enum class ProgramState {
	TITLE,
	NET_SELECT,
	IP_INPUT,
	GAME
};

class GUI_NetSelect : public I_GUI {
public:
	GUI_NetSelect(NetworkMode* netMode) : I_GUI() {
		this->netMode = netMode;
	}

	void init() override {
		if (initialized) return;
		
		btn_singleplayer.init({ 10, 10 }, "SINGLEPLAYER", 20, 4, 4, RED, BLACK, true);
		btn_client.init({ 10, 70 }, "CLIENT", 20, 4, 4, GREEN, BLACK, true);
		btn_server.init({ 10, 130 }, "SERVER", 20, 4, 4, BLUE, BLACK, true);
		
		netModeSet = false;
		initialized = true;
	}

	void update() override {
		if (btn_singleplayer.clicked()) {
			*netMode = NetworkMode::SINGLEPLAYER;
			
			deinit();
			return;
		}

		if (btn_client.clicked()) {
			*netMode = NetworkMode::CLIENT;
			
			deinit();
			return;
		}

		if (btn_server.clicked()) {
			*netMode = NetworkMode::SERVER;
			
			deinit();
			return;
		}
	}

	void deinit() override {
		if (!initialized) return;

		btn_singleplayer.disable();
		btn_client.disable();
		btn_server.disable();
		
		netModeSet = true;
		initialized = false;
	}

	void draw() override {
		btn_singleplayer.draw();
		btn_client.draw();
		btn_server.draw();
	}

	bool isNetModeSet() {
		return netModeSet;
	}
private:
	GUIButton btn_singleplayer{};
	GUIButton btn_client{};
	GUIButton btn_server{};

	NetworkMode* netMode{};
	bool netModeSet{};
};

class GUI_IPInput : public I_GUI {
public:
	void init() override {
		if (initialized) return;
		enterPressed = false;
		initialized = true;
	}
	
	void update() override {
		if (!input.is_focused()) return;

		int key = GetKeyPressed();
		enterPressed = false;
		
		if (key == KeyboardKey::KEY_BACKSPACE) {
			input.pop_back();
			return;
		}

		if (key == KeyboardKey::KEY_ENTER) {
			enterPressed = true;
			return;
		}

		input.append_if((char)key, [key]() { return (key >= KeyboardKey::KEY_ZERO && key <= KeyboardKey::KEY_NINE || key == KeyboardKey::KEY_PERIOD); });
	}
	
	void deinit() override {
		if (!initialized) return;
		enterPressed = false;
		input.clear();
		initialized = false;
	}
	
	const std::string& get_input() {
		return input.data();
	}

	void draw() override {
		input.draw(10, 10);
	}

	bool wasEnterPressed() {
		return enterPressed;
	}

private:
	GUITextField input{};
	bool enterPressed{};
};

int main() {
	initENet();

	InitWindow(800, 800, "Space Game Multiplayer Project");
	SetTargetFPS(60);

	ProgramState programState = ProgramState::TITLE;
	programState = ProgramState::NET_SELECT;

	NetworkMode netMode = NetworkMode::SINGLEPLAYER;

	GUI_IPInput gui_IPInput;
	GUI_NetSelect guiSelectorTest(&netMode);

	std::unique_ptr<LocalClient> DAGLocalClient{};
	std::unique_ptr<LocalServer> DAGLocalServer{};	

	Component player0(480.0f);
	Component player1(480.0f);
	Component player2(480.0f);

	Component* localPlayer{};
	//TODO: change to a list of available id's, as some may open up as a result of disconnections
	int connectedPlayers = 0;

	std::vector<Component*> players{};

	Player p;
	p.set_speed(8);

	ENetEvent netEvent{};
	while (!WindowShouldClose()) {
		BeginDrawing(); //called at start to allow queueing of textures during logic
		ClearBackground(WHITE);

		switch (programState) {
		case ProgramState::NET_SELECT: {
			DAGLocalClient.reset();
			DAGLocalServer.reset();

			guiSelectorTest.init();

			guiSelectorTest.update();

			if (guiSelectorTest.isNetModeSet()) {
				switch (netMode) {
				case NetworkMode::SINGLEPLAYER:
					programState = ProgramState::GAME;
					break;
				case NetworkMode::CLIENT:
					DAGLocalClient = std::make_unique<LocalClient>(1, 2, 0, 0);
					programState = ProgramState::IP_INPUT;
					break;
				case NetworkMode::SERVER:
					DAGLocalServer = std::make_unique<LocalServer>(4, 2, 0, 0);
					players.resize(DAGLocalServer->get_localHost()->peerCount, nullptr);
					programState = ProgramState::GAME;
					localPlayer = &player0;
					break;
				}
			}

			if (guiSelectorTest.isInitialized()) {
				guiSelectorTest.draw();
			}
			
			break;
		}
		case ProgramState::IP_INPUT: {
			gui_IPInput.init();

			gui_IPInput.update();

			if (gui_IPInput.wasEnterPressed()) {
				if (DAGLocalClient->tryConnect(gui_IPInput.get_input().c_str(), 7777, 5000)) {
					printf("Successful connection to server!\n");
					programState = ProgramState::GAME;
					gui_IPInput.deinit();

					switch (DAGLocalClient->get_clientID()) {
					case 1:
						localPlayer = &player1;
						break;
					case 2:
						localPlayer = &player2;
						break;
					}
				}
				else {
					fprintf(stderr, "Server connection unsuccessful.\n");
				}
				break;
			}
			
			if (gui_IPInput.isInitialized()) {
				gui_IPInput.draw();
			}
			
			break;
		}
		case ProgramState::GAME: {
			ClearBackground(BLACK);

			switch (netMode) {
			case NetworkMode::CLIENT: {
				while (enet_host_service(DAGLocalClient->get_localHost(), &netEvent, 0) > 0) {
					switch (netEvent.type) {
					case ENET_EVENT_TYPE_RECEIVE: {
						DAGPacket rPacket(netEvent.packet);
						//std::cout << "received from server: " << rPacket.get_data() << "\n";
						const std::vector<std::string>& rData = rPacket.get_data_array();

						int rID = std::atoi(rPacket.get_word(0));
						float rX = std::atof(rPacket.get_word(1));
						float rY = std::atof(rPacket.get_word(2));
						float rA = std::atof(rPacket.get_word(3));

						//ignore packet if data is about localClient
						if (rID == DAGLocalClient->get_clientID()) {
							enet_packet_destroy(netEvent.packet);
							break;
						}

						Component* playerToUpdate{};
						switch (rID) {
						case 0:
							playerToUpdate = &player0;
							break;
						case 1:
							playerToUpdate = &player1;
							break;
						case 2:
							playerToUpdate = &player2;
							break;
						}

						//client updates its own data for _rID_
						playerToUpdate->set_hitbox({ rX, rY, playerToUpdate->get_hitbox().width, playerToUpdate->get_hitbox().height });
						playerToUpdate->set_angle(rA);
						printf("successfully updated player client _%d_ with received data.\n", rID);

						//server sends packet to all peers

						//otherPlayer.set_hitbox({ rX, rY, otherPlayer.get_hitbox().width, otherPlayer.get_hitbox().height });
						//otherPlayer.set_angle(rA);

						enet_packet_destroy(netEvent.packet);

						/*player0.set_hitbox({ rX, rY, player0.get_hitbox().width, player0.get_hitbox().height });
						player0.set_angle(rA);

						enet_packet_destroy(netEvent.packet);*/
						break;
					}
					}
				}
				break;
			}
			case NetworkMode::SERVER: {
				while (enet_host_service(DAGLocalServer->get_localHost(), &netEvent, 0) > 0) {
					switch (netEvent.type) {
					case ENET_EVENT_TYPE_CONNECT: {
						DAGPacket packet;

						int incomingID = -1;
						//client id will be set to the first index in players at which the value is nullptr
						for (int i = 0; i < players.size(); i++) {
							if (players.at(i) == nullptr) {
								incomingID = i;
							}
						}

						packet.append(incomingID);

						netEvent.peer->data = (void*)(incomingID); //assigns new client id to newly connected peer

						DAGLocalServer->sendTo(netEvent.peer, 0, packet.makePacket(ENET_PACKET_FLAG_RELIABLE));
						//DAGLocalServer->sendAll(packet.makePacket(ENET_PACKET_FLAG_RELIABLE));
						break;
					}
					case ENET_EVENT_TYPE_RECEIVE: {
						//SERVER DATA RECEIVE PROCESS: (DOES NOT HANDLE DISCONNECTIONS)
						//server receives data from client _id_
						//server updates its own data for _id_ first
						//server then needs to send a packet to all peers 
						//	packet contains the received data
						//	client _id_ will intentionally ignore and delete this packet

						//std::cout << "received packet from client _" << (int)(netEvent.peer->data) << "_\n";
						
						DAGPacket rPacket(netEvent.packet);
						std::cout << "received from client _" << (int)(netEvent.peer->data) << "_: " << rPacket.get_data() << "\n";
						const std::vector<std::string>& rData = rPacket.get_data_array();

						//received data from client _rID_
						int rID = std::atoi(rData.at(0).c_str());
						float rX = std::atof(rData.at(1).c_str());
						float rY = std::atof(rData.at(2).c_str());
						float rA = std::atof(rData.at(3).c_str());

						//selecting which player to update
						Component* playerToUpdate{};
						switch (rID) {
						case 1:
							playerToUpdate = &player1;
							break;
						case 2:
							playerToUpdate = &player2;
							break;
						}

						//server updated its own data for _rID_
						playerToUpdate->set_hitbox({ rX, rY, playerToUpdate->get_hitbox().width, playerToUpdate->get_hitbox().height });
						playerToUpdate->set_angle(rA);

						//printf("successfully updated player client _%d_ with received data.\n", rID);

						//server sends packet to all peers
						DAGLocalServer->sendAll(netEvent.packet);
						break;
					}
					case ENET_EVENT_TYPE_DISCONNECT:
						printf("\n\n\npeer _%d_ disconnected.\n\n\n\n", (int)netEvent.peer->data);

						/* Reset the peer's client information. */

						netEvent.peer->data = NULL;
						break;
					}

				}
			}
			}

			//c.control();
			//p.move();
			localPlayer->control();

			if (netMode != NetworkMode::SINGLEPLAYER) {
				//get x and y of localHost's ship, prepare as packet to send to peers
				
				int sID;
				switch (netMode) {
				case NetworkMode::CLIENT:
					sID = DAGLocalClient->get_clientID();
					//enet_peer_send(serverPeer, 0, packet);
					break;
				case NetworkMode::SERVER:
					sID = 0;
					//enet_host_broadcast(localHost, 0, packet);
					break;
				}
				
				float sX = localPlayer->get_hitbox().x;
				float sY = localPlayer->get_hitbox().y;
				float sAngle = localPlayer->get_angle();

				DAGPacket sPacket;
				sPacket.append(sID);
				sPacket.append(TextFormat("%.1f", sX));
				sPacket.append(TextFormat("%.1f", sY));
				sPacket.append(TextFormat("%.2f", fmodf(sAngle, 2 * PI)));

				ENetPacket* packet = sPacket.makePacket(ENET_PACKET_FLAG_RELIABLE);
				switch (netMode) {
				case NetworkMode::CLIENT:
					DAGLocalClient->sendToServer(packet);
					//enet_peer_send(serverPeer, 0, packet);
					break;
				case NetworkMode::SERVER:
					DAGLocalServer->sendAll(packet);
					//enet_host_broadcast(localHost, 0, packet);
					break;
				}
			}

			
			localPlayer->draw();
			localPlayer->drawHUD(10, 10);
			//c.draw();
			//p.draw();
			//c.drawHUD(0, 0);

			if (netMode != NetworkMode::SINGLEPLAYER) {
				//otherPlayer.draw();
				//otherPlayer.drawHUD(0, 600);
				player0.draw();
				player1.draw();
				player2.draw();
			}
			break;
		}
		}

		EndDrawing();
	}

	enet_deinitialize();
	CloseWindow();
	return EXIT_SUCCESS;
}