

from CvPythonExtensions import *
from BasicFunctions import *
import CvUtil
import Popup as PyPopup
import CvScreensInterface
import sys
import PyHelpers
import CustomFunctions
import ScenarioFunctions
import CvEventInterface




# Common Definitions
gc                  = CyGlobalContext()
Manager             = CvEventInterface.getEventManager()
Terrain             = Manager.Terrain
Promo               = Manager.Promotions["Effects"]
Civ                 = Manager.Civilizations
Feature             = Manager.Feature
PyPlayer = PyHelpers.PyPlayer
getInfoType 		= gc.getInfoTypeForString
		

def spellWisdom(caster):
	py = PyPlayer(caster.getOwner())
	for pUnit in py.getUnitList():
		if pUnit.isHasPromotion(getInfoType('PROMOTION_GIANTKIN')):
			pUnit.setExperienceTimes100((pUnit.getExperienceTimes100() * 2), -1)

def reqSpreadJotAV(caster):
	pPlayer = gc.getPlayer(caster.getOwner())
	pCity = caster.plot().getPlotCity()

	if pPlayer.getStateReligion() != getInfoType('RELIGION_THE_ASHEN_VEIL'):
		return False

	pTeam = gc.getTeam(pPlayer.getTeam())
	if pTeam.isHasTech(getInfoType('TECH_CORRUPTION_OF_SPIRIT')) == False:
		return False

	if pCity.isHasReligion(getInfoType('RELIGION_THE_ASHEN_VEIL')):
		return False

	return True

def spellSteading(caster):
	pPlot = caster.plot()
	pPlayer = gc.getPlayer(caster.getOwner())

	pCity = pPlayer.initCity(pPlot.getX(),pPlot.getY())
	CvEventInterface.getEventManager().onCityBuilt([pCity])

def ReqSteading(pCaster):
	pPlot = pCaster.plot()
	iImprovement = pPlot.getImprovementType()
	if iImprovement != -1:
		pImprovement = gc.getImprovementInfo(iImprovement)
		if pImprovement.isUnique():
			return False
	return not pPlot.isWater() and not pPlot.isCity()

def reqJotBloom(caster):
	pPlot = caster.plot()
	pPlayer = gc.getPlayer(caster.getOwner())
	sValidTerrains = ['MARSH', 'GRASS', 'PLAINS', 'TAIGA', 'TUNDRA']
	iValidTerrains = [getInfoType('TERRAIN_' + sTerrain) for sTerrain in sValidTerrains]
	if (pPlot.getTerrainType() in iValidTerrains):
		return True
	return False

def spellJotFeed(caster):
	caster.setDamage(caster.getDamage() - 20, caster.getOwner())
	pVictim = -1
	pPlot = caster.plot()
	for i in range(pPlot.getNumUnits()):
		pUnit = pPlot.getUnit(i)
		if pUnit.getOwner() == caster.getOwner():
			if pUnit.getUnitType() == getInfoType('UNIT_THRALL_MILITIA'):
				if (pVictim == -1 or pVictim.getLevel() > pUnit.getLevel()):
					pVictim = pUnit
	if pVictim != -1:
		pVictim.kill(True, 0)

def reqJotFeed(caster):
	if caster.getDamage() == 0:
		return False
	pPlayer = gc.getPlayer(caster.getOwner())
	if pPlayer.isHuman() == False:
		if caster.getDamage() < 20:
			return False
	return True

