#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <list>
#include <raylib.h>
#include <raymath.h>

#define NOGDI
#define CloseWindow CloseWinWindow
#define ShowCursor ShowWinCursor
#define LoadImage LoadWinImage
#define DrawText DrawWinText
#define DrawTextEx DrawWinTextEx
#define PlaySound PlayWinSound
#include <enet/enet.h>
#undef CloseWindow
#undef Rectangle
#undef ShowCursor
#undef LoadImage
#undef DrawText
#undef DrawTextEx
#undef PlaySound

//Phase 1 Development:
/*
* Implement basic networking
* predefined, non-component ships
* gun points towards mouse pointer
* movement is rotation and step based
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
		DrawRectangleLinesEx(hitbox, 2.0f, RED);
		DrawCircle(hitbox.x, hitbox.y, 2.0f, RED);
		DrawLine(display.x, display.y, display.x + 30 * cosf(angle), display.y + 30 * sinf(angle), RED);
	}

	void drawHUD(int x, int y) {
		DrawText(TextFormat("a: %3.1f px/s^2", acceleration), 10 + x, 10 + y, 10, WHITE);
		DrawText(TextFormat("v: %4.1f / %4.1f px/s", velocity, maxVelocity), 10 + x, 30 + y, 10, WHITE);
		DrawText(TextFormat("x: %.1f, y: %.1f", hitbox.x, hitbox.y), 10 + x, 50 + y, 10, WHITE);
		DrawText(TextFormat("angle: %.2f", angle), 10 + x, 70 + y, 10, WHITE);
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

//TODO:
/*
* kill benjamin
* create network which can send and receive a ships's positional data (round data to 2 decimal precision?) 
*/

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

NetworkMode promptNetMode() {
	std::cout << "Choose network configuration:\n\t(0): Singleplayer\n\t(1): Client\n\t(2): Server\n\t Choice: ";
	int nM = 0;
	std::cin >> nM;

	return (NetworkMode)nM;
}

int main() {
	initENet();

	//initialize in console as server or client prior to game start
	NetworkMode netMode = NetworkMode::SINGLEPLAYER;
	netMode = promptNetMode();

	ENetHost* localHost{};
	
	ENetAddress localAddress{};
	localAddress.host = ENET_HOST_ANY;
	localAddress.port = 7777;

	//init host by netMode
	switch (netMode) {
	case NetworkMode::SINGLEPLAYER: {
		break;
	}
	case NetworkMode::CLIENT: {
		localHost = enet_host_create(NULL, 1, 2, 0, 0);
		break;
	}
	case NetworkMode::SERVER: {
		localHost = enet_host_create(&localAddress, 1, 2, 0, 0);
		break;
	}
	}

	if (localHost == nullptr && netMode != NetworkMode::SINGLEPLAYER) {
		fprintf(stderr, "Failed to initialize localHost for networking.\n");
		exit(EXIT_FAILURE);
	}
	else {
		printf("localHost initialized properly.\n");
	}
	//done initializing localHost

	//connection handling
	//TODO: make code return to input page if connection fails
	ENetPeer* serverPeer{}; //external for later use
	if (netMode == NetworkMode::CLIENT) {
		ENetAddress targetAddr{};
		ENetEvent connectEvent{};

		std::string addressStr{};
		std::cout << "\nPlease enter an IPv4 address to connect to:\n\tAddress: ";
		std::cin >> addressStr;

		enet_address_set_host(&targetAddr, addressStr.c_str());
		targetAddr.port = 7777;

		serverPeer = enet_host_connect(localHost, &targetAddr, 2, 0);
		if (serverPeer == nullptr) {
			fprintf(stderr, "No available peers for initiating an ENET connection.\n");
			exit(EXIT_FAILURE);
		}

		//try for 5 seconds to connect
		//exit if failure
		if (enet_host_service(localHost, &connectEvent, 5000) > 0 && connectEvent.type == ENET_EVENT_TYPE_CONNECT) {
			printf("Successful connection to server!\n");
		}
		else {
			enet_peer_reset(serverPeer);

			fprintf(stderr, "Server connection unsuccessful. Closing...\n");
			exit(EXIT_SUCCESS);
		}
	}
	
	//end client/server init

	InitWindow(800, 800, "Space Game Multiplayer Project");
	SetTargetFPS(60);

	//std::vector<Component*> players;
	//players.emplace_back(480.0f);

	Component c(480.0f);
	Component otherPlayer(480.0f);

	ENetEvent netEvent{};
	while (!WindowShouldClose()) {
		while (enet_host_service(localHost, &netEvent, 0)) {
			switch (netEvent.type) {
			case ENET_EVENT_TYPE_CONNECT: {
				//players.emplace_back(480.0f);
				if (netMode == NetworkMode::SERVER) {
					ENetPacket* packet = enet_packet_create("Welcome to the server!", 23, ENET_PACKET_FLAG_RELIABLE);
					enet_host_broadcast(localHost, 0, packet);
				}
				break;
			}
			case ENET_EVENT_TYPE_RECEIVE: {
				std::cout << netEvent.packet->data << "\n";
				const char* data = (const char*)netEvent.packet->data;
				std::string str_data{ data };

				float rX{};
				float rY{};
				float rA{};

				auto it = str_data.begin();
				
				std::string xStr{};
				while (*it != ' ') {
					xStr.push_back(*it);
					it++;
				}

				it++;
				std::string yStr{};
				while (*it != ' ') {
					yStr.push_back(*it);
					it++;
				}

				it++;
				std::string angleStr{};
				while (it != str_data.end()) {
					angleStr.push_back(*it);
					it++;
				}

				rX = std::atof(xStr.c_str());
				rY = std::atof(yStr.c_str());
				rA = std::atof(angleStr.c_str());
				std::cout << "x set to " << rX << "\n";
				std::cout << "y set to " << rY << "\n";
				std::cout << "angle set to " << rA << "\n";

				otherPlayer.set_hitbox({rX, rY, 32, 32});
				otherPlayer.set_angle(rA);

				enet_packet_destroy(netEvent.packet);
				break;
			}
			}
		}
		
		//get x and y of localHost's ship, prepare as packet to send to peers
		float sendX = c.get_hitbox().x;
		float sendY = c.get_hitbox().y;
		float sendAngle = c.get_angle();

		std::string packet_str{};

		packet_str.append(std::to_string(sendX));
		packet_str.append(" ");
		packet_str.append(std::to_string(sendY));
		packet_str.append(" ");
		packet_str.append(std::to_string(sendAngle));
		

		ENetPacket* packet = enet_packet_create(packet_str.c_str(), strlen(packet_str.c_str()) + 1, ENET_PACKET_FLAG_RELIABLE);
		if (netMode == NetworkMode::CLIENT) {
			enet_peer_send(serverPeer, 0, packet);
		}
		else if (netMode == NetworkMode::SERVER) {
			enet_host_broadcast(localHost, 0, packet);
		}

		c.control();

		BeginDrawing();
		ClearBackground(BLACK);
		
		c.draw();
		c.drawHUD(0, 0);

		otherPlayer.draw();
		otherPlayer.drawHUD(0, 600);

		EndDrawing();
	}

	//enet_deinitialize();
	CloseWindow();
	return EXIT_SUCCESS;
}