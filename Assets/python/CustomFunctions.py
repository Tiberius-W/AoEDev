## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005

from CvPythonExtensions import *
from BasicFunctions import *
import CvUtil
import Popup as PyPopup
import CvScreenEnums
import CvEventInterface

class CustomFunctions:
	def __init__(self):
		# Dictionaries
		self.Defines			= {}
		self.Eras				= {}
		self.Techs				= {}
		self.Victories			= {}
		self.GameSpeeds			= {}
		self.GameOptions		= {}
		self.EventTriggers		= {}
		# Civs, etc
		self.Civilizations		= {}
		self.Leaders			= {}
		self.LeaderStatus		= {}
		self.Traits				= {}
		self.Civics				= {}
		self.Religions			= {}
		self.Corporations		= {}
		self.Alignments			= {}
		# Buildings, etc
		self.Projects			= {}
		self.Buildings			= {}
		self.Specialists		= {}
		self.BuildingClasses	= {}
		self.Processes			= {}
		# Improvements, etc
		self.Lairs				= {}
		self.ManaNodes			= {}
		self.Improvements		= {}
		self.CivImprovements	= {}
		self.UniqueImprovements	= {}
		# Terrain, etc
		self.Mana				= {}
		self.Terrain			= {}
		self.Feature			= {}
		self.Resources			= {}
		self.WorldSizes			= {}
		# Units, etc
		self.Units				= {}
		self.Heroes				= {}
		self.UnitAI				= {}
		self.Promotions			= {}
		self.UnitClasses		= {}
		self.UnitCombats		= {}
		self.GreatPeople		= {}
		self.DamageTypes		= {}

	def initialize(self):
		Manager					= CvEventInterface.getEventManager()
		self.Defines			= Manager.Defines
		self.Eras				= Manager.Eras
		self.Techs				= Manager.Techs
		self.Victories			= Manager.Victories
		self.GameSpeeds			= Manager.GameSpeeds
		self.GameOptions		= Manager.GameOptions
		self.EventTriggers		= Manager.EventTriggers

		self.Civilizations		= Manager.Civilizations
		self.Leaders 			= Manager.Leaders
		self.LeaderStatus		= Manager.LeaderStatus
		self.Traits 			= Manager.Traits
		self.Civics 			= Manager.Civics
		self.Religions 			= Manager.Religions
		self.Corporations		= Manager.Corporations
		self.Alignments			= Manager.Alignments

		self.Projects			= Manager.Projects
		self.Buildings			= Manager.Buildings
		self.Specialists		= Manager.Specialists
		self.BuildingClasses	= Manager.BuildingClasses
		self.Processes			= Manager.Processes

		self.Lairs				= Manager.Lairs
		self.ManaNodes			= Manager.ManaNodes
		self.Improvements		= Manager.Improvements
		self.CivImprovements	= Manager.CivImprovements
		self.UniqueImprovements	= Manager.UniqueImprovements

		self.Mana				= Manager.Mana
		self.Terrain			= Manager.Terrain
		self.Feature			= Manager.Feature
		self.Resources			= Manager.Resources
		self.WorldSizes			= Manager.WorldSizes
		self.Goodies			= Manager.Goodies

		self.Units				= Manager.Units
		self.Heroes				= Manager.Heroes
		self.UnitAI				= Manager.UnitAI
		self.UnitClasses		= Manager.UnitClasses
		self.UnitCombats		= Manager.UnitCombats
		self.GreatPeople		= Manager.GreatPeople
		self.Promotions			= Manager.Promotions
		self.DamageTypes		= Manager.DamageTypes

	def showAutoPlayPopup(self):
		'Window for when user switches to AI Auto Play'
		popupSizeX	= 400
		popupSizeY	= 200
		screen		= CyGInterfaceScreen("MainInterface", CvScreenEnums.MAIN_INTERFACE)
		xRes		= screen.getXResolution()
		yRes		= screen.getYResolution()
		popup		= PyPopup.PyPopup(CvUtil.EventSetTurnsAutoPlayPopup, contextType = EventContextTypes.EVENTCONTEXT_ALL)
		popup.setPosition((xRes - popupSizeX) / 2, (yRes - popupSizeY) / 2 - 50)
		popup.setSize(popupSizeX, popupSizeY)
		popup.setHeaderString(CyTranslator().getText("TXT_KEY_AIAUTOPLAY_TURN_ON", ()))
		popup.setBodyString(CyTranslator().getText("TXT_KEY_AIAUTOPLAY_TURNS", ()))
		popup.addSeparator()
		popup.createPythonEditBox('10', 'Number of turns to turn over to AI', 0)
		popup.setEditBoxMaxCharCount(4, 2, 0)
		popup.addSeparator()
		popup.addButton("OK")
		popup.addButton(CyTranslator().getText("TXT_KEY_AIAUTOPLAY_CANCEL", ()))
		popup.launch(False, PopupStates.POPUPSTATE_IMMEDIATE)

	def showUnitPerTilePopup(self):
		'Window for when user switches to AI Auto Play'
		popupSizeX	= 400
		popupSizeY	= 250
		screen		= CyGInterfaceScreen("MainInterface", CvScreenEnums.MAIN_INTERFACE)
		xRes		= screen.getXResolution()
		yRes		= screen.getYResolution()
		popup		= PyPopup.PyPopup(CvUtil.EventSetUnitPerTilePopup, contextType = EventContextTypes.EVENTCONTEXT_ALL)
		popup.setPosition((xRes - popupSizeX) / 2, (yRes - popupSizeY) / 2 - 50)
		popup.setSize(popupSizeX, popupSizeY)
		popup.setHeaderString(CyTranslator().getText("TXT_KEY_UPT", ()))
		popup.setBodyString(CyTranslator().getText("TXT_KEY_UPT_VALUE", ()))
		popup.addSeparator()
		popup.createPythonEditBox('8', 'Number of Units per Tile', 0)
		popup.setEditBoxMaxCharCount(4, 2, 0)
		popup.addSeparator()
		popup.addButton("OK")
		popup.addButton("Lock xUPT Value")
		popup.addButton(CyTranslator().getText("TXT_KEY_AIAUTOPLAY_CANCEL", ()))
		popup.launch(False, PopupStates.POPUPSTATE_IMMEDIATE)

	def doCrusade(self, iPlayer):
		gc				= CyGlobalContext()
		iCrusadeChance	= self.Defines["Crusade Spawn"]
		pPlayer 		= gc.getPlayer(iPlayer)
		iSouth			= DirectionTypes.DIRECTION_SOUTH
		iNoAI			= UnitAITypes.NO_UNITAI
		Improvement		= self.Improvements
		Unit 			= self.Units["Bannor"]
		initUnit 		= pPlayer.initUnit
		for i in xrange(CyMap().numPlots()):
			pPlot = CyMap().plotByIndex(i)
			if pPlot.getImprovementType() != Improvement["Town (IV)"]:		continue
			if pPlot.getOwner() != iPlayer:									continue
			if CyGame().getSorenRandNum(100, "Crusade") >= iCrusadeChance:	continue
			newUnit = initUnit(Unit["Demagog"], pPlot.getX(), pPlot.getY(), iNoAI, iSouth)
			pPlot.setImprovementType(Improvement["Village (III)"])

	def doForestPush(self, pVictim, pPlot, pCaster, bResistable):
		gc		= CyGlobalContext()
		pPlayer	= gc.getPlayer(pVictim.getOwner())
		if pPlayer.isHasTech(self.Techs["Iron Working"]): return False
		if pPlayer.countNumBuildings(self.Buildings["Mines of Galdur"]) > 0: return False
		iX		= pVictim.getX()
		iY		= pVictim.getY()
		iOwner	= pVictim.getOwner()
		pVPlot	= CyMap().plot(iX,iY)
		if iOwner == gc.getANIMAL_PLAYER(): return False
		if pVPlot.getFeatureType() not in (self.Feature["Forest"], self.Feature["Jungle"], self.Feature["Forest New"]): return False
		pBestPlot	= -1
		iBestPlot	= 0
		for iiX,iiY in SURROUND1:
			pLoopPlot = CyMap().plot(iX+iiX,iY+iiY)
			if pLoopPlot.isNone():												continue
			if pLoopPlot.isVisibleEnemyUnit(iOwner):							continue
			if not pVictim.canMoveOrAttackInto(pLoopPlot, False):				continue
			if pLoopPlot.getImprovementType() == self.Lairs["Blighted Forest"]:	continue
			iRnd		= CyGame().getSorenRandNum(500, "Forest Push Scatter choose Plot")
			if iRnd <= iBestPlot:												continue
			iBestPlot	= iRnd
			pBestPlot	= pLoopPlot
		if pBestPlot != -1:
			pVictim.setXY(pBestPlot.getX(), pBestPlot.getY(), False, True, True, False)
			if pVictim.getOwner() == gc.getORC_PLAYER():
				pVictim.doDamageNoCaster(20, 100, self.DamageTypes["Physical"], False)
			return True
		return False

	def formEmpire(self, iCiv, iLeader, iTeam, pCity, iAlignment, pFromPlayer):
		gc 		= CyGlobalContext()
		iPlayer = getOpenPlayer()
		pPlot 	= pCity.plot()
		pPlot2 	= findClearPlot(-1, pCity.plot())
		if iPlayer == -1 or pPlot2.isNone(): return
		iX = pPlot2.getX(); iY = pPlot2.getY()
		for i in xrange(pPlot.getNumUnits(), -1, -1):
			pUnit = pPlot.getUnit(i)
			pUnit.setXY(iX, iY, True, True, True, False)
		CyGame().addPlayerAdvanced(iPlayer, iTeam, iLeader, iCiv, pFromPlayer.getID())
		pPlayer	= gc.getPlayer(iPlayer)
		iTeam	= pPlayer.getTeam()
		pTeam	= gc.getTeam(iTeam)
		if pFromPlayer != -1:
			iFromTeam	= pFromPlayer.getTeam()
			pFromTeam	= gc.getTeam(iFromTeam)
			hasTech		= pFromTeam.isHasTech
			for iTech in xrange(gc.getNumTechInfos()):
				if not hasTech(iTech): continue
				pTeam.setHasTech(iTech, True, iPlayer, True, False)
			if iFromTeam != iTeam:
				if not pFromTeam.isAtWar(gc.getORC_TEAM()):
					pFromTeam.makePeace(gc.getORC_TEAM())
				if not pFromTeam.isAtWar(gc.getANIMAL_TEAM()):
					pFromTeam.makePeace(gc.getANIMAL_TEAM())
				if not pFromTeam.isAtWar(gc.getDEMON_TEAM()):
					pFromTeam.makePeace(gc.getDEMON_TEAM())
		pPlayer.acquireCity(pCity, False, False)
		pCity = pPlot.getPlotCity()
		pCity.changeCulture(iPlayer, 100, True)
		iX = pPlot.getX(); iY = pPlot.getY();
		iNoAI 	= UnitAITypes.NO_UNITAI
		iSouth 	= DirectionTypes.DIRECTION_SOUTH
		pPlayer.initUnit(self.Units["Generic"]["Archer"], iX, iY, iNoAI, iSouth)
		pPlayer.initUnit(self.Units["Generic"]["Archer"], iX, iY, iNoAI, iSouth)
		pPlayer.initUnit(self.Units["Generic"]["Archer"], iX, iY, iNoAI, iSouth)
		pPlayer.initUnit(self.Units["Generic"]["Archer"], iX, iY, iNoAI, iSouth)
		pPlayer.initUnit(self.Units["Generic"]["Archer"], iX, iY, iNoAI, iSouth)
		if iAlignment != -1:
			pPlayer.setAlignment(iAlignment)

	def doCityFire(self, pCity):
		gc 				= CyGlobalContext()
		iCount 			= 0
		iOwner 			= pCity.getOwner()
		iX = pCity.getX(); iY = pCity.getY()
		iMessage		= InterfaceMessageTypes.MESSAGE_TYPE_MINOR_EVENT
		iGreen			= gc.getInfoTypeForString("COLOR_GREEN")
		iRed			= gc.getInfoTypeForString("COLOR_RED")
		for iBuilding in xrange(gc.getNumBuildingInfos()):
			if iBuilding == self.Buildings["Demonic Citizens"]:								continue
			if pCity.getNumRealBuilding(iBuilding) <= 0:									continue
			if gc.getBuildingInfo(iBuilding).getConquestProbability() == 100:				continue
			if isLimitedWonderClass(gc.getBuildingInfo(iBuilding).getBuildingClassType()):	continue
			if CyGame().getSorenRandNum(100, "City Fire") > 10:								continue
			pCity.setNumRealBuilding(iBuilding, 0)

			szButton	= gc.getBuildingInfo(iBuilding).getButton()
			szText		= CyTranslator().getText("TXT_KEY_MESSAGE_CITY_FIRE",(gc.getBuildingInfo(iBuilding).getDescription(), ))
			CyInterface().addMessage(iOwner, True, 25, szText, '', iMessage, szButton, iRed, iX, iY, True, True)
			iCount += 1
		if iCount == 0:
			szButton	= 'Art/Interface/Buttons/Fire.dds'
			szText		= CyTranslator().getText("TXT_KEY_MESSAGE_CITY_FIRE_NO_DAMAGE",())
			CyInterface().addMessage(iOwner, True, 25, szText, '', iMessage, szButton, iGreen, iX, iY, True, True)

	### TODO: Dictionaries
	def doFFTurn(self):
		iAreThereLizardsHere	= CyGame().getNumCivActive(self.Civilizations["Mazatl"]) + CyGame().getNumCivActive(self.Civilizations["Cualli"])
		iAreThereScionsHere		= CyGame().getNumCivActive(self.Civilizations["Scions"])
		if not (iAreThereLizardsHere or iAreThereScionsHere or not self.GameOptions["No Plot Counter"]): return
		gc				= CyGlobalContext()
		git				= gc.getInfoTypeForString
		iPlotTreshold	= gc.getDefineINT("PLOT_COUNTER_HELL_THRESHOLD")
		iJungleChance 	= 10
		iSwampChance 	= 25
		iHLSeed			= 0
		if iAreThereScionsHere:
			for iLoopPlayer in xrange(gc.getMAX_PLAYERS()):
				pLoopPlayer	= gc.getPlayer(iLoopPlayer)
				iLoopCiv	= pLoopPlayer.getCivilizationType()
				if iLoopCiv != self.Civilizations["Scions"]: continue
				iNatureMana = pLoopPlayer.getNumAvailableBonuses(self.Mana["Nature"])
				iManaMod = 1 + (iNatureMana * 0.1)
				iGhostwalkerFactor	= pLoopPlayer.getUnitClassCount(self.UnitClasses["Ranger"])
				iHauntFactor		= pLoopPlayer.getUnitClassCount(self.UnitClasses["Haunt"]) * 2
				iRedactorFactor		= pLoopPlayer.getUnitClassCount(self.UnitClasses["Druid"]) * 4
				iBlackLadyFactor	= pLoopPlayer.getUnitClassCount(self.UnitClasses["Korrina Black"]) + 1
				iHLSeed = (iGhostwalkerFactor + iHauntFactor + iRedactorFactor) * iBlackLadyFactor * iManaMod
				break

		for i in xrange(CyMap().numPlots()):
			pPlot			 	= CyMap().plotByIndex(i)
			if pPlot == None: continue
			iPlotCounter		= pPlot.getPlotCounter()
			iBonus 				= pPlot.getBonusType(-1)
			iFeature 			= pPlot.getFeatureType()
			iPlotEffect 		= pPlot.getPlotEffectType()
			iImprovement 		= pPlot.getImprovementType()
			iTerrain 			= pPlot.getTerrainType()
			bIsOwned 			= pPlot.isOwned()
			bPeak				= pPlot.isPeak()
			bCity				= pPlot.isCity()
			iConvert			= CyGame().getSorenRandNum(100, "Hell Convert")
			if bIsOwned:
				iPlayer 		= pPlot.getOwner()
				pPlayer 		= gc.getPlayer(iPlayer)
				eCiv 			= pPlayer.getCivilizationType()

				## Lizard Terrain Section
				if eCiv in (self.Civilizations["Mazatl"], self.Civilizations["Cualli"]):
					if pPlot.getRouteType() == self.Improvements["Road"]:
						if CyGame().getSorenRandNum(100, "Trail") < 20:
							pPlot.setRouteType(self.Improvements["Trail"])

					if iTerrain == self.Terrain["Marsh"]:
						if iImprovement == -1 and iBonus == -1 and not bCity and not bPeak:
							if not pPlayer.isHuman(): 	iSwampChance *=2
							if CyGame().getSorenRandNum(1000, "Swamp") < iSwampChance:
								pPlot.setImprovementType(self.Improvements["Swamp"])
						if   iFeature == self.Feature["Forest"] or iFeature == self.Feature["Ancient Forest"]:
							iJungleChance *= 5
						elif iFeature != -1:
							iJungleChance = 0
						if CyGame().getSorenRandNum(1000, "Jungle") < iJungleChance:
							pPlot.setFeatureType(self.Feature["Jungle"], 0)

				#### Haunted Lands Section
				elif eCiv == self.Civilizations["Scions"]:
					if iPlotEffect != self.Feature["Haunted Lands"] and iHLSeed > 0 and not bPeak and not bCity:
						if   iFeature in (self.Feature["Forest"], self.Feature["Ancient Forest"], self.Feature["Jungle"]): 
							iHLSeed *= 1.5
						elif iFeature == self.Feature["Flood Plains"] or iTerrain in (self.Terrain["Grass"], self.Terrain["Plains"], self.Terrain["Marsh"]):
							iHLSeed = iHLSeed
						elif iTerrain == self.Terrain["Desert"]:
							iHLSeed *= 0.5
						else:
							iHLSeed = 0
						if CyGame().getSorenRandNum(1100, "Chance for HL") < iHLSeed:
							pPlot.setPlotEffectType(self.Feature["Haunted Lands"])

			if iImprovement == self.Improvements["Swamp"] and iTerrain != self.Terrain["Marsh"]: pPlot.setImprovementType(-1)

			## Hell Terrain Section
			if not self.GameOptions["No Plot Counter"]:
				if iPlotCounter > iPlotTreshold and not iTerrain in (self.Terrain["Tundra"], self.Terrain["Taiga"]):
					if   iBonus in (self.Resources["Sheep"], self.Resources["Pig"], self.Resources["Fur"], self.Resources["Deer"]):
						pPlot.setBonusType(self.Resources["Toad"])
					elif iBonus in (self.Resources["Horse"], self.Resources["Cow"], self.Resources["Camel"], self.Resources["Ivory"]):
						pPlot.setBonusType(self.Resources["Nightmare"])
					elif iBonus in (self.Resources["Cotton"], self.Resources["Silk"], self.Resources["Wine"]):
						pPlot.setBonusType(self.Resources["Razorweed"])
					elif iBonus in (self.Resources["Fish"], self.Resources["Shrimp"], git('BONUS_WHALE')):
						pPlot.setBonusType(git('BONUS_JETEYE'))
					elif iBonus in (self.Resources["Clam"], self.Resources["Crab"], self.Resources["Pearl"]):
						pPlot.setBonusType(git('BONUS_BLEEDING_GOD_WINE'))
					elif iBonus in (self.Resources["Banana"], self.Resources["Sugar"]):
						pPlot.setBonusType(self.Resources["Gulagarm"])
					elif iBonus == self.Resources["Marble"]:
						pPlot.setBonusType(self.Resources["Sheut"])
					elif iBonus in (self.Resources["Corn"], self.Resources["Rice"], self.Resources["Wheat"]):
						pPlot.setBonusType(-1)
						pPlot.setImprovementType(self.Improvements["Snake Pillar"])
				elif iPlotCounter <= iPlotTreshold:
					if iImprovement == self.Improvements["Snake Pillar"]:
						pPlot.setImprovementType(-1)
						if   iConvert < 33:	pPlot.setBonusType(self.Resources["Corn"])
						elif iConvert < 67:	pPlot.setBonusType(self.Resources["Rice"])
						else:				pPlot.setBonusType(self.Resources["Wheat"])
				if iTerrain == self.Terrain["Burning sands"] and not bCity and not bPeak:
					if CyGame().getSorenRandNum(100, "Flames") <= self.Defines["Flame Spread"]:
						pPlot.setFeatureType(self.Feature["Flames"], 0)

	def doTurnKhazad(self, iPlayer):
		gc			= CyGlobalContext()
		pPlayer		= gc.getPlayer(iPlayer)
		iNumCities	= pPlayer.getNumCities()
		if iNumCities <= 0: return
		iGold = pPlayer.getGold() / iNumCities
		if 		iGold <= 49:						iNewVault = self.Buildings["Vault1"]
		elif	(iGold >= 50 and iGold <= 99):		iNewVault = self.Buildings["Vault2"]
		elif	(iGold >= 100 and iGold <= 149):	iNewVault = self.Buildings["Vault3"]
		elif	(iGold >= 150 and iGold <= 199):	iNewVault = self.Buildings["Vault4"]
		elif	(iGold >= 200 and iGold <= 299):	iNewVault = self.Buildings["Vault5"]
		elif	(iGold >= 300 and iGold <= 499):	iNewVault = self.Buildings["Vault6"]
		elif	iGold >= 500:						iNewVault = self.Buildings["Vault7"]

		for iCity in xrange(pPlayer.getNumCities()):
			pCity = pPlayer.getCity(iCity)
			pCity.setNumRealBuilding(self.Buildings["Vault1"], 0)
			pCity.setNumRealBuilding(self.Buildings["Vault2"], 0)
			pCity.setNumRealBuilding(self.Buildings["Vault3"], 0)
			pCity.setNumRealBuilding(self.Buildings["Vault4"], 0)
			pCity.setNumRealBuilding(self.Buildings["Vault5"], 0)
			pCity.setNumRealBuilding(self.Buildings["Vault6"], 0)
			pCity.setNumRealBuilding(self.Buildings["Vault7"], 0)
			pCity.setNumRealBuilding(iNewVault, 1)

	def doTurnLuchuirp(self, iPlayer):
		gc			= CyGlobalContext()
		pPlayer		= gc.getPlayer(iPlayer)
		if pPlayer.getUnitClassCount(self.Heroes["Class-Barnaxus"]) <= 0: return
		pBarnaxus	= -1
		lGolems		= []

		for iUnit in xrange(pPlayer.getNumUnits()):
			pUnit = pPlayer.getUnit(iUnit)
			
			if   pUnit.getUnitType() == self.Heroes["Barnaxus"]:
				pBarnaxus = pUnit
			elif pUnit.isHasPromotion(self.Promotions["Race"]["Golem"]) and not pUnit.isHasPromotion(self.Promotions["Generic"]["Empower V"]):
				lGolems.append(iUnit)

		if pBarnaxus == -1 or not lGolems: return
		else:
			bEmp1	= pBarnaxus.isHasPromotion(self.Promotions["Generic"]["Combat I"])
			bEmp2	= pBarnaxus.isHasPromotion(self.Promotions["Generic"]["Combat II"])
			bEmp3	= pBarnaxus.isHasPromotion(self.Promotions["Generic"]["Combat III"])
			bEmp4	= pBarnaxus.isHasPromotion(self.Promotions["Generic"]["Combat IV"])
			bEmp5	= pBarnaxus.isHasPromotion(self.Promotions["Generic"]["Combat V"])

		for iUnit in lGolems:
			pUnit = pPlayer.getUnit(iUnit)
			if bEmp1: pUnit.setHasPromotion(self.Promotions["Generic"]["Empower I"], True)
			if bEmp2: pUnit.setHasPromotion(self.Promotions["Generic"]["Empower II"], True)
			if bEmp3: pUnit.setHasPromotion(self.Promotions["Generic"]["Empower III"], True)
			if bEmp4: pUnit.setHasPromotion(self.Promotions["Generic"]["Empower IV"], True)
			if bEmp5: pUnit.setHasPromotion(self.Promotions["Generic"]["Empower V"], True)

	def doTurnArchos(self, iPlayer):
		gc			= CyGlobalContext()
		pPlayer		= gc.getPlayer(iPlayer)
		if pPlayer.getNumCities() <= 0: return
		pNest		= pPlayer.getCapitalCity()
		iNestPop	= pNest.getPopulation()
		if iNestPop < 4: return
		iNoAI		= UnitAITypes.NO_UNITAI
		iSouth		= DirectionTypes.DIRECTION_SOUTH
		iX = pNest.getX(); iY = pNest.getY()
		self.doChanceArchos(iPlayer)
		iSpawnChance			= pPlayer.getCivCounter()
		iScorpionSpawnChance	= pPlayer.getCivCounterMod()

		bSpider		= False
		bScorpion	= False
		if CyGame().getSorenRandNum(10000, "Spawn Archos Spider") < iSpawnChance:
			bSpider		= True
		elif CyGame().getSorenRandNum(10000, "Spawn Archos Scorpion") < iScorpionSpawnChance:
			bScorpion	= True
		if bSpider == False and bScorpion == False: return

		if   iNestPop >= 12:
			iUnit				= self.Units["Archos"]["Giant Spider"]
			if bScorpion: iUnit	= self.Units["Archos"]["Giant Scorpion"]
		elif iNestPop >= 8:
			iUnit				= self.Units["Archos"]["Spider"]
			if bScorpion: iUnit	= self.Units["Archos"]["Scorpion Swarm"]
		else:
			iUnit				= self.Units["Archos"]["Baby Spider"]
			if bScorpion: iUnit	= self.Units["Archos"]["Scorpion"]

		spawnUnit = pPlayer.initUnit(iUnit, iX, iY, iNoAI, iSouth)

		if pPlayer.hasTrait(self.Traits["Spiderkin"]) and iNestPop >= 10:
			spawnUnit.setHasPromotion(self.Promotions["Effects"]["Spiderkin"], True)

		if iNestPop >= 16:
			spawnUnit.setHasPromotion(self.Promotions["Effects"]["Strong"], True)

		szText		= CyTranslator().getText("TXT_KEY_MESSAGE_SPIDER_BORN_IN_NEST",())
		szArt		= gc.getUnitInfo(iUnit).getButton()
		iGreen		= gc.getInfoTypeForString("COLOR_GREEN")
		szSound		= 'AS2D_DISCOVERBONUS'
		iMessage	= InterfaceMessageTypes.MESSAGE_TYPE_MINOR_EVENT
		CyInterface().addMessage(iPlayer, True, 25, szText, szSound, iMessage, szArt, iGreen, iX, iY, True, True)
		pNest.applyBuildEffects(spawnUnit)

		if   pNest.getNumBuilding(self.Buildings["Nest Addon1"]) > 0:
			iBroodStrength = 1
		elif pNest.getNumBuilding(self.Buildings["Nest Addon2"]) > 0:
			iBroodStrength = 2
		elif pNest.getNumBuilding(self.Buildings["Nest Addon3"]) > 0:
			iBroodStrength = 3
		elif pNest.getNumBuilding(self.Buildings["Nest Addon4"]) > 0:
			iBroodStrength = 4
		else:
			iBroodStrength = 0
		spawnUnit.changeFreePromotionPick(iBroodStrength)

	def doChanceArchos(self, iPlayer):
		gc				= CyGlobalContext()
		pPlayer			= gc.getPlayer(iPlayer)
		if pPlayer.getNumCities() <= 0: return
		pNest			= pPlayer.getCapitalCity()
		iNestPop		= pNest.getPopulation()
		iX				= pNest.getX()
		iY				= pNest.getY()
		iNumNestHills	= 0
		iNumGroves		= pPlayer.countNumBuildings(self.Buildings["Dark Weald"])
		iNumSpiders		= ((pPlayer.getUnitClassCount(self.UnitClasses["Baby Spider"]) * 0.5)	+ pPlayer.getUnitClassCount(self.UnitClasses["Spider"])			+ (pPlayer.getUnitClassCount(self.UnitClasses["Giant Spider"]) * 2))
		iNumScorpions 	= ((pPlayer.getUnitClassCount(self.UnitClasses["Scorpion"]) * 0.5)		+ pPlayer.getUnitClassCount(self.UnitClasses["Scorpion Swarm"])	+ (pPlayer.getUnitClassCount(self.UnitClasses["Giant Scorpion"]) * 2))

		iNumScorpionCities = 0
		iNumSpiderCities = 0

		for iCity in xrange(pPlayer.getNumCities()):
			pCity = pPlayer.getCity(iCity)
			pCityPlot = pCity.plot()
			if pCityPlot.getTerrainType() == self.Terrain["Desert"]:
				iNumScorpionCities += 1
			else:
				iNumSpiderCities += 1

		for iiX,iiY in BFC:
			pLoopPlot = CyMap().plot(iX+iiX,iY+iiY)
			if pLoopPlot.isNone():		continue
			if not pLoopPlot.isHills():	continue
			iNumNestHills += 1

		fSpiderkin = 1
		if pPlayer.hasTrait(self.Traits["Spiderkin"]):
			fSpiderkin = 1.30

		iSpiderSpawnChance = ((iNestPop + (iNumSpiderCities*2) + (iNumGroves*4)) * fSpiderkin) - iNumSpiders
		iSpiderSpawnChance = (iSpiderSpawnChance * 100)
		iSpiderSpawnChance = scaleInverse(iSpiderSpawnChance)

		pPlayer.setCivCounter(iSpiderSpawnChance)

		iScorpionSpawnChance = (iNestPop + (iNumScorpionCities*2) + (iNumNestHills)) - iNumScorpions
		iScorpionSpawnChance = (iScorpionSpawnChance * 100)
		iScorpionSpawnChance = scaleInverse(iScorpionSpawnChance)

		pPlayer.setCivCounterMod(iScorpionSpawnChance)

	### Scions Start
	def doTurnScions(self, iPlayer):
		gc			= CyGlobalContext()
		pPlayer		= gc.getPlayer(iPlayer)
		pTomb		= pPlayer.getCapitalCity()
		if pTomb.isNone(): return
		self.doChanceAwakenedSpawn(iPlayer)
		iSpawnOdds	= pPlayer.getCivCounter()
		iTombPop	= pTomb.getPopulation()
		iTeam		= pPlayer.getTeam()
		pTeam		= gc.getTeam(iTeam)
		iNumCities	= pPlayer.getNumCities()

		iNumOpenBorders = (pTeam.getOpenBordersTradingCount()) - 1
		for iLoopCiv in xrange(gc.getMAX_CIV_PLAYERS()):
			if not (pTeam.isOpenBorders(iLoopCiv)): continue
			iNumOpenBorders += 1
		if pPlayer.isCivic(self.Civics["God King"]):
			iNumOpenBorders += 1
		iNumOpenBorders = min(iNumOpenBorders, 7)
		lOBB = [self.Buildings["Obbuilding1"], self.Buildings["Obbuilding2"], self.Buildings["Obbuilding3"], self.Buildings["Obbuilding4"], self.Buildings["Obbuilding5"], self.Buildings["Obbuilding6"], self.Buildings["Obbuilding7"]]
		iOBB = -1
		if iNumOpenBorders > 0:
			iOBB = lOBB[iNumOpenBorders - 1]
		iNumHealthBonus = 0
		for iBonus in xrange(gc.getNumBonusInfos()):
			if gc.getBonusInfo(iBonus).getHealth() > 0 and pPlayer.hasBonus(iBonus):
				iNumHealthBonus += 1
		## Awakened Spawn
		if CyGame().getSorenRandNum(10000, "Awakened Spawn") < iSpawnOdds:
			pTarget = pTomb
			iNumCities = pPlayer.getNumCities()
			if not pPlayer.isHuman() and iNumCities > 1:
				if CyGame().getSorenRandNum(100, "Awakened Distribution") >= 50:
					iRnd	= CyGame().getSorenRandNum(iNumCities, "Scion City")
					pTarget	= pPlayer.getCity(iRnd)
			spawnUnit = pPlayer.initUnit(self.Units["Scions"]["Awakened"], pTarget.getX(), pTarget.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
		## Pity Pop
		if iNumCities == 1 and iTombPop == 1:
			pTomb.changePopulation(1)
		### City Start
		for iCity in xrange(pPlayer.getNumCities()):
			pCity = pPlayer.getCity(iCity)
			### Clean Buildings
			for iBuilding in lOBB:
				pCity.setNumRealBuilding(iBuilding, 0)
			pCity.setNumRealBuilding(self.Buildings["Necro Bonus1"], 0)
			pCity.setNumRealBuilding(self.Buildings["Necro Bonus2"], 0)
			pCity.setNumRealBuilding(self.Buildings["Necro Bonus3"], 0)
			pCity.setNumRealBuilding(self.Buildings["Unhealthy Discontent I"], 0)
			pCity.setNumRealBuilding(self.Buildings["Unhealthy Discontent II"], 0)
			## Set Buildings
			if pCity.getProductionUnit() == self.Units["Scions"]["Reborn"] and iNumOpenBorders > 0:
				pCity.setNumRealBuilding(iOBB, 1)
			if pCity.getNumRealBuilding(self.Buildings["Necropolis"]) > 0:
				if   iNumHealthBonus > 8:
					pCity.setNumRealBuilding(self.Buildings["Necro Bonus3"], 1)
				elif iNumHealthBonus > 5:
					pCity.setNumRealBuilding(self.Buildings["Necro Bonus2"], 1)
				elif iNumHealthBonus > 3:
					pCity.setNumRealBuilding(self.Buildings["Necro Bonus1"], 1)
			else:
				iUH = pCity.badHealth(False) - pCity.goodHealth()
				if   iUH > 7:
					pCity.setNumRealBuilding(self.Buildings["Unhealthy Discontent II"], 1)
				elif iUH > 4:
					pCity.setNumRealBuilding(self.Buildings["Unhealthy Discontent I"], 1)
		### Alcinus AI
		for iUnit in xrange(pPlayer.getNumUnits()):
			pUnit		= pPlayer.getUnit(iUnit)
			iUnitType	= pUnit.getUnitType()
			if iUnitType in (self.Heroes["Alcinus"], self.Heroes["Alcinus (Archmage)"], self.Heroes["Alcinus (Upgraded)"]):
				if pUnit.isHasPromotion(self.Promotions["Effects"]["Rampage"]):
					pUnit.setUnitAIType( UnitAITypes.UNITAI_ATTACK )
				else:
					pUnit.setUnitAIType( UnitAITypes.UNITAI_RESERVE )
		### Horned Dread Shenanigans
		pPlayer.setFeatAccomplished(FeatTypes.FEAT_MANIFEST_HORNED_DREAD, True)
		if (pPlayer.getUnitClassCount(self.UnitClasses["Ranger"]) + pPlayer.getUnitClassCount(self.UnitClasses["Haunt"])) < 1:
			pPlayer.setFeatAccomplished(FeatTypes.FEAT_MANIFEST_FIRST_HORNED_DREAD, False)

	def doChanceAwakenedSpawn(self, iPlayer):
		gc			= CyGlobalContext()
		pPlayer		= gc.getPlayer(iPlayer)
		# Stasis
		if pPlayer.getDisableProduction() > 0:
			pPlayer.setCivCounter(0)
			return
		# Low pop & cap
		tScionCap	= self.calculateScionCap(iPlayer)
		if tScionCap == -1:
			pPlayer.setCivCounter(0)
			return
		iCurPop		= tScionCap[0]
		iMaxPop		= float(tScionCap[1])
		if iCurPop > iMaxPop:
			pPlayer.setCivCounter(0)
			return
		iMultLow	= 1
		if iCurPop < 6: iMultLow = 1.5
		iMultCap	= 1
		if iCurPop * 2 > iMaxPop:
			iMultCap = (iMaxPop - iCurPop) / ( iMaxPop / 2 )
			iMultCap = max(0, iMultCap)
		# Base
		iAddBase	= 3
		# Civics
		iLeader		= pPlayer.getLeaderType()
		iAddCivic	= 0
		iMultCivic	= 1
		if   pPlayer.isCivic(self.Civics["God King"]):
			if gc.getInfoTypeForString("MODULE_MCWHK") != -1:
				if iLeader == gc.getInfoTypeForString("LEADER_KOUNPEROR"):
					iMultCivic = 1.25
			if iLeader == self.Leaders["Risen Emperor"]:
				iMultCivic = 1.25
		elif pPlayer.isCivic(self.Civics["Aristocracy"]):
			iAddCivic = 5
			if iLeader == self.Leaders["Korrina"]:
				iMultCivic = 1.25
		# AI
		iMultAI		= 1
		if not pPlayer.isHuman(): iMultAI = 2.5
		# Speed Multi
		iMultSpeed	= float(gc.getGameSpeedInfo(gc.getGame().getGameSpeedType()).getGrowthPercent()) / 100
		if iMultSpeed == 0: iMultSpeed = 1
		# Bonuses
		iAddBonus	= 0
		for iBonus in xrange(gc.getNumBonusInfos()):
			if not pPlayer.hasBonus(iBonus): continue
			if gc.getBonusInfo(iBonus).getHappiness() > 0:
				iAddBonus += 0.5
			if iBonus == self.Mana["Body"]:
				if not pPlayer.countNumBuildings(self.Buildings["Flesh Studio"]): continue
				iAddBonus += pPlayer.getNumAvailableBonuses(iBonus)
			if iBonus == self.Resources["Patrian"]:
				iAddBonus += pPlayer.getNumAvailableBonuses(iBonus)
		# Buildings
		iAddBuild	= 0
		iAddBuild	+= pPlayer.countNumBuildings(self.Buildings["Hall of the Covenant"])
		iAddBuild	+= pPlayer.countNumBuildings(self.Buildings["Imperial Cenotaph"])
		iAddBuild	+= pPlayer.countNumBuildings(self.Buildings["Temple of the Gift"])		* 0.5
		iAddBuild	+= pPlayer.countNumBuildings(self.Buildings["Shrine to Kylorin"])		* 0.5
		iAddBuild	+= pPlayer.countNumBuildings(self.Buildings["Unhealthy Discontent I"])	* -2
		iAddBuild	+= pPlayer.countNumBuildings(self.Buildings["Unhealthy Discontent II"])	* -4
		# RoP
		iAddRoP		= 0
		if pPlayer.getImprovementCount(self.UniqueImprovements["Remnants of Patria"]) > 0: iAddRoP = 5
		# Calc
		iSpawnChance = round(((iAddBase + iAddCivic + iAddBonus + iAddBuild) * iMultLow * iMultCap * iMultCivic * iMultAI / iMultSpeed ), 2)
		iSpawnChance = int(iSpawnChance * 100)
		pPlayer.setCivCounter(iSpawnChance)

	def calculateScionCap(self, iPlayer):
		if self.UnitCombats == {}: self.initialize()
		pPlayer		= CyGlobalContext().getPlayer(iPlayer)
		pTomb		= pPlayer.getCapitalCity()
		if pTomb.isNone(): return -1
		iMaxPop		= 10;
		iCurPop		= 0
		iOwnedPlots	= pPlayer.getTotalLand()
		iMaxPop		= max(iMaxPop, iOwnedPlots / 2)
		iCurPop		+= pPlayer.getTotalPopulation()
		iCurPop		+= pPlayer.getUnitClassCount(self.UnitClasses["Awakened"])
		iCurPop		+= pPlayer.getUnitClassCount(self.UnitClasses["Reborn"])
		tScionCap = (iCurPop, iMaxPop)
		return tScionCap
	### Scions End

	### Grigori Adventurer Start
	def doTurnGrigori(self, iPlayer):
		gc				= CyGlobalContext()
		pPlayer			= gc.getPlayer(iPlayer)
		pCapital		= pPlayer.getCapitalCity()
		if pCapital.isNone(): return
		iChange			= self.doChanceAdventurerSpawn(iPlayer)
		pPlayer.changeCivCounter(iChange)
		iGrigoriSpawn	= pPlayer.getCivCounter()
		iGrigoriMod		= pPlayer.getCivCounterMod()

		if iGrigoriMod < 10000:
			pPlayer.setCivCounterMod(10000)
			iGrigoriMod = 10000

		if iGrigoriSpawn >= iGrigoriMod:
			spawnUnit = pPlayer.initUnit(self.Units["Grigori"]["Adventurer"], pCapital.getX(), pCapital.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
			pPlayer.changeCivCounter(-iGrigoriMod)
			pPlayer.changeCivCounterMod(2000)

	def doChanceAdventurerSpawn(self, iPlayer):
		gc				= CyGlobalContext()
		pPlayer			= gc.getPlayer(iPlayer)
		iPalaceMod		= pPlayer.countNumBuildings(self.Buildings["Grigori Palace"])
		iDagdaMod		= pPlayer.countNumBuildings(self.Buildings["Grigori Temple"]) * 0.33
		iMuseumMod		= pPlayer.countNumBuildings(self.Buildings["Museum"]) * 0.33
		iTavernsMod		= pPlayer.countNumBuildings(self.Buildings["Tavern"])
		iGuildsMod		= pPlayer.countNumBuildings(self.Buildings["Adventurers Guild"]) * 0.5
		iRefugeMod		= pPlayer.countNumBuildings(self.Buildings["Dwelling of Refuge"])
		iNumBountyMinor	= pPlayer.countNumBuildings(self.Buildings["Crime Bounty Minor"])
		iNumBountyMajor	= pPlayer.countNumBuildings(self.Buildings["Crime Bounty Major"])
		iNumBountyGrand	= pPlayer.countNumBuildings(self.Buildings["Crime Bounty Grand"])
		iCivicMod 		= 0.00
		iCivicMult 		= 1
		if pPlayer.isCivic(self.Civics["Apprenticeship"]):
			iCivicMod	= 2
			iCivicMult	= 1.25

		# Allows Statesmen to take affect after their city has an Assembly
		iStatesmanMod 	= 0.00
		iNumStatesmen 	= 0
		for iCity in xrange(pPlayer.getNumCities()):
			pCity = pPlayer.getCity(iCity)
			if pCity.getNumBuilding(self.Buildings["Forum"]) <= 0: continue
			iNumStatesmen = (pCity.getSpecialistCount(self.Specialists["Statesman"]) + pCity.getFreeSpecialistCount(self.Specialists["Statesman"]))
			iStatesmanMod += (iNumStatesmen * 0.33)
		# AI modifier
		iAImod = 1
		if not pPlayer.isHuman():
			iAImod = 1.5

		iGrigoriSpawn = round(((iPalaceMod + iDagdaMod + iMuseumMod +iTavernsMod + iGuildsMod + iNumBountyMinor + iNumBountyMajor + iNumBountyGrand + iCivicMod + iStatesmanMod + iRefugeMod) * iCivicMult * iAImod), 2)
		iGrigoriSpawn = int(iGrigoriSpawn * 100)
		iGrigoriSpawn = scaleInverse(iGrigoriSpawn)
		return iGrigoriSpawn
	### Grigori Adventurer End

	### Mekara Aspirant Start
	def doTurnMekara(self, iPlayer):
		gc				= CyGlobalContext()
		pPlayer			= gc.getPlayer(iPlayer)
		pCapital		= pPlayer.getCapitalCity()
		if pPlayer.getLeaderType() != self.Leaders["Iram Damarr"]:	return
		if pCapital.isNone():										return
		iChange			= self.doChanceAspirantSpawn(iPlayer)
		pPlayer.changeCivCounter(iChange)
		iMekaraSpawn	= pPlayer.getCivCounter()
		iMekaraMod		= pPlayer.getCivCounterMod()

		if iMekaraMod < 10000:
			pPlayer.setCivCounterMod(10000)
			iMekaraMod = 10000

		if iMekaraSpawn >= iMekaraMod:
			spawnUnit = pPlayer.initUnit(self.Units["Mekara Order"]["Aspirant"], pCapital.getX(), pCapital.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
			pPlayer.changeCivCounter(-iMekaraMod)
			pPlayer.changeCivCounterMod(2000)

	def doChanceAspirantSpawn(self, iPlayer):
		gc				= CyGlobalContext()
		pPlayer			= gc.getPlayer(iPlayer)
		# +2 for each Lab, +1 for each Mage Guild, +2/+25% with Slavery, +3 from Humanist I, +5/+25% from Humanist II, +10/+50% from Humanist III
		iBaseMod		= 3
		iLabMod			= pPlayer.countNumBuildings(self.Buildings["Shaper Cabal"]) * 2
		iCabalMod		= pPlayer.countNumBuildings(self.Buildings["Shaper's Laboratory"])
		iCivicMod		= 0.00
		iCivicMult		= 1
		if pPlayer.isCivic(self.Civics["Slavery"]):
			iCivicMod	= 2
			iCivicMult	= 1.25
		# Allows Healers (+1 for every 2) and Elders (+1 for every 3) to take effect with Labs.
		iHealer 		= self.Specialists["Healer"]
		iElder 			= self.Specialists["Scientist"]
		iGreatHealer 	= self.Specialists["Great Healer"]
		iGreatElder 	= self.Specialists["Great Scientist"]

		iSpecialistMod		= 0.00
		iNumHealers			= 0
		iNumGreatHealers	= 0
		iNumElders			= 0
		iNumGreatElders		= 0
		for iCity in xrange(pPlayer.getNumCities()):
			pCity = pPlayer.getCity(iCity)
			if pCity.getNumBuilding(self.Buildings["Shaper's Laboratory"]) <= 0: continue
			iNumElders			= pCity.getSpecialistCount(iElder)			+ pCity.getFreeSpecialistCount(iElder)
			iNumHealers			= pCity.getSpecialistCount(iHealer)			+ pCity.getFreeSpecialistCount(iHealer)
			iNumGreatElders		= pCity.getSpecialistCount(iGreatElder)		+ pCity.getFreeSpecialistCount(iGreatElder)
			iNumGreatHealers	= pCity.getSpecialistCount(iGreatHealer)	+ pCity.getFreeSpecialistCount(iGreatHealer)
			iSpecialistMod		+= (iNumHealers * 0.5) + (iNumElders * 0.5) + (iNumGreatElders * 1) + (iNumGreatHealers * 1)
		# AI modifier
		iAImod = 1
		if not pPlayer.isHuman():
			iAImod = 1.5

		iMekaraSpawn = round(((iLabMod + iCabalMod + iCivicMod + iBaseMod + iSpecialistMod) * iCivicMult * iAImod), 2)
		iMekaraSpawn = int(iMekaraSpawn * 100)
		iMekaraSpawn = scaleInverse(iMekaraSpawn)
		return iMekaraSpawn
	### Mekara Aspirant Start

	### Doviello Start
	def doCityTurnDoviello(self, iPlayer, pCity):
		gc			= CyGlobalContext()
		pPlayer		= gc.getPlayer(iPlayer)
		pPlot		= pCity.plot()
		iReligion	= pPlayer.getStateReligion()

		self.doChanceAnimalSpawn(iPlayer, pCity)
		iAnimalSpawnChance = pCity.getCityCounter()

		if CyGame().getSorenRandNum(10000, "Animal Spawn") >= iAnimalSpawnChance: return
		lList		= self.doAnimalListDoviello(iPlayer)
		iUnit		= lList[CyGame().getSorenRandNum(len(lList), "Pick Animal")]
		newUnit		= pPlayer.initUnit(iUnit, pPlot.getX(), pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
		newUnit.setHasPromotion(self.Promotions["Effects"]["Loyalty III"], True)

		szText		= CyTranslator().getText("TXT_KEY_MESSAGE_ANIMAL_SPAWN",())
		szSound		= 'AS2D_DISCOVERBONUS'
		iMessage	= InterfaceMessageTypes.MESSAGE_TYPE_MINOR_EVENT
		szArt		= gc.getUnitInfo(newUnit.getUnitType()).getButton()
		iGreen		= gc.getInfoTypeForString("COLOR_GREEN")
		CyInterface().addMessage(iPlayer, True, 25, szText, szSound, iMessage, szArt, iGreen, pCity.getX(), pCity.getY(), True, True)

		if iReligion != -1:
			newUnit.setReligion(iReligion)
		if pPlayer.isCivic(self.Civics["Wild Council"]):
			newUnit.setHasPromotion(self.Promotions["Effects"]["Heroic Strength I"], True)

	### TODO: Dictionaries
	def doChanceAnimalSpawn(self, iPlayer, pCity):
		gc				= CyGlobalContext()
		pPlayer			= gc.getPlayer(iPlayer)
		iDenPop			= (pCity.getPopulation() - 1) * 0.6
		iSpawnChance	= 8
		iReligion		= pPlayer.getStateReligion()
		if pCity.getNumBuilding(gc.getInfoTypeForString("BUILDING_BEAR_CAVE"))>0:
			iSpawnChance += 0.5
		if pCity.getNumBuilding(gc.getInfoTypeForString("BUILDING_GORILLA_FOREST"))>0:
			iSpawnChance += 0.5
		if pCity.getNumBuilding(gc.getInfoTypeForString("BUILDING_LION_SAVANNA"))>0:
			iSpawnChance += 0.5
		if pCity.getNumBuilding(gc.getInfoTypeForString("BUILDING_TIGER_MANGROVE"))>0:
			iSpawnChance += 0.5
		if pCity.getNumBuilding(gc.getInfoTypeForString("BUILDING_WOLF_BURROW"))>0:
			iSpawnChance += 0.5
		if pCity.getNumBuilding(gc.getInfoTypeForString("BUILDING_CRIME_RAVENOUS_PACK"))>0:
			iSpawnChance += 1
		if pCity.getNumBuilding(gc.getInfoTypeForString("BUILDING_CRIME_RAVENOUS_PACK_ESUS"))>0:
			iSpawnChance += 1
		if pPlayer.isCivic(self.Civics["Wild Council"]):
			iSpawnChance = iSpawnChance * 2
		if iReligion == self.Religions["Fellowship"]:
			iSpawnChance = iSpawnChance * 1.5

		iAnimalSpawnChance = iSpawnChance - iDenPop
		iAnimalSpawnChance = (iAnimalSpawnChance * 100)
		iAnimalSpawnChance = scaleInverse(iAnimalSpawnChance)

		pCity.setCityCounter(iAnimalSpawnChance)

	def doAnimalListDoviello(self, iPlayer):
		gc				= CyGlobalContext()
		pPlayer			= gc.getPlayer(iPlayer)
		iTeam			= pPlayer.getTeam()
		pTeam			= gc.getTeam(iTeam)

		lUnits			= []

		if   pTeam.isHasTech( self.Techs["Feral Bond"]):			lUnits.append(self.Units["Animal"]["Dire Wolf"])
		elif pTeam.isHasTech( self.Techs["Tracking"]):				lUnits.append(self.Units["Animal"]["Wolf Pack"])
		else:														lUnits.append(self.Units["Animal"]["Wolf"])

		if pPlayer.getBuildingClassCount(self.BuildingClasses["Bear Den"]) >= 1:
			if   pTeam.isHasTech(self.Techs["Iron Working"]):		lUnits.append(self.Units["Animal"]["Cave Bears"])
			elif pTeam.isHasTech(self.Techs["Bronze Working"]):		lUnits.append(self.Units["Animal"]["Bear group"])
			else:													lUnits.append(self.Units["Animal"]["Bear"])

		if pPlayer.getBuildingClassCount(self.BuildingClasses["Boar Pen"]) >= 1:
			if   pTeam.isHasTech(self.Techs["Engineering"]):		lUnits.append(self.Units["Animal"]["Blood Boar"])
			elif pTeam.isHasTech(self.Techs["Construction"]):		lUnits.append(self.Units["Animal"]["Boar Herd"])
			else:													lUnits.append(self.Units["Animal"]["Boar"])

		if pPlayer.getBuildingClassCount(self.BuildingClasses["Gorilla Hut"]) >= 1:
			if   pTeam.isHasTech(self.Techs["Bowyers"]):			lUnits.append(self.Units["Animal"]["Silverback"])
			elif pTeam.isHasTech(self.Techs["Archery"] ):			lUnits.append(self.Units["Animal"]["Gorilla Troop"])
			else:													lUnits.append(self.Units["Animal"]["Gorilla"])

		if pPlayer.getBuildingClassCount(self.BuildingClasses["Griffin Weyr"]) >= 1:
			if   pTeam.isHasTech(self.Techs["Stirrups"]):			lUnits.append(self.Units["Animal"]["Roc"])
			elif pTeam.isHasTech(self.Techs["Horseback Riding"]):	lUnits.append(self.Units["Animal"]["Griffon"])
			else:													lUnits.append(self.Units["Animal"]["Hippogriff"])

		if pPlayer.getBuildingClassCount(self.BuildingClasses["Stag Copse"]) >= 1:
			if   pTeam.isHasTech(self.Techs["Priesthood"]):			lUnits.append(self.Units["Animal"]["Stag"])
			else:													lUnits.append(self.Units["Animal"]["Elk"])

		return lUnits
	### Doviello End

	### Sheaim Start
	def doCityTurnPlanarGate(self, iPlayer, pCity):
		gc 					= CyGlobalContext()
		self.doPlanarGateChance(iPlayer, pCity)
		iPlanarGateChance	= pCity.getCityCounter()
		pPlayer 			= gc.getPlayer(iPlayer)
		if CyGame().getSorenRandNum(10000, "Planar Gate") > iPlanarGateChance: return
		lUnits				= self.doListPlanarGate(iPlayer, pCity)
		if not lUnits: return
		iUnit				= lUnits[CyGame().getSorenRandNum(len(lUnits), "Planar Gate")]
		newUnit = pPlayer.initUnit(iUnit, pCity.getX(), pCity.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)

		szText		= CyTranslator().getText("TXT_KEY_MESSAGE_PLANAR_GATE",())
		szSound		= 'AS2D_DISCOVERBONUS'
		iMessage	= InterfaceMessageTypes.MESSAGE_TYPE_MINOR_EVENT
		szArt		= gc.getUnitInfo(newUnit.getUnitType()).getButton()
		iGreen		= gc.getInfoTypeForString("COLOR_GREEN")
		CyInterface().addMessage(iPlayer, True, 25, szText, szSound, iMessage, szArt, iGreen, pCity.getX(), pCity.getY(), True, True)

		if iUnit == self.Units["Sheaim"]["Mobius Witch"]:
			lSpheres	= [	self.Promotions["Generic"]["Air I"],	self.Promotions["Generic"]["Earth I"],		self.Promotions["Generic"]["Fire I"],			self.Promotions["Generic"]["Ice I"],		self.Promotions["Generic"]["Water I"],
							self.Promotions["Generic"]["Chaos I"],	self.Promotions["Generic"]["Death I"],		self.Promotions["Generic"]["Dimensional I"],	self.Promotions["Generic"]["Entropy I"],	self.Promotions["Generic"]["Shadow I"],
							self.Promotions["Generic"]["Body I"],	self.Promotions["Generic"]["Creation I"],	self.Promotions["Generic"]["Enchantment I"],	self.Promotions["Generic"]["Force I"],		self.Promotions["Generic"]["Nature I"],
							self.Promotions["Generic"]["Metamagic I"]]
			newUnit.setLevel(3)
			newUnit.setExperienceTimes100(1000 + CyGame().getGlobalCounter() * 25, -1)
			for iSphere in lSpheres:
				if CyGame().getSorenRandNum(8, "Mobius Witch Free Promotions") != 0: continue
				newUnit.setHasPromotion(iSphere, True)
		else:
			newUnit.setExperienceTimes100(CyGame().getGlobalCounter() * 25, -1)

	def doPlanarGateChance(self, iPlayer, pCity):
		gc		= CyGlobalContext()
		iX		= pCity.getX()
		iY		= pCity.getY()
		iMult	= 1
		iHell	= 0.00
		iAC		= CyGame().getGlobalCounter()
		iMult += iAC * 0.015

		for iiX,iiY in BFC:
			pLoopPlot = CyMap().plot(iX+iiX,iY+iiY)
			if pLoopPlot.isNone(): continue
			if gc.getTerrainInfo(pLoopPlot.getTerrainType()).isHell(): iHell += 0.05
		iMult += iHell

		iPlanarGateChance = int(self.Defines["Planar Gate"] * iMult)
		iPlanarGateChance = scaleInverse(iPlanarGateChance)

		pCity.setCityCounter(iPlanarGateChance)

	def doListPlanarGate(self, iPlayer, pCity):
		gc = CyGlobalContext()
		pPlayer = gc.getPlayer(iPlayer)

		iMax = 1
		iCounter = CyGame().getGlobalCounter()
		if   iCounter >= 100: iMax = 4
		elif iCounter >= 75:  iMax = 3
		elif iCounter >= 50:  iMax = 2
		iMax = iMax * pPlayer.countNumBuildings( self.Buildings["Planar Gate"])

		lUnits = []

		if pPlayer.getUnitClassCount(self.UnitClasses["Fireball"]) < iMax:				lUnits.append(self.Units["Sheaim"]["Burning Eye"])
		if pCity.getNumBuilding(self.Buildings["Gambling House"]) > 0:
			if pPlayer.getUnitClassCount(self.UnitClasses["Revelers"]) < iMax:			lUnits.append(self.Units["Sheaim"]["Revelers"])
		if pCity.getNumBuilding(self.Buildings["Mage Guild"]) > 0:
			if pPlayer.getUnitClassCount(self.UnitClasses["Mobius Witch"]) < iMax:		lUnits.append(self.Units["Sheaim"]["Mobius Witch"])
		if pCity.getNumBuilding(self.Buildings["Carnival"]) > 0:
			if pPlayer.getUnitClassCount(self.UnitClasses["Chaos Marauder"]) < iMax:	lUnits.append(self.Units["Sheaim"]["Chaos Marauder"])
		if pCity.getNumBuilding(self.Buildings["Grove"]) > 0:
			if pPlayer.getUnitClassCount(self.UnitClasses["Manticore"]) < iMax:			lUnits.append(self.Units["Sheaim"]["Manticore"])
		if pCity.getNumBuilding(self.Buildings["Public Baths"]) > 0:
			if pPlayer.getUnitClassCount(self.UnitClasses["Succubus"]) < iMax:			lUnits.append(self.Units["Sheaim"]["Succubus"])
		if pCity.getNumBuilding(self.Buildings["Obsidian Gate"]) > 0:
			if pPlayer.getUnitClassCount(self.UnitClasses["Minotaur"]) < iMax:			lUnits.append(self.Units["Sheaim"]["Minotaur"])
		if pCity.getNumBuilding(self.Buildings["Barracks"]) > 0:
			if pPlayer.getUnitClassCount(self.UnitClasses["Colubra"]) < iMax:			lUnits.append(self.Units["Sheaim"]["Colubra"])
		if pCity.getNumBuilding(self.Buildings["Temple of the Veil"]) > 0:
			if pPlayer.getUnitClassCount(self.UnitClasses["Tar Demon"]) < iMax:			lUnits.append(self.Units["Sheaim"]["Tar Demon"])
		return lUnits
	### Sheaim End

	### Memorial of the Refugee Start
	def doCityTurnMemorial(self, iPlayer, pCity):
		gc 				= CyGlobalContext()
		pPlot 			= pCity.plot()
		pPlayer 		= gc.getPlayer(iPlayer)

		self.doMemorialChance(iPlayer, pCity)
		iMemorialChance	= pCity.getCityCounter()

		if CyGame().getSorenRandNum(10000, "Refugee") > iMemorialChance: return

		newUnit = pPlayer.initUnit(self.Units["Grigori"]["Refugee"], pPlot.getX(), pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)

		szText		= CyTranslator().getText("TXT_KEY_MESSAGE_REFUGEE",())
		szSound		= 'AS2D_DISCOVERBONUS'
		iMessage	= InterfaceMessageTypes.MESSAGE_TYPE_MINOR_EVENT
		szArt		= gc.getUnitInfo(newUnit.getUnitType()).getButton()
		iGreen		= gc.getInfoTypeForString("COLOR_GREEN")
		CyInterface().addMessage(iPlayer, True, 25, szText, szSound, iMessage, szArt, iGreen, pCity.getX(), pCity.getY(), True, True)
		if CyGame().getSorenRandNum(100, "MotR Race") <= 50:
			lRace = [	self.Promotions["Race"]["Angel"],		self.Promotions["Race"]["Dark Elven"],		self.Promotions["Race"]["Elven"],
						self.Promotions["Race"]["Orcish"],		self.Promotions["Race"]["Musteval"],		self.Promotions["Race"]["Lizardman"],
						self.Promotions["Race"]["Goblinoid"],	self.Promotions["Race"]["Fallen Angel"],	self.Promotions["Race"]["Centaur"]]
			iRace = lRace[CyGame().getSorenRandNum(len(lRace), "Pick Race")]
			newUnit.setHasPromotion(iRace, True)
		if CyGame().getSorenRandNum(100, "MotR Hero") <= 10:
			newUnit.setHasPromotion(self.Promotions["Effects"]["Hero"], True)
			newUnit.setName(self.MarnokNameGenerator(newUnit))

	def doMemorialChance(self, iPlayer, pCity):
		iChance		= 0
		if pCity.getNumBuilding(self.Buildings["Memorial Refugee"]) > 0:	iChance += 400
		if pCity.getNumBuilding(self.Buildings["Dwelling of Refuge"]) > 0:	iChance += 200

		iChance = scaleInverse(iChance)

		pCity.setCityCounter(iChance)
	### Memorial of the Refugee End

#### TODO: remove call from CvEventManager.py and remove function
	def doTurnCualli(self, iPlayer):
		return
		# for iUnit in xrange(pPlayer.getNumUnits()):
			# pUnit = pPlayer.getUnit(iUnit)
			# if iUnitType != self.Units["Cualli"]["Priest of Agruonn"] and iUnitType != self.Units["Cualli"]["Shadow Priest of Agruonn"] and iUnitType != self.Heroes["Miquiztli"]: return
			# if   pUnit.isHasPromotion(self.Promotions["Generic"]["Empower V"]):		pUnit.setHasPromotion(self.Promotions["Generic"]["Empower V"], False)
			# elif pUnit.isHasPromotion(self.Promotions["Generic"]["Empower IV"]):	pUnit.setHasPromotion(self.Promotions["Generic"]["Empower IV"], False)
			# elif pUnit.isHasPromotion(self.Promotions["Generic"]["Empower III"]):	pUnit.setHasPromotion(self.Promotions["Generic"]["Empower III"], False)
			# elif pUnit.isHasPromotion(self.Promotions["Generic"]["Empower II"]):	pUnit.setHasPromotion(self.Promotions["Generic"]["Empower II"], False)
			# elif pUnit.isHasPromotion(self.Promotions["Generic"]["Empower I"]):		pUnit.setHasPromotion(self.Promotions["Generic"]["Empower I"], False)

	def genesis(self, iPlayer):
		for i in xrange(CyMap().numPlots()):
			pPlot = CyMap().plotByIndex(i)
			if pPlot.getOwner() != iPlayer: continue
			iFeature = pPlot.getFeatureType()
			iTerrain = pPlot.getTerrainType()
			if   iTerrain == self.Terrain["Tundra"]:										pPlot.setTerrainType(self.Terrain["Taiga"], False, False)
			elif iTerrain == self.Terrain["Taiga"]:											pPlot.setTerrainType(self.Terrain["Plains"], False, False)
			elif iTerrain == self.Terrain["Desert"] and iFeature != self.Feature["Oasis"]:	pPlot.setTerrainType(self.Terrain["Plains"], False, False)
			elif iTerrain == self.Terrain["Plains"]:										pPlot.setTerrainType(self.Terrain["Grass"], False, False)
			elif iTerrain == self.Terrain["Grass"] and pPlot.getImprovementType() == -1 and not iFeature in (self.Feature["Ancient Forest"], self.Feature["Volcano"]) and not pPlot.isPeak():
				pPlot.setFeatureType(self.Feature["Forest"], 0)

	def getAshenVeilCity(self, iNum):
		gc = CyGlobalContext()
		lValuedCities	= []
		pBestCity1 =	-1
		pBestCity2 =	-1
		pBestCity3 =	-1
		for iPlayer in xrange(gc.getMAX_PLAYERS()):
			pPlayer = gc.getPlayer(iPlayer)
			if not pPlayer.isAlive() or pPlayer.getCivilizationType() == self.Civilizations["Infernal"]: continue
			for iCity in xrange(pPlayer.getNumCities()):
				pCity = pPlayer.getCity(iCity)
				if not pCity.isHasReligion(self.Religions["Ashen Veil"]) or pCity.isCapital(): continue
				iValue =  pCity.getPopulation() * 100
				iValue += pCity.getCulture(iPlayer) / 50
				iValue += pCity.getNumBuildings() * 10
				iValue += pCity.getNumWorldWonders() * 200
				iValue += pCity.countNumImprovedPlots() * 20
				lValuedCities.append((pCity,iValue))
		lValuedCities.sort(key=lambda tup: tup[1], reverse=True)
		if len(lValuedCities) > 3:
			pBestCity1 = lValuedCities[0][0]
			pBestCity2 = lValuedCities[1][0]
			pBestCity3 = lValuedCities[2][0]
		if   iNum == 1: return pBestCity1
		elif iNum == 2: return pBestCity2
		elif iNum == 3: return pBestCity3
		return -1

	### TODO: Update
	def getHero(self, pPlayer):
		iCiv 		= pPlayer.getCivilizationType()
		# Grey Fox: changed most if's to elif, cause pointless to check all
		if   iCiv == self.Civilizations["Bannor"]:			return self.Heroes["Class-Donal"]
		elif iCiv == self.Civilizations["Malakim"]:			return self.Heroes["Class-Teutorix"]
		elif iCiv == self.Civilizations["Elohim"]:			return self.Heroes["Class-Corlindale"]
		elif iCiv == self.Civilizations["Mercurians"]:		return self.Heroes["Class-Basium"]
		elif iCiv == self.Civilizations["Lanun"]:			return self.Heroes["Class-Guybrush"]
		elif iCiv == self.Civilizations["Kuriotates"]:		return self.Heroes["Class-Eurabatres"]
		elif iCiv == self.Civilizations["Ljosalfar"]:		return self.Heroes["Class-Gilden"]
		elif iCiv == self.Civilizations["Khazad"]:			return self.Heroes["Class-Maros"]
		elif iCiv == self.Civilizations["Hippus"]:			return self.Heroes["Class-Magnadine"]
		elif iCiv == self.Civilizations["Amurites"]:		return self.Heroes["Class-Govannon"]
		elif iCiv == self.Civilizations["Balseraphs"]:		return self.Heroes["Class-Loki"]
		elif iCiv == self.Civilizations["Clan of Embers"]:	return self.Heroes["Class-Rantine"]
		elif iCiv == self.Civilizations["Svartalfar"]:		return self.Heroes["Class-Alazkan"]
		elif iCiv == self.Civilizations["Calabim"]:			return self.Heroes["Class-Losha"]
		elif iCiv == self.Civilizations["Sheaim"]:			return self.Heroes["Class-Abashi"]
		elif iCiv == self.Civilizations["Sidar"]:			return self.Heroes["Class-Rathus"]
		elif iCiv == self.Civilizations["Illians"]:			return self.Heroes["Class-Wilboman"]
		elif iCiv == self.Civilizations["Infernal"]:		return self.Heroes["Class-Hyborem"]
		elif iCiv == self.Civilizations["Dural"]:			return self.Heroes["Class-Karrlson"]
		elif iCiv == self.Civilizations["Chislev"]:			return self.Heroes["Class-Meshwaki"]
		elif iCiv == self.Civilizations["Archos"]:			return self.Heroes["Class-Mother"]
		elif iCiv == self.Civilizations["Mazatl"]:			return self.Heroes["Class-Coatlann"]
		elif iCiv == self.Civilizations["Cualli"]:			return self.Heroes["Class-Miquiztli"]
		elif iCiv == self.Civilizations["Austrin"]:			return self.Heroes["Class-Harmatt"]
		elif iCiv == self.Civilizations["Scions"]:
			iLeader = pPlayer.getLeaderType()
			if   iLeader == self.Leaders["Risen Emperor"]:	return self.Heroes["Class-Korrina"]
			elif iLeader == self.Leaders["Korrina"]:		return self.Heroes["Class-The Risen Emperor"]
		elif iCiv == self.Civilizations["Frozen"]:			return self.Heroes["Class-Taranis"] # TODO Ronkhar: make modular and move to frozen module
		elif iCiv == self.Civilizations["Mechanos"]:		return self.Heroes["Class-Feris"]

		return -1

	def getUnholyVersion(self, pUnit):
		if( self.UnitCombats=={}): self.initialize()
		gc			= CyGlobalContext()
		iUnitCombat	= pUnit.getUnitCombatType()
		iTier		= gc.getUnitInfo(pUnit.getUnitType()).getTier()
		if   iUnitCombat == self.UnitCombats["Adept"]:
			if   iTier == 2: return self.Units["Infernal"]["Imp"]
			elif iTier == 3: return self.Units["Generic"]["Mage"]
			elif iTier == 4: return self.Units["Summons"]["Lich"]
		elif iUnitCombat in (self.UnitCombats["Animal"], self.UnitCombats["Beast"]):
			if   iTier == 1: return self.Units["Generic"]["Scout"]
			elif iTier == 2: return self.Units["Infernal"]["Hellhound"]
			elif iTier == 3: return self.Units["Generic"]["Assassin"]
			elif iTier == 4: return self.Units["Veil"]["Beast of Agares"]
		elif iUnitCombat == self.UnitCombats["Archer"]:
			if   iTier == 2: return self.Units["Generic"]["Archer"]
			elif iTier == 3: return self.Units["Generic"]["Longbowman"]
			elif iTier == 4: return self.Units["Generic"]["Crossbowman"]
		elif iUnitCombat == self.UnitCombats["Disciple"]:
			if   iTier == 2: return self.Units["Veil"]["Disciple"]
			elif iTier == 3: return self.Units["Veil"]["Ritualist"]
			elif iTier == 4: return self.Units["Generic"]["Eidolon"]
		elif iUnitCombat == self.UnitCombats["Melee"]:
			if   iTier == 1: return self.Units["Summons"]["Skeleton"]
			elif iTier == 2: return self.Units["Veil"]["Diseased Corpse"]
			elif iTier == 3: return self.Units["Generic"]["Champion"]
			elif iTier == 4: return self.Units["Infernal"]["Balor"]
		elif iUnitCombat == self.UnitCombats["Mounted"]:
			if   iTier == 2: return self.Units["Generic"]["Horseman"]
			elif iTier == 3: return self.Units["Generic"]["Chariot"]
			elif iTier == 4: return self.Units["Infernal"]["Death Knight"]
		elif iUnitCombat == self.UnitCombats["Recon"]:
			if   iTier == 1: return self.Units["Generic"]["Scout"]
			elif iTier == 2: return self.Units["Infernal"]["Hellhound"]
			elif iTier == 3: return self.Units["Generic"]["Assassin"]
			elif iTier == 4: return self.Units["Generic"]["Beastmaster"]
		return -1

	def giftUnit(self, iUnit, iCivilization, iXP, pFromPlot, iFromPlayer):
		gc = CyGlobalContext()
		iAngel		= self.Units["Mercurian"]["Angel"]
		iMane		= self.Units["Infernal"]["Manes"]
		iFrozenSoul	= self.Units["Frozen"]["Frozen Souls"]
		if iUnit in (iAngel, iMane, iFrozenSoul):
			iChance = 100 - (CyGame().countCivPlayersAlive() * 3) + iXP/100
			iChance = min(iChance, 95)
			iChance = max(iChance, 5)
			if CyGame().getSorenRandNum(100, "Gift Unit") <= iChance: return
		bValid = False

		for i in xrange(CyGame().getNumCivActive(iCivilization)):
			iPlayer = CyGame().getCivActivePlayer(iCivilization, i)
			pPlayer = gc.getPlayer(iPlayer)
			if not pPlayer.isAlive(): continue
			if pPlayer.getCivilizationType() != iCivilization: continue

#			if iUnit == iFrozenSoul: ### TW: Another chance check is a bit too harsh? Worst case scenario would be 0.25% to get a FS.
#				iChance = 100 * pPlayer.getAveragePopulation() / 15
#				iChance = min(iChance, 95)
#				if CyGame().getSorenRandNum(100, "Gift Frozen Soul") < iChance: break

			iNumCities = pPlayer.getNumCities()
			if iNumCities <= 0: continue

			bHuman	= pPlayer.isHuman()
			iRnd	= CyGame().getSorenRandNum(iNumCities, "Gift Unit")
			pCity	= pPlayer.getCity(iRnd)
			iX		= pCity.getX()
			iY		= pCity.getY()

			newUnit = pPlayer.initUnit(iUnit, iX, iY, UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
			newUnit.changeExperienceTimes100(iXP, -1, False, False, False)
			if pFromPlot != -1 and gc.getPlayer(iFromPlayer).isHuman(): bValid = True
			if bHuman:
				szSound		= 'AS2D_UNIT_FALLS'
				iMessage	= InterfaceMessageTypes.MESSAGE_TYPE_MINOR_EVENT
				iGreen		= gc.getInfoTypeForString("COLOR_GREEN")
				szText		= CyTranslator().getText("TXT_KEY_MESSAGE_ADD_FROZEN_SOULS",())
				szArt		= 'Art/Interface/Buttons/Promotions/Races/frostling.dds'
				if   iUnit == iMane:
					szText	= CyTranslator().getText("TXT_KEY_MESSAGE_ADD_MANES",())
					szArt	= 'Art/Interface/Buttons/Promotions/Races/Demon.dds'
				elif iUnit == iAngel:
					szText	= CyTranslator().getText("TXT_KEY_MESSAGE_ADD_ANGEL",())
					szArt	= 'Art/Interface/Buttons/Promotions/Races/Angel.dds'
				CyInterface().addMessage(iPlayer, True, 25, szText, szSound, iMessage, szArt, iGreen, iX, iY, True, True)
			elif iUnit in (iMane, iFrozenSoul):
				if CyGame().getSorenRandNum(100, "Manes") >= (100 - (pCity.getPopulation() * 5)): continue
				pCity.changePopulation(1)
				newUnit.kill(True, PlayerTypes.NO_PLAYER)

		if bValid:
			szSound		= 'AS2D_UNIT_FALLS'
			iMessage	= InterfaceMessageTypes.MESSAGE_TYPE_DISPLAY_ONLY
			iRed		= gc.getInfoTypeForString("COLOR_RED")
			szText		= CyTranslator().getText("TXT_KEY_MESSAGE_UNIT_FREEZES",())
			szArt		= 'Art/Interface/Buttons/Promotions/Races/frostling.dds'
			if   iUnit == iMane:
				szText	= CyTranslator().getText("TXT_KEY_MESSAGE_UNIT_FALLS",())
				szArt	= 'Art/Interface/Buttons/Promotions/Races/Demon.dds'
			elif iUnit == iAngel:
				szText	= CyTranslator().getText("TXT_KEY_MESSAGE_UNIT_RISES",())
				szArt	= 'Art/Interface/Buttons/Promotions/Races/Angel.dds'
			CyInterface().addMessage(iFromPlayer, True, 25, szText, szSound, iMessage, szArt, iRed, pFromPlot.getX(), pFromPlot.getY(), True, True)

	def warScript(self, iPlayer):
		gc				= CyGlobalContext()
		pPlayer			= gc.getPlayer(iPlayer)
		iTeam			= pPlayer.getTeam()
		pTeam			= gc.getTeam(iTeam)
		if pTeam.isAVassal(): return
		iCiv			= pPlayer.getCivilizationType()
		iAlignment		= pPlayer.getAlignment()
		iEnemy			= -1
		iCounter		= CyGame().getGlobalCounter()
		WarPlanTotal	= WarPlanTypes.WARPLAN_TOTAL
		iWarCount		= pTeam.getAtWarCount(True)

		for iPlayer2 in xrange(gc.getMAX_PLAYERS()):
			iWarCount	= pTeam.getAtWarCount(True)
			pPlayer2	= gc.getPlayer(iPlayer2)
			iCiv2		= pPlayer2.getCivilizationType()
			bBetter		= False
			bWorse2		= False

			if CyGame().getPlayerRank(iPlayer2) > CyGame().getPlayerRank(iPlayer):
				bBetter	= True
			if iEnemy == -1 or CyGame().getPlayerRank(iPlayer2) > CyGame().getPlayerRank(iEnemy):
				bWorse2 = True
			if not pPlayer2.isAlive(): continue
			iTeam2 = pPlayer2.getTeam()

			if pTeam.isAtWar(iTeam2):
				randName = "War Script, Player %s vs Player %s" % (iPlayer, iPlayer2)
				if CyGame().getSorenRandNum(100, randName) < 5: self.dogpile(iPlayer, iPlayer2)

			if not self.warScriptAllow(iPlayer, iPlayer2): continue

			if iWarCount == 0:
				if   pPlayer2.getBuildingClassMaking(self.BuildingClasses["Tower of Mastery"]) > 0:
					startWar(iPlayer, iPlayer2, WarPlanTotal)
				elif iAlignment == self.Alignments["Evil"]:
					if   pPlayer2.getNumBuilding(self.Buildings["Altar - Divine"]) > 0 or pPlayer2.getNumBuilding(self.Buildings["Altar - Exalted"]) > 0:
						startWar(iPlayer, iPlayer2, WarPlanTotal)
					elif (iCounter > 40 or iCiv in (self.Civilizations["Infernal"], self.Civilizations["Doviello"])) and bBetter and bWorse2:
						iEnemy = iPlayer2
			elif iCiv == self.Civilizations["Mercurians"] and pPlayer2.getStateReligion() == self.Religions["Ashen Veil"]:
				startWar(iPlayer, iPlayer2, WarPlanTotal)
			elif iCounter > 20:
				if iCiv in (self.Civilizations["Svartalfar"], self.Civilizations["Ljosalfar"]) and iCiv2 in (self.Civilizations["Svartalfar"], self.Civilizations["Ljosalfar"]) and bBetter and iCiv != iCiv2:
					startWar(iPlayer, iPlayer2, WarPlanTotal)

		if iEnemy != -1: startWar(iPlayer, iEnemy, WarPlanTotal)

	def warScriptAllow(self, iPlayer, iPlayer2):
		gc 				= CyGlobalContext()
		pPlayer 		= gc.getPlayer(iPlayer)
		pPlayer2 		= gc.getPlayer(iPlayer2)
		iTeam 			= gc.getPlayer(iPlayer).getTeam()
		iTeam2 			= gc.getPlayer(iPlayer2).getTeam()
		pTeam 			= gc.getTeam(iTeam)
		if pPlayer.isBarbarian() or pPlayer2.isBarbarian():	return False
		if pTeam.isHasMet(iTeam2) == False:					return False
		if pTeam.AI_getAtPeaceCounter(iTeam2) < 20:			return False
		if pTeam.isAtWar(iTeam2):							return False
		if pPlayer.getCivilizationType() == self.Civilizations["Infernal"] and pPlayer2.getStateReligion() == self.Religions["Ashen Veil"]: return False
		return True

	def dogpile(self, iPlayer, iVictim):
		gc			= CyGlobalContext()
		iCounter	= CyGame().getGlobalCounter()
		for iPlayer2 in xrange(gc.getMAX_PLAYERS()):
			pPlayer2 = gc.getPlayer(iPlayer2)
			iChance = -1
			if not pPlayer2.isAlive():						continue
			if not self.dogPileAllow(iPlayer, iPlayer2):	continue
			if not self.warScriptAllow(iPlayer2, iVictim):	continue
			iChance = pPlayer2.AI_getAttitude(iPlayer) * 5
			if iChance <= 0:								continue
			iChance -= (pPlayer2.AI_getAttitude(iVictim) * 5) + 10
			if not self.GameOptions["Aggressive AI"]: iChance -= 10
			if iChance <= 0:								continue
			iChance = iChance + (iCounter / 3)
			if pPlayer2.getCivilizationType() == self.Civilizations["Balseraphs"]:
				iChance = CyGame().getSorenRandNum(50, "Dogpile Balseraphs")
			if CyGame().getSorenRandNum(100, "Dogpile") < iChance:
				startWar(iPlayer2, iVictim, WarPlanTypes.WARPLAN_DOGPILE)

	def dogPileAllow(self, iPlayer, iPlayer2):
		gc			= CyGlobalContext()
		getPlayer	= gc.getPlayer
		pPlayer2	= getPlayer(iPlayer2)
		iCiv2		= pPlayer2.getCivilizationType()
		iTeam		= getPlayer(iPlayer).getTeam()
		iTeam2		= getPlayer(iPlayer2).getTeam()
		pTeam2		= gc.getTeam(iTeam2)
		if iPlayer == iPlayer2:						return False
		if iTeam == iTeam2:							return False
		if pPlayer2.isHuman():						return False
		if iCiv2 == self.Civilizations["Elohim"]:	return False
		if pTeam2.isAVassal():						return False
		return True

	def countMana(self, pPlayer):
		iNum = 0
		iNum += pPlayer.getNumAvailableBonuses(self.Mana["Air"]        )
		iNum += pPlayer.getNumAvailableBonuses(self.Mana["Body"]       )
		iNum += pPlayer.getNumAvailableBonuses(self.Mana["Chaos"]      )
		iNum += pPlayer.getNumAvailableBonuses(self.Mana["Creation"]   )
		iNum += pPlayer.getNumAvailableBonuses(self.Mana["Death"]      )
		iNum += pPlayer.getNumAvailableBonuses(self.Mana["Dimensional"])
		iNum += pPlayer.getNumAvailableBonuses(self.Mana["Earth"]      )
		iNum += pPlayer.getNumAvailableBonuses(self.Mana["Enchantment"])
		iNum += pPlayer.getNumAvailableBonuses(self.Mana["Entropy"]    )
		iNum += pPlayer.getNumAvailableBonuses(self.Mana["Fire"]       )
		iNum += pPlayer.getNumAvailableBonuses(self.Mana["Force"]      )
		iNum += pPlayer.getNumAvailableBonuses(self.Mana["Ice"]        )
		iNum += pPlayer.getNumAvailableBonuses(self.Mana["Law"]        )
		iNum += pPlayer.getNumAvailableBonuses(self.Mana["Life"]       )
		iNum += pPlayer.getNumAvailableBonuses(self.Mana["Metamagic"]  )
		iNum += pPlayer.getNumAvailableBonuses(self.Mana["Mind"]       )
		iNum += pPlayer.getNumAvailableBonuses(self.Mana["Nature"]     )
		iNum += pPlayer.getNumAvailableBonuses(self.Mana["Shadow"]     )
		iNum += pPlayer.getNumAvailableBonuses(self.Mana["Spirit"]     )
		iNum += pPlayer.getNumAvailableBonuses(self.Mana["Sun"]        )
		iNum += pPlayer.getNumAvailableBonuses(self.Mana["Water"]      )
		return iNum

	def canReceiveReligionUnit(self, pPlayer):
		iCiv = pPlayer.getCivilizationType()
		if   iCiv == self.Civilizations["Cualli"]: return False
	#	elif iCiv == self.Civilizations["Mazatl"]: return False
		return True

	def MarnokNameGenerator(self, pUnit):
		gc 				= CyGlobalContext()
		pPlayer 		= gc.getPlayer(pUnit.getOwner())
		iReligion 		= pPlayer.getStateReligion()
		pAlign 			= pPlayer.getAlignment()
		iUnitType 		= pUnit.getUnitType()

		lPre=["ta","go","da","bar","arc","ken","an","ad","mi","kon","kar","mar","wal","he", "ha", "re", "ar", "bal", "bel", "bo", "bri", "car","dag","dan","ma","ja","co","be","ga","qui","sa"]
		lMid=["ad","z","the","and","tha","ent","ion","tio","for","tis","oft","che","gan","an","en","wen","on","d","n","g","t","ow","dal"]
		lEnd=["ar","sta","na","is","el","es","ie","us","un","th", "er","on","an","re","in","ed","nd","at","en","le","man","ck","ton","nok","git","us","or","a","da","u","cha","ir"]

		lEpithet=["red","blue","black","grey","white","strong","brave","old","young","great","slayer","hunter","seeker"]
		lNoun=["spirit","soul","boon","born","staff","rod","shield","autumn","winter","spring","summer","wit","horn","tusk","glory","claw","tooth","head","heart", "blood","breath", "blade", "hand", "lover","bringer","maker","taker","river","stream","moon","star","face","foot","half","one","hundred","thousand"]
		lSchema=["CPME","CPMESCPME","CPESCPE","CPE","CPMME","CPMDCME","CPMAME","KCPMESCUM","CPMME[ the ]CX", "CPMESCXN", "CPMME[ of ]CPMME", "CNNSCXN"]

		if pAlign == self.Alignments["Evil"]:
			lNoun = lNoun + ["fear","terror","reign","brood","snare","war","strife","pain","hate","evil","hell","misery","murder","anger","fury","rage","spawn","sly","blood","bone","scythe","slave","bound","ooze","scum"]
			lEpithet=["dark","black","white","cruel","foul"]
		if iReligion == self.Religions["Ashen Veil"]:
			lEpithet = lEpithet + ["fallen","diseased","infernal","profane","corrupt"]
			lSchema = lSchema + ["CPME[ the ]CX"]
		if iReligion == self.Religions["Octopus Overlords"]:
			lPre = lPre + ["cth","cht","shu","az","ts","dag","hy","gla","gh","rh","x","ll"]
			lMid = lMid + ["ul","tha","on","ug","st","oi"]
			lEnd = lEnd + ["hu","on", "ha","ua","oa","uth","oth","ath","thua", "thoa","ur","ll","og","hua"]
			lEpithet = lEpithet + ["nameless","webbed","deep","watery"]
			lNoun = lNoun + ["tentacle","wind","wave","sea","ocean","dark","crab","abyss","island"]
			lSchema = lSchema + ["CPMME","CPDMME","CPAMAME","CPMAME","CPAMAMEDCPAMAE"]
		if iReligion == self.Religions["Order"]:
			lPre = lPre + ["ph","v","j"]
			lMid = lMid + ["an","al","un"]
			lEnd = lEnd + ["uel","in","il"]
			lEpithet = lEpithet + ["confessor","crusader", "faithful","obedient","good"]
			lNoun = lNoun + ["order", "faith", "heaven","law"]
			lSchema = lSchema + ["CPESCPME","CPMESCPE","CPMESCPME", "CPESCPE"]
		if iReligion == self.Religions["Fellowship"]:
			lPre = lPre + ["ki","ky","yv"]
			lMid = lMid + ["th","ri"]
			lEnd = lEnd + ["ra","el","ain"]
			lEpithet = lEpithet + ["green"]
			lNoun = lNoun + ["tree","bush","wood","berry","elm","willow","oak","leaf","flower","blossom"]
			lSchema = lSchema + ["CPESCN","CPMESCNN","CPMESCXN"]
		if iReligion == self.Religions["Runes of Kilmorph"]:
			lPre = lPre + ["bam","ar","khel","ki"]
			lMid = lMid + ["th","b","en"]
			lEnd = lEnd + ["ur","dain","ain","don"]
			lEpithet = lEpithet + ["deep","guard","miner"]
			lNoun = lNoun + ["rune","flint","slate","stone","rock","iron","copper","mithril","thane","umber"]
			lSchema = lSchema + ["CPME","CPMME"]
		if iReligion == self.Religions["Empyrean"]:
			lEpithet = lEpithet + ["radiant","holy"]
			lNoun = lNoun + ["honor"]
		if iReligion == self.Religions["Council of Esus"]:
			lEpithet = lEpithet + ["hidden","dark"]
			lNoun = lNoun + ["cloak","shadow","mask"]
			lSchema = lSchema + ["CPME","CPMME"]
		if pUnit.isHasPromotion(self.Promotions["Generic"]["Enraged"]):
			# I have left this as a copy of the Barbarian, see how it goes, this might do the trick. I plan to use it when there is a chance a unit will go Barbarian anyway.
			lPre = lPre + ["gru","bra","no","os","dir","ka","z"]
			lMid = lMid + ["g","ck","gg","sh","b","bh","aa"]
			lEnd = lEnd + ["al","e","ek","esh","ol","olg","alg"]
			lNoun = lNoun + ["death", "hate", "rage", "mad","insane","berserk"]
			lEpithet = lEpithet + ["smasher", "breaker", "mangle","monger"]
		if pUnit.isHasPromotion(self.Promotions["Generic"]["Crazed"]):
			# might want to tone this down, because I plan to use it as possession/driven to madness, less than madcap zaniness.
			lPre = lPre + ["mad","pim","zi","zo","fli","mum","dum","odd","slur"]
			lMid = lMid + ["bl","pl","gg","ug","bl","b","zz","abb","odd"]
			lEnd = lEnd + ["ad","ap","izzle","onk","ing","er","po","eep","oggle","y"]
		if pUnit.isHasPromotion(self.Promotions["Effects"]["Vampire"]):
			lPre = lPre + ["dra","al","nos","vam","vla","tep","bat","bar","cor","lil","ray","zar","stra","le"]
			lMid = lMid + ["cul","u","car","fer","pir","or","na","ov","sta"]
			lEnd = lEnd + ["a","d","u","e","es","y","bas","vin","ith","ne","ak","ich","hd","t"]
		if pUnit.isHasPromotion(self.Promotions["Race"]["Demon"]):
			lPre = lPre + ["aa","ab","adr","ah","al","de","ba","cro","da","be","eu","el","ha","ib","me","she","sth","z"]
			lMid = lMid + ["rax","lia","ri","al","as","b","bh","aa","al","ze","phi","sto","phe","cc","ee"]
			lEnd = lEnd + ["tor","tan","ept","lu","res","ah","mon","gon","bul","gul","lis","les","uz"]
			lSchema = ["CPMMME","CPMACME", "CPKMAUAPUE", "CPMMME[ the ]CNX"]
		if iUnitType == self.Units["Savage"]["Hill Giant"]:
			lPre = lPre + ["gor","gra","gar","gi","gol"]
			lMid = lMid + ["gan","li","ri","go"]
			lEnd = lEnd + ["tus","tan","ath","tha"]
			lSchema = lSchema +["CXNSCNN","CPESCNE", "CPMME[ the ]CX"]
			lEpithet = lEpithet + ["large","huge","collossal","brutal","basher","smasher","crasher","crusher"]
			lNoun = lNoun + ["fist","tor","hill","brute","stomp"]
		if iUnitType == self.Units["Clan of Embers"]["Lizardman"]:
			lPre = lPre + ["ss","s","th","sth","hss"]
			lEnd = lEnd + ["ess","iss","ath","tha"]
			lEpithet = lEpithet + ["cold"]
			lNoun = lNoun + ["hiss","tongue","slither","scale","tail","ruin"]
			lSchema = lSchema + ["CPAECPAE","CPAKECPAU","CPAMMAE"]
		if iUnitType in (self.Units["Summons"]["Fire Elemental"], self.Units["Summons"]["Azer"]):
			lPre = lPre + ["ss","cra","th","sth","hss","roa"]
			lMid = lMid + ["ss","ck","rr","oa","iss","tt"]
			lEnd = lEnd + ["le","iss","st","r","er"]
			lNoun = lNoun + ["hot", "burn","scald","roast","flame","scorch","char","sear","singe","fire","spit"]
			lSchema = ["CNN","CNX","CPME","CPME[ the ]CNX","CPMME","CNNSCPME"]
		if iUnitType == self.Units["Summons"]["Water Elemental"]:
			lPre = lPre + ["who","spl","dr","sl","spr","sw","b"]
			lMid = lMid + ["o","a","i","ub","ib"]
			lEnd = lEnd + ["sh","p","ter","ble"]
			lNoun = lNoun + ["wave","lap","sea","lake","water","tide","surf","spray","wet","damp","soak","gurgle","bubble"]
			lSchema = ["CNN","CNX","CPME","CPME[ the ]CNX","CPMME","CNNSCPME"]
		if iUnitType == self.Units["Summons"]["Air Elemental"]:
			lPre = lPre + ["ff","ph","th","ff","ph","th"]
			lMid = lMid + ["oo","aa","ee","ah","oh"]
			lEnd = lEnd + ["ff","ph","th","ff","ph","th"]
			lNoun = lNoun + ["wind","air","zephyr","breeze","gust","blast","blow"]
			lSchema = ["CNN","CNX","CPME","CPME[ the ]CNX","CPMME","CNNSCPME"]
		if iUnitType == self.Units["Summons"]["Earth Elemental"]:
			lPre = lPre + ["gra","gro","kro","ff","ph","th"]
			lMid = lMid + ["o","a","u"]
			lEnd = lEnd + ["ck","g","k"]
			lNoun = lNoun + ["rock","stone","boulder","slate","granite","rumble","quake"]
			lSchema = ["CNN","CNX","CPME","CPME[ the ]CNX","CPMME","CNNSCPME"]

		# SEA BASED
		# Check for ships - special schemas
		if pUnit.getUnitCombatType() == self.UnitCombats["Naval"]:
			lEnd = lEnd + ["ton","town","port"]
			lNoun = lNoun + ["lady","jolly","keel","bow","stern", "mast","sail","deck","hull","reef","wave"]
			lEpithet = lEpithet + ["sea", "red", "blue","grand","barnacle","gull"]
			lSchema = ["[The ]CNN", "[The ]CXN", "[The ]CNX","[The ]CNSCN", "[The ]CNSCX","CPME['s ]CN","[The ]CPME", "[The ]CNX","CNX","CN['s ]CN"]

		# # #
		# Pick a Schema
		sSchema = lSchema[CyGame().getSorenRandNum(len(lSchema), "Name Gen")]
		sFull = ""
		sKeep = ""
		iUpper = 0
		iKeep = 0
		iSkip = 0

		# Run through each character in schema to generate name
		for iCount in xrange(0,len(sSchema)):
			sAdd=""
			iDone = 0
			sAction = sSchema[iCount]
			if iSkip == 1:
				if sAction == "]":
					iSkip = 0
				else:
					sAdd = sAction
					iDone = 1
			else:					# MAIN SECTION
				if   sAction == "P": 	# Pre 	: beginnings of names
					sAdd = lPre[CyGame().getSorenRandNum(len(lPre), "Name Gen")]
					iDone = 1
				elif sAction == "M":	# Mid 	: middle syllables
					sAdd = lMid[CyGame().getSorenRandNum(len(lMid), "Name Gen")]
					iDone = 1
				elif sAction == "E":	# End	: end of names
					sAdd = lEnd[CyGame().getSorenRandNum(len(lEnd), "Name Gen")]
					iDone = 1
				elif sAction == "X":	# Epithet	: epithet word part
					#epithets ("e" was taken!)
					sAdd = lEpithet[CyGame().getSorenRandNum(len(lEpithet), "Name Gen")]
					iDone = 1
				elif sAction == "N":	# Noun	: noun word part
					#noun
					sAdd = lNoun[CyGame().getSorenRandNum(len(lNoun), "Name Gen")]
					iDone = 1
				elif sAction == "S":	# Space	: a space character. (Introduced before [ ] was possible )
					sAdd =  " "
					iDone = 1
				elif sAction == "D":	# Dash	: a - character. Thought to be common and useful enough to warrant inclusion : Introduced before [-] was possible
					sAdd =  "-"
					iDone = 1
				elif sAction == "A":	# '		: a ' character - as for -, introduced early
					sAdd = "'"
					iDone = 1
				elif sAction == "C":	# Caps	: capitalizes first letter of next phrase generated. No effect on non-letters.
					iUpper = 1
				elif sAction == "K":	# Keep	: stores the next phrase generated for re-use with U
					iKeep = 1
				elif sAction == "U":	# Use	: re-uses a stored phrase.
					sAdd = sKeep
					iDone = 1
				elif sAction == "[":	# Print	: anything between [] is added to the final phrase "as is". Useful for [ the ] and [ of ] among others.
					iSkip = 1
			# capitalizes phrase once.
			if iUpper == 1 and iDone == 1:
				sAdd = sAdd.capitalize()
				iUpper = 0
			# stores the next phrase generated.
			if iKeep == 1 and iDone == 1:
				sKeep = sAdd
				iKeep = 0
			# only adds the phrase if a new bit was actally created.
			if iDone == 1:
				sFull = "%s%s" % (sFull, sAdd)


		# trim name length
		if len(sFull) > 25:
			sFull = sFull[:25]
		#CyInterface().addMessage(caster.getOwner(),True,25,"NAME : "+sFull,'AS2D_POSITIVE_DINK',1,'Art/Interface/Buttons/Spells/Rob Grave.dds',ColorTypes(8),pPlot.getX(),pPlot.getY(),True,True)

		return sFull

	### TODO: Dictionaries
	def resetRepublicTraits(self, pPlayer):
		git	= CyGlobalContext().getInfoTypeForString
		pPlayer.setHasTrait(git("TRAIT_AGGRESSIVE_REPUBLIC"),	False)
		pPlayer.setHasTrait(git("TRAIT_DEFENDER_REPUBLIC"),		False)
		pPlayer.setHasTrait(git("TRAIT_FINANCIAL_REPUBLIC"),	False)
		pPlayer.setHasTrait(git("TRAIT_EXPANSIVE_REPUBLIC"),	False)
		pPlayer.setHasTrait(git("TRAIT_SPIRITUAL_REPUBLIC"),	False)
		pPlayer.setHasTrait(git("TRAIT_ORGANIZED_REPUBLIC"),	False)
		pPlayer.setHasTrait(git("TRAIT_PHILOSOPHICAL_REPUBLIC"),False)
		pPlayer.setHasTrait(git("TRAIT_INDUSTRIOUS_REPUBLIC"),	False)

	### TODO: Dictionaries
	def angelorMane(self, pUnit):
		if( self.Religions=={}): self.initialize()
		gc			= CyGlobalContext()
		iUnitCombat	= pUnit.getUnitCombatType()
		iAngel		= self.Units["Mercurian"]["Angel"]
		iMane		= self.Units["Infernal"]["Manes"]
		iReligion	= pUnit.getReligion()
		iPlayer		= pUnit.getOwner()
		pPlayer		= gc.getPlayer(iPlayer)
		iLeader		= pPlayer.getLeaderType()
		iAlignment	= pPlayer.getAlignment()
		bPurified	= pPlayer.isHasFlag(gc.getInfoTypeForString('FLAG_PURIFIED_WELL'))

		if not pUnit.isAlive():														return -1
		if iUnitCombat in (self.UnitCombats["Animal"], self.UnitCombats["Beast"]):	return -1

		if iReligion in (self.Religions["Ashen Veil"],	self.Religions["Octopus Overlords"], self.Religions["White Hand"], self.Religions["Council of Esus"]):
			return iMane
		if iReligion in (self.Religions["Order"],		self.Religions["Empyrean"]):
			return iAngel
		if iReligion in (self.Religions["Fellowship"],	self.Religions["Runes of Kilmorph"]):
			return -1

		lEvilProms = [	gc.getInfoTypeForString('PROMOTION_UNHOLY_TAINT'), gc.getInfoTypeForString('PROMOTION_GULAGARM_POISONED'),
						self.Promotions["Effects"]["Vampire"],
						self.Promotions["Generic"]["Chaos I"],			self.Promotions["Generic"]["Chaos II"],			self.Promotions["Generic"]["Chaos III"],
						self.Promotions["Generic"]["Death I"],			self.Promotions["Generic"]["Death II"],			self.Promotions["Generic"]["Death III"],
						self.Promotions["Generic"]["Dimensional I"],	self.Promotions["Generic"]["Dimensional II"],	self.Promotions["Generic"]["Dimensional III"],
						self.Promotions["Generic"]["Entropy I"],		self.Promotions["Generic"]["Entropy II"],		self.Promotions["Generic"]["Entropy III"],
						self.Promotions["Generic"]["Shadow I"],			self.Promotions["Generic"]["Shadow II"],		self.Promotions["Generic"]["Shadow III"]]
		for iProm in lEvilProms:
			if pUnit.isHasPromotion(iProm):					return iMane

		lGoodProms = []
		for iProm in lGoodProms:
			if pUnit.isHasPromotion(iProm):					return iAngel

		if pPlayer.isBarbarian():
			if CyGame().getSorenRandNum(100, "Barb") < 80:	return -1

		if iLeader == self.Leaders["Basium"]:				return iAngel

		iRoll = CyGame().getSorenRandNum(100, "Angel or Mane")
		iGoodThreshold = 5
		iEvilThreshold = 50
		if   iAlignment == self.Alignments["Good"]:
			iGoodThreshold += 15
			iEvilThreshold += 45
		elif iAlignment == self.Alignments["Evil"]:
			iGoodThreshold -= 100
			iEvilThreshold -= 40
		if   bPurified:
			iGoodThreshold += 20
			iEvilThreshold += 40
		if   iRoll < iGoodThreshold:	return iAngel
		elif iRoll > iEvilThreshold:	return iMane
		else:							return -1

	def findImprovements(self, iImprovementType):
		listImprovements = []
		for i in xrange (CyMap().numPlots()):
			pPlot = CyMap().plotByIndex(i)
			if pPlot.getImprovementType() == iImprovementType:
				listImprovements.append(pPlot)
		return listImprovements

	### TODO: Dictionaries
	# Demon Lord Spawn
	def spawnDemonLord(self,iLeader,iPlayer,bReassign = False):
		if self.UnitCombats == {}: self.initialize()
		iInfernalPlayer	= getOpenPlayer()
		if iInfernalPlayer == -1: return
		gc				= CyGlobalContext()
		git				= gc.getInfoTypeForString
		pBestPlot		= -1
		iBestPlot		= -1
		# Plot Picker
		for i in xrange(CyMap().numPlots()):
			pPlot = CyMap().plotByIndex(i)
			iPlot = -1
			if pPlot.isWater():				continue
			if pPlot.isCity():				continue
			if pPlot.isImpassable():		continue
			if pPlot.isPeak():				continue
			if pPlot.getNumUnits() != 0:	continue
			iPlot = CyGame().getSorenRandNum(500, "Place Demon Lord")
			iPlot += pPlot.area().getNumTiles() * 2
			iPlot += pPlot.area().getNumUnownedTiles() * 10
			if not pPlot.isOwned():
				iPlot += 500
			if pPlot.getOwner() == iPlayer:
				iPlot += 200
			if iPlot > iBestPlot:
				iBestPlot = iPlot
				pBestPlot = pPlot
		if pBestPlot.isNone(): return
		# Setting Civilization
		# Removing barbs around the plot
		pBestPlot.setPlotType(PlotTypes.PLOT_LAND, True, True)
		for iX,iY in RANGE2:
			pClearPlot = CyMap().plot(iX+pBestPlot.getX(),iY+pBestPlot.getY())
			for i in xrange(pClearPlot.getNumUnits()):
				pUnit = pPlot.getUnit(i)
				if pUnit.isBarbarian() and not gc.getUnitClassInfo(pUnit.getUnitClassType()).isUnique():
					pUnit.kill()
		# Spawning Civilization
		CyGame().addPlayerAdvanced(iInfernalPlayer, -1, iLeader, self.Civilizations["Infernal"], iPlayer)
		iFounderTeam	= gc.getPlayer(iPlayer).getTeam()
		eFounderTeam	= gc.getTeam(iFounderTeam)
		iInfernalTeam	= gc.getPlayer(iInfernalPlayer).getTeam()
		eInfernalTeam	= gc.getTeam(iInfernalTeam)
		iDemonTeam		= gc.getPlayer(gc.getDEMON_PLAYER()).getTeam()
		pInfernalPlayer	= gc.getPlayer(iInfernalPlayer)
		iX				= pBestPlot.getX()
		iY				= pBestPlot.getY()
		iAI				= UnitAITypes.NO_UNITAI
		iDirection		= DirectionTypes.DIRECTION_SOUTH
		iArcher			= self.Units["Generic"]["Longbowman"]
		iSect			= self.Units["Infernal"]["Sect of Flies"]
		iWorker			= self.Units["Generic"]["Worker"]
		iImp			= self.Units["Infernal"]["Imp"]
		iMane			= self.Units["Infernal"]["Manes"]
		iSet			= self.Units["Generic"]["Settler"]
		iMobility		= self.Promotions["Generic"]["Mobility I"]
		iStarting		= self.Promotions["Effects"]["Starting Settler"]
		iIron			= self.Promotions["Effects"]["Iron Weapons"]
		# Unique spawn for Demon Leader
		iHero = -1
		if   iLeader == self.Leaders["Hyborem"]:	iHero = self.Heroes["Hyborem"]
		elif iLeader == git("LEADER_JUDECCA"):		iHero = git("UNIT_JUDECCA")
		elif iLeader == git("LEADER_LETHE"):		iHero = git("UNIT_LETHE")
		elif iLeader == git("LEADER_MERESIN"):		iHero = git("UNIT_MERESIN")
		elif iLeader == git("LEADER_OUZZA"):		iHero = git("UNIT_OUZZA")
		elif iLeader == git("LEADER_SALLOS"):		iHero = git("UNIT_SALLOS")
		elif iLeader == git("LEADER_STATIUS"):		iHero = git("UNIT_STATIUS")
		if   iHero != -1:
			NewUnitHero = pInfernalPlayer.initUnit(iHero, iX, iY, iAI, iDirection)
			NewUnitHero.setHasPromotion(self.Promotions["Effects"]["Immortal"], True)
		# Common spawn between demons 
		NewUnitArcher1	= pInfernalPlayer.initUnit(iArcher,	iX, iY, iAI, iDirection)
		NewUnitArcher2	= pInfernalPlayer.initUnit(iArcher,	iX, iY, iAI, iDirection)
		NewUnitChamp1	= pInfernalPlayer.initUnit(iSect,	iX, iY, iAI, iDirection)
		NewUnitChamp2	= pInfernalPlayer.initUnit(iSect,	iX, iY, iAI, iDirection)
		NewUnitWorker	= pInfernalPlayer.initUnit(iWorker,	iX, iY, iAI, iDirection)
		NewUnitImp		= pInfernalPlayer.initUnit(iImp,	iX, iY, iAI, iDirection)
		NewUnitMane1	= pInfernalPlayer.initUnit(iMane,	iX, iY, iAI, iDirection)
		NewUnitMane2	= pInfernalPlayer.initUnit(iMane,	iX, iY, iAI, iDirection)
		NewUnitMane3	= pInfernalPlayer.initUnit(iMane,	iX, iY, iAI, iDirection)
		NewUnitSettler1	= pInfernalPlayer.initUnit(iSet,	iX, iY, iAI, iDirection)
		NewUnitSettler2	= pInfernalPlayer.initUnit(iSet,	iX, iY, iAI, iDirection)

		NewUnitArcher1.setHasPromotion(iMobility,	True)
		NewUnitArcher2.setHasPromotion(iMobility,	True)
		NewUnitChamp1.setHasPromotion(iMobility,	True)
		NewUnitChamp1.setHasPromotion(iIron,		True)
		NewUnitChamp2.setHasPromotion(iMobility,	True)
		NewUnitChamp2.setHasPromotion(iIron,		True)
		NewUnitImp.setHasPromotion(iMobility,		True)
		NewUnitSettler1.setHasPromotion(iStarting,	True)
		NewUnitSettler2.setHasPromotion(iStarting,	True)

		for iTech in xrange(gc.getNumTechInfos()):
			if eFounderTeam.isHasTech(iTech):
				eInfernalTeam.setHasTech(iTech, True, iInfernalPlayer, True, False)
		eFounderTeam.signOpenBorders(iInfernalTeam)
		eInfernalTeam.signOpenBorders(iFounderTeam)
		eInfernalTeam.makePeace(iDemonTeam)
		for iTeam in xrange(gc.getMAX_TEAMS()):
			if iTeam == iDemonTeam:	continue
			pTeam = gc.getTeam(iTeam)
			if not pTeam.isAlive():	continue
			if eFounderTeam.isAtWar(iTeam) or pTeam.isBarbarian():
				eInfernalTeam.declareWar(iTeam, False, WarPlanTypes.WARPLAN_LIMITED)
		pInfernalPlayer.AI_changeAttitudeExtra(iPlayer,4)
		
		if bReassign:
			CyMessageControl().sendModNetMessage(CvUtil.reassignPlayer, iPlayer, iInfernalPlayer, 0, 0)

	### TODO: Dictionaries
	# Check if equipment can`t be removed
	def canRemoveEquipment(self,pHolder,pTaker,iPromotion):
		if self.UnitCombats == {}: self.initialize()
		gc				= CyGlobalContext()
		git				= gc.getInfoTypeForString
		if pHolder != -1:
			if pHolder.getUnitType() == self.Heroes["Barnaxus"]:
				if iPromotion == git('PROMOTION_PIECES_OF_BARNAXUS'):
					return False
			elif pHolder.getUnitType() == git('UNIT_THE_HIVE'):
				if iPromotion == git('PROMOTION_PIECES_OF_THE_HIVE'):
					return False
			elif pHolder.getUnitType() == self.Heroes["Mithril Golem"]:
				if iPromotion == git('PROMOTION_PIECES_OF_MITHRIL_GOLEM'):
					return False
			elif pHolder.getUnitType() == self.Heroes["The War Machine"]:
				if iPromotion == git('PROMOTION_PIECES_OF_WAR_MACHINE'):
					return False
			elif pHolder.getUnitType() == git('UNIT_MECH_DRAGON'):
				if iPromotion == git('PROMOTION_PIECES_OF_MECH_DRAGON'):
					return False
			if pHolder.isHasPromotion(self.Promotions["Race"]["Illusion"]):
				return False
			if pHolder.isHasPromotion(git('PROMOTION_BAIR_GEM_RECHARGING')):
				if iPromotion == git('PROMOTION_BAIR_GEM_RECHARGING'):
					return False
		if pTaker != -1:
			if pTaker.getUnitCombatType() in (self.UnitCombats["Naval"], self.UnitCombats["Siege"]):
				return False
			if pTaker.getSpecialUnitType() in (git('SPECIALUNIT_SPELL'), git('SPECIALUNIT_BIRD')):
				return False
			if pTaker.isHasPromotion(self.Promotions["Race"]["Illusion"]):
				return False
		return True

	def doBarbarianWorld(self):
		lStartingPlots = []
		lValuedPlots = []
		iNumCities = 0
		pBarbPlayer = CyGlobalContext().getPlayer(CyGlobalContext().getORC_PLAYER())

		# Creating a list of starting points to avoid few checks every plot
		for iPlayer in xrange(CyGlobalContext().getMAX_PLAYERS()):
			pLoopPlayer = CyGlobalContext().getPlayer(iPlayer)
			if pLoopPlayer.isAlive():
				iNumCities += 1
				pStartingPlot = pLoopPlayer.getStartingPlot()
				if pStartingPlot.isNone(): continue
				lStartingPlots.append(pStartingPlot)

		# Early exits everywhere to shave some time
		for iPlot in xrange(CyMap().numPlots()):
			pLoopPlot = CyMap().plotByIndex(iPlot)
			bValid		= True
			iValue		= 0
			iAIValue	= 0
			iDist		= 0
			# Checks that are more likely to filter out a plot are higher
			if pLoopPlot.isWater(): continue
			elif pLoopPlot.isImpassable(): continue
			elif pLoopPlot.isPeak(): continue
			elif pLoopPlot.getBonusType(-1) != -1: continue
			elif pLoopPlot.getImprovementType() != -1: continue
			elif pLoopPlot.isCity(): continue
			elif pLoopPlot.isFoundDisabled(): continue

			iAIValue += pBarbPlayer.AI_foundValue(pLoopPlot.getX(), pLoopPlot.getY(), -1, true)
			if iAIValue == 0: continue

			for StartingPlot in lStartingPlots:
				iDist = plotDistance(pLoopPlot.getX(),pLoopPlot.getY(),StartingPlot.getX(),StartingPlot.getY())
				if iDist == -1:
					iValue += 100
				elif iDist < 7:
					bValid = False
					break
				# Without a cap on distance points, every other point increase becomes irrelevant on larger maps.
				else:
					iValue += min(iDist*5, 100)

			if not bValid: continue

			iValue += iAIValue
			iValue += CyGame().getSorenRandNum(250, "Barb World Plot Eval")
			lValuedPlots.append((iPlot,iValue))

		if not lValuedPlots: return
		# Sorting plots by iValue
		lValuedPlots.sort(key=lambda tup: tup[1], reverse=True)

		# Placing cities
		for i in xrange(iNumCities):

			iDist = 0

			if len(lValuedPlots) == 0: break

			iCityPlot = lValuedPlots[0][0]
			pCityPlot = CyMap().plotByIndex(iCityPlot)
			pBarbPlayer.found(pCityPlot.getX(), pCityPlot.getY())

			del lValuedPlots[0]

			# As plots are already valued, we need to remove plots that are too close to the placed city.
			for tLoopPlot in list(lValuedPlots):
				pLoopPlot = CyMap().plotByIndex(tLoopPlot[0])
				iDist = plotDistance(pLoopPlot.getX(),pLoopPlot.getY(),pCityPlot.getX(),pCityPlot.getY())
				if iDist > -1 and iDist < 7: lValuedPlots.remove(tLoopPlot)
