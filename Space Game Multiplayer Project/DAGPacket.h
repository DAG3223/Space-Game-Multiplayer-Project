#include <string>
#include <sstream>
#include <vector>
#include <enet/enet.h>

class DAGPacket {
public:
	DAGPacket() {}

	DAGPacket(ENetPacket* inPacket) {
		const char* cData = (const char*)inPacket->data;
		data = cData;

		std::string line{};
		std::stringstream reader(data);

		while (reader >> line) {
			dataArr.emplace_back(line);
			line.clear();
		}
	}

	template <typename Ty>
	void append(Ty val) {
		if (data.empty()) {
			data.append(std::to_string(val));
		}
		else {
			data.push_back(' ');
			data.append(std::to_string(val));
		}
	}

	void append(const char* txt) {
		if (data.empty()) {
			data.append(txt);
		}
		else {
			data.push_back(' ');
			data.append(txt);
		}
	}

	ENetPacket* makePacket(enet_uint32 packetFlags) {
		return enet_packet_create(data.c_str(), strlen(data.c_str()) + 1, packetFlags);
	}

	const std::string& get_data() {
		return data;
	}

	const std::vector<std::string>& get_data_array() {
		return dataArr;
	}


private:
	std::string data{};
	std::vector<std::string> dataArr{};
};
