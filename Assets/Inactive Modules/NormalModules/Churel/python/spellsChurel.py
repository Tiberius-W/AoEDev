## tribalLawElection.py
## This file applies the effects of each election choice.
## Created by Alsark, 2/3/2018
from CvPythonExtensions import *
import FoxDebug
import FoxTools
from BasicFunctions import *
import CustomFunctions
import CvEventInterface

#Global
gc = CyGlobalContext()
getInfoType = gc.getInfoTypeForString
localText = CyTranslator()

def reqFeastGraveleech(caster):
	pPlot = caster.plot()
	pCity = pPlot.getPlotCity()
	pPlayer = gc.getPlayer(caster.getOwner())
	eCiv = pPlayer.getCivilizationType()
#	if eCiv != Civ["Calabim"] and pPlayer.getLeaderType() != getInfoType('LEADER_ZARIA'):
#		return False
	if  caster.isHasPromotion(getInfoType('PROMOTION_GRAVELEECH')):
		if not pCity.getNumRealBuilding(getInfoType('BUILDING_UNDYING_RITUAL_HOUSE')):
			return False
	if pCity.getPopulation() < 3:
		return False
	return True	

def reqFeedGraveleech(caster):
	pPlayer = gc.getPlayer(caster.getOwner())
	if pPlayer.hasTrait(getInfoType('TRAIT_GRAVELEECH')): return False
	if not caster.isHurt(): return False
	pPlayer = gc.getPlayer(caster.getOwner())
	if not pPlayer.isHuman():
		if caster.getDamage() < 20:
			return False
	return True

def reqFeedUndeadGraveyard(caster):
	pPlot = caster.plot()
	if (pPlot.getImprovementType() == getInfoType("IMPROVEMENT_GRAVEYARD") or pPlot.getImprovementType() == getInfoType("IMPROVEMENT_BARROW") or pPlot.getImprovementType() == getInfoType("IMPROVEMENT_CITY_RUINS")  or pPlot.getImprovementType() == getInfoType("IMPROVEMENT_CITY_RUINS_ANCIENT")  or pPlot.getImprovementType() == getInfoType("IMPROVEMENT_BARROW_GLOOMY")  or pPlot.getImprovementType() == getInfoType("IMPROVEMENT_BARROW_HOWLING")):
		return True
	return False
def spellFeedUndeadGraveyard(caster):
	caster.setDamage(caster.getDamage() - 20, caster.getOwner())
	caster.setMadeAttack(False)
	pPlot = caster.plot()
	pPlot.setImprovementType(-1)
	caster.changeExperienceTimes100(500,-1,False,False,False)
	
def reqFeedUndead(caster):
	pPlayer = gc.getPlayer(caster.getOwner())
	if not caster.isHurt(): return False
	pPlayer = gc.getPlayer(caster.getOwner())
	if not pPlayer.isHuman():
		if caster.getDamage() < 20:
			return False
	pPlot = caster.plot()
	for i in range(pPlot.getNumUnits()):
		pUnit = pPlot.getUnit(i)
		if pUnit.getOwner() == caster.getOwner():
			if pUnit.getID() != caster.getID():
				if pUnit.isHasPromotion(getInfoType('PROMOTION_UNDEAD')):
					return True
	return False

	
def spellFeedUndead(caster):
	caster.setDamage(caster.getDamage() - 20, caster.getOwner())
	caster.setMadeAttack(False)
	pVictim = -1
	pPlot = caster.plot()
	for i in range(pPlot.getNumUnits()):
		pUnit = pPlot.getUnit(i)
		if pUnit.getOwner() == caster.getOwner():
			if pUnit.getID() != caster.getID():
				if pUnit.isHasPromotion(getInfoType('PROMOTION_UNDEAD')):
					if (pVictim == -1 or pVictim.getLevel() > pUnit.getLevel()):
						pVictim = pUnit
	if pVictim != -1:
		pVictim.kill(True, 0)
	pPlayer = gc.getPlayer(caster.getOwner())
	if pPlayer.hasTrait(getInfoType('TRAIT_GRAVELEECH2')): 
		caster.setHasPromotion(getInfoType('PROMOTION_UNDEAD_SOUL'),True)

def postCombatWinUndeadEater(pCaster, pOpponent):
	if pOpponent.isHasPromotion(getInfoType("PROMOTION_UNDEAD")):
		pCaster.setDamage(caster.getDamage() - 20, caster.getOwner())
		pCaster.setHasPromotion(getInfoType('PROMOTION_UNDEAD_SOUL'),True)
