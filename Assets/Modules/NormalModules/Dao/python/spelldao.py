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
getInfoType         = gc.getInfoTypeForString

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

def reqElementalEquilibrium(Caster):
	pPlot = Caster.plot()
	iImprovement = pPlot.getImprovementType()
        
	if iImprovement != -1 and gc.getImprovementInfo(iImprovement).isUnique():
		return False
	if pPlot.getBonusType(-1) == getInfoType('BONUS_MANA_SHADOW'):
		return True
	if pPlot.getBonusType(-1) == getInfoType('BONUS_MANA_ENCHANTMENT'):
		return True
	if pPlot.getBonusType(-1) == getInfoType('BONUS_MANA_LAW'):
		return True
	if pPlot.getBonusType(-1) == getInfoType('BONUS_MANA_DEATH'):
		return True
	if pPlot.getBonusType(-1) == getInfoType('BONUS_MANA_LIFE'):
		return True
	if pPlot.getBonusType(-1) == getInfoType('BONUS_MANA_SUN'):
		return True
	if pPlot.getBonusType(-1) == getInfoType('BONUS_MANA_ICE'):
		return True
	if pPlot.getBonusType(-1) == getInfoType('BONUS_MANA_CREATION'):
		return True
	if pPlot.getBonusType(-1) == getInfoType('BONUS_MANA_NATURE'):
		return True
	if pPlot.getBonusType(-1) == getInfoType('BONUS_MANA_ENTROPY'):
		return True
	if pPlot.getBonusType(-1) == getInfoType('BONUS_MANA_DIMENSIONAL'):
		return True
	if pPlot.getBonusType(-1) == getInfoType('BONUS_MANA_METAMAGIC'):
		return True
	if pPlot.getBonusType(-1) == getInfoType('BONUS_MANA_BODY'):
		return True
	if pPlot.getBonusType(-1) == getInfoType('BONUS_MANA_CHAOS'):
		return True
	if pPlot.getBonusType(-1) == getInfoType('BONUS_MANA_MIND'):
		return True
	if pPlot.getBonusType(-1) == getInfoType('BONUS_MANA_AIR'):
		return True
	if pPlot.getBonusType(-1) == getInfoType('BONUS_MANA_EARTH'):
		return True
	if pPlot.getBonusType(-1) == getInfoType('BONUS_MANA_FIRE'):
		return True
	if pPlot.getBonusType(-1) == getInfoType('BONUS_MANA_WATER'):
		return True
	if pPlot.getBonusType(-1) == getInfoType('BONUS_MANA_FORCE'):
		return True
	if pPlot.getBonusType(-1) == getInfoType('BONUS_MANA_SPIRIT'):
		return True


def spellElementalEquilibrium(Caster):
        pPlayer = gc.getPlayer(caster.getOwner())
	iKan = pPlayer.getImprovementCount(gc.getInfoTypeForString('IMPROVEMENT_MANA_WATER_I_DAO'))
	iLi = pPlayer.getImprovementCount(gc.getInfoTypeForString('IMPROVEMENT_MANA_FIRE_I_DAO'))
	iGen = pPlayer.getImprovementCount(gc.getInfoTypeForString('IMPROVEMENT_MANA_EARTH_I_DAO'))
	iQian = pPlayer.getImprovementCount(gc.getInfoTypeForString('IMPROVEMENT_MANA_AIR_I_DAO'))
	iPoshi = pPlayer.getImprovementCount(gc.getInfoTypeForString('IMPROVEMENT_FORCE_NODE'))
	iBonus = pPlot.getBonusType(-1)
	setBonus = pPlot.setBonusType
	randNum = CyGame().getSorenRandNum
	iImprovement = pPlot.getImprovementType()
	chance = randNum(100, "Balance")


        if (iKan + iLi + iGen + iPoshi) < (iQian):
                pPlot.setImprovementType(-1)
                if iBonus == getInfoType('BONUS_MANA_FORCE'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_AIR'))
                if iBonus == getInfoType('BONUS_MANA_FIRE'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_AIR'))
                if iBonus == getInfoType('BONUS_MANA_WATER'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_AIR'))
                if iBonus == getInfoType('BONUS_MANA_EARTH'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_AIR'))
                if iBonus == getInfoType('BONUS_MANA_AIR'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_AIR'))
                if iBonus == getInfoType('BONUS_MANA_MIND'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_AIR'))
                if iBonus == getInfoType('BONUS_MANA_CREATION'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_AIR'))
                if iBonus == getInfoType('BONUS_MANA_DEATH'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_AIR'))
                if iBonus == getInfoType('BONUS_MANA_SHADOW'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_AIR'))
                if iBonus == getInfoType('BONUS_MANA_ENTROPY'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_AIR'))
                if iBonus == getInfoType('BONUS_MANA_CHAOS'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_AIR'))
                if iBonus == getInfoType('BONUS_MANA_SUN'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_AIR'))
                if iBonus == getInfoType('BONUS_MANA_LAW'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_AIR'))
                if iBonus == getInfoType('BONUS_MANA_ICE'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_AIR'))
                if iBonus == getInfoType('BONUS_MANA_SPIRIT'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_AIR'))
                if iBonus == getInfoType('BONUS_MANA_BODY'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_AIR'))
                if iBonus == getInfoType('BONUS_MANA_DIMENSIONAL'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_AIR'))
                if iBonus == getInfoType('BONUS_MANA_METAMAGIC'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_AIR'))
                if iBonus == getInfoType('BONUS_MANA_NATURE'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_AIR'))
                if iBonus == getInfoType('BONUS_MANA_ENCHANTMENT'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_AIR'))
                if iBonus == getInfoType('BONUS_MANA_LIFE'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_AIR'))

        if (iQian + iLi + iGen + iPoshi) < (iKan):
                pPlot.setImprovementType(-1)
                if iBonus == getInfoType('BONUS_MANA_FORCE'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_WATER'))
                if iBonus == getInfoType('BONUS_MANA_FIRE'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_WATER'))
                if iBonus == getInfoType('BONUS_MANA_WATER'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_WATER'))
                if iBonus == getInfoType('BONUS_MANA_EARTH'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_WATER'))
                if iBonus == getInfoType('BONUS_MANA_AIR'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_WATER'))
                if iBonus == getInfoType('BONUS_MANA_MIND'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_WATER'))
                if iBonus == getInfoType('BONUS_MANA_CREATION'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_WATER'))
                if iBonus == getInfoType('BONUS_MANA_DEATH'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_WATER'))
                if iBonus == getInfoType('BONUS_MANA_SHADOW'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_WATER'))
                if iBonus == getInfoType('BONUS_MANA_ENTROPY'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_WATER'))
                if iBonus == getInfoType('BONUS_MANA_CHAOS'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_WATER'))
                if iBonus == getInfoType('BONUS_MANA_SUN'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_WATER'))
                if iBonus == getInfoType('BONUS_MANA_LAW'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_WATER'))
                if iBonus == getInfoType('BONUS_MANA_ICE'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_WATER'))
                if iBonus == getInfoType('BONUS_MANA_SPIRIT'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_WATER'))
                if iBonus == getInfoType('BONUS_MANA_BODY'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_WATER'))
                if iBonus == getInfoType('BONUS_MANA_DIMENSIONAL'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_WATER'))
                if iBonus == getInfoType('BONUS_MANA_METAMAGIC'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_WATER'))
                if iBonus == getInfoType('BONUS_MANA_NATURE'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_WATER'))
                if iBonus == getInfoType('BONUS_MANA_ENCHANTMENT'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_WATER'))
                if iBonus == getInfoType('BONUS_MANA_LIFE'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_WATER'))

        if (iKan + iQian + iGen + iPoshi) < (iLi):
                pPlot.setImprovementType(-1)
                if iBonus == getInfoType('BONUS_MANA_FORCE'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_FIRE'))
                if iBonus == getInfoType('BONUS_MANA_FIRE'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_FIRE'))
                if iBonus == getInfoType('BONUS_MANA_WATER'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_FIRE'))
                if iBonus == getInfoType('BONUS_MANA_EARTH'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_FIRE'))
                if iBonus == getInfoType('BONUS_MANA_AIR'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_FIRE'))
                if iBonus == getInfoType('BONUS_MANA_MIND'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_FIRE'))
                if iBonus == getInfoType('BONUS_MANA_CREATION'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_FIRE'))
                if iBonus == getInfoType('BONUS_MANA_DEATH'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_FIRE'))
                if iBonus == getInfoType('BONUS_MANA_SHADOW'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_FIRE'))
                if iBonus == getInfoType('BONUS_MANA_ENTROPY'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_FIRE'))
                if iBonus == getInfoType('BONUS_MANA_CHAOS'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_FIRE'))
                if iBonus == getInfoType('BONUS_MANA_SUN'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_FIRE'))
                if iBonus == getInfoType('BONUS_MANA_LAW'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_FIRE'))
                if iBonus == getInfoType('BONUS_MANA_ICE'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_FIRE'))
                if iBonus == getInfoType('BONUS_MANA_SPIRIT'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_FIRE'))
                if iBonus == getInfoType('BONUS_MANA_BODY'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_FIRE'))
                if iBonus == getInfoType('BONUS_MANA_DIMENSIONAL'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_FIRE'))
                if iBonus == getInfoType('BONUS_MANA_METAMAGIC'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_FIRE'))
                if iBonus == getInfoType('BONUS_MANA_NATURE'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_FIRE'))
                if iBonus == getInfoType('BONUS_MANA_ENCHANTMENT'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_FIRE'))
                if iBonus == getInfoType('BONUS_MANA_LIFE'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_EARTH'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_FIRE'))

        if (iKan + iLi + iQian + iPoshi) < (iGen):
                pPlot.setImprovementType(-1)
                if iBonus == getInfoType('BONUS_MANA_FORCE'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_EARTH'))
                if iBonus == getInfoType('BONUS_MANA_FIRE'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_EARTH'))
                if iBonus == getInfoType('BONUS_MANA_WATER'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_EARTH'))
                if iBonus == getInfoType('BONUS_MANA_EARTH'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_EARTH'))
                if iBonus == getInfoType('BONUS_MANA_AIR'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_EARTH'))
                if iBonus == getInfoType('BONUS_MANA_MIND'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_EARTH'))
                if iBonus == getInfoType('BONUS_MANA_CREATION'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_EARTH'))
                if iBonus == getInfoType('BONUS_MANA_DEATH'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_EARTH'))
                if iBonus == getInfoType('BONUS_MANA_SHADOW'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_EARTH'))
                if iBonus == getInfoType('BONUS_MANA_ENTROPY'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_EARTH'))
                if iBonus == getInfoType('BONUS_MANA_CHAOS'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_EARTH'))
                if iBonus == getInfoType('BONUS_MANA_SUN'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_EARTH'))
                if iBonus == getInfoType('BONUS_MANA_LAW'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_EARTH'))
                if iBonus == getInfoType('BONUS_MANA_ICE'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_EARTH'))
                if iBonus == getInfoType('BONUS_MANA_SPIRIT'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_EARTH'))
                if iBonus == getInfoType('BONUS_MANA_BODY'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_EARTH'))
                if iBonus == getInfoType('BONUS_MANA_DIMENSIONAL'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_EARTH'))
                if iBonus == getInfoType('BONUS_MANA_METAMAGIC'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_EARTH'))
                if iBonus == getInfoType('BONUS_MANA_NATURE'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_EARTH'))
                if iBonus == getInfoType('BONUS_MANA_ENCHANTMENT'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_EARTH'))
                if iBonus == getInfoType('BONUS_MANA_LIFE'):
                        if chance < 24: setBonus(getInfoType('BONUS_MANA_FIRE'))
                        elif chance < 48: setBonus(getInfoType('BONUS_MANA_WATER'))
                        elif chance < 72: setBonus(getInfoType('BONUS_MANA_AIR'))
                        elif chance < 95: setBonus(getInfoType('BONUS_MANA_FORCE'))
                        else: setBonus(getInfoType('BONUS_MANA_EARTH'))

