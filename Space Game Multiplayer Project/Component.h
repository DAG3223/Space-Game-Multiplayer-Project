#pragma once
#include "raylib.h"


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
			acceleration += 32;
		}
		if (IsKeyDown(KeyboardKey::KEY_S)) {
			acceleration -= 32;
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
		DrawCircle(static_cast<int>(hitbox.x), static_cast<int>(hitbox.y), 2.0f, RED);
		DrawLine(static_cast<int>(display.x), static_cast<int>(display.y), static_cast<int>(display.x + 30 * cosf(angle)), static_cast<int>(display.y + 30 * sinf(angle)), RED);
	}

	void drawHUD(int x, int y) {
		DrawText(TextFormat("HP: %d", hp), x, y, 20, DARKGREEN);

		DrawText(TextFormat("a: %3.1f px/s^2", acceleration), x, y + 40, 10, WHITE);
		DrawText(TextFormat("v: %4.1f / %4.1f px/s", velocity, maxVelocity), x, y + 60, 10, WHITE);
		DrawText(TextFormat("x: %.1f, y: %.1f", hitbox.x, hitbox.y), 10 + x, y + 80, 10, WHITE);
		DrawText(TextFormat("angle: %.2f", fmodf(angle, 2 * PI)), 10 + x, y + 100, 10, WHITE);
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

	void damage(int dmg) {
		hp -= dmg;
	}

	int get_hp() {
		return hp;
	}

	void set_hp(int HP) {
		hp = HP;
	}
private:
	Rectangle hitbox{};
	float angle{};
	float velocity{};
	float maxVelocity{}; //arbitrary
	float acceleration{};

	int hp{1000};
};