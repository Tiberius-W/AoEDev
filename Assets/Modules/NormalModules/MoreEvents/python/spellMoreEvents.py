## MODULAR PYTHON FOR THE MEKARA ORDER MODULE
## Eric 'Jheral' Lindroth
## Last Edited: 23-june-2010
## ericl87@hotmail.com

from CvPythonExtensions import *
import PyHelpers
import CvEventInterface
import CvUtil
import CvScreensInterface
from BasicFunctions import *
import CvSpellInterface

PyPlayer = PyHelpers.PyPlayer
gc = CyGlobalContext()
getInfoType = gc.getInfoTypeForString
localText = CyTranslator()

def doSpawnUndead (pPlot):

	bDemonPlayer 	= gc.getPlayer(gc.getDEMON_PLAYER())
	iX = pPlot.getX()
	iY = pPlot.getY()
	iSkeleton = getInfoType('UNIT_SKELETON')
	iSpectre = getInfoType('UNIT_SPECTRE')
	for iiX in range(iX-2, iX+3, 1):
		for iiY in range(iY-2, iY+3, 1):
			pPlot2 = CyMap().plot(iiX,iiY)
			if not pPlot2.isWater():
				if pPlot2.getNumUnits() == 0:
					if not pPlot2.isCity():
						Rand = CyGame().getSorenRandNum(500, "spawn Undead")
						if Rand <= 300:
							newUnit = bDemonPlayer.initUnit(iSkeleton, pPlot2.getX(), pPlot2.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
						else :
							if Rand<=400:
								newUnit = bDemonPlayer.initUnit(iSpectre, pPlot2.getX(), pPlot2.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)

def doSpawnDemon (pPlot):

	bDemonPlayer 	= gc.getPlayer(gc.getDEMON_PLAYER())
	iX = pPlot.getX()
	iY = pPlot.getY()
	iChampion = getInfoType('UNIT_CHAMPION')
	iBalor = getInfoType('UNIT_BALOR')
	iDemon=getInfoType('PROMOTION_DEMON')
	for iiX in range(iX-2, iX+3, 1):
		for iiY in range(iY-2, iY+3, 1):
			pPlot2 = CyMap().plot(iiX,iiY)
			if not pPlot2.isWater():
				if pPlot2.getNumUnits() == 0:
					if not pPlot2.isCity():
						Rand = CyGame().getSorenRandNum(500, "spawn Undead")
						if Rand <= 300:
							newUnit = bDemonPlayer.initUnit(iChampion, pPlot2.getX(), pPlot2.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
							newUnit.setHasPromotion(iDemon, True)
						else :
							if Rand<=325:
								newUnit = bDemonPlayer.initUnit(iBalor, pPlot2.getX(), pPlot2.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)

def doSpawnOrc (pPlot):

	bDemonPlayer 	= gc.getPlayer(gc.getORC_PLAYER())
	iX = pPlot.getX()
	iY = pPlot.getY()
	iChampion = getInfoType('UNIT_CHAMPION')
	iAxeman = getInfoType('UNIT_AXEMAN')
	iOrc=getInfoType('PROMOTION_ORC')
	for iiX in range(iX-2, iX+3, 1):
		for iiY in range(iY-2, iY+3, 1):
			pPlot2 = CyMap().plot(iiX,iiY)
			if not pPlot2.isWater():
				if pPlot2.getNumUnits() == 0:
					if not pPlot2.isCity():
						Rand = CyGame().getSorenRandNum(500, "spawn Undead")
						if Rand <= 300:

							newUnit = bDemonPlayer.initUnit(iAxeman, pPlot2.getX(), pPlot2.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
							newUnit.setHasPromotion(iOrc, True)
						else :
							if Rand<=400:
								newUnit = bDemonPlayer.initUnit(iChampion, pPlot2.getX(), pPlot2.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
								newUnit.setHasPromotion(iOrc, True)

def doSpawnDoviello (pPlot):

	bDemonPlayer 	= gc.getPlayer(gc.getORC_PLAYER())
	iX = pPlot.getX()
	iY = pPlot.getY()
	iChampion = getInfoType('UNIT_BATTLEMASTER')
	iAxeman = getInfoType('UNIT_SONS_OF_ASENA')
	for iiX in range(iX-2, iX+3, 1):
		for iiY in range(iY-2, iY+3, 1):
			pPlot2 = CyMap().plot(iiX,iiY)
			if not pPlot2.isWater():
				if pPlot2.getNumUnits() == 0:
					if not pPlot2.isCity():
						Rand = CyGame().getSorenRandNum(500, "spawn Undead")
						if Rand <= 300:
							newUnit = bDemonPlayer.initUnit(iAxeman, pPlot2.getX(), pPlot2.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
						else :
							if Rand<=400:
								newUnit = bDemonPlayer.initUnit(iChampion, pPlot2.getX(), pPlot2.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)

def doSpawnLizard (pPlot):

	bDemonPlayer 	= gc.getPlayer(gc.getORC_PLAYER())
	iX = pPlot.getX()
	iY = pPlot.getY()
	iChampion = getInfoType('UNIT_CHAMPION')
	iBlowpipe = getInfoType('UNIT_LIZARD_BLOWPIPE')
	iLizard=getInfoType('PROMOTION_LIZARDMAN')
	for iiX in range(iX-2, iX+3, 1):
		for iiY in range(iY-2, iY+3, 1):
			pPlot2 = CyMap().plot(iiX,iiY)
			if not pPlot2.isWater():
				if pPlot2.getNumUnits() == 0:
					if not pPlot2.isCity():
						Rand = CyGame().getSorenRandNum(500, "spawn Undead")
						if Rand <= 300:
							newUnit = bDemonPlayer.initUnit(iBlowpipe, pPlot2.getX(), pPlot2.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
						else :
							if Rand<=400:
								newUnit = bDemonPlayer.initUnit(iChampion, pPlot2.getX(), pPlot2.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
								newUnit.setHasPromotion(iLizard, True)
def doSpawnBeast(pPlot):

	pDemonPlayer 	= gc.getPlayer(gc.getANIMAL_PLAYER())
	iX = pPlot.getX()
	iY = pPlot.getY()
	iBear = getInfoType('UNIT_CAVE_BEARS')
	iLion = getInfoType('UNIT_LION')
	for iiX in range(iX-2, iX+3, 1):
		for iiY in range(iY-2, iY+3, 1):
			pPlot2 = CyMap().plot(iiX,iiY)
			if not pPlot2.isWater():
				if pPlot2.getNumUnits() == 0:
					if not pPlot2.isCity():
						Rand = CyGame().getSorenRandNum(500, "spawn Undead")
						if Rand <= 300:

							newUnit = pDemonPlayer.initUnit(iLion, pPlot2.getX(), pPlot2.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
						else :
							if Rand<=350:
								newUnit = pDemonPlayer.initUnit(iBear, pPlot2.getX(), pPlot2.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)


def doSpawnWaterBeast(pPlot):

	bDemonPlayer 	= gc.getPlayer(gc.getANIMAL_PLAYER())
	iX = pPlot.getX()
	iY = pPlot.getY()
	iBunyip = getInfoType('UNIT_BUNYIP')
	iSeaSerpent = getInfoType('UNIT_SEA_SERPENT')
	for iiX in range(iX-2, iX+3, 1):
		for iiY in range(iY-2, iY+3, 1):
			pPlot2 = CyMap().plot(iiX,iiY)
			if pPlot2.isWater():
				if pPlot2.getNumUnits() == 0:
					Rand = CyGame().getSorenRandNum(500, "spawn Undead")
					if Rand <= 150:
						newUnit = bDemonPlayer.initUnit(iSeaSerpent, pPlot2.getX(), pPlot2.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)

def doSpawnWaterUndead(pPlot):

	bDemonPlayer 	= gc.getPlayer(gc.getDEMON_PLAYER())
	iX = pPlot.getX()
	iY = pPlot.getY()
	iDrown = getInfoType('UNIT_DROWN')
	iStygian = getInfoType('UNIT_STYGIAN_GUARD')
	for iiX in range(iX-2, iX+3, 1):
		for iiY in range(iY-2, iY+3, 1):
			pPlot2 = CyMap().plot(iiX,iiY)
			if pPlot2.isWater():
				if pPlot2.getNumUnits() == 0:
					Rand = CyGame().getSorenRandNum(500, "spawn Undead")
					if Rand <= 300:
						bValid = True
						if bValid :
							newUnit = bDemonPlayer.initUnit(iDrown, pPlot2.getX(), pPlot2.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
					else :
						if Rand<=350:
							newUnit = bDemonPlayer.initUnit(iStygian, pPlot2.getX(), pPlot2.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)

def onMoveAifonIsle(pCaster, pPlot):
	iPlayer			= pCaster.getOwner()
	pPlayer			= gc.getPlayer(iPlayer)
	iImprovement	= pPlot.getImprovementType()
	if pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_AIFON_ISLE')):
		doTreasureHunterNewSearch(iPlayer,iImprovement)

def onMoveBairofLacuna(pCaster, pPlot):
	iPlayer			= pCaster.getOwner()
	pPlayer			= gc.getPlayer(iPlayer)
	iImprovement	= pPlot.getImprovementType()
	if pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_BAIR_OF_LACUNA')):
		doTreasureHunterNewSearch(iPlayer,iImprovement)

def onMoveBrokenSepulcher(pCaster, pPlot):
	iPlayer			= pCaster.getOwner()
	pPlayer			= gc.getPlayer(iPlayer)
	iImprovement	= pPlot.getImprovementType()
	iCaster			= pCaster.getID()
	if pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_BROKEN_SEPULCHER')):
		doTreasureHunterNewSearch(iPlayer,iImprovement)
		doSpawnUndead(pPlot)
	if ((pCaster.isHasPromotion(getInfoType('PROMOTION_GELA'))) and (not (pPlayer.getCivilizationType() == getInfoType("CIVILIZATION_INFERNAL"))) and (pPlayer.getAlignment() == getInfoType('ALIGNMENT_EVIL')) ):
		if pPlayer.isHuman():
			popupInfo = CyPopupInfo()
			popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON)
			popupInfo.setOnClickedPythonCallback("effectHumanGelaImprovement")
			popupInfo.setData1(iCaster)
			popupInfo.setData2(iPlayer)
			popupInfo.setData3(iImprovement)
			popupInfo.setText(CyTranslator().getText("TXT_KEY_EVENT_GELA_BROKEN_SEPULCHER", ()))
			popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_YES", ()),"")
			popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_NO", ()),"")
			popupInfo.addPopup(iPlayer)
		else:
			argsList = [0,iCaster,iPlayer,iImprovement]
			effectGelaImprovement(argsList)

def onMoveDragonBones(pCaster, pPlot):
	iPlayer			= pCaster.getOwner()
	pPlayer			= gc.getPlayer(iPlayer)
	iImprovement	= pPlot.getImprovementType()
	if pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_DRAGON_BONES')):
		doTreasureHunterNewSearch(iPlayer,iImprovement)

def onMoveMountKalshekk(pCaster, pPlot):
	iPlayer			= pCaster.getOwner()
	pPlayer			= gc.getPlayer(iPlayer)
	iImprovement	= pPlot.getImprovementType()
	if pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_MOUNT_KALSHEKK')):
		doTreasureHunterNewSearch(iPlayer,iImprovement)
		doSpawnLizard(pPlot)

def onMovePyreofTheSeraphic(pCaster, pPlot):
	iPlayer			= pCaster.getOwner()
	pPlayer			= gc.getPlayer(iPlayer)
	iImprovement	= pPlot.getImprovementType()
	iCaster			= pCaster.getID()
	if pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_PYRE_OF_THE_SERAPHIC')):
		doTreasureHunterNewSearch(iPlayer,iImprovement)
		doSpawnOrc(pPlot)
	if ((pCaster.isHasPromotion(getInfoType('PROMOTION_GELA'))) and (not (pPlayer.getCivilizationType() == getInfoType("CIVILIZATION_INFERNAL")))):
		if pPlayer.isHuman():
			popupInfo = CyPopupInfo()
			popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON)
			popupInfo.setOnClickedPythonCallback("effectHumanGelaImprovement")
			popupInfo.setData1(iCaster)
			popupInfo.setData2(iPlayer)
			popupInfo.setData3(iImprovement)
			popupInfo.setText(CyTranslator().getText("TXT_KEY_EVENT_GELA_PYRE_OF_THE_SERAPHIC", ()))
			popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_YES", ()),"")
			popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_NO", ()),"")
			popupInfo.addPopup(iPlayer)
		else:
			argsList = [0,iCaster,iPlayer,iImprovement]
			effectGelaImprovement(argsList)

def onMoveRinwell(pCaster, pPlot):
	iPlayer			= pCaster.getOwner()
	pPlayer			= gc.getPlayer(iPlayer)
	iImprovement	= pPlot.getImprovementType()
	if pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_RINWELL')):
		doTreasureHunterNewSearch(iPlayer,iImprovement)

def onMoveSevenPines(pCaster, pPlot):
	iPlayer			= pCaster.getOwner()
	pPlayer			= gc.getPlayer(iPlayer)
	iImprovement	= pPlot.getImprovementType()
	if pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_SEVEN_PINES')):
		doTreasureHunterNewSearch(iPlayer,iImprovement)

def onMoveStandingStones(pCaster, pPlot):
	iPlayer			= pCaster.getOwner()
	pPlayer			= gc.getPlayer(iPlayer)
	iImprovement	= pPlot.getImprovementType()
	if pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_STANDING_STONES')):
		doTreasureHunterNewSearch(iPlayer,iImprovement)

def onMoveTowerofEyes(pCaster, pPlot):
	iPlayer			= pCaster.getOwner()
	pPlayer			= gc.getPlayer(iPlayer)
	iImprovement	= pPlot.getImprovementType()
	if pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_TOWER_OF_EYES')):
		doTreasureHunterNewSearch(iPlayer,iImprovement)

def onMoveTombOfSucellus2(pCaster, pPlot):
	iPlayer			= pCaster.getOwner()
	pPlayer			= gc.getPlayer(iPlayer)
	iImprovement	= pPlot.getImprovementType()
	if pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_TOMB_OF_SUCELLUS')):
		doTreasureHunterNewSearch(iPlayer,iImprovement)

def onMoveYggdrasil(pCaster, pPlot):
	iPlayer			= pCaster.getOwner()
	pPlayer			= gc.getPlayer(iPlayer)
	iImprovement	= pPlot.getImprovementType()
	if pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_YGGDRASIL')):
		doTreasureHunterNewSearch(iPlayer,iImprovement)
		doSpawnBeast(pPlot)

def onMoveLetumFrigus2(pCaster, pPlot):
	iPlayer			= pCaster.getOwner()
	pPlayer			= gc.getPlayer(iPlayer)
	git				= gc.getInfoTypeForString
	iImprovement	= pPlot.getImprovementType()
	if pPlayer.isHasFlag(git('FLAG_TREASURE_HUNTER_LETUM_FRIGUS')):
		doTreasureHunterNewSearch(iPlayer,iImprovement)
	elif not pPlayer.isHasFlag(git("FLAG_LETUM_FRIGUS_FIRST_TIME")):
		pPlayer.setHasFlag(git("FLAG_LETUM_FRIGUS_FIRST_TIME"), True)
		CyInterface().addMessage(pCaster.getOwner(),True,25,CyTranslator().getText("TXT_KEY_EVENT_LETUM_FRIGUS", ()),'',3,'Art/Interface/Buttons/Improvements/letumfrigus.dds',ColorTypes(8),pPlot.getX(),pPlot.getY(),True,True)
		if pPlayer.getCivilizationType() == git("CIVILIZATION_ILLIANS"):
			CyInterface().addMessage(pCaster.getOwner(),True,25,CyTranslator().getText("TXT_KEY_EVENT_LETUM_FRIGUS_3_HELP", ()),'',3,'Art/Interface/Buttons/Improvements/letumfrigus.dds',ColorTypes(8),pPlot.getX(),pPlot.getY(),True,True)
			CyInterface().addMessage(pCaster.getOwner(),True,25,CyTranslator().getText("TXT_KEY_EVENT_LETUM_FRIGUS_3", ()),'',3,'Art/Interface/Buttons/Improvements/letumfrigus.dds',ColorTypes(8),pPlot.getX(),pPlot.getY(),True,True)
			if not gc.isNoCrash():
				pPlayer.setHasTrait(git('TRAIT_AGGRESSIVE'),True,-1,True,True)
			else:
				pPlayer.setHasTrait(git('TRAIT_AGGRESSIVE'),True)
		elif pPlayer.getCivilizationType() == git("CIVILIZATION_AMURITES"):
			CyInterface().addMessage(pCaster.getOwner(),True,25,CyTranslator().getText("TXT_KEY_EVENT_LETUM_FRIGUS_2", ()),'',3,'Art/Interface/Buttons/Improvements/letumfrigus.dds',ColorTypes(8),pPlot.getX(),pPlot.getY(),True,True)
			pPlayer.changeGoldenAgeTurns(CyGame().goldenAgeLength())

def onMovePoolOfTears2(pCaster, pPlot):
	iPlayer			= pCaster.getOwner()
	pPlayer			= gc.getPlayer(iPlayer)
	git				= gc.getInfoTypeForString
	iImprovement	= pPlot.getImprovementType()
	lIllness		= [git("PROMOTION_DISEASED"),git("PROMOTION_PLAGUED"),git("PROMOTION_POISONED"),git("PROMOTION_WITHERED")]
	for iPromotion in lIllness:
		if pCaster.isHasPromotion(iPromotion):
			pCaster.setHasPromotion(iPromotion, false)
			CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_POOL_OF_TEARS_CURED",(gc.getUnitInfo(pCaster.getUnitType()).getTextKey(),gc.getPromotionInfo(iPromotion).getTextKey())),'AS2D_FEATUREGROWTH',3,'Art/Interface/Buttons/Improvements/pooloftears.dds',ColorTypes(8),pCaster.getX(),pCaster.getY(),True,True)
	if pPlayer.isHasFlag(git('FLAG_TREASURE_HUNTER_POOL_OF_TEARS')):
		doTreasureHunterNewSearch(iPlayer,iImprovement)
	elif ((pCaster.isHasPromotion(getInfoType('PROMOTION_GELA'))) and (not (pPlayer.getCivilizationType() == getInfoType("CIVILIZATION_INFERNAL"))) ):
		if pPlayer.isHuman():
			popupInfo = CyPopupInfo()
			popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON)
			popupInfo.setOnClickedPythonCallback("effectHumanGelaImprovement")
			popupInfo.setData1(iCaster)
			popupInfo.setData2(iPlayer)
			popupInfo.setData3(iImprovement)
			popupInfo.setText(CyTranslator().getText("TXT_KEY_EVENT_GELA_POOL_OF_TEARS", ()))
			popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_YES", ()),"")
			popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_NO", ()),"")
			popupInfo.addPopup(iPlayer)
		else:
			argsList = [0,iCaster,iPlayer,iImprovement]
			effectGelaImprovement(argsList)

def onMoveSironasBeacon2(pCaster, pPlot):
	iPlayer			= pCaster.getOwner()
	pPlayer			= gc.getPlayer(iPlayer)
	iImprovement	= pPlot.getImprovementType()
	if pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_SIRONAS_BEACON')):
		doTreasureHunterNewSearch(iPlayer,iImprovement)

def onMoveMirrorOfHeaven2(pCaster, pPlot):
	iPlayer			= pCaster.getOwner()
	pPlayer			= gc.getPlayer(iPlayer)
	iImprovement	= pPlot.getImprovementType()
	iCaster			= pCaster.getID()
	pCapital		= pPlayer.getCapitalCity()
	if pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_MIRROR_OF_HEAVEN')):
		doTreasureHunterNewSearch(iPlayer,iImprovement)
	elif not pCapital.isNone() and pPlayer.isFeatAccomplished(FeatTypes.FEAT_VISIT_MIRROR_OF_HEAVEN) == False and pPlayer.getCivilizationType() == getInfoType("CIVILIZATION_MALAKIM"):
		pPlayer.setFeatAccomplished(FeatTypes.FEAT_VISIT_MIRROR_OF_HEAVEN, True) # If set locally in another function, popups for human players will queue up
		if pPlayer.isHuman():
			popupInfo = CyPopupInfo()
			popupInfo.setOption2(True)
			popupInfo.setFlags(126)
			popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON)
			popupInfo.setOnClickedPythonCallback("passToModNetMessage")
			popupInfo.setData1(iPlayer)
			popupInfo.setData3(105) # onModNetMessage id
			popupInfo.setText(CyTranslator().getText("TXT_KEY_EVENT_MALAKIM_MIRROR", ()))
			popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_MALAKIM_MIRROR_1", ()),"EVENT_MALAKIM_MIRROR_1")
			popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_MALAKIM_MIRROR_2", ()),"EVENT_MALAKIM_MIRROR_2")
			popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_MALAKIM_MIRROR_3", ()),"EVENT_MALAKIM_MIRROR_3")
			popupInfo.addPopup(iPlayer)
		else:
			AIPick = CyGame().getSorenRandNum(2, "MalakimMirror AI pick") + 1
			argsList = [AIPick,iPlayer]
			CvSpellInterface.effectMalakimMirror(argsList) ### TODO CHECK CALL FROM CORE SPELL FILE
	elif ((pCaster.isHasPromotion(getInfoType('PROMOTION_GELA'))) and (not (pPlayer.getCivilizationType() == getInfoType("CIVILIZATION_INFERNAL"))) and (pPlayer.getAlignment() == getInfoType('ALIGNMENT_GOOD')) ):
		if pPlayer.isHuman():
			popupInfo = CyPopupInfo()
			popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON)
			popupInfo.setOnClickedPythonCallback("effectHumanGelaImprovement")
			popupInfo.setData1(iCaster)
			popupInfo.setData2(iPlayer)
			popupInfo.setData3(iImprovement)
			popupInfo.setText(CyTranslator().getText("TXT_KEY_EVENT_GELA_MIRROR_OF_HEAVEN", ()))
			popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_YES", ()),"")
			popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_NO", ()),"")
			popupInfo.addPopup(iPlayer)
		else:
			argsList = [0,iCaster,iPlayer,iImprovement]
			effectGelaImprovement(argsList)

def onMoveRemnantsOfPatria2(pCaster, pPlot):
	iPlayer			= pCaster.getOwner()
	pPlayer			= gc.getPlayer(iPlayer)
	git				= gc.getInfoTypeForString
	iImprovement	= pPlot.getImprovementType()
	if pPlayer.isHasFlag(git('FLAG_TREASURE_HUNTER_REMNANTS_OF_PATRIA')):
		doTreasureHunterNewSearch(iPlayer,iImprovement)
	elif pPlayer.isFeatAccomplished(FeatTypes.FEAT_VISIT_REMNANTS_OF_PATRIA) == False:
		if pPlayer.getCivilizationType() == git("CIVILIZATION_SCIONS"):
			pPlayer.setFeatAccomplished(FeatTypes.FEAT_VISIT_REMNANTS_OF_PATRIA, True)
			newUnit1 = pPlayer.initUnit(git('UNIT_SUPPLIES'), pPlot.getX(), pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
			newUnit2 = pPlayer.initUnit(git('UNIT_SUPPLIES'), pPlot.getX(), pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
			CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_EVENT_REMNANTS_OF_PATRIA_SCIONS", ()),'',3,"Art/Interface/Buttons/Improvements/remnantsofpatria.dds",git("COLOR_GREEN"),pPlot.getX(),pPlot.getY(),True,True)

def onMoveOdiosPrison2(pCaster, pPlot):
	iPlayer			= pCaster.getOwner()
	pPlayer			= gc.getPlayer(iPlayer)
	iImprovement	= pPlot.getImprovementType()
	if pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_ODIOS_PRISON')):
		doTreasureHunterNewSearch(iPlayer,iImprovement)

def onMoveBradelinesWell2(pCaster, pPlot):
	iPlayer			= pCaster.getOwner()
	pPlayer			= gc.getPlayer(iPlayer)
	iImprovement	= pPlot.getImprovementType()
	if pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_BRADELINES_WELL')):
		doTreasureHunterNewSearch(iPlayer,iImprovement)
def onMoveFoxford2(pCaster, pPlot):
	iPlayer			= pCaster.getOwner()
	pPlayer			= gc.getPlayer(iPlayer)
	iImprovement	= pPlot.getImprovementType()
	git				= gc.getInfoTypeForString
	iCaster			= pCaster.getID()
	pDemonPlayer	= gc.getPlayer(gc.getDEMON_PLAYER())
	if pPlayer.isHasFlag(git('FLAG_TREASURE_HUNTER_FOXFORD')):
		doTreasureHunterNewSearch(iPlayer,iImprovement)
	elif not pDemonPlayer.isHasFlag(git("FLAG_FOXFORD_FIRST_TIME")): # Flag is set globally, and even if a new player is presented to the game, the flag value for the demon player would be right
		if not pPlayer.isBarbarian():
			gc.getGame().setGlobalFlag(git("FLAG_FOXFORD_FIRST_TIME"),True) # If set locally in another function, popups for human players will queue up
			if pPlayer.isHuman():
				popupInfo = CyPopupInfo()
				popupInfo.setOption2(True)
				popupInfo.setFlags(126)
				popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON)
				popupInfo.setOnClickedPythonCallback("passToModNetMessage")
				popupInfo.setData1(iCaster)
				popupInfo.setData2(iPlayer)
				popupInfo.setData3(106)
				popupInfo.setText(CyTranslator().getText("TXT_KEY_EVENT_FOXFORD", ()))
				popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_FOXFORD_1", ()),"EVENT_FOXFORD_1")
				popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_FOXFORD_2", ()),"EVENT_FOXFORD_2")
				popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_FOXFORD_3", ()),"EVENT_FOXFORD_3")
				popupInfo.addPopup(iPlayer)
			else:
				if pPlayer.getAlignment() == git("ALIGNMENT_EVIL") or pPlayer.getLeaderType() == git("LEADER_DUIN"):
					iButtonId = 1
				else:
					iButtonId = 0
				argsList = [iButtonId,iCaster,iPlayer]
				CvSpellInterface.effectFoxford(argsList) ### TODO CHECK CALL FROM CORE SPELL FILE

def onMoveMaelstrom2(pCaster, pPlot):
	iPlayer			= pCaster.getOwner()
	pPlayer			= gc.getPlayer(iPlayer)
	iCaster			= pCaster.getID()
	iImprovement	= pPlot.getImprovementType()
	if ((pCaster.isHasPromotion(getInfoType('PROMOTION_GELA'))) and (not (pPlayer.getCivilizationType() == getInfoType("CIVILIZATION_INFERNAL"))) ):
		if pPlayer.isHuman():
			popupInfo = CyPopupInfo()
			popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON)
			popupInfo.setOnClickedPythonCallback("effectHumanGelaImprovement")
			popupInfo.setData1(iCaster)
			popupInfo.setData2(iPlayer)
			popupInfo.setData3(iImprovement)
			popupInfo.setText(CyTranslator().getText("TXT_KEY_EVENT_GELA_MAELSTROM", ()))
			popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_YES", ()),"")
			popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_NO", ()),"")
			popupInfo.addPopup(iPlayer)
		else:
			argsList = [0,iCaster,iPlayer,iImprovement]
			effectGelaImprovement(argsList)
	else:
		if CyGame().getSorenRandNum(100, "Maelstrom") <= 25:
			CyInterface().addMessage(pCaster.getOwner(),True,25,CyTranslator().getText("TXT_KEY_MESSAGE_MAELSTROM_KILL",()),'AS2D_FEATUREGROWTH',1,'Art/Interface/Buttons/Improvements/Maelstrom.dds',ColorTypes(7),pPlot.getX(),pPlot.getY(),True,True)
			pCaster.kill(True, PlayerTypes.NO_PLAYER)
		else:
			iOcean = getInfoType('TERRAIN_OCEAN')
			iBestValue = 0
			pBestPlot = -1
			for i in range (CyMap().numPlots()):
				iValue = 0
				pTargetPlot = CyMap().plotByIndex(i)
				if pTargetPlot.getTerrainType() == iOcean:
					iValue = CyGame().getSorenRandNum(1000, "Maelstrom")
					if pTargetPlot.isOwned() == false:
						iValue += 1000
					if iValue > iBestValue:
						iBestValue = iValue
						pBestPlot = pTargetPlot
			if pBestPlot != -1:
				pCaster.setXY(pBestPlot.getX(), pBestPlot.getY(), false, True, True)
				pCaster.setDamage(25, PlayerTypes.NO_PLAYER)
				CyInterface().addMessage(pCaster.getOwner(),True,25,CyTranslator().getText("TXT_KEY_MESSAGE_MAELSTROM_MOVE",()),'AS2D_FEATUREGROWTH',1,'Art/Interface/Buttons/Improvements/Maelstrom.dds',ColorTypes(7),pCaster.getX(),pCaster.getY(),True,True)

def postCombatLossOrphanedGoblin(pCaster, pOpponent):
	git			= gc.getInfoTypeForString
	iPlayer		= pOpponent.getOwner()
	pPlayer		= gc.getPlayer(iPlayer)
	iLostPlayer	= pCaster.getOwner()
	pCapital	= pPlayer.getCapitalCity()
	iUnit		= pOpponent.getID()
	iRnd		= CyGame().getSorenRandNum(100,"OrphanedGoblin")
	if iLostPlayer == gc.getORC_PLAYER() and iRnd < 2:
		if pPlayer.getCivilizationType() == git("CIVILIZATION_DTESH"):
			newUnit = pPlayer.initUnit(git('UNIT_SLAVE_UNDEAD'), pCapital.getX(),pCapital.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
			newUnit.setName(CyTranslator().getText("TXT_KEY_EVENT_ORPHANED_GOBLIN_DTESH_SLAVE_NAME", ()))
			CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_EVENT_ORPHANED_GOBLIN_DTESH", ()),'',3,'Art/Interface/Buttons/Promotions/Races/Goblin.dds',ColorTypes(8),pCapital.getX(),pCapital.getY(),True,True)
		elif pPlayer.isHuman():
			popupInfo	= CyPopupInfo()
			popupInfo.setOption2(True)
			popupInfo.setFlags(126)
			popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON)
			popupInfo.setOnClickedPythonCallback("passToModNetMessage")
			popupInfo.setData1(iPlayer)
			popupInfo.setData2(iUnit)
			popupInfo.setData3(120) # onModNetMessage id
			popupInfo.setText(CyTranslator().getText("TXT_KEY_EVENT_ORPHANED_GOBLIN", ()))
			popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_ORPHANED_GOBLIN_1", ()),"EVENT_ORPHANED_GOBLIN_1")
			popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_ORPHANED_GOBLIN_2", ()),"EVENT_ORPHANED_GOBLIN_2")
			popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_ORPHANED_GOBLIN_3", ()),"EVENT_ORPHANED_GOBLIN_3")
			popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_ORPHANED_GOBLIN_4", ()),"EVENT_ORPHANED_GOBLIN_4")
			popupInfo.addPopup(iPlayer)
		else:
			if pPlayer.getAlignment() == git("ALIGNMENT_GOOD"):
				AIPick = CyGame().getSorenRandNum(3,"OrphanedGoblin AI pick, Good") + 1
			elif pPlayer.getAlignment() == git("ALIGNMENT_NEUTRAL"):
				AIPick = CyGame().getSorenRandNum(2,"OrphanedGoblin AI pick, Neutral") + 1
			else:
				AIPick = CyGame().getSorenRandNum(3,"OrphanedGoblin AI pick, Evil")
			argsList = [AIPick,iPlayer,iUnit]
			effectOrphanedGoblin(argsList)

def effectOrphanedGoblin(argsList):
	iButtonId		= argsList[0]
	iPlayer			= argsList[1]
	iUnit			= argsList[2]
	git				= gc.getInfoTypeForString
	pPlayer			= gc.getPlayer(iPlayer)
	pUnit			= pPlayer.getUnit(iUnit)
	pPlot			= pUnit.plot()
	iRnd			= CyGame().getSorenRandNum(100,"OrphanedGoblin effect")
	if iButtonId == 0:
		pUnit.setHasPromotion(git('PROMOTION_ORC_SLAYING'), True)
		pUnit.setHasPromotion(git('PROMOTION_CRAZED'), True)
	elif iButtonId == 1:
		pUnit.setHasPromotion(git('PROMOTION_GOBLIN'), True)
	elif iButtonId == 2:
		pNewPlot = findClearPlot(-1, pPlot)
		if pNewPlot != -1:
			if iRnd < 50:
				pGoblinPlayer = gc.getPlayer(gc.getORC_PLAYER())
			else:
				pGoblinPlayer = pPlayer
			newUnit = pGoblinPlayer.initUnit(git('UNIT_GOBLIN'), pNewPlot.getX(),pNewPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
			newUnit.setHasPromotion(git('PROMOTION_WEAK'), True)
	else:
		pUnit.changeExperience(-1,-1,False,False,False)
		newUnit = pPlayer.initUnit(git('UNIT_GOBLIN'), pPlot.getX(),pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)

def doTreasureHunterNewSearch(iPlayer,iSearchedImp):
	git				= gc.getInfoTypeForString
	pPlayer			= gc.getPlayer(iPlayer)
	pHaven			= pPlayer.getCapitalCity()
	# Every dirty list item has a common improvement within an equal index.
	lDirtyImp		= [git("IMPROVEMENT_AIFON_ISLE"),git("IMPROVEMENT_BAIR_OF_LACUNA"),git("IMPROVEMENT_BRADELINES_WELL"),git("IMPROVEMENT_BRADELINES_WELL_PURIFIED"),git("IMPROVEMENT_BROKEN_SEPULCHER"),git("IMPROVEMENT_DRAGON_BONES"),git("IMPROVEMENT_FOXFORD"),git("IMPROVEMENT_LETUM_FRIGUS"),git("IMPROVEMENT_MIRROR_OF_HEAVEN"),git("IMPROVEMENT_MOUNT_KALSHEKK"),git("IMPROVEMENT_ODIOS_PRISON"),git("IMPROVEMENT_POOL_OF_TEARS"),git("IMPROVEMENT_PYRE_OF_THE_SERAPHIC"),git("IMPROVEMENT_RINWELL"),git("IMPROVEMENT_RINWELL2"),git("IMPROVEMENT_RINWELL3"),git("IMPROVEMENT_SEVEN_PINES"),git("IMPROVEMENT_SIRONAS_BEACON"),git("IMPROVEMENT_STANDING_STONES"),git("IMPROVEMENT_TOWER_OF_EYES"),git("IMPROVEMENT_TOMB_OF_SUCELLUS"),git("IMPROVEMENT_YGGDRASIL"),git("IMPROVEMENT_REMNANTS_OF_PATRIA")]
	lDirtyImpFlags	= [git("FLAG_TREASURE_HUNTER_AIFON_ISLE"),git("FLAG_TREASURE_HUNTER_BAIR_OF_LACUNA"),git("FLAG_TREASURE_HUNTER_BRADELINES_WELL"),git("FLAG_TREASURE_HUNTER_BRADELINES_WELL"),git("FLAG_TREASURE_HUNTER_BROKEN_SEPULCHER"),git("FLAG_TREASURE_HUNTER_DRAGON_BONES"),git("FLAG_TREASURE_HUNTER_FOXFORD"),git("FLAG_TREASURE_HUNTER_LETUM_FRIGUS"),git("FLAG_TREASURE_HUNTER_MIRROR_OF_HEAVEN"),git("FLAG_TREASURE_HUNTER_MOUNT_KALSHEKK"),git("FLAG_TREASURE_HUNTER_ODIOS_PRISON"),git("FLAG_TREASURE_HUNTER_POOL_OF_TEARS"),git("FLAG_TREASURE_HUNTER_PYRE_OF_THE_SERAPHIC"),git("FLAG_TREASURE_HUNTER_RINWELL"),git("FLAG_TREASURE_HUNTER_RINWELL"),git("FLAG_TREASURE_HUNTER_RINWELL"),git("FLAG_TREASURE_HUNTER_SEVEN_PINES"),git("FLAG_TREASURE_HUNTER_SIRONAS_BEACON"),git("FLAG_TREASURE_HUNTER_STANDING_STONES"),git("FLAG_TREASURE_HUNTER_TOWER_OF_EYES"),git("FLAG_TREASURE_HUNTER_TOMB_OF_SUCELLUS"),git("FLAG_TREASURE_HUNTER_YGGDRASIL"),git("FLAG_TREASURE_HUNTER_REMNANTS_OF_PATRIA"),git(""),git("")]
	lDirtySearched	= [git("FLAG_TREASURE_HUNTER_AIFON_ISLE_SEARCHED"),git("FLAG_TREASURE_HUNTER_BAIR_OF_LACUNA_SEARCHED"),git("FLAG_TREASURE_HUNTER_BRADELINES_WELL_SEARCHED"),git("FLAG_TREASURE_HUNTER_BRADELINES_WELL_SEARCHED"),git("FLAG_TREASURE_HUNTER_BROKEN_SEPULCHER_SEARCHED"),git("FLAG_TREASURE_HUNTER_DRAGON_BONES_SEARCHED"),git("FLAG_TREASURE_HUNTER_FOXFORD_SEARCHED"),git("FLAG_TREASURE_HUNTER_LETUM_FRIGUS_SEARCHED"),git("FLAG_TREASURE_HUNTER_MIRROR_OF_HEAVEN_SEARCHED"),git("FLAG_TREASURE_HUNTER_MOUNT_KALSHEKK_SEARCHED"),git("FLAG_TREASURE_HUNTER_ODIOS_PRISON_SEARCHED"),git("FLAG_TREASURE_HUNTER_POOL_OF_TEARS_SEARCHED"),git("FLAG_TREASURE_HUNTER_PYRE_OF_THE_SERAPHIC_SEARCHED"),git("FLAG_TREASURE_HUNTER_RINWELL_SEARCHED"),git("FLAG_TREASURE_HUNTER_RINWELL_SEARCHED"),git("FLAG_TREASURE_HUNTER_RINWELL_SEARCHED"),git("FLAG_TREASURE_HUNTER_SEVEN_PINES_SEARCHED"),git("FLAG_TREASURE_HUNTER_SIRONAS_BEACON_SEARCHED"),git("FLAG_TREASURE_HUNTER_STANDING_STONES_SEARCHED"),git("FLAG_TREASURE_HUNTER_TOWER_OF_EYES_SEARCHED"),git("FLAG_TREASURE_HUNTER_TOMB_OF_SUCELLUS_SEARCHED"),git("FLAG_TREASURE_HUNTER_YGGDRASIL_SEARCHED"),git("FLAG_TREASURE_HUNTER_REMNANTS_OF_PATRIA_SEARCHED")]
	lDirtyTexts		= ["TXT_KEY_EVENT_TREASURE_HUNTER_AIFON_ISLE","TXT_KEY_EVENT_TREASURE_HUNTER_BAIR_OF_LACUNA","TXT_KEY_EVENT_TREASURE_HUNTER_BRADELINES_WELL","TXT_KEY_EVENT_TREASURE_HUNTER_BRADELINES_WELL","TXT_KEY_EVENT_TREASURE_HUNTER_BROKEN_SEPULCHER","TXT_KEY_EVENT_TREASURE_HUNTER_DRAGON_BONES","TXT_KEY_EVENT_TREASURE_HUNTER_FOXFORD","TXT_KEY_EVENT_TREASURE_HUNTER_LETUM_FRIGUS","TXT_KEY_EVENT_TREASURE_HUNTER_MIRROR_OF_HEAVEN","TXT_KEY_EVENT_TREASURE_HUNTER_MOUNT_KALSHEKK","TXT_KEY_EVENT_TREASURE_HUNTER_ODIOS_PRISON","TXT_KEY_EVENT_TREASURE_HUNTER_POOL_OF_TEARS","TXT_KEY_EVENT_TREASURE_HUNTER_PYRE_OF_THE_SERAPHIC","TXT_KEY_EVENT_TREASURE_HUNTER_RINWELL","TXT_KEY_EVENT_TREASURE_HUNTER_RINWELL","TXT_KEY_EVENT_TREASURE_HUNTER_RINWELL","TXT_KEY_EVENT_TREASURE_HUNTER_SEVEN_PINES","TXT_KEY_EVENT_TREASURE_HUNTER_SIRONAS_BEACON","TXT_KEY_EVENT_TREASURE_HUNTER_STANDING_STONES","TXT_KEY_EVENT_TREASURE_HUNTER_TOWER_OF_EYES","TXT_KEY_EVENT_TREASURE_HUNTER_TOMB_OF_SUCELLUS","TXT_KEY_EVENT_TREASURE_HUNTER_YGGDRASIL","TXT_KEY_EVENT_TREASURE_HUNTER_REMNANTS_OF_PATRIA"]
	lCounterFlags	= [git("FLAG_TREASURE_HUNTER_5"),git("FLAG_TREASURE_HUNTER_4"),git("FLAG_TREASURE_HUNTER_3"),git("FLAG_TREASURE_HUNTER_2"),git("FLAG_TREASURE_HUNTER_1")]
	lCleanImpTexts	= []
	lCleanImpFlags	= []
	if pPlayer.isHasFlag(git("FLAG_TREASURE_HUNTER_5")):		# Spawn Patrian, clean up flags
		doSpawnPatrian(iPlayer)
		return
	for iCounter in lCounterFlags:								# Add next counter flag to the player every time function is called
		if pPlayer.isHasFlag(iCounter):
			iNextFlag = lCounterFlags.index(iCounter) - 1
			pPlayer.setHasFlag(lCounterFlags[iNextFlag], True)
	if iSearchedImp != -1:										# Set searched flag and remove active search flag
		iSearchedIndex = lDirtyImp.index(iSearchedImp)
		pPlayer.setHasFlag(lDirtyImpFlags[iSearchedIndex], False)
		pPlayer.setHasFlag(lDirtySearched[iSearchedIndex], True)
	for i in range(CyMap().numPlots()):							# Assemble Clean Improvement lists, based on if UF is present on a map
		loopPlot = CyMap().plotByIndex(i)
		if loopPlot.getImprovementType() != -1:
			iLoopImp = loopPlot.getImprovementType()
			if gc.getImprovementInfo(iLoopImp).isUnique() == True:
				if iLoopImp in lDirtyImp:
					iImpIndex = lDirtyImp.index(iLoopImp)
					if not pPlayer.isHasFlag(lDirtySearched[iImpIndex]):
						lCleanImpTexts.append(lDirtyTexts[iImpIndex])
						lCleanImpFlags.append(lDirtyImpFlags[iImpIndex])
	if lCleanImpFlags:											# If Clean list contains at least one improvement start a new search
		iNewSearchIndex = CyGame().getSorenRandNum(len(lCleanImpFlags), "Treasure Hunter, New Search")
		pPlayer.setHasFlag(lCleanImpFlags[iNewSearchIndex], True)
		if pPlayer.isHuman():
			popupInfo	= CyPopupInfo()
			popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON)
			popupInfo.setText(CyTranslator().getText(lCleanImpTexts[iNewSearchIndex], ()))
			popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_CONTINUE", ()),"")
			popupInfo.addPopup(iPlayer)
			CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText(lCleanImpTexts[iNewSearchIndex], ()),'',3,'Art/Interface/Buttons/TechTree/Astronomy.dds',ColorTypes(8),pHaven.getX(),pHaven.getY(),True,True)
	else:														# If Clean list is empty but counter is not reached (small maps) spawn a Patrian
		doSpawnPatrian(iPlayer)

def perTurnGela(pCaster):
	pPlayer = gc.getPlayer(pCaster.getOwner())
	if pPlayer.getCivilizationType() != gc.getInfoTypeForString("CIVILIZATION_INFERNAL") and pPlayer.isHuman():
		if not pPlayer.isHasFlag(gc.getInfoTypeForString("FLAG_GELA_START")):
			pPlayer.setHasFlag(gc.getInfoTypeForString("FLAG_GELA_START"), True)
			popupInfo	= CyPopupInfo()
			popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON)
			popupInfo.setText(CyTranslator().getText("TXT_KEY_EVENT_GELA", ()))
			popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_CONTINUE", ()),"")
			popupInfo.addPopup(pCaster.getOwner())

def doSpawnPatrian(iPlayer):
	git			= gc.getInfoTypeForString
	pPlayer		= gc.getPlayer(iPlayer)
	pCapital	= pPlayer.getCapitalCity()
	pPlayer.setHasFlag(git('FLAG_TREASURE_HUNTER_1'), False)
	pPlayer.setHasFlag(git('FLAG_TREASURE_HUNTER_2'), False)
	pPlayer.setHasFlag(git('FLAG_TREASURE_HUNTER_3'), False)
	pPlayer.setHasFlag(git('FLAG_TREASURE_HUNTER_4'), False)
	pPlayer.setHasFlag(git('FLAG_TREASURE_HUNTER_5'), False)
	pPlayer.setHasFlag(git('FLAG_TREASURE_HUNTER_DONE'), True)
	newUnit		= pPlayer.initUnit(git('UNIT_THE_FLYING_PATRIAN'), pCapital.getX(), pCapital.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
	newUnit.setHasPromotion(git('PROMOTION_SPIRIT_GUIDE'), True)
	if pPlayer.isHuman():
		popupInfo	= CyPopupInfo()
		popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON)
		popupInfo.setText(CyTranslator().getText("TXT_KEY_EVENT_TREASURE_HUNTER_END", ()))
		popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_EVENT_CONTINUE", ()),"")
		popupInfo.addPopup(iPlayer)

def effectHumanGelaImprovement(argsList):
	iButtonId		= argsList[0]
	iUnit			= argsList[1]
	iPlayer			= argsList[2]
	iImprovement	= argsList[3]
	CyMessageControl().sendModNetMessage(119,iButtonId,iUnit,iPlayer,iImprovement)

def effectGelaImprovement(argsList):
	iButtonId		= argsList[0]
	iUnit			= argsList[1]
	iPlayer			= argsList[2]
	iImprovement	= argsList[3]
	git				= gc.getInfoTypeForString
	pPlayer			= gc.getPlayer(iPlayer)
	pUnit			= pPlayer.getUnit(iUnit)
	pPlot			= pUnit.plot()
	if iButtonId == 1:
		return
	if iImprovement == git("IMPROVEMENT_BROKEN_SEPULCHER"):
		pUnit.setHasPromotion(git('PROMOTION_GELA'), False)
		for pyCity in PyPlayer(iPlayer).getCityList() :
			pCity = pyCity.GetCy()
			if CyGame().getSorenRandNum(100,"effect Gela, Broken Sepulcher") <= 60:
				newUnit = pPlayer.initUnit(git('UNIT_MANES'), pCity.getX(), pCity.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
			if pCity.getPopulation() > 2:
				pCity.changePopulation(-2)
		CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_GELA_BROKEN",()),'AS2D_FEATUREGROWTH',3,'Art/Interface/Buttons/Improvements/brokensepulcher.dds',ColorTypes(7),pUnit.getX(),pUnit.getY(),True,True)
	elif iImprovement == git("IMPROVEMENT_MIRROR_OF_HEAVEN"):
		pUnit.setHasPromotion(git('PROMOTION_GELA'), False)
		pUnit.setHasPromotion(git('PROMOTION_TEMP_HELD'), True)
		pUnit.setHasPromotion(git('PROMOTION_SOL'), True)
		CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_GELA_MIRROR",()),'AS2D_FEATUREGROWTH',3,'Art/Interface/Buttons/Improvements/mirrorofheaven.dds',ColorTypes(7),pUnit.getX(),pUnit.getY(),True,True)
		pDemonPlayer = gc.getPlayer(gc.getDEMON_PLAYER())
		for iPlayer2 in range(gc.getMAX_PLAYERS()):
			pPlayer2 = gc.getPlayer(iPlayer2)
			if (pPlayer2.isAlive()):
				if pPlayer2.getCivilizationType() == git('CIVILIZATION_INFERNAL'):
					pDemonPlayer = pPlayer2
					enemyTeam = pDemonPlayer.getTeam()
					pTeam = gc.getTeam(pPlayer.getTeam())
					pTeam.declareWar(enemyTeam, true, WarPlanTypes.WARPLAN_TOTAL)
		for iiX in range(pUnit.getX()-2, pUnit.getX()+3, 1):
			for iiY in range(pUnit.getY()-2, pUnit.getY()+3, 1):
				pPlot2 = CyMap().plot(iiX,iiY)
				if not pPlot2.isWater() and not pPlot2.isCity() and pPlot2.getNumUnits() == 0 and pPlot2.isFlatlands():
					if CyGame().getSorenRandNum(500, "effect Gela, Hellfire") <= 400:
						iImprovement = pPlot2.getImprovementType()
						bValid = True
						if iImprovement != -1 :
							if gc.getImprovementInfo(iImprovement).isPermanent():
								bValid = False
						if bValid :
							pPlot2.setImprovementType(git('IMPROVEMENT_HELLFIRE'))
							newUnit = pDemonPlayer.initUnit(git('UNIT_SECT_OF_FLIES'), pPlot2.getX(), pPlot2.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
							newUnit.setHasPromotion(git('PROMOTION_DEMON'), True)
	elif iImprovement == git("IMPROVEMENT_POOL_OF_TEARS"):
		pUnit.setHasPromotion(git('PROMOTION_GELA'), False)
		pUnit.setHasPromotion(git('PROMOTION_PIKE_OF_TEARS'), True)
		CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_TEARS_GELA",()),'AS2D_FEATUREGROWTH',3,'Art/Interface/Buttons/Improvements/pooloftears.dds',ColorTypes(8),pUnit.getX(),pUnit.getY(),True,True)
		iRnd = CyGame().getSorenRandNum(100, "effect Gela, Pool of Tears, Plague") <= 20
		if iRnd <= 20 or (pPlayer.getStateReligion() != git('RELIGION_FELLOWSHIP_OF_LEAVES') and iRnd <= 50):
			CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_TEARS_GELA_PLAGUE",()),'AS2D_FEATUREGROWTH',3,'Art/Interface/Buttons/Improvements/pooloftears.dds',ColorTypes(8),pUnit.getX(),pUnit.getY(),True,True)
			for iPlayer2 in range(gc.getMAX_PLAYERS()):
				pPlayer2 = gc.getPlayer(iPlayer2)
				if pPlayer2.getCivilizationType() != git('CIVILIZATION_INFERNAL'):
					for pyCity in PyPlayer(iPlayer2).getCityList() :
						pCity = pyCity.GetCy()
						i = CyGame().getSorenRandNum(5, "Blight")
						i += pCity.getPopulation() - 2
						i -= pCity.totalGoodBuildingHealth()
						pCity.changeEspionageHealthCounter(i)
						py = PyPlayer(iPlayer2)
						for pUnit2 in py.getUnitList():
							if pUnit2.isAlive():
								pUnit2.doDamageNoCaster(10, 100, git('DAMAGE_DEATH'), false)
	elif iImprovement == git("IMPROVEMENT_PYRE_OF_THE_SERAPHIC"):
		pUnit.setHasPromotion(git('PROMOTION_GELA'), False)
		pPlot.setImprovementType(-1)
		if CyGame().getSorenRandNum(100,"effect Gela, Pyre") <= 40:
			pUnit.setHasPromotion(git('PROMOTION_FROZEN_FLAME'), True)
			CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_TEARS_GELA_PYRE_1",()),'AS2D_FEATUREGROWTH',3,'Art/Interface/Buttons/Improvements/pyreoftheseraphic.dds',ColorTypes(8),pUnit.getX(),pUnit.getY(),True,True)
		else:
			mapSize = CyMap().getWorldSize()
			# i from 1 (duel) to 9 (huger)
			i = 1 + mapSize + int(mapSize/5) + int(mapSize/6)
			addBonus('BONUS_MANA',i,'Art/Interface/Buttons/WorldBuilder/mana_button.dds')
			CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_TEARS_GELA_PYRE_2",()),'AS2D_FEATUREGROWTH',3,'Art/Interface/Buttons/Improvements/pyreoftheseraphic.dds',ColorTypes(8),pUnit.getX(),pUnit.getY(),True,True)
	elif iImprovement == git("IMPROVEMENT_MAELSTROM"):
		pUnit.setHasPromotion(git('PROMOTION_GELA'), False)
		pUnit.kill(True, PlayerTypes.NO_PLAYER)
		if  pPlayer.getStateReligion() == git('RELIGION_OCTOPUS_OVERLORDS'):
			newUnit1 = pPlayer.initUnit(git('UNIT_STYGIAN_GUARD'), pUnit.getX()+1, pUnit.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
			newUnit1.setHasPromotion(git('PROMOTION_WATER_WALKING'), True)
			newUnit2 = pPlayer.initUnit(git('UNIT_STYGIAN_GUARD'), pUnit.getX()+1, pUnit.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
			newUnit2.setHasPromotion(git('PROMOTION_WATER_WALKING'), True)
			newUnit3 = pPlayer.initUnit(git('UNIT_STYGIAN_GUARD'), pUnit.getX()+1, pUnit.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
			newUnit3.setHasPromotion(git('PROMOTION_WATER_WALKING'), True)
			newUnit4 = pPlayer.initUnit(git('UNIT_STYGIAN_GUARD'), pUnit.getX()+1, pUnit.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
			newUnit4.setHasPromotion(git('PROMOTION_WATER_WALKING'), True)
			newUnit5 = pPlayer.initUnit(git('UNIT_DISCIPLE_OCTOPUS_OVERLORDS'), pUnit.getX()+1, pUnit.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
			newUnit5.setHasPromotion(git('PROMOTION_WATER_WALKING'), True)
			newUnit5.setHasPromotion(git('PROMOTION_HERO'), True)
			CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_MAELSTROM_GELA_1",()),'AS2D_FEATUREGROWTH',3,'Art/Interface/Buttons/Improvements/Maelstrom.dds',ColorTypes(7),pUnit.getX(),pUnit.getY(),True,True)
		else:
			iStygianChance = 300
			pDemonPlayer = gc.getPlayer(gc.getDEMON_PLAYER())
			for i in range (CyMap().numPlots()):
				pPlot = CyMap().plotByIndex(i)
				if pPlot.isWater() and pPlot.getNumUnits() == 0:
					if CyGame().getSorenRandNum(10000, "effect Gela, Stygian") <= iStygianChance:
						newUnit = pDemonPlayer.initUnit(git('UNIT_STYGIAN_GUARD'), pPlot.getX(), pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
						newUnit.setUnitAIType(git('UNITAI_ANIMAL'))
						newUnit.setHasPromotion(git('PROMOTION_WATER_WALKING'), True)
					elif CyGame().getSorenRandNum(10000, "effect Gela, SeaSerpent") <= iStygianChance:
						newUnit = pDemonPlayer.initUnit(git('UNIT_SEA_SERPENT'), pPlot.getX(), pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
						newUnit.setUnitAIType(git('UNITAI_ANIMAL'))
						newUnit.setHasPromotion(git('PROMOTION_WATER_WALKING'), True)
			CyInterface().addMessage(pUnit.getOwner(),True,25,CyTranslator().getText("TXT_KEY_MESSAGE_MAELSTROM_GELA_2",()),'AS2D_FEATUREGROWTH',3,'Art/Interface/Buttons/Improvements/Maelstrom.dds',ColorTypes(7),pUnit.getX(),pUnit.getY(),True,True)

def reqCheckLanunQuest(pCaster):
	pPlayer = gc.getPlayer(pCaster.getOwner())
	FlagList = ['FLAG_TREASURE_HUNTER_AIFON_ISLE','FLAG_TREASURE_HUNTER_BAIR_OF_LACUNA','FLAG_TREASURE_HUNTER_BRADELINES_WELL','FLAG_TREASURE_HUNTER_BROKEN_SEPULCHER','FLAG_TREASURE_HUNTER_DRAGON_BONES','FLAG_TREASURE_HUNTER_FOXFORD','FLAG_TREASURE_HUNTER_LETUM_FRIGUS','FLAG_TREASURE_HUNTER_MIRROR_OF_HEAVEN','FLAG_TREASURE_HUNTER_MOUNT_KALSHEKK','FLAG_TREASURE_HUNTER_ODIOS_PRISON','FLAG_TREASURE_HUNTER_POOL_OF_TEARS','FLAG_TREASURE_HUNTER_PYRE_OF_THE_SERAPHIC','FLAG_TREASURE_HUNTER_RINWELL','FLAG_TREASURE_HUNTER_SEVEN_PINES','FLAG_TREASURE_HUNTER_SIRONAS_BEACON','FLAG_TREASURE_HUNTER_STANDING_STONES','FLAG_TREASURE_HUNTER_TOWER_OF_EYES','FLAG_TREASURE_HUNTER_TOMB_OF_SUCELLUS','FLAG_TREASURE_HUNTER_YGGDRASIL','FLAG_TREASURE_HUNTER_REMNANTS_OF_PATRIA']
	for Flag in FlagList:
		if pPlayer.isHasFlag(getInfoType(Flag)) == True:
			return True
	return False

def spellCheckLanunQuest(pCaster):
	pPlayer = gc.getPlayer(pCaster.getOwner())
	szHelp = ""
	if pCaster == -1 or pCaster.isNone():
		szHelp = ""
	else:
		iPlayer 	= pCaster.getOwner()
		if pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_AIFON_ISLE')) == True:
			CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_EVENT_TREASURE_HUNTER_AIFON_ISLE", ()), '', 3, '', ColorTypes(gc.getInfoTypeForString("COLOR_GREEN")), -1, -1, False, False)
		elif pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_BAIR_OF_LACUNA')) == True:
			CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_EVENT_TREASURE_HUNTER_BAIR_OF_LACUNA", ()), '', 3, '', ColorTypes(gc.getInfoTypeForString("COLOR_GREEN")), -1, -1, False, False)
		elif pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_BRADELINES_WELL')) == True:
			CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_EVENT_TREASURE_HUNTER_BRADELINES_WELL", ()), '', 3, '', ColorTypes(gc.getInfoTypeForString("COLOR_GREEN")), -1, -1, False, False)
		elif pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_BROKEN_SEPULCHER')) == True:
			CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_EVENT_TREASURE_HUNTER_BROKEN_SEPULCHER", ()), '', 3, '', ColorTypes(gc.getInfoTypeForString("COLOR_GREEN")), -1, -1, False, False)
		elif pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_DRAGON_BONES')) == True:
			CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_EVENT_TREASURE_HUNTER_DRAGON_BONES", ()), '', 3, '', ColorTypes(gc.getInfoTypeForString("COLOR_GREEN")), -1, -1, False, False)
		elif pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_FOXFORD')) == True:
			CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_EVENT_TREASURE_HUNTER_FOXFORD", ()), '', 3, '', ColorTypes(gc.getInfoTypeForString("COLOR_GREEN")), -1, -1, False, False)
		elif pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_LETUM_FRIGUS')) == True:
			CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_EVENT_TREASURE_HUNTER_LETUM_FRIGUS", ()), '', 3, '', ColorTypes(gc.getInfoTypeForString("COLOR_GREEN")), -1, -1, False, False)
		elif pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_MIRROR_OF_HEAVEN')) == True:
			CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_EVENT_TREASURE_HUNTER_MIRROR_OF_HEAVEN", ()), '', 3, '', ColorTypes(gc.getInfoTypeForString("COLOR_GREEN")), -1, -1, False, False)
		elif pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_MOUNT_KALSHEKK')) == True:
			CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_EVENT_TREASURE_HUNTER_MOUNT_KALSHEKK", ()), '', 3, '', ColorTypes(gc.getInfoTypeForString("COLOR_GREEN")), -1, -1, False, False)
		elif pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_ODIOS_PRISON')) == True:
			CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_EVENT_TREASURE_HUNTER_ODIOS_PRISON", ()), '', 3, '', ColorTypes(gc.getInfoTypeForString("COLOR_GREEN")), -1, -1, False, False)
		elif pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_POOL_OF_TEARS')) == True:
			CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_EVENT_TREASURE_HUNTER_POOL_OF_TEARS", ()), '', 3, '', ColorTypes(gc.getInfoTypeForString("COLOR_GREEN")), -1, -1, False, False)
		elif pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_PYRE_OF_THE_SERAPHIC')) == True:
			CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_EVENT_TREASURE_HUNTER_PYRE_OF_THE_SERAPHIC", ()), '', 3, '', ColorTypes(gc.getInfoTypeForString("COLOR_GREEN")), -1, -1, False, False)
		elif pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_RINWELL')) == True:
			CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_EVENT_TREASURE_HUNTER_RINWELL", ()), '', 3, '', ColorTypes(gc.getInfoTypeForString("COLOR_GREEN")), -1, -1, False, False)
		elif pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_SEVEN_PINES')) == True:
			CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_EVENT_TREASURE_HUNTER_SEVEN_PINES", ()), '', 3, '', ColorTypes(gc.getInfoTypeForString("COLOR_GREEN")), -1, -1, False, False)
		elif pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_SIRONAS_BEACON')) == True:
			CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_EVENT_TREASURE_HUNTER_SIRONAS_BEACON", ()), '', 3, '', ColorTypes(gc.getInfoTypeForString("COLOR_GREEN")), -1, -1, False, False)
		elif pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_STANDING_STONES')) == True:
			CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_EVENT_TREASURE_HUNTER_STANDING_STONES", ()), '', 3, '', ColorTypes(gc.getInfoTypeForString("COLOR_GREEN")), -1, -1, False, False)
		elif pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_TOWER_OF_EYES')) == True:
			CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_EVENT_TREASURE_HUNTER_TOWER_OF_EYES", ()), '', 3, '', ColorTypes(gc.getInfoTypeForString("COLOR_GREEN")), -1, -1, False, False)
		elif pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_TOMB_OF_SUCELLUS')) == True:
			CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_EVENT_TREASURE_HUNTER_TOMB_OF_SUCELLUS", ()), '', 3, '', ColorTypes(gc.getInfoTypeForString("COLOR_GREEN")), -1, -1, False, False)
		elif pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_YGGDRASIL')) == True:
			CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_EVENT_TREASURE_HUNTER_YGGDRASIL", ()), '', 3, '', ColorTypes(gc.getInfoTypeForString("COLOR_GREEN")), -1, -1, False, False)
		elif pPlayer.isHasFlag(getInfoType('FLAG_TREASURE_HUNTER_REMNANTS_OF_PATRIA')) == True:
			CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_EVENT_TREASURE_HUNTER_REMNANTS_OF_PATRIA", ()), '', 3, '', ColorTypes(gc.getInfoTypeForString("COLOR_GREEN")), -1, -1, False, False)

CvScreensInterface.effectHumanGelaImprovement	= effectHumanGelaImprovement