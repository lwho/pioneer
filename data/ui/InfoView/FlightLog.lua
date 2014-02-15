-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

local Engine = import("Engine")
local FlightLog = import("FlightLog")
local Format = import("Format")
local Lang = import("Lang")
local SmartTable = import("ui/SmartTable")

local ui = Engine.ui
local l = Lang.GetResource("ui-core");

-- we keep LogList to remember players preferences
-- (now it is column he wants to sort by)
local LogList
local LogFilter = 0xffffffff
local flightLog = function (tabGroup)
	-- This Flight Log screen
	local function categoryID(type)
		if type == 'career' then
			return 0
		elseif type == 'ship' then
			return 1
		elseif type == 'travel' then
			return 2
		elseif type == 'mission' then
			return 3
		end
	end

	local function eventDescription(entry)
		local type = entry[1]
		local event = entry[2]
		if type == 'career' then
			if event == 'startedCareer' then
				return l.CAREER, l.STARTED_CAREER, l.SHIP_TYPE_ID:interp({type=entry[6], id=entry[7]})
			elseif event == 'rip' then
				return l.CAREER, l.DIED, l.DESTROYED_BY:interp({destroyer = entry[6]})
			end
		elseif type == 'travel' then
			if event == 'enterSystem' then
				return l.TRAVEL, l.ENTERED_SYSTEM
			elseif event == 'leaveSystem' then
				return l.TRAVEL, l.LEFT_SYSTEM
			elseif event == 'dock' then
				return l.TRAVEL, l.DOCKED_TO_STATION
			elseif event == 'undock' then
				return l.TRAVEL, l.UNDOCKED_FROM_STATION
			elseif event == 'land' then
				return l.TRAVEL, l.LANDED
			elseif event == 'takeoff' then
				return l.TRAVEL, l.STARTED
			end
		elseif type == 'ship' then
			if event == 'switchShip' then
				return l.SHIP, l.BOUGHT_SHIP, l.SHIP_TYPE_ID:interp({type=entry[6], id=entry[7]})
			end
		elseif type == 'mission' then
			local dest = entry[8]
			local destName
			if dest.bodyIndex then
				destName = string.format('%s, %s [%d,%d,%d]', dest:GetSystemBody().name, dest:GetStarSystem().name, dest.sectorX, dest.sectorY, dest.sectorZ)
			else
				destName = string.format('%s [%d,%d,%d]', dest:GetStarSystem().name, dest.sectorX, dest.sectorY, dest.sectorZ)
			end
			if event == 'newMission' then
				return l.MISSION, l.NEW_MISSION, l.MISSION_FOR_X_TO_Y:interp({missiontype=entry[6], customer=entry[7], dest=destName})
			elseif event == 'achievedMission' then
				return l.MISSION, l.MISSION_SUCCESSFUL, l.MISSION_FOR_X_YIELDED_Y:interp({missiontype=entry[6], customer=entry[7], reward=Format.Money(entry[10])})
			elseif event == 'failedMission' then
				return l.MISSION, l.MISSION_FAILED, l.MISSION_FOR_X_TO_Y:interp({missiontype=entry[6], customer=entry[7], dest=destName})
			end
		end
	end

	local function eventColor(entry)
		local type = entry[1]
		if type == 'career' then
			return { r = 1.0, g = 0.3, b = 0.3 }	-- light red
		elseif type == 'travel' then
			return { r = 0.0, g = 1.0, b = 0.2 }	-- green
		elseif type == 'ship' then
			return { r = 1.0, g = 1.0, b = 0.0 }	-- yellow
		elseif type == 'mission' then
			return { r = 0.5, g = 0.7, b = 1.0 }	-- light blue
		end
	end

	local function eventSortKey(event)
		if event == 'startedCareer' then
			return 0
		elseif event == 'rip' then
			return 0xffffffff
		elseif event == 'enterSystem' then
			return 100
		elseif event == 'dock' then
			return 300
		elseif event == 'land' then
			return 300
		elseif event == 'achievedMission' then
			return 400
		elseif event == 'failedMission' then
			return 450
		elseif event == 'switchShip' then
			return 500
		elseif event == 'newMission' then
			return 600
		elseif event == 'undock' then
			return 700
		elseif event == 'takeoff' then
			return 700
		elseif event == 'leaveSystem' then
			return 800
		end
		print(event)
		return 550
	end

	local LogScreen = ui:Expand()
	--return LogScreen:SetInnerWidget( ui:Label(t("No log.")) )

	local LogListWidget = ui:Margin(0)

	local function updateLogList()
		local rowspec = {4,2,5,6,2,12} -- 5 columns
		if LogList then
			LogList:Clear()
		else
			LogList = SmartTable.New(rowspec)
			LogList.sortCol = 1
		end

		-- setup headers
		local headers =
		{
			l.TIME,
			l.TYPE,
			l.EVENT,
			l.LOCATION,
			l.MONEY,
			l.INFORMATION
		}
		LogList:SetHeaders(headers)

		-- we're not happy with default sort function so we specify one by ourselves
		local function sortLog(logList)
			local col = logList.sortCol
			-- Newest first
			local function cmpByTime(a,b)
				if a.data[col] == b.data[col] then
					return eventSortKey(a.data[3]) >= eventSortKey(b.data[3])
				else
					return a.data[col] > b.data[col]
				end
			end
			local function cmpByLocation(a,b)
				if a.data[col].sectorX < b.data[col].sectorX then
					return true
				elseif a.data[col].sectorX > b.data[col].sectorX then
					return false
				else
					if a.data[col].sectorY < b.data[col].sectorY then
						return true
					elseif a.data[col].sectorY > b.data[col].sectorY then
						return false
					else
						if a.data[col].sectorZ < b.data[col].sectorZ then
							return true
						elseif a.data[col].sectorZ > b.data[col].sectorZ then
							return false
						else
							local starA = a.data[col]:GetStarSystem().name
							local starB = b.data[col]:GetStarSystem().name
							if starA < starB then
								return true
							elseif starA > starB then
								return false
							else
								if a.data[col].bodyIndex == b.data[col].bodyIndex then
									if a.data[1] == b.data[1] then
										return eventSortKey(a.data[3]) >= eventSortKey(b.data[3])
									else
										return a.data[1] > b.data[1]
									end
								elseif not a.data[col].bodyIndex then
									return true
								elseif not b.data[col].bodyIndex then
									return false
								else
									return a.data[col]:GetSystemBody().name < b.data[col]:GetSystemBody().name
								end
							end					
						end	
					end	
				end
			end
			local comparators =
			{ 	-- by column num
				[1]	= cmpByTime,
				[4] = cmpByLocation,
			}
			logList:defaultSortFunction(comparators[col])
		end
		LogList:SetSortFunction(sortLog)

		for entry in FlightLog.GetLog() do
			if bit32.extract(LogFilter, categoryID(entry[1])) == 1 then
				-- Format the location
				local location = entry[4]
				local locationName
				if location.bodyIndex then
					locationName = string.format('%s, %s [%d,%d,%d]', location:GetSystemBody().name, location:GetStarSystem().name, location.sectorX, location.sectorY, location.sectorZ)
				else
					locationName = string.format('%s [%d,%d,%d]', location:GetStarSystem().name, location.sectorX, location.sectorY, location.sectorZ)
				end

				local class, event, information = eventDescription(entry)
				local color = eventColor(entry)
				local row =
				{ -- if we don't specify widget, default one will be used
					{data = entry[3], widget = ui:Label(Format.Date(entry[3])):SetColor(color) },
					{data = entry[1], widget = ui:Label(class):SetColor(color) },
					{data = entry[2], widget = ui:Label(event):SetColor(color) },
					{data = location, widget = ui:Label(locationName):SetColor(color) },
					{data = entry[5], widget = ui:Label(Format.Money(entry[5])):SetColor(color) },
					{data = information or "", widget = ui:Label(information or ""):SetColor(color)  }
				}
				LogList:AddRow(row)
			end
		end
		return LogList
	end

	LogListWidget:SetInnerWidget(updateLogList())

	local function toggleFilter(category)
		local enabled = true
		return function ()
			enabled = not enabled
			LogFilter = bit32.replace(LogFilter, enabled and 1 or 0, category)
			LogList.widget:ScrollToTop()
			LogListWidget:SetInnerWidget(updateLogList())
		end
	end

	local careerFilter = ui:CheckBox():Toggle()
	local shipFilter = ui:CheckBox():Toggle()
	local travelFilter = ui:CheckBox():Toggle()
	local missionFilter = ui:CheckBox():Toggle()
	careerFilter.onClick:Connect(toggleFilter(categoryID('career')))
	shipFilter.onClick:Connect(toggleFilter(categoryID('ship')))
	travelFilter.onClick:Connect(toggleFilter(categoryID('travel')))
	missionFilter.onClick:Connect(toggleFilter(categoryID('mission')))

	LogScreen:SetInnerWidget(
		ui:VBox(20):PackStart({
			ui:HBox(20):PackEnd({
				ui:Label(l.FILTER),
				ui:HBox(10):PackEnd({ careerFilter, ui:Label(l.CAREER):SetColor(eventColor({'career'}))}),
				ui:HBox(10):PackEnd({ shipFilter, ui:Label(l.SHIP):SetColor(eventColor({'ship'}))}),
				ui:HBox(10):PackEnd({ travelFilter, ui:Label(l.TRAVEL):SetColor(eventColor({'travel'}))}),
				ui:HBox(10):PackEnd({ missionFilter, ui:Label(l.MISSION):SetColor(eventColor({'mission'}))}),
			}),
			LogListWidget,
		})
	)

	return LogScreen
end

return flightLog
