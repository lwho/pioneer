-- Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

--
-- Class: FlightLog
--
-- A flight log, containing the last systems and stations visited by the
-- player. Can be used by scripts to find out where the player has been
-- recently.

local Game = import("Game")
local Event = import("Event")
local Format = import("Format")
local Serializer = import("Serializer")
local ShipDef = import("ShipDef")

-- default values (private)
local FlightLogSystemQueueLength = 1000
local FlightLogStationQueueLength = 1000

-- private data - the log itself
local FlightLogSystem = {}
local FlightLogStation = {}

local FlightLog

-- CompleteLog
--
-- List of entries:
-- 1) event CLASS
-- 2) EVENT
-- 3) time
-- 4) system path where the event happened (tuple of { path, name } )
-- 5) money
-- 6-x) additional data depending on event
--
-- CLASS 'career'
--   EVENT 'startedCareer': Start of game
--     6) ship type
--     7) ship name
--   EVENT 'rip': RIP old bean
--     6) destroyed by
-- CLASS 'travel'
--   EVENT 'enterSystem': System arrival (hyperjump end)
--   EVENT 'leaveSystem': System departure (hyperjump start)
--   EVENT 'dock': Docking station
--   EVENT 'undock': Undocking station
--   EVENT 'land': Landing on a surface outside of a station
--   EVENT 'takeoff': Starting from surface outside of a station
-- CLASS 'ship'
--   EVENT 'switchShip': Changing to a new ship
--     6) New ship type
--     7) New ship identity
-- CLASS 'mission'
--   6) Mission type
--   7) Customer
--   8) Destination
--   9) Due
--   10) Reward
--   EVENT 'newMission':
--   EVENT 'achievedMission':
--   EVENT 'failedMission':
-- TODO
  -- CLASS 'fight'
  --   EVENT 'fightStart'
  --     6) List of oponents
  --   EVENT 'fightEnd'
  -- CLASS 'cargo'
local CompleteLog = {}

FlightLog = {

--
-- Group: Methods
--

--
-- Method: GetSystemPaths
--
-- Returns an iterator returning a SystemPath object for each system visited
-- by the player, backwards in turn, starting with the most recent. If count
-- is specified, returns no more than that many systems.
--
-- > iterator = FlightLog.GetSystemPaths(count)
--
-- Parameters:
--
--   count - Optional. The maximum number of systems to return.
--
-- Return:
--
--   iterator - A function which will generate the paths from the log, returning
--              one each time it is called until it runs out, after which it
--              returns nil. It also returns, as secondary and tertiary values,
--              the game times at shich the system was entered and left.
--
-- Example:
--
-- Print the names and departure times of the last five systems visited by
-- the player
--
-- > for systemp,arrtime,deptime in FlightLog.GetSystemPaths(5) do
-- >   print(systemp:GetStarSystem().name, Format.Date(deptime))
-- > end

	GetSystemPaths = function (maximum)
		local counter = 0
		local maximum = maximum or FlightLogSystemQueueLength
		return function ()
			if counter < maximum then
				counter = counter + 1
				if FlightLogSystem[counter] then
					return FlightLogSystem[counter][1],
				    	   FlightLogSystem[counter][2],
				    	   FlightLogSystem[counter][3]
				end
			end
			return nil, nil, nil
		end
	end,

--
-- Method: GetStationPaths
--
-- Returns an iterator returning a SystemPath object for each station visited
-- by the player, backwards in turn, starting with the most recent. If count
-- is specified, returns no more than that many stations.
--
-- > iterator = FlightLog.GetStationPaths(count)
--
-- Parameters:
--
--   count - Optional. The maximum number of systems to return.
--
-- Return:
--
--   iterator - A function which will generate the paths from the log, returning
--              one each time it is called until it runs out, after which it
--              returns nil. It also returns, as a secondary value, the game
--              time at which the player undocked.
--
-- Example:
--
-- Print the names and departure times of the last five stations visited by
-- the player
--
-- > for systemp, deptime in FlightLog.GetStationPaths(5) do
-- >   print(systemp:GetSystemBody().name, Format.Date(deptime))
-- > end

	GetStationPaths = function (maximum)
		local counter = 0
		local maximum = maximum or FlightLogStationQueueLength
		return function ()
			if counter < maximum then
				counter = counter + 1
				if FlightLogStation[counter] then
					return FlightLogStation[counter][1],
				    	   FlightLogStation[counter][2]
				end
			end
			return nil, nil
		end
	end,

--
-- Method: GetPreviousSystemPath
--
-- Returns a SystemPath object that points to the star system where the
-- player was before jumping to this one. If none is on record (such as
-- before any hyperjumps have been made) it returns nil.
--
-- > path = FlightLog.GetPreviousSystemPath()
--
-- Return:
--
--   path - a SystemPath object
--
-- Availability:
--
--   alpha 20
--
-- Status:
--
--   experimental
--

	GetPreviousSystemPath = function ()
		if FlightLogSystem[2] then
			return FlightLogSystem[2][1]
		else return nil end
	end,

--
-- Method: GetPreviousStationPath
--
-- Returns a SystemPath object that points to the starport most recently
-- visited. If the player is currently docked, then the starport prior to
-- the present one (which might be the same one, if the player launches
-- and lands in the same port). If none is on record (such as before the
-- player has ever launched) it returns nil.
--
-- > path = FlightLog.GetPreviousStationPath()
--
-- Return:
--
--   path - a SystemPath object
--
-- Availability:
--
--   alpha 20
--
-- Status:
--
--   experimental
--

	GetPreviousStationPath = function ()
		if FlightLogStation[1] then
			return FlightLogStation[1][1]
		else return nil end
	end,

--
-- Method: GetLog
--
-- Returns an iterator returning a log entry, backwards in turn, starting with the most recent. If count
-- is specified, returns no more than that many stations.
--
-- iterator = FlightLog.GetLog(count)
--
-- Parameters:
--
--   count - Optional. The maximum number of systems to return.
--
-- Return:
--
--   iterator - A function which will generate the entries from the log, returning
--              one each time it is called until it runs out, after which it
--              returns nil.
--

	GetLog = function (maximum)
		local counter = 0
		return function ()
			if maximum == nil or counter < maximum then
				counter = counter + 1
				return CompleteLog[counter]
			end
			return nil
		end
	end,

	NewMission = function (mission)
		local location = Game.player:GetDockedWith()
		if location then
			location = location.path
		else
			location = Game.system.path
		end
		table.insert(CompleteLog,1,{'mission', 'newMission', Game.time, location, Game.player:GetMoney(), mission:GetTypeDescription(), mission.client.name, mission.location, mission.due, mission.reward })
	end,

	RemoveMission = function (mission, failed)
		local location = Game.player:GetDockedWith()
		if location then
			location = location.path
		else
			location = Game.system.path
		end
		if failed then
			table.insert(CompleteLog,1,{'mission', 'failedMission', Game.time, location, Game.player:GetMoney(), mission:GetTypeDescription(), mission.client.name, mission.location, mission.due, mission.reward })
		else
			table.insert(CompleteLog,1,{'mission', 'achievedMission', Game.time, location, Game.player:GetMoney(), mission:GetTypeDescription(), mission.client.name, mission.location, mission.due, mission.reward })
		end
	end
}

-- LOGGING

-- onLeaveSystem
local AddSystemDepartureToLog = function (ship)
	if not ship:IsPlayer() then return end
	FlightLogSystem[1][3] = Game.time
	while #FlightLogSystem > FlightLogSystemQueueLength do
		table.remove(FlightLogSystem,FlightLogSystemQueueLength + 1)
	end
	table.insert(CompleteLog,1,{'travel', 'leaveSystem', Game.time, Game.system.path, Game.player:GetMoney() })
end

-- onEnterSystem
local AddSystemArrivalToLog = function (ship)
	if not ship:IsPlayer() then return end
	table.insert(FlightLogSystem,1,{Game.system.path,Game.time,nil})
	while #FlightLogSystem > FlightLogSystemQueueLength do
		table.remove(FlightLogSystem,FlightLogSystemQueueLength + 1)
	end
	table.insert(CompleteLog,1,{'travel', 'enterSystem', Game.time, Game.system.path, Game.player:GetMoney() })
end

-- onSystemExplored
local SystemExplored = function (system)
	table.insert(CompleteLog,1,{'exploration', 'exploredSystem', Game.time, system.path, Game.player:GetMoney() })
end

-- onShipUndocked
local AddStationToLog = function (ship, station)
	if not ship:IsPlayer() then return end
	table.insert(FlightLogStation,1,{station.path,Game.time})
	while #FlightLogStation > FlightLogStationQueueLength do
		table.remove(FlightLogStation,FlightLogStationQueueLength + 1)
	end
	table.insert(CompleteLog,1,{'travel', 'undock', Game.time, station.path, Game.player:GetMoney() })
end

-- onShipDocked
local ShipDocked = function (ship, station)
	if not ship:IsPlayer() then return end
	table.insert(CompleteLog,1,{'travel', 'dock', Game.time, station.path, Game.player:GetMoney() })
end

-- onShipLanded
local ShipLanded = function (ship, body)
	if not ship:IsPlayer() then return end
	table.insert(CompleteLog,1,{'travel', 'land', Game.time, body.path, Game.player:GetMoney() })
end

-- onShipTakeOff
local ShipTakeoff = function (ship, body)
	if not ship:IsPlayer() then return end
	table.insert(CompleteLog,1,{'travel', 'takeoff', Game.time, body.path, Game.player:GetMoney() })
end

-- onShipTypeChanged
local ShipSwitched = function (ship)
	if not ship:IsPlayer() then return end
	local shipType = ShipDef[ship.shipId]
	local location = Game.player:GetDockedWith()
	if location then
		location = location.path
	else
		location = Game.system.path
	end
	table.insert(CompleteLog,1,{'ship', 'switchShip', Game.time, location, Game.player:GetMoney(), shipType.name, ship.label })
end

-- onShipDestroyed
local RestInPeace = function (ship, attacker)
	if not ship:IsPlayer() then return end
	local location = Game.player:GetDockedWith()
	if location then
		location = location.path
	else
		location = Game.system.path
	end
	table.insert(CompleteLog,1,{'career', 'rip', Game.time, location, Game.player:GetMoney(), attacker.label })
end

-- LOADING AND SAVING

local loaded_data

local onGameStart = function ()
	if loaded_data then
		FlightLogSystem = loaded_data.System
		FlightLogStation = loaded_data.Station
		if loaded_data.Complete then
			CompleteLog = loaded_data.Complete
		else
			CompleteLog = {}
		end
	else
		FlightLogSystem = {}
		FlightLogStation = {}
		CompleteLog = {}
		table.insert(FlightLogSystem,1,{Game.system.path,nil,nil})
		local location = Game.player:GetDockedWith()
		if location then
			location = location.path
		else
			location = Game.system.path
		end
		local shipType = ShipDef[Game.player.shipId]
		table.insert(CompleteLog,1,{'career', 'startedCareer', Game.time, location, Game.player:GetMoney(), shipType.name, Game.player.label })
	end
	loaded_data = nil
end

local serialize = function ()
    return { System = FlightLogSystem, Station = FlightLogStation, Complete = CompleteLog }
end

local unserialize = function (data)
    loaded_data = data
end

Event.Register("onEnterSystem", AddSystemArrivalToLog)
Event.Register("onLeaveSystem", AddSystemDepartureToLog)
Event.Register("onSystemExplored", SystemExplored)
Event.Register("onShipDocked", ShipDocked)
Event.Register("onShipUndocked", AddStationToLog)
Event.Register("onShipLanded", ShipLanded)
Event.Register("onShipTakeOff", ShipTakeoff)
Event.Register("onShipTypeChanged", ShipSwitched)
Event.Register("onGameStart", onGameStart)
Event.Register("onShipDestroyed", RestInPeace)
Serializer:Register("FlightLog", serialize, unserialize)

return FlightLog
