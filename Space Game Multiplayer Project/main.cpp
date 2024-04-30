#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <list>
#include <raylib.h>
#include <raymath.h>
#include <sstream>

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
		//DrawRectangleLinesEx(hitbox, 2.0f, RED);
		//DrawCircle(hitbox.x, hitbox.y, 2.0f, RED);
		//DrawLine(display.x, display.y, display.x + 30 * cosf(angle), display.y + 30 * sinf(angle), RED);
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

//ship controlled by a player
class Ship {
public:
	Ship() {

	}

	void drawHUD(int x, int y) {
		DrawText(TextFormat("a: %3.1f px/s^2", acceleration), 10 + x, 10 + y, 10, WHITE);
		DrawText(TextFormat("v: %4.1f / %4.1f px/s", velocity, maxVelocity), 10 + x, 30 + y, 10, WHITE);
		DrawText(TextFormat("x: %.1f, y: %.1f", component->get_hitbox().x, component->get_hitbox().x), 10 + x, 50 + y, 10, WHITE);
		DrawText(TextFormat("angle: %.2f", angle), 10 + x, 70 + y, 10, WHITE);
	}

private:
	Component* component{};

	float angle{};
	float velocity{};
	float maxVelocity{}; //arbitrary
	float acceleration{};
	//std::list<Component> components;
};

enum class NetworkMode {
	SINGLEPLAYER = 0,
	CLIENT,
	SERVER
};

enum GAME_STATES {
	TITLE = 0,
	GAME = 1
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

class DAGPacket {
public:
	DAGPacket() {}

	DAGPacket(ENetPacket* inPacket) {
		const char* cData = (const char*)inPacket->data;
		data = cData;

		std::string line{};
		std::stringstream reader(data);

		while (reader >> line) {
			dataArr.emplace_back(line);
			line.clear();
		}
	}

	template <typename Ty>
	void append(Ty val) {
		if (data.empty()) {
			data.append(std::to_string(val));
		}
		else {
			data.push_back(' ');
			data.append(std::to_string(val));
		}
	}

	void append(const char* txt) {
		if (data.empty()) {
			data.append(txt);
		}
		else {
			data.push_back(' ');
			data.append(txt);
		}
	}

	ENetPacket* makePacket(enet_uint32 packetFlags) {
		return enet_packet_create(data.c_str(), strlen(data.c_str()) + 1, packetFlags);
	}

	const std::string& get_data() {
		return data;
	}

	const std::vector<std::string>& get_data_array() {
		return dataArr;
	}
private:
	std::string data{};
	std::vector<std::string> dataArr{};
};

class Button {
public:
	Button(Rectangle rec, Color clr, bool enabled) {
		hitbox = rec;
		color = clr;
		this->enabled = enabled;
	}

	bool clicked() {
		return CheckCollisionPointRec(GetMousePosition(), hitbox) && IsMouseButtonPressed(MouseButton::MOUSE_BUTTON_LEFT) && enabled;
	}

	void enable() {
		enabled = true;
	}

	void disable() {
		enabled = false;
	}

	bool isEnabled() {
		return enabled;
	}

	void draw() {
		if (!enabled) return;
		DrawRectangleRec(hitbox, color);
	}

private:
	Rectangle hitbox{ 0, 0, 10, 10 };
	Color color{ RED };
	bool enabled{};
};

int main() {
	initENet();

	InitWindow(800, 800, "Space Game Multiplayer Project");
	SetTargetFPS(60);

	std::string input_str{};
	NetworkMode netMode = NetworkMode::SINGLEPLAYER;

	ENetHost* localHost{};

	ENetAddress localAddress{};
	localAddress.host = ENET_HOST_ANY;
	localAddress.port = 7777;

	int game_state = GAME_STATES::TITLE;
	Button btn_singleplayer({ 10, 10, 50, 50 }, RED, true);
	Button btn_client({ 70, 10, 50, 50 }, GREEN, true);
	Button btn_server({ 130, 10, 50, 50 }, BLUE, true);
	
	//select net mode and init localHost
	while (!WindowShouldClose()) {
		if (btn_singleplayer.clicked()) {
			netMode = NetworkMode::SINGLEPLAYER;
			break;
		}

		if (btn_client.clicked()) {
			netMode = NetworkMode::CLIENT;
			localHost = enet_host_create(NULL, 1, 2, 0, 0);

			break;
		}

		if (btn_server.clicked()) {
			netMode = NetworkMode::SERVER;
			localHost = enet_host_create(&localAddress, 1, 2, 0, 0);

			break;
		}

		BeginDrawing();
		ClearBackground(WHITE);

		btn_singleplayer.draw();
		btn_client.draw();
		btn_server.draw();
		
		EndDrawing();
	}
	btn_singleplayer.disable();
	btn_client.disable();
	btn_server.disable();

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
	std::string addressStr{};
	//
	while (!WindowShouldClose() && netMode == NetworkMode::CLIENT) {
		ENetAddress targetAddr{};
		ENetEvent connectEvent{};

		int key = GetKeyPressed();
		char c = GetCharPressed();
		std::cout << (int)netMode << "\n";
		std::cout << addressStr << "\n";

		if (c <= '9' && c >= '0' || c == '.') {
			addressStr.push_back(c);
		}

		if (key == KeyboardKey::KEY_BACKSPACE) {
			if (addressStr.size() != 0) addressStr.pop_back();
		}

		if (key == KeyboardKey::KEY_ENTER) break;

		BeginDrawing();
		ClearBackground(WHITE);

		DrawText(addressStr.c_str(), 10, 10, 10, BLACK);

		EndDrawing();
	}
	
	if (netMode == NetworkMode::CLIENT) {
		ENetAddress targetAddr{};
		ENetEvent connectEvent{};

		/*std::string addressStr{};
		std::cout << "\nPlease enter an IPv4 address to connect to:\n\tAddress: ";
		std::cin >> addressStr;*/
		
		enet_address_set_host(&targetAddr, addressStr.c_str());
		targetAddr.port = 7777;

		serverPeer = enet_host_connect(localHost, &targetAddr, 2, 0);
		if (serverPeer == nullptr) {
			fprintf(stderr, "No available peers for initiating an ENET connection.\n");
			exit(EXIT_FAILURE);
		}

		//try for 30 seconds to connect
		//exit if failure
		if (enet_host_service(localHost, &connectEvent, 30000) > 0 && connectEvent.type == ENET_EVENT_TYPE_CONNECT) {
			printf("Successful connection to server!\n");
		}
		else {
			enet_peer_reset(serverPeer);

			fprintf(stderr, "Server connection unsuccessful. Closing...\n");
			exit(EXIT_SUCCESS);
		}
	}
	
	//end client/server init

	

	//---------------------------------------------------------------------------//
	//---------------------------------------------------------------------------//
	// GAME CODE STARTS HERE
	//---------------------------------------------------------------------------//
	//---------------------------------------------------------------------------//
	
	//std::vector<Component*> players;
	//players.emplace_back(480.0f);

	Component c(480.0f);
	Component otherPlayer(480.0f);

	ENetEvent netEvent{};
	while (!WindowShouldClose()) {
		if (netMode != NetworkMode::SINGLEPLAYER) {
			while (enet_host_service(localHost, &netEvent, 0)) {
				
				switch (netEvent.type) {
					case ENET_EVENT_TYPE_CONNECT: {
						if (netMode == NetworkMode::SERVER) {
							//ENetPacket* packet = enet_packet_create("Welcome to the server!", 23, ENET_PACKET_FLAG_RELIABLE);
							DAGPacket packet;
							packet.append("Welcome to the server!");
						
							enet_host_broadcast(localHost, 0, packet.makePacket(ENET_PACKET_FLAG_RELIABLE));
						}
						break;
					}
					case ENET_EVENT_TYPE_RECEIVE: {
						DAGPacket rPacket(netEvent.packet);
						//std::cout << rPacket.get_data() << "\n";
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

			//get x and y of localHost's ship, prepare as packet to send to peers
			float sX = c.get_hitbox().x;
			float sY = c.get_hitbox().y;
			float sAngle = c.get_angle();

			DAGPacket sPacket;
			sPacket.append(sX);
			sPacket.append(sY);
			sPacket.append(sAngle);

			ENetPacket* packet = sPacket.makePacket(ENET_PACKET_FLAG_RELIABLE);
			switch (netMode) {
			case NetworkMode::CLIENT:
				enet_peer_send(serverPeer, 0, packet);
				break;
			case NetworkMode::SERVER:
				enet_host_broadcast(localHost, 0, packet);
				break;
			}
		}
		

		c.control();

		BeginDrawing();
		ClearBackground(BLACK);
		
		c.draw();
		c.drawHUD(0, 0);

		if (netMode != NetworkMode::SINGLEPLAYER) {
			otherPlayer.draw();
			otherPlayer.drawHUD(0, 600);
		}
		

		EndDrawing();
	}

	enet_deinitialize();        
	CloseWindow();
	return EXIT_SUCCESS;
}