#include "raylib.h"
#include <string>

//class design
/*
* click to focus
* click off to defocus
* only input characters if focused
* only input characters if not at the character limit
* if char limit = 0, input is unlimited
* 
* expandToText flag
* if flag is set, expand field border when text goes past
* 
* 
*/


class GUITextField {
public:
	template <typename _Pr>
	void append_if(char c, _Pr _Pred) {
		if (_Pred()) {
			append(c);
		}
	}

	void append(char c) {
		if (!is_focused()) return;
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

	bool is_focused() {
		return focused;
	}

	void set_border() {

	}
private:
	std::string s{};
	bool focused{true};
	int charLimit{};
	Rectangle border{};
};