#pragma once

enum NetworkMessage {
	INFORM_PLAYER,
	ADD_PLAYER,
	UPDATE_PLAYER,
	UPDATE_PLAYER_LOSSES,
	REMOVE_PLAYER,
	ADD_PROJECTILE,
	REMOVE_PROJECTILE
};

enum class NetworkMode {
	SINGLEPLAYER = 0,
	CLIENT,
	SERVER
};