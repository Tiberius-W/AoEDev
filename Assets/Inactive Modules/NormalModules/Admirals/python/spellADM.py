from CvPythonExtensions import *
from BasicFunctions import *
from CvSpellInterface import *
import PyHelpers
import CvEventInterface
import CvUtil

# Globals
PyPlayer			= PyHelpers.PyPlayer
gc					= CyGlobalContext()
localText			= CyTranslator()
git					= gc.getInfoTypeForString

def getHelpAdmiralJoin(argsList):
	eSpell, pCaster = argsList
	pPlot		= pCaster.plot()
	iTeam		= pCaster.getTeam()
	iAdmClass	= git("UNITCLASS_COMMANDER")
	fCasterXP	= pCaster.getExperience()
	pTarget		= -1
	szHelp		= ""
	for iUnit in xrange(pPlot.getNumUnits()):
		pUnit = pPlot.getUnit(iUnit)
		if not pUnit.getUnitClassType() == iAdmClass:
			continue
		if not pUnit.getTeam() == iTeam:
			continue
		pTarget = pUnit
		szHelp = localText.getText("TXT_KEY_ADM_JOIN_PYHELP_TARGET", (pTarget.getNameKey(),))
		return szHelp
	return szHelp

def spellAdmiralJoin(pCaster):
	pPlot		= pCaster.plot()
	iTeam		= pCaster.getTeam()
	iAdmClass	= git("UNITCLASS_COMMANDER")
	fCasterXP	= pCaster.getExperience()
	pTarget		= -1
	for iUnit in xrange(pPlot.getNumUnits()):
		pUnit = pPlot.getUnit(iUnit)
		if not pUnit.getUnitClassType() == iAdmClass:
			continue
		if not pUnit.getTeam() == iTeam:
			continue
		pTarget = pUnit
		sTargetName = pTarget.getNameKey()
		fTargetXP = pTarget.getExperience()
		pCaster.setName(sTargetName)
		if fTargetXP > fCasterXP:
			pCaster.setExperience(fTargetXP,-1)
		pTarget.kill(True, PlayerTypes.NO_PLAYER)
		return

def reqAdmiralJoin(pCaster):
	pPlot		= pCaster.plot()
	iTeam		= pCaster.getTeam()
	iAdmClass	= git("UNITCLASS_COMMANDER")
	lBlackList	= [git("UNITCLASS_TRADESHIP"),git("UNITCLASS_WYRMFISHER"),git("UNITCLASS_WORKBOAT")]
	if not pCaster.getDomainType() == DomainTypes.DOMAIN_SEA:
		return False
	if pCaster.getUnitClassType() in lBlackList:
		return False
	for iUnit in xrange(pPlot.getNumUnits()):
		pUnit = pPlot.getUnit(iUnit)
		if not pUnit.getUnitClassType() == iAdmClass:
			continue
		if not pUnit.getTeam() == iTeam:
			continue
		return True
	return False