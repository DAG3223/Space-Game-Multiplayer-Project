#pragma once
#include <string>
#include <sstream>
#include <vector>
#include "enetfix.h"

class DAGPacket {
public:
	DAGPacket() {}

	DAGPacket(ENetPacket* inPacket) {
		const char* cData = reinterpret_cast<const char*>(inPacket->data);
		//printf("Initializing DAGPacket with data:\"%s\"\n", cData);
		//std::cout << "Initializing DAGPacket with data:\"" << cData << "\"\n";
		data = cData;

		std::string line{""};
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

	/*void appendf(float val, uint8_t precision) {
		append(TextFormat("%.1f", val));
	}*/
	/// <summary>
	/// Appends a header to the packet. 
	/// This header contains the sender of the packet, the type of message being sent, and the ID of the peer to be affected
	/// </summary>
	/// <param name="senderID">: ID of the client sending the data.</param>
	/// <param name="messageType">: Type of message being sent.</param>
	void appendHeader(int senderID, int messageType) {
		append(senderID);
		append(messageType);
		//append(affectedID);
	}

	ENetPacket* makePacket(enet_uint32 packetFlags) {
		//std::cout << "making packet: \"" << data << "\" size: " << strlen(data.c_str()) + 1 << "\n";
		return enet_packet_create(data.c_str(), strlen(data.c_str()) + 1, packetFlags);
	}

	const std::string& get_data() {
		return data;
	}

	const std::vector<std::string>& get_data_array() {
		return dataArr;
	}

	const char* get_word(int i) {
		return dataArr.at(i).c_str();
	}

	int get_int(int i) {
		return std::atoi(get_word(i));
	}

	float get_float(int i) {
		return static_cast<float>(std::atof(get_word(i)));
	}
private:
	std::string data{""};
	std::vector<std::string> dataArr{};
};
