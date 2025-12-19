# Unit Statistics Mod
#  by
# Teg_Navanis
# based on Kill List Mod by Roger Bacon

from UnitStatisticsDefines import *


class UnitStatisticsTools:

	#checks highscores for all units.
	def checkHighScoresAllUnits(self, htype, iPlayer, objIgnoreUnit):

		if (not g_bTrackHighScore):
			return

		PlayerList = [iPlayer]

		#shouldn't be needed. Could be used to update the high scores for all players if g_bGlobalHighScore is set to True
#		if (g_bGlobalHighScore and (g_bTrackAllPlayers or gc.getGame().isGameMultiPlayer())):
#			PlayerList = range(gc.getMAX_PLAYERS())


		for iPlayer in PlayerList:

			objPlayer = gc.getPlayer(iPlayer)
			unitList = PyPlayer(iPlayer).getUnitList()

			self.setHighScoreVal("UnitStats", objPlayer, "HighscoreUnits", htype, -1)

			try:
				InoreUnitID = str(objIgnoreUnit.getID())+ "X" + str(objIgnoreUnit.getOwner())
				for objUnit in unitList:
					UnitID = str(objUnit.getID())+ "X" + str(objUnit.getOwner())
					if UnitID == InoreUnitID:
						unitList.pop(unitList.index(objUnit))
						break
			except:
				pass

			if (len(unitList) == 0):
				print "no units left"
				self.setHighScoreVal("UnitStats", objPlayer, "Highscores", htype, HighScoreTypes[htype])
				self.setHighScoreVal("UnitStats", objPlayer, "HighscoreUnits", htype, -1)
				return


			datalist = []

	#		# Assert that all units have unitstats information.
			self.checkUnitListIntegrity(unitList)

			if (htype == UNITAGE):
				for objUnit in unitList:
					startturn = sdObjectGetVal("UnitStats", objUnit, STARTTURN)
					iScore = (gc.getGame().getGameTurn() - startturn)
					datalist.append(iScore)

			if (htype == MAXTURNSFORTIFIED):
				for objUnit in unitList:
					turninformation = sdObjectGetVal("UnitStats", objUnit, TURNINFORMATION)
					datalist.append(turninformation[2])

			elif (htype == TOTALTURNSFORTIFIED):
				for objUnit in unitList:
					turninformation = sdObjectGetVal("UnitStats", objUnit, TURNINFORMATION)
					datalist.append(turninformation[0])

			elif (htype == COMMANDO):
				for objUnit in unitList:
					turninformation = sdObjectGetVal("UnitStats", objUnit, TURNINFORMATION)
					datalist.append(turninformation[3])

			elif (htype == AVERAGEODDSLOWEST or htype == AVERAGEODDSHIGHEST):
				for objUnit in unitList:
					datalist.append(sdObjectGetVal("UnitStats", objUnit, AVERAGEODDS))

			elif (htype == BODYCOUNT or htype == BATTLECOUNT):
				for objUnit in unitList:
					combatcount = sdObjectGetVal("UnitStats", objUnit, COMBATCOUNT)
					if (htype == BODYCOUNT):
						data = combatcount[ATTACK][1] + combatcount[DEFENSE][1]
					else:
						data = combatcount[ATTACK][0] + combatcount[DEFENSE][0]
					datalist.append(data)

			else:

				for i in range(14):
					if (htype == DAMAGETYPE[i]):
						for objUnit in unitList:
							damagestats = sdObjectGetVal("UnitStats", objUnit, DAMAGESTATS)
							if (not i / 7):
								data = damagestats[INFLICTED][i % 7]
							else:
								data = damagestats[SUFFERED][i % 7]
							datalist.append(data)
						break

				else:

					for objUnit in unitList:
						datalist.append(sdObjectGetVal("UnitStats", objUnit, htype))


			#in order to check the luckiest unit or the unit with the luckiest fight, we want the lowest score
			if (htype == BESTODDS or htype == AVERAGEODDSLOWEST or htype == LIFEODDS):
				while min(datalist) == -1:
					datalist[datalist.index(-1)] = 6000
				highest = datalist.index(min(datalist))
				objUnit = unitList[highest]
				# Get an ID string that is unique (unit.getID() isn't unique)
				UnitID = str(objUnit.getID())+ "X" + str(objUnit.getOwner())

				if (min(datalist) < 101):

					self.setHighScoreVal("UnitStats", objPlayer, "Highscores", htype, min(datalist))
					self.setHighScoreVal("UnitStats", objPlayer, "HighscoreUnits", htype, UnitID)
					if (min(datalist) < self.getHighScoreVal("UnitStats", objPlayer, "HighscoresHoF", htype)):
						self.setHighScoreVal("UnitStats", objPlayer, "HighscoresHoF", htype, min(datalist))
						self.setHighScoreVal("UnitStats", objPlayer, "HighscoreUnitsHoF", htype, UnitID)

				else:

					self.setHighScoreVal("UnitStats", objPlayer, "Highscores", htype, min(datalist))
					self.setHighScoreVal("UnitStats", objPlayer, "HighscoreUnits", htype, -1)

			# for all other high scores, we want the highest score
			else:
				highest = datalist.index(max(datalist))
				objUnit = unitList[highest]
				# Get an ID string that is unique (unit.getID() isn't unique)
				UnitID = str(objUnit.getID())+ "X" + str(objUnit.getOwner())
				if (max(datalist) > 0):

					self.setHighScoreVal("UnitStats", objPlayer, "Highscores", htype, max(datalist))
					self.setHighScoreVal("UnitStats", objPlayer, "HighscoreUnits", htype, UnitID)
					if (max(datalist) > self.getHighScoreVal("UnitStats", objPlayer, "HighscoresHoF", htype)):
						self.setHighScoreVal("UnitStats", objPlayer, "HighscoresHoF", htype, max(datalist))
						self.setHighScoreVal("UnitStats", objPlayer, "HighscoreUnitsHoF", htype, UnitID)
				else:

					self.setHighScoreVal("UnitStats", objPlayer, "Highscores", htype, max(datalist))
					self.setHighScoreVal("UnitStats", objPlayer, "HighscoreUnits", htype, -1)


	def checkTop10(self, htype, iPlayer):

		if (not g_bTrackHighScore):
			return

		objPlayer = gc.getPlayer(iPlayer)
		unitList = []
		UnitIDList = []
		ValList = []

		if (g_bGlobalHighScore and (g_bTrackAllPlayers or gc.getGame().isGameMultiPlayer())):
			for iPlayer in range(gc.getMAX_PLAYERS()):
				if (gc.getPlayer(iPlayer).isEverAlive()):
					pyPlayer = PyPlayer(iPlayer)
					unitList = unitList + pyPlayer.getUnitList()
		else:
			unitList = PyPlayer(iPlayer).getUnitList()

		if (len(unitList) == 0):
			return [], []

		datalist = []

#		# Assert that all units have unitstats information.
		self.checkUnitListIntegrity(unitList)

		if (htype == UNITAGE):
			for objUnit in unitList:
				startturn = sdObjectGetVal("UnitStats", objUnit, STARTTURN)
				iScore = (gc.getGame().getGameTurn() - startturn)
				datalist.append(iScore)

		if (htype == MAXTURNSFORTIFIED):
			for objUnit in unitList:
				turninformation = sdObjectGetVal("UnitStats", objUnit, TURNINFORMATION)
				datalist.append(turninformation[2])

		elif (htype == TOTALTURNSFORTIFIED):
			for objUnit in unitList:
				turninformation = sdObjectGetVal("UnitStats", objUnit, TURNINFORMATION)
				datalist.append(turninformation[0])

		elif (htype == COMMANDO):
			for objUnit in unitList:
				turninformation = sdObjectGetVal("UnitStats", objUnit, TURNINFORMATION)
				datalist.append(turninformation[3])

		elif (htype == AVERAGEODDSLOWEST or htype == AVERAGEODDSHIGHEST):
			for objUnit in unitList:
				datalist.append(sdObjectGetVal("UnitStats", objUnit, AVERAGEODDS))

		elif (htype == BODYCOUNT or htype == BATTLECOUNT):
			for objUnit in unitList:
				combatcount = sdObjectGetVal("UnitStats", objUnit, COMBATCOUNT)
				if (htype == BODYCOUNT):
					data = combatcount[ATTACK][1] + combatcount[DEFENSE][1]
				else:
					data = combatcount[ATTACK][0] + combatcount[DEFENSE][0]
				datalist.append(data)

		else:

			for i in range(14):
				if (htype == DAMAGETYPE[i]):
					for objUnit in unitList:
						damagestats = sdObjectGetVal("UnitStats", objUnit, DAMAGESTATS)
						if (not i / 7):
							data = damagestats[INFLICTED][i % 7]
						else:
							data = damagestats[SUFFERED][i % 7]
						datalist.append(data)
					break

			else:

				for objUnit in unitList:
					datalist.append(sdObjectGetVal("UnitStats", objUnit, htype))


		#in order to check the luckiest unit or the unit with the luckiest fight, we want the lowest score
		if (htype == BESTODDS or htype == AVERAGEODDSLOWEST or htype == LIFEODDS):
			while min(datalist) == -1:
				datalist[datalist.index(-1)] = 6000

			for i in range(10):
				if (len(unitList) == 0):
					return UnitIDList, ValList
				if (min(datalist) < 101):
					highest = datalist.index(min(datalist))
					value = datalist.pop(highest)
					objUnit = unitList.pop(highest)
					UnitID = str(objUnit.getID())+ "X" + str(objUnit.getOwner())
					UnitIDList.append(UnitID)
					ValList.append(value)


		else:
			for i in range(10):
				if (len(unitList) == 0):
					return UnitIDList, ValList
				if (max(datalist) > 0):
					highest = datalist.index(max(datalist))
					value = datalist.pop(highest)
					objUnit = unitList.pop(highest)
					UnitID = str(objUnit.getID())+ "X" + str(objUnit.getOwner())
					UnitIDList.append(UnitID)
					ValList.append(value)

		return UnitIDList, ValList

	#check if a current unit has the high score
	def checkHighScoresCurrentUnit(self, highscorelist, objUnit):

		if (not g_bTrackHighScore):
			return

		objPlayer = gc.getPlayer(objUnit.getOwner())

		# Get an ID string that is unique (unit.getID() isn't unique)
		UnitID = str(objUnit.getID())+ "X" + str(objUnit.getOwner())

		#for every high score that is checked
		for htype in highscorelist:


			#get the value of the unit that shall be tested
			if (htype == UNITAGE):
				startturn = sdObjectGetVal("UnitStats", objUnit, STARTTURN)
				iScore = (gc.getGame().getGameTurn() - startturn)
			elif (htype == AVERAGEODDSLOWEST or htype == AVERAGEODDSHIGHEST):
				iScore = sdObjectGetVal("UnitStats", objUnit, AVERAGEODDS)
			elif (htype == BODYCOUNT):
				combatcount = sdObjectGetVal("UnitStats", objUnit, COMBATCOUNT)
				iScore = combatcount[ATTACK][1] + combatcount[DEFENSE][1]
			elif (htype == BATTLECOUNT):
				combatcount = sdObjectGetVal("UnitStats", objUnit, COMBATCOUNT)
				iScore = combatcount[ATTACK][0] + combatcount[DEFENSE][0]

			else:
				damagelist = [INFLICTED, SUFFERED]
				for i in range(14):
					if (htype == DAMAGETYPE[i]):
						damagestats = sdObjectGetVal("UnitStats", objUnit, DAMAGESTATS)
						if (not i / 7):
							iScore = damagestats[INFLICTED][i % 7]
						else:
							iScore = damagestats[SUFFERED][i % 7]
						break
				else:
					iScore = sdObjectGetVal("UnitStats", objUnit, htype)


			#get the current high score
			iHighScore = self.getHighScoreVal("UnitStats", objPlayer, "Highscores", htype)


			#in order to check the unit with the luckiest fight, we want the lowest score
			if (htype == BESTODDS or htype == AVERAGEODDSLOWEST or htype == LIFEODDS):
			#if the current unit's score is lower than the high score, mark the unit as the best

				if(iScore < iHighScore or iHighScore == -1):
					self.setHighScoreVal("UnitStats", objPlayer, "Highscores", htype, iScore)
					self.setHighScoreVal("UnitStats", objPlayer, "HighscoreUnits", htype, UnitID)
					if (iScore < self.getHighScoreVal("UnitStats", objPlayer, "HighscoresHoF", htype)):
						self.setHighScoreVal("UnitStats", objPlayer, "HighscoresHoF", htype, iScore)
						self.setHighScoreVal("UnitStats", objPlayer, "HighscoreUnitsHoF", htype, UnitID)


			#if the current unit's score is higher than the high score, mark the unit as the best
			else:

				if (iScore > iHighScore):
					self.setHighScoreVal("UnitStats", objPlayer, "Highscores", htype, iScore)
					self.setHighScoreVal("UnitStats", objPlayer, "HighscoreUnits", htype, UnitID)
					if (iScore > self.getHighScoreVal("UnitStats", objPlayer, "HighscoresHoF", htype)):
						self.setHighScoreVal("UnitStats", objPlayer, "HighscoresHoF", htype, iScore)
						self.setHighScoreVal("UnitStats", objPlayer, "HighscoreUnitsHoF", htype, UnitID)

	# Get a list with all units that hold at least one high score. Needed for the unitstats screen.
	def getHighScoreUnits(self, strHSorHOF):
		highscorelist = self.getAllHighScores()
		HSIDList = []
		for htype in highscorelist:
			strUnit = self.getHSUnit(htype, strHSorHOF)

			if (not isinstance(strUnit, (int))):
				if (HSIDList.count(strUnit) == 0):
					HSIDList.append(strUnit)
		return HSIDList

	# Get a list with all high scores a specific unit holds. Needed for the unitstats screen.
	def getHighScoresCurrentUnit(self, objUnit, strHSorHOF):
		highscorelist = self.getAllHighScores()
		HSList = []
		if (strHSorHOF == "top10"):
			strHSorHOF = "highscore"
		# Get an ID string that is unique (unit.getID() isn't unique)
		UnitID = sdObjectGetVal("UnitStats", objUnit, UNITID)

		for htype in highscorelist:
			strUnit = self.getHSUnit(htype, strHSorHOF)

			if (not isinstance(strUnit, (int))):
				if (UnitID == strUnit):
					HSList.append(htype)
		return HSList

	# Get a the unit that holds a certain high score. Needed for the unitstats screen. Only needed to compile global high scores. Use getHighScoreVal otherwise.
	def getHSUnit(self, htype, strHSorHOF):

		strUnit = 0

		if (strHSorHOF == "top10"):
			strHSorHOF = "highscore"

		if (g_bGlobalHighScore):
			if (htype == BESTODDS or htype == AVERAGEODDSLOWEST or htype == LIFEODDS):
				lowScore = 1
				highscoreTemp = 101
			else:
				lowScore = 0
				highscoreTemp = 0
			if strHSorHOF == "highscore":
				for iPlayer in range(gc.getMAX_PLAYERS()):
					if (gc.getPlayer(iPlayer).isEverAlive()):
						highscoreNew = self.getHighScoreVal("UnitStats", gc.getPlayer(iPlayer), "Highscores", htype)
						if ((highscoreNew > highscoreTemp and lowScore == 0) or (highscoreNew < highscoreTemp and lowScore == 1)):
							highscoreTemp = highscoreNew
							strUnit = self.getHighScoreVal("UnitStats", gc.getPlayer(iPlayer), "HighscoreUnits", htype)
			elif strHSorHOF == "halloffame":
				for iPlayer in range(gc.getMAX_PLAYERS()):
					if (gc.getPlayer(iPlayer).isEverAlive()):
						highscoreNew = self.getHighScoreVal("UnitStats", gc.getPlayer(iPlayer), "HighscoresHoF", htype)
						if ((highscoreNew > highscoreTemp and lowScore == 0) or (highscoreNew < highscoreTemp and lowScore == 1)):
							highscoreTemp = highscoreNew
							strUnit = self.getHighScoreVal("UnitStats", gc.getPlayer(iPlayer), "HighscoreUnitsHoF", htype)

		else:
			if strHSorHOF == "highscore":
				strUnit = self.getHighScoreVal("UnitStats", gc.getActivePlayer(), "HighscoreUnits", htype)
			elif strHSorHOF == "halloffame":
				strUnit = self.getHighScoreVal("UnitStats", gc.getActivePlayer(), "HighscoreUnitsHoF", htype)
		if (not isinstance(strUnit, (int))):
			return strUnit
		return 0

	# Get a certain high score. Needed for the unitstats screen. Only needed to compile global high scores. Use getHighScoreVal otherwise.
	def getHSVal(self, htype, strHSorHOF):

		if (g_bGlobalHighScore):
			if (htype == BESTODDS or htype == AVERAGEODDSLOWEST or htype == LIFEODDS):
				lowScore = 1
				strHighScore = 101
			else:
				lowScore = 0
				strHighScore = 0
			if strHSorHOF == "highscore":
				for iPlayer in range(gc.getMAX_PLAYERS()):
					if (gc.getPlayer(iPlayer).isEverAlive()):
						highscoreTemp = self.getHighScoreVal("UnitStats", gc.getPlayer(iPlayer), "Highscores", htype)
						if ((highscoreTemp > strHighScore and lowScore == 0) or (highscoreTemp < strHighScore and lowScore == 1)):
							strHighScore = highscoreTemp
			elif strHSorHOF == "halloffame":
				for iPlayer in range(gc.getMAX_PLAYERS()):
					if (gc.getPlayer(iPlayer).isEverAlive()):
						highscoreTemp = self.getHighScoreVal("UnitStats", gc.getPlayer(iPlayer), "HighscoresHoF", htype)
						if ((highscoreTemp > strHighScore and lowScore == 0) or (highscoreTemp < strHighScore and lowScore == 1)):
							strHighScore = highscoreTemp
		else:
			if strHSorHOF == "highscore":
				strHighScore = self.getHighScoreVal("UnitStats", gc.getActivePlayer(), "Highscores", htype)
			elif strHSorHOF == "halloffame":
				strHighScore = self.getHighScoreVal("UnitStats", gc.getActivePlayer(), "HighscoresHoF", htype)

		return strHighScore

	# Get a list with all high scores. Needed for the unitstats screen.
	def getAllHighScores(self):
		messagestringlist = []

		if (g_bShowCombatCount):
			messagestringlist.extend([
								BODYCOUNT,
								BATTLECOUNT])

		if g_bFfHMode:
			messagestringlist.append(HEROCOUNT)
			messagestringlist.append(SPELLCOUNT)
			messagestringlist.append(CASTCOUNT)

		if(g_bShowDamageInformation):
			messagestringlist.extend([
								DAMAGETYPE[0],
								DAMAGETYPE[7],
					   			DAMAGETYPE[2],
					   			DAMAGETYPE[1],
#								DAMAGETYPE[6],
								DAMAGETYPE[8],
								DAMAGETYPE[9],
								DAMAGETYPE[3],
								DAMAGETYPE[10],
								DAMAGETYPE[4],
								DAMAGETYPE[11]
#								,DAMAGETYPE[13]
								])
			if not g_bFfHMode:
				messagestringlist.extend([
								DAMAGETYPE[5],
								DAMAGETYPE[12]])

		if (g_bTrackMovement):
			messagestringlist.extend([
							  	MOVEMENT_COUNTER,
								WARP,
								CARGO_COUNTER])

		if (g_bShowExperience):
			messagestringlist.append(
								EXPERIENCE)

		if (g_bShowOdds):
			messagestringlist.extend([
							  	BESTODDS,
								AVERAGEODDSHIGHEST,
								AVERAGEODDSLOWEST,
								LIFEODDS])

		if (g_bTrackTurnInformation):
			messagestringlist.extend([
								TOTALTURNSFORTIFIED,
								MAXTURNSFORTIFIED,
								COMMANDO,
								UNITAGE])

		return messagestringlist

	# Get an explanatory string for this high score. Needed for the unitstats screen.
	# strPlayerUnit is either "player" or "unit"
	# Use "player" for strings such as "Most collateral damage inflicted: 0 (living units: 0)"
	# Use "units" for strings such as "This unit has inflicted the most collateral damage"
	def getHSString(self, htype, strPlayerUnit):

		objPlayer = gc.getActivePlayer()

		if (strPlayerUnit == "player"):

			# The high score and the hall of fame high score for this high score type are loaded
			htypeHS = self.getHSVal(htype, "highscore")
			htypeHSHOF = self.getHSVal(htype, "halloffame")

			if (htype == BESTODDS):

				if (htypeHSHOF >= 101):
					strHS = localText.getText("TXT_KEY_PLAYERHIGHSCORE_" + htype, ()) + "\n- (" + localText.getText("TXT_KEY_LIVING_UNITS", ()) +" -)"
				elif (htypeHS >= 101):
					temp = localText.getText("TXT_KEY_PLAYERHIGHSCORE_" + htype, ()) + "\n%.1f%% (" + localText.getText("TXT_KEY_LIVING_UNITS", ()) +" -)"
					strHS = temp %(htypeHSHOF)
				else:
					temp = localText.getText("TXT_KEY_PLAYERHIGHSCORE_" + htype, ()) + "\n%.1f%% (" + localText.getText("TXT_KEY_LIVING_UNITS", ()) +" %.1f%%)"
					strHS = temp %(htypeHSHOF, htypeHS)
				return strHS

			elif (htype == AVERAGEODDSHIGHEST):

				if (htypeHSHOF == -1):
					strHS = localText.getText("TXT_KEY_PLAYERHIGHSCORE_" + htype, ()) + "\n- (" + localText.getText("TXT_KEY_LIVING_UNITS", ()) +" -)"
				elif (htypeHS == -1):
					temp = localText.getText("TXT_KEY_PLAYERHIGHSCORE_" + htype, ()) + "\n%.1f%% (" + localText.getText("TXT_KEY_LIVING_UNITS", ()) +" -)"
					strHS = temp %(htypeHSHOF)
				else:
					temp = localText.getText("TXT_KEY_PLAYERHIGHSCORE_" + htype, ()) + "\n%.1f%% (" + localText.getText("TXT_KEY_LIVING_UNITS", ()) +" %.1f%%)"
					strHS = temp %(htypeHSHOF, htypeHS)
				return strHS

			elif (htype == AVERAGEODDSLOWEST):

				if (htypeHSHOF >= 101):
					strHS = localText.getText("TXT_KEY_PLAYERHIGHSCORE_" + htype, ()) + "\n- (" + localText.getText("TXT_KEY_LIVING_UNITS", ()) +" -)"
				elif (htypeHS >= 101):
					temp = localText.getText("TXT_KEY_PLAYERHIGHSCORE_" + htype, ()) + "\n%.1f%% (" + localText.getText("TXT_KEY_LIVING_UNITS", ()) +" -)"
					strHS = temp %(htypeHSHOF)
				else:
					temp = localText.getText("TXT_KEY_PLAYERHIGHSCORE_" + htype, ()) + "\n%.1f%% (" + localText.getText("TXT_KEY_LIVING_UNITS", ()) +" %.1f%%)"
					strHS = temp %(htypeHSHOF, htypeHS)
				return strHS

			elif (htype == LIFEODDS):

				if (htypeHSHOF >= 101):
					strHS = localText.getText("TXT_KEY_PLAYERHIGHSCORE_" + htype, ()) + "\n- (" + localText.getText("TXT_KEY_LIVING_UNITS", ()) +" -)"
				elif (htypeHS >= 101):
					temp = localText.getText("TXT_KEY_PLAYERHIGHSCORE_" + htype, ()) + "\n%.1f%% (" + localText.getText("TXT_KEY_LIVING_UNITS", ()) +" -)"
					strHS = temp %(htypeHSHOF)
				else:
					temp = localText.getText("TXT_KEY_PLAYERHIGHSCORE_" + htype, ()) + "\n%.1f%% (" + localText.getText("TXT_KEY_LIVING_UNITS", ()) +" %.1f%%)"
					strHS = temp %(htypeHSHOF, htypeHS)
				return strHS

			elif (htype == UNITAGE):

				if (htypeHSHOF == -1 and htypeHS == -1):
					strHS = localText.getText("TXT_KEY_PLAYERHIGHSCORE_" + htype, (0,0))
				elif (htypeHSHOF == -1 and htypeHS != -1):
					strHS = localText.getText("TXT_KEY_PLAYERHIGHSCORE_" + htype, (htypeHS, htypeHS))
				elif (htypeHSHOF != -1 and htypeHS == -1):
					strHS = localText.getText("TXT_KEY_PLAYERHIGHSCORE_" + htype, (htypeHSHOF, 0))
				else:
					strHS = localText.getText("TXT_KEY_PLAYERHIGHSCORE_" + htype, (htypeHSHOF, htypeHS))
				return strHS

			elif (htype == DAMAGETYPE[0] or htype == DAMAGETYPE[1] or htype == DAMAGETYPE[2] or htype == DAMAGETYPE[3] or htype == DAMAGETYPE[4] or htype == DAMAGETYPE[5] or htype == DAMAGETYPE[6] or htype == DAMAGETYPE[7] or htype == DAMAGETYPE[8] or htype == DAMAGETYPE[9] or htype == DAMAGETYPE[10] or htype == DAMAGETYPE[11] or htype == DAMAGETYPE[12] or htype == DAMAGETYPE[13]):
				str2ndpartstring = "\n %.2f (" + localText.getText("TXT_KEY_LIVING_UNITS", ()) + " %.2f)"
			else:
				str2ndpartstring = "\n%s (" + localText.getText("TXT_KEY_LIVING_UNITS", ()) + " %s)"

			strHS = localText.getText("TXT_KEY_PLAYERHIGHSCORE_" + htype, ()) + str2ndpartstring


			# A message is added, using the string saved in the messagestringlist and actual high scores.
			strHS = strHS %(htypeHSHOF, htypeHS)
			return strHS

		if (strPlayerUnit == "unit"):

			strHS = localText.getText("TXT_KEY_UNITHIGHSCORE_" + htype, ())

			return strHS

	def getUnitStatisticsString(self, objUnit):

		# Return immediately if we got an invalid unit
		if(objUnit == None):
			return 0

		# Return immediately if the unit passed in doesn't belong to the
		# currently active player. If objUnit.getExperience() fails,
		# this is probably because a dead unit is processed (objPlot instead of objUnit)
		try:
			test = objUnit.getExperience()
			if(objUnit.getOwner() != gc.getGame().getActivePlayer() and not g_bShowAllPlayers):
				return 0
		except:
			pass

		message1 = sdObjectGetVal("UnitStats", objUnit, LIST)

		turninformation = sdObjectGetVal("UnitStats", objUnit, TURNINFORMATION)
		totalturnsfortified = turninformation[0]
		maxturnsfortified = turninformation[2]
		commando = turninformation[3]

		capturecount = sdObjectGetVal("UnitStats", objUnit, CAPTURECOUNT)

		combatcount = sdObjectGetVal("UnitStats", objUnit, COMBATCOUNT)
		numberOfKills = combatcount[ATTACK][1] + combatcount[DEFENSE][1]
		numberOfKillsAttacking = combatcount[ATTACK][1]
		numberOfKillsDefending = combatcount[DEFENSE][1]
		battlecount = combatcount[ATTACK][0] + combatcount[DEFENSE][0]
		battlecountAttacking = combatcount[ATTACK][0]
		battlecountDefending = combatcount[DEFENSE][0]
		withdrawalcount = combatcount[ATTACK][3] + combatcount[DEFENSE][3]
		withdrawalcountAttacking = combatcount[ATTACK][3]
		withdrawalcountDefending = combatcount[DEFENSE][3]
		losscount = combatcount[ATTACK][2] + combatcount[DEFENSE][2]
		losscountAttacking = combatcount[ATTACK][2]
		losscountDefending = combatcount[DEFENSE][2]
		numberOfKillsCollateral = combatcount[COLLATERAL][1]
		losscountCollateral = combatcount[COLLATERAL][2]
		numberOfKillsFlanking = combatcount[FLANKING][1]
		losscountFlanking = combatcount[FLANKING][2]
		numberOfKillsOther = combatcount[OTHER][1]
		losscountOther = combatcount[OTHER][2]
		numberOfKillsAirStrike = combatcount[AIRSTRIKE][1]
		losscountAirStrike = combatcount[AIRSTRIKE][2]

		heroCount = sdObjectGetVal("UnitStats", objUnit, HEROCOUNT)
		spellCount = sdObjectGetVal("UnitStats", objUnit, SPELLCOUNT)
		castCount = sdObjectGetVal("UnitStats", objUnit, CASTCOUNT)

		airnumberOfKills = combatcount[AIRATTACK][1] + combatcount[AIRDEFENSE][1]
		airnumberOfKillsAttacking = combatcount[AIRATTACK][1]
		airnumberOfKillsDefending = combatcount[AIRDEFENSE][1]
		airbattlecount = combatcount[AIRATTACK][0] + combatcount[AIRDEFENSE][0]
		airbattlecountAttacking = combatcount[AIRATTACK][0]
		airbattlecountDefending = combatcount[AIRDEFENSE][0]
		airstrikecount = combatcount[AIRSTRIKE][0]
		airstrikecountdefending = combatcount[AIRSTRIKE][3]
		airlosscount = combatcount[AIRATTACK][2] + combatcount[AIRDEFENSE][2]
		airlosscountAttacking = combatcount[AIRATTACK][2]
		airlosscountDefending = combatcount[AIRDEFENSE][2]

		cargocounter = sdObjectGetVal("UnitStats", objUnit, CARGO_COUNTER)
		movecounter = sdObjectGetVal("UnitStats", objUnit, MOVEMENT_COUNTER)
		distancewarped = sdObjectGetVal("UnitStats", objUnit, WARP)
		distancetransported = sdObjectGetVal("UnitStats", objUnit, WARP) - sdObjectGetVal("UnitStats", objUnit, MOVEMENT_COUNTER)
		#sdObjectSetVal("UnitStats", objUnit, TRANSPORT, distancetransported)


		# The numbers in the DamageStats signify the stats that are being updated (compare with iTypeofAttack).
		# 0: damage inflicted/received
		# 1: damage inflicted while defending/received while attacking
		# 2: damage inflicted while attacking/received while defending
		# 3: collateral damage inflicted/received
		# 4: flanking damage inflicted/received
		# 5: air strike damage inflicted/received
		# 6: other damage inflicted/received

		damagestats = sdObjectGetVal("UnitStats", objUnit, DAMAGESTATS)
		damageinflicted = str("%.2f") %(damagestats[INFLICTED][0])
		damagesuffered = str("%.2f") %(damagestats[SUFFERED][0])
		damageinflictedattacking = str("%.2f") %(damagestats[INFLICTED][2])
		damageinflicteddefending = str("%.2f") %(damagestats[INFLICTED][1])
		damagesufferedattacking = str("%.2f") %(damagestats[SUFFERED][1])
		damagesuffereddefending = str("%.2f") %(damagestats[SUFFERED][2])
		collateraldamageinflicted = str("%.2f") %(damagestats[INFLICTED][3])
		collateraldamagesuffered = str("%.2f") %(damagestats[SUFFERED][3])
		flankingdamageinflicted = str("%.2f") %(damagestats[INFLICTED][4])
		flankingdamagesuffered = str("%.2f") %(damagestats[SUFFERED][4])
		airstrikedamageinflicted = str("%.2f") %(damagestats[INFLICTED][5])
		airstrikedamagesuffered = str("%.2f") %(damagestats[SUFFERED][5])
		otherdamageinflicted = str("%.2f") %(damagestats[INFLICTED][6])
		otherdamagesuffered = str("%.2f") %(damagestats[SUFFERED][6])

		totalotherdamageinflicted = str("%.2f") %(float(collateraldamageinflicted) + (float(otherdamageinflicted)) + (float(flankingdamageinflicted)))
		totalotherdamagesuffered = str("%.2f") %(float(collateraldamagesuffered) + (float(otherdamagesuffered)) + (float(flankingdamagesuffered)))


		averageodds = str("%.1f%%") %(sdObjectGetVal("UnitStats", objUnit, AVERAGEODDS))
		bestodds = sdObjectGetVal("UnitStats", objUnit, BESTODDS)
		if (not isinstance(bestodds, (str))):
			bestodds = str("%.1f%%") %(sdObjectGetVal("UnitStats", objUnit, BESTODDS))
		oddsdata = sdObjectGetVal("UnitStats", objUnit, ODDSDATA)
		lifeodds = str("%.1f%%") %(sdObjectGetVal("UnitStats", objUnit, LIFEODDS))

		strCombatCount = ""
		if(g_bShowCombatCount):
			strCombatCount = localText.getText("TXT_KEY_BATTLES_FOUGHT", (battlecount, battlecountAttacking, battlecountDefending)) + "\n"
			strCombatCount = strCombatCount + localText.getText("TXT_KEY_UNITS_KILLED", (numberOfKills + numberOfKillsCollateral + numberOfKillsFlanking + numberOfKillsOther + numberOfKillsAirStrike, numberOfKillsAttacking, numberOfKillsDefending, numberOfKillsCollateral + numberOfKillsFlanking + numberOfKillsOther + numberOfKillsAirStrike)) + "\n"
			if (losscount > 0):
				strCombatCount = strCombatCount + localText.getText("TXT_KEY_UNITS_LOST", (losscount + losscountCollateral + losscountFlanking + losscountOther + losscountAirStrike, losscountAttacking, losscountDefending, losscountCollateral + losscountFlanking + losscountOther + losscountAirStrike)) + "\n"
			strCombatCount = strCombatCount + localText.getText("TXT_KEY_UNDECIDED_BATTLES", (withdrawalcount, withdrawalcountAttacking, withdrawalcountDefending)) + "\n\n"
#			strCombatCount = strCombatCount + "Cities captured: %s\n\n" %(capturecount)

			# In case that there is a dead unit processed through (objUnit actually being a plot object, the domain type is calculated differently)
			try:
				domaintype = objUnit.getDomainType()
			except:
				unittype = sdObjectGetVal("UnitStats", objUnit, UNITTYPE)
				domaintype = gc.getUnitInfo(unittype).getDomainType()

			if (domaintype == 1):
				strCombatCount = localText.getText("TXT_KEY_NUM_AIR_STRIKES", (airstrikecount, ())) + "\n"
				strCombatCount = strCombatCount + localText.getText("TXT_KEY_NUM_AIR_COMBATS", (airbattlecount, airbattlecountAttacking, airbattlecountDefending)) + "\n"
				strCombatCount = strCombatCount + localText.getText("TXT_KEY_NUM_AIR_KILLS", (airnumberOfKills, airnumberOfKillsAttacking, airnumberOfKillsDefending)) + "\n\n"

			if g_bFfHMode:
				strCombatCount = strCombatCount + localText.getText("TXT_KEY_UNITSTATS_HEROCOUNT", (heroCount, ())) + "\n"
				strCombatCount = strCombatCount + localText.getText("TXT_KEY_UNITSTATS_CASTCOUNT", (castCount, ())) + "\n"
				strCombatCount = strCombatCount + localText.getText("TXT_KEY_UNITSTATS_SPELLCOUNT", (spellCount, ())) + "\n\n"

		strDistanceInformation = ""

		if(g_bTrackMovement):
			strDistanceInformation = localText.getText("TXT_KEY_MOVECOUNTERS", (movecounter, distancetransported, distancewarped))
			if (cargocounter > 0):
				strDistanceInformation = strDistanceInformation + "\n" + localText.getText("TXT_KEY_CARGOCOUNTER", (cargocounter, ()))

		# If some distance information is being displayed then add the blank
		# line spacers.
			if(len(strDistanceInformation) > 0):
				strDistanceInformation = strDistanceInformation + " \n \n"

		if(not g_bShowUnitEventLog):
			message1 = ""

		strDamageInformation = ""
		if(g_bShowDamageInformation):
			strDamageInformation = strDamageInformation + localText.getText("TXT_KEY_DAMAGE_INFLICTED", (damageinflicted, damageinflictedattacking, damageinflicteddefending, airstrikedamageinflicted, totalotherdamageinflicted)) + "\n"
			strDamageInformation = strDamageInformation + localText.getText("TXT_KEY_DAMAGE_SUFFERED", (damagesuffered, damagesufferedattacking, damagesuffereddefending, airstrikedamagesuffered, totalotherdamagesuffered)) + "\n\n"

		strOddsInformation = "\n\n\n"

		if (g_bShowOdds and averageodds != "-1.0%"):
			strOddsInformation = localText.getText("TXT_KEY_ODDSINFORMATION", (averageodds, lifeodds, oddsdata[2])) + "\n"

		strTurnInformation = ""
		if (g_bTrackTurnInformation):
			strTurnInformation = localText.getText("TXT_KEY_TOTALTIMEFORTIFIED", (totalturnsfortified,())) + "\n"
			strTurnInformation = strTurnInformation + localText.getText("TXT_KEY_MAXTIMEFORTIFIED", (maxturnsfortified, ())) + "\n"
			strTurnInformation = strTurnInformation + localText.getText("TXT_KEY_UNITSTATS_COMMANDO", (commando, ())) + "\n\n"


		return "%s%s%s%s%s%s" %(strCombatCount, strOddsInformation, strDamageInformation, strDistanceInformation, strTurnInformation, message1)

	# Launches the player statistics
	def getPlayerStatisticsString(self, iPlayer):

		# Get the currently active player
		PlayerID = "PlayerStats" + str(iPlayer)
		objPlayer = gc.getPlayer(iPlayer)

		combatcount = sdObjectGetVal("UnitStats", objPlayer, COMBATCOUNT)
		numberOfKills = combatcount[ATTACK][1] + combatcount[DEFENSE][1]
		numberOfKillsAttacking = combatcount[ATTACK][1]
		numberOfKillsDefending = combatcount[DEFENSE][1]
		battlecount = combatcount[ATTACK][0] + combatcount[DEFENSE][0]
		battlecountAttacking = combatcount[ATTACK][0]
		battlecountDefending = combatcount[DEFENSE][0]
		withdrawalcount = combatcount[ATTACK][3] + combatcount[DEFENSE][3]
		withdrawalcountAttacking = combatcount[ATTACK][3]
		withdrawalcountDefending = combatcount[DEFENSE][3]
		losscount = combatcount[ATTACK][2] + combatcount[DEFENSE][2]
		losscountAttacking = combatcount[ATTACK][2]
		losscountDefending = combatcount[DEFENSE][2]
		numberOfKillsCollateral = combatcount[COLLATERAL][1]
		losscountCollateral = combatcount[COLLATERAL][2]
		numberOfKillsFlanking = combatcount[FLANKING][1]
		losscountFlanking = combatcount[FLANKING][2]
		numberOfKillsOther = combatcount[OTHER][1]
		losscountOther = combatcount[OTHER][2]
		numberOfKillsAirStrike = combatcount[AIRSTRIKE][1]
		losscountAirStrike = combatcount[AIRSTRIKE][2]


		airnumberOfKills = combatcount[AIRATTACK][1] + combatcount[AIRDEFENSE][1]
		airnumberOfKillsAttacking = combatcount[AIRATTACK][1]
		airnumberOfKillsDefending = combatcount[AIRDEFENSE][1]
		airbattlecount = combatcount[AIRATTACK][0] + combatcount[AIRDEFENSE][0]
		airbattlecountAttacking = combatcount[AIRATTACK][0]
		airbattlecountDefending = combatcount[AIRDEFENSE][0]
		airstrikecount = combatcount[AIRSTRIKE][0]
		airstrikecountdefending = combatcount[AIRSTRIKE][3]
		airlosscount = combatcount[AIRATTACK][2] + combatcount[AIRDEFENSE][2]
		airlosscountAttacking = combatcount[AIRATTACK][2]
		airlosscountDefending = combatcount[AIRDEFENSE][2]

		heroCount = sdObjectGetVal("UnitStats", objPlayer, HEROCOUNT)
		spellCount = sdObjectGetVal("UnitStats", objPlayer, SPELLCOUNT)
		castCount = sdObjectGetVal("UnitStats", objPlayer, CASTCOUNT)

		highestDefeatOdds = sdObjectGetVal("UnitStats", objPlayer, HIGHEST_DEFEAT_ODDS)
		highestDefeatOdds = str("%.1f%%") %(highestDefeatOdds)


		lowestVictoryOdds = self.getHighScoreVal("UnitStats", objPlayer, "HighscoresHoF", BESTODDS)
		if lowestVictoryOdds >= 101:
			lowestVictoryOdds = "0.0%"
		else:
			lowestVictoryOdds = str("%.1f%%") %(lowestVictoryOdds)

		totalodds = sdObjectGetVal("UnitStats", objPlayer, ODDSDATA)

		if (numberOfKills + losscount) == 0:
			strAverageOdds = "0.0%"
			strAverageResults = "0.0%"
			strLuckIndex = "0"
		else:
			# Average Odds is the accumulated battle odds divided by the number of battles (excluding withdrawals)
			fAverageOdds = totalodds / (numberOfKills + losscount)
			strAverageOdds = str("%.1f%%") %(fAverageOdds)

			# Average Results is the actual percentage of battles won (excluding withdrawals)
			fAverageResults = (0.0 + numberOfKills) / (numberOfKills + losscount) * 100
			strAverageResults = str("%.1f%%") %(fAverageResults)

			# The discrepancy between averageOdds and averageResults is used to measure the luck of a player
			# The result is not given in percent, but divided by 100. 1 means that you should have lost one more unit
			# than you actually did. If you lose 10 battles with 50% odds you will get the same result as if you lose
			# 5 with 99.9% odds. Of course the likelyhood of the latter is much smaller, but in both cases, you have lost
			# 5 more battles than you should have.
			fLuckIndex = (fAverageResults - fAverageOdds) * (numberOfKills + losscount) /100
			strLuckIndex = str("%.1f") %(fLuckIndex)


		turninformation = sdObjectGetVal("UnitStats", objPlayer, TURNINFORMATION)
		totalturnsfortified = turninformation[0]
		maxturnsfortified = turninformation[2]

		cargocounter = sdObjectGetVal("UnitStats", objPlayer, CARGO_COUNTER)
		movecounter = sdObjectGetVal("UnitStats", objPlayer, MOVEMENT_COUNTER)
		distancewarped = sdObjectGetVal("UnitStats", objPlayer, WARP)
		distancetransported = sdObjectGetVal("UnitStats", objPlayer, WARP) - sdObjectGetVal("UnitStats", objPlayer, MOVEMENT_COUNTER)
		#sdObjectSetVal("UnitStats", objPlayer, TRANSPORT, distancetransported)

		damagestats = sdObjectGetVal("UnitStats", objPlayer, DAMAGESTATS)
		damageinflicted = str("%.2f") %(damagestats[INFLICTED][0])
		damagesuffered = str("%.2f") %(damagestats[SUFFERED][0])
		damageinflictedattacking = str("%.2f") %(damagestats[INFLICTED][2])
		damageinflicteddefending = str("%.2f") %(damagestats[INFLICTED][1])
		damagesufferedattacking = str("%.2f") %(damagestats[SUFFERED][1])
		damagesuffereddefending = str("%.2f") %(damagestats[SUFFERED][2])
		collateraldamageinflicted = str("%.2f") %(damagestats[INFLICTED][3])
		collateraldamagesuffered = str("%.2f") %(damagestats[SUFFERED][3])
		flankingdamageinflicted = str("%.2f") %(damagestats[INFLICTED][4])
		flankingdamagesuffered = str("%.2f") %(damagestats[SUFFERED][4])
		airstrikedamageinflicted = str("%.2f") %(damagestats[INFLICTED][5])
		airstrikedamagesuffered = str("%.2f") %(damagestats[SUFFERED][5])
		otherdamageinflicted = str("%.2f") %(damagestats[INFLICTED][6])
		otherdamagesuffered = str("%.2f") %(damagestats[SUFFERED][6])

		totalotherdamageinflicted = str("%.2f") %((float(collateraldamageinflicted)) + (float(otherdamageinflicted)) + (float(flankingdamageinflicted)))
		totalotherdamagesuffered = str("%.2f") %((float(collateraldamagesuffered)) + (float(otherdamagesuffered)) + (float(flankingdamagesuffered)))


		goodies = sdObjectGetVal("UnitStats", objPlayer, GOODIES)
		experience = sdObjectGetVal("UnitStats", objPlayer, EXPERIENCE)

		strCombatCount = ""
		strAirCombatCount = ""
		if(g_bShowCombatCount):
			strCombatCount = localText.getText("TXT_KEY_BATTLES_FOUGHT", (battlecount, battlecountAttacking, battlecountDefending)) + "\n"
			strCombatCount = strCombatCount + localText.getText("TXT_KEY_UNITS_KILLED", (numberOfKills + numberOfKillsCollateral + numberOfKillsFlanking + numberOfKillsOther + numberOfKillsAirStrike, numberOfKillsAttacking, numberOfKillsDefending, numberOfKillsCollateral + numberOfKillsFlanking + numberOfKillsOther + numberOfKillsAirStrike)) + "\n"
			strCombatCount = strCombatCount + localText.getText("TXT_KEY_UNITS_LOST", (losscount + losscountCollateral + losscountFlanking + losscountOther + losscountAirStrike, losscountAttacking, losscountDefending, losscountCollateral + losscountFlanking + losscountOther + losscountAirStrike)) + "\n"
			strCombatCount = strCombatCount + localText.getText("TXT_KEY_UNDECIDED_BATTLES", (withdrawalcount, withdrawalcountAttacking, withdrawalcountDefending)) + "\n\n"
		if g_bFfHMode:
			strCombatCount = strCombatCount + localText.getText("TXT_KEY_UNITSTATS_HEROCOUNT", (heroCount, ())) + "\n"
			strCombatCount = strCombatCount + localText.getText("TXT_KEY_UNITSTATS_CASTCOUNT", (castCount, ())) + "\n"
			strCombatCount = strCombatCount + localText.getText("TXT_KEY_UNITSTATS_SPELLCOUNT", (spellCount, ())) + "\n\n"
		else:
			if(g_bShowCombatCount):
				strAirCombatCount = localText.getText("TXT_KEY_NUM_AIR_STRIKES", (airstrikecount, ())) + "\n"
				strAirCombatCount = strAirCombatCount + localText.getText("TXT_KEY_NUM_AIR_COMBATS", (airbattlecount, airbattlecountAttacking, airbattlecountDefending)) + "\n"
				strAirCombatCount = strAirCombatCount + localText.getText("TXT_KEY_NUM_AIR_KILLS", (airnumberOfKills, airnumberOfKillsAttacking, airnumberOfKillsDefending)) + "\n"
				strAirCombatCount = strAirCombatCount + localText.getText("TXT_KEY_NUM_AIR_LOSSES", (airlosscount, airlosscountAttacking, airlosscountDefending)) + "\n\n"

		strOddsInformation = ""
		strOddsInformation = localText.getText("TXT_KEY_LOWEST_VICTORY_ODDS", (lowestVictoryOdds, ())) + "\n"
		strOddsInformation = strOddsInformation + localText.getText("TXT_KEY_HIGHEST_DEFEAT_ODDS", (highestDefeatOdds, ())) + "\n"
		strOddsInformation = strOddsInformation + localText.getText("TXT_KEY_AVERAGE_ODDS", (strAverageOdds, ())) + "\n"
		strOddsInformation = strOddsInformation + localText.getText("TXT_KEY_AVERAGE_RESULTS", (strAverageResults, ())) + "\n"
		strOddsInformation = strOddsInformation + localText.getText("TXT_KEY_LUCK_INDEX", (strLuckIndex, ())) + "\n\n"


		strDistanceInformation = ""
		if(g_bTrackMovement):
			strDistanceInformation = localText.getText("TXT_KEY_MOVECOUNTERS", (movecounter, distancetransported, distancewarped))
			if (cargocounter > 0):
				strDistanceInformation = strDistanceInformation + "\n" + localText.getText("TXT_KEY_CARGOCOUNTER", (cargocounter, ()))
		# If some distance information is being displayed then add the blank
		# line spacers.
			if(len(strDistanceInformation) > 0):
				strDistanceInformation = strDistanceInformation + " \n \n"

		if(not g_bShowUnitEventLog):
			message1 = ""

		strTurnInformation = ""
		if (g_bTrackTurnInformation):
			strTurnInformation = strTurnInformation + localText.getText("TXT_KEY_TOTALTIMEFORTIFIED", (totalturnsfortified, ())) + "\n"
			strTurnInformation = strTurnInformation + localText.getText("TXT_KEY_MAXTIMEFORTIFIED", (maxturnsfortified, ())) + "\n\n"

		strDamageInformation = ""

		if(g_bShowDamageInformation):
			strDamageInformation = strDamageInformation + localText.getText("TXT_KEY_DAMAGE_INFLICTED", (damageinflicted, damageinflictedattacking, damageinflicteddefending, airstrikedamageinflicted, totalotherdamageinflicted)) + "\n"
			strDamageInformation = strDamageInformation + localText.getText("TXT_KEY_DAMAGE_SUFFERED", (damagesuffered, damagesufferedattacking, damagesuffereddefending, airstrikedamagesuffered, totalotherdamagesuffered)) + "\n\n"

		strMiscInformation = ""

		if (g_bShowExperience):
			strMiscInformation = strMiscInformation + localText.getText("TXT_KEY_EXPERIENCE_GAINED", (experience, ())) + "\n"
		if (g_bTrackGoodyReceived):
			strMiscInformation = strMiscInformation + localText.getText("TXT_KEY_GOODIES_RECEIVED", (goodies,()))


		return "%s%s%s%s%s%s%s" %(strCombatCount, strDamageInformation, strOddsInformation, strAirCombatCount, strDistanceInformation, strTurnInformation, strMiscInformation)


	# This removes any data that was saved for the hall of fame and is now obsolete
	def cleanGraveYardList(self, iPlayer):
		graveyardlist = sdObjectGetVal("UnitStats", gc.getPlayer(iPlayer), GRAVEYARDLIST)
		graveyardlisttemp = graveyardlist[:]

		highscorelist = self.getAllHighScores()
		HSList = []

		#create a list that includes all high score holders.
		for htype in highscorelist:
			strUnit = self.getHighScoreVal("UnitStats", gc.getPlayer(iPlayer), "HighscoreUnitsHoF", htype)
			if (not isinstance(strUnit, (int))):
				if (HSList.count(strUnit) == 0):
					HSList.append(strUnit)

		# delete all legitimate high score holders from graveyardlisttemp.
		for id in HSList:
			try:
				graveyardlisttemp.pop(graveyardlisttemp.index(id))
			except:
				continue

		# the remaining units in graveyardlisttemp are no longer holding any high score and are here removed from the graveyardlist.
		for id in graveyardlisttemp:
			try:
				graveyardlist[graveyardlist.index(id)] = 0
				objPlot = gc.getMap().plot(graveyardlist.index(id), iPlayer)
				sdObjectWipe("UnitStats", objPlot)
			except:
				continue
		sdObjectSetVal("UnitStats", gc.getPlayer(iPlayer), GRAVEYARDLIST, graveyardlist)

	# Converts a number into its string representation. This is needed since
	# for whatever reason, numbers did not work very well when using them
	# for all of the different panels in the screen. The
	# unit ID number 382343 is converted to: CHBCDC.
	def numberToAlpha(self, iNum):
		#             1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26
		alphaList = ['A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z']
		strNum = str(iNum)
		strAlpha = ""

		# Go though the alphaList and convert the numbers to letters
		for i in range (len(strNum)):
			strAlpha = strAlpha + alphaList[int(strNum[i])]

		return strAlpha


	# Converts a number into its string representation. This is needed since
	# for whatever reason, numbers did not work very well when using them
	# for all of the different panels in the screen. The
	# string "CHBCDC" is converted to: 382343.
	def alphaToNumber(self, strAlpha):
		#             1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26
		alphaList = ['A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z']

		strNum = ""

		# Go though the alphaList and convert the letters to numbers
		for i in range (len(strAlpha)):
			strNum = strNum + str(alphaList.index(strAlpha[i]))

		return int(strNum)

	# Input: UnitID in the form of 1235X1 or ABCD-A
	# Output: unit object or plot object, depending on whether the unit is alive or dead.
	def unitIDToObject(self, UnitID):

		if (UnitID == None or isinstance(UnitID, (int))):
			return 0, 0, 0

		# Get the Player ID and the Unit ID (there are two different methods of storing them,
		# 1235X1 and ABCD-A
		try:
			strUnitID, strPlayerID = UnitID.split("X")
			iPlayer = int(strPlayerID)
			iUnitID = int(strUnitID)
		except:
			strUnitID, strPlayerID = UnitID.split("-")
			iPlayer = self.alphaToNumber(strPlayerID)
			iUnitID = self.alphaToNumber(strUnitID)

		# Get the list of dead units
		graveyardlist = []
		if ((g_bTrackAllPlayers or gc.getGame().isGameMultiPlayer()) and g_bShowAllPlayers):
			for iLoopPlayer in range(gc.getMAX_PLAYERS()):
				if (gc.getPlayer(iLoopPlayer).isEverAlive()):
					newlist = sdObjectGetVal("UnitStats", gc.getPlayer(iLoopPlayer), GRAVEYARDLIST)
					if (len(newlist) > 0):
						graveyardlist.extend(newlist)
		else:
			graveyardlist = sdObjectGetVal("UnitStats", gc.getActivePlayer(), GRAVEYARDLIST)

		UnitID = str(iUnitID)+ "X" + str(iPlayer)
		# Get the actual object. If the unit object is invalid, it returns the plot object
		# in which the lost unit's data is saved.
		try:
			object = gc.getPlayer(iPlayer).getUnit(iUnitID)
			unittype = object.getUnitType()
			if unittype == -1:
				playergraveyardlist = sdObjectGetVal("UnitStats", gc.getPlayer(iPlayer), GRAVEYARDLIST)
				object = gc.getMap().plot(playergraveyardlist.index(UnitID), iPlayer)
			return iUnitID, iPlayer, object
		except:
			playergraveyardlist = sdObjectGetVal("UnitStats", gc.getPlayer(iPlayer), GRAVEYARDLIST)
			object = gc.getMap().plot(playergraveyardlist.index(UnitID), iPlayer)
			return iUnitID, iPlayer, object


	# This is just a placeholder.
	def isModOption(self, argument):
#		if gc.getUSE_UNIT_STATISTICS_CALLBACK():
		return True
	#	else:
#			return False

	# Due to a bug with too many dictionaries within each other, the high scores
	# will be stored in plot objects from now on. This function assigns a plot
	# to each of the different high score lists necessary and then runs the normal
	# SdToolKit code for them.
	def getHighScoreVal(self, ModID, playerobject, strHighScoreListType, htype):

		mapwidth = gc.getMap().getGridWidth()

		# Depending on the high score list type passed in, a different X coordinate
		# is selected for the plot.
		if strHighScoreListType == "HighscoresHoF":
			plotX = mapwidth - 1
		elif strHighScoreListType == "Highscores":
			plotX = mapwidth - 2
		elif strHighScoreListType == "HighscoreUnits":
			plotX = mapwidth - 3
		elif strHighScoreListType == "HighscoreUnitsHoF":
			plotX = mapwidth - 4

		# Get the plot object.
		objPlot = gc.getMap().plot(plotX, playerobject.getID())

		# Initialize the object if necessary.
		if (sdObjectExists(ModID, objPlot) == False):
			sdObjectInit(ModID, objPlot, HighScoreTypes)

		return sdObjectGetVal(ModID, objPlot, htype)

	# Due to a bug with too many dictionaries within each other, the high scores
	# will be stored in plot objects from now on. This function assigns a plot
	# to each of the different high score lists necessary and then runs the normal
	# SdToolKit code for them.
	def setHighScoreVal(self, ModID, playerobject, strHighScoreListType, htype, val):

		mapwidth = gc.getMap().getGridWidth()

		# Depending on the high score list type passed in, a different X coordinate
		# is selected for the plot.
		if strHighScoreListType == "HighscoresHoF":
			plotX = mapwidth - 1
		elif strHighScoreListType == "Highscores":
			plotX = mapwidth - 2
		elif strHighScoreListType == "HighscoreUnits":
			plotX = mapwidth - 3
		elif strHighScoreListType == "HighscoreUnitsHoF":
			plotX = mapwidth - 4

		# Get the plot object.
		objPlot = gc.getMap().plot(plotX, playerobject.getID())

		# Initialize the object if necessary.
		if (sdObjectExists(ModID, objPlot) == False):
			sdObjectInit(ModID, objPlot, HighScoreTypes)

		sdObjectSetVal(ModID, objPlot, htype, val)


	def combatStrFunction(self, objUnit, bAttacking):
		if (objUnit.getDomainType() == DomainTypes.DOMAIN_AIR):
			return objUnit.airBaseCombatStr()
#		elif g_bFfHMode:
#			return objUnit.baseCombatStr(bAttacking)
		else:
			return objUnit.baseCombatStr()


	# For now, excludes spells from being tracked by unit statistics.
	def isNotSpell(self, objUnit):

		if g_bFfHMode:

			if objUnit.getDuration() > 0:
				return False
			else:
				return True

		else:
			return True

	def determineTrackedUnits(self, attackerUnit, defenderUnit):

		if((g_bTrackAllPlayers or gc.getGame().isGameMultiPlayer())):
			if (self.isNotSpell(attackerUnit) and self.isNotSpell(defenderUnit)) or g_bFfHTrackSpells:
				return 0
			elif not self.isNotSpell(attackerUnit):
				return 2
			elif not self.isNotSpell(defenderUnit):
				return 1

		elif(attackerUnit.getOwner() == gc.getGame().getActivePlayer() and (self.isNotSpell(attackerUnit) or g_bFfHTrackSpells)):
			return 1

		elif(defenderUnit.getOwner() == gc.getGame().getActivePlayer() and (self.isNotSpell(defenderUnit) or g_bFfHTrackSpells)):
			return 2

		else:
			return None

	# Gets the string needed for the COMBATCOUNT list.
	def returnTypeOfCombat(self, iTypeofAttack, bIsAttacker, bIsAir):
		# iTypeofAttack
		# 0: (empty, reserved for total damage)
		# 1: Attacker is hit
		# 2: Defender is hit
		# 3: Collateral damage (defender is hit)
		# 4: Flanking strike (defender is hit)
		# 5: Air Strike (defender is hit)
		# 6: Other (FfH magic?) (defender is hit)

		if iTypeofAttack == 1 or iTypeofAttack == 2:
			if not bIsAir:
				if bIsAttacker:
					return ATTACK
				else:
					return DEFENSE
			else:
				if bIsAttacker:
					return AIRATTACK
				else:
					return AIRDEFENSE

		if iTypeofAttack == 3:
			return COLLATERAL

		if iTypeofAttack == 4:
			return FLANKING

		if iTypeofAttack == 5:
			return AIRSTRIKE

		if iTypeofAttack == 6:
			return OTHER

	# Prevents errors arising from units without UnitStats information
	def checkUnitListIntegrity(self, UnitList):

		for objUnit in UnitList:

			# Create an empty data sheet for the unit
			# if for some reason we haven't been logging it
			if(sdObjectExists("UnitStats", objUnit) == False):
				self.setupUnitStats(objUnit)

	# creates the scriptdata dummy files
	def setupUnitStats(self, objUnit):

		if (sdObjectExists("UnitStats", objUnit) == False):
			sdObjectInit("UnitStats", objUnit, UnitStatsData)

		if(sdObjectExists("UnitStats", gc.getPlayer(objUnit.getOwner())) == False and gc.getPlayer(objUnit.getOwner()) != None):
			sdObjectInit("UnitStats", gc.getPlayer(objUnit.getOwner()), PlayerStatsData)