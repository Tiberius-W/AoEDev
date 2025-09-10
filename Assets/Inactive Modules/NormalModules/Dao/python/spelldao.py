#Spell system and FfH specific callout python functions
#All code by Kael, all bugs by woodelf

from CvPythonExtensions import *
import CvUtil
import Popup as PyPopup
import CvScreensInterface
import sys
import PyHelpers
import CustomFunctions
import ScenarioFunctions


PyInfo = PyHelpers.PyInfo
PyPlayer = PyHelpers.PyPlayer
gc = CyGlobalContext()
cf = CustomFunctions.CustomFunctions()
sf = ScenarioFunctions.ScenarioFunctions()


def reqElementalSwarm(caster):
	pPlayer = gc.getPlayer(caster.getOwner())
	iWater = pPlayer.getImprovementCount(gc.getInfoTypeForString('IMPROVEMENT_WATER_NODE'))
	iFire = pPlayer.getImprovementCount(gc.getInfoTypeForString('IMPROVEMENT_FIRE_NODE'))
	iEarth = pPlayer.getImprovementCount(gc.getInfoTypeForString('IMPROVEMENT_EARTH_NODE'))
	iAir = pPlayer.getImprovementCount(gc.getInfoTypeForString('IMPROVEMENT_AIR_NODE'))
	if pPlayer.isHuman() == False:
		if (iWater + iFire + iEarth + iAir) < 3:
			return False
	return True

def spellElementalSwarm(caster):
	iPlayer = caster.getOwner()
	pPlayer = gc.getPlayer(iPlayer)
	py = PyPlayer(caster.getOwner())
	iWaterAncestry = gc.getInfoTypeForString('PROMOTION_ANCESTRY_WATER')
	iFireAncestry = gc.getInfoTypeForString('PROMOTION_ANCESTRY_FIRE')
	iEarthAncestry = gc.getInfoTypeForString('PROMOTION_ANCESTRY_EARTH')
	iAirAncestry = gc.getInfoTypeForString('PROMOTION_ANCESTRY_AIR')
	for i in range (CyMap().numPlots()):
		pPlot = CyMap().plotByIndex(i)
		if pPlot.isOwned():
			if pPlot.getOwner() == iPlayer:
				if pPlot.getImprovementType() == gc.getInfoTypeForString('IMPROVEMENT_MANA_WATER'):
					pPlayer.initUnit(gc.getInfoTypeForString('UNIT_WATER_ELEMENTAL'), pPlot.getX(), pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
				if pPlot.getImprovementType() == gc.getInfoTypeForString('IMPROVEMENT_MANA_FIRE'):
					pPlayer.initUnit(gc.getInfoTypeForString('UNIT_FIRE_ELEMENTAL'), pPlot.getX(), pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
				if pPlot.getImprovementType() == gc.getInfoTypeForString('IMPROVEMENT_MANA_EARTH'):
					pPlayer.initUnit(gc.getInfoTypeForString('UNIT_EARTH_ELEMENTAL'), pPlot.getX(), pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
				if pPlot.getImprovementType() == gc.getInfoTypeForString('IMPROVEMENT_MANA_AIR'):
					pPlayer.initUnit(gc.getInfoTypeForString('UNIT_AIR_ELEMENTAL'), pPlot.getX(), pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
	iCount = 0
	for pUnit in py.getUnitList():
		if pUnit.getDuration() == 0:
			if pUnit.isHasPromotion(gc.getInfoTypeForString('PROMOTION_ELEMENTAL')):
				iCount = (iCount + 1)
			if pUnit.isHasPromotion(gc.getInfoTypeForString('PROMOTION_ELEMENTAL_HUGE')):
				iCount = (iCount + 1)
	for pUnit in py.getUnitList():
		if (pUnit.isHasPromotion(iWaterAncestry) or pUnit.isHasPromotion(iFireAncestry) or pUnit.isHasPromotion(iEarthAncestry) or pUnit.isHasPromotion(iAirAncestry)):
			pUnit.changeExperience(iCount, -1, False, False, False, False)

def reqElementalUnity(caster):
	pPlayer = gc.getPlayer(caster.getOwner())
	if not caster.isHurt():
		return False
	if pPlayer.isHuman() == False:
		if caster.getDamage() < 25:
			return False
	return True

def spellElementalUnity(caster,amount):
	caster.changeDamage(-amount,0)
	caster.finishMoves()
