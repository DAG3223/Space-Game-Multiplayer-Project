#pragma once
#include "raylib.h"

class GUIButton {
public:
	GUIButton() {}

	GUIButton(Vector2 pos, const char* txt, int txtSz, int vPad, int hPad, Color bgClr, Color fgClr, bool enabled) {
		hitbox.x = pos.x;
		hitbox.y = pos.y;
		
		text = txt;

		textSize = txtSz;

		verticalPadding = static_cast<float>(vPad);
		horizontalPadding = static_cast<float>(hPad);

		updateRec();

		this->bgClr = bgClr;
		this->fgClr = fgClr;

		this->enabled = enabled;
	}

	void init(Vector2 pos, const char* txt, int txtSz, int vPad, int hPad, Color bgClr, Color fgClr, bool enabled){
		hitbox.x = pos.x;
		hitbox.y = pos.y;

		text = txt;

		textSize = txtSz;

		verticalPadding = static_cast<float>(vPad);
		horizontalPadding = static_cast<float>(hPad);

		updateRec();

		this->bgClr = bgClr;
		this->fgClr = fgClr;

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
		DrawRectangleRec(hitbox, bgClr);
		DrawText(text, static_cast<int>(hitbox.x + horizontalPadding), static_cast<int>(hitbox.y + verticalPadding), textSize, fgClr);
	}

	void set_pos(Vector2 pos) {
		hitbox.x = pos.x;
		hitbox.y = pos.y;
		updateRec();
	}

	void set_text(const char* txt) {
		text = txt;
		updateRec();
	}

	void set_textSize(int txtSz) {
		textSize = txtSz;
		updateRec();
	}

	void set_hPad(float padding) {
		horizontalPadding = padding;
		updateRec();
	}

	void set_vPad(float padding) {
		verticalPadding = padding;
		updateRec();
	}

	void set_bgClr(Color clr) {
		bgClr = clr;
	}

	void set_fgClr(Color clr) {
		fgClr = clr;
	}
private:
	Rectangle hitbox{ 0, 0, 0, 0 };
	float verticalPadding{};
	float horizontalPadding{};
	const char* text{};
	int textSize{};
	Color bgClr{ RED };
	Color fgClr{ BLACK };
	bool enabled{};

	void updateRec() {
		hitbox.width = 2 * horizontalPadding + MeasureText(text, textSize);
		hitbox.height = 2 * verticalPadding + textSize;
	}
};