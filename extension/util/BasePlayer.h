#pragma once

#include "BaseEntity.h"
#include <vector.h>
#include <const.h>

class BasePlayer: public BaseEntity {
public:
	BasePlayer(edict_t *ent): BaseEntity(ent) {
	}

	virtual ~BasePlayer() {
	}

	virtual int getPlayerClass() const {
		return -1;
	}

	virtual bool isOnLadder() const {
		int *moveType = getMoveType();
		return moveType != nullptr && (*moveType & 15) == MOVETYPE_LADDER;
	}

	int* getAmmo() const {
		return getPtr<int>("m_iAmmo");
	}

	Vector *getVelocity() const {
		return getPtr<Vector>("m_vecVelocity[0]");
	}
};
