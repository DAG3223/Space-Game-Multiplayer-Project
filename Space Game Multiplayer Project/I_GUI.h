class I_GUI {
public:
	virtual ~I_GUI() {}

	virtual void init() = 0;
	virtual void update() = 0;
	virtual void deinit() = 0;
	virtual void draw() = 0;

	bool isInitialized() {
		return initialized;
	}
protected:
	bool initialized{};
};