// Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _GAME_H
#define _GAME_H

#include "vector3.h"
#include "galaxy/SystemPath.h"
#include "Serializer.h"
#include "gameconsts.h"

class HyperspaceCloud;
class Player;
class ShipController;
class Space;

class Game {
public:
	// LoadGame and SaveGame throw exceptions on failure
	static Game *LoadGame(const std::string &filename);
	// XXX game arg should be const, and this should probably be a member function
	// (or LoadGame/SaveGame should be somewhere else entirely)
	static void SaveGame(const std::string &filename, Game *game);

	// start docked in station referenced by path
	Game(const SystemPath &path);

	// start at position relative to body referenced by path
	Game(const SystemPath &path, const vector3d &pos);

	// load game
	Game(Serializer::Reader &rd);

	~Game();

	// save game
	void Serialize(Serializer::Writer &wr);

	// various game states
	bool IsNormalSpace() const { return m_state == STATE_NORMAL; }
	bool IsHyperspace() const { return m_state == STATE_HYPERSPACE; }

	Space *GetSpace() const { return m_space.Get(); }
	double GetTime() const { return m_time; }
	Player *GetPlayer() const { return m_player.Get(); }

	// physics step
	void TimeStep(float step);

	// update time acceleration once per render frame
	// returns true if timeaccel was changed
	bool UpdateTimeAccel();

	// request switch to hyperspace
	void WantHyperspace();

	// hyperspace parameters. only meaningful when IsHyperspace() is true
	float GetHyperspaceProgress() const { return m_hyperspaceProgress; }
	double GetHyperspaceDuration() const { return m_hyperspaceDuration; }
	double GetHyperspaceEndTime() const { return m_hyperspaceEndTime; }
	double GetHyperspaceArrivalProbability() const;

	void SetHyperspaceAllowInAtmosphere(bool allowInAtmosphere) {
		m_hyperspaceJumpOptions.allowInAtmosphere = allowInAtmosphere;
	}
	void SetHyperspaceMinTerrainDistance(double minTerrainDistance) {
		m_hyperspaceJumpOptions.minTerrainDistance = minTerrainDistance;
	}
	void SetHyperspaceMinStationDistance(double minStationDistance) {
		m_hyperspaceJumpOptions.minStationDistance = minStationDistance;
	}
	bool IsHyperspaceAllowedInAtmosphere() const { return m_hyperspaceJumpOptions.allowInAtmosphere; }
	double GetHyperspaceMinTerrainDistance() const { return m_hyperspaceJumpOptions.minTerrainDistance; }
	double GetHyperspaceMinStationDistance() const { return m_hyperspaceJumpOptions.minStationDistance; }

	enum TimeAccel {
		TIMEACCEL_PAUSED,
		TIMEACCEL_1X,
		TIMEACCEL_10X,
		TIMEACCEL_100X,
		TIMEACCEL_1000X,
		TIMEACCEL_10000X,
		TIMEACCEL_HYPERSPACE
    };

	void SetTimeAccel(TimeAccel t);
	void RequestTimeAccel(TimeAccel t, bool force = false);

	TimeAccel GetTimeAccel() const { return m_timeAccel; }
	TimeAccel GetRequestedTimeAccel() const { return m_requestedTimeAccel; }
	bool IsPaused() const { return m_timeAccel == TIMEACCEL_PAUSED; }

	float GetTimeAccelRate() const { return s_timeAccelRates[m_timeAccel]; }
	float GetInvTimeAccelRate() const { return s_timeInvAccelRates[m_timeAccel]; }

	float GetTimeStep() const { return s_timeAccelRates[m_timeAccel]*(1.0f/PHYSICS_HZ); }

private:
	void CreateViews();
	void LoadViews(Serializer::Reader &rd);
	void DestroyViews();

	void SwitchToHyperspace();
	void SwitchToNormalSpace();

	ScopedPtr<Space> m_space;
	double m_time;

	ScopedPtr<Player> m_player;

	enum State {
		STATE_NORMAL,
		STATE_HYPERSPACE,
	};
	State m_state;

	bool m_wantHyperspace;

	std::list<HyperspaceCloud*> m_hyperspaceClouds;
	SystemPath m_hyperspaceSource;
	double m_hyperspaceProgress;
	double m_hyperspaceDuration;
	double m_hyperspaceEndTime;

	struct HyperspaceJumpOptions {
		bool allowInAtmosphere;
		double minTerrainDistance;		// in metres
		double minStationDistance;		// in metres

		HyperspaceJumpOptions()
			: allowInAtmosphere(false), minTerrainDistance(15000.0), minStationDistance(15000.0) {}
	};
	HyperspaceJumpOptions m_hyperspaceJumpOptions;

	TimeAccel m_timeAccel;
	TimeAccel m_requestedTimeAccel;
	bool m_forceTimeAccel;

	static const float s_timeAccelRates[];
	static const float s_timeInvAccelRates[];
};

#endif
