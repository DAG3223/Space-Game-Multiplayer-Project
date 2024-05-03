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

#include "DAGPacket.h"

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
		DrawText(TextFormat("x: %.1f, y: %.1f", component->get_hitbox().x, component->get_hitbox().x), 10 + x, 50 + y, 10, WHITE);
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

class TextField {
public:
	template <typename _Pr>
	void append_if(char c, _Pr _Pred) {
		if (_Pred()) {
			append(c);
		}
	}

	void append(char c) {
		s.push_back(c);
	}

	void pop_back() {
		if (s.size() == 0) return;
		s.pop_back();
	}

	std::string flush() {
		std::string t = s;
		s.clear();

		return t;
	}

	const std::string& data() {
		return s;
	}

	void clear() {
		s.clear();
	}

	void draw(int x, int y) {
		DrawText(s.c_str(), x, y, 10, BLACK);
	}

	void set_focus(bool flag) {
		focused = flag;
	}

	bool get_focused() {
		return focused;
	}
private:
	std::string s{};
	bool focused{};
};

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

class LocalHost {
public:
	ENetHost* get_localHost() {
		return localHost;
	}

protected:
	LocalHost(size_t connections = 1, size_t channels = 2, enet_uint32 limit_in = 0, enet_uint32 limit_out = 0) {
		localAddress.host = ENET_HOST_ANY;
		localAddress.port = 7777;
	}

	virtual ~LocalHost() { if (localHost != nullptr) enet_host_destroy(localHost); }

public:
	ENetAddress localAddress{};
	ENetHost* localHost{};
};

class LocalClient : public LocalHost {
public:
	LocalClient(size_t connections = 1, size_t channels = 2, enet_uint32 limit_in = 0, enet_uint32 limit_out = 0) :
	LocalHost (connections, channels, limit_in, limit_out) {
		localHost = enet_host_create(nullptr, connections, channels, limit_in, limit_out);
		std::cout << localHost << "\n";
	}

	~LocalClient() {
		//enet_host_destroy(localHost);
	}

	bool tryConnect(const char* address = "127.0.0.1", enet_uint16 port = 7777, int ms = 5000) {
		enet_address_set_host(&serverAddress, address);
		serverAddress.port = port;

		serverPeer = enet_host_connect(localHost, &serverAddress, 2, 0);
		std::cout << "Connecting " << localHost << "\n";
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

		while (enet_host_service(localHost, &disconnectEvent, ms) > 0){
			switch (disconnectEvent.type){
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
private:
	ENetAddress serverAddress{};
	ENetPeer* serverPeer{};
};

class LocalServer : public LocalHost {
public:
	LocalServer(size_t connections = 1, size_t channels = 2, enet_uint32 limit_in = 0, enet_uint32 limit_out = 0) :
	LocalHost(connections, channels, limit_in, limit_out) {
		localHost = enet_host_create(&localAddress, connections, channels, limit_in, limit_out);
	}

	~LocalServer() {
		enet_host_destroy(localHost);
	}

	void sendAll(ENetPacket* packet) {
		enet_host_broadcast(localHost, 0, packet);
	}
private:

};

int main() {
	initENet();

	InitWindow(800, 800, "Space Game Multiplayer Project");
	SetTargetFPS(60);

	NetworkMode netMode = NetworkMode::SINGLEPLAYER;

	std::unique_ptr<LocalClient> DAGLocalClient{};
	std::unique_ptr<LocalServer> DAGLocalServer{};

	Button btn_singleplayer({ 10, 10, 50, 50 }, RED, true);
	Button btn_client({ 70, 10, 50, 50 }, GREEN, true);
	Button btn_server({ 130, 10, 50, 50 }, BLUE, true);
	
	//select net mode and init localHost
label1_menu:
	DAGLocalClient.reset();
	DAGLocalServer.reset();
	
	btn_singleplayer.enable();
	btn_client.enable();
	btn_server.enable();
	while (!WindowShouldClose()) {
		if (btn_singleplayer.clicked()) {
			netMode = NetworkMode::SINGLEPLAYER;
			break;
		}

		if (btn_client.clicked()) {
			netMode = NetworkMode::CLIENT;
			DAGLocalClient = std::make_unique<LocalClient>(1, 2, 0, 0);

			break;
		}

		if (btn_server.clicked()) {
			netMode = NetworkMode::SERVER;
			DAGLocalServer = std::make_unique<LocalServer>(4, 2, 0, 0);

			break;
		}

		BeginDrawing();
		ClearBackground(WHITE);

		btn_singleplayer.draw();
		btn_client.draw();
		btn_server.draw();
		
		EndDrawing();
	}
	if (WindowShouldClose()) {
		CloseWindow();
	}
	btn_singleplayer.disable();
	btn_client.disable();
	btn_server.disable();

	//localHost == nullptr
	switch (netMode) {
	case NetworkMode::CLIENT:
		if (DAGLocalClient == nullptr) {
			fprintf(stderr, "Failed to initialize localHost for networking.\n");
			exit(EXIT_FAILURE);
		}
		printf("localHost initialized properly.\n");
		break;
	case NetworkMode::SERVER:
		if (DAGLocalServer == nullptr) {
			fprintf(stderr, "Failed to initialize localHost for networking.\n");
			exit(EXIT_FAILURE);
		}
		printf("localHost initialized properly.\n");
		break;
	case NetworkMode::SINGLEPLAYER:
	default:
		printf("localHost initialized properly.\n");
	}

	/*if (DAGLocalHost == nullptr && netMode != NetworkMode::SINGLEPLAYER) {
		fprintf(stderr, "Failed to initialize localHost for networking.\n");
		exit(EXIT_FAILURE);
	}
	else {
		printf("localHost initialized properly.\n");
	}*/
	//done initializing localHost

	//connection handling
	//TODO: make code return to input page if connection fails
	TextField addressField;

	while (!WindowShouldClose() && netMode == NetworkMode::CLIENT) {
		int key = GetKeyPressed();
		
		if (key == KeyboardKey::KEY_BACKSPACE) {
			addressField.pop_back();
		}

		if (key == KeyboardKey::KEY_ENTER) {
			break;
		}

		char c = (char)min(key, SCHAR_MAX);
		addressField.append_if(c, [c]() { return c >= KeyboardKey::KEY_ZERO && c <= KeyboardKey::KEY_NINE || c == KeyboardKey::KEY_PERIOD; });

		BeginDrawing();
		ClearBackground(WHITE);

		addressField.draw(10, 10);

		EndDrawing();
	}
	if (WindowShouldClose()) {
		CloseWindow();
	}
	
	//connect client to server
	if (netMode == NetworkMode::CLIENT) {
		if (DAGLocalClient->tryConnect(addressField.flush().c_str(), 7777, 5000)) {
			printf("Successful connection to server!\n");
		}
		else {
			fprintf(stderr, "Server connection unsuccessful. Closing...\n");
			goto label1_menu;
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
		}
		case NetworkMode::SERVER: {
			while (enet_host_service(DAGLocalServer->get_localHost(), &netEvent, 0) > 0) {
				switch (netEvent.type) {
				case ENET_EVENT_TYPE_CONNECT: {
					if (netMode != NetworkMode::SERVER) break;

					DAGPacket packet;
					packet.append("Welcome to the server!");

					DAGLocalServer->sendAll(packet.makePacket(ENET_PACKET_FLAG_RELIABLE));

					//enet_host_broadcast(localHost, 0, packet.makePacket(ENET_PACKET_FLAG_RELIABLE));
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
				}
			}
		}
		}

		if (netMode != NetworkMode::SINGLEPLAYER) {
			//while (enet_host_service(LocallocalHost, &netEvent, 0) > 0) {
			//	switch (netEvent.type) {
			//	case ENET_EVENT_TYPE_CONNECT: {
			//		if (netMode != NetworkMode::SERVER) break;
			//		
			//		DAGPacket packet;
			//		packet.append("Welcome to the server!");

			//		DAGLocalServer->sendAll(packet.makePacket(ENET_PACKET_FLAG_RELIABLE));
			//		
			//		//enet_host_broadcast(localHost, 0, packet.makePacket(ENET_PACKET_FLAG_RELIABLE));
			//		break;
			//	}
			//	case ENET_EVENT_TYPE_RECEIVE: {
			//		DAGPacket rPacket(netEvent.packet);
			//		std::vector<std::string> rData = rPacket.get_data_array();

			//		float rX = std::atof(rData.at(0).c_str());
			//		float rY = std::atof(rData.at(1).c_str());
			//		float rA = std::atof(rData.at(2).c_str());

			//		otherPlayer.set_hitbox({ rX, rY, otherPlayer.get_hitbox().width, otherPlayer.get_hitbox().height });
			//		otherPlayer.set_angle(rA);

			//		enet_packet_destroy(netEvent.packet);
			//		break;
			//	}
			//	}
			//}

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