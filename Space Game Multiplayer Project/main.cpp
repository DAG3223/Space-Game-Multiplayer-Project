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


int main() {
	/*if (enet_initialize() != 0) {
		fprintf(stderr, "uh oh! enet didn't initialize!\n");
		return EXIT_FAILURE;
	}
	atexit(enet_deinitialize);*/

	InitWindow(800, 800, "Space Game Multiplayer Project");
	SetTargetFPS(60);

	Rectangle hitbox{ 0.0f, 0.0f, 25.0f, 25.0f };
	Rectangle display{ 0.0f, 0.0f, 25.0f, 25.0f };

	while (!WindowShouldClose()) {
		if (IsKeyDown(KeyboardKey::KEY_W)) {
			hitbox.y -= 3;
			display.y -= 3;
		}
		if (IsKeyDown(KeyboardKey::KEY_A)) {
			hitbox.x -= 3;
			display.x -= 3;
		}
		if (IsKeyDown(KeyboardKey::KEY_S)) {
			hitbox.y += 3;
			display.y += 3;
		}
		if (IsKeyDown(KeyboardKey::KEY_D)) {
			hitbox.x += 3;
			display.x += 3;
		}


		BeginDrawing();
		ClearBackground(BLACK);

		DrawRectangleRec(display, WHITE);
		DrawRectangleLinesEx(hitbox, 2.0f, RED);
		DrawCircle(hitbox.x, hitbox.y, 2.0f, RED);

		EndDrawing();
	}

	//enet_deinitialize();
	CloseWindow();
	return EXIT_SUCCESS;
}