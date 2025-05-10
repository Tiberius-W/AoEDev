## Arachnophobia CvSpellInterface.py
## By LeastCraft 05/08/2023

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
getInfoType			= gc.getInfoTypeForString

Manager             = CvEventInterface.getEventManager()
Bonus               = Manager.Resources
Civ                 = Manager.Civilizations
Trait               = Manager.Traits
Buildings           = Manager.Buildings
UnitCombat          = Manager.UnitCombats
Race                = Manager.Promotions["Race"]
GenericPromo        = Manager.Promotions["Generic"]
Effect              = Manager.Promotions["Effects"]


def GetWorstOwnUnitsInCasterTile(caster, amount=1, unitType='', withPromo='', withoutPromo=''):
	pPlot = caster.plot()
	validUnits = []

	for i in range(pPlot.getNumUnits()):
		pUnit = pPlot.getUnit(i)
		if pUnit.getOwner() == caster.getOwner() and pUnit.getID() != caster.getID():
			if (unitType == '' or pUnit.getUnitType() == getInfoType(unitType)) and \
			   (withPromo == '' or pUnit.isHasPromotion(getInfoType(withPromo))) and \
			   (withoutPromo == '' or not pUnit.isHasPromotion(getInfoType(withoutPromo))):
				
				iLevel = pUnit.getLevel()
				iStrength = pUnit.baseCombatStr()
				fStrength = iStrength * (1.0 - (pUnit.getDamage() / 100))  # Actual strength factoring in damage
				fModifier = iLevel / 2.0 + 1.0

				# Adjust modifier based on promotions
				if pUnit.isHasPromotion(getInfoType('PROMOTION_HERO')) or pUnit.isHasPromotion(getInfoType('PROMOTION_ADVENTURER')):
					fModifier += 9999  # Heroes should never be chosen
				if pUnit.isHasPromotion(getInfoType('PROMOTION_HEROIC')):
					fModifier += 9999
				if pUnit.isHasPromotion(getInfoType('PROMOTION_STRONG')):
					fModifier += 2
				if pUnit.isHasPromotion(getInfoType('PROMOTION_GROWTH_SPURTS')):
					fModifier += 2
				if pUnit.isHasPromotion(getInfoType('PROMOTION_WEAK')):
					fModifier -= 1
				if pUnit.isHasPromotion(getInfoType('PROMOTION_CRAZED')):
					fModifier -= 1
				if pUnit.isHasPromotion(getInfoType('PROMOTION_DISEASED')):
					fModifier -= 1
				if pUnit.isHasPromotion(getInfoType('PROMOTION_ENRAGED')):
					fModifier -= 1
				if pUnit.isHasPromotion(getInfoType('PROMOTION_UNDISCIPLINED')):
					fModifier -= 1
				if pUnit.isHasPromotion(getInfoType('PROMOTION_PLAGUED')):
					fModifier -= 1
				if pUnit.isHasPromotion(getInfoType('PROMOTION_MOUNT')):
					fModifier -= 99

				fValue = fStrength * fModifier
				validUnits.append((pUnit, fValue))

	# Sort valid units by their calculated value (ascending order)
	validUnits.sort(key=lambda x: x[1])

	# Return the worst unit or the requested number of worst units
	if amount == 1:
		if validUnits:
			return validUnits[0][0]
		else:
			return -1
	else:
		return [unit[0] for unit in validUnits[:amount]]


def CheckOwnUnitsExistInCasterTile(caster, amount=1, unitType='', withPromo='', withoutPromo=''):
	pPlot = caster.plot()
	count = 0

	for i in range(pPlot.getNumUnits()):
		pUnit = pPlot.getUnit(i)
		if pUnit.getOwner() == caster.getOwner() and pUnit.getID() != caster.getID():
			if (unitType == '' or pUnit.getUnitType() == getInfoType(unitType)) and \
			   (withPromo == '' or pUnit.isHasPromotion(getInfoType(withPromo))) and \
			   (withoutPromo == '' or not pUnit.isHasPromotion(getInfoType(withoutPromo))):
				count += 1
				if count >= amount:
					return True

	return False


# Spider Upgrading Spells Start


def reqSpiderUpgradeToGiant(caster):
	pPlayer = gc.getPlayer(caster.getOwner())
	if (pPlayer.getNumCities() > 0):
		pNest = pPlayer.getCapitalCity()
		iNestPop = pNest.getPopulation()
		if iNestPop >= 8:
			return True
	return False


def reqGiantSpiderUpgradeToBehemoth(caster):
	iBehemothGreen = getInfoType('PROMOTION_SPIDER_VENENUM_BEHEMOTH')
	iBehemothGrey = getInfoType('PROMOTION_SPIDER_MUCRO_BEHEMOTH')
	if caster.isHasPromotion(iBehemothGreen) or caster.isHasPromotion(iBehemothGrey): return False

	pPlayer = gc.getPlayer(caster.getOwner())
	if (pPlayer.getNumCities() > 0):
		pNest = pPlayer.getCapitalCity()
		iNestPop = pNest.getPopulation()
		if iNestPop >= 16:
			return True
	return False
# Spider Upgrading Spells End


# Spider Summoning Spells Start
def setSpiderPromo(spawnUnit, pPlayer, pCity):
	if spawnUnit:
		setPromo = spawnUnit.setHasPromotion
		pNest       = pPlayer.getCapitalCity()
		iNestPop    = pNest.getPopulation()
		getNum      = pNest.getNumBuilding

		if (pPlayer.hasTrait(Trait["Spiderkin"]) and iNestPop >= 8) or iNestPop >= 24:
			setPromo( Effect["Strong"], True)

		if pCity:
			pCity.applyBuildEffects(spawnUnit)

		if   getNum( Buildings["Nest Addon1"]) > 0:
			iBroodStrength = 1
		elif getNum( Buildings["Nest Addon2"]) > 0:
			iBroodStrength = 2
		elif getNum( Buildings["Nest Addon3"]) > 0:
			iBroodStrength = 3
		elif getNum( Buildings["Nest Addon4"]) > 0:
			iBroodStrength = 4
		else:
			iBroodStrength = 0
		spawnUnit.changeFreePromotionPick(iBroodStrength)

		iBroodExp = 0
		meatBonuses = ["BONUS_BISON", "BONUS_COW", "BONUS_CAMEL", "BONUS_DEER", "BONUS_DEER_ARCTIC", "BONUS_FUR", "BONUS_HORSE", "BONUS_HYAPON", "BONUS_IVORY", "BONUS_NIGHTMARE", "BONUS_PIG", "BONUS_SHEEP", "BONUS_TOAD"]
		for bonus in meatBonuses:
			if pPlayer.getNumAvailableBonuses(getInfoType(bonus)) > 0:
				iBroodExp += 1
		spawnUnit.changeExperience(iBroodExp, -1, False, False, False)


def spellCallBabySpider(caster):
	pPlot = caster.plot()
	pCity = pPlot.getPlotCity()
	pPlayer = gc.getPlayer(caster.getOwner())
	iSpider = getInfoType('UNIT_BABY_SPIDER')

	spawnUnit = pPlayer.initUnit(iSpider, caster.getX(), caster.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
	setSpiderPromo(spawnUnit, pPlayer, pCity)
	spawnUnit.finishMoves()

	iBroodActivity = pPlayer.getCivCounter()
	iCost = 2500
	pPlayer.setCivCounter(iBroodActivity - iCost)


def reqCallBabySpider(caster):
	pPlot = caster.plot()
	pCity = pPlot.getPlotCity()
	pPlayer = gc.getPlayer(caster.getOwner())

	return pCity.getPopulation() >= 1 and pPlayer.getCivCounter() >= 2500


def spellCallSpider(caster):
	pPlot = caster.plot()
	pCity = pPlot.getPlotCity()
	pPlayer = gc.getPlayer(caster.getOwner())
	iSpider = getInfoType('UNIT_SPIDER')

	spawnUnit = pPlayer.initUnit(iSpider, caster.getX(), caster.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
	setSpiderPromo(spawnUnit, pPlayer, pCity)
	spawnUnit.finishMoves()
	
	iBroodActivity = pPlayer.getCivCounter()
	iCost = 7500
	pPlayer.setCivCounter(iBroodActivity - iCost)


def reqCallSpider(caster):
	pPlot = caster.plot()
	pCity = pPlot.getPlotCity()
	pPlayer = gc.getPlayer(caster.getOwner())

	return pCity.getPopulation() >= 8 and pPlayer.getCivCounter() >= 7500


def spellCallGiantSpider(caster):
	pPlot = caster.plot()
	pCity = pPlot.getPlotCity()
	pPlayer = gc.getPlayer(caster.getOwner())
	iSpider = getInfoType('UNIT_GIANT_SPIDER')

	spawnUnit = pPlayer.initUnit(iSpider, caster.getX(), caster.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
	setSpiderPromo(spawnUnit, pPlayer, pCity)
	spawnUnit.finishMoves()
	
	iBroodActivity = pPlayer.getCivCounter()
	iCost = 15000
	pPlayer.setCivCounter(iBroodActivity - iCost)


def reqCallGiantSpider(caster):
	pPlot = caster.plot()
	pCity = pPlot.getPlotCity()
	pPlayer = gc.getPlayer(caster.getOwner())

	return pCity.getPopulation() >= 16 and pPlayer.getCivCounter() >= 15000
# Spider Summoning Spells End


def spellSmearPoison(caster):
	pVictim = GetWorstOwnUnitsInCasterTile(caster, unitType='UNIT_BABY_SPIDER')
	if pVictim != -1:
		pVictim.kill(True, 0)


def reqSmearPoison(caster):
	iPoisonedBlade = getInfoType('PROMOTION_POISONED_BLADE')
	pPoisonedBlade = gc.getPromotionInfo(iPoisonedBlade)
	if caster.isHasPromotion(iPoisonedBlade): return False
	if caster.getUnitCombatType()==-1: return False
	if not pPoisonedBlade.getUnitCombat(caster.getUnitCombatType()): return False
	
	return True
	

def spellCannibalize(caster):
	caster.setDamage(caster.getDamage() - 15, caster.getOwner())
	pVictim = GetWorstOwnUnitsInCasterTile(caster, unitType='UNIT_BABY_SPIDER')
	if pVictim != -1:
		pVictim.kill(True, 0)


def reqCannibalize(caster):
	eligibleUnits = [getInfoType('UNIT_SPIDER'), getInfoType('UNIT_GIANT_SPIDER'), getInfoType('UNIT_NESTING_SPIDER'), getInfoType('UNIT_MOTHER_SPIDER')]
	if caster.getUnitType() not in eligibleUnits: return False
	if caster.getDamage() == 0: return False
	
	return True
	

def spellSurvivalOfTheFittest(caster):
	caster.changeExperience(2, -1, False, False, False)
	pVictim = GetWorstOwnUnitsInCasterTile(caster, unitType='UNIT_SPIDER')
	if pVictim != -1:
		pVictim.kill(True, 0)


def reqSurvivalOfTheFittest(caster):
	eligibleUnits = [getInfoType('UNIT_SPIDER'), getInfoType('UNIT_GIANT_SPIDER')]
	if caster.getUnitType() not in eligibleUnits: return False
	
	return CheckOwnUnitsExistInCasterTile(caster, unitType='UNIT_SPIDER')


def onMoveMazeOfWebs(caster, pPlot):
	spiderUnits = [getInfoType('UNIT_BABY_SPIDER'), getInfoType('UNIT_SPIDER'), getInfoType('UNIT_GIANT_SPIDER'), getInfoType('UNIT_NESTING_SPIDER'), getInfoType('UNIT_MOTHER_SPIDER')]
	iHasted = getInfoType('PROMOTION_HASTED')
	iSlow = getInfoType('PROMOTION_SLOW')
	if caster.getUnitType() in spiderUnits:
		caster.safeRemovePromotion(iSlow)
	elif gc.getPlayer(caster.getOwner()).getCivilizationType() != Civ["Archos"]:
		caster.safeRemovePromotion(iHasted)
		caster.setHasPromotion(iSlow, True)


def spellMountSpider(caster):
	pVictim = GetWorstOwnUnitsInCasterTile(caster, unitType='UNIT_SPIDER')
	if pVictim != -1:
		pVictim.kill(True, 0)


def spellDismountSpider(caster):
	pPlayer = gc.getPlayer(caster.getOwner())
	iSpider = getInfoType('UNIT_SPIDER')
	spawnUnit = pPlayer.initUnit(iSpider, caster.getX(), caster.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
	spawnUnit.setHasPromotion( getInfoType("PROMOTION_MOUNT"), True)


def babySpiderSwarm(caster):
	pPlot = caster.plot()
	iSpider = getInfoType('UNIT_BABY_SPIDER')

	iBabySpiderCount = 0
	for i in range(pPlot.getNumUnits()):
		pUnit = pPlot.getUnit(i)
		if pUnit.getUnitType() == iSpider:
			iBabySpiderCount += 1
	
	iPromos = min(4, iBabySpiderCount / 10)
	for i in range(1,5):
		iSwarmPromo = getInfoType('PROMOTION_SWARMING' + str(i))
		if i <= iPromos:
			caster.setHasPromotion(iSwarmPromo, True)
		else:
			caster.safeRemovePromotion(iSwarmPromo)


def spiderCombatBAGain(caster,opponent):
	pPlayer = gc.getPlayer(caster.getOwner())
	if gc.getPlayer(caster.getOwner()).getCivilizationType() == Civ["Archos"]:
		iBroodActivity = pPlayer.getCivCounter()
		iGain = 500
		pPlayer.setCivCounter(iBroodActivity + iGain)


def arachnomancy1(caster):
	pPlot = caster.plot()
	iSpider = getInfoType('UNIT_BABY_SPIDER')
	iSwarmPromo = getInfoType('PROMOTION_SWARM_CONCEALED1')

	iBabySpiderCount = 0
	for i in range(pPlot.getNumUnits()):
		pUnit = pPlot.getUnit(i)
		if pUnit.getUnitType() == iSpider:
			iBabySpiderCount += 1
	
	if iBabySpiderCount >= 10:
		caster.setHasPromotion(iSwarmPromo, True)


def arachnomancy2(caster):
	pPlot = caster.plot()
	iSpider = getInfoType('UNIT_BABY_SPIDER')
	iSwarmPromo = getInfoType('PROMOTION_SWARM_CONCEALED2')

	iBabySpiderCount = 0
	for i in range(pPlot.getNumUnits()):
		pUnit = pPlot.getUnit(i)
		if pUnit.getUnitType() == iSpider:
			iBabySpiderCount += 1
	
	if iBabySpiderCount >= 20:
		caster.setHasPromotion(iSwarmPromo, True)


def arachnomancy3(caster):
	pPlot = caster.plot()
	iSpider = getInfoType('UNIT_BABY_SPIDER')
	iSwarmPromo = getInfoType('PROMOTION_SWARM_CONCEALED3')

	iBabySpiderCount = 0
	for i in range(pPlot.getNumUnits()):
		pUnit = pPlot.getUnit(i)
		if pUnit.getUnitType() == iSpider:
			iBabySpiderCount += 1
	
	if iBabySpiderCount >= 40:
		caster.setHasPromotion(iSwarmPromo, True)


def CountBabySpidersAndLog(caster, amount, icon):
	babyVictims = GetWorstOwnUnitsInCasterTile(caster, amount=amount, unitType='UNIT_BABY_SPIDER')
	if len(babyVictims) < amount:
		pPlot = caster.plot()
		iPlayer = caster.getOwner()
		CyInterface().addMessage(iPlayer, True, 25, CyTranslator().getText("TXT_KEY_MESSAGE_NOT_ENOUGH_BABY_SPIDERS",()) + ' (' + str(len(babyVictims)) + '/' + str(amount) + ')','',1,icon,gc.getInfoTypeForString("COLOR_YELLOW"),pPlot.getX(),pPlot.getY(),True,True)
		return None
	return babyVictims


def spellArachnophagy(caster):
	babyVictims = CountBabySpidersAndLog(caster, 10, 'Art/Modules/Arachnophobia/Buttons/Arachnophagy.dds')
	if not babyVictims:
		caster.setHasCasted(False)
		return

	for pVictim in babyVictims:
		pVictim.kill(True, 0)


def reqArachnophagy(caster):
	return CheckOwnUnitsExistInCasterTile(caster, amount=10, unitType='UNIT_BABY_SPIDER')


def spellCommunion(caster, spider_type):
	babyVictims = CountBabySpidersAndLog(caster, 20, 'Art/Modules/Arachnophobia/Buttons/SymbioticCommunion.dds')
	if not babyVictims:
		return
	
	iSpiderVariant = ''
	iMutation = ''

	if spider_type == 1:
		iSpiderVariant = 'PROMOTION_SPIDER_ARGYRONETA'
		iMutation = 'PROMOTION_SPIDERMUTATION_TRAIL_PHEROMONE'
	if spider_type == 2:
		iSpiderVariant = 'PROMOTION_SPIDER_VENENUM'
		iMutation = 'PROMOTION_SPIDERMUTATION_SPITTER_GLAND'
	if spider_type == 3:
		iSpiderVariant = 'PROMOTION_SPIDER_MUCRO'
		iMutation = 'PROMOTION_SPIDERMUTATION_CHITIN_CARAPACE'
	if spider_type == 4:
		iSpiderVariant = 'PROMOTION_SPIDER_RHAGODESSA'
		iMutation = 'PROMOTION_SPIDERMUTATION_VENOM_SECRETION'
	if spider_type == 5:
		iSpiderVariant = 'PROMOTION_SPIDER_TEXTUS'
		iMutation = 'PROMOTION_SPIDERMUTATION_JOINTED_LIMBS'
	
	pVictim = GetWorstOwnUnitsInCasterTile(caster, unitType='UNIT_SPIDER', withPromo=iSpiderVariant)
	if pVictim != -1:
		pVictim.kill(True, 0)
		
		for pVictim in babyVictims:
			pVictim.kill(True, 0)

		#Grant Mutation
		iMelee = getInfoType('UNITCOMBAT_MELEE')
		iRecon = getInfoType('UNITCOMBAT_RECON')

		pPlot = caster.plot()
		for i in range(pPlot.getNumUnits()):
			pUnit = pPlot.getUnit(i)
			if pUnit.getUnitCombatType() == iMelee or pUnit.getUnitCombatType() == iRecon:
				pUnit.setHasPromotion(getInfoType(iMutation), True)


def reqCommunion(caster, spider_type):
	iSpiderVariant = ''
	
	if spider_type == 1:
		iSpiderVariant = 'PROMOTION_SPIDER_ARGYRONETA'
	if spider_type == 2:
		iSpiderVariant = 'PROMOTION_SPIDER_VENENUM'
	if spider_type == 3:
		iSpiderVariant = 'PROMOTION_SPIDER_MUCRO'
	if spider_type == 4:
		iSpiderVariant = 'PROMOTION_SPIDER_RHAGODESSA'
	if spider_type == 5:
		iSpiderVariant = 'PROMOTION_SPIDER_TEXTUS'
	
	bBabySpidersInTile = CheckOwnUnitsExistInCasterTile(caster, amount=10, unitType='UNIT_BABY_SPIDER')
	bVariantSpiderInTile = CheckOwnUnitsExistInCasterTile(caster, unitType='UNIT_SPIDER', withPromo=iSpiderVariant)
	return bBabySpidersInTile and bVariantSpiderInTile


def spellSummonArachnidAvatar(caster):
	babyVictims = CountBabySpidersAndLog(caster, 40, 'Art/Modules/Arachnophobia/Buttons/Arachnomancy3.dds')
	if not babyVictims:
		caster.setHasCasted(False)
		return
	
	for pVictim in babyVictims:
		pVictim.kill(True, 0)


def reqSummonArachnidAvatar(caster):
	return CheckOwnUnitsExistInCasterTile(caster, amount=40, unitType='UNIT_BABY_SPIDER')


def spellAmalgamate(caster):
	caster.setDamage(caster.getDamage() - 10, caster.getOwner())
	caster.setMadeAttack(False)
	babyVictims = CountBabySpidersAndLog(caster, 10, 'Art/Modules/Arachnophobia/Buttons/Swarming.dds')
	if not babyVictims:
		return
	
	for pVictim in babyVictims:
		caster.setHasCasted(False)
		pVictim.kill(True, 0)


def reqAmalgamate(caster):
	return CheckOwnUnitsExistInCasterTile(caster, amount=10, unitType='UNIT_BABY_SPIDER')