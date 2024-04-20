#include <iostream>
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
		DrawText(TextFormat("a: %3.1f px/s^2", acceleration), 10 + x, 10, 10 + y, WHITE);
		DrawText(TextFormat("v: %4.1f / %4.1f px/s", velocity, maxVelocity), 10 + x, 30 + y, 10, WHITE);
		DrawText(TextFormat("x: %.1f, y: %.1f", hitbox.x, hitbox.y), 10 + x, 50 + y, 10, WHITE);
		DrawText(TextFormat("angle: %.2f", angle), 10 + x, 70 + y, 10, WHITE);
	}


private:
	Rectangle hitbox{};
	float angle{};
	float velocity{};
	float maxVelocity{}; //arbitrary
	float acceleration{};
};

int main() {
	/*if (enet_initialize() != 0) {
		fprintf(stderr, "uh oh! enet didn't initialize!\n");
		return EXIT_FAILURE;
	}
	atexit(enet_deinitialize);
	std::cout << "ENET init successful.\n";*/

	InitWindow(800, 800, "Space Game Multiplayer Project");
	SetTargetFPS(60);

	Component c(480.0f);

	while (!WindowShouldClose()) {
		c.control();

		BeginDrawing();
		ClearBackground(BLACK);
		
		c.draw();
		c.drawHUD(0, 0);

		EndDrawing();
	}

	/*enet_deinitialize();*/
	CloseWindow();
	return EXIT_SUCCESS;
}