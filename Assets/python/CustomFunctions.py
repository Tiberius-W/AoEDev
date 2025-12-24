## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005

from CvPythonExtensions import *
from BasicFunctions import *
import CvUtil
import Popup as PyPopup
import PyHelpers
import CvScreenEnums
import CvCameraControls
import CvEventInterface


# globals
#gc = CyGlobalContext()
PyPlayer = PyHelpers.PyPlayer

from collections import deque

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
		self.Leaders 			= {}
		self.LeaderStatus		= {}
		self.Traits 			= {}
		self.Civics 			= {}
		self.Religions			= {}
		self.Corporations		= {}
		self.Alignments			= {}

		# Buildings, etc
		self.Projects 			= {}
		self.Buildings 			= {}
		self.Specialists		= {}
		self.BuildingClasses	= {}
		self.Processes			= {}

		# Improvements, etc
		self.Lairs 				= {}
		self.ManaNodes			= {}
		self.Improvements 		= {}
		self.CivImprovements	= {}
		self.UniqueImprovements	= {}

		# Terrain, etc
		self.Mana	 			= {}
		self.Terrain 			= {}
		self.Feature 			= {}
		self.Resources 			= {}
		self.WorldSizes			= {}

		# Units, etc
		self.Units 				= {}
		self.Heroes				= {}
		self.UnitAI				= {}
		self.Promotions 		= {}
		self.UnitClasses		= {}
		self.UnitCombats 		= {}
		self.GreatPeople 		= {}
		self.DamageTypes 		= {}

	def initialize(self):
		Manager		  		= CvEventInterface.getEventManager()
		self.Defines 		= Manager.Defines
		self.Eras 			= Manager.Eras
		self.Techs			= Manager.Techs
		self.Victories		= Manager.Victories
		self.GameSpeeds 	= Manager.GameSpeeds
		self.GameOptions 	= Manager.GameOptions
		self.EventTriggers	= Manager.EventTriggers

		self.Civilizations 	= Manager.Civilizations
		self.Leaders 		= Manager.Leaders
		self.LeaderStatus	= Manager.LeaderStatus
		self.Traits 		= Manager.Traits
		self.Civics 		= Manager.Civics
		self.Religions 		= Manager.Religions
		self.Corporations	= Manager.Corporations
		self.Alignments		= Manager.Alignments

		self.Projects 		= Manager.Projects
		self.Buildings 		= Manager.Buildings
		self.Specialists	= Manager.Specialists
		self.BuildingClasses= Manager.BuildingClasses
		self.Processes		= Manager.Processes

		self.Lairs 				= Manager.Lairs
		self.ManaNodes 			= Manager.ManaNodes
		self.Improvements 		= Manager.Improvements
		self.CivImprovements 	= Manager.CivImprovements
		self.UniqueImprovements	= Manager.UniqueImprovements

		self.Mana	 		= Manager.Mana
		self.Terrain 		= Manager.Terrain
		self.Feature 		= Manager.Feature
		self.Resources 		= Manager.Resources
		self.WorldSizes 	= Manager.WorldSizes
		self.Goodies 		= Manager.Goodies

		self.Units 			= Manager.Units
		self.Heroes			= Manager.Heroes
		self.UnitAI			= Manager.UnitAI
		self.UnitClasses 	= Manager.UnitClasses
		self.UnitCombats 	= Manager.UnitCombats
		self.GreatPeople 	= Manager.GreatPeople
		self.Promotions 	= Manager.Promotions
		self.DamageTypes 	= Manager.DamageTypes

	def showAutoPlayPopup(self):
		'Window for when user switches to AI Auto Play'
		popupSizeX = 400
		popupSizeY = 200
		screen = CyGInterfaceScreen("MainInterface", CvScreenEnums.MAIN_INTERFACE)
		xRes = screen.getXResolution()
		yRes = screen.getYResolution()
		popup = PyPopup.PyPopup(CvUtil.EventSetTurnsAutoPlayPopup, contextType = EventContextTypes.EVENTCONTEXT_ALL)
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
		popupSizeX = 400
		popupSizeY = 250
		screen     = CyGInterfaceScreen("MainInterface", CvScreenEnums.MAIN_INTERFACE)
		xRes       = screen.getXResolution()
		yRes       = screen.getYResolution()
		popup      = PyPopup.PyPopup(CvUtil.EventSetUnitPerTilePopup, contextType = EventContextTypes.EVENTCONTEXT_ALL)
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
		gc = CyGlobalContext()
		iCrusadeChance = self.Defines["Crusade Spawn"]
		pPlayer 	= gc.getPlayer(iPlayer)
		iSouth		= DirectionTypes.DIRECTION_SOUTH
		iNoAI		= UnitAITypes.NO_UNITAI

		Improvement = self.Improvements
		Unit 		= self.Units["Bannor"]

		map 		= CyMap()
		plotByIndex = map.plotByIndex
		randNum 	= CyGame().getSorenRandNum
		initUnit 	= pPlayer.initUnit
		for i in xrange(map.numPlots()):
			pPlot = plotByIndex(i)
			if pPlot.getImprovementType() == Improvement["Town (IV)"]:
				if pPlot.getOwner() == iPlayer:
					if randNum(100, "Crusade") < iCrusadeChance:
						newUnit = initUnit(Unit["Demagog"], pPlot.getX(), pPlot.getY(), iNoAI, iSouth)
						pPlot.setImprovementType(Improvement["Village (III)"])

	def doForestPush(self, pVictim, pPlot, pCaster, bResistable):
		gc = CyGlobalContext()
		pPlayer = gc.getPlayer(pVictim.getOwner())
		if pPlayer.isHasTech(self.Techs["Iron Working"]):
			return False
		if pPlayer.countNumBuildings(self.Buildings["Mines of Galdur"]) > 0:
			return False
		iX = pVictim.getX()
		iY = pVictim.getY()
		iOwner = pVictim.getOwner()
		canMove = pVictim.canMoveOrAttackInto
		map = CyMap()
		randNum = CyGame().getSorenRandNum
		getPlot = map.plot
		pVPlot = getPlot(iX,iY)
		iPlotX = pPlot.getX(); iPlotY = pPlot.getY()
		if iOwner != gc.getANIMAL_PLAYER():
			Feature = self.Feature
			if (pVPlot.getFeatureType() == Feature["Forest"] or pVPlot.getFeatureType() == Feature["Jungle"] or pVPlot.getFeatureType() == Feature["Forest New"]):
				pBestPlot = -1
				iBestPlot = 0
				for iiX,iiY in RANGE1:
					pLoopPlot = getPlot(iX+iiX,iY+iiY)
					if not pLoopPlot.isNone():
						if not pLoopPlot.isVisibleEnemyUnit(iOwner):
							if canMove(pLoopPlot, False):
								if (abs(pLoopPlot.getX() - iPlotX)>1) or (abs(pLoopPlot.getY() - iPlotY)>1):
									iRnd = randNum(500, "Forest Push Scatter choose Plot")
									if iRnd > iBestPlot:
										iBestPlot = iRnd
										pBestPlot = pLoopPlot
				if pBestPlot != -1:
					pVictim.setXY(pBestPlot.getX(), pBestPlot.getY(), False, true, true)
					if pVictim.getOwner() == gc.getORC_PLAYER():
#						pVictim.doDamage(70, 100, pCaster, getInfoType('DAMAGE_PHYSICAL'), False)
						pVictim.doDamageNoCaster(90, 100, self.DamageTypes["Physical"], False)
						return True
					else:
#						pVictim.doDamage(20, 100, pCaster, getInfoType('DAMAGE_PHYSICAL'), False)
#						pVictim.doDamageNoCaster(20, 90, getInfoType('DAMAGE_PHYSICAL'), False)
						return True
				return False

				

	
	def exploreLairBigBad(self, caster):
		gc = CyGlobalContext()
		getInfoType 		= gc.getInfoTypeForString
		getTeam 			= gc.getTeam
		getGame				= gc.getGame
		isOption			= gc.getGame().isOption
		orcTeam				= gc.getORC_TEAM()
		animalTeam			= gc.getANIMAL_TEAM()
		demonTeam			= gc.getDEMON_TEAM()
		iPlayer 			= caster.getOwner()
		pPlot 				= caster.plot()
		getPlayer 			= gc.getPlayer
		pPlayer				= getPlayer(iPlayer)
		iTeam 				= pPlayer.getTeam()
		bOrcs 				= False
		bDemons		 		= False
		bAnimals 			= False
		iSpawnPlayer 		= -1
		iCount 				= 0
		iNoAI				= UnitAITypes.NO_UNITAI
		iSouth				= DirectionTypes.DIRECTION_SOUTH
		isAtWar 			= getTeam(iTeam).isAtWar
		isAtWarOrcs 		= isAtWar(orcTeam)
		isAtWarAnimals		= isAtWar(animalTeam)
		isAtWarDemons 		= isAtWar(demonTeam)
		game 				= CyGame()
		getGlobalCounter	= game.getGlobalCounter()
		bNoBarbarians 		= isOption(GameOptionTypes.GAMEOPTION_NO_BARBARIANS)
		bNoAnimals 			= isOption(GameOptionTypes.GAMEOPTION_NO_ANIMALS)
		bNoDemons 			= isOption(GameOptionTypes.GAMEOPTION_NO_DEMONS)
		iFeature			= pPlot.getFeatureType()
		Terrain				= self.Terrain
		Feature				= self.Feature
		randNum 			= game.getSorenRandNum
		while iSpawnPlayer == -1:
			iCheckValid = randNum(3, "Choose Barbarian for BigBad")
			iCount += 1
			if iCount == 15:
				return 0
			if iCheckValid == 0 and isAtWarOrcs and not bNoBarbarians:
				iSpawnPlayer = orcTeam
				bOrcs = True
			if iCheckValid == 1 and isAtWarAnimals and not bNoAnimals:
				iSpawnPlayer = animalTeam
				bAnimals = True
			if iCheckValid == 2 and isAtWarDemons and not bNoDemons:
				iSpawnPlayer = demonTeam
				bDemons = True

		lList = []
		lHenchmanList = []
		if bDemons:
			lList += ['UNIT_AZER']
			lHenchmanList += ['UNIT_AZER']
		if bAnimals:
			lList += ['UNIT_ROC']
			lHenchmanList += ['UNIT_ROC']
		lPromoList = ['PROMOTION_MUTATED', 'PROMOTION_CANNIBALIZE', 'PROMOTION_MOBILITY1', 'PROMOTION_STRONG', 'PROMOTION_BLITZ', 'PROMOTION_COMMAND1', 'PROMOTION_HEROIC_STRENGTH', 'PROMOTION_HEROIC_DEFENSE', 'PROMOTION_MAGIC_IMMUNE', 'PROMOTION_STONESKIN', 'PROMOTION_VALOR', 'PROMOTION_VILE_TOUCH']
		if not grace():
			if bDemons:
				lList += ['UNIT_AIR_ELEMENTAL']
		if not pPlot.isWater():
			if bOrcs:
				lList += ['UNIT_OGRE', 'UNIT_ASSASSIN', 'UNIT_AXEMAN', 'UNIT_AXEMAN', 'UNIT_AXEMAN', 'UNIT_WOLF_RIDER', 'UNIT_LIZARDMAN']
				lHenchmanList += ['UNIT_AXEMAN', 'UNIT_AXEMAN', 'UNIT_AXEMAN', 'UNIT_WOLF_RIDER', 'UNIT_LIZARDMAN', 'UNIT_WARRIOR', 'UNIT_WARRIOR', 'UNIT_WARRIOR', 'UNIT_WARRIOR', 'UNIT_WARRIOR', 'UNIT_GOBLIN', 'UNIT_GOBLIN', 'UNIT_GOBLIN', 'UNIT_GOBLIN', 'UNIT_GOBLIN']
			if bAnimals:
				lList += ['UNIT_GIANT_SPIDER', 'UNIT_LION', 'UNIT_RED_DRAKE']
				lHenchmanList += ['UNIT_WOLF', 'UNIT_LION', 'UNIT_TIGER', 'UNIT_BABY_SPIDER', 'UNIT_FAWN']
			if bDemons:
				lList += ['UNIT_SPECTRE', 'UNIT_HELLHOUND', 'UNIT_SKELETON', 'UNIT_SKELETON', 'UNIT_SKELETON', 'UNIT_SUCCUBUS', 'UNIT_DROWN', 'UNIT_DISEASED_CORPSE', 'UNIT_DISEASED_CORPSE', 'UNIT_DISEASED_CORPSE', 'UNIT_PYRE_ZOMBIE']
				lHenchmanList += ['UNIT_CHAOS_MARAUDER', 'UNIT_MISTFORM', 'UNIT_SKELETON', 'UNIT_SKELETON', 'UNIT_SKELETON', 'UNIT_DISEASED_CORPSE', 'UNIT_DISEASED_CORPSE', 'UNIT_DISEASED_CORPSE', 'UNIT_HELLHOUND', 'UNIT_HELLHOUND']
			if not grace():
				if bOrcs:
					lList += ['UNIT_OGRE_WARCHIEF']
					lHenchmanList += ['UNIT_OGRE']
				if bAnimals:
					lList += ['UNIT_MYCONID', 'UNIT_SATYR']
				if bDemons:
					lList += ['UNIT_EARTH_ELEMENTAL', 'UNIT_FIRE_ELEMENTAL', 'UNIT_GARGOYLE', 'UNIT_VAMPIRE', 'UNIT_EIDOLON', 'UNIT_LICH']
				lPromoList = lPromoList + ['PROMOTION_FIRE2', 'PROMOTION_AIR2', 'PROMOTION_HERO', 'PROMOTION_MARKSMAN', 'PROMOTION_SHADOWWALK']
				if iFeature == Feature["Forest"] or iFeature == Feature["Ancient Forest"]:
					if bAnimals:
						lList += ['UNIT_TREANT']
			if pPlot.getTerrainType() == Terrain["Tundra"]:
				if bOrcs:
					lHenchmanList += ['UNIT_FROSTLING_ARCHER', 'UNIT_FROSTLING_WOLF_RIDER']
				if bAnimals:
					lHenchmanList += ['UNIT_WHITE_DRAKE']
			if pPlot.isHills():
				if bOrcs:
					lList += ['UNIT_HILL_GIANT']
				lPromoList = ['PROMOTION_GUERILLA1','PROMOTION_GUERILLA2']
			if iFeature == Feature["Forest"] or iFeature == Feature["Ancient Forest"]:
				if bAnimals:
					lList += ['UNIT_TREANT','UNIT_SATYR']
					lHenchmanList += ['UNIT_TIGER', 'UNIT_BABY_SPIDER', 'UNIT_FAWN']
				lPromoList = lPromoList + ['PROMOTION_WOODSMAN1', 'PROMOTION_WOODSMAN2']
			if pPlot.getImprovementType() == self.Lair["Barrow"]:
				if bDemons:
					lList += ['UNIT_SPECTRE']
					lHenchmanList += ['UNIT_SKELETON', 'UNIT_PYRE_ZOMBIE']
				lPromoList = lPromoList + ['PROMOTION_DEATH2']
				if not grace():
					if bDemons:
						lList += ['UNIT_LICH', 'UNIT_WRAITH']
			if pPlot.getImprovementType() == self.Lair["Ruins"]:
				lPromoList = lPromoList + ['PROMOTION_POISONED_BLADE']
				if bOrcs:
					lHenchmanList += ['UNIT_LIZARDMAN']
				if bAnimals:
					lHenchmanList += ['UNIT_GORILLA_TROOP']
				if not grace():
					if bDemons:
						lList += ['UNIT_MANTICORE']
			if getGlobalCounter > 40:
				if bDemons:
					lList += ['UNIT_PIT_BEAST', 'UNIT_DEATH_KNIGHT', 'UNIT_BALOR']
					lHenchmanList += ['UNIT_IMP', 'UNIT_HELLHOUND']
				lPromoList = lPromoList + ['PROMOTION_FEAR']
		if pPlot.isWater():
			if bOrcs:
				lList += ['UNIT_PIRATE']
				lHenchmanList += ['UNIT_PIRATE']
			if bAnimals:
				lList += ['UNIT_SEA_SERPENT']
			if bDemons:
				lList += ['UNIT_STYGIAN_GUARD']
				lHenchmanList += ['UNIT_DROWN']
			if not grace():
				if bAnimals:
					lList += ['UNIT_KRAKEN']
				if bDemons:
					lList += ['UNIT_WATER_ELEMENTAL']

		sMonster 	= lList[randNum(len(lList), "Pick Monster")]
		sHenchman 	= lHenchmanList[randNum(len(lHenchmanList), "Pick Henchman")-1]
		iUnit 		= getInfoType(sMonster)
		iHenchman 	= getInfoType(sHenchman)
		newUnit 	= addUnitFixed(pPlot, iUnit, iSpawnPlayer)
		if newUnit != -1:
			setPromo 	= newUnit.setHasPromotion
			for i in xrange(randNum(len(lPromoList)/4 + 1, "Pick Promotion Quantity")):
				sPromo = lPromoList[randNum(len(lPromoList), "Pick Promotion")]
				setPromo(getInfoType(sPromo), True)
			newUnit.setName(self.MarnokNameGenerator(newUnit))
			CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_EXPLORE_LAIR_BIGBAD",()),'',1,gc.getUnitInfo(iUnit).getButton(),ColorTypes(7),newUnit.getX(),newUnit.getY(),True,True)
			bPlayer = getPlayer(iSpawnPlayer)
			iHench1 = randNum(4, "BigBad Lair Henchmen number 1")
			iHench2 = randNum(4, "BigBad Lair Henchmen number 2")
			iNoBadMod = caster.getNoBadExplore()/10
			iHenchtotal = iHench1 + iHench2 - iNoBadMod
			getHandicap = getGame().getHandicapType()
			if iHenchtotal > int(getHandicap):
				iHenchtotal = int(getHandicap)
			initUnit = bPlayer.initUnit
			iX = newUnit.getX(); iY = newUnit.getY()
			for i in xrange(iHenchtotal):
				initUnit(iHenchman, iX, iY, iNoAI, iSouth)
		return 0

	def exploreLairBad(self, caster):
		gc 			= CyGlobalContext()
		iPlayer 	= caster.getOwner()
		pPlot 		= caster.plot()
		pPlayer 	= gc.getPlayer(caster.getOwner())
		recGoody	= pPlayer.receiveGoody
		canGoody	= pPlayer.canReceiveGoody
		iRnd 		= CyGame().getSorenRandNum(100, "Lair Bad Result List")
		Goody		= self.Goodies

		iRnd += caster.getNoBadExplore()/2

		if iRnd <= 10:
			caster.kill(True,0)
			CyInterface().addMessage(caster.getOwner(),True,25,CyTranslator().getText("TXT_KEY_MESSAGE_EXPLORE_LAIR_DEATH", ()),'',1,'Art/Interface/Buttons/Spells/Explore Lair.dds',ColorTypes(7),pPlot.getX(),pPlot.getY(),True,True)
			return 0
		if iRnd <= 13:
			if canGoody(pPlot,  Goody["Plagued"], caster):
				recGoody(pPlot, Goody["Plagued"], caster)
				return 50
		if iRnd <= 16:
			if canGoody(pPlot,  Goody["Diseased"], caster):
				recGoody(pPlot, Goody["Diseased"], caster)
				return 80
		if iRnd <= 19:
			if canGoody(pPlot,  Goody["Withered"], caster):
				recGoody(pPlot, Goody["Withered"], caster)
				return 80
		if iRnd <= 21:
			if canGoody(pPlot,  Goody["Possessed"], caster):
				recGoody(pPlot, Goody["Possessed"], caster)
				return 80
		if iRnd <= 27:
			if canGoody(pPlot,  Goody["Crazed"], caster):
				recGoody(pPlot, Goody["Crazed"], caster)
				return 80
		if iRnd <= 33:
			if canGoody(pPlot,  Goody["Spider"], caster):
				recGoody(pPlot, Goody["Spider"], caster)
				return 80
		if iRnd <= 60:
			if canGoody(pPlot,  Goody["Grave - Spectre"], caster):
				recGoody(pPlot, Goody["Grave - Spectre"], caster)
				return 80
		if iRnd <= 63:
			if canGoody(pPlot,  Goody["Sea Serpent"], caster):
				recGoody(pPlot, Goody["Sea Serpent"], caster)
				return 80
		if iRnd <= 66:
			if canGoody(pPlot,  Goody["Drown"], caster):
				recGoody(pPlot, Goody["Drown"], caster)
				return 80
		if iRnd <= 69:
			if canGoody(pPlot,  Goody["Rusted"], caster):
				recGoody(pPlot, Goody["Rusted"], caster)
				return 50
		if iRnd <= 72:
			if canGoody(pPlot,  Goody["Enraged"], caster):
				recGoody(pPlot, Goody["Enraged"], caster)
				return 50
		if iRnd <= 75:
			if canGoody(pPlot,  Goody["Poisoned"], caster):
				caster.doDamageNoCaster(25, 90, self.DamageTypes["Poison"], False)
				recGoody(pPlot, Goody["Poisoned"], caster)
				return 50
		if iRnd <= 90:
			caster.doDamageNoCaster(50, 90, self.DamageTypes["Physical"], False)
			CyInterface().addMessage(caster.getOwner(),True,25,CyTranslator().getText("TXT_KEY_MESSAGE_EXPLORE_LAIR_COLLAPSE", ()),'',1,'Art/Interface/Buttons/Spells/Explore Lair.dds',ColorTypes(7),pPlot.getX(),pPlot.getY(),True,True)
			return 100

		return self.exploreLairNeutral(caster)

	def exploreLairNeutral(self, caster):
		gc 				= CyGlobalContext()
		getInfoType		= gc.getInfoTypeForString
		eventTrigger	= gc.getEventTriggerInfo
		iPlayer 		= caster.getOwner()
		pPlot 			= caster.plot()
		pPlayer 		= gc.getPlayer(caster.getOwner())
		recGoody		= pPlayer.receiveGoody
		canGoody		= pPlayer.canReceiveGoody
		Goody			= self.Goodies

		iRnd 			= CyGame().getSorenRandNum(100, "Lair Neutral Result List")
		iRnd += caster.getNoBadExplore()/2

		if iRnd <= 60:
			Lair		 = self.Lairs
			eImprovement = pPlot.getImprovementType()
			if pPlot.isWater():
				if canGoody(pPlot,  Goody["Drown"], caster):
					recGoody(pPlot, Goody["Drown"], caster)
					return 50
			if   eImprovement == Lair["Steading"]:
				if canGoody(pPlot,  Goody["Hill Giant"], caster):
					recGoody(pPlot, Goody["Hill Giant"], caster)
					return 80
			elif eImprovement == Lair["Barrow"]:
				if canGoody(pPlot,  Goody["Skeleton"], caster):
					recGoody(pPlot, Goody["Skeleton"], caster)
					return 80
			elif eImprovement == Lair["Ruins"]:
				if canGoody(pPlot,  Goody["Lizardman"], caster):
					recGoody(pPlot, Goody["Lizardman"], caster)
					return 80
			elif eImprovement == Lair["Dungeon"]:
				if canGoody(pPlot,  Goody["Minotaur"], caster):
					recGoody(pPlot, Goody["Minotaur"], caster)
					return 80
			elif eImprovement == Lair["Goblin Camp"]:
				if canGoody(pPlot,  Goody["Troll"], caster):
					recGoody(pPlot, Goody["Troll"], caster)
					return 80
			elif canGoody(pPlot, Goody["Spider"], caster):
				recGoody(pPlot,  Goody["Spider"], caster)
				return 80

		initTriggerData = pPlayer.initTriggeredData
		Event			= self.EventTriggers
		if iRnd <= 65:
			iUnitID = getUnitPlayerID(caster)
			if iUnitID != -1:
				initTriggerData( Event["Lair Portal"], true, -1, caster.getX(), caster.getY(), caster.getOwner(), -1, -1, -1, iUnitID, -1)
				return 0
		if iRnd <= 70:
			iUnitID = getUnitPlayerID(caster)
			if iUnitID != -1:
				initTriggerData( Event["Dwarf vs Lizardmen"], true, -1, caster.getX(), caster.getY(), caster.getOwner(), -1, -1, -1, iUnitID, -1)
				return 100
		if iRnd <= 75:
			if canGoody(pPlot,  Goody["Mutated"], caster):
				recGoody(pPlot, Goody["Mutated"], caster)
				return 50
		if iRnd <= 90:
			iUnitID = getUnitPlayerID(caster)
			if iUnitID != -1:
				initTriggerData( Event["Lair Depths"], true, -1, caster.getX(), caster.getY(), caster.getOwner(), -1, -1, -1, iUnitID, -1)
				return 100

		return self.exploreLairGood(caster)

	def exploreLairGood(self, caster):
		gc 			= CyGlobalContext()
		getInfoType	= gc.getInfoTypeForString
		iPlayer 	= caster.getOwner()
		pPlot 		= caster.plot()
		pPlayer 	= gc.getPlayer(caster.getOwner())
		recGoody	= pPlayer.receiveGoody
		canGoody	= pPlayer.canReceiveGoody
		Goody		= self.Goodies

		iRnd 		= CyGame().getSorenRandNum(100, "Lair Good Result List")
		iRnd += caster.getNoBadExplore()/2

		if iRnd <= 5:
			if canGoody(pPlot,  Goody["Healing Salve"], caster):
				recGoody(pPlot, Goody["Healing Salve"], caster)
				return 100
		if iRnd <= 10:
			if canGoody(pPlot,  Goody["Spirit Guide"], caster):
				recGoody(pPlot, Goody["Spirit Guide"], caster)
				return 80
		if iRnd <= 15:
			if canGoody(pPlot,  Goody["Experience"], caster):
				recGoody(pPlot, Goody["Experience"], caster)
				return 100
		if iRnd <= 25:
			if canGoody(pPlot,  Goody["Supplies"], caster):
				recGoody(pPlot, Goody["Supplies"], caster)
				return 100
		if iRnd <= 30:
			if canGoody(pPlot,  Goody["Potion of Restoration"], caster):
				recGoody(pPlot, Goody["Potion of Restoration"], caster)
				return 100
		if iRnd <= 35:
			if canGoody(pPlot,  Goody["Potion of Invisibility"], caster):
				recGoody(pPlot, Goody["Potion of Invisibility"], caster)
				return 100
		if iRnd <= 45:
			if canGoody(pPlot,  Goody["Shield of Faith"], caster):
				recGoody(pPlot, Goody["Shield of Faith"], caster)
				return 100
		if iRnd <= 55:
			if canGoody(pPlot,  Goody["Enchanted Blade"], caster):
				recGoody(pPlot, Goody["Enchanted Blade"], caster)
				return 100
			if canGoody(pPlot,  Goody["Spellstaff"], caster):
				recGoody(pPlot, Goody["Spellstaff"], caster)
				return 100
			if canGoody(pPlot,  Goody["Poisoned Blade"], caster):
				recGoody(pPlot, Goody["Poisoned Blade"], caster)
				return 100
			if canGoody(pPlot,  Goody["Flaming Arrows"], caster):
				recGoody(pPlot, Goody["Flaming Arrows"], caster)
				return 100
		if iRnd <= 70:
			isHasPromotion 	= caster.isHasPromotion
			setHasPromotion = caster.setHasPromotion
			Promo			= self.Promotions["Effects"]
			pUnit 		= gc.getUnitInfo(caster.getUnitType())
			iWeapMax 	= pUnit.getWeaponTierMax()
			iWeapMin 	= pUnit.getWeaponTierMin()
			if iWeapMax >= 1 and iWeapMin <= 3:
				if not isHasPromotion( Promo["Mithril Weapons"]):
					if not isHasPromotion( Promo["Iron Weapons"]):
						if (iWeapMax >= 2 and iWeapMin <= 2 and pPlayer.isHasTech( self.Techs["Bronze Working"])):
							if isHasPromotion( Promo["Rusted"]):
								setHasPromotion( Promo["Rusted"], False)
							setHasPromotion( Promo["Iron Weapons"], True)
							setHasPromotion( Promo["Bronze Weapons"], False)
							CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_EXPLORE_LAIR_IRON_WEAPONS",()),'',1,'Art/Interface/Buttons/Promotions/IronWeapons.dds',ColorTypes(8),pPlot.getX(),pPlot.getY(),True,True)
							return 100
						if not isHasPromotion( Promo["Bronze Weapons"]):
							if isHasPromotion( Promo["Rusted"]):
								setHasPromotion( Promo["Rusted"],False)
							setHasPromotion( Promo["Bronze Weapons"], True)
							CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_EXPLORE_LAIR_BRONZE_WEAPONS",()),'',1,'Art/Interface/Buttons/Promotions/BronzeWeapons.dds',ColorTypes(8),pPlot.getX(),pPlot.getY(),True,True)
							return 100
		if iRnd <= 75:
			if canGoody(pPlot,  Goody["Prisoner Assassin"], caster):
				recGoody(pPlot, Goody["Prisoner Assassin"], caster)
				return 100
		if iRnd <= 80:
			if canGoody(pPlot,  Goody["Prisoner Champion"], caster):
				recGoody(pPlot, Goody["Prisoner Champion"], caster)
				return 100
		if iRnd <= 85:
			if canGoody(pPlot,  Goody["Prisoner Mage"], caster):
				recGoody(pPlot, Goody["Prisoner Mage"], caster)
				return 100
		if iRnd <= 90:
			if canGoody(pPlot,  Goody["Prisoner Monk"], caster):
				recGoody(pPlot, Goody["Prisoner Monk"], caster)
				return 100
		if iRnd <= 95:
			if canGoody(pPlot,  Goody["Prisoner Angel"], caster):
				recGoody(pPlot, Goody["Prisoner Angel"], caster)
				return 100
		if iRnd <= 100:
			placeTreasure(iPlayer, self.Units["Equipment"]["Treasure"])
			return 80

		return self.exploreLairBigGood(caster)

	def exploreLairGoodEquipment(self, caster):
		gc 			= CyGlobalContext()
		getInfoType	= gc.getInfoTypeForString
		iPlayer 	= caster.getOwner()
		pPlot 		= caster.plot()
		pPlayer 	= gc.getPlayer(caster.getOwner())
		recGoody	= pPlayer.receiveGoody
		canGoody	= pPlayer.canReceiveGoody
		eFeature	= pPlot.getFeatureType()
		eTerrain	= pPlot.getTerrainType()
		Terrain		= self.Terrain
		Feature		= self.Feature
		Goody		= self.Goodies

		lList = []
		if canGoody(pPlot, Goody["Healing Salve"], caster):
			lList = ['ITEM_HEALING_SALVE']
		if pPlot.isHills():
			if canGoody(pPlot, Goody["Climbing Kit - Recon"], caster):
				lList = lList + ['PROMOTION_CLIMBING_KIT_RECON']
		if eTerrain == Terrain["Desert"]:
			if canGoody(pPlot, Goody["Desert Gear - Recon"], caster):
				lList = lList + ['PROMOTION_DESERT_GEAR_RECON']
		if eTerrain == Terrain["Tundra"] or eTerrain == Terrain["Taiga"]:
			if canGoody(pPlot, Goody["Snow Gear - Recon"], caster):
				lList = lList + ['PROMOTION_SNOW_GEAR_RECON']
		if eFeature == Feature["Forest"] or eFeature == Feature["Ancient Forest"]:
			if canGoody(pPlot, Goody["Woods Gear - Recon"], caster):
				lList = lList + ['PROMOTION_WOODS_GEAR_RECON']
		if canGoody(pPlot, Goody["Fine Kit - Recon"], caster):
			lList = lList + ['PROMOTION_FINE_KIT_RECON']
		if canGoody(pPlot, Goody["Mantraps - Recon"], caster):
			lList = lList + ['PROMOTION_MANTRAPS_RECON']

		if len(lList) > 0:
			sGoody = lList[CyGame().getSorenRandNum(len(lList), "Pick Goody")]
			if sGoody == 'ITEM_HEALING_SALVE':
				recGoody(pPlot, Goody["Healing Salve"], caster)
				return 100
			if sGoody == 'PROMOTION_CLIMBING_KIT_RECON':
				recGoody(pPlot, Goody["Climbing Kit - Recon"], caster)
				return 100
			if sGoody == 'PROMOTION_DESERT_GEAR_RECON':
				recGoody(pPlot, Goody["Desert Gear - Recon"], caster)
				return 100
			if sGoody == 'PROMOTION_FINE_KIT_RECON':
				recGoody(pPlot, Goody["Fine Kit - Recon"], caster)
				return 100
			if sGoody == 'PROMOTION_MANTRAPS_RECON':
				recGoody(pPlot, Goody["Mantraps - Recon"], caster)
				return 100
			if sGoody == 'PROMOTION_SNOW_GEAR_RECON':
				recGoody(pPlot, Goody["Snow Gear - Recon"], caster)
				return 100
			if sGoody == 'PROMOTION_WOODS_GEAR_RECON':
				recGoody(pPlot, Goody["Woods Gear - Recon"], caster)
				return 100
		return self.exploreLairGood(caster)

	def exploreLairBigGood(self, caster):
		gc 			= CyGlobalContext()
		getInfoType	= gc.getInfoTypeForString
		iPlayer 	= caster.getOwner()
		pPlot 		= caster.plot()
		pPlayer 	= gc.getPlayer(caster.getOwner())
		game		= CyGame()
		iRnd 		= game.getSorenRandNum(100, "Lair BigGood Result List")
		iRnd += caster.getNoBadExplore()/2
		recGoody	= pPlayer.receiveGoody
		canGoody	= pPlayer.canReceiveGoody
		Resource	= self.Resources
		Tech		= self.Techs
		Goody		= self.Goodies

		if iRnd <= 25:
			if canGoody(pPlot,  Goody["Grave - Tech"], caster):
				recGoody(pPlot, Goody["Grave - Tech"], caster)
				return 100
		if iRnd <= 75:
			if pPlot.isWater():
				if iRnd <= 25:
					if canGoody(pPlot, Goody["Prisoner Serpent"], caster):
						recGoody(pPlot,Goody["Prisoner Serpent"], caster)
						return 100
				if iRnd <= 40:
					if pPlot.getBonusType(-1) == -1:
						pPlot.setBonusType(Resource["Clam"])
						CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_EXPLORE_LAIR_BONUS_CLAM",()),'',1,'Art/Interface/Buttons/WorldBuilder/Crab.dds',ColorTypes(8),pPlot.getX(),pPlot.getY(),True,True)
						return 100
				if iRnd <= 55:
					if pPlot.getBonusType(-1) == -1:
						pPlot.setBonusType(Resource["Crab"])
						CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_EXPLORE_LAIR_BONUS_CRAB",()),'',1,'Art/Interface/Buttons/WorldBuilder/Crab.dds',ColorTypes(8),pPlot.getX(),pPlot.getY(),True,True)
						return 100
				if iRnd <= 70:
					if pPlot.getBonusType(-1) == -1:
						pPlot.setBonusType(Resource["Fish"])
						CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_EXPLORE_LAIR_BONUS_FISH",()),'',1,'Art/Interface/Buttons/WorldBuilder/Crab.dds',ColorTypes(8),pPlot.getX(),pPlot.getY(),True,True)
						return 100
			if not pPlot.isWater():
				if iRnd <= 30:
					return self.exploreLairGoodEquipment(caster)
				if iRnd <= 33:
					if pPlot.getBonusType(-1) == -1:
						pPlot.setBonusType(Resource["Mana"])
						CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_EXPLORE_LAIR_BONUS_MANA",()),'',1,'Art/Interface/Buttons/WorldBuilder/mana_button.dds',ColorTypes(8),pPlot.getX(),pPlot.getY(),True,True)
						return 100
				if iRnd <= 36:
					if pPlot.getBonusType(-1) == -1:
						if pPlayer.isHasTech(Tech["Mining"]):
							pPlot.setBonusType(Resource["Copper"])
							CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_EXPLORE_LAIR_BONUS_COPPER",()),'',1,'Art/Interface/Buttons/WorldBuilder/Copper.dds',ColorTypes(8),pPlot.getX(),pPlot.getY(),True,True)
							return 100
				if iRnd <= 38:
					if pPlot.getBonusType(-1) == -1:
						if pPlayer.isHasTech(Tech["Mining"]):
							pPlot.setBonusType(Resource["Gold"])
							CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_EXPLORE_LAIR_BONUS_GOLD",()),'',1,'Art/Interface/Buttons/WorldBuilder/Gold.dds',ColorTypes(8),pPlot.getX(),pPlot.getY(),True,True)
							return 100
				if iRnd <= 40:
					if pPlot.getBonusType(-1) == -1:
						if pPlayer.isHasTech(Tech["Mining"]):
							pPlot.setBonusType(Resource["Gems"])
							CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_EXPLORE_LAIR_BONUS_GEMS",()),'',1,'Art/Interface/Buttons/WorldBuilder/Gems.dds',ColorTypes(8),pPlot.getX(),pPlot.getY(),True,True)
							return 100
				if iRnd <= 43:
					if pPlot.getBonusType(-1) == -1:
						if pPlayer.isHasTech(Tech["Smelting"]):
							pPlot.setBonusType(Resource["Iron"])
							CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_EXPLORE_LAIR_BONUS_IRON",()),'',1,'Art/Interface/Buttons/WorldBuilder/Iron.dds',ColorTypes(8),pPlot.getX(),pPlot.getY(),True,True)
							return 100
				if iRnd <= 45:
					if canGoody(pPlot,  Goody["Jade Torc"], caster):
						recGoody(pPlot, Goody["Jade Torc"], caster)
						return 100
				if iRnd <= 47:
					if canGoody(pPlot,  Goody["Rod of Winds"], caster):
						recGoody(pPlot, Goody["Rod of Winds"], caster)
						return 100
				if iRnd <= 49:
					if canGoody(pPlot,  Goody["Timor Mask"], caster):
						recGoody(pPlot, Goody["Timor Mask"], caster)
						return 100
				if iRnd <= 52:
					if canGoody(pPlot,  Goody["Prisoner Adventurer"], caster):
						recGoody(pPlot, Goody["Prisoner Adventurer"], caster)
						return 100
				if iRnd <= 55:
					if canGoody(pPlot,  Goody["Prisoner Artist"], caster):
						recGoody(pPlot, Goody["Prisoner Artist"], caster)
						return 100
				if iRnd <= 58:
					if canGoody(pPlot,  Goody["Prisoner Commander"], caster):
						recGoody(pPlot, Goody["Prisoner Commander"], caster)
						return 100
				if iRnd <= 61:
					if canGoody(pPlot,  Goody["Prisoner Engineer"], caster):
						recGoody(pPlot, Goody["Prisoner Engineer"], caster)
						return 100
				if iRnd <= 64:
					if canGoody(pPlot,  Goody["Prisoner Merchant"], caster):
						recGoody(pPlot, Goody["Prisoner Merchant"], caster)
						return 100
				if iRnd <= 67:
					if canGoody(pPlot,  Goody["Prisoner Prophet"], caster):
						recGoody(pPlot, Goody["Prisoner Prophet"], caster)
						return 100
				if iRnd <= 70:
					if canGoody(pPlot,  Goody["Prisoner Scientist"], caster):
						recGoody(pPlot, Goody["Prisoner Scientist"], caster)
						return 100
		if not grace():
			if iRnd <= 75:
				if canGoody(pPlot,  Goody["Prisoner Assassin"], caster):
					recGoody(pPlot, Goody["Prisoner Assassin"], caster)
					return 100
			if iRnd <= 80:
				if canGoody(pPlot,  Goody["Prisoner Champion"], caster):
					recGoody(pPlot, Goody["Prisoner Champion"], caster)
					return 100
			if iRnd <= 85:
				if canGoody(pPlot,  Goody["Prisoner Mage"], caster):
					recGoody(pPlot, Goody["Prisoner Mage"], caster)
					return 100
			if iRnd <= 90:
				if canGoody(pPlot,  Goody["Prisoner Monk"], caster):
					recGoody(pPlot, Goody["Prisoner Monk"], caster)
					return 100
			if iRnd <= 95:
				if canGoody(pPlot,  Goody["Prisoner Angel"], caster):
					recGoody(pPlot, Goody["Prisoner Angel"], caster)
					return 100
		pPlayer.changeGoldenAgeTurns(game.goldenAgeLength())
		CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_EXPLORE_LAIR_GOLDEN_AGE",()),'',1,'Art/Interface/Buttons/Spells/Explore Lair.dds',ColorTypes(8),pPlot.getX(),pPlot.getY(),True,True)
		return 100

	def formEmpire(self, iCiv, iLeader, iTeam, pCity, iAlignment, pFromPlayer):
		gc 		= CyGlobalContext()
		iPlayer = getOpenPlayer()
		pPlot 	= pCity.plot()
		pPlot2 	= findClearPlot(-1, pCity.plot())
		getUnit = pPlot.getUnit
		if (iPlayer != -1 and pPlot2 != -1):
			iX = pPlot2.getX(); iY = pPlot2.getY()
			for i in xrange(pPlot.getNumUnits(), -1, -1):
				pUnit = getUnit(i)
				pUnit.setXY(iX, iY, true, true, true)
			CyGame().addPlayerAdvanced(iPlayer, iTeam, iLeader, iCiv,pFromPlayer.getID())
			pPlayer = gc.getPlayer(iPlayer)
			initUnit= pPlayer.initUnit
			getTeam = gc.getTeam
			iTeam 	= pPlayer.getTeam()
			if pFromPlayer != -1:
				eFromTeam = getTeam(pFromPlayer.getTeam())
				pTeam = getTeam(iTeam)
				setTech = pTeam.setHasTech
				hasTech = eFromTeam.isHasTech
				for iTech in xrange(gc.getNumTechInfos()):
					if hasTech(iTech):
						setTech(iTech, true, iPlayer, true, False)
				if not pFromPlayer.getTeam() == iTeam:
					if not eFromTeam.isAtWar(gc.getORC_TEAM()):
						eFromTeam.makePeace(gc.getORC_TEAM())
					if not eFromTeam.isAtWar(gc.getANIMAL_TEAM()):
						eFromTeam.makePeace(gc.getANIMAL_TEAM())
					if not eFromTeam.isAtWar(gc.getDEMON_TEAM()):
						eFromTeam.makePeace(gc.getDEMON_TEAM())
			pPlayer.acquireCity(pCity, False, False)
			pCity = pPlot.getPlotCity()
			pCity.changeCulture(iPlayer, 100, True)
			iX = pPlot.getX(); iY = pPlot.getY();
			iNoAI 	= UnitAITypes.NO_UNITAI
			iSouth 	= DirectionTypes.DIRECTION_SOUTH
			Unit 	= self.Units["Generic"]
			initUnit(Unit["Archer"], iX, iY, iNoAI, iSouth)
			initUnit(Unit["Archer"], iX, iY, iNoAI, iSouth)
			initUnit(Unit["Archer"], iX, iY, iNoAI, iSouth)
			initUnit(Unit["Archer"], iX, iY, iNoAI, iSouth)
			initUnit(Unit["Archer"], iX, iY, iNoAI, iSouth)
			if iAlignment != -1:
				pPlayer.setAlignment(iAlignment)

	def doCityFire(self, pCity):
		gc 				= CyGlobalContext()
		addMessage 		= CyInterface().addMessage
		getText 		= CyTranslator().getText
		iCount 			= 0
		getNumRealB 	= pCity.getNumRealBuilding
		getBuildingInfo = gc.getBuildingInfo
		randNum 		= CyGame().getSorenRandNum
		iOwner 			= pCity.getOwner()
		Building		= self.Buildings
		setNumB			= pCity.setNumRealBuilding
		iX = pCity.getX(); iY = pCity.getY()
		for iBuilding in xrange(gc.getNumBuildingInfos()):
			if iBuilding != Building["Demonic Citizens"]:
				if getNumRealB(iBuilding) > 0:
					if getBuildingInfo(iBuilding).getConquestProbability() != 100:
						if not isLimitedWonderClass(gc.getBuildingInfo(iBuilding).getBuildingClassType()):
							if randNum(100, "City Fire") <= 10:
								setNumB(iBuilding, 0)
								addMessage(iOwner,True,25,getText("TXT_KEY_MESSAGE_CITY_FIRE",(getBuildingInfo(iBuilding).getDescription(), )),'',1,getBuildingInfo(iBuilding).getButton(),ColorTypes(8),iX,iY,True,True)
								iCount += 1
		if iCount == 0:
			addMessage(iOwner,True,25,getText("TXT_KEY_MESSAGE_CITY_FIRE_NO_DAMAGE",()),'AS2D_SPELL_FIRE_ELEMENTAL',1,'Art/Interface/Buttons/Fire.dds',ColorTypes(8),iX,iY,True,True)

	def doFFTurn(self):
		gc 			= CyGlobalContext()
		game 		= CyGame()
		Option		= self.GameOptions
		Civ			= self.Civilizations
		getNum		= game.getNumCivActive

		iAreThereLizardsHere 	= getNum(Civ["Mazatl"]) + getNum(Civ["Cualli"])
		iAreThereScionsHere		= getNum(Civ["Scions"])
		iAreThereMalakimHere  	= getNum(Civ["Malakim"])
		if not (iAreThereLizardsHere or iAreThereScionsHere or not Option["No Plot Counter"]):
			return

		map 			= CyMap()
		Terrain 		= self.Terrain
		Feature 		= self.Feature
		UnitCombat		= self.UnitCombats
		UnitClass		= self.UnitClasses
		Improvement		= self.Improvements
		Mana			= self.Mana
		Rel				= self.Religions
		Bonus			= self.Resources
		Define			= self.Defines
		Alignment		= self.Alignments
		iMarshChance	= 5
		iPlainsChance 	= 2
		iJungleChance 	= 10
		iSwampChance 	= 25
		iGameTurn 		= game.getGameTurn()
		iGameSpeed		= gc.getGameSpeedInfo(game.getGameSpeedType()).getTrainPercent()
		iGameSpeedMod 	= iGameSpeed / 5
		interface 		= CyInterface()
		iCount			= game.getGlobalCounter()
		getPlot 		= map.plot

		byIndex 	= map.plotByIndex
		getPlayer 	= gc.getPlayer
		randNum 	= game.getSorenRandNum
		for i in xrange(map.numPlots()):
			pPlot			 	= byIndex(i)
			if pPlot == None: continue
			iBonus 				= pPlot.getBonusType(-1)
			iFeature 			= pPlot.getFeatureType()
			iPlotEffect 		= pPlot.getPlotEffectType()
			iImprovement 		= pPlot.getImprovementType()
			iTerrain 			= pPlot.getTerrainType()
			bIsOwned 			= pPlot.isOwned()
			setFeature 			= pPlot.setFeatureType
			setImprov			= pPlot.setImprovementType
			bPeak				= pPlot.isPeak()
			bCity				= pPlot.isCity()

			if bIsOwned:
				iOwner 			= pPlot.getOwner()
				pPlayer 		= getPlayer(iOwner)
				iAlignment 		= pPlayer.getAlignment()
				iStateReligion	= pPlayer.getStateReligion()
				eCiv 			= pPlayer.getCivilizationType()

		#### Lizard Terrain Section
			if iAreThereLizardsHere:
				if bIsOwned:
					if eCiv == Civ["Mazatl"] or eCiv == Civ["Cualli"]:
						bHills = pPlot.isHills()
						if pPlot.getRouteType() == Improvement["Road"]:
							if randNum(100, "Trail") < 20 :
								pPlot.setRouteType(Improvement["Trail"])

						if iTerrain == Terrain["Marsh"]:
							if iImprovement == -1 and iBonus == -1 and not bCity and not bPeak:
								iChance = iSwampChance
								if not pPlayer.isHuman(): 	iChance *=2
								if randNum(1000, "Swamp") < iChance:
									setImprov(Improvement["Swamp"])
							if iFeature == Feature["Forest"] or iFeature == Feature["Ancient Forest"]:
								iChance = iJungleChance * 5
								if randNum(1000, "Jungle") < iChance :
									setFeature(Feature["Jungle"], 0)
							if iFeature == -1:
								iChance = iJungleChance
								if randNum(1000, "Jungle") < iChance:
									setFeature(Feature["Jungle"], 0)

				if (iImprovement == Improvement["Swamp"]) and (iTerrain != Terrain["Marsh"]) :
					setImprov(-1)

		#	if iAreThereMalakimHere and bIsOwned:
		#		if eCiv == Civ["Malakim"]:
		#			if iTerrain == Terrain["Desert"]:
		#				if pPlot.getRouteType() == Improvement["Road"]:
		#					if randNum(100, "Sand Storm") < 20:
		#						pPlot.setRouteType(-1)

		#### Haunted Lands Section
			if iAreThereScionsHere:
				if bIsOwned:
					if iGameTurn % iGameSpeedMod == 0:	iCreeperSpawn = 1
					else:	iCreeperSpawn = 0
					getUnitClassCount = pPlayer.getUnitClassCount
					if iPlotEffect == Feature["Haunted Lands"]:
						if iCreeperSpawn == 1:
							if eCiv == Civ["Scions"]:
								iNatureMana = pPlayer.getNumAvailableBonuses(Mana["Nature"])
								iManaMod = 1 + (iNatureMana * 0.25)
								#iCreeperCount 		= getUnitClassCount(UnitClass["Creeper"])
								iGhostwalkerFactor 	= getUnitClassCount(UnitClass["Ranger"])
								iHauntFactor 		= getUnitClassCount(UnitClass["Haunt"]) * 2
								iRedactorFactor 	= getUnitClassCount(UnitClass["Druid"]) * 4
								#iCreeperLimit = (iGhostwalkerFactor + iHauntFactor + iRedactorFactor) * iManaMod * 4
								#iCreeperSpawnChance = 8 + (iCreeperLimit * 0.04)
								#if iCreeperCount <= (10 +iCreeperLimit):
								#	if randNum(150, "HL creeper spawn chance") <= iCreeperSpawnChance:
								#		iX = pPlot.getX(); iY = pPlot.getY()
								#		newUnit = pPlayer.initUnit(self.Units["Scions"]["Reaching Creeper"], iX, iY, UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
								#		interface.addMessage(iOwner,True,25,CyTranslator().getText("A Reaching Creeper has grown in the Haunted Lands.", ()),'',1,'Art/Interface/Buttons/Units/Scions/creeper.dds',ColorTypes(8),iX, iY,True,True)

					if eCiv == Civ["Scions"]:
						if iPlotEffect != Feature["Haunted Lands"]:
							iNatureMana = pPlayer.getNumAvailableBonuses(Mana["Nature"])
							iManaMod = 1 + (iNatureMana * 0.1)
							iGhostwalkerFactor 	= getUnitClassCount(UnitClass["Ranger"])
							iHauntFactor 		= getUnitClassCount(UnitClass["Haunt"]) * 2
							iRedactorFactor 	= getUnitClassCount(UnitClass["Druid"]) * 4
							iBlackLadyFactor 	= getUnitClassCount(UnitClass["Korrina Black"]) + 1
							iHLSeed = (iGhostwalkerFactor + iHauntFactor + iRedactorFactor) * iBlackLadyFactor * iManaMod

							if iHLSeed > 0:
								if iFeature == Feature["Forest"] or iFeature == Feature["Ancient Forest"] or iFeature == Feature["Jungle"]:
									iChance = iHLSeed * 1.5
									if randNum(1100, "Chance for HL in wooded tile") < iChance :
										pPlot.setPlotEffectType(Feature["Haunted Lands"])
								if iFeature == Feature["Flood Plains"] or iTerrain == Terrain["Grass"] or iTerrain == Terrain["Plains"] or iTerrain == Terrain["Marsh"] and not bPeak and not bCity:
									iChance = iHLSeed
									if randNum(1100, "Chance for HL in unwooded tile") < iChance :
										pPlot.setPlotEffectType(Feature["Haunted Lands"])
								if iTerrain == Terrain["Desert"] and not bPeak and not bCity:
									iChance = iHLSeed * 0.5
									if randNum(1100, "Chance for HL in desert tile") < iChance :
										pPlot.setPlotEffectType(Feature["Haunted Lands"])

		#### Hell Terrain Section
			if not Option["No Plot Counter"]:
                                gc          = CyGlobalContext()
                                getInfoType = gc.getInfoTypeForString
				iPlotCount = pPlot.getPlotCounter()
				setBonus = pPlot.setBonusType
				if iPlotCount > 9 and not (iTerrain ==Terrain["Tundra"] or iTerrain == Terrain["Taiga"]):
					if (iBonus == Bonus["Sheep"] or iBonus == Bonus["Pig"] or iBonus == Bonus["Fur"] or iBonus == Bonus["Deer"]):
						setBonus(Bonus["Toad"])
					elif (iBonus == Bonus["Horse"] or iBonus == Bonus["Cow"] or iBonus == Bonus["Camel"] or iBonus == Bonus["Ivory"]):
						setBonus(Bonus["Nightmare"])
					elif (iBonus == Bonus["Cotton"] or iBonus == Bonus["Silk"] or iBonus == Bonus["Wine"]):
						setBonus(Bonus["Razorweed"])
					elif (iBonus == Bonus["Fish"] or iBonus == Bonus["Shrimp"] or iBonus == getInfoType('BONUS_WHALE')):
						setBonus(getInfoType('BONUS_JETEYE'))
					elif (iBonus == Bonus["Clam"] or iBonus == Bonus["Crab"] or iBonus == Bonus["Pearl"]):
						setBonus(getInfoType('BONUS_BLEEDING_GOD_WINE'))
					elif (iBonus == Bonus["Banana"] or iBonus == Bonus["Sugar"]):
						setBonus(Bonus["Gulagarm"])
					elif (iBonus == Bonus["Marble"]): setBonus(Bonus["Sheut"])
					elif (iBonus == Bonus["Corn"] or iBonus == Bonus["Rice"] or iBonus == Bonus["Wheat"]):
						setBonus(-1)
						setImprov(Improvement["Snake Pillar"])
				if iPlotCount < 10:
					if iImprovement == Improvement["Snake Pillar"]:
						setImprov(-1)
						iCount = randNum(100, "Hell Convert")
						if  iCount < 33: setBonus(Bonus["Corn"])
						else:
							if iCount < 66: setBonus(Bonus["Rice"])
							else: setBonus(Bonus["Wheat"])
				if iTerrain == Terrain["Burning sands"]:
					if not bCity and not bPeak:
						if randNum(100, "Flames") <= Define["Flame Spread"]:
							setFeature(Feature["Flames"], 0)


	def doTurnKhazad(self, iPlayer):
		gc          = CyGlobalContext()
		pPlayer     = gc.getPlayer(iPlayer)
		iNumCities  = pPlayer.getNumCities()
		Building    = self.Buildings
		Status      = self.LeaderStatus
		Leader      = self.Leaders
		if iNumCities > 0:
			iGold = pPlayer.getGold() / iNumCities
			if    iGold <= 49:                      iNewVault = Building["Vault1"]
			elif (iGold >= 50 and iGold <= 99):     iNewVault = Building["Vault2"]
			elif (iGold >= 100 and iGold <= 149):   iNewVault = Building["Vault3"]
			elif (iGold >= 150 and iGold <= 199):   iNewVault = Building["Vault4"]
			elif (iGold >= 200 and iGold <= 299):   iNewVault = Building["Vault5"]
			elif (iGold >= 300 and iGold <= 499):   iNewVault = Building["Vault6"]
			elif iGold >= 500:                      iNewVault = Building["Vault7"]

			for pyCity in PyPlayer(iPlayer).getCityList():
				pCity = pyCity.GetCy()
				setNumB = pCity.setNumRealBuilding
				setNumB(Building["Vault1"], 0)
				setNumB(Building["Vault2"], 0)
				setNumB(Building["Vault3"], 0)
				setNumB(Building["Vault4"], 0)
				setNumB(Building["Vault5"], 0)
				setNumB(Building["Vault6"], 0)
				setNumB(Building["Vault7"], 0)
				setNumB(iNewVault, 1)

	def doTurnLuchuirp(self, iPlayer):
		gc          = CyGlobalContext()
		pPlayer     = gc.getPlayer(iPlayer)
		iBarnaxus   = self.Heroes["Class-Barnaxus"]
		Leader      = self.Leaders
		if pPlayer.getUnitClassCount(iBarnaxus) > 0:
			Promo       = self.Promotions["Generic"]
			iGolem      = self.Promotions["Race"]["Golem"]
			py          = PyPlayer(iPlayer)
			pBarnaxus   = -1
			bEmp1       = False
			bEmp2       = False
			bEmp3       = False
			bEmp4       = False
			bEmp5       = False

			lGolems = []
			append = lGolems.append
			for pUnit in py.getUnitList():
				if pUnit.getUnitClassType() == iBarnaxus:
					pBarnaxus = pUnit
				elif pUnit.isHasPromotion(iGolem) :
					append(pUnit)
			if pBarnaxus != -1 :
				isHas = pBarnaxus.isHasPromotion
				bEmp1 = bool(isHas(Promo["Combat I"]))
				bEmp2 = bool(isHas(Promo["Combat II"]))
				bEmp3 = bool(isHas(Promo["Combat III"]))
				bEmp4 = bool(isHas(Promo["Combat IV"]))
				bEmp5 = bool(isHas(Promo["Combat V"]))
			for pUnit in lGolems :
				setPromo = pUnit.setHasPromotion
				setPromo(Promo["Empower I"], False)
				setPromo(Promo["Empower II"], False)
				setPromo(Promo["Empower III"], False)
				setPromo(Promo["Empower IV"], False)
				setPromo(Promo["Empower V"], False)
				if bEmp1:
					setPromo(Promo["Empower I"], True)
				if bEmp2:
					setPromo(Promo["Empower II"], True)
				if bEmp3:
					setPromo(Promo["Empower III"], True)
				if bEmp4:
					setPromo(Promo["Empower IV"], True)
				if bEmp5:
					setPromo(Promo["Empower V"], True)

	def doTurnArchos(self, iPlayer):
		gc              = CyGlobalContext()
		pPlayer         = gc.getPlayer(iPlayer)
		if pPlayer.getNumCities() > 0:
			pNest       = pPlayer.getCapitalCity()
			iNestPop    = pNest.getPopulation()
			if iNestPop < 4:
				return
			Unit        = self.Units["Archos"]
			Promo       = self.Promotions["Effects"]
			Building    = self.Buildings
			Trait       = self.Traits
			iNoAI       = UnitAITypes.NO_UNITAI
			iSouth      = DirectionTypes.DIRECTION_SOUTH
			getNum      = pNest.getNumBuilding
			iX = pNest.getX(); iY = pNest.getY()
			self.doChanceArchos(iPlayer)
			iSpawnChance = pPlayer.getCivCounter()
			iScorpionSpawnChance = pPlayer.getCivCounterMod()

			if CyGame().getSorenRandNum(10000, "Spawn Roll") < iSpawnChance:
				if iNestPop >= 12:
					spawnUnit = pPlayer.initUnit( Unit["Giant Spider"], iX, iY, iNoAI, iSouth)
				elif iNestPop >= 8:
					spawnUnit = pPlayer.initUnit( Unit["Spider"], iX, iY, iNoAI, iSouth)
				elif iNestPop >= 4:
			 		spawnUnit = pPlayer.initUnit( Unit["Baby Spider"], iX, iY, iNoAI, iSouth)

				setPromo = spawnUnit.setHasPromotion
				if pPlayer.hasTrait(Trait["Spiderkin"]):
					if iNestPop >= 10:
						setPromo( Promo["Spiderkin"], True)

				if iNestPop >= 16:
					setPromo( Promo["Strong"], True)

				CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_MESSAGE_SPIDER_BORN_IN_NEST",()),'AS2D_DISCOVERBONUS',1,'Art/Interface/Buttons/Units/Giant Spider.dds',gc.getInfoTypeForString("COLOR_GREEN"),iX,iY,True,True)
				pNest.applyBuildEffects(spawnUnit)

				if   getNum( Building["Nest Addon1"]) > 0:
					iBroodStrength = 1
				elif getNum( Building["Nest Addon2"]) > 0:
					iBroodStrength = 2
				elif getNum( Building["Nest Addon3"]) > 0:
					iBroodStrength = 3
				elif getNum( Building["Nest Addon4"]) > 0:
					iBroodStrength = 4
				else:
					iBroodStrength = 0
				spawnUnit.changeFreePromotionPick(iBroodStrength)

			elif CyGame().getSorenRandNum(10000, "Spawn Roll") < iScorpionSpawnChance:

				if iNestPop >= 12:
					spawnUnit = pPlayer.initUnit( Unit["Giant Scorpion"], iX, iY, iNoAI, iSouth)
				elif iNestPop >= 8:
					spawnUnit = pPlayer.initUnit( Unit["Scorpion Swarm"], iX, iY, iNoAI, iSouth)
				elif iNestPop >= 4:
			 		spawnUnit = pPlayer.initUnit( Unit["Scorpion"], iX, iY, iNoAI, iSouth)

				setPromo = spawnUnit.setHasPromotion
				if pPlayer.hasTrait(Trait["Spiderkin"]):
					if iNestPop >= 10:
						setPromo( Promo["Spiderkin"], True)

				if iNestPop >= 16:
					setPromo( Promo["Strong"], True)

				CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_MESSAGE_SCORPION_BORN_IN_NEST",()),'AS2D_DISCOVERBONUS',1,'Art/Interface/Buttons/Units/Scorpion.dds',gc.getInfoTypeForString("COLOR_GREEN"),iX,iY,True,True)
				pNest.applyBuildEffects(spawnUnit)

				if   getNum( Building["Nest Addon1"]) > 0:
					iBroodStrength = 1
				elif getNum( Building["Nest Addon2"]) > 0:
					iBroodStrength = 2
				elif getNum( Building["Nest Addon3"]) > 0:
					iBroodStrength = 3
				elif getNum( Building["Nest Addon4"]) > 0:
					iBroodStrength = 4
				else:
					iBroodStrength = 0
				spawnUnit.changeFreePromotionPick(iBroodStrength)

	def doChanceArchos(self, iPlayer):
		gc = CyGlobalContext()
		if iPlayer == -1:
			pPlayer = gc.getPlayer(gc.getGame().getActivePlayer())
		else:
			pPlayer = gc.getPlayer(iPlayer)

		iNumCities = pPlayer.getNumCities()
		if iNumCities > 0:
			UnitClass		= self.UnitClasses
			Building		= self.Buildings
			Trait			= self.Traits
			Terrain				= self.Terrain
			pNest 			= pPlayer.getCapitalCity()
			iNestPop 		= pNest.getPopulation()
			iX 				= pNest.getX()
			iY 				= pNest.getY()
			iNumNestHills 	= 0
			iNumGroves 		= pPlayer.countNumBuildings(Building["Dark Weald"])
			getUCC			= pPlayer.getUnitClassCount
			getPlot			= CyMap().plot
			iNumSpiders		= ((getUCC(UnitClass["Baby Spider"]) * 0.5) + getUCC(UnitClass["Spider"]) + (getUCC(UnitClass["Giant Spider"]) * 2))
			iNumScorpions 	= ((getUCC(UnitClass["Scorpion"]) * 0.5) + getUCC(UnitClass["Scorpion Swarm"]) + (getUCC(UnitClass["Giant Scorpion"]) * 2))

			iNumScorpionCities = 0
			iNumSpiderCities = 0
			for pyCity in PyPlayer(iPlayer).getCityList():
				pCity = pyCity.GetCy()
				pCityPlot = pCity.plot()
				if pCityPlot.getTerrainType() == Terrain["Desert"]:
					iNumScorpionCities += 1
				else:
					iNumSpiderCities += 1

			for iiX,iiY in BFC:
				pLoopPlot = getPlot(iX+iiX,iY+iiY)
				if not pLoopPlot.isNone():
					if pLoopPlot.isHills():
						iNumNestHills += 1

			fSpiderkin = 1
			if pPlayer.hasTrait(Trait["Spiderkin"]):
				fSpiderkin = 1.30

			iSpiderSpawnChance = ((iNestPop + (iNumSpiderCities*2) + (iNumGroves*4)) * fSpiderkin) - iNumSpiders
			iSpiderSpawnChance = (iSpiderSpawnChance * 100)
			iSpiderSpawnChance = scaleInverse(iSpiderSpawnChance)

			pPlayer.setCivCounter(iSpiderSpawnChance)

			iScorpionSpawnChance = (iNestPop + (iNumScorpionCities*2) + (iNumNestHills)) - iNumScorpions
			iScorpionSpawnChance = (iScorpionSpawnChance * 100)
			iScorpionSpawnChance = scaleInverse(iScorpionSpawnChance)

			pPlayer.setCivCounterMod(iScorpionSpawnChance)
# SCIONS START - Awakened spawning

	def doTurnScions(self, iPlayer):
		gc 			= CyGlobalContext()
		pPlayer 	= gc.getPlayer(iPlayer)
		py 			= PyPlayer(iPlayer)
		pTomb 		= pPlayer.getCapitalCity()
		self.doChanceAwakenedSpawn(iPlayer)
		iSpawnOdds = pPlayer.getCivCounter()

		Manager		= CvEventInterface.getEventManager()
		Building	= Manager.Buildings
		Unit		= Manager.Units["Scions"]
		Civic 		= Manager.Civics
		Mana		= Manager.Mana
		Bonus		= Manager.Resources
		numBoni		= pPlayer.getNumAvailableBonuses
		isCivic 	= pPlayer.isCivic
		iTeam		= pPlayer.getTeam()
		getTeam		= gc.getTeam
		iMaxPlayers = gc.getMAX_CIV_PLAYERS()

		if CyGame().getSorenRandNum(10000, "Spawn Roll") < iSpawnOdds:
			spawnUnit = pPlayer.initUnit(Unit["Awakened"], pTomb.getX(), pTomb.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)

		pTomb = pPlayer.getCapitalCity()
		iTombPop = pTomb.getPopulation()
		iNumCities = pPlayer.getNumCities()
		if iNumCities == 1:
			if iTombPop == 1:
				pTomb.changePopulation(1)

		if iNumCities > 0:
			for pyCity in PyPlayer(iPlayer).getCityList():
				pCity 		= pyCity.GetCy()
				setBuilding = pCity.setNumRealBuilding
				eProdUnit 	= pCity.getProductionUnit()
				getNumB		= pCity.getNumRealBuilding
				if eProdUnit != Unit["Reborn"]:
					setBuilding(Building["Obbuilding1"], 0)
					setBuilding(Building["Obbuilding2"], 0)
					setBuilding(Building["Obbuilding3"], 0)
					setBuilding(Building["Obbuilding4"], 0)
					setBuilding(Building["Obbuilding5"], 0)
					setBuilding(Building["Obbuilding6"], 0)
					setBuilding(Building["Obbuilding7"], 0)

				if eProdUnit == Unit["Reborn"]:
					pTeam = getTeam(iTeam)
					iNumOpenBorders = (pTeam.getOpenBordersTradingCount()) - 1
					for iLoopCiv in xrange(iMaxPlayers):
						if (pTeam.isOpenBorders(iLoopCiv)):
							iNumOpenBorders = iNumOpenBorders + 1

					if isCivic(Civic["God King"]):
						iNumOpenBorders = iNumOpenBorders + 1

					if   iNumOpenBorders == 1:
						iCurrentBuilding = Building["Obbuilding1"]
					elif iNumOpenBorders == 2:
						iCurrentBuilding = Building["Obbuilding2"]
					elif iNumOpenBorders == 3:
						iCurrentBuilding = Building["Obbuilding3"]
					elif iNumOpenBorders == 4:
						iCurrentBuilding = Building["Obbuilding4"]
					elif iNumOpenBorders == 5:
						iCurrentBuilding = Building["Obbuilding5"]
					elif iNumOpenBorders == 6:
						iCurrentBuilding = Building["Obbuilding6"]
					elif iNumOpenBorders > 6:
						iCurrentBuilding = Building["Obbuilding7"]
					setBuilding(Building["Obbuilding1"], 0)
					setBuilding(Building["Obbuilding2"], 0)
					setBuilding(Building["Obbuilding3"], 0)
					setBuilding(Building["Obbuilding4"], 0)
					setBuilding(Building["Obbuilding5"], 0)
					setBuilding(Building["Obbuilding6"], 0)
					setBuilding(Building["Obbuilding7"], 0)
					if iNumOpenBorders > 0:
						setBuilding(iCurrentBuilding, 1)

				if getNumB(Building["Necropolis"]) > 0:
					iNumHealthBonus = 0
					if numBoni(Mana["Life"]) > 0: 		iNumHealthBonus += 1
					if numBoni(Bonus["Toad"]) > 0: 		iNumHealthBonus += 1
					if numBoni(Bonus["Banana"]) > 0:	iNumHealthBonus += 1
					if numBoni(Bonus["Wheat"]) > 0:		iNumHealthBonus += 1
					if numBoni(Bonus["Corn"]) > 0:		iNumHealthBonus += 1
					if numBoni(Bonus["Rice"]) > 0:		iNumHealthBonus += 1
					if numBoni(Bonus["Crab"]) > 0:		iNumHealthBonus += 1
					if numBoni(Bonus["Clam"]) > 0:		iNumHealthBonus += 1
					if numBoni(Bonus["Fish"]) > 0:		iNumHealthBonus += 1
					if numBoni(Bonus["Deer"]) > 0:		iNumHealthBonus += 1
					if numBoni(Bonus["Arctic Deer"]) > 0:iNumHealthBonus += 1
					if numBoni(Bonus["Pig"]) > 0:		iNumHealthBonus += 1
					if numBoni(Bonus["Cow"]) > 0:		iNumHealthBonus += 1
					if numBoni(Bonus["Sheep"]) > 0:		iNumHealthBonus += 1
					iBonusBuilding1 = Building["Necro Bonus1"]
					iBonusBuilding2 = Building["Necro Bonus2"]
					iBonusBuilding3 = Building["Necro Bonus3"]
					if iNumHealthBonus < 4:
						iNecroBonusBuilding = 0
					if (iNumHealthBonus >= 4 and iNumHealthBonus <= 5):
						iNecroBonusBuilding = Building["Necro Bonus1"]
					if (iNumHealthBonus >= 6 and iNumHealthBonus <= 8):
						iNecroBonusBuilding = Building["Necro Bonus2"]
					if iNumHealthBonus > 8:
						iNecroBonusBuilding = Building["Necro Bonus3"]
					setBuilding(Building["Necro Bonus1"], 0)
					setBuilding(Building["Necro Bonus2"], 0)
					setBuilding(Building["Necro Bonus3"], 0)
					if iNecroBonusBuilding != 0:
						setBuilding(iNecroBonusBuilding, 1)
					setBuilding(Building["Unhealthy Discontent I"], 0)
					setBuilding(Building["Unhealthy Discontent II"], 0)

				if getNumB(Building["Necropolis"]) == 0:
					iUH = 0
					iUH = pCity.badHealth(False) - pCity.goodHealth()

					if (iUH >= 5 and iUH <= 7):
						setBuilding(Building["Unhealthy Discontent I"], 1)
						setBuilding(Building["Unhealthy Discontent II"], 0)
					if iUH >= 8:
						setBuilding(Building["Unhealthy Discontent II"], 1)
						setBuilding(Building["Unhealthy Discontent I"], 0)
					if iUH < 4:
						setBuilding(Building["Unhealthy Discontent I"], 0)
						setBuilding(Building["Unhealthy Discontent II"], 0)

		Promo 	= self.Promotions["Effects"]
		Hero 	= self.Heroes
		for pUnit in py.getUnitList():
			iUnitType = pUnit.getUnitType()
			if  iUnitType == Hero["Alcinus"] or iUnitType == Hero["Alcinus (Archmage)"] or iUnitType == Hero["Alcinus (Upgraded)"]:
				if pUnit.isHasPromotion(Promo["Rampage"]):
					pUnit.setUnitAIType( UnitAITypes.UNITAI_ATTACK )
				else:
					pUnit.setUnitAIType( UnitAITypes.UNITAI_RESERVE )

		pPlayer.setFeatAccomplished(FeatTypes.FEAT_MANIFEST_HORNED_DREAD, True)

		getUnitCount = pPlayer.getUnitClassCount
		iEligibleNum = (getUnitCount(self.UnitClasses["Ranger"]) + getUnitCount(self.UnitClasses["Haunt"]))

		if iEligibleNum < 1:
			pPlayer.setFeatAccomplished(FeatTypes.FEAT_MANIFEST_FIRST_HORNED_DREAD, False)

	def doChanceAwakenedSpawn(self, iPlayer):
		gc 			= CyGlobalContext()
		getPlayer 	= gc.getPlayer
		game		= CyGame()
		Civ			= self.Civilizations

		if iPlayer == -1:
			pPlayer = getPlayer(game.getActivePlayer())
		else:
			pPlayer = getPlayer(iPlayer)

		if pPlayer.getNumCities() > 0 and pPlayer.getCivilizationType() == Civ["Scions"]:
			Resource 	= self.Resources
			Building	= self.Buildings
			Civic		= self.Civics
			Unit		= self.Units
			Bonus		= self.Resources
			iSpawnChance= 0
			pTomb 		= pPlayer.getCapitalCity()
			iTombPop 	= pTomb.getPopulation()
			iTPop 		= pPlayer.getTotalPopulation()

			iTotalLand 	= int(CyMap().getLandPlots())
			iTPopLmt 	= (iTotalLand / 40) + 6

			numB		= pPlayer.countNumBuildings
			iNumTemples = numB(Building["Temple of the Gift"])
			iNumHalls 	= numB(Building["Hall of the Covenant"])
			iNumCen 	= numB(Building["Imperial Cenotaph"])
			iNumShrine 	= numB(Building["Shrine to Kylorin"])

# Different buildings give different modifiers.  Aristocracy gives a +66% mod.  (
			iShrineMod 	= iNumShrine * 0.5
			iCenMod 	= iNumCen
			iHallsMod 	= iNumHalls * 2
			iTemplesMod = iNumTemples
			iCapMod 	= iTombPop * 0.25
			iTPopLmtMod = iTPopLmt * 0.025
			iCivicMod 	= 0.00
			iCivicMult 	= 1
			if pPlayer.isCivic(Civic["Aristocracy"]):
				iCivicMod = 4
				iCivicMult = 1.25

# Makes it very unlikely the Scions are screwed by missing "rolls" in the very early game.
			iPopLowMod = 1
			if iTPop <= 5:
				iPopLowMod = 1.5

			iSilkMod 	= 0
			iGoldMod 	= 0
			iGemsMod 	= 0
			iIvoryMod 	= 0
			iDyeMod 	= 0
			iCottonMod 	= 0
			iFurMod 	= 0
			iIncenseMod = 0
			iPearlMod 	= 0

			numAvailable = pPlayer.getNumAvailableBonuses
			if numAvailable( Bonus["Pearl"]	) > 0: 	iPearlMod 	= 0.5
			if numAvailable( Bonus["Incense"]) > 0: iIncenseMod = 0.5
			if numAvailable( Bonus["Fur"]	) > 0: 	iFurMod 	= 0.5
			if numAvailable( Bonus["Silk"]	) > 0: 	iSilkMod 	= 1
			if numAvailable( Bonus["Gold"]	) > 0: 	iGoldMod 	= 0.5
			if numAvailable( Bonus["Gems"]	) > 0: 	iGemsMod 	= 1
			if numAvailable( Bonus["Ivory"]	) > 0:	iIvoryMod 	= 0.5
			if numAvailable( Bonus["Dye"]	) > 0: 	iDyeMod 	= 1
			if numAvailable( Bonus["Cotton"]) > 0:	iCottonMod 	= 0.5

			iLuxuryMod = iSilkMod + iGoldMod + iGemsMod + iIvoryMod + iDyeMod + iCottonMod + iFurMod + iIncenseMod

			if iLuxuryMod >= 4.5:
				iLuxuryMod = iLuxuryMod * 2

# Raise ASpnd to power (between 1 and 2) instead?
# Modifies the rate based on gamespeed.
			iSpeedMod = 1
			game = CyGame()
			estiEnd = game.getEstimateEndTurn()
			if ( estiEnd >= 1500 ):	iSpeedMod = 2.0
			elif ( estiEnd >= 750 ):iSpeedMod = 1.5
			elif ( estiEnd >= 500 ):iSpeedMod = 1.0
			elif ( estiEnd >= 330 ):iSpeedMod = 0.67
			else:					iSpeedMod = iSpeedMod

#Decaymod
			iTurnMod = 1
			estiEnd = game.getEstimateEndTurn()
			if ( estiEnd >= 1500 ):	iTurnMod = 0.106
			elif ( estiEnd >= 750 ):iTurnMod = 0.14
			elif ( estiEnd >= 500 ):iTurnMod = 0.213
			elif ( estiEnd >= 330 ):iTurnMod = 0.425
			else:					iTurnMod = iTurnMod

# The Body-mana bonus given by the Flesh Studio.
			iNumCorpusB = numB(Building["Flesh Studio"])
			if iNumCorpusB > 0: iBodyMana = numAvailable( Bonus["Mana"])
			else: iBodyMana = 0

			iBodyManaMod = iBodyMana * 3

# Difficulty level modifier
			iHumanmod = 1
			iAImod = 1
			bHuman = pPlayer.isHuman()
			if bHuman:
				iDifficulty = gc.getNumHandicapInfos() + 1 - int(game.getHandicapType())
				iHumanmod = .5 + (iDifficulty * 0.1)
				iAImod = 1

			if not bHuman:
				iDifficulty = gc.getNumHandicapInfos() + 1 - int(game.getHandicapType())
				iAImod = .7 + (iDifficulty * 0.1)
				iHumanmod = 1

# Per/resource Patrian Artifact modifier
			iPA = numAvailable(Bonus["Patrian"])
			iPAMod = iPA * 0.75
# Slight reduction in odds each turn.
			iGTurn = game.getGameTurn()
			iDecayMod = 111 - (iGTurn * iTurnMod)
			if (iDecayMod < 1):
				iDecayMod = 1
# default adjustments of gamespeed training are M =2, E = 1.5, N = 1, Q = .67
			iSpawnChance = round(((iCapMod + iBodyManaMod + iTemplesMod + iCenMod + iHallsMod + iShrineMod + iPAMod + iLuxuryMod + iPearlMod + iTPopLmtMod + iCivicMod + 3) * iCivicMult * iHumanmod / iSpeedMod * iDecayMod * 0.01 * iPopLowMod / iAImod),2)

			if iTPop >= iTPopLmt:
				iSpawnChance = 0

			if pPlayer.getDisableProduction() > 0:
				iSpawnChance = 0

			iSpawnChance = int(iSpawnChance * 100)
			pPlayer.setCivCounter(iSpawnChance)

# Start Grigori Adventurer spawning

	def doTurnGrigori(self, iPlayer):
		gc = CyGlobalContext()
		pPlayer = gc.getPlayer(iPlayer)
		pCapital = pPlayer.getCapitalCity()

		self.doChanceAdventurerSpawn(iPlayer)

		iGrigoriSpawn 	= pPlayer.getCivCounter()
		iGrigoriMod 	= pPlayer.getCivCounterMod()

		if iGrigoriMod < 10000:
			pPlayer.setCivCounterMod(10000)
			iGrigoriMod = 10000

		if iGrigoriSpawn >= iGrigoriMod:
			spawnUnit = pPlayer.initUnit(self.Units["Grigori"]["Adventurer"], pCapital.getX(), pCapital.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
			pPlayer.changeCivCounter(0 - iGrigoriMod)
			pPlayer.changeCivCounterMod(2000)

	def doChanceAdventurerSpawn(self, iPlayer):
		gc 			= CyGlobalContext()
		if iPlayer == -1:
			pPlayer = gc.getPlayer(CyGame().getActivePlayer())
		else:
			pPlayer = gc.getPlayer(iPlayer)

		iGrigoriSpawn = (pPlayer.getCivCounter() / 100)

		Civ			= self.Civilizations

		if pPlayer.getNumCities() > 0 and pPlayer.getCivilizationType() == Civ["Grigori"]:
			Civic			= self.Civics
			Building		= self.Buildings
			Specialist		= self.Specialists
			countB 			= pPlayer.countNumBuildings
			iNumMuseums 	= countB(Building["Museum"])
			iNumTaverns 	= countB(Building["Tavern"])
			iNumGuilds 		= countB(Building["Adventurers Guild"])
			iNumPalace 		= countB(Building["Grigori Palace"])
			iNumDagda 		= countB(Building["Grigori Temple"])
			iNumRefuge 		= countB(Building["Dwelling of Refuge"])
			iNumBountyMinor		= countB(Building["Crime Bounty Minor"])
			iNumBountyMajor 	= countB(Building["Crime Bounty Major"])
			iNumBountyGrand 	= countB(Building["Crime Bounty Grand"])

# Different buildings give different modifiers.  Aristocracy gives a +66% mod.
			iPalaceMod 		= iNumPalace
			iDagdaMod 		= iNumDagda * 0.33
			iMuseumMod 		= iNumMuseums * 0.33
			iTavernsMod 	= iNumTaverns
			iGuildsMod 		= iNumGuilds * 0.5
			iRefugeMod 		= iNumRefuge
			iCivicMod 		= 0.00
			iCivicMult 		= 1
			if pPlayer.isCivic(Civic["Apprenticeship"]):
				iCivicMod = 2
				iCivicMult = 1.25

# Allows Statesmen to take affect after their city has an Assembly
			iStatesman 		= Specialist["Statesman"]
			iAssembly 		= Building["Forum"]
			iStatesmanMod 	= 0.00
			iNumStatesmen 	= 0
			for pyCity in PyPlayer(iPlayer).getCityList():
				pCity = pyCity.GetCy()
				if pCity.getNumBuilding(iAssembly) > 0:
					iNumStatesmen = (pCity.getSpecialistCount(iStatesman) + pCity.getFreeSpecialistCount(iStatesman))
					if iNumStatesmen > 0:
						iStatesmanMod += (iNumStatesmen * 0.33)
# AI modifier
			iAImod = 1
			if not pPlayer.isHuman():
				iAImod = 1.5

			iGrigoriSpawn = round(((iPalaceMod + iDagdaMod + iMuseumMod +iTavernsMod + iGuildsMod + iNumBountyMinor + iNumBountyMajor + iNumBountyGrand + iCivicMod + iStatesmanMod + iRefugeMod) * iCivicMult * iAImod), 2)
			iGrigoriSpawn = int(iGrigoriSpawn * 100)
			iGrigoriSpawn = scaleInverse(iGrigoriSpawn)

			pPlayer.changeCivCounter(iGrigoriSpawn)
# End Grigori
# Start Mekara
	def doTurnMekara(self, iPlayer):
		gc = CyGlobalContext()
		pPlayer = gc.getPlayer(iPlayer)
		pCapital = pPlayer.getCapitalCity()

		self.doChanceAspirantSpawn(iPlayer)

		iMekaraSpawn 	= pPlayer.getCivCounter()
		iMekaraMod 	= pPlayer.getCivCounterMod()

		if iMekaraMod < 10000:
			pPlayer.setCivCounterMod(10000)
			iMekaraMod = 10000

		if iMekaraSpawn >= iMekaraMod:
			spawnUnit = pPlayer.initUnit(self.Units["Mekara Order"]["Aspirant"], pCapital.getX(), pCapital.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
			pPlayer.changeCivCounter(0 - iMekaraMod)
			pPlayer.changeCivCounterMod(2000)

	def doChanceAspirantSpawn(self, iPlayer):
		gc 			= CyGlobalContext()
		if iPlayer == -1:
			pPlayer = gc.getPlayer(CyGame().getActivePlayer())
		else:
			pPlayer = gc.getPlayer(iPlayer)

		iMekaraSpawn = (pPlayer.getCivCounter() / 100)

		Civ			= self.Civilizations

		if pPlayer.getNumCities() > 0 and pPlayer.getCivilizationType() == gc.getInfoTypeForString('CIVILIZATION_MEKARA_V2') and pPlayer.getLeaderType() == gc.getInfoTypeForString('LEADER_IRAM'):
			Civic			= self.Civics
			Building		= self.Buildings
			Specialist		= self.Specialists
			countB 			= pPlayer.countNumBuildings
			iNumLabs 	= countB(Building["Shaper's Laboratory"])
			iNumCabals 	= countB(Building["Shaper Cabal"])
			iNumPalace 	= countB(Building["Mekaran Palace"])

# +2 for each Lab, +1 for each Mage Guild, +2/+25% with Slavery, +3 from Humanist I, +5/+25% from Humanist II, +10/+50% from Humanist III
			iLabMod 		= iNumCabals*2
			iCabalMod 		= iNumLabs
			iPalaceMod 		= iNumPalace*0
			iCivicMod 		= 0.00
			iCivicMult 		= 1
			if pPlayer.isCivic(Civic["Slavery"]):
				iCivicMod = 2
				iCivicMult = 1.25
			iTraitMod		= 0.00
			iTraitMult		= 1
			if pPlayer.hasTrait(gc.getInfoTypeForString('TRAIT_HUMANIST1')):
				iTraitMod		= 3
				iTraitMult		= 1
#			if pPlayer.hasTrait(gc.getInfoTypeForString('TRAIT_HUMANIST2')):
#				iTraitMod		= 5
#				iTraitMult		= 1.25
#			if pPlayer.hasTrait(gc.getInfoTypeForString('TRAIT_HUMANIST3')):
#				iTraitMod		= 10
#				iTraitMult		= 1.5

# Allows Healers (+1 for every 2) and Elders (+1 for every 3) to take effect with Labs.
			iHealer 		= Specialist["Healer"]
			iElder 			= Specialist["Scientist"]
			iGreatHealer 	= Specialist["Great Healer"]
			iGreatElder 	= Specialist["Great Scientist"]
			iLab			= Building["Shaper's Laboratory"]

			iSpecialistMod 	= 0.00
			iNumHealers 	= 0
			iNumGreatHealers 	= 0
			iNumElders		= 0
			iNumGreatElders		= 0
			for pyCity in PyPlayer(iPlayer).getCityList():
				pCity = pyCity.GetCy()
				if pCity.getNumBuilding(iLab) > 0:
					iNumElders 				= (pCity.getSpecialistCount(iElder) + pCity.getFreeSpecialistCount(iElder))
					iNumHealers 			= (pCity.getSpecialistCount(iHealer) + pCity.getFreeSpecialistCount(iHealer))
					iNumGreatElders 		= (pCity.getSpecialistCount(iGreatElder) + pCity.getFreeSpecialistCount(iGreatElder))
					iNumGreatHealers 	= (pCity.getSpecialistCount(iGreatHealer) + pCity.getFreeSpecialistCount(iGreatHealer))
					if iNumHealers > 0 or iNumElders > 0 or iNumGreatElders > 0 or iNumGreatHealers > 0:
						iSpecialistMod += (iNumHealers * 0.5) + (iNumElders * 0.5) + (iNumGreatElders * 1) + (iNumGreatHealers * 1)
# AI modifier
			iAImod = 1
			if not pPlayer.isHuman():
				iAImod = 1.5

			iMekaraSpawn = round(((iLabMod + iCabalMod + iPalaceMod + iCivicMod + iTraitMod + iSpecialistMod) * iTraitMult * iCivicMult * iAImod), 2)
			iMekaraSpawn = int(iMekaraSpawn * 100)
			iMekaraSpawn = scaleInverse(iMekaraSpawn)

			pPlayer.changeCivCounter(iMekaraSpawn)
# End Mekara
# Start Doviello
	def doCityTurnDoviello(self, iPlayer, pCity):
		gc          = CyGlobalContext()
		getInfoType = gc.getInfoTypeForString
		pPlayer     = gc.getPlayer(iPlayer)
		pPlot       = pCity.plot()
		iReligion   = pPlayer.getStateReligion()
		randNum     = CyGame().getSorenRandNum

		self.doChanceAnimalSpawn(iPlayer, pCity)
		iAnimalSpawnChance = pCity.getCityCounter()

		if randNum(10000, "Animal Spawn") < iAnimalSpawnChance:
			lList   = self.doAnimalListDoviello(iPlayer)
			sAnimal = lList[randNum(len(lList), "Pick Animal")]
			iUnit   = getInfoType(sAnimal)
			Civic           = self.Civics
			Promo           = self.Promotions["Effects"]
			newUnit         = pPlayer.initUnit(iUnit, pPlot.getX(), pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_NORTH)
			newUnit.setHasPromotion( Promo["Loyalty III"], True)
			CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_ANIMAL_SPAWN",()),'AS2D_DISCOVERBONUS',1,gc.getUnitInfo(newUnit.getUnitType()).getButton(),ColorTypes(8),pCity.getX(),pCity.getY(),True,True)
			if iReligion != -1:
				newUnit.setReligion(iReligion)
			if pPlayer.getCivics(Civic["Membership"]) == Civic["Wild Council"]:
				newUnit.setHasPromotion(Promo["Heroic Strength I"], True)

	def doChanceAnimalSpawn(self, iPlayer, pCity):
		gc = CyGlobalContext()
		if iPlayer == -1:
			pPlayer = gc.getPlayer(CyGame().getActivePlayer())
		else:
			pPlayer = gc.getPlayer(iPlayer)

		pPlot           = pCity.plot()
		iDenPop         =(pCity.getPopulation() - 1) * 0.6
		iSpawnChance    = 8
		iReligion       = pPlayer.getStateReligion()
		Rel             = self.Religions
		Civic           = self.Civics
		if (pCity.getNumRealBuilding(gc.getInfoTypeForString("BUILDING_BEAR_CAVE")))>0:
			iSpawnChance=iSpawnChance+0.5
		if (pCity.getNumRealBuilding(gc.getInfoTypeForString("BUILDING_GORILLA_FOREST")))>0:
			iSpawnChance=iSpawnChance+0.5
		if (pCity.getNumRealBuilding(gc.getInfoTypeForString("BUILDING_LION_SAVANNA")))>0:
			iSpawnChance=iSpawnChance+0.5
		if (pCity.getNumRealBuilding(gc.getInfoTypeForString("BUILDING_TIGER_MANGROVE")))>0:
			iSpawnChance=iSpawnChance+0.5
		if (pCity.getNumRealBuilding(gc.getInfoTypeForString("BUILDING_WOLF_BURROW")))>0:
			iSpawnChance=iSpawnChance+0.5
		if (pCity.getNumRealBuilding(gc.getInfoTypeForString("BUILDING_CRIME_RAVENOUS_PACK")))>0:
			iSpawnChance=iSpawnChance+1
		if (pCity.getNumRealBuilding(gc.getInfoTypeForString("BUILDING_CRIME_RAVENOUS_PACK_ESUS")))>0:
			iSpawnChance=iSpawnChance+1
			
		if pPlayer.isCivic(Civic["Wild Council"]):
			iSpawnChance = iSpawnChance * 2
		if iReligion == Rel["Fellowship"]:
			iSpawnChance = iSpawnChance * 1.5

		iAnimalSpawnChance = iSpawnChance - iDenPop
		iAnimalSpawnChance = (iAnimalSpawnChance * 100)
		iAnimalSpawnChance = scaleInverse(iAnimalSpawnChance)

		pCity.setCityCounter(iAnimalSpawnChance)

	def doAnimalListDoviello(self, iPlayer):
		gc = CyGlobalContext()
		if iPlayer == -1:
			pPlayer = gc.getPlayer(gc.getGame().getActivePlayer())
		else:
			pPlayer = gc.getPlayer(iPlayer)

		pTeam = gc.getTeam(pPlayer.getTeam())

		lList = []
		BuildingClass   = self.BuildingClasses
		Tech            = self.Techs
		hasTech         = pTeam.isHasTech
		getBCC          = pPlayer.getBuildingClassCount

		if   hasTech( Tech["Feral Bond"]):              lList += ['UNIT_DIRE_WOLF']
		elif hasTech( Tech["Tracking"]):                lList += ['UNIT_WOLF_PACK']
		else:                                           lList += ['UNIT_WOLF']

		if getBCC(BuildingClass["Bear Den"]) >= 1:
			if hasTech( Tech["Iron Working"]):          lList += ['UNIT_CAVE_BEARS']
			elif hasTech( Tech["Bronze Working"]):      lList += ['UNIT_BEAR_GROUP']
			else:                                       lList += ['UNIT_BEAR']

		if getBCC(BuildingClass["Boar Pen"]) >= 1:
			if hasTech( Tech["Engineering"]):           lList += ['UNIT_BLOOD_BOAR']
			elif hasTech( Tech["Construction"]):        lList += ['UNIT_BOAR_HERD']
			else:                                       lList += ['UNIT_BOAR']

		if getBCC(BuildingClass["Gorilla Hut"]) >= 1:
			if hasTech( Tech["Bowyers"]):               lList += ['UNIT_SILVERBACK']
			elif hasTech( Tech["Archery"] ):            lList += ['UNIT_GORILLA_TROOP']
			else:                                       lList += ['UNIT_GORILLA']

		if getBCC(BuildingClass["Griffin Weyr"]) >= 1:
			if   hasTech( Tech["Stirrups"]):            lList += ['UNIT_ROC']
			elif hasTech( Tech["Horseback Riding"]):    lList += ['UNIT_GRIFFON']
			else:                                       lList += ['UNIT_HIPPOGRIFF']

		if getBCC(BuildingClass["Stag Copse"]) >= 1:
			if hasTech( Tech["Priesthood"]):            lList += ['UNIT_ELK']
			else:                                       lList += ['UNIT_STAG']

		return lList
# End Doviello

# Start Sheaim
	def doCityTurnPlanarGate(self, iPlayer, pCity):
		gc 					= CyGlobalContext()
		game				= CyGame()
		self.doPlanarGateChance(iPlayer, pCity)
		iPlanarGateChance	= pCity.getCityCounter()
		pPlot 				= pCity.plot()
		pPlayer 			= gc.getPlayer(iPlayer)
		randNum 			= CyGame().getSorenRandNum
		if randNum(10000, "Planar Gate") <= iPlanarGateChance:
			getInfoType = gc.getInfoTypeForString
			listUnits 	= self.doListPlanarGate(iPlayer, pCity)
			if len(listUnits) > 0:
				iUnit = listUnits[randNum(len(listUnits), "Planar Gate")]
				newUnit = pPlayer.initUnit(iUnit, pPlot.getX(), pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_NORTH)
				CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_PLANAR_GATE",()),'AS2D_DISCOVERBONUS',1,gc.getUnitInfo(newUnit.getUnitType()).getButton(),ColorTypes(8),pCity.getX(),pCity.getY(),True,True)
				if iUnit == self.Units["Sheaim"]["Mobius Witch"]:
					promotions = [ 'PROMOTION_AIR1','PROMOTION_BODY1','PROMOTION_CHAOS1','PROMOTION_DEATH1','PROMOTION_EARTH1','PROMOTION_ENCHANTMENT1','PROMOTION_ENTROPY1','PROMOTION_FIRE1','PROMOTION_LAW1','PROMOTION_LIFE1','PROMOTION_MIND1','PROMOTION_NATURE1','PROMOTION_SHADOW1','PROMOTION_SPIRIT1','PROMOTION_SUN1','PROMOTION_WATER1' ]
					newUnit.setLevel(4)
					newUnit.setExperienceTimes100(1400, -1)
					for i in promotions:
						if randNum(10, "Mobius Witch Free Promotions") == 1:
							newUnit.setHasPromotion(getInfoType(i), True)
				else:
					newUnit.setExperienceTimes100(game.getGlobalCounter() * 25, -1)

	def doPlanarGateChance(self, iPlayer, pCity):
		gc 			= CyGlobalContext()
		game		= CyGame()
		iX 			 = pCity.getX()
		iY 			 = pCity.getY()
		getPlot	= CyMap().plot
		getTerrainInfo = gc.getTerrainInfo

		if iPlayer == -1:
			pPlayer = gc.getPlayer(game.getActivePlayer())
		else:
			pPlayer = gc.getPlayer(iPlayer)

		iMult = 1
		iHell = 0
		iAC = game.getGlobalCounter()

		if iAC >= 10:
			iMult += 0.15
		if iAC >= 20:
			iMult += 0.15
		if iAC >= 30:
			iMult += 0.15
		if iAC >= 40:
			iMult += 0.15
		if iAC >= 50:
			iMult += 0.15
		if iAC >= 60:
			iMult += 0.15
		if iAC >= 70:
			iMult += 0.15
		if iAC >= 80:
			iMult += 0.15
		if iAC >= 90:
			iMult += 0.15
		if iAC == 100:
			iMult += 0.15

		for iiX,iiY in BFC:
			pLoopPlot = getPlot(iX+iiX,iY+iiY)
			if not pLoopPlot.isNone():
				iTerrain = getTerrainInfo(pLoopPlot.getTerrainType())
				if iTerrain.isHell():
					iHell += 1

		if iHell != 0:
			iMult += iHell * 0.05

		iPlanarGateChance = (self.Defines["Planar Gate"] * iMult)
		iPlanarGateChance = scaleInverse(iPlanarGateChance)

		pCity.setCityCounter(iPlanarGateChance)

	def doListPlanarGate(self, iPlayer, pCity):
		gc = CyGlobalContext()
		if iPlayer == -1:
			pPlayer = gc.getPlayer(gc.getGame().getActivePlayer())
		else:
			pPlayer = gc.getPlayer(iPlayer)

		iMax = 1
		iCounter = CyGame().getGlobalCounter()
		if iCounter >= 50:  iMax = 2
		if iCounter >= 75:  iMax = 3
		if iCounter == 100: iMax = 4

		Building      = self.Buildings
		UnitClass     = self.UnitClasses
		Sheaim        = self.Units["Sheaim"]
		numB          = pCity.getNumBuilding
		unitCC        = pPlayer.getUnitClassCount
		countNumB     = pPlayer.countNumBuildings
		getBCC        = pPlayer.getBuildingClassCount
		listUnits = []
		iMax = iMax * countNumB( Building["Planar Gate"])
		if numB( Building["Gambling House"]) > 0:
			if unitCC( UnitClass["Revelers"]) < iMax:
				listUnits.append( Sheaim["Revelers"])
		if numB( Building["Mage Guild"]) > 0:
			if unitCC( UnitClass["Mobius Witch"]) < iMax:
				listUnits.append( Sheaim["Mobius Witch"])
		if numB( Building["Carnival"]) > 0:
			if unitCC( UnitClass["Chaos Marauder"]) < iMax:
				listUnits.append( Sheaim["Chaos Marauder"])
		if numB( Building["Grove"]) > 0:
			if unitCC( UnitClass["Manticore"]) < iMax:
				listUnits.append( Sheaim["Manticore"])
		if numB( Building["Public Baths"]) > 0:
			if unitCC( UnitClass["Succubus"]) < iMax:
				listUnits.append( Sheaim["Succubus"])
		if numB( Building["Obsidian Gate"]) > 0:
			if unitCC( UnitClass["Minotaur"]) < iMax:
				listUnits.append( Sheaim["Minotaur"])
		if numB( Building["Barracks"]) > 0:
			if unitCC( UnitClass["Colubra"]) < iMax:
				listUnits.append( Sheaim["Colubra"])
		if numB( Building["Temple of the Veil"]) > 0:
			if unitCC( UnitClass["Tar Demon"]) < iMax:
				listUnits.append( Sheaim["Tar Demon"])
		if numB( Building["Planar Gate"]) > 0:
			if unitCC( UnitClass["Fireball"]) < iMax:
				listUnits.append( Sheaim["Burning Eye"])
		return listUnits
# End Sheaim
# Start Memorial of the Refugee
	def doCityTurnMemorial(self, iPlayer, pCity):
		gc 			= CyGlobalContext()
		pPlot 		= pCity.plot()
		pPlayer 	= gc.getPlayer(iPlayer)
		randNum		= CyGame().getSorenRandNum

		lList = [ 'PROMOTION_ANGEL','PROMOTION_DWARF','PROMOTION_DARK_ELF','PROMOTION_ELF','PROMOTION_ORC','PROMOTION_MUSTEVAL','PROMOTION_LIZARDMAN','PROMOTION_GOBLIN','PROMOTION_FALLEN_ANGEL','PROMOTION_CENTAUR' ]
		sRace = lList[randNum(len(lList), "Pick Race")]
		iRace = gc.getInfoTypeForString(sRace)

		self.doMemorialChance(iPlayer, pCity)
		iMemorialChance = pCity.getCityCounter()

		if randNum(10000, "Refugee") <= iMemorialChance:
			newUnit = pPlayer.initUnit(self.Units["Grigori"]["Refugee"], pPlot.getX(), pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_NORTH)
			CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_REFUGEE", ()),'AS2D_DISCOVERBONUS',1,gc.getUnitInfo(newUnit.getUnitType()).getButton(),ColorTypes(8),pCity.getX(),pCity.getY(),True,True)
			if randNum(100, "Hero") <= 10:
				newUnit.setHasPromotion(self.Promotions["Effects"]["Hero"], True)
				newUnit.setName(self.MarnokNameGenerator(newUnit))
			if randNum(100, "Race") <= 50:
				newUnit.setHasPromotion(iRace, True)

	def doMemorialChance(self, iPlayer, pCity):
		gc 			= CyGlobalContext()
		if iPlayer == -1:
			pPlayer = gc.getPlayer(CyGame().getActivePlayer())
		else:
			pPlayer = gc.getPlayer(iPlayer)

		iChance 	= 0
		getNumB 	= pCity.getNumBuilding
		Building	= self.Buildings
		if getNumB(Building["Memorial Refugee"]) > 0:	iChance += 400
		if getNumB(Building["Dwelling of Refuge"]) > 0:	iChance += 200

		iChance = scaleInverse(iChance)

		pCity.setCityCounter(iChance)
# End Memorial of the Refugee

#### TODO: Update this to use PromotionDegrades
	def doTurnCualli(self, iPlayer):
		gc 			= CyGlobalContext()
		pPlayer 	= gc.getPlayer(iPlayer)
		py 			= PyPlayer(iPlayer)
		Promo		= self.Promotions["Generic"]
		Cualli		= self.Units["Cualli"]

		for pUnit in py.getUnitList():
			iUnitType = pUnit.getUnitType()
			if iUnitType == Cualli["Priest of Agruonn"] or iUnitType == Cualli["Shadow Priest of Agruonn"] or iUnitType == self.Heroes["Miquiztli"]:
				isHas = pUnit.isHasPromotion
				setPromo = pUnit.setHasPromotion
				if   isHas(Promo["Empower V"]):   setPromo(Promo["Empower V"], False)
				elif isHas(Promo["Empower IV"]):  setPromo(Promo["Empower IV"], False)
				elif isHas(Promo["Empower III"]): setPromo(Promo["Empower III"], False)
				elif isHas(Promo["Empower II"]):  setPromo(Promo["Empower II"], False)
				elif isHas(Promo["Empower I"]):   setPromo(Promo["Empower I"], False)

	def genesis(self, iPlayer):
		gc 			= CyGlobalContext()
		map 		= CyMap()
		Terrain		= self.Terrain
		Feature		= self.Feature
		plotByIndex = map.plotByIndex
		for i in xrange(map.numPlots()):
			pPlot = plotByIndex(i)
			if pPlot.getOwner() == iPlayer:
				iFeature = pPlot.getFeatureType()
				iTerrain = pPlot.getTerrainType()
# FF: Changed by Jean Elcard 14/01/2009 (speed tweak)
				if(iTerrain == Terrain["Tundra"]):
					pPlot.setTerrainType(Terrain["Taiga"], False, False)
				elif(iTerrain == Terrain["Taiga"]):
					pPlot.setTerrainType(Terrain["Plains"], False, False)
				elif(iTerrain == Terrain["Desert"] and iFeature != Feature["Oasis"]):
					pPlot.setTerrainType(Terrain["Plains"], False, False)
				elif(iTerrain == Terrain["Plains"]):
					pPlot.setTerrainType(Terrain["Grass"], False, False)
# FF: End Change
				elif(iTerrain == Terrain["Grass"] and pPlot.getImprovementType() == -1 and iFeature != Feature["Ancient Forest"] and pPlot.isPeak() == False):
					pPlot.setFeatureType(Feature["Forest"], 0)

	def getAshenVeilCity(self, iNum):
		gc = CyGlobalContext()
		Civ			= self.Civilizations
		Rel			= self.Religions
		iBestValue1 = 0
		iBestValue2 = 0
		iBestValue3 = 0
		pBestCity1 = -1
		pBestCity2 = -1
		pBestCity3 = -1
		for iPlayer in xrange(gc.getMAX_PLAYERS()):
			pPlayer = gc.getPlayer(iPlayer)
			if (pPlayer.isAlive() and pPlayer.getCivilizationType() != Civ["Infernal"]):
				for pyCity in PyPlayer(iPlayer).getCityList():
					pCity = pyCity.GetCy()
					if (pCity.isHasReligion(Rel["Ashen Veil"]) and pCity.isCapital() == False):
						iValue = pCity.getPopulation() * 100
						iValue += pCity.getCulture(iPlayer) / 3
						iValue += pCity.getNumBuildings() * 10
						iValue += pCity.getNumWorldWonders() * 100
						iValue += pCity.countNumImprovedPlots()
						if iValue > iBestValue1:
							iBestValue3 = iBestValue2
							pBestCity3 = pBestCity2
							iBestValue2 = iBestValue1
							pBestCity2 = pBestCity1
							iBestValue1 = iValue
							pBestCity1 = pCity
						elif (iValue > iBestValue2):
							iBestValue3 = iBestValue2
							pBestCity3 = pBestCity2
							iBestValue2 = iValue
							pBestCity2 = pCity
						elif (iValue > iBestValue3):
							iBestValue3 = iValue
							pBestCity3 = pCity
		if iNum == 1:
			return pBestCity1
		if iNum == 2:
			return pBestCity2
		if iNum == 3:
			return pBestCity3
		return -1

	def getHero(self, pPlayer):
		iHero 		= -1
		iCiv 		= pPlayer.getCivilizationType()
		Hero		= self.Heroes
		Civ			= self.Civilizations
		# Grey Fox: changed most if's to elif, cause pointless to check all
		if   iCiv == Civ["Bannor"]: 	return self.Heroes["Class-Donal"]
		elif iCiv == Civ["Malakim"]:	return self.Heroes["Class-Teutorix"]
		elif iCiv == Civ["Elohim"]:		return self.Heroes["Class-Corlindale"]
		elif iCiv == Civ["Mercurians"]: return self.Heroes["Class-Basium"]
		elif iCiv == Civ["Lanun"]:		return self.Heroes["Class-Guybrush"]
		elif iCiv == Civ["Kuriotates"]:	return self.Heroes["Class-Eurabatres"]
		elif iCiv == Civ["Ljosalfar"]:	return self.Heroes["Class-Gilden"]
		elif iCiv == Civ["Khazad"]:		return self.Heroes["Class-Maros"]
		elif iCiv == Civ["Hippus"]:		return self.Heroes["Class-Magnadine"]
		elif iCiv == Civ["Amurites"]:	return self.Heroes["Class-Govannon"]
		elif iCiv == Civ["Balseraphs"]:	return self.Heroes["Class-Loki"]
		elif iCiv == Civ["Clan of Embers"]: return self.Heroes["Class-Rantine"]
		elif iCiv == Civ["Svartalfar"]:	return self.Heroes["Class-Alazkan"]
		elif iCiv == Civ["Calabim"]:	return self.Heroes["Class-Losha"]
		elif iCiv == Civ["Sheaim"]:		return self.Heroes["Class-Abashi"]
		elif iCiv == Civ["Sidar"]:		return self.Heroes["Class-Rathus"]
		elif iCiv == Civ["Illians"]:	return self.Heroes["Class-Wilboman"]
		elif iCiv == Civ["Infernal"]:	return self.Heroes["Class-Hyborem"]
		elif iCiv == Civ["Dural"]:		return self.Heroes["Class-Karrlson"]
		elif iCiv == Civ["Chislev"]:	return self.Heroes["Class-Meshwaki"]
		elif iCiv == Civ["Archos"]:		return self.Heroes["Class-Mother"]
		elif iCiv == Civ["Mazatl"]: 	return self.Heroes["Class-Coatlann"]
		elif iCiv == Civ["Cualli"]:		return self.Heroes["Class-Miquiztli"]
		elif iCiv == Civ["Austrin"]:	return self.Heroes["Class-Harmatt"]
		elif iCiv == Civ["Scions"]:
			iLeader = pPlayer.getLeaderType()
			if   iLeader == self.Leaders["Risen Emperor"]: return self.Heroes["Class-Korrina"]
			elif iLeader == self.Leaders["Korrina"]: return self.Heroes["Class-The Risen Emperor"]
		elif iCiv == Civ["Frozen"]: 	return self.Heroes["Class-Taranis"] # TODO Ronkhar: make modular and move to frozen module
		elif iCiv == Civ["Mechanos"]:	return self.Heroes["Class-Feris"]

		return iHero

	def getUnholyVersion(self, pUnit):
		if( self.UnitCombats=={}):
			self.initialize()
		gc          = CyGlobalContext()
		iUnitCombat = pUnit.getUnitCombatType()
		iTier       = gc.getUnitInfo(pUnit.getUnitType()).getTier()
		iUnit       = -1
		UnitCombat  = self.UnitCombats
		Unit        = self.Units
		if iUnitCombat    == UnitCombat["Adept"]:
			if      iTier == 2: return Unit["Infernal"]["Imp"]
			elif    iTier == 3: return Unit["Generic"]["Mage"]
			elif    iTier == 4: return gc.getInfoTypeForString("UNIT_LICH")
		elif iUnitCombat  == UnitCombat["Animal"] or iUnitCombat == UnitCombat["Beast"]:
			if      iTier == 1: return Unit["Generic"]["Scout"]
			elif    iTier == 2: return Unit["Infernal"]["Hellhound"]
			elif    iTier == 3: return Unit["Generic"]["Assassin"]
			elif    iTier == 4: return self.Units["Veil"]["Beast of Agares"]
		elif iUnitCombat  == UnitCombat["Archer"]:
			if      iTier == 2: return Unit["Generic"]["Archer"]
			elif    iTier == 3: return Unit["Generic"]["Longbowman"]
			elif    iTier == 4: return Unit["Generic"]["Crossbowman"]
		elif iUnitCombat  == UnitCombat["Disciple"]:
			if      iTier == 2: return Unit["Veil"]["Disciple"]
			elif    iTier == 3: return Unit["Veil"]["Ritualist"]
			elif    iTier == 4: return Unit["Generic"]["Eidolon"]
		elif iUnitCombat  == UnitCombat["Melee"]:
			if      iTier == 1: return Unit["Summons"]["Skeleton"]
			elif    iTier == 2: return Unit["Veil"]["Diseased Corpse"]
			elif    iTier == 3: return Unit["Generic"]["Champion"]
			elif    iTier == 4: return Unit["Infernal"]["Balor"]
		elif iUnitCombat  == UnitCombat["Mounted"]:
			if      iTier == 2: return Unit["Generic"]["Horseman"]
			elif    iTier == 3: return Unit["Generic"]["Chariot"]
			elif    iTier == 4: return Unit["Infernal"]["Death Knight"]
		elif iUnitCombat  == UnitCombat["Recon"]:
			if      iTier == 1: return Unit["Generic"]["Scout"]
			elif    iTier == 2: return Unit["Infernal"]["Hellhound"]
			elif    iTier == 3: return Unit["Generic"]["Assassin"]
			elif    iTier == 4: return Unit["Generic"]["Beastmaster"]
		return iUnit

	def giftUnit(self, iUnit, iCivilization, iXP, pFromPlot, iFromPlayer):
		gc 			 	= CyGlobalContext()
		game		 	= CyGame()
		randNum			= game.getSorenRandNum
		addMessage 		= CyInterface().addMessage
		getText	 	 	= CyTranslator().getText
		Unit			= self.Units
		Mercurian		= Unit["Mercurian"]
		Infernal		= Unit["Infernal"]
		Frozen			= Unit["Frozen"]
		iNoAI			= UnitAITypes.NO_UNITAI
		iNorth			= DirectionTypes.DIRECTION_NORTH
#Changed in Frozen: TC01
		if iUnit in (Mercurian["Angel"], Infernal["Manes"], Frozen["Frozen Souls"]):
#End of Frozen
			iChance = 100 - (game.countCivPlayersAlive() * 3)
			iChance = iChance + iXP/100
			if iChance < 5:
				iChance = 5
			if iChance > 95:
				iChance = 95
			if randNum(100, "Gift Unit") > iChance:
				iUnit = -1
		if iUnit != -1:
			bValid = False
			iNumCivs 	 = game.getNumCivActive(iCivilization)
			activePlayer = game.getCivActivePlayer
			getPlayer 	 = gc.getPlayer
			for i in xrange(iNumCivs):
				iPlayer = activePlayer(iCivilization, i)
				pPlayer = getPlayer(iPlayer)
				if (pPlayer.isAlive()):
					if pPlayer.getCivilizationType() == iCivilization:
						if iUnit == Frozen["Frozen Souls"]:
							iChance = 100 * pPlayer.getAveragePopulation()
							iChance /= 15
							if iChance > 95 :    iChance = 95 # even when population is too high, 1 chance in 20 to receive a frozen soul
							if randNum(100, "Gift Frozen Soul") < iChance:
								break
						py = PyPlayer(iPlayer)
						iNumCities = py.getNumCities()
						if iNumCities > 0:
							iRnd = randNum(iNumCities, "Gift Unit")
							pCity = py.getCityList()[iRnd]
							pPlot = pCity.plot()
							bHuman = pPlayer.isHuman()
							iX = pPlot.getX(); iY = pPlot.getY()
							newUnit = pPlayer.initUnit(iUnit, iX, iY, iNoAI, iNorth)
							newUnit.changeExperienceTimes100(iXP, -1, False, False, False)
							if (pFromPlot != -1 and getPlayer(iFromPlayer).isHuman()):
								bValid = True
							if bHuman:
								if iUnit == Infernal["Manes"]:
									addMessage(iPlayer,True,25,getText("TXT_KEY_MESSAGE_ADD_MANES",()),'AS2D_UNIT_FALLS',1,'Art/Interface/Buttons/Promotions/Races/Demon.dds',ColorTypes(7),iX,iY,True,True)
								if iUnit == Mercurian["Angel"]:
									addMessage(iPlayer,True,25,getText("TXT_KEY_MESSAGE_ADD_ANGEL",()),'AS2D_UNIT_FALLS',1,'Art/Interface/Buttons/Promotions/Races/Angel.dds',ColorTypes(7),iX,iY,True,True)
		#Changed in Frozen: TC01
								if iUnit == Frozen["Frozen Souls"]:
									addMessage(iPlayer,True,25,getText("TXT_KEY_MESSAGE_ADD_FROZEN_SOULS",()),'AS2D_UNIT_FALLS',1,'Art/Civs/Frozen/wintered.dds',ColorTypes(7),iX,iY,True,True)
							if (not bHuman and (iUnit == Infernal["Manes"] or iUnit == Frozen["Frozen Souls"]) and pCity != -1):
		#End of Frozen
								if randNum(100, "Manes") < (100 - (pCity.getPopulation() * 5)):
									pCity.changePopulation(1)
									newUnit.kill(True, PlayerTypes.NO_PLAYER)
			if bValid:
				if iUnit == Infernal["Manes"]:
					addMessage(iFromPlayer,True,25,getText("TXT_KEY_MESSAGE_UNIT_FALLS",()),'AS2D_UNIT_FALLS',1,'Art/Interface/Buttons/Promotions/Races/Demon.dds',ColorTypes(7),pFromPlot.getX(),pFromPlot.getY(),True,True)
				if iUnit == Mercurian["Angel"]:
					addMessage(iFromPlayer,True,25,getText("TXT_KEY_MESSAGE_UNIT_RISES",()),'AS2D_UNIT_FALLS',1,'Art/Interface/Buttons/Promotions/Races/Angel.dds',ColorTypes(7),pFromPlot.getX(),pFromPlot.getY(),True,True)
#Added in Frozen: TC01
				if iUnit == Frozen["Frozen Souls"]:
					addMessage(iFromPlayer,True,25,getText("TXT_KEY_MESSAGE_UNIT_FREEZES",()),'AS2D_UNIT_FALLS',1,'Art/Interface/Buttons/Promotions/Races/frostling.dds',ColorTypes(7),pFromPlot.getX(),pFromPlot.getY(),True,True)
#End of Frozen

	def warScript(self, iPlayer):
		gc 				= CyGlobalContext()
		getPlayer 		= gc.getPlayer
		game 			= CyGame()
		getRank 		= game.getPlayerRank
		getGCounter		= game.getGlobalCounter
		getTeam 		= gc.getTeam
		pPlayer 		= getPlayer(iPlayer)
		Civ				= self.Civilizations
		Rel				= self.Religions
		iCiv 			= pPlayer.getCivilizationType()
		iTeam 			= pPlayer.getTeam()
		iAlignment		= pPlayer.getAlignment()
		Alignment		= self.Alignments
		BuildingClass	= self.BuildingClasses
		Building		= self.Buildings
		iEnemy 			= -1
		WarPlanTotal 	= WarPlanTypes.WARPLAN_TOTAL

		getRandNum 	= game.getSorenRandNum
		for iPlayer2 in xrange(gc.getMAX_PLAYERS()):
			pPlayer2 = getPlayer(iPlayer2)
			if pPlayer2.isAlive():
				iTeam2 = pPlayer2.getTeam()
				pTeam = getTeam(iTeam)
				if pTeam.isAVassal() == False:
					if pTeam.isAtWar(iTeam2):
						randName = "War Script, Player %s vs Player %s" % (iPlayer, iPlayer2)
						if getRandNum(100, randName) < 5:
							self.dogpile(iPlayer, iPlayer2)
					if self.warScriptAllow(iPlayer, iPlayer2):
						getNumB = pPlayer2.getNumBuilding
						if pPlayer2.getBuildingClassMaking(BuildingClass["Tower of Mastery"]) > 0:
							if pTeam.getAtWarCount(True) == 0:
								startWar(iPlayer, iPlayer2, WarPlanTotal)
						if (getNumB(Building["Altar - Divine"]) > 0 or getNumB(Building["Altar - Exalted"]) > 0):
							if iAlignment == Alignment["Evil"]:
								if pTeam.getAtWarCount(True) == 0:
									startWar(iPlayer, iPlayer2, WarPlanTotal)
						if iCiv == Civ["Mercurians"]:
							if pPlayer2.getStateReligion() == Rel["Ashen Veil"]:
								startWar(iPlayer, iPlayer2, WarPlanTotal)
						if getGCounter() > 20:
							if iCiv == Civ["Svartalfar"]:
								if (pPlayer2.getCivilizationType() == Civ["Ljosalfar"] and getRank(iPlayer) > getRank(iPlayer2)):
									startWar(iPlayer, iPlayer2, WarPlanTotal)
							elif iCiv == Civ["Ljosalfar"]:
								if (pPlayer2.getCivilizationType() == Civ["Svartalfar"] and getRank(iPlayer) > getRank(iPlayer2)):
									startWar(iPlayer, iPlayer2, WarPlanTotal)
						if (getGCounter() > 40 or iCiv == Civ["Infernal"] or Civ["Doviello"]):
							if iAlignment == Alignment["Evil"]:
								if (pTeam.getAtWarCount(True) == 0 and getRank(iPlayer2) > getRank(iPlayer)):
									if (iEnemy == -1 or getRank(iPlayer2) > getRank(iEnemy)):
										iEnemy = iPlayer2
		if iEnemy != -1:
			if getRank(iPlayer) > getRank(iEnemy):
				startWar(iPlayer, iEnemy, WarPlanTotal)

	def warScriptAllow(self, iPlayer, iPlayer2):
		gc 				= CyGlobalContext()
		getPlayer 		= gc.getPlayer
		pPlayer 		= getPlayer(iPlayer)
		pPlayer2 		= getPlayer(iPlayer2)
		iTeam 			= getPlayer(iPlayer).getTeam()
		iTeam2 			= getPlayer(iPlayer2).getTeam()
		pTeam 			= gc.getTeam(iTeam)
		if pPlayer.isBarbarian() or pPlayer2.isBarbarian(): return False
		if pTeam.isHasMet(iTeam2) == False: return False
		if pTeam.AI_getAtPeaceCounter(iTeam2) < 20: return False
		if pTeam.isAtWar(iTeam2): return False
		if pPlayer.getCivilizationType() == self.Civilizations["Infernal"]:
			if pPlayer2.getStateReligion() == self.Religions["Ashen Veil"]:
				return False
		return True

	def dogpile(self, iPlayer, iVictim):
		gc 				= CyGlobalContext()
		getPlayer 		= gc.getPlayer
		pPlayer 		= getPlayer(iPlayer)
		Civ				= self.Civilizations
		Option			= self.GameOptions
		warAllow 		= self.warScriptAllow
		game			= CyGame()
		iGlobalCounter	= game.getGlobalCounter()
		randNum 		= game.getSorenRandNum
		for iPlayer2 in xrange(gc.getMAX_PLAYERS()):
			pPlayer2 = getPlayer(iPlayer2)
			iChance = -1
			if pPlayer2.isAlive():
				if (self.dogPileAllow(iPlayer, iPlayer2) and warAllow(iPlayer2, iVictim)):
					iChance = pPlayer2.AI_getAttitude(iPlayer) * 5
					if iChance > 0:
						iChance = iChance - (pPlayer2.AI_getAttitude(iVictim) * 5) - 10
						if not Option["Aggressive AI"]: iChance = iChance - 10
						if iChance > 0:
							iChance = iChance + (iGlobalCounter / 3)
							if pPlayer2.getCivilizationType() == Civ["Balseraphs"]:
								iChance = randNum(50, "Dogpile")
							if randNum(100, "Dogpile") < iChance:
								startWar(iPlayer2, iVictim, WarPlanTypes.WARPLAN_DOGPILE)

	def dogPileAllow(self, iPlayer, iPlayer2):
		gc 				= CyGlobalContext()
		getPlayer 		= gc.getPlayer
		pPlayer 		= getPlayer(iPlayer)
		pPlayer2 		= getPlayer(iPlayer2)
		iTeam 			= getPlayer(iPlayer).getTeam()
		iTeam2 			= getPlayer(iPlayer2).getTeam()
		if iPlayer 	== iPlayer2: return False
		if iTeam 	== iTeam2: 	 return False
		if pPlayer2.isHuman():   return False
		if pPlayer2.getCivilizationType() == self.Civilizations["Elohim"]: return False
		if gc.getTeam(iTeam2).isAVassal(): return False
		return True

	def warn(self, iPlayer, szText, pPlot):
		gc 			= CyGlobalContext()
		getPlayer 	= gc.getPlayer
		pPlayer 	= getPlayer(iPlayer)
		addMsg		= CyInterface().addMessage
		getText		= CyTranslator().getText
		iX = pPlot.getX(); iY = pPlot.getY()
		for iPlayer2 in xrange(gc.getMAX_PLAYERS()):
			pPlayer2 = getPlayer(iPlayer2)
			if (pPlayer2.isAlive() and iPlayer != iPlayer2):
				if pPlayer2.isHuman():
					popupInfo = CyPopupInfo()
					popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON)
					popupInfo.setText(szText)
					popupInfo.setOnClickedPythonCallback("selectWarn")
					popupInfo.addPythonButton(getText("TXT_KEY_MAIN_MENU_OK",()), "")
					popupInfo.addPopup(iPlayer2)
				if pPlot != -1:
					addMsg(iPlayer2,True,25, getText("TXT_KEY_MESSAGE_ALTAR_OF_THE_LUONNOTAR",()),'AS2D_DISCOVERBONUS',1,'Art/Interface/Buttons/Buildings/AltaroftheLuonnotar.dds',ColorTypes(7),pPlot.getX(),pPlot.getY(),True,True)

	def countMana(self, pPlayer):
		getNumBoni 	= pPlayer.getNumAvailableBonuses
		Mana		= self.Mana
		iNum = 0
		iNum += getNumBoni( Mana["Air"]        )
		iNum += getNumBoni( Mana["Body"]       )
		iNum += getNumBoni( Mana["Chaos"]      )
		iNum += getNumBoni( Mana["Creation"]   )
		iNum += getNumBoni( Mana["Death"]      )
		iNum += getNumBoni( Mana["Dimensional"])
		iNum += getNumBoni( Mana["Earth"]      )
		iNum += getNumBoni( Mana["Enchantment"])
		iNum += getNumBoni( Mana["Entropy"]    )
		iNum += getNumBoni( Mana["Fire"]       )
		iNum += getNumBoni( Mana["Force"]      )
		iNum += getNumBoni( Mana["Ice"]        )
		iNum += getNumBoni( Mana["Law"]        )
		iNum += getNumBoni( Mana["Life"]       )
		iNum += getNumBoni( Mana["Metamagic"]  )
		iNum += getNumBoni( Mana["Mind"]       )
		iNum += getNumBoni( Mana["Nature"]     )
		iNum += getNumBoni( Mana["Shadow"]     )
		iNum += getNumBoni( Mana["Spirit"]     )
		iNum += getNumBoni( Mana["Sun"]        )
		iNum += getNumBoni( Mana["Water"]      )

		return iNum

	def canReceiveReligionUnit(self, pPlayer):
		iCiv 		= pPlayer.getCivilizationType()
		Civ			= self.Civilizations
		if   iCiv == Civ["Cualli"]: return False
	#	elif iCiv == Civ["Mazatl"]: return False
		return True

	def MarnokNameGenerator(self, unit):
		gc 				= CyGlobalContext()
		pPlayer 		= gc.getPlayer(unit.getOwner())
		pCiv 			= pPlayer.getCivilizationType()
		iReligion 		= pPlayer.getStateReligion()
		pAlign 			= pPlayer.getAlignment()
		iUnitType 		= unit.getUnitType()
		isHasPromotion	= unit.isHasPromotion
		randNum			= CyGame().getSorenRandNum
		Alignment		= self.Alignments
		Rel				= self.Religions
		Generic			= self.Promotions["Generic"]
		Promo			= self.Promotions["Effects"]
		Race			= self.Promotions["Race"]
		Unit			= self.Units
		Summon			= self.Units["Summons"]

		lPre=["ta","go","da","bar","arc","ken","an","ad","mi","kon","kar","mar","wal","he", "ha", "re", "ar", "bal", "bel", "bo", "bri", "car","dag","dan","ma","ja","co","be","ga","qui","sa"]
		lMid=["ad","z","the","and","tha","ent","ion","tio","for","tis","oft","che","gan","an","en","wen","on","d","n","g","t","ow","dal"]
		lEnd=["ar","sta","na","is","el","es","ie","us","un","th", "er","on","an","re","in","ed","nd","at","en","le","man","ck","ton","nok","git","us","or","a","da","u","cha","ir"]

		lEpithet=["red","blue","black","grey","white","strong","brave","old","young","great","slayer","hunter","seeker"]
		lNoun=["spirit","soul","boon","born","staff","rod","shield","autumn","winter","spring","summer","wit","horn","tusk","glory","claw","tooth","head","heart", "blood","breath", "blade", "hand", "lover","bringer","maker","taker","river","stream","moon","star","face","foot","half","one","hundred","thousand"]
		lSchema=["CPME","CPMESCPME","CPESCPE","CPE","CPMME","CPMDCME","CPMAME","KCPMESCUM","CPMME[ the ]CX", "CPMESCXN", "CPMME[ of ]CPMME", "CNNSCXN"]

		if pAlign == Alignment["Evil"]:
			lNoun = lNoun + ["fear","terror","reign","brood","snare","war","strife","pain","hate","evil","hell","misery","murder","anger","fury","rage","spawn","sly","blood","bone","scythe","slave","bound","ooze","scum"]
			lEpithet=["dark","black","white","cruel","foul"]
		if iReligion == Rel["Ashen Veil"]:
			lEpithet = lEpithet + ["fallen","diseased","infernal","profane","corrupt"]
			lSchema = lSchema + ["CPME[ the ]CX"]
		if iReligion == Rel["Octopus Overlords"]:
			lPre = lPre + ["cth","cht","shu","az","ts","dag","hy","gla","gh","rh","x","ll"]
			lMid = lMid + ["ul","tha","on","ug","st","oi"]
			lEnd = lEnd + ["hu","on", "ha","ua","oa","uth","oth","ath","thua", "thoa","ur","ll","og","hua"]
			lEpithet = lEpithet + ["nameless","webbed","deep","watery"]
			lNoun = lNoun + ["tentacle","wind","wave","sea","ocean","dark","crab","abyss","island"]
			lSchema = lSchema + ["CPMME","CPDMME","CPAMAME","CPMAME","CPAMAMEDCPAMAE"]
		if iReligion == Rel["Order"]:
			lPre = lPre + ["ph","v","j"]
			lMid = lMid + ["an","al","un"]
			lEnd = lEnd + ["uel","in","il"]
			lEpithet = lEpithet + ["confessor","crusader", "faithful","obedient","good"]
			lNoun = lNoun + ["order", "faith", "heaven","law"]
			lSchema = lSchema + ["CPESCPME","CPMESCPE","CPMESCPME", "CPESCPE"]
		if iReligion == Rel["Fellowship"]:
			lPre = lPre + ["ki","ky","yv"]
			lMid = lMid + ["th","ri"]
			lEnd = lEnd + ["ra","el","ain"]
			lEpithet = lEpithet + ["green"]
			lNoun = lNoun + ["tree","bush","wood","berry","elm","willow","oak","leaf","flower","blossom"]
			lSchema = lSchema + ["CPESCN","CPMESCNN","CPMESCXN"]
		if iReligion == Rel["Runes of Kilmorph"]:
			lPre = lPre + ["bam","ar","khel","ki"]
			lMid = lMid + ["th","b","en"]
			lEnd = lEnd + ["ur","dain","ain","don"]
			lEpithet = lEpithet + ["deep","guard","miner"]
			lNoun = lNoun + ["rune","flint","slate","stone","rock","iron","copper","mithril","thane","umber"]
			lSchema = lSchema + ["CPME","CPMME"]
		if iReligion == Rel["Empyrean"]:
			lEpithet = lEpithet + ["radiant","holy"]
			lNoun = lNoun + ["honor"]
		if iReligion == Rel["Council of Esus"]:
			lEpithet = lEpithet + ["hidden","dark"]
			lNoun = lNoun + ["cloak","shadow","mask"]
			lSchema = lSchema + ["CPME","CPMME"]
		if isHasPromotion(Generic["Enraged"]):
			# I have left this as a copy of the Barbarian, see how it goes, this might do the trick. I plan to use it when there is a chance a unit will go Barbarian anyway.
			lPre = lPre + ["gru","bra","no","os","dir","ka","z"]
			lMid = lMid + ["g","ck","gg","sh","b","bh","aa"]
			lEnd = lEnd + ["al","e","ek","esh","ol","olg","alg"]
			lNoun = lNoun + ["death", "hate", "rage", "mad","insane","berserk"]
			lEpithet = lEpithet + ["smasher", "breaker", "mangle","monger"]
		if isHasPromotion(Generic["Crazed"]):
			# might want to tone this down, because I plan to use it as possession/driven to madness, less than madcap zaniness.
			lPre = lPre + ["mad","pim","zi","zo","fli","mum","dum","odd","slur"]
			lMid = lMid + ["bl","pl","gg","ug","bl","b","zz","abb","odd"]
			lEnd = lEnd + ["ad","ap","izzle","onk","ing","er","po","eep","oggle","y"]
		if isHasPromotion(Promo["Vampire"]):
			lPre = lPre + ["dra","al","nos","vam","vla","tep","bat","bar","cor","lil","ray","zar","stra","le"]
			lMid = lMid + ["cul","u","car","fer","pir","or","na","ov","sta"]
			lEnd = lEnd + ["a","d","u","e","es","y","bas","vin","ith","ne","ak","ich","hd","t"]
		if isHasPromotion(Race["Demon"]):
			lPre = lPre + ["aa","ab","adr","ah","al","de","ba","cro","da","be","eu","el","ha","ib","me","she","sth","z"]
			lMid = lMid + ["rax","lia","ri","al","as","b","bh","aa","al","ze","phi","sto","phe","cc","ee"]
			lEnd = lEnd + ["tor","tan","ept","lu","res","ah","mon","gon","bul","gul","lis","les","uz"]
			lSchema = ["CPMMME","CPMACME", "CPKMAUAPUE", "CPMMME[ the ]CNX"]
		if iUnitType == Unit["Savage"]["Hill Giant"]:
			lPre = lPre + ["gor","gra","gar","gi","gol"]
			lMid = lMid + ["gan","li","ri","go"]
			lEnd = lEnd + ["tus","tan","ath","tha"]
			lSchema = lSchema +["CXNSCNN","CPESCNE", "CPMME[ the ]CX"]
			lEpithet = lEpithet + ["large","huge","collossal","brutal","basher","smasher","crasher","crusher"]
			lNoun = lNoun + ["fist","tor","hill","brute","stomp"]
		if iUnitType == Unit["Clan of Embers"]["Lizardman"]:
			lPre = lPre + ["ss","s","th","sth","hss"]
			lEnd = lEnd + ["ess","iss","ath","tha"]
			lEpithet = lEpithet + ["cold"]
			lNoun = lNoun + ["hiss","tongue","slither","scale","tail","ruin"]
			lSchema = lSchema + ["CPAECPAE","CPAKECPAU","CPAMMAE"]
		if iUnitType == Summon["Fire Elemental"] or iUnitType == Summon["Azer"]:
			lPre = lPre + ["ss","cra","th","sth","hss","roa"]
			lMid = lMid + ["ss","ck","rr","oa","iss","tt"]
			lEnd = lEnd + ["le","iss","st","r","er"]
			lNoun = lNoun + ["hot", "burn","scald","roast","flame","scorch","char","sear","singe","fire","spit"]
			lSchema = ["CNN","CNX","CPME","CPME[ the ]CNX","CPMME","CNNSCPME"]
		if iUnitType == Summon["Water Elemental"]:
			lPre = lPre + ["who","spl","dr","sl","spr","sw","b"]
			lMid = lMid + ["o","a","i","ub","ib"]
			lEnd = lEnd + ["sh","p","ter","ble"]
			lNoun = lNoun + ["wave","lap","sea","lake","water","tide","surf","spray","wet","damp","soak","gurgle","bubble"]
			lSchema = ["CNN","CNX","CPME","CPME[ the ]CNX","CPMME","CNNSCPME"]
		if iUnitType == Summon["Air Elemental"]:
			lPre = lPre + ["ff","ph","th","ff","ph","th"]
			lMid = lMid + ["oo","aa","ee","ah","oh"]
			lEnd = lEnd + ["ff","ph","th","ff","ph","th"]
			lNoun = lNoun + ["wind","air","zephyr","breeze","gust","blast","blow"]
			lSchema = ["CNN","CNX","CPME","CPME[ the ]CNX","CPMME","CNNSCPME"]
		if iUnitType == Summon["Earth Elemental"]:
			lPre = lPre + ["gra","gro","kro","ff","ph","th"]
			lMid = lMid + ["o","a","u"]
			lEnd = lEnd + ["ck","g","k"]
			lNoun = lNoun + ["rock","stone","boulder","slate","granite","rumble","quake"]
			lSchema = ["CNN","CNX","CPME","CPME[ the ]CNX","CPMME","CNNSCPME"]

		# SEA BASED
		# Check for ships - special schemas
		if unit.getUnitCombatType() == self.UnitCombats["Naval"]:
			lEnd = lEnd + ["ton","town","port"]
			lNoun = lNoun + ["lady","jolly","keel","bow","stern", "mast","sail","deck","hull","reef","wave"]
			lEpithet = lEpithet + ["sea", "red", "blue","grand","barnacle","gull"]
			lSchema = ["[The ]CNN", "[The ]CXN", "[The ]CNX","[The ]CNSCN", "[The ]CNSCX","CPME['s ]CN","[The ]CPME", "[The ]CNX","CNX","CN['s ]CN"]

		# # #
		# Pick a Schema
		sSchema = lSchema[randNum(len(lSchema), "Name Gen")]
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
					sAdd = lPre[randNum(len(lPre), "Name Gen")]
					iDone = 1
				elif sAction == "M":	# Mid 	: middle syllables
					sAdd = lMid[randNum(len(lMid), "Name Gen")]
					iDone = 1
				elif sAction == "E":	# End	: end of names
					sAdd = lEnd[randNum(len(lEnd), "Name Gen")]
					iDone = 1
				elif sAction == "X":	# Epithet	: epithet word part
					#epithets ("e" was taken!)
					sAdd = lEpithet[randNum(len(lEpithet), "Name Gen")]
					iDone = 1
				elif sAction == "N":	# Noun	: noun word part
					#noun
					sAdd = lNoun[randNum(len(lNoun), "Name Gen")]
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

	def resetRepublicTraits(self, pPlayer):
		hasTrait 	= pPlayer.hasTrait
		setFeat 	= pPlayer.setFeatAccomplished
		setHasTrait = pPlayer.setHasTrait
		Trait		= self.Traits
		gc = CyGlobalContext()
		getInfoType 		= gc.getInfoTypeForString
		
		if hasTrait(getInfoType("TRAIT_AGGRESSIVE_REPUBLIC")):
			setHasTrait(getInfoType("TRAIT_AGGRESSIVE_REPUBLIC"),False)
		
		if hasTrait(getInfoType("TRAIT_DEFENDER_REPUBLIC")):
			setHasTrait(getInfoType("TRAIT_DEFENDER_REPUBLIC"),False)
		
		if hasTrait(getInfoType("TRAIT_FINANCIAL_REPUBLIC")):
			setHasTrait(getInfoType("TRAIT_FINANCIAL_REPUBLIC"),False)
		
		if hasTrait(getInfoType("TRAIT_EXPANSIVE_REPUBLIC")):
			setHasTrait(getInfoType("TRAIT_EXPANSIVE_REPUBLIC"),False)
		
		if hasTrait(getInfoType("TRAIT_SPIRITUAL_REPUBLIC")):
			setHasTrait(getInfoType("TRAIT_SPIRITUAL_REPUBLIC"),False)
		
		if hasTrait(getInfoType("TRAIT_ORGANIZED_REPUBLIC")):
			setHasTrait(getInfoType("TRAIT_ORGANIZED_REPUBLIC"),False)
		
		if hasTrait(getInfoType("TRAIT_PHILOSOPHICAL_REPUBLIC")):
			setHasTrait(getInfoType("TRAIT_PHILOSOPHICAL_REPUBLIC"),False)
		
		if hasTrait(getInfoType("TRAIT_INDUSTRIOUS_REPUBLIC")):
			setHasTrait(getInfoType("TRAIT_INDUSTRIOUS_REPUBLIC"),False)

	# Doviello Experience Share - MrUnderhill
	def inheritExperience(self, newUnit, fReturnRate):
		if not newUnit.isAlive():
			return
		py = PyPlayer(newUnit.getOwner())
		iTotalXP = 0
		iMatchingUnits = 0
		if newUnit.getUnitCombatType() == -1:
			return
		for pUnit in py.getUnitList():
			if pUnit.getUnitCombatType() == newUnit.getUnitCombatType() and pUnit != newUnit:
				iTotalXP += pUnit.getExperience()
				iTotalXP += pUnit.baseCombatStr()
				iMatchingUnits += 1
		if iMatchingUnits == 0:
			return
		iAverage = int(iTotalXP / (iMatchingUnits ** fReturnRate))
		newUnit.changeExperience(iAverage, -1, False, False, False)
		
	def angelorMane(self, unit):
		if( self.Religions=={}):
			self.initialize()
		gc 				= CyGlobalContext()
		iUnitCombat = unit.getUnitCombatType()
		UnitCombat  = self.UnitCombats
		iManes = gc.getInfoTypeForString('UNIT_MANES')
		iReligion = unit.getReligion()
		iAngel = gc.getInfoTypeForString('UNIT_ANGEL')
		pPlayer = gc.getPlayer(unit.getOwner())
		Rel				= self.Religions
		Alignment		= self.Alignments
		randNum 	= CyGame().getSorenRandNum
					
		if unit.isAlive() and iUnitCombat  != gc.getInfoTypeForString ("UNITCOMBAT_ANIMAL") and iUnitCombat != gc.getInfoTypeForString ("UNITCOMBAT_BEAST"):
			if iReligion in [Rel["Ashen Veil"], Rel["Octopus Overlords"], Rel["White Hand"], Rel["Council of Esus"]]:
				return iManes
			if iReligion in [Rel["Order"],Rel["Empyrean"]]:
				return iAngel
			if iReligion in [ Rel["Fellowship"], Rel["Runes of Kilmorph"]]:
				return -1
				
			lEvilProms = [	'PROMOTION_UNHOLY_TAINT',
					'PROMOTION_VAMPIRE',
					'PROMOTION_DEATH1', 'PROMOTION_DIMENSIONAL1', 'PROMOTION_ENTROPY1',
					'PROMOTION_CHAOS2','PROMOTION_DEATH2', 'PROMOTION_DIMENSIONAL2', 'PROMOTION_ENTROPY2',
					'PROMOTION_CHAOS3','PROMOTION_DEATH3', 'PROMOTION_DIMENSIONAL3', 'PROMOTION_ENTROPY3',"PROMOTION_GULAGARM_POISONED"
					]
			for sProm in lEvilProms:
				if unit.isHasPromotion(gc.getInfoTypeForString(sProm)):
					return iManes

			
			pPlayer = gc.getPlayer(unit.getOwner())
			if pPlayer.isBarbarian():
				TestBarb = randNum(100, "Barb")
				if TestBarb<80:
					return -1
			if pPlayer.getLeaderType() == gc.getInfoTypeForString('LEADER_BASIUM'):
				return iAngel
			if not (pPlayer.isHasFlag(gc.getInfoTypeForString('FLAG_UNPURIFIED_WELL')) or pPlayer.isHasFlag(gc.getInfoTypeForString('FLAG_PURIFIED_WELL'))):
				for i in range(CyMap().numPlots()):
					loopPlot = CyMap().plotByIndex(i)
					if (loopPlot.getImprovementType() == gc.getInfoTypeForString('IMPROVEMENT_BRADELINES_WELL'))  :
						gc.getGame().setGlobalFlag(gc.getInfoTypeForString('FLAG_UNPURIFIED_WELL'),True)
						break
					if (loopPlot.getImprovementType() == gc.getInfoTypeForString('IMPROVEMENT_BRADELINES_WELL_PURIFIED')):
						gc.getGame().setGlobalFlag(gc.getInfoTypeForString('FLAG_PURIFIED_WELL'),True)
						break
				if not (pPlayer.isHasFlag(gc.getInfoTypeForString('FLAG_UNPURIFIED_WELL')) or pPlayer.isHasFlag(gc.getInfoTypeForString('FLAG_PURIFIED_WELL'))):
					gc.getGame().setGlobalFlag(gc.getInfoTypeForString('FLAG_PURIFIED_WELL'),True)
					
			lGoodProms = ['PROMOTION_HERALDS_BLESSING']
			for sProm in lGoodProms:
				if unit.isHasPromotion(gc.getInfoTypeForString(sProm)):
					return iAngel
			if (not pPlayer.isHasFlag(gc.getInfoTypeForString('FLAG_PURIFIED_WELL'))):		
				if pPlayer.getAlignment()==Alignment["Good"]:
					TestGood = randNum(100, "Good")
					if TestGood < 20:
						return iAngel
					elif TestGood >95:
						return iManes
					else :
						return -1
				if pPlayer.getAlignment()==Alignment["Neutral"]:
					TestGood = randNum(100, "Good")
					if TestGood < 5:
						return iAngel
					elif TestGood >50:
						return iManes
					else :
						return -1
				if pPlayer.getAlignment()==Alignment["Evil"]:
					TestGood = randNum(100, "Good")
					if TestGood < 0:
						return iAngel
					elif TestGood >10:
						return iManes
					else :
						return -1
			else:		
				if pPlayer.getAlignment()==Alignment["Good"]:
					TestGood = randNum(100, "Good")
					if TestGood < 40:
						return iAngel
					elif TestGood >105:
						return iManes
					else :
						return -1
				if pPlayer.getAlignment()==Alignment["Neutral"]:
					TestGood = randNum(100, "Good")
					if TestGood < 5:
						return iAngel
					elif TestGood >95:
						return iManes
					else :
						return -1
				if pPlayer.getAlignment()==Alignment["Evil"]:
					TestGood = randNum(100, "Good")
					if TestGood < 0:
						return iAngel
					elif TestGood >50:
						return iManes
					else :
						return -1
		return -1
		
	def findImprovements(self, iImprovementType):
		listImprovements = []
		for i in xrange (CyMap().numPlots()):
			pPlot = CyMap().plotByIndex(i)
			if pPlot.getImprovementType() == iImprovementType:
				listImprovements.append(pPlot)
		return listImprovements

	# Demon Lord Spawn
	def spawnDemonLord(self,iLeader,iPlayer,bReassign = False):
		iInfernalPlayer	= getOpenPlayer()
		gc				= CyGlobalContext()
		git				= gc.getInfoTypeForString
		pBestPlot		= -1
		iBestPlot		= -1
		print "debug spawnDemonLord"
		print self
		print iLeader
		print iPlayer
		print bReassign
		# Plot Picker
		for i in xrange(CyMap().numPlots()):
			pPlot = CyMap().plotByIndex(i)
			iPlot = -1
			if not pPlot.isWater() and not pPlot.isCity() and not pPlot.isImpassable() and not pPlot.isPeak():
				if pPlot.getNumUnits() == 0:
					iPlot = CyGame().getSorenRandNum(500, "Place Demon Lord")
					iPlot += pPlot.area().getNumTiles() * 2
					iPlot += pPlot.area().getNumUnownedTiles() * 10
					if not pPlot.isOwned():
						iPlot += 500
					if pPlot.getOwner() == iPlayer: # Probability of Infernals spawning in players' borders is minuscule. Useless block.
						iPlot += 200
			if iPlot > iBestPlot:
				iBestPlot = iPlot
				pBestPlot = pPlot
		# Setting Civilization
		if iInfernalPlayer != -1 and pBestPlot != -1:
			# Removing barbs around the plot
			pBestPlot.setPlotType(PlotTypes.PLOT_LAND, True, True)
			for iX,iY in RANGE2:
				getPlot = CyMap().plot
				pClearPlot = getPlot(iX+pBestPlot.getX(),iY+pBestPlot.getY())
				for i in xrange(pClearPlot.getNumUnits()):
					pUnit = pPlot.getUnit(i)
					if pUnit.isBarbarian() and not gc.getUnitClassInfo(pUnit.getUnitClassType()).isUnique():
						pUnit.kill()
			# Spawning Civilization
			CyGame().addPlayerAdvanced(iInfernalPlayer, -1, iLeader, git("CIVILIZATION_INFERNAL"),iPlayer)
			iFounderTeam	= gc.getPlayer(iPlayer).getTeam()
			eFounderTeam	= gc.getTeam(iFounderTeam)
			iInfernalTeam	= gc.getPlayer(iInfernalPlayer).getTeam()
			eInfernalTeam	= gc.getTeam(iInfernalTeam)
			iDemonTeam		= gc.getPlayer(gc.getDEMON_PLAYER()).getTeam()
			pInfernalPlayer	= gc.getPlayer(iInfernalPlayer)
			# Unique spawn for Demon Leader
			if iLeader == git("LEADER_HYBOREM"):
				NewUnitHero = pInfernalPlayer.initUnit(git("UNIT_HYBOREM"),pBestPlot.getX(),pBestPlot.getY(),UnitAITypes.NO_UNITAI,DirectionTypes.DIRECTION_SOUTH)
				NewUnitHero.setHasPromotion(git("PROMOTION_IMMORTAL"), True)
			if iLeader == git("LEADER_JUDECCA"):
				NewUnitHero = pInfernalPlayer.initUnit(git("UNIT_JUDECCA"),pBestPlot.getX(),pBestPlot.getY(),UnitAITypes.NO_UNITAI,DirectionTypes.DIRECTION_SOUTH)
				NewUnitHero.setHasPromotion(git("PROMOTION_IMMORTAL"), True)
			if iLeader == git("LEADER_LETHE"):
				NewUnitHero = pInfernalPlayer.initUnit(git("UNIT_LETHE"),pBestPlot.getX(),pBestPlot.getY(),UnitAITypes.NO_UNITAI,DirectionTypes.DIRECTION_SOUTH)
				NewUnitHero.setHasPromotion(git("PROMOTION_IMMORTAL"), True)
			if iLeader == git("LEADER_MERESIN"):
				NewUnitHero = pInfernalPlayer.initUnit(git("UNIT_MERESIN"),pBestPlot.getX(),pBestPlot.getY(),UnitAITypes.NO_UNITAI,DirectionTypes.DIRECTION_SOUTH)
				NewUnitHero.setHasPromotion(git("PROMOTION_IMMORTAL"), True)
			if iLeader == git("LEADER_OUZZA"):
				NewUnitHero = pInfernalPlayer.initUnit(git("UNIT_OUZZA"),pBestPlot.getX(),pBestPlot.getY(),UnitAITypes.NO_UNITAI,DirectionTypes.DIRECTION_SOUTH)
				NewUnitHero.setHasPromotion(git("PROMOTION_IMMORTAL"), True)
			if iLeader == git("LEADER_SALLOS"):
				NewUnitHero = pInfernalPlayer.initUnit(git("UNIT_SALLOS"),pBestPlot.getX(),pBestPlot.getY(),UnitAITypes.NO_UNITAI,DirectionTypes.DIRECTION_SOUTH)
				NewUnitHero.setHasPromotion(git("PROMOTION_IMMORTAL"), True)
			if iLeader == git("LEADER_STATIUS"):
				NewUnitHero = pInfernalPlayer.initUnit(git("UNIT_STATIUS"),pBestPlot.getX(),pBestPlot.getY(),UnitAITypes.NO_UNITAI,DirectionTypes.DIRECTION_SOUTH)
				NewUnitHero.setHasPromotion(git("PROMOTION_IMMORTAL"), True)
				
			# Common spawn between demons
			NewUnitArcher1	= pInfernalPlayer.initUnit(git("UNIT_LONGBOWMAN"),pBestPlot.getX(),pBestPlot.getY(),UnitAITypes.NO_UNITAI,DirectionTypes.DIRECTION_SOUTH)
			NewUnitArcher2	= pInfernalPlayer.initUnit(git("UNIT_LONGBOWMAN"),pBestPlot.getX(),pBestPlot.getY(),UnitAITypes.NO_UNITAI,DirectionTypes.DIRECTION_SOUTH)
			NewUnitChamp1	= pInfernalPlayer.initUnit(git("UNIT_SECT_OF_FLIES"),pBestPlot.getX(),pBestPlot.getY(),UnitAITypes.NO_UNITAI,DirectionTypes.DIRECTION_SOUTH)
			NewUnitChamp2	= pInfernalPlayer.initUnit(git("UNIT_SECT_OF_FLIES"),pBestPlot.getX(),pBestPlot.getY(),UnitAITypes.NO_UNITAI,DirectionTypes.DIRECTION_SOUTH)
			NewUnitWorker	= pInfernalPlayer.initUnit(git("UNIT_WORKER"),pBestPlot.getX(),pBestPlot.getY(),UnitAITypes.NO_UNITAI,DirectionTypes.DIRECTION_SOUTH)
			NewUnitImp		= pInfernalPlayer.initUnit(git("UNIT_IMP"),pBestPlot.getX(),pBestPlot.getY(),UnitAITypes.NO_UNITAI,DirectionTypes.DIRECTION_SOUTH)
			NewUnitMane1	= pInfernalPlayer.initUnit(git("UNIT_MANES"),pBestPlot.getX(),pBestPlot.getY(),UnitAITypes.NO_UNITAI,DirectionTypes.DIRECTION_SOUTH)
			NewUnitMane2	= pInfernalPlayer.initUnit(git("UNIT_MANES"),pBestPlot.getX(),pBestPlot.getY(),UnitAITypes.NO_UNITAI,DirectionTypes.DIRECTION_SOUTH)
			NewUnitMane3	= pInfernalPlayer.initUnit(git("UNIT_MANES"),pBestPlot.getX(),pBestPlot.getY(),UnitAITypes.NO_UNITAI,DirectionTypes.DIRECTION_SOUTH)
			NewUnitSettler1	= pInfernalPlayer.initUnit(git("UNIT_SETTLER"),pBestPlot.getX(),pBestPlot.getY(),UnitAITypes.NO_UNITAI,DirectionTypes.DIRECTION_SOUTH)
			NewUnitSettler2	= pInfernalPlayer.initUnit(git("UNIT_SETTLER"),pBestPlot.getX(),pBestPlot.getY(),UnitAITypes.NO_UNITAI,DirectionTypes.DIRECTION_SOUTH)
			NewUnitArcher1.setHasPromotion(git("PROMOTION_MOBILITY1"), True)
			NewUnitArcher2.setHasPromotion(git("PROMOTION_MOBILITY1"), True)
			NewUnitChamp1.setHasPromotion(git("PROMOTION_MOBILITY1"), True)
			NewUnitChamp1.setHasPromotion(git("PROMOTION_IRON_WEAPONS"), True)
			NewUnitChamp2.setHasPromotion(git("PROMOTION_MOBILITY1"), True)
			NewUnitChamp2.setHasPromotion(git("PROMOTION_IRON_WEAPONS"), True)
			NewUnitImp.setHasPromotion(git("PROMOTION_MOBILITY1"), True)
			NewUnitSettler1.setHasPromotion(git("PROMOTION_STARTING_SETTLER"), True)
			NewUnitSettler2.setHasPromotion(git("PROMOTION_STARTING_SETTLER"), True)
			for iTech in xrange(gc.getNumTechInfos()):
				if eFounderTeam.isHasTech(iTech):
					eInfernalTeam.setHasTech(iTech, True, iInfernalPlayer, True, False)
			eFounderTeam.signOpenBorders(iInfernalTeam)
			eInfernalTeam.signOpenBorders(iFounderTeam)
			eInfernalTeam.makePeace(iDemonTeam)
			for iTeam in xrange(gc.getMAX_TEAMS()):
				if iTeam != iDemonTeam:
					pTeam = gc.getTeam(iTeam)
					if pTeam.isAlive():
						if eFounderTeam.isAtWar(iTeam) or pTeam.isBarbarian():
							eInfernalTeam.declareWar(iTeam, False, WarPlanTypes.WARPLAN_LIMITED)
			pInfernalPlayer.AI_changeAttitudeExtra(iPlayer,4)
			
			if bReassign:
				CyMessageControl().sendModNetMessage(CvUtil.reassignPlayer, iPlayer, iInfernalPlayer, 0, 0)
				
	# Check if equipment can`t be removed
	def canRemoveEquipment(self,pHolder,pTaker,iPromotion):
		gc				= CyGlobalContext()
		git				= gc.getInfoTypeForString
		if pHolder != -1:
			if pHolder.getUnitType() == git('UNIT_BARNAXUS'):
				if iPromotion == git('PROMOTION_PIECES_OF_BARNAXUS'):
					return False
			elif pHolder.getUnitType() == git('UNIT_THE_HIVE'):
				if iPromotion == git('PROMOTION_PIECES_OF_THE_HIVE'):
					return False
			elif pHolder.getUnitType() == git('UNIT_MITHRIL_GOLEM'):
				if iPromotion == git('PROMOTION_PIECES_OF_MITHRIL_GOLEM'):
					return False
			elif pHolder.getUnitType() == git('UNIT_WAR_MACHINE'):
				if iPromotion == git('PROMOTION_PIECES_OF_WAR_MACHINE'):
					return False
			elif pHolder.getUnitType() == git('UNIT_MECH_DRAGON'):
				if iPromotion == git('PROMOTION_PIECES_OF_MECH_DRAGON'):
					return False
			if pHolder.isHasPromotion(git('PROMOTION_ILLUSION')):
				return False
			if pHolder.isHasPromotion(git('PROMOTION_BAIR_GEM_RECHARGING')):
				if iPromotion == git('PROMOTION_BAIR_GEM_RECHARGING'):
					return False
		if pTaker != -1:
			if pTaker.getUnitCombatType() == git('UNITCOMBAT_NAVAL') or pTaker.getUnitCombatType() == git('UNITCOMBAT_SIEGE'):
				return False
			if pTaker.getSpecialUnitType() == git('SPECIALUNIT_SPELL') or pTaker.getSpecialUnitType() == git('SPECIALUNIT_BIRD'):
				return False
			if pTaker.isHasPromotion(git('PROMOTION_ILLUSION')):
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
				if not pStartingPlot.isNone():
					lStartingPlots.append(pStartingPlot)

		# Early exits everywhere to shave some time
		for iPlot in xrange(CyMap().numPlots()):
			pLoopPlot = CyMap().plotByIndex(iPlot)
			bValid = True
			iValue		= 0
			iAIValue	= 0
			iDist		= 0

			# Checks that are more likely to filter out a plot are higher
			if pLoopPlot.isWater():
				continue

			elif pLoopPlot.isImpassable():
				continue

			elif pLoopPlot.isPeak():
				continue

			elif pLoopPlot.getBonusType(-1) != -1:
				continue

			elif pLoopPlot.getImprovementType() != -1:
				continue

			elif pLoopPlot.isCity():
				continue

			elif pLoopPlot.isFoundDisabled():
				continue

			iAIValue += pBarbPlayer.AI_foundValue(pLoopPlot.getX(), pLoopPlot.getY(), -1, true)

			if iAIValue == 0:
				continue

			for StartingPlot in lStartingPlots:
				iDist = CyMap().calculatePathDistance(pLoopPlot,StartingPlot)

				if iDist == -1:
					iValue += 100

				elif iDist < 7:
					bValid = False
					break

				# Without a cap on distance points, every other point increase becomes irrelevant on larger maps.
				else:
					iValue += min(iDist*5, 100)

			if not bValid:
				continue

			iValue += iAIValue

			iValue += CyGame().getSorenRandNum(250, "Barb World Plot Eval")

			lValuedPlots.append((iPlot,iValue))

		if lValuedPlots:
			# Sorting plots by iValue
			lValuedPlots.sort(key=lambda tup: tup[1], reverse=True)

			# Placing cities
			for i in xrange(iNumCities):

				iDist = 0

				if len(lValuedPlots) == 0:
					break

				iCityPlot = lValuedPlots[0][0]
				pCityPlot = CyMap().plotByIndex(iCityPlot)
				pBarbPlayer.found(pCityPlot.getX(), pCityPlot.getY())

				del lValuedPlots[0]

				# As plots are already valued, we need to remove plots that are too close to the placed city.
				for tLoopPlot in list(lValuedPlots):
					pLoopPlot = CyMap().plotByIndex(tLoopPlot[0])
					iDist = CyMap().calculatePathDistance(pCityPlot,pLoopPlot) 
					if iDist > -1 and iDist < 7:
						lValuedPlots.remove(tLoopPlot)
