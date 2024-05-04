#pragma once
#include "raylib.h"
#include <string>

class GUIButton {
public:
	GUIButton() {}

	GUIButton(Vector2 pos, std::string txt, int txtSz, int vPad, int hPad, Color bgClr, Color fgClr, bool enabled) {
		set_pos(pos);
		set_text(txt);
		set_textSize(txtSz);
		set_bgClr(bgClr);
		set_fgClr(fgClr);
		set_vPad(vPad);
		set_hPad(hPad);
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
		DrawText(text.c_str(), hitbox.x + horizontalPadding, hitbox.y + verticalPadding, textSize, fgClr);
	}

	void set_pos(Vector2 pos) {
		hitbox.x = pos.x;
		hitbox.y = pos.y;
		updateRec();
	}

	void set_text(std::string txt) {
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
	std::string text{};
	int textSize{};
	Color bgClr{ RED };
	Color fgClr{ BLACK };
	bool enabled{};

	void updateRec() {
		hitbox.width = 2 * horizontalPadding + MeasureText(text.c_str(), textSize);
		hitbox.height = 2 * verticalPadding + textSize;
	}
};