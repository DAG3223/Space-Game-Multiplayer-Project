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
float perSecond(float value) {
	return value * GetFrameTime();                
}

float angleBetween(float x1, float y1, float x2, float y2) {
	return atan2f(y2 - y1, x2 - x1);
}

class Component {
public:
	Component(float tVel) {
		hitbox = { 0.0f, 0.0f, 32.0f, 32.0f };
		maxVelocity = tVel;
	}

	void control() {
		//control linear acceleration
		acceleration = 0;
		if (IsKeyDown(KeyboardKey::KEY_W)) {
			acceleration += 16;
		}
		if (IsKeyDown(KeyboardKey::KEY_S)) {
			acceleration -= 16;
		}

		//change and clamp velocity, (v = v0 + at)
		velocity += perSecond(acceleration);
		velocity = fminf(velocity, maxVelocity);
		velocity = fmaxf(velocity, -maxVelocity);

		//move
		hitbox.x += perSecond(velocity) * cosf(angle);
		hitbox.y += perSecond(velocity) * sinf(angle);

		if (IsKeyDown(KeyboardKey::KEY_A)) {
			angle -= perSecond(PI); //turn left
		}
		
		if (IsKeyDown(KeyboardKey::KEY_D)) {
			angle += perSecond(PI); //turn right
		}
	}

	void draw() {
		Rectangle display = { hitbox.x + hitbox.width / 2.0f, hitbox.y + hitbox.height / 2.0f, hitbox.width, hitbox.height }; //display oriented inside hitbox
		Vector2 center = { hitbox.width / 2.0f, hitbox.height / 2.0f }; //offset from origin of hitbox
		DrawRectanglePro(display, center, RAD2DEG * angle, WHITE);

		//debug displays
		//DrawRectangleLinesEx(hitbox, 2.0f, RED);
		//DrawCircle(hitbox.x, hitbox.y, 2.0f, RED);
		//DrawLine(display.x, display.y, display.x + 30 * cosf(angle), display.y + 30 * sinf(angle), RED);
	}

	void drawHUD(int x, int y) {
		DrawText(TextFormat("a: %3.1f px/s^2", acceleration), 10 + x, 10 + y, 10, WHITE);
		DrawText(TextFormat("v: %4.1f / %4.1f px/s", velocity, maxVelocity), 10 + x, 30 + y, 10, WHITE);
		DrawText(TextFormat("x: %.1f, y: %.1f", hitbox.x, hitbox.y), 10 + x, 50 + y, 10, WHITE);
		DrawText(TextFormat("angle: %.2f", fmodf(angle, 2 * PI)), 10 + x, 70 + y, 10, WHITE);
	}

	const Rectangle& get_hitbox() {
		return hitbox;
	}

	void set_hitbox(const Rectangle& rec) {
		hitbox = rec;
	}

	float get_angle() {
		return angle;
	}

	void set_angle(float radians) {
		angle = radians;
	}
private:
	Rectangle hitbox{};
	float angle{};
	float velocity{};
	float maxVelocity{}; //arbitrary
	float acceleration{};
};

//ship controlled by a player
class Ship {
public:
	Ship(int hp) {
		this->hp = hp;
	}

	void control() {
		component->control();
	}

	void draw() {
		component->draw();
	}

	void drawHUD(int x, int y) {
		DrawText(TextFormat("a: %3.1f px/s^2", acceleration), 10 + x, 10 + y, 10, WHITE);
		DrawText(TextFormat("v: %4.1f / %4.1f px/s", velocity, maxVelocity), 10 + x, 30 + y, 10, WHITE);
		DrawText(TextFormat("x: %.1f, y: %.1f", component->get_hitbox().x, component->get_hitbox().y), 10 + x, 50 + y, 10, WHITE);
		DrawText(TextFormat("angle: %.2f", angle), 10 + x, 70 + y, 10, WHITE);
	}

	//void update() //general update function
	//void component_update() // to be called when a component is destroyed, when ships are multi-component.
private:
	Component* component{};
	//DAG::SMatrix<Component*>

	float angle{};
	float velocity{};
	float maxVelocity{}; //arbitrary
	float acceleration{};

	int hp{};
	//std::list<Component> components;
};

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

enum PROGRAM_STATES {
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

	NetworkMode netMode = NetworkMode::SINGLEPLAYER;

	std::unique_ptr<LocalClient> DAGLocalClient{};
	std::unique_ptr<LocalServer> DAGLocalServer{};

	int programState = TITLE;
	programState = NET_SELECT;

	//GUITextField addressField;
	GUI_IPInput gui_IPInput;
	GUI_NetSelect guiSelectorTest(&netMode);

	Component c(480.0f);
	Component otherPlayer(480.0f);

	ENetEvent netEvent{};
	while (!WindowShouldClose()) {
		BeginDrawing(); //called at start to allow queueing of textures during logic
		ClearBackground(WHITE);

		switch (programState) {
		case PROGRAM_STATES::NET_SELECT: {
			DAGLocalClient.reset();
			DAGLocalServer.reset();

			guiSelectorTest.init();

			guiSelectorTest.update();

			if (guiSelectorTest.isNetModeSet()) {
				switch (netMode) {
				case NetworkMode::SINGLEPLAYER:
					programState = PROGRAM_STATES::GAME;
					break;
				case NetworkMode::CLIENT:
					DAGLocalClient = std::make_unique<LocalClient>(1, 2, 0, 0);
					programState = PROGRAM_STATES::IP_INPUT;
					break;
				case NetworkMode::SERVER:
					DAGLocalServer = std::make_unique<LocalServer>(4, 2, 0, 0);
					programState = PROGRAM_STATES::GAME;
					break;
				}
			}

			if (guiSelectorTest.isInitialized()) {
				guiSelectorTest.draw();
			}
			
			break;
		}
		case PROGRAM_STATES::IP_INPUT: {
			gui_IPInput.init();

			gui_IPInput.update();

			if (gui_IPInput.wasEnterPressed()) {
				if (DAGLocalClient->tryConnect(gui_IPInput.get_input().c_str(), 7777, 5000)) {
					printf("Successful connection to server!\n");
					programState = PROGRAM_STATES::GAME;
					gui_IPInput.deinit();
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
		case PROGRAM_STATES::GAME: {
			ClearBackground(BLACK);

			switch (netMode) {
			case NetworkMode::CLIENT: {
				while (enet_host_service(DAGLocalClient->get_localHost(), &netEvent, 0) > 0) {
					switch (netEvent.type) {
					case ENET_EVENT_TYPE_RECEIVE: {
						DAGPacket rPacket(netEvent.packet);
						std::vector<std::string> rData = rPacket.get_data_array();

						float rX = std::atof(rData.at(0).c_str());
						float rY = std::atof(rData.at(1).c_str());
						float rA = std::atof(rData.at(2).c_str());

						otherPlayer.set_hitbox({ rX, rY, otherPlayer.get_hitbox().width, otherPlayer.get_hitbox().height });
						otherPlayer.set_angle(rA);

						enet_packet_destroy(netEvent.packet);
						break;
					}
					}
				}
				break;
			}
			case NetworkMode::SERVER: {
				while (enet_host_service(DAGLocalServer->get_localHost(), &netEvent, 0) > 0) {
					std::cout << "\n\nserver received event\n\n";
					switch (netEvent.type) {
					case ENET_EVENT_TYPE_CONNECT: {
						if (netMode != NetworkMode::SERVER) break;

						DAGPacket packet;
						packet.append("Welcome to the server!");

						DAGLocalServer->sendAll(packet.makePacket(ENET_PACKET_FLAG_RELIABLE));
						break;
					}
					case ENET_EVENT_TYPE_RECEIVE: {
						DAGPacket rPacket(netEvent.packet);
						std::vector<std::string> rData = rPacket.get_data_array();

						float rX = std::atof(rData.at(0).c_str());
						float rY = std::atof(rData.at(1).c_str());
						float rA = std::atof(rData.at(2).c_str());

						otherPlayer.set_hitbox({ rX, rY, otherPlayer.get_hitbox().width, otherPlayer.get_hitbox().height });
						otherPlayer.set_angle(rA);

						enet_packet_destroy(netEvent.packet);
						break;
					}
					case ENET_EVENT_TYPE_DISCONNECT:
						//printf("%s disconnected.\n", netEvent.peer->data);

						/* Reset the peer's client information. */

						netEvent.peer->data = NULL;
						break;
					}

				}
			}
			}

			if (netMode != NetworkMode::SINGLEPLAYER) {
				//get x and y of localHost's ship, prepare as packet to send to peers
				float sX = c.get_hitbox().x;
				float sY = c.get_hitbox().y;
				float sAngle = c.get_angle();

				DAGPacket sPacket;
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

			c.control();

			c.draw();
			c.drawHUD(0, 0);

			if (netMode != NetworkMode::SINGLEPLAYER) {
				otherPlayer.draw();
				otherPlayer.drawHUD(0, 600);
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