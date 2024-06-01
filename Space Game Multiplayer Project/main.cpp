#include <iostream>
#include <algorithm>
#include <vector>
#include <list>
#include <string>

#include "raylib.h"
#include "raymath.h"

#include "DAGNetworking.h"
#include "DAG_GUI.h"

#include "Utils.h"
#include "Component.h"
#include "Player.h"

#include "GameNetworkGlobals.h"
#include "NetTypes.h"

enum class ProgramState {
	TITLE,
	NET_SELECT,
	IP_INPUT,
	GAME
};

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

void initENet() {
	if (enet_initialize() != 0) {
		fprintf(stderr, "uh oh! enet didn't initialize!\n");
		exit(EXIT_FAILURE);
	}
	atexit(enet_deinitialize);
	std::cout << "ENET init successful.\n";
}

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

class Projectile {
public:
	Projectile() {}

	Projectile(float x, float y, float v, float a, netID_t id) {
		hitbox.x = x;
		hitbox.y = y;
		velocity = v;
		angle = a;
		netID = id;
	}

	bool isTouching(const Rectangle& rec) {
		return CheckCollisionRecs(hitbox, rec);
	}

	void move() {
		hitbox.x += perSecond(velocity) * cosf(angle);
		hitbox.y += perSecond(velocity) * sinf(angle);
	}

	void draw() {
		DrawRectangleRec(hitbox, RED);
	}

	double get_initTime() {
		return initTime;
	}

	netID_t get_netID() {
		return netID;
	}
private:
	Rectangle hitbox{0, 0, 8, 8};
	float angle{};
	float velocity{};
	double initTime{ GetTime() };
	netID_t netID{};
};

class ProjectileManager {
public:
	ProjectileManager(size_t maxProjectiles) {
		projectiles.resize(maxProjectiles);
		std::fill(projectiles.begin(), projectiles.end(), nullptr);
	}

	size_t add(float x, float y, float v, float a) {
		auto it = std::find(projectiles.begin(), projectiles.end(), nullptr);

		size_t id = static_cast<size_t>(std::distance(projectiles.begin(), it));
		*it = std::move(std::make_unique<Projectile>(x, y, v, a, id));
		projectileCount++;
		
		return id;
	}

	void set(size_t id, float x, float y, float v, float a) {
		if (projectiles.at(id) == nullptr) projectileCount++;
		
		projectiles.at(id).reset();
		projectiles.at(id) = std::move(std::make_unique<Projectile>(x, y, v, a, id));
	}

	bool canAdd() {
		return std::find(projectiles.begin(), projectiles.end(), nullptr) != projectiles.end();
	}

	void kill(size_t netID) {
		projectiles.at(netID).reset();
	}

	void killOld(double seconds = 20.0f) {
		forAll([this, seconds](std::unique_ptr<Projectile>& p) { if ((GetTime() - p->get_initTime()) > seconds) { p.reset(); projectileCount--; } });
	}

	void drawAll() {
		forAll([](std::unique_ptr<Projectile>& p) { p->draw(); });
	}

	void moveAll() {
		forAll([](std::unique_ptr<Projectile>& p) { p->move(); });
	}

	//inefficient, will be ran multiple times to perform all collisions
	auto findColliding(const Rectangle& hitbox) {
		return std::find_if(projectiles.begin(), projectiles.end(), [&hitbox](std::unique_ptr<Projectile>& p) { if (p == nullptr) return false; return p->isTouching(hitbox); });
	}

	const std::vector<std::unique_ptr<Projectile>>& data() {
		return projectiles;
	}
private:
	template<typename _Fn>
	void forAll(_Fn func) {
		std::for_each(projectiles.begin(), projectiles.end(), [func](std::unique_ptr<Projectile>& p) {if (p == nullptr) return; func(p); });
	}

	std::vector<std::unique_ptr<Projectile>> projectiles{};
	size_t projectileCount = 0;
};

int main() {
	//INITIALIZATION
	initENet();

	InitWindow(800, 800, "Space Game Multiplayer Project");
	SetTargetFPS(60);

	ProgramState programState = ProgramState::TITLE;
	programState = ProgramState::NET_SELECT;

	NetworkMode netMode = NetworkMode::SINGLEPLAYER;

	GUI_IPInput gui_IPInput;
	GUI_NetSelect guiSelectorTest(&netMode);

	std::unique_ptr<Client> localClient{};
	std::unique_ptr<Server> localServer{};

	Component* localPlayer{};
	std::vector<std::unique_ptr<Component>> players{};
	std::vector<int> playerLosses{};

	ProjectileManager pManager(2048);

	//UPDATE LOOP
	ENetEvent netEvent{};
	while (!WindowShouldClose()) {
		BeginDrawing(); //called at start to allow queueing of textures during logic
		ClearBackground(WHITE);

		switch (programState) {
		case ProgramState::NET_SELECT: {
			localClient.reset();
			localServer.reset();

			guiSelectorTest.init();

			guiSelectorTest.update();

			if (guiSelectorTest.isNetModeSet()) {
				switch (netMode) {
				case NetworkMode::SINGLEPLAYER:
					programState = ProgramState::GAME;
					break;
				case NetworkMode::CLIENT:
					localClient = std::make_unique<Client>(1, 2, 0, 0);
					
					programState = ProgramState::IP_INPUT;
					break;
				case NetworkMode::SERVER:
					localServer = std::make_unique<Server>(4, 2, 0, 0);
					
					players.resize(localServer->get_playerLimit());
					playerLosses.resize(localServer->get_playerLimit());
					players.at(0) = std::move(std::make_unique<Component>(480.0f));
					
					localPlayer = players.at(0).get();
					
					programState = ProgramState::GAME;
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
				if (localClient->tryConnect(gui_IPInput.get_input().c_str(), 7777, 5000)) {
					printf("Successful connection to server!\n");
					gui_IPInput.deinit();

					players.resize(localClient->get_playerLimit());
					playerLosses.resize(localClient->get_playerLimit(), 0);
					players.at(localClient->get_clientID()) = std::make_unique<Component>(480.0f); //adds client to player list, client will add players as server says there are more

					localPlayer = players.at(localClient->get_clientID()).get();

					programState = ProgramState::GAME;
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

			//handle network events
			switch (netMode) {
			case NetworkMode::CLIENT: {
				while (enet_host_service(localClient->get_localHost(), &netEvent, 0) > 0) {
					if (netEvent.type == ENET_EVENT_TYPE_DISCONNECT) {
						printf("received disconnect req\n");
						enet_deinitialize();
						CloseWindow();
						return EXIT_SUCCESS;
					}
					
					if (netEvent.type != ENET_EVENT_TYPE_RECEIVE) break;
					DAGPacket rPacket(netEvent.packet);

					int rID = rPacket.get_int(0);
					int rMessage = rPacket.get_int(1);

					switch ((NetworkMessage)rMessage) {
					case NetworkMessage::UPDATE_PLAYER: {
						if (rID == localClient->get_clientID()) break;

						if (players.at(rID) == nullptr) {
							players.at(rID) = std::make_unique<Component>(480.0f);
						}

						//player data from packet
						float rX = rPacket.get_float(2);
						float rY = rPacket.get_float(3);
						float rA = rPacket.get_float(4);
						float rHP = rPacket.get_float(5);

						Component* playerToUpdate = players.at(rID).get();

						playerToUpdate->set_hitbox({ rX, rY, playerToUpdate->get_hitbox().width, playerToUpdate->get_hitbox().height });
						playerToUpdate->set_angle(rA);
						playerToUpdate->set_hp(rHP);

						break;
					}
					case NetworkMessage::REMOVE_PLAYER: {
						unsigned int tbrID = rPacket.get_int(2);

						if (players.at(tbrID) != nullptr) {
							players.at(tbrID).reset();
						}

						break;
					}
					case NetworkMessage::ADD_PROJECTILE: {
						size_t pID = rPacket.get_int(2);
						float pX = rPacket.get_float(3);
						float pY = rPacket.get_float(4);
						float pV = rPacket.get_float(5);
						float pA = rPacket.get_float(6);

						pManager.set(pID, pX, pY, pV, pA);
						break;
					}
					case NetworkMessage::REMOVE_PROJECTILE: {
						int pID = rPacket.get_int(2);
						pManager.kill(pID);
						break;
					}
					case NetworkMessage::UPDATE_PLAYER_LOSSES: {
						playerLosses.at(rID) = rPacket.get_int(2);

						break;
					}
					default:
						printf("Encountered unhandled network event id (%d). Discarding packet...\n", rMessage);
						break;
					}
					
					enet_packet_destroy(netEvent.packet);
				}
				break;
			}
			case NetworkMode::SERVER: {
				while (enet_host_service(localServer->get_localHost(), &netEvent, 0) > 0) {
					switch (netEvent.type) {
					case ENET_EVENT_TYPE_CONNECT: {
						//localServer->handleConnection(netEvent.peer);
						DAGPacket packet; //packet to be sent to inform the new client about the game

						int newID = localServer->addPeer(netEvent.peer); //client id will be set to the first index in players at which the value is nullptr
						players.at(newID) = std::move(std::make_unique<Component>(480.0f));

						netEvent.peer->data = reinterpret_cast<void*>(newID); //assigns new client id to newly connected peer //TODO: use for basic message validation?

						//0 INFORM_PLAYER id playerCap
						packet.appendHeader(SERVER_ID, NetworkMessage::INFORM_PLAYER);
						packet.append(newID);
						packet.append(localServer->get_playerLimit());
						
						localServer->sendTo(newID, packet.makePacket(ENET_PACKET_FLAG_RELIABLE));

						break;
					}
					case ENET_EVENT_TYPE_RECEIVE: {
						//SERVER DATA RECEIVE PROCESS: (DOES NOT HANDLE DISCONNECTIONS)
						//server receives data from client _id_
						//server updates its own data for _id_ first
						//server then needs to send a packet to all peers 
						//	packet contains the received data
						//	client _id_ will intentionally ignore and delete this packet

						//EXPECTED DATA: clientID, UPDATE_PLAYER, x, y, theta
						DAGPacket rPacket(netEvent.packet);

						//received data from client _rID_
						int rID = rPacket.get_int(0);
						int rMessage = rPacket.get_int(1);

						switch ((NetworkMessage)rMessage) {
						case NetworkMessage::UPDATE_PLAYER: {
							float rX = rPacket.get_float(2);
							float rY = rPacket.get_float(3);
							float rA = rPacket.get_float(4);
							int rHP = rPacket.get_int(5);

							//server updates its own data for _rID_
							Component* playerToUpdate = players.at(rID).get();

							playerToUpdate->set_hitbox({ rX, rY, playerToUpdate->get_hitbox().width, playerToUpdate->get_hitbox().height });
							playerToUpdate->set_angle(rA);
							playerToUpdate->set_hp(rHP);

							//server forwards packet to all peers
							localServer->sendAll(netEvent.packet);
							break;
						}
						case NetworkMessage::ADD_PROJECTILE: {
							/*
							shootPacket.append(pID);
							shootPacket.append(pX);
							shootPacket.append(pY);
							shootPacket.append(240.0f);
							shootPacket.append(shootAngle);
							*/
							size_t pID = rPacket.get_int(2);
							float pX = rPacket.get_float(3);
							float pY = rPacket.get_float(4);
							float pV = rPacket.get_float(5);
							float pA = rPacket.get_float(6);
							
							pManager.set(pID, pX, pY, pV, pA);

							localServer->sendAll(netEvent.packet);
							break;
						}
						case NetworkMessage::REMOVE_PROJECTILE: {
							int pID = rPacket.get_int(2);
							pManager.kill(pID);

							localServer->sendAll(netEvent.packet);
							break;
						}
						case NetworkMessage::UPDATE_PLAYER_LOSSES: {
							playerLosses.at(rID) = rPacket.get_int(2);

							localServer->sendAll(netEvent.packet);
							break;
						}
						}
						
						break;
					}
					case ENET_EVENT_TYPE_DISCONNECT: {
						int id = reinterpret_cast<int>(netEvent.peer->data);

						printf("Peer %d disconnected.\n", id);

						DAGPacket disconnectPacket;
						disconnectPacket.appendHeader(SERVER_ID, NetworkMessage::REMOVE_PLAYER);
						disconnectPacket.append(id);
						
						/* Reset the peer's client information. */
						netEvent.peer->data = NULL;

						localServer->removePeer(id);
						players.at(id).reset();

						localServer->sendAll(disconnectPacket.makePacket(ENET_PACKET_FLAG_RELIABLE));
						break;
					}
					}
				}
			}
			}

			pManager.killOld(5.0f);
			pManager.moveAll();

			//control local player
			localPlayer->control();

			//process all collisions with localHost's player, and send packets to delete the projectile with netID pID
			auto projectileIt = pManager.findColliding(localPlayer->get_hitbox());
			while (projectileIt != pManager.data().end()) {
				localPlayer->damage(50);
				
				int sID = SERVER_ID;
				if (netMode == NetworkMode::CLIENT) {
					sID = localClient->get_clientID();
				}

				size_t pID = (*projectileIt)->get_netID();
				pManager.kill(pID);

				DAGPacket pDestroyPacket;
				pDestroyPacket.appendHeader(sID, NetworkMessage::REMOVE_PROJECTILE);
				pDestroyPacket.append(pID);

				ENetPacket* packet = pDestroyPacket.makePacket(ENET_PACKET_FLAG_RELIABLE);

				switch (netMode) {
				case NetworkMode::CLIENT:
					localClient->sendToServer(packet);
					break;
				case NetworkMode::SERVER:
					localServer->sendAll(packet);
					break;
				}

				projectileIt = pManager.findColliding(localPlayer->get_hitbox());
			}

			//if dead, regen health and increase death count
			if (localPlayer->get_hp() <= 0) {
				localPlayer->set_hp(1000);

				int playerID = SERVER_ID;
				if (netMode == NetworkMode::CLIENT) {
					playerID = localClient->get_clientID();
				}

				playerLosses.at(playerID)++;

				if (netMode != NetworkMode::SINGLEPLAYER) {
					DAGPacket deathPacket;

					deathPacket.appendHeader(playerID, NetworkMessage::UPDATE_PLAYER_LOSSES);
					deathPacket.append(playerLosses.at(playerID));

					ENetPacket* packet = deathPacket.makePacket(ENET_PACKET_FLAG_RELIABLE);

					switch (netMode) {
					case NetworkMode::CLIENT:
						localClient->sendToServer(packet);
						break;
					case NetworkMode::SERVER:
						localServer->sendAll(packet);
						break;
					}
				}
			}

			//shoot
			if (IsMouseButtonPressed(MouseButton::MOUSE_BUTTON_LEFT)) {
				if (pManager.canAdd()) {
					float pX = (localPlayer->get_hitbox().x + localPlayer->get_hitbox().width / 2.0f);
					float pY = (localPlayer->get_hitbox().y + localPlayer->get_hitbox().height / 2.0f);
					float shootAngle = angleBetween(pX, pY, static_cast<float>(GetMouseX()), static_cast<float>(GetMouseY()));
					pX += 50 * cosf(shootAngle);
					pY += 50 * sinf(shootAngle);

					size_t pID = pManager.add(pX, pY, 240.0f, shootAngle);
				
					if (netMode != NetworkMode::SINGLEPLAYER) {
						DAGPacket shootPacket;

						int sID = SERVER_ID;
						if (netMode == NetworkMode::CLIENT) {
							sID = localClient->get_clientID();
						}
						
						shootPacket.appendHeader(sID, NetworkMessage::ADD_PROJECTILE);
						shootPacket.append(pID);
						shootPacket.append(pX);
						shootPacket.append(pY);
						shootPacket.append(240.0f);
						shootPacket.append(shootAngle);

						ENetPacket* packet = shootPacket.makePacket(ENET_PACKET_FLAG_RELIABLE);

						switch (netMode) {
						case NetworkMode::CLIENT:
							localClient->sendToServer(packet);
							break;
						case NetworkMode::SERVER:
							localServer->sendAll(packet);
							break;
						}
					}
				}
				
				if (netMode != NetworkMode::SINGLEPLAYER && pManager.canAdd()) {
					DAGPacket shootPacket;

					int sID = SERVER_ID;
					if (netMode == NetworkMode::CLIENT) {
						sID = localClient->get_clientID();
					}
					shootPacket.appendHeader(sID, NetworkMessage::ADD_PROJECTILE);

				}
			}

			//position packet
			if (netMode != NetworkMode::SINGLEPLAYER) {
				//get id to send
				int sID = SERVER_ID;
				if (netMode == NetworkMode::CLIENT) {
					sID = localClient->get_clientID();
				}

				//data
				float sX = localPlayer->get_hitbox().x;
				float sY = localPlayer->get_hitbox().y;
				float sAngle = localPlayer->get_angle();
				sAngle = fmodf(sAngle, 2 * PI); //cap sAngle between 0 and 2 * PI
				int sHP = localPlayer->get_hp();

				//EXPECTED DATA: clientID, UPDATE_PLAYER, x, y, theta
				DAGPacket sPacket;
				sPacket.appendHeader(sID, NetworkMessage::UPDATE_PLAYER);

				sPacket.append(TextFormat("%.1f", sX));
				sPacket.append(TextFormat("%.1f", sY));
				sPacket.append(TextFormat("%.2f", sAngle));
				sPacket.append(sHP);

				ENetPacket* packet = sPacket.makePacket(ENET_PACKET_FLAG_RELIABLE);

				switch (netMode) {
				case NetworkMode::CLIENT:
					localClient->sendToServer(packet);
					break;
				case NetworkMode::SERVER:
					localServer->sendAll(packet);
					break;
				}
			}
			
			
			if (netMode != NetworkMode::SINGLEPLAYER) {
				for (int i = 0; i < players.size(); i++) {
					Component* player = players.at(i).get();
					if (player == nullptr) continue;

					player->draw();
					DrawText(TextFormat("HP: %d", player->get_hp()), player->get_hitbox().x, player->get_hitbox().y - 30, 5, DARKGREEN);
					DrawText(TextFormat("Deaths: %d", playerLosses.at(i)), player->get_hitbox().x, player->get_hitbox().y - 20, 5, WHITE);
				}
			}
			localPlayer->drawHUD(10, 10);

			pManager.drawAll();
			break;
		}
		}

		EndDrawing();
	}


	//DEINITIALIZATION
	switch (netMode) {
	case NetworkMode::CLIENT:
		if (localClient->isConnected()) {
			localClient->tryDisconnect();
		}
		break;
	case NetworkMode::SERVER:
		localServer->closeServer();
		break;
	}

	enet_deinitialize();
	CloseWindow();
	return EXIT_SUCCESS;
}