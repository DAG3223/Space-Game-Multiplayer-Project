#include <iostream>
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

float angleBetween(float x1, float y1, float x2, float y2) {
	return atan2f(y2 - y1, x2 - x1);
}

class Component {
public:
	Component() {

	}

	void control() {
		angle = angleBetween(hitbox.x + hitbox.width / 2.0f, hitbox.y + hitbox.height / 2.0f, GetMouseX(), GetMouseY());

		if (IsKeyDown(KeyboardKey::KEY_W)) {
			hitbox.y -= 3;
		}
		if (IsKeyDown(KeyboardKey::KEY_A)) {
			hitbox.x -= 3;
		}
		if (IsKeyDown(KeyboardKey::KEY_S)) {
			hitbox.y += 3;
		}
		if (IsKeyDown(KeyboardKey::KEY_D)) {
			hitbox.x += 3;
		}
	}

	void draw() {
		Rectangle display = { hitbox.x + hitbox.width / 2.0f, hitbox.y + hitbox.height / 2.0f, hitbox.width, hitbox.height };
		Vector2 center = { hitbox.width / 2.0f, hitbox.height / 2.0f };
		DrawRectanglePro(display, center, RAD2DEG * angle, WHITE);

		DrawRectangleLinesEx(hitbox, 2.0f, RED);
		DrawCircle(hitbox.x, hitbox.y, 2.0f, RED);
	}

private:
	Rectangle hitbox{};
	float angle;
};

int main() {
	/*if (enet_initialize() != 0) {
		fprintf(stderr, "uh oh! enet didn't initialize!\n");
		return EXIT_FAILURE;
	}
	atexit(enet_deinitialize);*/

	InitWindow(800, 800, "Space Game Multiplayer Project");
	SetTargetFPS(60);

	Component c;

	while (!WindowShouldClose()) {
		c.control();

		BeginDrawing();
		ClearBackground(BLACK);

		c.draw();

		EndDrawing();
	}

	//enet_deinitialize();
	CloseWindow();
	return EXIT_SUCCESS;
}