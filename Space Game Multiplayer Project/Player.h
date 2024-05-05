#pragma once
#include "raylib.h"
#include "Utils.h"

#include "enetfix.h"
#include "DAGPacket.h"

class Player {
public:
	Player() {}

	void move() {
		int dir = IsKeyDown(KeyboardKey::KEY_UP) - IsKeyDown(KeyboardKey::KEY_DOWN);
		int angleDir = IsKeyDown(KeyboardKey::KEY_RIGHT) - IsKeyDown(KeyboardKey::KEY_LEFT);

		angle += angleDir * perSecond(PI / 2.0f);

		hitbox.x += dir * perSecond(speed) * cosf(angle);
		hitbox.y += dir * perSecond(speed) * sinf(angle);
	}

	void draw() {
		Rectangle display = { hitbox.x + hitbox.width / 2.0f, hitbox.y + hitbox.height / 2.0f, hitbox.width, hitbox.height }; //display oriented inside hitbox
		Vector2 center = { hitbox.width / 2.0f, hitbox.height / 2.0f }; //offset from origin of hitbox
		DrawRectanglePro(display, center, RAD2DEG * angle, RED);
	}

	//GETTERS
	//------------------------------
	const Rectangle& get_hitbox() {
		return hitbox;
	}

	float get_speed() {
		return speed;
	}

	float get_angle() {
		return angle;
	}

	int get_hp() {
		return hp;
	}

	int get_client_id() {
		return client_id;
	}
	//------------------------------


	//SETTERS
	//------------------------------
	void set_hitbox(const Rectangle& value) {
		hitbox = value;
	}

	void set_speed(float value) {
		speed = value;
	}

	void set_angle(float value) {
		angle = value;
	}

	void set_hp(int value) {
		hp = value;
	}

	void set_client_id(int value) {
		client_id = value;
	}
	//------------------------------

	//NETWORKING
	//------------------------------
	DAGPacket makeDAGPacket() {
		DAGPacket packet;
		packet.append(TextFormat("%.1f", hitbox.x));
		packet.append(TextFormat("%.1f", hitbox.y));
		packet.append(TextFormat("%.2f", fmodf(angle, 2 * PI)));

		return packet;
	}


	//------------------------------
private:
	Rectangle hitbox{ 0,0,16,16 };
	float speed{};
	float angle{};
	int hp{};
	int client_id{};
};