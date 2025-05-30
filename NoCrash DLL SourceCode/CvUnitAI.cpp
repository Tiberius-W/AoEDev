// unitAI.cpp

#include "CvGameCoreDLL.h"
#include "CvUnitAI.h"
#include "CvMap.h"
#include "CvArea.h"
#include "CvPlot.h"
#include "CvGlobals.h"
#include "CvGameAI.h"
#include "CvTeamAI.h"
#include "CvPlayerAI.h"
#include "CvGameCoreUtils.h"
#include "CvRandom.h"
#include "CyUnit.h"
#include "CyArgsList.h"
#include "CvDLLPythonIFaceBase.h"
#include "CvInfos.h"
#include "FProfiler.h"
#include "FAStarNode.h"

// interface uses
#include "CvDLLInterfaceIFaceBase.h"
#include "CvDLLFAStarIFaceBase.h"

#define FOUND_RANGE				(7)

// Public Functions...

CvUnitAI::CvUnitAI()
{
	AI_reset();
}


CvUnitAI::~CvUnitAI()
{
	AI_uninit();
}


void CvUnitAI::AI_init(UnitAITypes eUnitAI)
{
	AI_reset(eUnitAI);

	//--------------------------------
	// Init other game data
	AI_setBirthmark(GC.getGameINLINE().getSorenRandNum(10000, "AI Unit Birthmark"));

	FAssertMsg(AI_getUnitAIType() != NO_UNITAI, "AI_getUnitAIType() is not expected to be equal with NO_UNITAI");
	area()->changeNumAIUnits(getOwnerINLINE(), AI_getUnitAIType(), 1);
	GET_PLAYER(getOwnerINLINE()).AI_changeNumAIUnits(AI_getUnitAIType(), 1);
}


void CvUnitAI::AI_uninit()
{
}


void CvUnitAI::AI_reset(UnitAITypes eUnitAI)
{
	AI_uninit();

	m_iBirthmark = 0;

	m_eUnitAIType = eUnitAI;

	m_iAutomatedAbortTurn = -1;
}

// AI_update returns true when we should abort the loop and wait until next slice
bool CvUnitAI::AI_update()
{
	PROFILE_FUNC();

	CvUnit* pTransportUnit;

	FAssertMsg(canMove(), "canMove is expected to be true");
	FAssertMsg(isGroupHead(), "isGroupHead is expected to be true"); // XXX is this a good idea???

	// Places same restrictions on the AI as are placed on the Player for Blindness : Xienwolf 05/12/09
	if (isBlind() && (!plot()->isVisible(getTeam(), false) || !plot()->isRevealed(getTeam(), false)))
	{
		finishMoves();
		return false;
	}
	// allow python to handle it
	// FfH: Modified by Kael 10/02/2008
	if (isBarbarian())
	{
		claimFort(); // Barbs always try to claim forts before running off : Blazenclaw
		CyUnit* pyUnit = new CyUnit(this);
		CyArgsList argsList;
		argsList.add(gDLL->getPythonIFace()->makePythonObject(pyUnit));	// pass in unit class
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "AI_unitUpdate", argsList.makeFunctionArgs(), &lResult);
		delete pyUnit;	// python fxn must not hold on to this pointer
		if (lResult == 1)
		{	
			// Clearing Asserts and helping the AI stop looping so much : Xienwolf
			finishMoves();
			return false;
		}
	}
	//FfH: End Modify

	/*************************************************************************************************/
	/**	Xienwolf Tweak							01/04/09											**/
	/**					Clearing Asserts and helping the AI stop looping so much					**/
	/**Near as I can tell, this is refusing to allow the unit to do ANYTHING if it winds up isolated**/
	/**	I personally don't care for that, but it makes sense to keep the AI from trying to move the	**/
	/**	Unit, so need to re-apply this once I figure out how to check for possible missions/spells	**/
	/**				before canceling the unit's ability to make any movements						**/
	/*************************************************************************************************/

	/**								---- Start Original Code ----									**
	//FfH: Added by Kael 12/22/2007
	CvPlot* pPlot = plot();
	CvPlot* pLoopPlot;
	bool bValid = false;
	for (int iDX = -1; iDX <= 1; iDX++)
	{
		for (int iDY = -1; iDY <= 1; iDY++)
		{
			pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);
			if (pLoopPlot != NULL)
			{
				if (canMoveOrAttackInto(pLoopPlot))
				{
					bValid = true;
				}
			}
		}
	}
	if (bValid == false)
	{
		getGroup()->pushMission(MISSION_SKIP);
		return false;
	}
	//FfH: End Add
	/**								----  End Original Code  ----									**/
	/*************************************************************************************************/
	/**	Tweak									END													**/
	/*************************************************************************************************/

	if (getDomainType() == DOMAIN_LAND)
	{
		// Only ignore land units if they aren't allowed on the water tile
		if (plot()->isWater()
		 && !canMoveAllTerrain()
		 && !plot()->getImprovementType() != NO_IMPROVEMENT
		 && !GC.getImprovementInfo(plot()->getImprovementType()).isActsAsCity())
		{
			getGroup()->pushMission(MISSION_SKIP);
			return false;
		}
		else
		{
			pTransportUnit = getTransportUnit();

			if (pTransportUnit != NULL)
			{
				if (pTransportUnit->getGroup()->hasMoved() || (pTransportUnit->getGroup()->headMissionQueueNode() != NULL))
				{
					getGroup()->pushMission(MISSION_SKIP);
					return false;
				}
			}
		}
	}

	if (AI_afterAttack())
	{
		return false;
	}

	//FfH: Added by Kael 10/26/2008
	if (!isBarbarian())
	{
		// Claim fort without mission if we can : MISSION_CLAIM_FORT Snarko 15/06/10
		claimFort();

		if (getLevel() < 2)
		{
			bool bDoesBuild = false;
			for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
			{
				BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI);
				if (NO_BUILDING != eBuilding)
				{
					if ((m_pUnitInfo->getForceBuildings(eBuilding)) || (m_pUnitInfo->getBuildings(eBuilding)))
					{
						bDoesBuild = true;
					}
				}
			}
			if (bDoesBuild)
			{
				if (AI_construct())
				{
					return false;
				}
			}
		}
	}
	/*************************************************************************************************/
	/**	Xienwolf Tweak							12/30/08											**/
	/**																								**/
	/**							Makes Enraged considerably more aggressive							**/
	/*************************************************************************************************/
	/**								---- Start Original Code ----									**
	if (isHuman())
	{
		if (getGroup()->getHeadUnit()->isAIControl())
		{
			if (AI_anyAttack(3, 20))
			{
				return true;
			}
			AI_barbAttackMove();
		}
	}
	/**								----  End Original Code  ----									**/
	if (getGroup()->getHeadUnit()->isAIControl())
	{
		if (AI_anyAttack(1, 90))
		{
			return true;
		}
		if (AI_anyAttack(1, 70))
		{
			return true;
		}
		if (AI_anyAttack(2, 50))
		{
			return true;
		}
		if (AI_anyAttack(4, 20))
		{
			return true;
		}
		AI_summonAttackMove();
		// Moving on here makes little sense (unit is automated but no automation type set) AI_summonAttackMove should be enough : AITweak Snarko 30/05/10
		return true;
	}
	/*************************************************************************************************/
	/**	Tweak									END													**/
	/*************************************************************************************************/
	//FfH: End Add

	if (getGroup()->isAutomated())
	{
		switch (getGroup()->getAutomateType())
		{
		case AUTOMATE_BUILD:
			if (AI_getUnitAIType() == UNITAI_WORKER)
			{
				AI_workerMove();
			}
			else if (AI_getUnitAIType() == UNITAI_WORKER_SEA)
			{
				AI_workerSeaMove();
			}
			else
			{
				FAssert(false);
			}
			break;

		case AUTOMATE_NETWORK:
			AI_networkAutomated();
			// XXX else wake up???
			break;

		case AUTOMATE_CITY:
			AI_cityAutomated();
			// XXX else wake up???
			break;

		case AUTOMATE_EXPLORE:
			switch (getDomainType())
			{
			case DOMAIN_SEA:
				AI_exploreSeaMove();
				break;

			case DOMAIN_AIR:
				// if we are cargo (on a carrier), hold if the carrier is not done moving yet
				pTransportUnit = getTransportUnit();
				if (pTransportUnit != NULL)
				{
					if (pTransportUnit->isAutomated() && pTransportUnit->canMove() && pTransportUnit->getGroup()->getActivityType() != ACTIVITY_HOLD)
					{
						getGroup()->pushMission(MISSION_SKIP);
						break;
					}
				}
				break;

			case DOMAIN_LAND:
				AI_exploreMove();
				break;

			default:
				FAssert(false);
				break;
			}

			// if we have air cargo (we are a carrier), and we done moving, explore with the aircraft as well
			if (hasCargo() && domainCargo() == DOMAIN_AIR && (!canMove() || getGroup()->getActivityType() == ACTIVITY_HOLD))
			{
				std::vector<CvUnit*> aCargoUnits;
				getCargoUnits(aCargoUnits);
				for (uint i = 0; i < aCargoUnits.size() && isAutomated(); ++i)
				{
					CvUnit* pCargoUnit = aCargoUnits[i];
					if (pCargoUnit->getDomainType() == DOMAIN_AIR)
					{
						if (pCargoUnit->canMove())
						{
							pCargoUnit->getGroup()->setAutomateType(AUTOMATE_EXPLORE);
							pCargoUnit->getGroup()->setActivityType(ACTIVITY_AWAKE);
						}
					}
				}
			}
			break;

		case AUTOMATE_RELIGION:
			if (AI_getUnitAIType() == UNITAI_MISSIONARY)
			{
				AI_missionaryMove();
			}
			break;

		default:
			FAssert(false);
			break;
		}

		// if no longer automated, then we want to bail
		return !getGroup()->isAutomated();
	}
	else
	{
		switch (AI_getUnitAIType())
		{
		case UNITAI_UNKNOWN:
			getGroup()->pushMission(MISSION_SKIP);
			break;

		case UNITAI_ANIMAL:
			AI_animalMove();
			break;

		case UNITAI_SETTLE:
			AI_settleMove();
			break;

		case UNITAI_WORKER:
			AI_workerMove();
			break;

		case UNITAI_ATTACK:

			//Added by Kael 09/19/2007
			if (getDuration() > 0)
			{
				AI_summonAttackMove();
				break;
			}
			//FfH: End Add

			if (isBarbarian())
			{
				AI_barbAttackMove();
			}
			else
			{
				AI_attackMove();
			}
			break;

		case UNITAI_ATTACK_CITY:
			//AI_attackCityMove();
			AI_ConquestMove();
			break;

		case UNITAI_COLLATERAL:
			AI_collateralMove();
			break;

		case UNITAI_PILLAGE:
			AI_pillageMove();
			break;

		case UNITAI_RESERVE:
			AI_reserveMove();
			break;

		case UNITAI_COUNTER:
			AI_counterMove();
			break;

		case UNITAI_PARADROP:
			AI_paratrooperMove();
			break;

		case UNITAI_CITY_DEFENSE:
			AI_cityDefenseMove();
			break;

		case UNITAI_CITY_COUNTER:
		case UNITAI_CITY_SPECIAL:
			AI_cityDefenseExtraMove();
			break;

		case UNITAI_EXPLORE:
			// Making barbarian units set to explore AI more aggressive : AITweak Snarko 07/10/12
			if (isBarbarian())
			{
				AI_barbExploreMove();
			}
			else
			{
				AI_exploreMove();
			}
			break;

		case UNITAI_MISSIONARY:
			AI_missionaryMove();
			break;

		case UNITAI_PROPHET:
			AI_prophetMove();
			break;

		case UNITAI_ARTIST:
			AI_artistMove();
			break;

		case UNITAI_SCIENTIST:
			AI_scientistMove();
			break;

		case UNITAI_GENERAL:
			AI_generalMove();
			break;

		case UNITAI_MERCHANT:
			AI_merchantMove();
			break;

		case UNITAI_ENGINEER:
			AI_engineerMove();
			break;

		case UNITAI_SPY:
			AI_spyMove();
			break;

		case UNITAI_ICBM:
			AI_ICBMMove();
			break;

		case UNITAI_WORKER_SEA:
			AI_workerSeaMove();
			break;

		case UNITAI_ATTACK_SEA:
			if (isBarbarian())
			{
				AI_barbAttackSeaMove();
			}
			else
			{
				AI_attackSeaMove();
			}
			break;

		case UNITAI_RESERVE_SEA:
			AI_reserveSeaMove();
			break;

		case UNITAI_ESCORT_SEA:
			AI_escortSeaMove();
			break;

		case UNITAI_EXPLORE_SEA:
			AI_exploreSeaMove();
			break;

		case UNITAI_ASSAULT_SEA:
			AI_assaultSeaMove();
			break;

		case UNITAI_SETTLER_SEA:
			AI_settlerSeaMove();
			break;

		case UNITAI_MISSIONARY_SEA:
			AI_missionarySeaMove();
			break;

		case UNITAI_SPY_SEA:
			AI_spySeaMove();
			break;

		case UNITAI_CARRIER_SEA:
			AI_carrierSeaMove();
			break;

		case UNITAI_MISSILE_CARRIER_SEA:
			AI_missileCarrierSeaMove();
			break;

		case UNITAI_PIRATE_SEA:
			AI_pirateSeaMove();
			break;

		case UNITAI_ATTACK_AIR:
			AI_attackAirMove();
			break;

		case UNITAI_DEFENSE_AIR:
			AI_defenseAirMove();
			break;

		case UNITAI_CARRIER_AIR:
			AI_carrierAirMove();
			break;

		case UNITAI_MISSILE_AIR:
			AI_missileAirMove();
			break;

		case UNITAI_ATTACK_CITY_LEMMING:
			AI_attackCityLemmingMove();
			break;
		case UNITAI_HERO:
			if (isBarbarian())
			{
				AI_barbAttackMove();
			}
			else
			{
				AI_ConquestMove();
				//AI_attackMove();
				//AI_heromove();
			}
			break;
		case UNITAI_INQUISITOR:
			AI_inquisitionMove();
			break;
		case UNITAI_FEASTING:
		//	AI_feastingmove();
			break;
		case UNITAI_WARWIZARD:
			AI_ConquestMove();
			break;
		case UNITAI_TERRAFORMER:
			AI_terraformerMove();
			break;
		case UNITAI_MAGE:
			//AI_mageMove();
			AI_cityDefenseMove();
			break;
		case UNITAI_MANA_UPGRADE:
			AI_upgrademanaMove();
			break;
		case UNITAI_FORT_COMMANDER:
			AI_fortcommanderMove();
			break;
		case UNITAI_HEALER:
			AI_healerMove();
			break;

		default:
			FAssert(false);
			break;
		}
	}

	return false;
}


// Returns true if took an action or should wait to move later...
bool CvUnitAI::AI_follow()
{
	if (AI_followBombard())
	{
		return true;
	}
	/************************************************************************************************/
	/* BETTER_BTS_AI_MOD                      03/31/10                              jdog5000        */
	/*                                                                                              */
	/* War tactics AI                                                                               */
	/************************************************************************************************/
	// Pushing MISSION_MOVE_TO missions when not all units could move resulted in stack being
	// broken up on the next turn.  Also, if we can't attack now we don't want to queue up an
	// attack for next turn, better to re-evaluate.
	bool bCanAllMove = getGroup()->canAllMove();

	if( bCanAllMove )
	{
		if (AI_cityAttack(1, 65, true))
		{
			return true;
		}
	}

	if (isEnemy(plot()->getTeam()))
	{
		if (canPillage(plot()))
		{
			getGroup()->pushMission(MISSION_PILLAGE);
			return true;
		}
	}

	if( bCanAllMove )
	{
		if (AI_anyAttack(1, 70, 2, true, true))
		{
			return true;
		}
	}
	/************************************************************************************************/
	/* BETTER_BTS_AI_MOD                       END                                                  */
	/************************************************************************************************/

	if (isFound())
	{
		if (area()->getBestFoundValue(getOwnerINLINE()) > 0)
		{
			if (AI_foundRange(FOUND_RANGE, true))
			{
				return true;
			}
		}
	}

	return false;
}


// XXX what if a unit gets stuck b/c of it's UnitAIType???
// XXX is this function costing us a lot? (it's recursive...)
void CvUnitAI::AI_upgrade()
{
	PROFILE_FUNC();

	FAssertMsg(!isHuman(), "isHuman did not return false as expected");
	FAssertMsg(AI_getUnitAIType() != NO_UNITAI, "AI_getUnitAIType() is not expected to be equal with NO_UNITAI");

	CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());
	UnitAITypes eUnitAI = AI_getUnitAIType();
	CvArea* pArea = area();

	int iCurrentValue = kPlayer.AI_unitValue(getUnitType(), eUnitAI, pArea);

	for (int iPass = 0; iPass < 2; iPass++)
	{
		int iBestValue = 0;
		UnitTypes eBestUnit = NO_UNIT;

		for (int iI = 0; iI < GC.getNumUnitInfos(); iI++)
		{
			if ((iPass > 0) || GC.getUnitInfo((UnitTypes)iI).getUnitAIType(AI_getUnitAIType()))
			{
				int iNewValue = kPlayer.AI_unitValue(((UnitTypes)iI), eUnitAI, pArea);
				if ((iPass == 0 || iNewValue > 0) && iNewValue > iCurrentValue)
				{
					if (canUpgrade((UnitTypes)iI))
					{
/*************************************************************************************************/
/**	AITweak								30/05/10							Snarko				**/
/**																								**/
/**			We do not take into account how much we value the unit? Completely random!			**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
						int iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Upgrade"));
/**								----  End Original Code  ----									**/
						int iValue = (1 + GC.getGameINLINE().getSorenRandNum(1000, "AI Upgrade"));
						iValue += iNewValue;
/*************************************************************************************************/
/**	AITweak									END													**/
/*************************************************************************************************/

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							eBestUnit = ((UnitTypes)iI);
						}
					}
				}
			}
		}

		if (eBestUnit != NO_UNIT)
		{
			upgrade(eBestUnit);
			doDelayedDeath();
			return;
		}
	}
}

/*************************************************************************************************/
/**	Speedup								12/02/12										Snarko	**/
/**																								**/
/**			Only store the unitclasses we upgrade to, for faster looping						**/
/*************************************************************************************************/
UnitTypes CvUnitAI::AI_getBestUpgradeUnit(UnitTypes eUnit, int* iBestValue, CvCity* pCity) const
{
	PROFILE("CvUnitAI::AI_getBestUpgradeUnit");
	FAssertMsg(eUnit != NO_UNIT, "eUnit is not expected to be NO_UNIT");

	if (GC.getUnitInfo(eUnit).getNumUpgradeUnitClass() == 0)
	{
		return NO_UNIT;
	}

	if (pCity == NULL)
	{
		pCity = GC.getMapINLINE().findCity(getX_INLINE(), getY_INLINE(), getOwnerINLINE(), NO_TEAM, true, getDomainType() == DOMAIN_SEA); //Could there ever be a situation where the closest city in our area is not coastal if this is DOMAIN_SEA anyway?
		if (pCity == NULL)
		{
			return NO_UNIT;
		}
	}

	int iValue;
	UnitTypes eBestUnit = NO_UNIT;
	UnitTypes eUpgradeUnit;
	UnitClassTypes eUpgradeClass;

	for (int iI = 0; iI < GC.getUnitInfo(eUnit).getNumUpgradeUnitClass(); iI++)
	{
		eUpgradeClass = (UnitClassTypes)GC.getUnitInfo(eUnit).getUpgradeUnitClass(iI);
		eUpgradeUnit = (UnitTypes)getUpgradeUnit(eUpgradeClass, eUnit, pCity);
		if (eUpgradeUnit != NO_UNIT)
		{
			if (canUpgrade(eUpgradeUnit))
			{
				iValue = GET_PLAYER(getOwnerINLINE()).AI_unitValue(eUpgradeUnit, AI_getUnitAIType(), area());
				if (iValue > *iBestValue)
				{
					*iBestValue = iValue;
					eBestUnit = eUpgradeUnit;
				}
			}
			eUpgradeUnit = AI_getBestUpgradeUnit(eUpgradeUnit, &iValue, pCity);
			if (eUpgradeUnit != NO_UNIT)
			{
				if (iValue > *iBestValue)
				{
					*iBestValue = iValue;
					eBestUnit = eUpgradeUnit;
				}
			}
		}
	}

	return eBestUnit;
}
/*************************************************************************************************/
/**	Speedup									END													**/
/*************************************************************************************************/


void CvUnitAI::AI_promote()
{
	PROFILE_FUNC();

	PromotionTypes eBestPromotion;
	int iValue;
	int iBestValue;
	int iI;

	iBestValue = 0;
	eBestPromotion = NO_PROMOTION;

	for (iI = 0; iI < GC.getNumPromotionInfos(); iI++)
	{
		if (canPromote((PromotionTypes)iI, -1))
		{
			iValue = AI_promotionValue((PromotionTypes)iI);

			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				eBestPromotion = ((PromotionTypes)iI);
			}
		}
	}

	if (eBestPromotion != NO_PROMOTION)
	{
		promote(eBestPromotion, -1);
		AI_promote();
	}
}

//Seems to determine which unit should control the Group for AI decisions.  Higher numbers mean higher priority.  Thus any group with a settler in it acts like a settler...
//Could write an AI type for the Commanders here pretty easily.  In CvUnitAI they will do nothing but look for the best Offensive Unit, go to him and join his group.
//Then make it so that the AI will always cast Join Commander if grouped with a commander?
//Make the weight of the Commander AI be insanely high if the Commander has space available in his command limit, but be 0 if he does not have space any longer.
//But then we need a function that will change out who is head of the group when the Commander gains/loses units...  Could use the ::addMinion function for that
//When a Minion is added or removed, you take the Commander out of his current group, add the minion, then re-group all of the units
//Problem is that the Commander is probably the Head unit, so when you take him out of the group the group will disband.  So I would need to re-write that function to prevent it
int CvUnitAI::AI_groupFirstVal()
{
	switch (AI_getUnitAIType())
	{
	case UNITAI_UNKNOWN:
	case UNITAI_ANIMAL:
		FAssert(false);
		break;

/*************************************************************************************************/
/**	Tweak							04/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	case UNITAI_SETTLE:
		return 21;
		break;

	case UNITAI_WORKER:
		return 20;
		break;

	case UNITAI_ATTACK:
		if (collateralDamage() > 0)
		{
			return 17;
		}
		else if (withdrawalProbability() > 0)
		{
			return 15;
		}
		else
		{
			return 13;
		}
		break;

	case UNITAI_ATTACK_CITY:
		if (bombardRate() > 0)
		{
			return 19;
		}
		else if (collateralDamage() > 0)
		{
			return 18;
		}
		else if (withdrawalProbability() > 0)
		{
			return 16;
		}
		else
		{
			return 14;
		}
		break;

	case UNITAI_COLLATERAL:
		return 7;
		break;

	case UNITAI_PILLAGE:
		return 12;
		break;

	case UNITAI_RESERVE:
		return 6;
		break;

	case UNITAI_COUNTER:
		return 5;
		break;

	case UNITAI_CITY_DEFENSE:
		return 3;
		break;

	case UNITAI_CITY_COUNTER:
		return 2;
		break;

	case UNITAI_CITY_SPECIAL:
		return 1;
		break;

	case UNITAI_PARADROP:
		return 4;
		break;

	case UNITAI_EXPLORE:
		return 8;
		break;

	case UNITAI_MISSIONARY:
		return 10;
		break;

	case UNITAI_PROPHET:
	case UNITAI_ARTIST:
	case UNITAI_SCIENTIST:
	case UNITAI_GENERAL:
	case UNITAI_MERCHANT:
	case UNITAI_ENGINEER:
		return 11;
		break;

	case UNITAI_SPY:
		return 9;
		break;

	case UNITAI_ICBM:
		break;

	case UNITAI_WORKER_SEA:
		return 8;
		break;

	case UNITAI_ATTACK_SEA:
		return 3;
		break;

	case UNITAI_RESERVE_SEA:
		return 2;
		break;

	case UNITAI_ESCORT_SEA:
		return 1;
		break;

	case UNITAI_EXPLORE_SEA:
		return 5;
		break;

	case UNITAI_ASSAULT_SEA:
		return 11;
		break;

	case UNITAI_SETTLER_SEA:
		return 9;
		break;

	case UNITAI_MISSIONARY_SEA:
		return 9;
		break;

	case UNITAI_SPY_SEA:
		return 10;
		break;

	case UNITAI_CARRIER_SEA:
		return 7;
		break;

	case UNITAI_MISSILE_CARRIER_SEA:
		return 6;
		break;

	case UNITAI_PIRATE_SEA:
		return 4;
		break;

	case UNITAI_ATTACK_AIR:
	case UNITAI_DEFENSE_AIR:
	case UNITAI_CARRIER_AIR:
	case UNITAI_MISSILE_AIR:
		break;

	case UNITAI_ATTACK_CITY_LEMMING:
		return 1;
		break;

	default:
		FAssert(false);
		break;
	}
/**								----  End Original Code  ----									**/
	case UNITAI_SETTLE:
		return 32;
		break;

	case UNITAI_WORKER:
		return 31;
		break;

	case UNITAI_ATTACK:
		if (canRangeStrike(true))
		{
			return 28;
		}
		if (collateralDamage() > 0)
		{
			return 26;
		}
		else if (withdrawalProbability() > 0)
		{
			return 24;
		}
		else
		{
			return 22;
		}
		break;

	case UNITAI_ATTACK_CITY:
		if (bombardRate() > 0)
		{
			return 30;
		}
		if (canRangeStrike(true))
		{
			return 29;
		}
		else if (collateralDamage() > 0)
		{
			return 27;
		}
		else if (withdrawalProbability() > 0)
		{
			return 25;
		}
		else
		{
			return 23;
		}
		break;

	case UNITAI_COLLATERAL:
		if (canRangeStrike(true))
		{
			return 14;
		}
		return 13;
		break;

	case UNITAI_PILLAGE:
		if (canRangeStrike(true))
		{
			return 21;
		}
		return 20;
		break;

	case UNITAI_RESERVE:
		if (canRangeStrike(true))
		{
			return 12;
		}
		return 11;
		break;

	case UNITAI_COUNTER:
		if (canRangeStrike(true))
		{
			return 10;
		}
		return 9;
		break;

	case UNITAI_CITY_DEFENSE:
		if (canRangeStrike(true))
		{
			return 6;
		}
		return 5;
		break;

	case UNITAI_CITY_COUNTER:
		if (canRangeStrike(true))
		{
			return 4;
		}
		return 3;
		break;

	case UNITAI_CITY_SPECIAL:
		if (canRangeStrike(true))
		{
			return 2;
		}
		return 1;
		break;

	case UNITAI_PARADROP:
		if (canRangeStrike(true))
		{
			return 8;
		}
		return 7;
		break;

	case UNITAI_EXPLORE:
		if (canRangeStrike(true))
		{
			return 16;
		}
		return 15;
		break;

	case UNITAI_MISSIONARY:
		return 18;
		break;

	case UNITAI_PROPHET:
	case UNITAI_ARTIST:
	case UNITAI_SCIENTIST:
	case UNITAI_GENERAL:
	case UNITAI_MERCHANT:
	case UNITAI_ENGINEER:
		return 19;
		break;

	case UNITAI_SPY:
		return 17;
		break;

	case UNITAI_ICBM:
		break;

	case UNITAI_WORKER_SEA:
		return 15;
		break;

	case UNITAI_ATTACK_SEA:
		if (canRangeStrike(true))
		{
			return 6;
		}
		return 5;
		break;

	case UNITAI_RESERVE_SEA:
		if (canRangeStrike(true))
		{
			return 4;
		}
		return 3;
		break;

	case UNITAI_ESCORT_SEA:
		if (canRangeStrike(true))
		{
			return 2;
		}
		return 1;
		break;

	case UNITAI_EXPLORE_SEA:
		if (canRangeStrike(true))
		{
			return 10;
		}
		return 9;
		break;

	case UNITAI_ASSAULT_SEA:
		if (canRangeStrike(true))
		{
			return 19;
		}
		return 18;
		break;

	case UNITAI_SETTLER_SEA:
		return 16;
		break;

	case UNITAI_MISSIONARY_SEA:
		return 16;
		break;

	case UNITAI_SPY_SEA:
		return 17;
		break;

	case UNITAI_CARRIER_SEA:
		if (canRangeStrike(true))
		{
			return 14;
		}
		return 13;
		break;

	case UNITAI_MISSILE_CARRIER_SEA:
		if (canRangeStrike(true))
		{
			return 12;
		}
		return 11;
		break;

	case UNITAI_PIRATE_SEA:
		if (canRangeStrike(true))
		{
			return 8;
		}
		return 7;
		break;

	case UNITAI_ATTACK_AIR:
	case UNITAI_DEFENSE_AIR:
	case UNITAI_CARRIER_AIR:
	case UNITAI_MISSILE_AIR:
		break;

	case UNITAI_ATTACK_CITY_LEMMING:
		return 1;
		break;

	case UNITAI_FEASTING:
		return 1;
		break;
	case UNITAI_MAGE:
	case UNITAI_MANA_UPGRADE:
	case UNITAI_TERRAFORMER:
	case UNITAI_WARWIZARD:
	case UNITAI_FORT_COMMANDER:

		if (canRangeStrike(true))
		{
			return 2;
		}
		return 1;
		break;

	case UNITAI_HERO:
		return 100; //Heroes don't like to get pushed around
		break;

	default:
		FAssert(false);
		break;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	return 0;
}


int CvUnitAI::AI_groupSecondVal()
{
	return ((getDomainType() == DOMAIN_AIR) ? airBaseCombatStr() : baseCombatStr());
}


// Returns attack odds out of 100 (the higher, the better...)
// Withdrawal odds included in returned value
int CvUnitAI::AI_attackOdds(const CvPlot* pPlot, bool bPotentialEnemy) const
{
	PROFILE_FUNC();

	CvUnit* pDefender;
	int iOurStrength;
	int iTheirStrength;
	int iOurFirepower;
	int iTheirFirepower;
	int iBaseOdds;
	int iStrengthFactor;
	int iDamageToUs;
	int iDamageToThem;
	int iNeededRoundsUs;
	int iNeededRoundsThem;
	int iHitLimitThem;

	pDefender = pPlot->getBestDefender(NO_PLAYER, getOwnerINLINE(), this, !bPotentialEnemy, bPotentialEnemy);

	if (pDefender == NULL)
	{
		return 100;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/21/10                                jdog5000      */
/*                                                                                              */
/* Efficiency, Lead From Behind                                                                 */
/************************************************************************************************/
	// From Lead From Behind by UncutDragon
	if (GC.getLFBEnable() && GC.getLFBUseCombatOdds())
	{
		// Combat odds are out of 1000 - we need odds out of 100
		CvUnit* pThis = (CvUnit*)this;
		int iOdds = (getCombatOdds(pThis, pDefender) + 5) / 10;
		iOdds += GET_PLAYER(getOwnerINLINE()).AI_getAttackOddsChange();

		return std::max(1, std::min(iOdds, 99));
	}

	iOurStrength = ((getDomainType() == DOMAIN_AIR) ? airCurrCombatStr(NULL) : currCombatStr(NULL, NULL));
	iOurFirepower = ((getDomainType() == DOMAIN_AIR) ? iOurStrength : currFirepower(NULL, NULL));

	if (iOurStrength == 0)
	{
		return 1;
	}

	iTheirStrength = pDefender->currCombatStr(pPlot, this);
	iTheirFirepower = pDefender->currFirepower(pPlot, this);


	FAssert((iOurStrength + iTheirStrength) > 0);
	FAssert((iOurFirepower + iTheirFirepower) > 0);

	iBaseOdds = (100 * iOurStrength) / (iOurStrength + iTheirStrength);
	if (iBaseOdds == 0)
	{
		return 1;
	}

	iStrengthFactor = ((iOurFirepower + iTheirFirepower + 1) / 2);

	// UncutDragon
	// original
	//iDamageToUs = std::max(1,((GC.getDefineINT("COMBAT_DAMAGE") * (iTheirFirepower + iStrengthFactor)) / (iOurFirepower + iStrengthFactor)));
	//iDamageToThem = std::max(1,((GC.getDefineINT("COMBAT_DAMAGE") * (iOurFirepower + iStrengthFactor)) / (iTheirFirepower + iStrengthFactor)));
	// modified
	iDamageToUs = std::max(1,((GC.getCOMBAT_DAMAGE() * (iTheirFirepower + iStrengthFactor)) / (iOurFirepower + iStrengthFactor)));
	iDamageToThem = std::max(1,((GC.getCOMBAT_DAMAGE() * (iOurFirepower + iStrengthFactor)) / (iTheirFirepower + iStrengthFactor)));
	// /UncutDragon
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	iHitLimitThem = pDefender->maxHitPoints() - combatLimit();

	iNeededRoundsUs = (std::max(0, pDefender->currHitPoints() - iHitLimitThem) + iDamageToThem - 1 ) / iDamageToThem;
	iNeededRoundsThem = (std::max(0, currHitPoints()) + iDamageToUs - 1 ) / iDamageToUs;

	if (getDomainType() != DOMAIN_AIR)
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/30/09                      Mongoose & jdog5000     */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
		// From Mongoose SDK
		if (!pDefender->immuneToFirstStrikes()) {
			iNeededRoundsUs   -= ((iBaseOdds * firstStrikes()) + ((iBaseOdds * chanceFirstStrikes()) / 2)) / 100;
		}
		if (!immuneToFirstStrikes()) {
			iNeededRoundsThem -= (((100 - iBaseOdds) * pDefender->firstStrikes()) + (((100 - iBaseOdds) * pDefender->chanceFirstStrikes()) / 2)) / 100;
		}
		iNeededRoundsUs   = std::max(1, iNeededRoundsUs);
		iNeededRoundsThem = std::max(1, iNeededRoundsThem);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	}

	int iRoundsDiff = iNeededRoundsUs - iNeededRoundsThem;
	if (iRoundsDiff > 0)
	{
		iTheirStrength *= (1 + iRoundsDiff);
	}
	else
	{
		iOurStrength *= (1 - iRoundsDiff);
	}

	int iOdds = (((iOurStrength * 100) / (iOurStrength + iTheirStrength)));
	iOdds += ((100 - iOdds) * std::min(GC.getDefineINT("MAX_WITHDRAWAL_PROBABILITY"), std::max(0, (withdrawalProbability() + pDefender->enemyWithdrawalProbability() + pDefender->getExtraEnemyWithdrawal())))) / 100;
	iOdds += GET_PLAYER(getOwnerINLINE()).AI_getAttackOddsChange();

//FfH: Added by Kael 04/11/2008
	if (isWorldUnitClass((UnitClassTypes)(m_pUnitInfo->getUnitClassType())) || getSpellCasterXP() > 0)
	{
		if (iOdds < 95)
		{
			iOdds = 0;
		}
	}
//FfH: End Add

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/30/09                      Mongoose & jdog5000     */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	// From Mongoose SDK
	return range(iOdds, 1, 99);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
}


// Returns true if the unit found a build for this city...
bool CvUnitAI::AI_bestCityBuild(CvCity* pCity, CvPlot** ppBestPlot, BuildTypes* peBestBuild, CvPlot* pIgnorePlot, CvUnit* pUnit)
{
	PROFILE_FUNC();

	BuildTypes eBuild;
	int iValue;
	int iBestValue;
	int iI;

	iBestValue = 0;
	BuildTypes eBestBuild = NO_BUILD;
	CvPlot* pBestPlot = NULL;

/*************************************************************************************************/
/**	K-mod merger								16/02/12								Snarko	**/
/**																								**/
/**					Merging in features of K-mod, most notably the pathfinder					**/
/*************************************************************************************************/
	// K-Mod. hack: For the AI, I want to use the standard pathfinder, CvUnit::generatePath.
	// but this function is also used to give action recommendations for the player
	// - and for that I do not want to disrupt the standard pathfinder. (because I'm paranoid about OOS bugs.)
	KmodPathFinder alt_finder;
	KmodPathFinder& pathFinder = getGroup()->AI_isControlled() ? CvSelectionGroup::path_finder : alt_finder;
	if (getGroup()->AI_isControlled())
	{
		// standard settings. cf. CvUnit::generatePath
		pathFinder.SetSettings(getGroup(), MOVE_SAFE_TERRITORY);
	}
	else
	{
		// like I said - this is only for action recommendations. It can be rough.
		pathFinder.SetSettings(getGroup(), 0, 5, GC.getMOVE_DENOMINATOR());
	}
	// K-Mod end

/*************************************************************************************************/
/**	K-mod merger								END												**/
/*************************************************************************************************/

	for (int iPass = 0; iPass < 2; iPass++)
	{
		for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
			CvPlot* pLoopPlot = plotCity(pCity->getX_INLINE(), pCity->getY_INLINE(), iI);

			if (pLoopPlot != NULL)
			{
				if (AI_plotValid(pLoopPlot))
				{
					if (pLoopPlot != pIgnorePlot)
					{
						if ((pLoopPlot->getImprovementType() == NO_IMPROVEMENT) || !(GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_SAFE_AUTOMATION) && !(pLoopPlot->getImprovementType() == (GC.getDefineINT("RUINS_IMPROVEMENT")))))
						{
							iValue = pCity->AI_getBestBuildValue(iI);

							if (iValue > iBestValue)
							{
								eBuild = pCity->AI_getBestBuild(iI);
								FAssertMsg(eBuild < GC.getNumBuildInfos(), "Invalid Build");

								if (eBuild != NO_BUILD)
								{
									if (0 == iPass)
									{
										iBestValue = iValue;
										pBestPlot = pLoopPlot;
										eBestBuild = eBuild;
									}
									else if (canBuild(pLoopPlot, eBuild))
									{
										if (!(pLoopPlot->isVisibleEnemyUnit(this)))
										{
/*************************************************************************************************/
/**	K-mod merger								16/02/12								Snarko	**/
/**																								**/
/**					Merging in features of K-mod, most notably the pathfinder					**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
											int iPathTurns;
/*************************************************************************************************/
/**	TipToe									04/04/09								Xienwolf	**/
/**																								**/
/**						Safe Movement is ideal situation where possible							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
											if (generatePath(pLoopPlot, 0, true, &iPathTurns))
/**								----  End Original Code  ----									**/
/**								---- Start Original Code ----									**
											if (generatePath(pLoopPlot, MOVE_SAFE_TERRITORY, true, &iPathTurns))
/*************************************************************************************************/
/**	TipToe									END													**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
											{
												// XXX take advantage of range (warning... this could lead to some units doing nothing...)
												int iMaxWorkers = 1;
												if (getPathLastNode()->m_iData1 == 0)
												{
													iPathTurns++;
												}
												else if (iPathTurns <= 1)
												{
													iMaxWorkers = AI_calculatePlotWorkersNeeded(pLoopPlot, eBuild);
												}
												if (pUnit != NULL)
												{
													if (pUnit->plot()->isCity() && iPathTurns == 1 && getPathLastNode()->m_iData1 > 0)
													{
														iMaxWorkers += 10;
													}
												}
/**								----  End Original Code  ----									**/
											// K-Mod. basically the same thing, but using pathFinder.
											if (pathFinder.GeneratePath(pLoopPlot))
											{
												FAStarNode* pEndNode = pathFinder.GetEndNode();
												int iPathTurns = pEndNode->m_iData2 + (pEndNode->m_iData1 == 0 ? 1 : 0);
												int iMaxWorkers = iPathTurns > 1 ? 1 : AI_calculatePlotWorkersNeeded(pLoopPlot, eBuild);
												if (pUnit && pUnit->plot()->isCity() && iPathTurns == 1)
													iMaxWorkers += 10;
											// K-Mod end
/*************************************************************************************************/
/**	K-mod merger								END												**/
/*************************************************************************************************/
												if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_BUILD, getGroup()) < iMaxWorkers)
												{
													//XXX this could be improved greatly by
													//looking at the real build time and other factors
													//when deciding whether to stack.
													iValue /= iPathTurns;

													iBestValue = iValue;
													pBestPlot = pLoopPlot;
													eBestBuild = eBuild;
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}

		if (0 == iPass)
		{
			if (eBestBuild != NO_BUILD)
			{
				FAssert(pBestPlot != NULL);
/*************************************************************************************************/
/**	K-mod merger								16/02/12								Snarko	**/
/**																								**/
/**					Merging in features of K-mod, most notably the pathfinder					**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
				int iPathTurns;
/*************************************************************************************************/
/**	TipToe									04/04/09								Xienwolf	**/
/**																								**/
/**						Safe Movement is ideal situation where possible							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
				if ((generatePath(pBestPlot, 0, true, &iPathTurns)) && canBuild(pBestPlot, eBestBuild)
/**								----  End Original Code  ----									**/
/**								---- Start Original Code ----									**
				if ((generatePath(pBestPlot, MOVE_SAFE_TERRITORY, true, &iPathTurns)) && canBuild(pBestPlot, eBestBuild)
/*************************************************************************************************/
/**	TipToe									END													**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
					&& !(pBestPlot->isVisibleEnemyUnit(this)))
				{
					int iMaxWorkers = 1;
					if (pUnit != NULL)
					{
						if (pUnit->plot()->isCity())
						{
							iMaxWorkers += 10;
						}
					}
					if (getPathLastNode()->m_iData1 == 0)
					{
						iPathTurns++;
					}
					else if (iPathTurns <= 1)
					{
						iMaxWorkers = AI_calculatePlotWorkersNeeded(pBestPlot, eBestBuild);
					}
/**								----  End Original Code  ----									**/
				// K-Mod. basically the same thing, but using pathFinder.
				if (pathFinder.GeneratePath(pBestPlot))
				{
					FAStarNode* pEndNode = pathFinder.GetEndNode();
					int iPathTurns = pEndNode->m_iData2 + (pEndNode->m_iData1 == 0 ? 1 : 0);
					int iMaxWorkers = iPathTurns > 1 ? 1 : AI_calculatePlotWorkersNeeded(pBestPlot, eBestBuild);
					if (pUnit && pUnit->plot()->isCity() && iPathTurns == 1)
						iMaxWorkers += 10;
				// K-Mod end
/*************************************************************************************************/
/**	K-mod merger								END												**/
/*************************************************************************************************/
					int iWorkerCount = GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pBestPlot, MISSIONAI_BUILD, getGroup());
					if (iWorkerCount < iMaxWorkers)
					{
						//Good to go.
						break;
					}
				}
				eBestBuild = NO_BUILD;
				iBestValue = 0;
			}
		}
	}

	if (NO_BUILD != eBestBuild)
	{
		FAssert(NULL != pBestPlot);
		if (ppBestPlot != NULL)
		{
			*ppBestPlot = pBestPlot;
		}
		if (peBestBuild != NULL)
		{
			*peBestBuild = eBestBuild;
		}
	}


	return (NO_BUILD != eBestBuild);
}


bool CvUnitAI::AI_isCityAIType() const
{
	return ((AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
			  (AI_getUnitAIType() == UNITAI_CITY_COUNTER) ||
					(AI_getUnitAIType() == UNITAI_CITY_SPECIAL) ||
						(AI_getUnitAIType() == UNITAI_RESERVE)||(AI_getUnitAIType()== UNITAI_MAGE));
}


int CvUnitAI::AI_getBirthmark() const
{
	return m_iBirthmark;
}


void CvUnitAI::AI_setBirthmark(int iNewValue)
{
	m_iBirthmark = iNewValue;
	if (AI_getUnitAIType() == UNITAI_EXPLORE_SEA)
	{
		if (GC.getGame().circumnavigationAvailable())
		{
			m_iBirthmark -= m_iBirthmark % 4; //creates a number with 2 lowest bits at 0 : XXXXX00
			int iExplorerCount = GET_PLAYER(getOwnerINLINE()).AI_getNumAIUnits(UNITAI_EXPLORE_SEA);
			iExplorerCount += getOwnerINLINE() % 4;
			if (GC.getMap().isWrapX())
			{
				if ((iExplorerCount % 2) == 1)
				{
					m_iBirthmark += 1;
				}
			}
			if (GC.getMap().isWrapY())
			{
				if (!GC.getMap().isWrapX())
				{
					iExplorerCount *= 2;
				}

				if (((iExplorerCount >> 1) % 2) == 1)
				{
					m_iBirthmark += 2;
				}
			}
		}
	}
}


UnitAITypes CvUnitAI::AI_getUnitAIType() const
{
	return m_eUnitAIType;
}


// XXX make sure this gets called...
void CvUnitAI::AI_setUnitAIType(UnitAITypes eNewValue)
{
	FAssertMsg(eNewValue != NO_UNITAI, "NewValue is not assigned a valid value");

	if (AI_getUnitAIType() != eNewValue)
	{
		area()->changeNumAIUnits(getOwnerINLINE(), AI_getUnitAIType(), -1);
		GET_PLAYER(getOwnerINLINE()).AI_changeNumAIUnits(AI_getUnitAIType(), -1);

		m_eUnitAIType = eNewValue;

		area()->changeNumAIUnits(getOwnerINLINE(), AI_getUnitAIType(), 1);
		GET_PLAYER(getOwnerINLINE()).AI_changeNumAIUnits(AI_getUnitAIType(), 1);

		joinGroup(NULL);
	}
}

int CvUnitAI::AI_sacrificeValue(const CvPlot* pPlot) const
{

/*************************************************************************************************/
/**	K-mod merger								16/02/12								Snarko	**/
/**																								**/
/**					Merging in features of K-mod, most notably the pathfinder					**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	int iValue;
/**								----  End Original Code  ----									**/
	long iValue; // K-Mod. (the int will overflow)
/*************************************************************************************************/
/**	K-mod merger								END												**/
/*************************************************************************************************/
	int iCollateralDamageValue = 0;
	if (pPlot != NULL)
	{
		int iPossibleTargets = std::min((pPlot->getNumVisibleEnemyDefenders(this) - 1), collateralDamageMaxUnits());

		if (iPossibleTargets > 0)
		{
			iCollateralDamageValue = collateralDamage();
			iCollateralDamageValue += std::max(0, iCollateralDamageValue - 100);
			iCollateralDamageValue *= iPossibleTargets;
			iCollateralDamageValue /= 5;
		}
	}

	if (getDomainType() == DOMAIN_AIR)
	{
		iValue = 128 * (100 + currInterceptionProbability());
		if (m_pUnitInfo->getNukeRange() != -1)
		{
			iValue += 25000;
		}
		iValue /= std::max(1, (1 + m_pUnitInfo->getProductionCost()));
/*************************************************************************************************/
/**	Higher hitpoints				01/02/11											Snarko	**/
/**						Makes higher values than 100 HP possible.								**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		iValue *= (maxHitPoints() - getDamage());
/**								----  End Original Code  ----									**/
		iValue *= (maxHitPoints() - getDamageReal()) / GC.getDefineINT("HIT_POINT_FACTOR"); //higher max hp != higher value
/*************************************************************************************************/
/**	Higher hitpoints						END													**/
/*************************************************************************************************/
		iValue /= 100;
	}
	else
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/14/10                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
/*
// original bts code
		iValue  = 128 * (currEffectiveStr(pPlot, ((pPlot == NULL) ? NULL : this)));
		iValue *= (100 + iCollateralDamageValue);
		iValue /= (100 + cityDefenseModifier());
		iValue *= (100 + withdrawalProbability());
		iValue /= std::max(1, (1 + m_pUnitInfo->getProductionCost()));
		iValue /= (10 + getExperience());
*/
		iValue  = 128 * (currEffectiveStr(pPlot, ((pPlot == NULL) ? NULL : this)));
		iValue *= (100 + iCollateralDamageValue);
		iValue /= (100 + cityDefenseModifier());
		iValue *= (100 + withdrawalProbability());
/*************************************************************************************************/
/**	K-mod merger								16/02/12								Snarko	**/
/**																								**/
/**					Merging in features of K-mod, most notably the pathfinder					**/
/*************************************************************************************************/
		iValue /= 100; // K-Mod
/*************************************************************************************************/
/**	K-mod merger								END												**/
/*************************************************************************************************/

		// Experience and medics now better handled in LFB
/*************************************************************************************************/
/**	DecimalXP								04/04/09								Xienwolf	**/
/**																								**/
/**								New Decimal Experience System									**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		iValue /= (10 + getExperience());
/**								----  End Original Code  ----									**/
		iValue /= (10 + getExperience()/100);
/*************************************************************************************************/
/**	DecimalXP								END													**/
/*************************************************************************************************/
		if( !GC.getLFBEnable() )
		{
			iValue *= 10;
			iValue /= std::max(1, (10 + getSameTileHeal() + getAdjacentTileHeal()));
		}

		// Value units which can't kill units later, also combat limits mean higher survival odds
		if (combatLimit() < 100)
		{
			iValue *= 150;
			iValue /= 100;

			iValue *= 100;
			iValue /= std::max(1, combatLimit());
		}

		iValue /= std::max(1, (1 + m_pUnitInfo->getProductionCost()));
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	}
/*************************************************************************************************/
/**	Xienwolf Tweak							01/04/09											**/
/**																								**/
/**								Corrects AI Unit value judgements								**/
/*************************************************************************************************/
	if (isImmortal())
	{
		iValue *= 5;
	}
	if (getDuration() > 0)
	{
		iValue *= 15/getDuration();
	}
	if (isLimitedUnitClass(getUnitClassType()))
	{
		iValue /= 3;
	}
	if (getExperience() < getFreeXPCap())
	{
		iValue /= (getFreeXPCap()/100);
	}
	if (getExperience() < getSpellCasterXP())
	{
		iValue /= (int)(2+getCasterXPRate());
	}
	if (isTargetWeakestUnit())
	{
		iValue *= (currEffectiveStr(pPlot, ((pPlot == NULL) ? NULL : this)));
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/14/10                                jdog5000      */
/*                                                                                              */
/* From Lead From Behind                                                                        */
/************************************************************************************************/
	// From Lead From Behind by UncutDragon
	if (GC.getLFBEnable())
	{
		// Reduce the value of sacrificing 'valuable' units - based on great general, limited, healer, experience
		iValue *= 100;
		int iRating = LFBgetRelativeValueRating();
		if (iRating > 0)
		{
			iValue /= (1 + 3*iRating);
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
/*************************************************************************************************/
/**	K-mod merger								16/02/12								Snarko	**/
/**																								**/
/**					Merging in features of K-mod, most notably the pathfinder					**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	return iValue;
/**								----  End Original Code  ----									**/
	return std::min((long)MAX_INT, iValue); // K-Mod
/*************************************************************************************************/
/**	K-mod merger								END												**/
/*************************************************************************************************/
}

// Protected Functions...

void CvUnitAI::AI_animalMove()
{
	PROFILE_FUNC();

//FfH: Added by Kael 10/26/2008 So that animals can built their pens...
	if (!isBarbarian())
	{
		if (getLevel() < 2)
		{
			if (AI_construct())
			{
				return;
			}
		}
	}
//FfH: End Add

	if (GC.getGameINLINE().getSorenRandNum(100, "Animal Attack") < GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getAnimalAttackProb())
	{
		if (AI_anyAttack(1, 0))
		{
			return;
		}
/*************************************************************************************************/
/**	Tweak							04/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
		if (AI_rangeAttack(airRange(), 100))
		{
			return;
		}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
	}

	if (AI_heal())
	{
		return;
	}

	if (AI_patrol())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_settleMove()
{
	PROFILE_FUNC();

	if (GET_PLAYER(getOwnerINLINE()).getNumCities() == 0 && (GC.getGame().getGameTurn() > 4))
	{
		if (canFound(plot()))
		{
			getGroup()->pushMission(MISSION_FOUND);
			return;
		}
	}
	if (AI_AddPopToCity())
		return;

	// For the few cases where getPlotDanger is still used, use a threshold where possible : Speedup Snarko 11/02/12
	if (GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 3, true, getGroup()->canFight(true, true)))
	{
		joinGroup(NULL);
		if (AI_retreatToCity())
		{
			return;
		}
		if (AI_safety())
		{
			return;
		}
		getGroup()->pushMission(MISSION_SKIP);
	}


	int iAreaBestFoundValue = 0;
	int iOtherBestFoundValue = 0;

	for (int iI = 0; iI < GET_PLAYER(getOwnerINLINE()).AI_getNumCitySites(); iI++)
	{
		CvPlot* pCitySitePlot = GET_PLAYER(getOwnerINLINE()).AI_getCitySite(iI);

		// Bugfix, settler AI : UNOFFICIAL_PATCH jdog5000 01/10/09
		/* original bts code
		if (pCitySitePlot->getArea() == getArea())
		*/
		// Only count city sites we can get to
		if ((pCitySitePlot->getArea() == getArea() || canMoveAllTerrain()) && generatePath(pCitySitePlot, MOVE_NO_ENEMY_TERRITORY, true))
		{
			if (plot() == pCitySitePlot)
			{
				if (canFound(plot()))
				{
					getGroup()->pushMission(MISSION_FOUND);
					return;
				}
			}
			iAreaBestFoundValue = std::max(iAreaBestFoundValue, pCitySitePlot->getFoundValue(getOwnerINLINE()));

		}
		else
		{
			iOtherBestFoundValue = std::max(iOtherBestFoundValue, pCitySitePlot->getFoundValue(getOwnerINLINE()));
		}
	}

	// No new settling of colonies when AI is in financial trouble (Gold AI) : BETTER_BTS_AI_MOD jdog5000 01/16/09
	if( plot()->isCity() && (plot()->getOwnerINLINE() == getOwnerINLINE()) )
	{
		if( GET_PLAYER(getOwnerINLINE()).AI_isFinancialTrouble() )
		{
			iOtherBestFoundValue = 0;
		}
	}

	if ((iAreaBestFoundValue == 0) && (iOtherBestFoundValue == 0))
	{
		if ((GC.getGame().getGameTurn() - getGameTurnCreated()) > 20)
		{
			if (NULL != getTransportUnit())
			{
				getTransportUnit()->unloadAll();
			}

			if (NULL == getTransportUnit())
			{
				//may seem wasteful, but settlers confuse the AI (Unit AI) : BETTER_BTS_AI_MOD jdog5000 11/30/08
				/* original bts code
				scrap();
				return;
				*/
				if( GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(getGroup()->getHeadUnit(), MISSIONAI_PICKUP) == 0 )
				{
					//may seem wasteful, but settlers confuse the AI.
					scrap();
					return;
				}
			}
		}
	}

	if ((iOtherBestFoundValue * 100) > (iAreaBestFoundValue * 110))
	{
		if (plot()->getOwnerINLINE() == getOwnerINLINE())
		{
			if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, NO_UNITAI, -1, -1, -1, 0, MOVE_SAFE_TERRITORY))
			{
				return;
			}
		}
	}

	/*************************************************************************************************/
	/**	K-mod merger								16/02/12								Snarko	**/
	/**																								**/
	/**					Merging in features of K-mod, most notably the pathfinder					**/
	/*************************************************************************************************/
	// disabled by K-Mod. We go to a lot of trouble to pick good city sites. Don't let this mess it up for us!
	/**								---- Start Original Code ----									**
	if ((iAreaBestFoundValue > 0) && plot()->isBestAdjacentFound(getOwnerINLINE()))
	{
		if (canFound(plot()))
		{
			getGroup()->pushMission(MISSION_FOUND);
			return;
		}
	}
	/**								----  End Original Code  ----									**/

	// Adapting AI to RifE : Improved AI Snarko 01/05/11
	/**								---- Start Original Code ----									**
	if (!GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_PEACE) && !GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) && !getGroup()->canDefend())
	/**								----  End Original Code  ----									**/
	if (!GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_PEACE) && (getGroup()->canFight(true, true) < 2))
	{
		if (AI_retreatToCity())
		{
			return;
		}
	}

	if (plot()->isCity() && (plot()->getOwnerINLINE() == getOwnerINLINE()))
	{
		// Unit AI, Efficiency : BETTER_BTS_AI_MOD jdog5000 08/20/09
		//if ((GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot()) > 0)
		if ((GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot()))
			&& (GC.getGameINLINE().getMaxCityElimination() > 0))
		{
			// Adapting AI to RifE : Improved AI Snarko 01/05/11
			/**								---- Start Original Code ----									**
			if (getGroup()->getNumUnits() < 3)
			/**								----  End Original Code  ----									**/
			if (getGroup()->canFight(true, true) < 2)
			{
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}
		}
	}

	if (iAreaBestFoundValue > 0)
	{
		if (AI_found())
		{
			return;
		}
	}

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, NO_UNITAI, -1, -1, -1, 0, MOVE_NO_ENEMY_TERRITORY))
		{
			return;
		}

		// BBAI TODO: Go to a good city (like one with a transport) ...
	}

	if (AI_retreatToCity())
	{
		return;
	}

	// BETTER_BTS_AI_MOD : Settler AI jdog5000 09/18/09
	if( getGroup()->isStranded() )
	{
		if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, NO_UNITAI, -1, -1, -1, -1, MOVE_NO_ENEMY_TERRITORY, 1))
		{
			return;
		}
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_workerMove()
{
	PROFILE_FUNC();

	CvCity* pCity;
	bool bCanRoute;
	bool bNextCity;

	bCanRoute = canBuildRoute();
	bNextCity = false;

	// XXX could be trouble...
	if (plot()->getOwnerINLINE() != getOwnerINLINE())
	{
/*************************************************************************************************/
/**	Improved AI							29/06/10										Snarko	**/
/**																								**/
/**								Making the AI build forts										**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		if (AI_retreatToCity())
		{
			return;
		}
/**								----  End Original Code  ----									**/
/*************************************************************************************************/
/**	Speedup								11/02/12										Snarko	**/
/**																								**/
/**		For the few cases where getPlotDanger is still used, use a threshold where possible		**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		if (GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 3) > getGroup()->canFight(true, true))
/**								----  End Original Code  ----									**/
		if (GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 3, true, getGroup()->canFight(true, true)))
/*************************************************************************************************/
/**	Speedup									END													**/
/*************************************************************************************************/
		{
			if (AI_retreatToCity())
			{
				return;
			}
		}
/*************************************************************************************************/
/**	Improved AI								END													**/
/*************************************************************************************************/
	}

	if (!isHuman())
	{
		if (plot()->getOwnerINLINE() == getOwnerINLINE())
		{
			if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, UNITAI_SETTLE, 2, -1, -1, 0, MOVE_SAFE_TERRITORY))
			{
				return;
			}
		}
	}

/*************************************************************************************************/
/**	Xienwolf Tweak							02/01/09											**/
/**																								**/
/**			First attempt at fixing the AI propensity for failing to run away with workers		**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	if (!(getGroup()->canDefend()))
/**								----  End Original Code  ----									**/
	if (!(getGroup()->canDefend()) || getGroup()->getHeadUnitAI() == UNITAI_WORKER)
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
	{
/*************************************************************************************************/
/**	Speedup								17/02/12										Snarko	**/
/**																								**/
/**	We have a function which does almost exactly the same thing, and uses a cache, so use that	**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		if (GET_PLAYER(getOwnerINLINE()).AI_isPlotThreatened(plot(), 2))
/**								----  End Original Code  ----									**/
		if (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2))
/*************************************************************************************************/
/**	Speedup									END													**/
/*************************************************************************************************/
		{
			if (AI_retreatToCity()) // XXX maybe not do this??? could be working productively somewhere else...
			{
				return;
			}
		}
	}

/*************************************************************************************************/
/**	Xienwolf Notes							NOTES												**/
/**	If the current plot belongs to you and has a properly improved/connectable bonus, connect it**/
/*************************************************************************************************/
	if (bCanRoute)
	{
		if (plot()->getOwnerINLINE() == getOwnerINLINE()) // XXX team???
		{
			BonusTypes eNonObsoleteBonus = plot()->getNonObsoleteBonusType(getTeam());
			if (NO_BONUS != eNonObsoleteBonus)
			{
				if (!(plot()->isConnectedToCapital()))
				{
					ImprovementTypes eImprovement = plot()->getImprovementType();
/*************************************************************************************************/
/**	TipToe									04/04/09								Xienwolf	**/
/**																								**/
/**						Safe Movement is ideal situation where possible							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
					if (NO_IMPROVEMENT != eImprovement && GC.getImprovementInfo(eImprovement).isImprovementBonusTrade(eNonObsoleteBonus))
/**								----  End Original Code  ----									**/
					if (NO_IMPROVEMENT != eImprovement && (GC.getImprovementInfo(eImprovement).isImprovementBonusTrade(eNonObsoleteBonus) || GC.getImprovementInfo(eImprovement).isActsAsCity()))
/*************************************************************************************************/
/**	TipToe									END													**/
/*************************************************************************************************/
					{
						if (AI_connectPlot(plot()))
						{
							return;
						}
					}
				}
			}
		}
	}

	CvPlot* pBestBonusPlot = NULL;
	BuildTypes eBestBonusBuild = NO_BUILD;
	int iBestBonusValue = 0;

/*************************************************************************************************/
/**	Xienwolf Notes							NOTES												**/
/**	Looks for best plot to improve, but only if the desire to improve it is LESS than 25 points?**/
/**			Not sure why you would want to avoid plots that you have more preference for...		**/
/*************************************************************************************************/
	if (AI_improveBonus(25, &pBestBonusPlot, &eBestBonusBuild, &iBestBonusValue))
	{
		return;
	}

	if (bCanRoute && !isBarbarian())
	{
		if (AI_connectCity())
		{
			return;
		}
	}

	pCity = NULL;

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		pCity = plot()->getPlotCity();
		if (pCity == NULL)
		{
			pCity = plot()->getWorkingCity();
		}
	}


//	if (pCity != NULL)
//	{
//		bool bMoreBuilds = false;
//		for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
//		{
//			CvPlot* pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), iI);
//			if ((iI != CITY_HOME_PLOT) && (pLoopPlot != NULL))
//			{
//				if (pLoopPlot->getWorkingCity() == pCity)
//				{
//					if (pLoopPlot->isBeingWorked())
//					{
//						if (pLoopPlot->getImprovementType() == NO_IMPROVEMENT)
//						{
//							if (pCity->AI_getBestBuildValue(iI) > 0)
//							{
//								ImprovementTypes eImprovement;
//								eImprovement = (ImprovementTypes)GC.getBuildInfo((BuildTypes)pCity->AI_getBestBuild(iI)).getImprovement();
//								if (eImprovement != NO_IMPROVEMENT)
//								{
//									bMoreBuilds = true;
//									break;
//								}
//							}
//						}
//					}
//				}
//			}
//		}
//
//		if (bMoreBuilds)
//		{
//			if (AI_improveCity(pCity))
//			{
//				return;
//			}
//		}
//	}
	if (pCity != NULL)
	{
		if ((pCity->AI_getWorkersNeeded() > 0) && (plot()->isCity() || (pCity->AI_getWorkersNeeded() < ((1 + pCity->AI_getWorkersHave() * 2) / 3))))
		{
			if (AI_improveCity(pCity))
			{
				return;
			}
		}
	}

	if (AI_improveLocalPlot(2, pCity))
	{
		return;
	}

//FfH: Modified by Kael 11/11/2007
//	bool bBuildFort = false;
//
//	if (GC.getGame().getSorenRandNum(5, "AI Worker build Fort with Priority"))
//	{
//		bool bCanal = ((100 * area()->getNumCities()) / std::max(1, GC.getGame().getNumCities()) < 85);
//		CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());
//		bool bAirbase = false;
//		bAirbase = (kPlayer.AI_totalUnitAIs(UNITAI_PARADROP) || kPlayer.AI_totalUnitAIs(UNITAI_ATTACK_AIR) || kPlayer.AI_totalUnitAIs(UNITAI_MISSILE_AIR));
//
//		if (bCanal || bAirbase)
//		{
//			if (AI_fortTerritory(bCanal, bAirbase))
//			{
//				return;
//			}
//		}
//		bBuildFort = true;
//	}
//FfH: End Modify

	if (bCanRoute && isBarbarian())
	{
		if (AI_connectCity())
		{
			return;
		}
	}

	if ((pCity == NULL) || (pCity->AI_getWorkersNeeded() == 0) || ((pCity->AI_getWorkersHave() > (pCity->AI_getWorkersNeeded() + 1))))
	{
		if ((pBestBonusPlot != NULL) && (iBestBonusValue >= 15))
		{
			if (AI_improvePlot(pBestBonusPlot, eBestBonusBuild))
			{
				return;
			}
		}

//		if (pCity == NULL)
//		{
//			pCity = GC.getMapINLINE().findCity(getX_INLINE(), getY_INLINE(), getOwnerINLINE()); // XXX do team???
//		}

		if (AI_nextCityToImprove(pCity))
		{
			return;
		}

		bNextCity = true;
	}

	if (pBestBonusPlot != NULL)
	{
		if (AI_improvePlot(pBestBonusPlot, eBestBonusBuild))
		{
			return;
		}
	}

	if (pCity != NULL)
	{
		if (AI_improveCity(pCity))
		{
			return;
		}
	}

	if (!bNextCity)
	{
		if (AI_nextCityToImprove(pCity))
		{
			return;
		}
	}

	if (bCanRoute)
	{
		if (AI_routeTerritory(true))
		{
			return;
		}

		if (AI_connectBonus(false))
		{
			return;
		}

		if (AI_routeCity())
		{
			return;
		}
	}

/*************************************************************************************************/
/**	Improved AI							16/06/10										Snarko	**/
/**																								**/
/**								Making the AI build forts										**/
/*************************************************************************************************/
	if (!isHuman())
	{
		if (AI_fort())
		{
			return;
		}
	}
/*************************************************************************************************/
/**	Improved AI								END													**/
/*************************************************************************************************/

	if (AI_irrigateTerritory())
	{
		return;
	}

//FfH: Modified by Kael 11/11/2007
//		if (!bBuildFort)
//		{
//			bool bCanal = ((100 * area()->getNumCities()) / std::max(1, GC.getGame().getNumCities()) < 85);
//			CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());
//			bool bAirbase = false;
//			bAirbase = (kPlayer.AI_totalUnitAIs(UNITAI_PARADROP) || kPlayer.AI_totalUnitAIs(UNITAI_ATTACK_AIR) || kPlayer.AI_totalUnitAIs(UNITAI_MISSILE_AIR));
//
//			if (bCanal || bAirbase)
//			{
//				if (AI_fortTerritory(bCanal, bAirbase))
//				{
//					return;
//				}
//			}
//		}
//FfH: End Modify

	if (bCanRoute)
	{
		if (AI_routeTerritory())
		{
			return;
		}
	}

	if (!isHuman() || (isAutomated() && GET_TEAM(getTeam()).getAtWarCount(true) == 0))
	{
		if (!isHuman() || (getGameTurnCreated() < GC.getGame().getGameTurn()))
		{
			if (AI_nextCityToImproveAirlift())
			{
				return;
			}
		}
		if (!isHuman())
		{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/14/09                                jdog5000      */
/*                                                                                              */
/* Worker AI                                                                                    */
/************************************************************************************************/
/*
			if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, NO_UNITAI, -1, -1, -1, -1, MOVE_SAFE_TERRITORY))
			{
				return;
			}
*/
			// Fill up boats which already have workers
			if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, UNITAI_WORKER, -1, -1, -1, -1, MOVE_SAFE_TERRITORY))
			{
				return;
			}

			// Avoid filling a galley which has just a settler in it, reduce chances for other ships
			if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, NO_UNITAI, -1, 2, -1, -1, MOVE_SAFE_TERRITORY))
			{
				return;
			}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		}
	}

	if (AI_improveLocalPlot(3, NULL))
	{
		return;
	}

	if (!(isHuman()) && (AI_getUnitAIType() == UNITAI_WORKER))
	{
		if (GC.getGameINLINE().getElapsedGameTurns() > 10)
		{
/*************************************************************************************************/
/**	Xienwolf Tweak							03/18/09											**/
/**																								**/
/**			Stops the AI from deleting workers that it will just try to rebuild anyway			**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
			if (GET_PLAYER(getOwnerINLINE()).AI_totalUnitAIs(UNITAI_WORKER) > GET_PLAYER(getOwnerINLINE()).getNumCities())
			{
				if (GET_PLAYER(getOwnerINLINE()).calculateUnitCost() > 0)
/**								----  End Original Code  ----									**/
			CvCity* pLoopCity;
			int iNumWorkersNeeded = 0;
			int iLoop = 0;
			for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
			{
				iNumWorkersNeeded += pLoopCity->AI_getWorkersNeeded();
			}
			if (GET_PLAYER(getOwnerINLINE()).AI_totalUnitAIs(UNITAI_WORKER) > iNumWorkersNeeded)
			{
				if (GET_PLAYER(getOwnerINLINE()).calculateUnitCost() > 0 && !(isFreeUnit() || getExtraSupport() < 0))
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
				{
					scrap();
					return;
				}
			}
		}
	}

	if (AI_retreatToCity(false, true))
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Worker AI                                                                                    */
/************************************************************************************************/
	if( getGroup()->isStranded() )
	{
		if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, NO_UNITAI, -1, -1, -1, -1, MOVE_NO_ENEMY_TERRITORY, 1))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_barbAttackMove()
{
	PROFILE_FUNC();

/*************************************************************************************************/
/**	Xienwolf Tweak							02/06/09											**/
/**																								**/
/**								Enhanced Barbarian Hero Logic									**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	if (AI_guardCity(false, true, 1))
	{
		return;
	}


	if (plot()->isGoody())
	{
		if (plot()->plotCount(PUF_isUnitAIType, UNITAI_ATTACK, -1, getOwnerINLINE()) == 1)
		{
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
	}

	if (GC.getGameINLINE().getSorenRandNum(2, "AI Barb") == 0)
	{
		if (AI_pillageRange(1))
		{
			return;
		}
	}

/**								----  End Original Code  ----									**/
	if (!isWorldUnitClass((UnitClassTypes)(m_pUnitInfo->getUnitClassType())))
	{
		if (plot()->isGoody())
		{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/15/10                                jdog5000      */
/*                                                                                              */
/* Barbarian AI                                                                                 */
/************************************************************************************************/
			if (AI_anyAttack(1, 90))
			{
				return;
			}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

			if (plot()->plotCount(PUF_isUnitAIType, UNITAI_ATTACK, -1, getOwnerINLINE()) == 1)
			{
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}
		}
	}
	else
	{
		if (AI_anyAttack(1, 90))
		{
			return;
		}
	}

	if (AI_cityAttack(1, 50))
	{
		return;
	}

	if (GC.getGameINLINE().getSorenRandNum(4, "Barb AI - Pillage Attempt") == 0)
	{
		if (AI_pillageRange(1))
		{
			return;
		}
	}

/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
	if (AI_anyAttack(1, 20))
	{
		return;
	}

/*************************************************************************************************/
/**	Tweak							04/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 60))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (GC.getGameINLINE().isOption(GAMEOPTION_RAGING_BARBARIANS))
	{
		if (AI_pillageRange(4))
		{
			return;
		}

		if (AI_cityAttack(3, 10))
		{
			return;
		}

		if (area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE)
		{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/15/10                                jdog5000      */
/*                                                                                              */
/* Barbarian AI                                                                                 */
/************************************************************************************************/
			if (AI_groupMergeRange(UNITAI_ATTACK, 1, true, true, true))
			{
				return;
			}

			if (AI_groupMergeRange(UNITAI_ATTACK_CITY, 3, true, true, true))
			{
				return;
			}
/*************************************************************************************************/
/**	Xienwolf Tweak							02/06/09											**/
/**																								**/
/**					Hopefully lets Barbarians move through Chokepoints with defenders			**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
			if (AI_targetCity())
/**								----  End Original Code  ----									**/
/**								---- Start Original BBAI Code ----								**
			if (AI_goToTargetCity(0, 12))
/**								----  End Original BBAI Code  ----								**/
			if (AI_goToTargetCity(MOVE_THROUGH_ENEMY | MOVE_IGNORE_DANGER, 12))
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
			{
				return;
			}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		}
	}

//FfH: Modified by Kael 10/11/2008
//	else if (GC.getGameINLINE().getNumCivCities() > (GC.getGameINLINE().countCivPlayersAlive() * 3))
//	{
//		if (AI_cityAttack(1, 15))
//		{
//			return;
//		}
//		if (AI_pillageRange(3))
//		{
//			return;
//		}
//		if (AI_cityAttack(2, 10))
//		{
//			return;
//		}
//		if (area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE)
//		{
//			if (AI_targetCity())
//			{
//				return;
//			}
//		}
//	}
//	else if (GC.getGameINLINE().getNumCivCities() > (GC.getGameINLINE().countCivPlayersAlive() * 2))
//	{
//		if (AI_pillageRange(2))
//		{
//			return;
//		}
//		if (AI_cityAttack(1, 10))
//		{
//			return;
//		}
//	}
//	if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 1))
//	{
//		return;
//	}
//	if (AI_heal())
//	{
//		return;
//	}
//	if (AI_guardCity(false, true, 2))
//	{
//		return;
//	}
	if (AI_cityAttack(1, 15))
	{
		return;
	}
	if (AI_pillageRange(3))
	{
		return;
	}
	if (AI_cityAttack(2, 10))
	{
		return;
	}
	if (area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE)
	{
/*************************************************************************************************/
/**	Xienwolf Tweak							02/06/09											**/
/**																								**/
/**					Hopefully lets Barbarians move through Chokepoints with defenders			**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
			if (AI_targetCity())
/**								----  End Original Code  ----									**/
/**								---- Start Original BBAI Code ----								**
		if (AI_goToTargetCity(0, 12))
/**								----  End Original BBAI Code  ----								**/
		if (AI_goToTargetCity(MOVE_THROUGH_ENEMY | MOVE_IGNORE_DANGER, 12))
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
		{
			return;
		}
	}
	if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 1))
	{
		return;
	}
	if (!isWorldUnitClass((UnitClassTypes)(m_pUnitInfo->getUnitClassType())))
	{
		if (AI_guardCity(false, true, 2))
		{
			return;
		}
	}
//FfH: End Modify
/*************************************************************************************************/
/**	Xienwolf Tweak							02/01/09											**/
/**																								**/
/**								Enhances Barbarian Aggression									**/
/*************************************************************************************************/
	if (AI_anyAttack(4, 20))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	Tweak							04/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 101))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (AI_patrol())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_attackMove()
{
	PROFILE_FUNC();

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/14/10                                jdog5000      */
/*                                                                                              */
/* Unit AI, Settler AI, Efficiency                                                              */
/************************************************************************************************/
/*************************************************************************************************/
/**	AI anti barb force					02/02/12							Snarko				**/
/**																								**/
/**					Helping the AI take out barbs, one unit at the time...						**/
/*************************************************************************************************/
	int iGroupMax = GET_PLAYER(getOwnerINLINE()).AI_getAntiBarbGroupSize(area());
/*************************************************************************************************/
/**	AI anti barb force						END													**/
/*************************************************************************************************/
	bool bDanger = (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 3));

	if (AI_AddPopToCity())
		return;

	if( getGroup()->getNumUnits() > 2 )
	{
		UnitAITypes eGroupAI = getGroup()->getHeadUnitAI();
		if( eGroupAI == AI_getUnitAIType() )
		{
			if( plot()->getOwnerINLINE() == getOwnerINLINE() && !bDanger )
			{
/*************************************************************************************************/
/**	AI anti barb force					02/02/12							Snarko				**/
/**																								**/
/**					Helping the AI take out barbs, one unit at the time...						**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
				if( getGroup()->countNumUnitAIType(UNITAI_ATTACK) > 2 )
				{
					getGroup()->AI_separate(); // will change group

					FAssert( eGroupAI == getGroup()->getHeadUnitAI() );
				}
/**								----  End Original Code  ----									**/
				if( getGroup()->countNumUnitAIType(UNITAI_ATTACK) > iGroupMax * 2 )
				{
					getGroup()->AI_separate(); // will change group

					FAssert( eGroupAI == getGroup()->getHeadUnitAI() );
				}
/*************************************************************************************************/
/**	AI anti barb force						END													**/
/*************************************************************************************************/

				// Should never have attack city group lead by attack unit
				if( getGroup()->countNumUnitAIType(UNITAI_ATTACK_CITY) > 0 )
				{
					getGroup()->AI_separateAI(UNITAI_ATTACK_CITY); // will change group

					// Since ATTACK can try to joing ATTACK_CITY again, need these units to
					// take a break to let ATTACK_CITY group move and avoid hang
					getGroup()->pushMission(MISSION_SKIP);
					return;
				}
			}
		}
	}


	// Attack choking units
	if( plot()->isCity() && plot()->getOwnerINLINE() == getOwnerINLINE() && bDanger )
	{
		int iOurDefense = GET_TEAM(getTeam()).AI_getOurPlotStrength(plot(),0,true,false,true);
		int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),2,false,false);

		if( iOurDefense < 3*iEnemyOffense )
		{
			if (AI_guardCity(true))
			{
				return;
			}
		}

		if( iOurDefense > 2*iEnemyOffense )
		{
			if (AI_anyAttack(2, 55))
			{
				return;
			}
		}

		if (AI_groupMergeRange(UNITAI_ATTACK, 1, true, true, false))
		{
			return;
		}

		if( iOurDefense > 2*iEnemyOffense )
		{
			if (AI_anyAttack(2, 30))
			{
				return;
			}
		}
	}

	{
		PROFILE("CvUnitAI::AI_attackMove() 1");

		// Guard a city we're in if it needs it
		if (AI_guardCity(true))
		{
			return;
		}

		if( !(plot()->isOwned()) )
		{
			// Group with settler after naval drop
			if( AI_groupMergeRange(UNITAI_SETTLE, 2, true, false, false) )
			{
				return;
			}
		}

		if( !(plot()->isOwned()) || (plot()->getOwnerINLINE() == getOwnerINLINE()) )
		{
			if( area()->getCitiesPerPlayer(getOwnerINLINE()) > GET_PLAYER(getOwnerINLINE()).AI_totalAreaUnitAIs(area(), UNITAI_CITY_DEFENSE) )
			{
				// Defend colonies in new world
				if (AI_guardCity(true, true, 3))
				{
					return;
				}
			}
		}

		if (AI_heal(30, 1))
		{
			return;
		}

		if (!bDanger)
		{
			if (AI_group(UNITAI_SETTLE, 1, -1, -1, false, false, false, 3, true))
			{
				return;
			}

			if (AI_group(UNITAI_SETTLE, 2, -1, -1, false, false, false, 3, true))
			{
				return;
			}
		}

		if (AI_guardCityAirlift())
		{
			return;
		}

		if (AI_guardCity(false, true, 1))
		{
			return;
		}

		//join any city attacks in progress
		if (plot()->isOwned() && plot()->getOwnerINLINE() != getOwnerINLINE())
		{
			if (AI_groupMergeRange(UNITAI_ATTACK_CITY, 1, true, true))
			{
				return;
			}
		}

		AreaAITypes eAreaAIType = area()->getAreaAIType(getTeam());
		if (plot()->isCity())
		{
			if (plot()->getOwnerINLINE() == getOwnerINLINE())
			{
				if ((eAreaAIType == AREAAI_ASSAULT) || (eAreaAIType == AREAAI_ASSAULT_ASSIST))
				{
					if (AI_offensiveAirlift())
					{
						return;
					}
				}
			}
		}

		if (bDanger)
		{
			if (AI_cityAttack(1, 55))
			{
				return;
			}

			if (AI_anyAttack(1, 65))
			{
				return;
			}

			if (collateralDamage() > 0)
			{
				if (AI_anyAttack(1, 45, 3))
				{
					return;
				}
			}
/*************************************************************************************************/
/**	Tweak							04/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
			if (AI_rangeAttack(airRange(), 60))
			{
				return;
			}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
		}
/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
		if (isInquisition())
		{
			if (AI_inquisitionMove(30))
			{
				return;
			}
		}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

		if (!noDefensiveBonus())
		{
			if (AI_guardCity(false, false))
			{
				return;
			}
		}

		if (!bDanger)
		{
			if (plot()->getOwnerINLINE() == getOwnerINLINE())
			{
				bool bAssault = ((eAreaAIType == AREAAI_ASSAULT) || (eAreaAIType == AREAAI_ASSAULT_MASSING) || (eAreaAIType == AREAAI_ASSAULT_ASSIST));
				if ( bAssault )
				{
					if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, UNITAI_ATTACK_CITY, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4))
					{
						return;
					}
				}

				if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, UNITAI_SETTLE, -1, -1, -1, 1, MOVE_SAFE_TERRITORY, 3))
				{
					return;
				}

				bool bLandWar = ((eAreaAIType == AREAAI_OFFENSIVE) || (eAreaAIType == AREAAI_DEFENSIVE) || (eAreaAIType == AREAAI_MASSING));
				if (!bLandWar)
				{
					// Fill transports before starting new one, but not just full of our unit ai
					if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, 1, -1, -1, 1, MOVE_SAFE_TERRITORY, 4))
					{
						return;
					}

					// Pick new transport which has space for other unit ai types to join
					if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, 2, -1, -1, MOVE_SAFE_TERRITORY, 4))
					{
						return;
					}
				}

				if (GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, MISSIONAI_GROUP) > 0)
				{
					getGroup()->pushMission(MISSION_SKIP);
					return;
				}
			}
		}

		// Allow larger groups if outside territory
		if( getGroup()->getNumUnits() < 3 )
		{
			if( plot()->isOwned() && GET_TEAM(getTeam()).isAtWar(plot()->getTeam()) )
			{
				if (AI_groupMergeRange(UNITAI_ATTACK, 1, true, true, true))
				{
					return;
				}
			}
		}

		if (AI_goody(3))
		{
			return;
		}

		if (AI_anyAttack(1, 70))
		{
			return;
		}
	}

	{
		PROFILE("CvUnitAI::AI_attackMove() 2");

		if (bDanger)
		{
			if (AI_pillageRange(1, 20))
			{
				return;
			}

			if (AI_cityAttack(1, 35))
			{
				return;
			}

			if (AI_anyAttack(1, 45))
			{
				return;
			}
/*************************************************************************************************/
/**	Tweak							04/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
			if (AI_rangeAttack(airRange(), 80))
			{
				return;
			}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

			if (AI_pillageRange(3, 20))
			{
				return;
			}

			if( getGroup()->getNumUnits() < 4 )
			{
				if (AI_choke(1))
				{
					return;
				}
			}
		}

		if (AI_goody(3))
		{
			return;
		}

		if (AI_anyAttack(1, 70))
		{
			return;
		}

/*************************************************************************************************/
/**	MISSION_CLAIM_FORT/MISSION_EXPLORE_LAIR	15/06/10 & 28/06/10							Snarko	**/
/**																								**/
/**						Adding a mission for the claim_fort action...							**/
/**							And teaching the AI how/when to do it								**/
/**									Likewise for explore_lair									**/
/*************************************************************************************************/
		if (AI_claimFort(3, 60))
		{
			return;
		}

		if (AI_exploreLair(3, 60))
		{
			return;
		}
/*************************************************************************************************/
/**	MISSION_CLAIM_FORT/MISSION_EXPLORE_LAIR				END										**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
		if (isInquisition())
		{
			if (AI_inquisitionMove(20))
			{
				return;
			}
		}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	AI anti barb force				12/04/11											Snarko	**/
/**					Helping the AI take out barbs, one unit at the time...						**/
/*************************************************************************************************/
		if (!bDanger && getGroup()->getNumUnits() < iGroupMax)
		{
			if (GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, MISSIONAI_GROUP) > 0)
			{
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}
			if (AI_group(AI_getUnitAIType(), iGroupMax, -1, -1, false, false, false, 3))
			{
				return;
			}
		}
/*************************************************************************************************/
/**	AI anti barb force						END													**/
/*************************************************************************************************/
	}

	{

		if (bDanger)
		{
			if (AI_cityAttack(4, 30))
			{
				return;
			}

			if (AI_anyAttack(2, 40))
			{
				return;
			}
		}

		if (!isEnemy(plot()->getTeam()))
		{
			if (AI_heal())
			{
				return;
			}
		}

		if ((GET_PLAYER(getOwnerINLINE()).AI_getNumAIUnits(UNITAI_CITY_DEFENSE) > 0) || (GET_TEAM(getTeam()).getAtWarCount(true) > 0))
		{
			// BBAI TODO: If we're fast, maybe shadow an attack city stack and pillage off of it

			bool bIgnoreFaster = false;
			if (GET_PLAYER(getOwnerINLINE()).AI_isDoStrategy(AI_STRATEGY_LAND_BLITZ))
			{
				if (area()->getAreaAIType(getTeam()) != AREAAI_ASSAULT)
				{
					bIgnoreFaster = true;
				}
			}

			if (AI_group(UNITAI_ATTACK_CITY, /*iMaxGroup*/ 1, /*iMaxOwnUnitAI*/ 1, -1, bIgnoreFaster, true, true, /*iMaxPath*/ 5))
			{
				return;
			}

			if (AI_group(UNITAI_ATTACK, /*iMaxGroup*/ 1, /*iMaxOwnUnitAI*/ 1, -1, true, true, false, /*iMaxPath*/ 4))
			{
				return;
			}

			// BBAI TODO: Need group to be fast, need to ignore slower groups
			//if (GET_PLAYER(getOwnerINLINE()).AI_isDoStrategy(AI_STRATEGY_FASTMOVERS))
			//{
			//	if (AI_group(UNITAI_ATTACK, /*iMaxGroup*/ 4, /*iMaxOwnUnitAI*/ 1, -1, true, false, false, /*iMaxPath*/ 3))
			//	{
			//		return;
			//	}
			//}

			if (AI_group(UNITAI_ATTACK, /*iMaxGroup*/ 1, /*iMaxOwnUnitAI*/ 1, -1, true, false, false, /*iMaxPath*/ 1))
			{
				return;
			}
		}

		if (area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE)
		{
			if (getGroup()->getNumUnits() > 1)
			{
				//if (AI_targetCity())
				if (AI_goToTargetCity(MOVE_AVOID_ENEMY_WEIGHT_2, 12))
				{
					return;
				}
			}
		}
		else if( area()->getAreaAIType(getTeam()) != AREAAI_DEFENSIVE )
		{
/*************************************************************************************************/
/**	MultiBarb							12/23/08									Xienwolf	**/
/**																								**/
/**							Adds extra Barbarian Civilizations									**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
			if (area()->getCitiesPerPlayer(BARBARIAN_PLAYER) > 0)
/**								----  End Original Code  ----									**/
			if (area()->getCitiesPerPlayer(ORC_PLAYER) + area()->getCitiesPerPlayer(ANIMAL_PLAYER) + area()->getCitiesPerPlayer(DEMON_PLAYER) > 0)
/*************************************************************************************************/
/**	MultiBarb								END													**/
/*************************************************************************************************/
			{
				if (getGroup()->getNumUnits() >= GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getBarbarianInitialDefenders())
				{
					if (AI_goToTargetBarbCity(10))
					{
						return;
					}
				}
			}
		}

		if (AI_guardCity(false, true, 3))
		{
			return;
		}

/*************************************************************************************************/
/**	AI anti barb force				12/04/11											Snarko	**/
/**					Helping the AI take out barbs, one unit at the time...						**/
/*************************************************************************************************/
		if (!bDanger && getGroup()->getNumUnits() < iGroupMax)
		{
			if (GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, MISSIONAI_GROUP) > 0)
			{
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}
			if (AI_group(AI_getUnitAIType(), iGroupMax, -1, -1, false, false, false, 3))
			{
				return;
			}
		}
/*************************************************************************************************/
/**	AI anti barb force						END													**/
/*************************************************************************************************/

		if ((GET_PLAYER(getOwnerINLINE()).getNumCities() > 1) && (getGroup()->getNumUnits() == 1))
		{
			if (area()->getAreaAIType(getTeam()) != AREAAI_DEFENSIVE)
			{
				if (area()->getNumUnrevealedTiles(getTeam()) > 0)
				{
					if (GET_PLAYER(getOwnerINLINE()).AI_areaMissionAIs(area(), MISSIONAI_EXPLORE, getGroup()) < (GET_PLAYER(getOwnerINLINE()).AI_neededExplorers(area()) + 1))
					{
/*************************************************************************************************/
/**	Tweak							26/01/12								Snarko				**/
/**																								**/
/**	Making explorers explore near their own territory, because anything else is too dangerous	**/
/**	Plus AI_explore can be really slow (check every plot on the map) so good to get away from it**/
/**	Also helping the AI take out barbs, one unit at the time...									**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
						if (AI_exploreRange(3))
						{
							return;
						}

						if (AI_explore())
						{
							return;
						}
/**								----  End Original Code  ----									**/
						if (getGroup()->getNumUnits() > iGroupMax * 2 / 3) //Roughly 2 units
						{
							if (AI_exploreRange(4))
							{
								return;
							}
						}
/*************************************************************************************************/
/**	Tweak								END														**/
/*************************************************************************************************/
					}
				}
			}
		}

/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
		if (isInquisition())
		{
			if (AI_inquisitionMove(10))
			{
				return;
			}
		}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

		if (AI_protect(35, 5))
		{
			return;
		}

		if (AI_offensiveAirlift())
		{
			return;
		}

		if (!bDanger && (area()->getAreaAIType(getTeam()) != AREAAI_DEFENSIVE))
		{
			if (plot()->getOwnerINLINE() == getOwnerINLINE())
			{
				if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, 1, -1, -1, 1, MOVE_SAFE_TERRITORY, 4))
				{
					return;
				}

				if( (GET_TEAM(getTeam()).getAtWarCount(true) > 0) && !(getGroup()->isHasPathToAreaEnemyCity(false)) )
				{
					if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4))
					{
						return;
					}
				}
			}
		}
		if (AI_AddPopToCity())
			return;

		if (AI_defend())
		{
			return;
		}

		if (AI_travelToUpgradeCity())
		{
			return;
		}
/*************************************************************************************************/
/**	Tweak							04/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
		if (AI_rangeAttack(airRange(), 101))
		{
			return;
		}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
		if( getGroup()->isStranded() )
		{
			if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_NO_ENEMY_TERRITORY, 1))
			{
				return;
			}
		}

		if( !bDanger && !isHuman() && plot()->isCoastalLand() && GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, MISSIONAI_PICKUP) > 0 )
		{
			// If no other desireable actions, wait for pickup
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
/*************************************************************************************************/
/**	AI anti barb force					02/02/12							Snarko				**/
/**																								**/
/**					Helping the AI take out barbs, one unit at the time...						**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		if( getGroup()->getNumUnits() < 4 )
		{
			if (AI_patrol())
			{
				return;
			}
		}
/**								----  End Original Code  ----									**/
		if (AI_patrol())
		{
			return;
		}
/*************************************************************************************************/
/**	AI anti barb force						END													**/
/*************************************************************************************************/


		if (AI_retreatToCity())
		{
			return;
		}

		if (AI_safety())
		{
			return;
		}
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
}


void CvUnitAI::AI_paratrooperMove()
{
	PROFILE_FUNC();

	bool bHostile = (plot()->isOwned() && isPotentialEnemy(plot()->getTeam()));
	if (!bHostile)
	{
		if (AI_guardCity(true))
		{
			return;
		}

		if (plot()->getTeam() == getTeam())
		{
			if (plot()->isCity())
			{
				if (AI_heal(30, 1))
				{
					return;
				}
			}

			AreaAITypes eAreaAIType = area()->getAreaAIType(getTeam());
			bool bLandWar = ((eAreaAIType == AREAAI_OFFENSIVE) || (eAreaAIType == AREAAI_DEFENSIVE) || (eAreaAIType == AREAAI_MASSING));
			if (!bLandWar)
			{
				if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, 0, MOVE_SAFE_TERRITORY, 4))
				{
					return;
				}
			}
		}

		if (AI_guardCity(false, true, 1))
		{
			return;
		}
	}

	if (AI_cityAttack(1, 45))
	{
		return;
	}

	if (AI_anyAttack(1, 55))
	{
		return;
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
		if (AI_rangeAttack(airRange(), 60))
		{
			return;
		}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (isInquisition())
	{
		if (AI_inquisitionMove(30))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

	if (!bHostile)
	{
		if (AI_paradrop(getDropRange()))
		{
			return;
		}

		if (AI_offensiveAirlift())
		{
			return;
		}

		if (AI_moveToStagingCity())
		{
			return;
		}

		if (AI_guardFort(true))
		{
			return;
		}

		if (AI_guardCityAirlift())
		{
			return;
		}
	}

	if (collateralDamage() > 0)
	{
		if (AI_anyAttack(1, 45, 3))
		{
			return;
		}
	}

	if (AI_pillageRange(1, 15))
	{
		return;
	}

	if (bHostile)
	{
		if (AI_choke(1))
		{
			return;
		}
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
		if (AI_rangeAttack(airRange(), 101))
		{
			return;
		}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (AI_heal())
	{
		return;
	}

/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (isInquisition())
	{
		if (AI_inquisitionMove(10))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

	if (AI_retreatToCity())
	{
		return;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	//if (AI_protect(35))
	if (AI_protect(35, 5))
	{
		return;
	}

	if( getGroup()->isStranded() )
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_NO_ENEMY_TERRITORY, 1))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/02/10                                jdog5000      */
/*                                                                                              */
/* War tactics AI, Barbarian AI                                                                 */
/************************************************************************************************/
void CvUnitAI::AI_attackCityMove()
{
	PROFILE_FUNC();

	AreaAITypes eAreaAIType = area()->getAreaAIType(getTeam());
	bool bLandWar = !isBarbarian() && ((eAreaAIType == AREAAI_OFFENSIVE) || (eAreaAIType == AREAAI_DEFENSIVE) || (eAreaAIType == AREAAI_MASSING));
	bool bAssault = !isBarbarian() && ((eAreaAIType == AREAAI_ASSAULT) || (eAreaAIType == AREAAI_ASSAULT_ASSIST) || (eAreaAIType == AREAAI_ASSAULT_MASSING));

	bool bTurtle = GET_PLAYER(getOwnerINLINE()).AI_isDoStrategy(AI_STRATEGY_TURTLE);
	bool bAlert1 = GET_PLAYER(getOwnerINLINE()).AI_isDoStrategy(AI_STRATEGY_ALERT1);
	bool bIgnoreFaster = false;
	if (GET_PLAYER(getOwnerINLINE()).AI_isDoStrategy(AI_STRATEGY_LAND_BLITZ))
	{
		if (!bAssault && area()->getCitiesPerPlayer(getOwnerINLINE()) > 0)
		{
			bIgnoreFaster = true;
		}
	}

	bool bInCity = plot()->isCity();

	if( bInCity && plot()->getOwnerINLINE() == getOwnerINLINE() )
	{
		// force heal if we in our own city and damaged
		// can we remove this or call AI_heal here?
		if ((getGroup()->getNumUnits() == 1) && (getDamage() > 0))
		{
			getGroup()->pushMission(MISSION_HEAL);
			return;
		}

		if( bIgnoreFaster )
		{
			// BBAI TODO: split out slow units ... will need to test to make sure this doesn't cause loops
		}

		if ((GC.getGame().getGameTurn() - plot()->getPlotCity()->getGameTurnAcquired()) <= 1)
		{
			CvSelectionGroup* pOldGroup = getGroup();

			pOldGroup->AI_separateNonAI(UNITAI_ATTACK_CITY);

			if (pOldGroup != getGroup())
			{
				return;
			}
		}

		if ((eAreaAIType == AREAAI_ASSAULT) || (eAreaAIType == AREAAI_ASSAULT_ASSIST))
		{
			if (AI_offensiveAirlift())
			{
				return;
			}
		}
	}

	bool bAtWar = isEnemy(plot()->getTeam());

	bool bHuntBarbs = false;
	if (!isBarbarian() && (area()->getCitiesPerPlayer(ORC_PLAYER) + area()->getCitiesPerPlayer(ANIMAL_PLAYER) + area()->getCitiesPerPlayer(DEMON_PLAYER) > 0))
	{
		if ((eAreaAIType != AREAAI_OFFENSIVE) && (eAreaAIType != AREAAI_DEFENSIVE) && !bAlert1 && !bTurtle)
		{
			bHuntBarbs = true;
		}
	}

	bool bReadyToAttack = false;
	if( !bTurtle )
	{
		bReadyToAttack = ((getGroup()->getNumUnits() >= ((bHuntBarbs) ? 3 : AI_stackOfDoomExtra())));
	}

	if( isBarbarian() )
	{
		bLandWar = (area()->getNumCities() - area()->getCitiesPerPlayer(BARBARIAN_PLAYER) > 0);
		bReadyToAttack = (getGroup()->getNumUnits() >= 3);
	}

	if( bReadyToAttack )
	{
		// Check that stack has units which can capture cities
		bReadyToAttack = false;
		int iCityCaptureCount = 0;

		CLLNode<IDInfo>* pUnitNode = getGroup()->headUnitNode();
		while (pUnitNode != NULL && !bReadyToAttack)
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = getGroup()->nextUnitNode(pUnitNode);

			if( !pLoopUnit->isOnlyDefensive() )
			{
				if( !(pLoopUnit->isCannotCapture(true)) && (pLoopUnit->combatLimit() >= 100) )
				{
					iCityCaptureCount++;

					if( iCityCaptureCount > 5 || 3*iCityCaptureCount > getGroup()->getNumUnits() )
					{
						bReadyToAttack = true;
					}
				}
			}
		}
	}


	if (AI_guardCity(false, false))
	{
		if( bReadyToAttack && (eAreaAIType != AREAAI_DEFENSIVE))
		{
			CvSelectionGroup* pOldGroup = getGroup();

			pOldGroup->AI_separateNonAI(UNITAI_ATTACK_CITY);
		}

		return;
	}

	if (AI_groupMergeRange(UNITAI_ATTACK_CITY, 0, true, true, bIgnoreFaster))
	{
		return;
	}

	CvCity* pTargetCity = NULL;
	if( isBarbarian() )
	{
		pTargetCity = AI_pickTargetCity(0, 12);
	}
	else
	{
		// BBAI TODO: Find some way of reliably targetting nearby cities with less defense ...
		pTargetCity = AI_pickTargetCity(0, MAX_INT, bHuntBarbs);
	}

	if( pTargetCity != NULL )
	{
		int iStepDistToTarget = stepDistance(pTargetCity->getX_INLINE(), pTargetCity->getY_INLINE(), getX_INLINE(), getY_INLINE());
		int iAttackRatio = std::max(100, GC.getBBAI_ATTACK_CITY_STACK_RATIO());

		if( isBarbarian() )
		{
			iAttackRatio = 80;
		}

		int iComparePostBombard = 0;
		// AI gets a 1-tile sneak peak to compensate for lack of memory
		if( iStepDistToTarget <= 2 || pTargetCity->isVisible(getTeam(),false) )
		{
			iComparePostBombard = getGroup()->AI_compareStacks(pTargetCity->plot(), true, true, true);

			int iDefenseModifier = pTargetCity->getDefenseModifier(true);
			int iBombardTurns = getGroup()->getBombardTurns(pTargetCity);
			iDefenseModifier *= std::max(0, 20 - iBombardTurns);
			iDefenseModifier /= 20;
			iComparePostBombard *= 100 + std::max(0, iDefenseModifier);
			iComparePostBombard /= 100;
		}

		if( iStepDistToTarget <= 2 )
		{
			if( iComparePostBombard < iAttackRatio )
			{
				if (AI_groupMergeRange(UNITAI_ATTACK_CITY, 2, true, true, bIgnoreFaster))
				{
					return;
				}

				int iOurOffense = GET_TEAM(getTeam()).AI_getOurPlotStrength(plot(),1,false,false,true);
				int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(pTargetCity->plot(),2,false,false);

				// If in danger, seek defensive ground
				if( 4*iOurOffense < 3*iEnemyOffense )
				{
					if( AI_choke(1, true) )
					{
						return;
					}
				}
			}

			if (iStepDistToTarget == 1)
			{
				// If next to target city and we would attack after bombarding down defenses,
				// or if defenses have crept up past half
				if( (iComparePostBombard >= iAttackRatio) || (pTargetCity->getDefenseDamage() < ((GC.getMAX_CITY_DEFENSE_DAMAGE() * 1) / 2)) )
				{
					if( (iComparePostBombard < std::max(150, GC.getDefineINT("BBAI_SKIP_BOMBARD_MIN_STACK_RATIO"))) )
					{
						// Move to good tile to attack from unless we're way more powerful
						if( AI_goToTargetCity(0,1,pTargetCity) )
						{
							return;
						}
					}

					// Bombard may skip if stack is powerful enough
					if (AI_bombardCity())
					{
						return;
					}

					//stack attack
					if (getGroup()->getNumUnits() > 1)
					{
						// BBAI TODO: What is right ratio?
						if (AI_stackAttackCity(1, iAttackRatio, true))
						{
							return;
						}
					}

					// If not strong enough alone, merge if another stack is nearby
					if (AI_groupMergeRange(UNITAI_ATTACK_CITY, 2, true, true, bIgnoreFaster))
					{
						return;
					}

					if( getGroup()->getNumUnits() == 1 )
					{
						if( AI_cityAttack(1, 50) )
						{
							return;
						}
					}
				}
			}
			if( iComparePostBombard < iAttackRatio )
			{
				// If not strong enough, pillage around target city without exposing ourselves
				if( AI_pillageRange(0) )
				{
					return;
				}

				if( AI_anyAttack(1, 60, 0, false) )
				{
					return;
				}

				if (AI_heal(30, 1))
				{
					return;
				}

				// Pillage around enemy city
				if( AI_pillageAroundCity(pTargetCity, 11, 3) )
				{
					return;
				}

				if( AI_pillageAroundCity(pTargetCity, 0, 5) )
				{
					return;
				}

				if( AI_choke(1) )
				{
					return;
				}
			}
			else
			{
				if( AI_goToTargetCity(0,4,pTargetCity) )
				{
					return;
				}
			}
		}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
		if (AI_rangeAttack(airRange(), 35))
		{
			return;
		}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
	}

/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (isInquisition())
	{
		if (AI_inquisitionMove(30))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

	if (AI_groupMergeRange(UNITAI_ATTACK_CITY, 2, true, true, bIgnoreFaster))
	{
		return;
	}

	if (AI_heal(30, 1))
	{
		return;
	}

	// BBAI TODO: Stack v stack combat ... definitely want to do in own territory, but what about enemy territory?
	if (collateralDamage() > 0 && plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		if (AI_anyAttack(1, 45, 3, false))
		{
			return;
		}

		if( !bReadyToAttack )
		{
			if (AI_anyAttack(1, 25, 5, false))
			{
				return;
			}
		}
	}

	if (AI_anyAttack(1, 60, 0, false))
	{
		return;
	}

	if (bAtWar && (getGroup()->getNumUnits() <= 2))
	{
		if (AI_pillageRange(3, 11))
		{
			return;
		}

		if (AI_pillageRange(1))
		{
			return;
		}
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 60))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (isInquisition())
	{
		if (AI_inquisitionMove(20))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		if (!bLandWar)
		{
			if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4))
			{
				return;
			}
		}

		if( bReadyToAttack )
		{
			// Wait for units about to join our group
			MissionAITypes eMissionAIType = MISSIONAI_GROUP;
			int iJoiners = GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, &eMissionAIType, 1, getGroup(), 2);

			if( (iJoiners*5) > getGroup()->getNumUnits() )
			{
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}
		}
		else
		{
			if( !isBarbarian() && (eAreaAIType == AREAAI_DEFENSIVE) )
			{
				// Use smaller attack city stacks on defense
				if (AI_guardCity(false, true, 3))
				{
					return;
				}
			}

			if( bTurtle )
			{
				if (AI_guardCity(false, true, 7))
				{
					return;
				}
			}

			int iTargetCount = GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, MISSIONAI_GROUP);
			if ((iTargetCount * 5) > getGroup()->getNumUnits())
			{
				MissionAITypes eMissionAIType = MISSIONAI_GROUP;
				int iJoiners = GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, &eMissionAIType, 1, getGroup(), 2);

				if( (iJoiners*5) > getGroup()->getNumUnits() )
				{
					getGroup()->pushMission(MISSION_SKIP);
					return;
				}

				if (AI_moveToStagingCity())
				{
					return;
				}
			}
		}
	}

	if (AI_heal(50, 3))
	{
		return;
	}

	if (!bAtWar)
	{
		if (AI_heal())
		{
			return;
		}

		if ((getGroup()->getNumUnits() == 1) && (getTeam() != plot()->getTeam()))
		{
			if (AI_retreatToCity())
			{
				return;
			}
		}
	}

	if (!bReadyToAttack && !noDefensiveBonus())
	{
		if (AI_guardCity(false, false))
		{
			return;
		}
	}

	bool bAnyWarPlan = (GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0);

	if (bReadyToAttack)
	{
		if( isBarbarian() )
		{
			if (AI_goToTargetCity(MOVE_AVOID_ENEMY_WEIGHT_2, 12))
			{
				return;
			}

			if (AI_pillageRange(3, 11))
			{
				return;
			}

			if (AI_pillageRange(1))
			{
				return;
			}
		}
		else if (bHuntBarbs && AI_goToTargetBarbCity((bAnyWarPlan ? 7 : 12)))
		{
			return;
		}
		else if (bLandWar && pTargetCity != NULL)
		{
			// Before heading out, check whether to wait to allow unit upgrades
			if( bInCity && plot()->getOwnerINLINE() == getOwnerINLINE() )
			{
				if( !(GET_PLAYER(getOwnerINLINE()).AI_isFinancialTrouble()) )
				{
					// Check if stack has units which can upgrade
					int iNeedUpgradeCount = 0;

					CLLNode<IDInfo>* pUnitNode = getGroup()->headUnitNode();
					while (pUnitNode != NULL)
					{
						CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
						pUnitNode = getGroup()->nextUnitNode(pUnitNode);

/*************************************************************************************************/
/**	Speedup								12/02/12										Snarko	**/
/**																								**/
/**			Only store the unitclasses we upgrade to, for faster looping						**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
						if( pLoopUnit->getUpgradeCity(false) != NULL )
/**								----  End Original Code  ----									**/
						int iValue = GET_PLAYER(getOwnerINLINE()).AI_unitValue(pLoopUnit->getUnitType(), pLoopUnit->AI_getUnitAIType(), area()); //I assume we're on the same area...
						if ( pLoopUnit->AI_getBestUpgradeUnit(pLoopUnit->getUnitType(), &iValue, plot()->getPlotCity()) != NULL)
/*************************************************************************************************/
/**	Speedup									END													**/
/*************************************************************************************************/
						{
							iNeedUpgradeCount++;

							if( 8*iNeedUpgradeCount > getGroup()->getNumUnits() )
							{
								getGroup()->pushMission(MISSION_SKIP);
								return;
							}
						}
					}
				}
			}

			if (AI_goToTargetCity(MOVE_AVOID_ENEMY_WEIGHT_2, 5, pTargetCity))
			{
				return;
			}

			if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 2, 2))
			{
				return;
			}

			if (AI_goToTargetCity(MOVE_AVOID_ENEMY_WEIGHT_2, 8, pTargetCity))
			{
				return;
			}

			// Load stack if walking will take a long time
			if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4, 3))
			{
				return;
			}

			if (AI_goToTargetCity(MOVE_AVOID_ENEMY_WEIGHT_2, 12, pTargetCity))
			{
				return;
			}

			if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4, 7))
			{
				return;
			}

			if (AI_goToTargetCity(MOVE_AVOID_ENEMY_WEIGHT_2, MAX_INT, pTargetCity))
			{
				return;
			}

			if (bAnyWarPlan)
			{
				CvCity* pTargetCity = area()->getTargetCity(getOwnerINLINE());

				if (pTargetCity != NULL)
				{
					if (AI_solveBlockageProblem(pTargetCity->plot(), (GET_TEAM(getTeam()).getAtWarCount(true) == 0)))
					{
						return;
					}
				}
			}
		}
	}
	else
	{
		int iTargetCount = GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, MISSIONAI_GROUP);
		if( ((iTargetCount * 4) > getGroup()->getNumUnits()) || ((getGroup()->getNumUnits() + iTargetCount) >= (bHuntBarbs ? 3 : AI_stackOfDoomExtra())) )
		{
			MissionAITypes eMissionAIType = MISSIONAI_GROUP;
			int iJoiners = GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, &eMissionAIType, 1, getGroup(), 2);

			if( (iJoiners*6) > getGroup()->getNumUnits() )
			{
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}

		if ((bombardRate() > 0) && noDefensiveBonus())
		{
			// BBAI Notes: Add this stack lead by bombard unit to stack probably not lead by a bombard unit
			// BBAI TODO: Some sense of minimum stack size?  Can have big stack moving 10 turns to merge with tiny stacks
			if (AI_group(UNITAI_ATTACK_CITY, -1, -1, -1, bIgnoreFaster, true, true, /*iMaxPath*/ 10, /*bAllowRegrouping*/ true))
			{
				return;
			}
		}
		else
		{
			if (AI_group(UNITAI_ATTACK_CITY, AI_stackOfDoomExtra() * 2, -1, -1, bIgnoreFaster, true, true, /*iMaxPath*/ 10, /*bAllowRegrouping*/ false))
			{
				return;
			}
		}
	}

	if (plot()->getOwnerINLINE() == getOwnerINLINE() && bLandWar)
	{
		if( (GET_TEAM(getTeam()).getAtWarCount(true) > 0) )
		{
			// if no land path to enemy cities, try getting there another way
			if (AI_offensiveAirlift())
			{
				return;
			}

			if( pTargetCity == NULL )
			{
				if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4))
				{
					return;
				}
			}
		}
	}

	if (AI_moveToStagingCity())
	{
		return;
	}

	if (AI_offensiveAirlift())
	{
		return;
	}

/*************************************************************************************************/
/**	MISSION_CLAIM_FORT/MISSION_EXPLORE_LAIR	15/06/10 & 28/06/10							Snarko	**/
/**																								**/
/**						Adding a mission for the claim_fort action...							**/
/**							And teaching the AI how/when to do it								**/
/**									Likewise for explore_lair									**/
/*************************************************************************************************/
	if (AI_claimFort(3, 60))
	{
		return;
	}

	if (AI_exploreLair(3, 60))
	{
		return;
	}
/*************************************************************************************************/
/**	MISSION_CLAIM_FORT/MISSION_EXPLORE_LAIR				END										**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 101))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (isInquisition())
	{
		if (AI_inquisitionMove(10))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

	if (AI_retreatToCity())
	{
		return;
	}

	if( getGroup()->isStranded() )
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_NO_ENEMY_TERRITORY, 1))
		{
			return;
		}

		if( !isHuman() && plot()->isCoastalLand() && GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, MISSIONAI_PICKUP) > 0 )
		{
			// If no other desireable actions, wait for pickup
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}

		if (AI_patrol())
		{
			return;
		}
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


void CvUnitAI::AI_attackCityLemmingMove()
{
	if (AI_cityAttack(1, 80))
	{
		return;
	}

	if (AI_bombardCity())
	{
		return;
	}

	if (AI_cityAttack(1, 40))
	{
		return;
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 60))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/29/10                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	if (AI_goToTargetCity(MOVE_THROUGH_ENEMY))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


	if (AI_anyAttack(1, 70))
	{
		return;
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 101))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (AI_anyAttack(1, 0))
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
}


void CvUnitAI::AI_collateralMove()
{
	PROFILE_FUNC();

	if (AI_leaveAttack(1, 20, 100))
	{
		return;
	}

	if (AI_guardCity(false, true, 1))
	{
		return;
	}

	if (AI_heal(30, 1))
	{
		return;
	}

	if (AI_cityAttack(1, 35))
	{
		return;
	}

	if (AI_anyAttack(1, 45, 3))
	{
		return;
	}

	if (AI_anyAttack(1, 55, 2))
	{
		return;
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 30))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (AI_anyAttack(1, 35, 3))
	{
		return;
	}

	if (AI_anyAttack(1, 30, 4))
	{
		return;
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 50))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (AI_anyAttack(1, 20, 5))
	{
		return;
	}

	if (AI_heal())
	{
		return;
	}

/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (isInquisition())
	{
		if (AI_inquisitionMove(30))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

	if (!noDefensiveBonus())
	{
		if (AI_guardCity(false, false))
		{
			return;
		}
	}

	if (AI_anyAttack(2, 55, 3))
	{
		return;
	}

	if (AI_cityAttack(2, 50))
	{
		return;
	}

	if (AI_anyAttack(2, 60))
	{
		return;
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 101))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (isInquisition())
	{
		if (AI_inquisitionMove(20))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/01/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	//if (AI_protect(50))
	if (AI_protect(50, 8))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	{
		return;
	}

	if (AI_guardCity(false, true, 3))
	{
		return;
	}

/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (isInquisition())
	{
		if (AI_inquisitionMove(10))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

	if (AI_retreatToCity())
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	if( getGroup()->isStranded() )
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_NO_ENEMY_TERRITORY, 1))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_pillageMove()
{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/05/10                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	PROFILE_FUNC();

	if (AI_guardCity(false, true, 1))
	{
		return;
	}

	if (AI_heal(30, 1))
	{
		return;
	}

	// BBAI TODO: Shadow ATTACK_CITY stacks and pillage

	//join any city attacks in progress
	if (plot()->isOwned() && plot()->getOwnerINLINE() != getOwnerINLINE())
	{
		if (AI_groupMergeRange(UNITAI_ATTACK_CITY, 1, true, true))
		{
			return;
		}
	}

	if (AI_cityAttack(1, 55))
	{
		return;
	}

	if (AI_anyAttack(1, 65))
	{
		return;
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 40))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (!noDefensiveBonus())
	{
		if (AI_guardCity(false, false))
		{
			return;
		}
	}

	if (AI_pillageRange(3, 11))
	{
		return;
	}

	if (AI_choke(1))
	{
		return;
	}

	if (AI_pillageRange(1))
	{
		return;
	}

/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (isInquisition())
	{
		if (AI_inquisitionMove(30))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, UNITAI_ATTACK, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4))
		{
			return;
		}
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 60))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (AI_heal(50, 3))
	{
		return;
	}

	if (!isEnemy(plot()->getTeam()))
	{
		if (AI_heal())
		{
			return;
		}
	}

/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (isInquisition())
	{
		if (AI_inquisitionMove(20))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

	if (AI_group(UNITAI_PILLAGE, /*iMaxGroup*/ 1, /*iMaxOwnUnitAI*/ 1, -1, /*bIgnoreFaster*/ true, false, false, /*iMaxPath*/ 3))
	{
		return;
	}

	if ((area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE) || isEnemy(plot()->getTeam()))
	{
		if (AI_pillage(20))
		{
			return;
		}
	}

/*************************************************************************************************/
/**	MISSION_CLAIM_FORT/MISSION_EXPLORE_LAIR	15/06/10 & 28/06/10							Snarko	**/
/**																								**/
/**						Adding a mission for the claim_fort action...							**/
/**							And teaching the AI how/when to do it								**/
/**									Likewise for explore_lair									**/
/*************************************************************************************************/
	if (AI_claimFort(3, 70))
	{
		return;
	}

	if (AI_exploreLair(3, 70))
	{
		return;
	}
/*************************************************************************************************/
/**	MISSION_CLAIM_FORT/MISSION_EXPLORE_LAIR				END										**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 101))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (AI_heal())
	{
		return;
	}

	if (AI_guardCity(false, true, 3))
	{
		return;
	}

	if (AI_offensiveAirlift())
	{
		return;
	}

	if (AI_travelToUpgradeCity())
	{
		return;
	}

/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (isInquisition())
	{
		if (AI_inquisitionMove(10))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

	if( !isHuman() && plot()->isCoastalLand() && GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, MISSIONAI_PICKUP) > 0 )
	{
		getGroup()->pushMission(MISSION_SKIP);
		return;
	}

	if (AI_patrol())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
}


void CvUnitAI::AI_reserveMove()
{
	PROFILE_FUNC();

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
	//bool bDanger = (GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 3) > 0);
	bool bDanger = (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 3));
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (bDanger && AI_leaveAttack(2, 55, 130))
	{
		return;
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 101))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (isInquisition())
	{
		if (AI_inquisitionMove(30))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, UNITAI_SETTLE, -1, -1, 1, -1, MOVE_SAFE_TERRITORY))
		{
			return;
		}
		if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, UNITAI_WORKER, -1, -1, 1, -1, MOVE_SAFE_TERRITORY))
		{
			return;
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Settler AI                                                                                   */
/************************************************************************************************/
	if( !(plot()->isOwned()) )
	{
		if (AI_group(UNITAI_SETTLE, 1, -1, -1, false, false, false, 1, true))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (!bDanger)
	{
		if (AI_group(UNITAI_SETTLE, 2, -1, -1, false, false, false, 3, true))
		{
			return;
		}
	}

	if (AI_guardCity(true))
	{
		return;
	}

/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (isInquisition())
	{
		if (AI_inquisitionMove(20))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

	if (!noDefensiveBonus())
	{
		if (AI_guardFort(false))
		{
			return;
		}
	}

	if (AI_guardCityAirlift())
	{
		return;
	}

	if (AI_guardCity(false, true, 1))
	{
		return;
	}

	if (AI_guardCitySite())
	{
		return;
	}

	if (!noDefensiveBonus())
	{
		if (AI_guardFort(true))
		{
			return;
		}

		if (AI_guardBonus(15))
		{
			return;
		}
	}

	if (AI_heal(30, 1))
	{
		return;
	}

	if (bDanger)
	{
		if (AI_cityAttack(1, 55))
		{
			return;
		}

		if (AI_anyAttack(1, 60))
		{
			return;
		}
	}

	if (!noDefensiveBonus())
	{
		if (AI_guardCity(false, false))
		{
			return;
		}
	}

	if (bDanger)
	{
		if (AI_cityAttack(3, 45))
		{
			return;
		}

		if (AI_anyAttack(3, 50))
		{
			return;
		}
	}

/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (isInquisition())
	{
		if (AI_inquisitionMove(10))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/01/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	//if (AI_protect(45))
	if (AI_protect(45, 8))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	{
		return;
	}

	if (AI_guardCity(false, true, 3))
	{
		return;
	}

	if (AI_defend())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	if( getGroup()->isStranded() )
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_NO_ENEMY_TERRITORY, 1))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_counterMove()
{
	PROFILE_FUNC();

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/03/10                                jdog5000      */
/*                                                                                              */
/* Unit AI, Settler AI                                                                          */
/************************************************************************************************/
	// Should never have group lead by counter unit
	if( getGroup()->getNumUnits() > 1 )
	{
		UnitAITypes eGroupAI = getGroup()->getHeadUnitAI();
		if( eGroupAI == AI_getUnitAIType() )
		{
			if( plot()->isCity() && plot()->getOwnerINLINE() == getOwnerINLINE() )
			{
				//FAssert(false); // just interested in when this happens, not a problem
				getGroup()->AI_separate(); // will change group
				return;
			}
		}
	}

	if( !(plot()->isOwned()) )
	{
		if( AI_groupMergeRange(UNITAI_SETTLE, 2, true, false, false) )
		{
			return;
		}
	}

	if (AI_guardCity(false, true, 1))
	{
		return;
	}

	if (getSameTileHeal() > 0)
	{
		if (!canAttack())
		{
			// Don't restrict to groups carrying cargo ... does this apply to any units in standard bts anyway?
			if (AI_shadow(UNITAI_ATTACK_CITY, -1, 21, false, false, 4))
			{
				return;
			}
		}
	}

	bool bDanger = (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 3));
	AreaAITypes eAreaAIType = area()->getAreaAIType(getTeam());

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		if( !bDanger )
		{
			if (plot()->isCity())
			{
				if ((eAreaAIType == AREAAI_ASSAULT) || (eAreaAIType == AREAAI_ASSAULT_ASSIST))
				{
					if (AI_offensiveAirlift())
					{
						return;
					}
				}
			}

			if( (eAreaAIType == AREAAI_ASSAULT) || (eAreaAIType == AREAAI_ASSAULT_ASSIST) || (eAreaAIType == AREAAI_ASSAULT_MASSING) )
			{
				if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, UNITAI_ATTACK_CITY, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4))
				{
					return;
				}

				if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, UNITAI_ATTACK, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4))
				{
					return;
				}
			}
		}

		if (!noDefensiveBonus())
		{
			if (AI_guardCity(false, false))
			{
				return;
			}
		}
	}

	//join any city attacks in progress
	if (plot()->getOwnerINLINE() != getOwnerINLINE())
	{
		if (AI_groupMergeRange(UNITAI_ATTACK_CITY, 1, true, true))
		{
			return;
		}
	}

	if (bDanger)
	{
		if (AI_cityAttack(1, 35))
		{
			return;
		}

		if (AI_anyAttack(1, 40))
		{
			return;
		}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
		if (AI_rangeAttack(airRange(), 60))
		{
			return;
		}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (isInquisition())
	{
		if (AI_inquisitionMove(20))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

	bool bIgnoreFasterStacks = false;
	if (GET_PLAYER(getOwnerINLINE()).AI_isDoStrategy(AI_STRATEGY_LAND_BLITZ))
	{
		if (area()->getAreaAIType(getTeam()) != AREAAI_ASSAULT)
		{
			bIgnoreFasterStacks = true;
		}
	}

	if (AI_group(UNITAI_ATTACK_CITY, /*iMaxGroup*/ -1, 2, -1, bIgnoreFasterStacks, /*bIgnoreOwnUnitType*/ true, /*bStackOfDoom*/ true, /*iMaxPath*/ 6))
	{
		return;
	}

	if (AI_group(UNITAI_ATTACK_CITY, /*iMaxGroup*/ -1, 2, -1, bIgnoreFasterStacks, /*bIgnoreOwnUnitType*/ true, /*bStackOfDoom*/ true, /*iMaxPath*/ 6))
	{
		return;
	}

	bool bFastMovers = (GET_PLAYER(getOwnerINLINE()).AI_isDoStrategy(AI_STRATEGY_FASTMOVERS));

	if (AI_group(UNITAI_ATTACK, /*iMaxGroup*/ 2, -1, -1, bFastMovers, /*bIgnoreOwnUnitType*/ true, /*bStackOfDoom*/ true, /*iMaxPath*/ 5))
	{
		return;
	}

	// BBAI TODO: merge with nearby pillage

	if (AI_guardCity(false, true, 3))
	{
		return;
	}

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		if( !bDanger )
		{
			if( (eAreaAIType != AREAAI_DEFENSIVE) )
			{
				if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, UNITAI_ATTACK_CITY, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4))
				{
					return;
				}
				if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, UNITAI_ATTACK, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4))
				{
					return;
				}
			}
		}
	}
/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 101))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (AI_heal())
	{
		return;
	}

	if (AI_offensiveAirlift())
	{
		return;
	}

/*************************************************************************************************/
/**	MISSION_CLAIM_FORT/MISSION_EXPLORE_LAIR	15/06/10 & 28/06/10							Snarko	**/
/**																								**/
/**						Adding a mission for the claim_fort action...							**/
/**							And teaching the AI how/when to do it								**/
/**									Likewise for explore_lair									**/
/*************************************************************************************************/
	if (AI_claimFort(3, 60))
	{
		return;
	}

	if (AI_exploreLair(3, 60))
	{
		return;
	}
/*************************************************************************************************/
/**	MISSION_CLAIM_FORT/MISSION_EXPLORE_LAIR				END										**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (isInquisition())
	{
		if (AI_inquisitionMove(10))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
}


void CvUnitAI::AI_cityDefenseMove()
{
	PROFILE_FUNC();

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
	//bool bDanger = (GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 3) > 0);
	bool bDanger = (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 3));
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Settler AI                                                                                   */
/************************************************************************************************/
	if( !(plot()->isOwned()) )
	{
		if (AI_group(UNITAI_SETTLE, 1, -1, -1, false, false, false, 2, true))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (bDanger)
	{
		if (AI_leaveAttack(1, 70, 175))
		{
			return;
		}

		if (AI_chokeDefend())
		{
			return;
		}
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 101))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (AI_guardCityBestDefender())
	{
		return;
	}

/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (!bDanger)
	{
		if (isInquisition())
		{
			if (AI_inquisitionMove(30))
			{
				return;
			}
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

	if (!bDanger)
	{
		if (plot()->getOwnerINLINE() == getOwnerINLINE())
		{
			if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, UNITAI_SETTLE, -1, -1, 1, -1, MOVE_SAFE_TERRITORY, 1))
			{
				return;
			}
		}
	}

	if (AI_guardCityMinDefender(true))
	{
		return;
	}

	if (AI_guardCity(true))
	{
		return;
	}

	if (!bDanger)
	{
/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
		if (isInquisition())
		{
			if (AI_inquisitionMove(20))
			{
				return;
			}
		}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	Xienwolf Tweak							04/09/09											**/
/**			Ensure each settler has up to 3 defenders (prioritizing the one least defended)		**/
/**						then try to ensure each worker has 1 defender							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		if (AI_group(UNITAI_SETTLE, /*iMaxGroup* 1, -1, -1, false, false, false, /*iMaxPath* 2, /*bAllowRegrouping* true))
		{
			return;
		}

		if (AI_group(UNITAI_SETTLE, /*iMaxGroup* 2, -1, -1, false, false, false, /*iMaxPath* 2, /*bAllowRegrouping* true))
		{
			return;
		}
/**								----  End Original Code  ----									**/
		if (AI_group(UNITAI_SETTLE, /*iMaxGroup*/ 1, -1, -1, false, false, false, /*iMaxPath*/ 2, /*bAllowRegrouping*/ true))
		{
			return;
		}

		if (AI_group(UNITAI_SETTLE, /*iMaxGroup*/ 2, -1, -1, false, false, false, /*iMaxPath*/ 2, /*bAllowRegrouping*/ true))
		{
			return;
		}

		if (AI_group(UNITAI_SETTLE, /*iMaxGroup*/ 3, -1, -1, false, false, false, /*iMaxPath*/ 2, /*bAllowRegrouping*/ true))
		{
			return;
		}

		if (AI_group(UNITAI_WORKER, /*iMaxGroup*/ 1, -1, -1, false, false, false, /*iMaxPath*/ 2, /*bAllowRegrouping*/ true))
		{
			return;
		}
/*************************************************************************************************/
/**	Tweak									25/01/12									Snarko	**/
/**																								**/
/**			The AI has a tendency to lose workers... defended workers...						**/
/*************************************************************************************************/
		if (AI_group(UNITAI_WORKER, /*iMaxGroup*/ 2, -1, -1, false, false, false, /*iMaxPath*/ 2, /*bAllowRegrouping*/ true))
		{
			return;
		}
/*************************************************************************************************/
/**	Tweak	 END																				**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
		if (plot()->getOwnerINLINE() == getOwnerINLINE())
		{
			if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, UNITAI_SETTLE, -1, -1, 1, -1, MOVE_SAFE_TERRITORY))
			{
				return;
			}
		}
	}

	AreaAITypes eAreaAI = area()->getAreaAIType(getTeam());
	if ((eAreaAI == AREAAI_ASSAULT) || (eAreaAI == AREAAI_ASSAULT_MASSING) || (eAreaAI == AREAAI_ASSAULT_ASSIST))
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, UNITAI_ATTACK_CITY, -1, -1, -1, 0, MOVE_SAFE_TERRITORY))
		{
			return;
		}
	}

	if ((AI_getBirthmark() % 4) == 0)
	{
		if (AI_guardFort())
		{
			return;
		}
	}

	if (AI_guardCityAirlift())
	{
		return;
	}

	if (AI_guardCity(false, true, 1))
	{
		return;
	}

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, UNITAI_SETTLE, 3, -1, -1, -1, MOVE_SAFE_TERRITORY))
		{
			// will enter here if in danger
			return;
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/02/10                                jdog5000      */
/*                                                                                              */
/* City AI                                                                                      */
/************************************************************************************************/
	//join any city attacks in progress
	if (plot()->getOwnerINLINE() != getOwnerINLINE())
	{
		if (AI_groupMergeRange(UNITAI_ATTACK_CITY, 1, true, true))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_guardCity(false, true))
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/04/10                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	if (!isBarbarian() && ((area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE) || (area()->getAreaAIType(getTeam()) == AREAAI_MASSING)))
	{
		bool bIgnoreFaster = false;
		if (GET_PLAYER(getOwnerINLINE()).AI_isDoStrategy(AI_STRATEGY_LAND_BLITZ))
		{
			if (area()->getAreaAIType(getTeam()) != AREAAI_ASSAULT)
			{
				bIgnoreFaster = true;
			}
		}

		if (AI_group(UNITAI_ATTACK_CITY, -1, 2, 4, bIgnoreFaster))
		{
			return;
		}
	}

	if (area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT)
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, UNITAI_ATTACK_CITY, 2, -1, -1, 1, MOVE_SAFE_TERRITORY))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (isInquisition())
	{
		if (AI_inquisitionMove(10))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

	if (AI_retreatToCity())
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	if( getGroup()->isStranded() )
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_NO_ENEMY_TERRITORY, 1))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_cityDefenseExtraMove()
{
	PROFILE_FUNC();

	CvCity* pCity;

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Settler AI                                                                                   */
/************************************************************************************************/
	if( !(plot()->isOwned()) )
	{
		if (AI_group(UNITAI_SETTLE, 1, -1, -1, false, false, false, 1, true))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_leaveAttack(2, 55, 150))
	{
		return;
	}

	if (AI_chokeDefend())
	{
		return;
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 101))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (AI_guardCityBestDefender())
	{
		return;
	}

	if (AI_guardCity(true))
	{
		return;
	}

/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (isInquisition())
	{
		if (AI_inquisitionMove(30))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

	if (AI_group(UNITAI_SETTLE, /*iMaxGroup*/ 1, -1, -1, false, false, false, /*iMaxPath*/ 2, /*bAllowRegrouping*/ true))
	{
		return;
	}

	if (AI_group(UNITAI_SETTLE, /*iMaxGroup*/ 2, -1, -1, false, false, false, /*iMaxPath*/ 2, /*bAllowRegrouping*/ true))
	{
		return;
	}

	pCity = plot()->getPlotCity();

	if ((pCity != NULL) && (pCity->getOwnerINLINE() == getOwnerINLINE())) // XXX check for other team?
	{
		if (plot()->plotCount(PUF_canDefendGroupHead, -1, -1, getOwnerINLINE(), NO_TEAM, PUF_isUnitAIType, AI_getUnitAIType()) == 1)
		{
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
	}

	if (AI_guardCityAirlift())
	{
		return;
	}

/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (isInquisition())
	{
		if (AI_inquisitionMove(20))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

	if (AI_guardCity(false, true, 1))
	{
		return;
	}

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		if (AI_load(UNITAI_SETTLER_SEA, MISSIONAI_LOAD_SETTLER, UNITAI_SETTLE, 3, -1, -1, -1, MOVE_SAFE_TERRITORY, 3))
		{
			return;
		}
	}

	if (AI_guardCity(false, true))
	{
		return;
	}

/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (isInquisition())
	{
		if (AI_inquisitionMove(10))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}

/*************************************************************************************************/
/**	AITweak								07/10/12							Snarko				**/
/**																								**/
/**		Making barbarian units set to explore AI more aggressive		**/
/*************************************************************************************************/
void CvUnitAI::AI_barbExploreMove()
{
	PROFILE_FUNC();

	if (!isWorldUnitClass((UnitClassTypes)(m_pUnitInfo->getUnitClassType())))
	{
		if (plot()->isGoody())
		{
			if (AI_anyAttack(1, 90))
			{
				return;
			}

			if ((plot()->plotCount(PUF_isUnitAIType, UNITAI_ATTACK, -1, getOwnerINLINE()) == 0) && (plot()->plotCount(PUF_isUnitAIType, UNITAI_EXPLORE, -1, getOwnerINLINE()) == 1)) //Have explore units defend them if there's no attack units doing it.
			{
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}
		}
	}
	else
	{
		if (AI_anyAttack(1, 90))
		{
			return;
		}
	}

	if (AI_cityAttack(1, 50))
	{
		return;
	}

	if (GC.getGameINLINE().getSorenRandNum(4, "Barb AI - Pillage Attempt") == 0)
	{
		if (AI_pillageRange(2))
		{
			return;
		}
	}

	if (AI_anyAttack(1, 20))
	{
		return;
	}

	if (AI_rangeAttack(airRange(), 60))
	{
		return;
	}

	if (GC.getGameINLINE().isOption(GAMEOPTION_RAGING_BARBARIANS))
	{
		if (AI_pillageRange(4))
		{
			return;
		}

		if (AI_cityAttack(3, 10))
		{
			return;
		}

		if (area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE)
		{
			if (AI_goToTargetCity(MOVE_THROUGH_ENEMY | MOVE_IGNORE_DANGER, 8))
			{
				return;
			}
		}
	}

	if (AI_cityAttack(1, 15))
	{
		return;
	}
	if (AI_pillageRange(3))
	{
		return;
	}
	if (AI_cityAttack(2, 10))
	{
		return;
	}

	if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 1))
	{
		return;
	}

	if (AI_group(UNITAI_EXPLORE, 2, -1, -1, true, false, false, 3))
	{
		return;
	}

	if (AI_exploreRange(4))
	{
		return;
	}

	if (!isWorldUnitClass((UnitClassTypes)(m_pUnitInfo->getUnitClassType())))
	{
		if (AI_guardCity(false, true, 2))
		{
			return;
		}
	}

	if (AI_anyAttack(4, 20))
	{
		return;
	}

	if (AI_rangeAttack(airRange(), 101))
	{
		return;
	}

	if (AI_patrol())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}
/*************************************************************************************************/
/**	AITweak									END													**/
/*************************************************************************************************/

void CvUnitAI::AI_exploreMove()
{
	PROFILE_FUNC();

	if (!isHuman() && canAttack())
	{
		if (AI_cityAttack(1, 60))
		{
			return;
		}

		if (AI_anyAttack(1, 70))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	Tweak							04/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (!isHuman())
	{
		if (AI_rangeAttack(airRange(), 30))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (getDamage() > 0)
	{
		if ((plot()->getFeatureType() == NO_FEATURE) || (GC.getFeatureInfo(plot()->getFeatureType()).getTurnDamage() == 0))
		{
/*************************************************************************************************/
/**	HealRight								04/18/09								Xienwolf	**/
/**																								**/
/**		Units will not attempt to heal in a location which will make such a thing impossible	**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
			getGroup()->pushMission(MISSION_HEAL);
			return;
/**								----  End Original Code  ----									**/
/*************************************************************************************************/
/**	Speedup								11/02/12										Snarko	**/
/**																								**/
/**				Use the must faster getAnyPlotDanger where possible								**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
			int iDangerLevel = GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot());
			if(iDangerLevel == 0)
/**								----  End Original Code  ----									**/
			if (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot()))
/*************************************************************************************************/
/**	Speedup									END													**/
/*************************************************************************************************/
			{
				if(healRate(plot()) > 5)
				{
					getGroup()->pushMission(MISSION_HEAL);
					return;
				}
				else
				{
					if(getDamage() > 40)
					{
						if (AI_retreatToCity())
						{
							return;
						}

						if (AI_safety())
						{
							return;
						}
						getGroup()->pushMission(MISSION_HEAL);
						return;
					}
				}
			}
			else
			{
				if(getDamage() > 20)
				{
					if (AI_retreatToCity())
					{
						return;
					}

					if (AI_safety())
					{
						return;
					}
				}
			}
/*************************************************************************************************/
/**	HealRight								END													**/
/*************************************************************************************************/
		}
	}

	if (!isHuman())
	{
		if (AI_pillageRange(1))
		{
			return;
		}

		if (AI_cityAttack(3, 80))
		{
			return;
		}
	}

	if (AI_goody(4))
	{
		return;
	}

/*************************************************************************************************/
/**	MISSION_CLAIM_FORT/MISSION_EXPLORE_LAIR	15/06/10 & 28/06/10							Snarko	**/
/**																								**/
/**						Adding a mission for the claim_fort action...							**/
/**							And teaching the AI how/when to do it								**/
/**									Likewise for explore_lair									**/
/*************************************************************************************************/
	if (AI_claimFort(4, 85))
	{
		return;
	}

	if (AI_exploreLair(4, 80))
	{
		return;
	}
/*************************************************************************************************/
/**	MISSION_CLAIM_FORT/MISSION_EXPLORE_LAIR				END										**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	AI anti barb force				12/04/11											Snarko	**/
/**					Helping the AI take out barbs, one unit at the time...						**/
/*************************************************************************************************/
	if (!isHuman())
	{
		int iGroupMax = GET_PLAYER(getOwnerINLINE()).AI_getAntiBarbGroupSize(area());
		if (getGroup()->getNumUnits() < iGroupMax)
		{
			if (AI_group(AI_getUnitAIType(), iGroupMax))
			{
				return;
			}
		}
	}
/*************************************************************************************************/
/**	AI anti barb force						END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (!isHuman())
	{
		if (isInquisition())
		{
			if (AI_inquisitionMove(30))
			{
				return;
			}
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	Tweak							26/01/12								Snarko				**/
/**																								**/
/**	Making explorers explore near their own territory, because anything else is too dangerous	**/
/**	Plus AI_explore can be really slow (check every plot on the map) so good to get away from it**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	if (AI_exploreRange(3))
	{
		return;
	}

	if (!isHuman())
	{
		if (AI_pillageRange(3))
		{
			return;
		}
	}


	if (AI_explore())
	{
		return;
	}
/**								----  End Original Code  ----									**/
	if (AI_exploreRange(4))
	{
		return;
	}

	if (!isHuman())
	{
		if (AI_pillageRange(3))
		{
			return;
		}
	}
/*************************************************************************************************/
/**	Tweak								END														**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (!isHuman())
	{
		if (isInquisition())
		{
			if (AI_inquisitionMove(20))
			{
				return;
			}
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

	if (!isHuman())
	{
		if (AI_pillage())
		{
			return;
		}
	}

	if (!isHuman())
	{
		if (AI_travelToUpgradeCity())
		{
			return;
		}

/*************************************************************************************************/
/**	Tweak							04/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
		if (AI_rangeAttack(airRange(), 80))
		{
			return;
		}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
	}

//FfH: Modified by Kael 08/21/2008
//	if (!isHuman() && (AI_getUnitAIType() == UNITAI_EXPLORE))
//	{
//		if (GET_PLAYER(getOwnerINLINE()).AI_totalAreaUnitAIs(area(), UNITAI_EXPLORE) > GET_PLAYER(getOwnerINLINE()).AI_neededExplorers(area()))
//		{
//			if (GET_PLAYER(getOwnerINLINE()).calculateUnitCost() > 0)
//			{
//				scrap();
//				return;
//			}
//		}
//	}
//FfH: End Add

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/03/08                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	if( !isHuman() && plot()->isCoastalLand() && GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, MISSIONAI_PICKUP) > 0 )
	{
		getGroup()->pushMission(MISSION_SKIP);
		return;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_patrol())
	{
		return;
	}

/*************************************************************************************************/
/**	MISSION_CLAIM_FORT/MISSION_EXPLORE_LAIR	15/06/10 & 28/06/10							Snarko	**/
/**																								**/
/**						Adding a mission for the claim_fort action...							**/
/**							And teaching the AI how/when to do it								**/
/**									Likewise for explore_lair									**/
/*************************************************************************************************/
	if (AI_claimFort(3, 60))
	{
		return;
	}

	if (AI_exploreLair(3, 60))
	{
		return;
	}
/*************************************************************************************************/
/**	MISSION_CLAIM_FORT/MISSION_EXPLORE_LAIR				END										**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (!isHuman())
	{
		if (isInquisition())
		{
			if (AI_inquisitionMove(10))
			{
				return;
			}
		}
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

	if (AI_retreatToCity())
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	if( getGroup()->isStranded() )
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_NO_ENEMY_TERRITORY, 1))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_missionaryMove()
{
	PROFILE_FUNC();

	if (AI_spreadReligion())
	{
		return;
	}

	if (AI_spreadCorporation())
	{
		return;
	}

	if (!isHuman() || (isAutomated() && GET_TEAM(getTeam()).getAtWarCount(true) == 0))
	{
		if (!isHuman() || (getGameTurnCreated() < GC.getGame().getGameTurn()))
		{
			if (AI_spreadReligionAirlift())
			{
				return;
			}
			if (AI_spreadCorporationAirlift())
			{
				return;
			}
		}

		if (!isHuman())
		{
			if (AI_load(UNITAI_MISSIONARY_SEA, MISSIONAI_LOAD_SPECIAL, NO_UNITAI, -1, -1, -1, 0, MOVE_SAFE_TERRITORY))
			{
				return;
			}

			if (AI_load(UNITAI_MISSIONARY_SEA, MISSIONAI_LOAD_SPECIAL, NO_UNITAI, -1, -1, -1, 0, MOVE_NO_ENEMY_TERRITORY))
			{
				return;
			}
		}
	}

	if (AI_retreatToCity())
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	if( getGroup()->isStranded() )
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_NO_ENEMY_TERRITORY, 1))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_prophetMove()
{
	PROFILE_FUNC();

	if (AI_construct(1))
	{
		return;
	}

	if (AI_discover(true, true))
	{
		return;
	}

	if (AI_construct(3))
	{
		return;
	}

	int iGoldenAgeValue = (GET_PLAYER(getOwnerINLINE()).AI_calculateGoldenAgeValue() / (GET_PLAYER(getOwnerINLINE()).unitsRequiredForGoldenAge()));
	int iDiscoverValue = std::max(1, getDiscoverResearch(NO_TECH));

	if (((iGoldenAgeValue * 100) / iDiscoverValue) > 60)
	{
		if (AI_goldenAge())
		{
			return;
		}

		if (iDiscoverValue > iGoldenAgeValue)
		{
			if (AI_discover())
			{
				return;
			}
			if (GET_PLAYER(getOwnerINLINE()).getUnitClassCount(getUnitClassType()) > 1)
			{
				if (AI_join())
				{
					return;
				}
			}
		}
	}
	else
	{
		if (AI_discover())
		{
			return;
		}

		if (AI_join())
		{
			return;
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
	//if ((GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 2) > 0) ||
	if ((GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2)) ||
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		  (getGameTurnCreated() < (GC.getGameINLINE().getGameTurn() - 25)))
	{
		if (AI_discover())
		{
			return;
		}
	}

	if (AI_retreatToCity())
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	if( getGroup()->isStranded() )
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_NO_ENEMY_TERRITORY, 1))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_artistMove()
{
	PROFILE_FUNC();

	if (AI_artistCultureVictoryMove())
	{
		return;
	}

	if (AI_construct())
	{
		return;
	}

	if (AI_discover(true, true))
	{
		return;
	}

	if (AI_greatWork())
	{
		return;
	}

	int iGoldenAgeValue = (GET_PLAYER(getOwnerINLINE()).AI_calculateGoldenAgeValue() / (GET_PLAYER(getOwnerINLINE()).unitsRequiredForGoldenAge()));
	int iDiscoverValue = std::max(1, getDiscoverResearch(NO_TECH));

	if (((iGoldenAgeValue * 100) / iDiscoverValue) > 60)
	{
		if (AI_goldenAge())
		{
			return;
		}

		if (iDiscoverValue > iGoldenAgeValue)
		{
			if (AI_discover())
			{
				return;
			}
			if (GET_PLAYER(getOwnerINLINE()).getUnitClassCount(getUnitClassType()) > 1)
			{
				if (AI_join())
				{
					return;
				}
			}
		}
	}
	else
	{
		if (AI_discover())
		{
			return;
		}

		if (AI_join())
		{
			return;
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
	//if ((GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 2) > 0) ||
	if ((GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2)) ||
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		  (getGameTurnCreated() < (GC.getGameINLINE().getGameTurn() - 25)))
	{
		if (AI_discover())
		{
			return;
		}
	}

	if (AI_retreatToCity())
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	if( getGroup()->isStranded() )
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_NO_ENEMY_TERRITORY, 1))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_scientistMove()
{
	PROFILE_FUNC();

	if (AI_discover(true, true))
	{
		return;
	}

	if (AI_construct(MAX_INT, 1))
	{
		return;
	}
	if (GET_PLAYER(getOwnerINLINE()).getCurrentEra() < 3)
	{
		if (AI_join(2))
		{
			return;
		}
	}

	if (GET_PLAYER(getOwnerINLINE()).getCurrentEra() <= (GC.getNumEraInfos() / 2))
	{
		if (AI_construct())
		{
			return;
		}
	}

	int iGoldenAgeValue = (GET_PLAYER(getOwnerINLINE()).AI_calculateGoldenAgeValue() / (GET_PLAYER(getOwnerINLINE()).unitsRequiredForGoldenAge()));
	int iDiscoverValue = std::max(1, getDiscoverResearch(NO_TECH));

	if (((iGoldenAgeValue * 100) / iDiscoverValue) > 60)
	{
		if (AI_goldenAge())
		{
			return;
		}

		if (iDiscoverValue > iGoldenAgeValue)
		{
			if (AI_discover())
			{
				return;
			}
			if (GET_PLAYER(getOwnerINLINE()).getUnitClassCount(getUnitClassType()) > 1)
			{
				if (AI_join())
				{
					return;
				}
			}
		}
	}
	else
	{
		if (AI_discover())
		{
			return;
		}

		if (AI_join())
		{
			return;
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
	//if ((GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 2) > 0) ||
	if ((GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2)) ||
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		  (getGameTurnCreated() < (GC.getGameINLINE().getGameTurn() - 25)))
	{
		if (AI_discover())
		{
			return;
		}
	}

	if (AI_retreatToCity())
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	if( getGroup()->isStranded() )
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_NO_ENEMY_TERRITORY, 1))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_generalMove()
{
	PROFILE_FUNC();

	std::vector<UnitAITypes> aeUnitAITypes;

/*************************************************************************************************/
/**	Speedup								11/02/12										Snarko	**/
/**																								**/
/**				Use the must faster getAnyPlotDanger where possible								**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	int iDanger = GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 2);

	bool bOffenseWar = (area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE);

	if (iDanger > 0)
/**								----  End Original Code  ----									**/
	bool bOffenseWar = (area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE);

	if (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2))
/*************************************************************************************************/
/**	Speedup									END													**/
/*************************************************************************************************/
	{
		aeUnitAITypes.clear();
		aeUnitAITypes.push_back(UNITAI_ATTACK);
		aeUnitAITypes.push_back(UNITAI_COUNTER);
		if (AI_lead(aeUnitAITypes))
		{
			return;
		}
	}

	if (AI_construct(1))
	{
		return;
	}
	if (AI_join(1))
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/14/10                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	if (bOffenseWar && (AI_getBirthmark() % 2 == 0))
	{
		aeUnitAITypes.clear();
		aeUnitAITypes.push_back(UNITAI_ATTACK_CITY);
		if (AI_lead(aeUnitAITypes))
		{
			return;
		}

		aeUnitAITypes.clear();
		aeUnitAITypes.push_back(UNITAI_ATTACK);
		if (AI_lead(aeUnitAITypes))
		{
			return;
		}
	}

	if (AI_join(2))
	{
		return;
	}

	if (AI_construct(2))
	{
		return;
	}
	if (AI_join(4))
	{
		return;
	}

	if (GC.getGameINLINE().getSorenRandNum(3, "AI General Construct") == 0)
	{
		if (AI_construct())
		{
			return;
		}
	}

	if (AI_join())
	{
		return;
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 101))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (AI_retreatToCity())
	{
		return;
	}

	if( getGroup()->isStranded() )
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_NO_ENEMY_TERRITORY, 1))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_merchantMove()
{
	PROFILE_FUNC();

	if (AI_construct())
	{
		return;
	}

	if (AI_discover(true, true))
	{
		return;
	}

	int iGoldenAgeValue = (GET_PLAYER(getOwnerINLINE()).AI_calculateGoldenAgeValue() / (GET_PLAYER(getOwnerINLINE()).unitsRequiredForGoldenAge()));
	int iDiscoverValue = std::max(1, getDiscoverResearch(NO_TECH));

	if (AI_trade(iGoldenAgeValue * 2))
	{
		return;
	}

	if (((iGoldenAgeValue * 100) / iDiscoverValue) > 60)
	{
		if (AI_goldenAge())
		{
			return;
		}

		if (AI_trade(iGoldenAgeValue))
		{
			return;
		}

		if (iDiscoverValue > iGoldenAgeValue)
		{
			if (AI_discover())
			{
				return;
			}
			if (GET_PLAYER(getOwnerINLINE()).getUnitClassCount(getUnitClassType()) > 1)
			{
				if (AI_join())
				{
					return;
				}
			}
		}
	}
	else
	{
		if (AI_discover())
		{
			return;
		}

		if (AI_join())
		{
			return;
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
	//if ((GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 2) > 0) ||
	if ((GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2)) ||
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		  (getGameTurnCreated() < (GC.getGameINLINE().getGameTurn() - 25)))
	{
		if (AI_discover())
		{
			return;
		}
	}

	if (AI_retreatToCity())
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	if( getGroup()->isStranded() )
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_NO_ENEMY_TERRITORY, 1))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_engineerMove()
{
	PROFILE_FUNC();

	if (AI_construct())
	{
		return;
	}

	if (AI_switchHurry())
	{
		return;
	}

	if (AI_hurry())
	{
		return;
	}

	if (AI_discover(true, true))
	{
		return;
	}

	int iGoldenAgeValue = (GET_PLAYER(getOwnerINLINE()).AI_calculateGoldenAgeValue() / (GET_PLAYER(getOwnerINLINE()).unitsRequiredForGoldenAge()));
	int iDiscoverValue = std::max(1, getDiscoverResearch(NO_TECH));

	if (((iGoldenAgeValue * 100) / iDiscoverValue) > 60)
	{
		if (AI_goldenAge())
		{
			return;
		}

		if (iDiscoverValue > iGoldenAgeValue)
		{
			if (AI_discover())
			{
				return;
			}
			if (GET_PLAYER(getOwnerINLINE()).getUnitClassCount(getUnitClassType()) > 1)
			{
				if (AI_join())
				{
					return;
				}
			}
		}
	}
	else
	{
		if (AI_discover())
		{
			return;
		}

		if (AI_join())
		{
			return;
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
	//if ((GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 2) > 0) ||
	if ((GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2)) ||
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		  (getGameTurnCreated() < (GC.getGameINLINE().getGameTurn() - 25)))
	{
		if (AI_discover())
		{
			return;
		}
	}

	if (AI_retreatToCity())
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	if( getGroup()->isStranded() )
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_NO_ENEMY_TERRITORY, 1))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/25/10                                jdog5000      */
/*                                                                                              */
/* Espionage AI                                                                                 */
/************************************************************************************************/
void CvUnitAI::AI_spyMove()
{
	PROFILE_FUNC();

	CvTeamAI& kTeam = GET_TEAM(getTeam());
	int iEspionageChance = 0;
	if (plot()->isOwned() && (plot()->getTeam() != getTeam()))
	{
		switch (GET_PLAYER(getOwnerINLINE()).AI_getAttitude(plot()->getOwnerINLINE()))
		{
		case ATTITUDE_FURIOUS:
			iEspionageChance = 100;
			break;

		case ATTITUDE_ANNOYED:
			iEspionageChance = 50;
			break;

		case ATTITUDE_CAUTIOUS:
			iEspionageChance = (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 30 : 10);
			break;

		case ATTITUDE_PLEASED:
			iEspionageChance = (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 20 : 0);
			break;

		case ATTITUDE_FRIENDLY:
			iEspionageChance = 0;
			break;

		default:
			FAssert(false);
			break;
		}

		WarPlanTypes eWarPlan = kTeam.AI_getWarPlan(plot()->getTeam());
		if (eWarPlan != NO_WARPLAN)
		{
			if (eWarPlan == WARPLAN_LIMITED)
			{
				iEspionageChance += 50;
			}
			else
			{
				iEspionageChance += 20;
			}
		}

		if (plot()->isCity() && plot()->getTeam() != getTeam())
		{
			bool bTargetCity = false;

			// would we have more power if enemy defenses were down?
			int iOurPower = GET_PLAYER(getOwnerINLINE()).AI_getOurPlotStrength(plot(),1,false,true);
			int iEnemyPower = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),0,false,false);

			if( 5*iOurPower > 6*iEnemyPower && eWarPlan != NO_WARPLAN )
			{
				bTargetCity = true;

				if( AI_revoltCitySpy() )
				{
					return;
				}

				if (GC.getGame().getSorenRandNum(5, "AI Spy Skip Turn") > 0)
				{
					getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY);
					return;
				}

				if ( AI_cityOffenseSpy(5, plot()->getPlotCity()) )
				{
					return;
				}
			}

			if( GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(plot(), MISSIONAI_ASSAULT, getGroup()) > 0 )
			{
				bTargetCity = true;

				getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY);
				return;
			}

			if( !bTargetCity )
			{
				// normal city handling
				if (getFortifyTurns() >= GC.getDefineINT("MAX_FORTIFY_TURNS"))
				{
					if (AI_espionageSpy())
					{
						return;
					}
				}
				else if (GC.getGame().getSorenRandNum(100, "AI Spy Skip Turn") > 5)
				{
					// don't get stuck forever
					getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY);
					return;
				}
			}
		}
		else if (GC.getGameINLINE().getSorenRandNum(100, "AI Spy Espionage") < iEspionageChance)
		{
			// This applies only when not in an enemy city, so for destroying improvements
			if (AI_espionageSpy())
			{
				return;
			}
		}
	}

	if (plot()->getTeam() == getTeam())
	{
		if (kTeam.getAnyWarPlanCount(true) == 0 || GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_SPACE, 4) || GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE, 3))
		{
			if( GC.getGame().getSorenRandNum(10, "AI Spy defense") > 0)
			{
				if (AI_guardSpy(0))
				{
					return;
				}
			}
		}

		if (GC.getGame().getSorenRandNum(100, "AI Spy pillage improvement") < 25)
		{
			if (AI_bonusOffenseSpy(5))
			{
				return;
			}
		}
		else
		{
			if (AI_cityOffenseSpy(10))
			{
				return;
			}
		}
	}

	if (iEspionageChance > 0 && (plot()->isCity() || (plot()->getNonObsoleteBonusType(getTeam()) != NO_BONUS)))
	{
		if (GC.getGame().getSorenRandNum(7, "AI Spy Skip Turn") > 0)
		{
			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY);
			return;
		}
	}

	if( area()->getNumCities() > area()->getCitiesPerPlayer(getOwnerINLINE()) )
	{
		if (GC.getGame().getSorenRandNum(4, "AI Spy Choose Movement") > 0)
		{
			if (AI_reconSpy(3))
			{
				return;
			}
		}
		else
		{
			if (AI_cityOffenseSpy(10))
			{
				return;
			}
		}
	}

	if (AI_load(UNITAI_SPY_SEA, MISSIONAI_LOAD_SPECIAL, NO_UNITAI, -1, -1, -1, 0, MOVE_NO_ENEMY_TERRITORY))
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                   */
/************************************************************************************************/

void CvUnitAI::AI_ICBMMove()
{
	PROFILE_FUNC();

//	CvCity* pCity = plot()->getPlotCity();

//	if (pCity != NULL)
//	{
//		if (pCity->AI_isDanger())
//		{
//			if (!(pCity->AI_isDefended()))
//			{
//				if (AI_airCarrier())
//				{
//					return;
//				}
//			}
//		}
//	}

	if (airRange() > 0)
	{
		if (AI_nukeRange(airRange()))
		{
			return;
		}
	}
	else if (AI_nuke())
	{
		return;
	}

	if (isCargo())
	{
		getGroup()->pushMission(MISSION_SKIP);
		return;
	}

	if (airRange() > 0)
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/25/10                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
		if (plot()->isCity(true))
		{
			int iOurDefense = GET_TEAM(getTeam()).AI_getOurPlotStrength(plot(),0,true,false,true);
			int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),2,false,false);

			if (4*iEnemyOffense > iOurDefense || iOurDefense == 0)
			{
				// Too risky, pull back
				if (AI_airOffensiveCity())
				{
					return;
				}
			}
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

		if (AI_missileLoad(UNITAI_MISSILE_CARRIER_SEA, 2, true))
		{
			return;
		}

		if (AI_missileLoad(UNITAI_MISSILE_CARRIER_SEA, 1, false))
		{
			return;
		}

		if (AI_getBirthmark() % 3 == 0)
		{
			if (AI_missileLoad(UNITAI_ATTACK_SEA, 0, false))
			{
				return;
			}
		}

		if (AI_airOffensiveCity())
		{
			return;
		}
	}

	getGroup()->pushMission(MISSION_SKIP);
}


void CvUnitAI::AI_workerSeaMove()
{
	PROFILE_FUNC();

	CvCity* pCity;

	int iI;

	if (!(getGroup()->canDefend()))
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
		//if (GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot()) > 0)
		if (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot()))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		{
			if (AI_retreatToCity())
			{
				return;
			}
		}
	}

	if (AI_improveBonus(20))
	{
		return;
	}

	if (AI_improveBonus(10))
	{
		return;
	}

	if (AI_improveBonus())
	{
		return;
	}
/*************************************************************************************************/
/**	Improved AI							16/10/12										Snarko	**/
/**							Making Lanun build pirate coves.									**/
/*************************************************************************************************/
	pCity = NULL;

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		pCity = plot()->getPlotCity();
		if (pCity == NULL)
		{
			pCity = plot()->getWorkingCity();
		}
	}

	if (pCity != NULL)
	{
		if (AI_improveCity(pCity))
		{
			return;
		}
	}

	if (AI_nextCityToImprove(pCity))
	{
		return;
	}
/*************************************************************************************************/
/**	Improved AI								END													**/
/*************************************************************************************************/

	if (isHuman())
	{
		FAssert(isAutomated());
		if (plot()->getBonusType() != NO_BONUS)
		{
			if ((plot()->getOwnerINLINE() == getOwnerINLINE()) || (!plot()->isOwned()))
			{
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}
		}

		for (iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
		{
			CvPlot* pLoopPlot = plotDirection(getX_INLINE(), getY_INLINE(), (DirectionTypes)iI);
			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->getBonusType() != NO_BONUS)
				{
					if (pLoopPlot->isValidDomainForLocation(*this))
					{
						getGroup()->pushMission(MISSION_SKIP);
						return;
					}
				}
			}
		}
	}

	if (!(isHuman()) && (AI_getUnitAIType() == UNITAI_WORKER_SEA))
	{
		pCity = plot()->getPlotCity();

		if (pCity != NULL)
		{
			if (pCity->getOwnerINLINE() == getOwnerINLINE())
			{
				if (pCity->AI_neededSeaWorkers() == 0)
				{
					if (GC.getGameINLINE().getElapsedGameTurns() > 10)
					{
						if (GET_PLAYER(getOwnerINLINE()).calculateUnitCost() > 0)
						{
							scrap();
							return;
						}
					}
				}
				else
				{
					//Probably icelocked since it can't perform actions.
					scrap();
					return;
				}
			}
		}
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_barbAttackSeaMove()
{
	PROFILE_FUNC();

	if (GC.getGameINLINE().getSorenRandNum(2, "Barb AI - Pillage Attempt (Sea)") == 0)
	{
		if (AI_pillageRange(1))
		{
			return;
		}
	}

	if (AI_anyAttack(2, 25))
	{
		return;
	}

	if (AI_pillageRange(4))
	{
		return;
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 101))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	Xienwolf Tweak							12/22/08											**/
/**																								**/
/**							Barbarians shouldn't ever stop to heal anymore						**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	if (AI_heal())
	{
		return;
	}
/**								----  End Original Code  ----									**/
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (AI_patrol())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/23/10                                jdog5000      */
/*                                                                                              */
/* Pirate AI                                                                                    */
/************************************************************************************************/
void CvUnitAI::AI_pirateSeaMove()
{
	PROFILE_FUNC();

	CvArea* pWaterArea;

	// heal in defended, unthreatened forts and cities
	if (plot()->isCity(true) && (GET_PLAYER(getOwnerINLINE()).AI_getOurPlotStrength(plot(),0,true,false) > 0) && !(GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2, false)) )
	{
		if (AI_heal())
		{
			return;
		}
	}

	if (plot()->isOwned() && (plot()->getTeam() == getTeam()))
	{
		if (AI_anyAttack(2, 40))
		{
			return;
		}

		//if (AI_protect(30))
		if (AI_protect(40, 3))
		{
			return;
		}


		if (((AI_getBirthmark() / 8) % 2) == 0)
		{
			// Previously code actually blocked grouping
			if (AI_group(UNITAI_PIRATE_SEA, -1, 1, -1, true, false, false, 8))
			{
				return;
			}
		}
	}
	else
	{
		if (AI_anyAttack(2, 51))
		{
			return;
		}
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 60))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (GC.getGame().getSorenRandNum(10, "AI Pirate Explore") == 0)
	{
		pWaterArea = plot()->waterArea();

		if (pWaterArea != NULL)
		{
			if (pWaterArea->getNumUnrevealedTiles(getTeam()) > 0)
			{
				if (GET_PLAYER(getOwnerINLINE()).AI_areaMissionAIs(pWaterArea, MISSIONAI_EXPLORE, getGroup()) < (GET_PLAYER(getOwnerINLINE()).AI_neededExplorers(pWaterArea)))
				{
					if (AI_exploreRange(2))
					{
						return;
					}
				}
			}
		}
	}

	if (GC.getGame().getSorenRandNum(11, "AI Pirate Pillage") == 0)
	{
		if (AI_pillageRange(1))
		{
			return;
		}
	}

	//Includes heal and retreat to sea routines.
	if (AI_pirateBlockade())
	{
		return;
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 101))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


void CvUnitAI::AI_attackSeaMove()
{
	PROFILE_FUNC();

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						06/14/09	Solver & jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	if (plot()->isCity(true))
	{
		int iOurDefense = GET_TEAM(getTeam()).AI_getOurPlotStrength(plot(),0,true,false,true);
		int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),2,false,false);

		if( getDamage() > 0 )	// extra risk to leaving when wounded
		{
			iOurDefense *= 2;
		}

		if( iEnemyOffense > iOurDefense/4  || iOurDefense == 0) //prioritize getting outta there
		{
			if (AI_anyAttack(2, 50))
			{
				return;
			}

			if (AI_shadow(UNITAI_ASSAULT_SEA, 4, 34, false, true, getMoves()))
			{
				return;
			}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/01/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
			//if (AI_protect(35))
			if (AI_protect(35, 3))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			{
				return;
			}

			if (AI_retreatToCity())
			{
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/


	if (AI_heal(30, 1))
	{
		return;
	}

/*************************************************************************************************/
/**	MISSION_CLAIM_FORT/MISSION_EXPLORE_LAIR	15/06/10 & 28/06/10							Snarko	**/
/**																								**/
/**						Adding a mission for the claim_fort action...							**/
/**							And teaching the AI how/when to do it								**/
/**									Likewise for explore_lair									**/
/*************************************************************************************************/
	if (AI_claimFort(3, 50))
	{
		return;
	}

	if (AI_exploreLair(3, 50))
	{
		return;
	}
/*************************************************************************************************/
/**	MISSION_CLAIM_FORT/MISSION_EXPLORE_LAIR				END										**/
/*************************************************************************************************/

	if (AI_anyAttack(1, 35))
	{
		return;
	}

	if (AI_anyAttack(2, 40))
	{
		return;
	}

	if (AI_seaBombardRange(6))
	{
		return;
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 60))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (AI_heal(50, 3))
	{
		return;
	}

	if (AI_heal())
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/10/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
	// BBAI TODO: Turn this into a function, have docked escort ships do it to
	CvCity* pCity = plot()->getPlotCity();

	if( pCity != NULL )
	{
		if( pCity->isBlockaded() )
		{
			// City under blockade
			// Attacker has low odds since anyAttack checks above passed, try to break if sufficient numbers

			int iAttackers = plot()->plotCount(PUF_isUnitAIType, UNITAI_ATTACK_SEA, -1, NO_PLAYER, getTeam(), PUF_isGroupHead, -1, -1);
			int iBlockaders = GET_PLAYER(getOwnerINLINE()).AI_getWaterDanger(plot(), 4);

			if( iAttackers > (iBlockaders + 2) )
			{
				if( iAttackers > GC.getGameINLINE().getSorenRandNum(2*iBlockaders + 1, "AI - Break blockade") )
				{
					// BBAI TODO: Make odds scale by # of blockaders vs number of attackers
					if (AI_anyAttack(1, 15))
					{
						return;
					}
				}
			}
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_group(UNITAI_CARRIER_SEA, /*iMaxGroup*/ 4, 1, -1, true, false, false, /*iMaxPath*/ 5))
	{
		return;
	}

	if (AI_group(UNITAI_ATTACK_SEA, /*iMaxGroup*/ 1, -1, -1, true, false, false, /*iMaxPath*/ 3))
	{
		return;
	}

	if (!plot()->isOwned() || !isEnemy(plot()->getTeam()))
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/11/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
/* original bts code
		if (AI_shadow(UNITAI_ASSAULT_SEA, 4, 34))
		{
			return;
		}

		if (AI_shadow(UNITAI_CARRIER_SEA, 4, 51))
		{
			return;
		}

		if (AI_group(UNITAI_ASSAULT_SEA, -1, 4, -1, false, false, false))
		{
			return;
		}
	}

	if (AI_group(UNITAI_CARRIER_SEA, -1, 1, -1, false, false, false))
	{
		return;
	}
*/
		if (AI_shadow(UNITAI_ASSAULT_SEA, 4, 34, true, false, 4))
		{
			return;
		}

		if (AI_shadow(UNITAI_CARRIER_SEA, 4, 51, true, false, 5))
		{
			return;
		}

		// Group with large flotillas first
		if (AI_group(UNITAI_ASSAULT_SEA, -1, 4, 3, false, false, false, 3, false, true, false))
		{
			return;
		}

		if (AI_group(UNITAI_ASSAULT_SEA, -1, 2, -1, false, false, false, 5, false, true, false))
		{
			return;
		}
	}

	if (AI_group(UNITAI_CARRIER_SEA, -1, 1, -1, false, false, false, 10))
	{
		return;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


	if (plot()->isOwned() && (isEnemy(plot()->getTeam())))
	{
		if (AI_blockade())
		{
			return;
		}
	}

	if (AI_pillageRange(4))
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/01/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
	//if (AI_protect(35))
	if (AI_protect(35, 3))
	{
		return;
	}

	if (AI_protect(35, 8))
	{
		return;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 101))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (AI_travelToUpgradeCity())
	{
		return;
	}

	if (AI_patrol())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_reserveSeaMove()
{
	PROFILE_FUNC();

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						06/14/09	Solver & jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	if (plot()->isCity(true))
	{
		int iOurDefense = GET_TEAM(getTeam()).AI_getOurPlotStrength(plot(),0,true,false,true);
		int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),2,false,false);

		if( getDamage() > 0 )	// extra risk to leaving when wounded
		{
			iOurDefense *= 2;
		}

		if( iEnemyOffense > iOurDefense/4  || iOurDefense == 0)  //prioritize getting outta there
		{
			if (AI_anyAttack(2, 60))
			{
				return;
			}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/01/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
			//if (AI_protect(40))
			if (AI_protect(40, 3))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			{
				return;
			}

			if (AI_shadow(UNITAI_SETTLER_SEA, 2, -1, false, true, getMoves()))
			{
				return;
			}

			if (AI_retreatToCity())
			{
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	if (AI_guardBonus(30))
	{
		return;
	}

	if (AI_heal(30, 1))
	{
		return;
	}

	if (AI_anyAttack(1, 55))
	{
		return;
	}

	if (AI_seaBombardRange(6))
	{
		return;
	}


/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/01/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
	//if (AI_protect(40))
	if (AI_protect(40, 5))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	{
		return;
	}


/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 60))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/03/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
/* original bts code
	if (AI_shadow(UNITAI_SETTLER_SEA, 1, -1, true))
	{
		return;
	}

	if (AI_group(UNITAI_RESERVE_SEA, 1))
	{
		return;
	}

	if (bombardRate() > 0)
	{
		if (AI_shadow(UNITAI_ASSAULT_SEA, 2, 30, true))
		{
			return;
		}
	}
*/
	// Shadow any nearby settler sea transport out at sea
	if (AI_shadow(UNITAI_SETTLER_SEA, 2, -1, false, true, 5))
	{
		return;
	}

	if (AI_group(UNITAI_RESERVE_SEA, 1, -1, -1, false, false, false, 8))
	{
		return;
	}

	if (bombardRate() > 0)
	{
		if (AI_shadow(UNITAI_ASSAULT_SEA, 2, 30, true, false, 8))
		{
			return;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_heal(50, 3))
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/01/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
	if (AI_protect(40))
	{
		return;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_anyAttack(3, 45))
	{
		return;
	}

	if (AI_heal())
	{
		return;
	}

	if (!isNeverInvisible())
	{
		if (AI_anyAttack(5, 35))
		{
			return;
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/03/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                      */
/************************************************************************************************/
	// Shadow settler transport with cargo
	if (AI_shadow(UNITAI_SETTLER_SEA, 1, -1, true, false, 10))
	{
		return;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_travelToUpgradeCity())
	{
		return;
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 101))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (AI_patrol())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_escortSeaMove()
{
	PROFILE_FUNC();

//	// if we have cargo, possibly convert to UNITAI_ASSAULT_SEA (this will most often happen with galleons)
//	// note, this should not happen when we are not the group head, so escort galleons are fine joining a group, just not as head
//	if (hasCargo() && (getUnitAICargo(UNITAI_ATTACK_CITY) > 0 || getUnitAICargo(UNITAI_ATTACK) > 0))
//	{
//		// non-zero AI_unitValue means that UNITAI_ASSAULT_SEA is valid for this unit (that is the check used everywhere)
//		if (GET_PLAYER(getOwnerINLINE()).AI_unitValue(getUnitType(), UNITAI_ASSAULT_SEA, NULL) > 0)
//		{
//			// save old group, so we can merge it back in
//			CvSelectionGroup* pOldGroup = getGroup();
//
//			// this will remove this unit from the current group
//			AI_setUnitAIType(UNITAI_ASSAULT_SEA);
//
//			// merge back the rest of the group into the new group
//			CvSelectionGroup* pNewGroup = getGroup();
//			if (pOldGroup != pNewGroup)
//			{
//				pOldGroup->mergeIntoGroup(pNewGroup);
//			}
//
//			// perform assault sea action
//			AI_assaultSeaMove();
//			return;
//		}
//	}

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						06/14/09	Solver & jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	if (plot()->isCity(true)) //prioritize getting outta there
	{
		int iOurDefense = GET_TEAM(getTeam()).AI_getOurPlotStrength(plot(),0,true,false,true);
		int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),2,false,false);

		if( getDamage() > 0 )	// extra risk to leaving when wounded
		{
			iOurDefense *= 2;
		}

		if( iEnemyOffense > iOurDefense/4  || iOurDefense == 0)
		{
			if (AI_anyAttack(1, 60))
			{
				return;
			}

			if (AI_group(UNITAI_ASSAULT_SEA, -1, /*iMaxOwnUnitAI*/ 1, -1, /*bIgnoreFaster*/ true, false, false, /*iMaxPath*/ getMoves()))
			{
				return;
			}

			if (AI_retreatToCity())
			{
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	if (AI_heal(30, 1))
	{
		return;
	}

	if (AI_anyAttack(1, 55))
	{
		return;
	}

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						9/14/08			jdog5000		*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	// Galleons can get stuck with this AI type since they don't upgrade to any escort unit
	// Galleon escorts are much less useful once Frigates or later are available
	if (!isHuman() && !isBarbarian())
	{
		if( getCargo() > 0 && (GC.getUnitInfo(getUnitType()).getSpecialCargo() == NO_SPECIALUNIT) )
		{
			//Obsolete?
			int iValue = GET_PLAYER(getOwnerINLINE()).AI_unitValue(getUnitType(), AI_getUnitAIType(), area());
			int iBestValue = GET_PLAYER(getOwnerINLINE()).AI_bestAreaUnitAIValue(AI_getUnitAIType(), area());

			if (iValue < iBestValue)
			{
				if (GET_PLAYER(getOwnerINLINE()).AI_unitValue(getUnitType(), UNITAI_ASSAULT_SEA, area()) > 0)
				{
					AI_setUnitAIType(UNITAI_ASSAULT_SEA);
					return;
				}

				if (GET_PLAYER(getOwnerINLINE()).AI_unitValue(getUnitType(), UNITAI_SETTLER_SEA, area()) > 0)
				{
					AI_setUnitAIType(UNITAI_SETTLER_SEA);
					return;
				}

				scrap();
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	if (AI_group(UNITAI_CARRIER_SEA, -1, /*iMaxOwnUnitAI*/ 0, -1, /*bIgnoreFaster*/ true))
	{
		return;
	}

	if (AI_group(UNITAI_ASSAULT_SEA, -1, /*iMaxOwnUnitAI*/ 0, -1, /*bIgnoreFaster*/ true, false, false, /*iMaxPath*/ 3))
	{
		return;
	}

	if (AI_heal(50, 3))
	{
		return;
	}

	if (AI_pillageRange(2))
	{
		return;
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 60))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (AI_group(UNITAI_MISSILE_CARRIER_SEA, 1, 1, true))
	{
		return;
	}

	if (AI_group(UNITAI_ASSAULT_SEA, 1, /*iMaxOwnUnitAI*/ 0, /*iMinUnitAI*/ -1, /*bIgnoreFaster*/ true))
	{
		return;
	}

	if (AI_group(UNITAI_ASSAULT_SEA, -1, /*iMaxOwnUnitAI*/ 2, /*iMinUnitAI*/ -1, /*bIgnoreFaster*/ true))
	{
		return;
	}

	if (AI_group(UNITAI_CARRIER_SEA, -1, /*iMaxOwnUnitAI*/ 2, /*iMinUnitAI*/ -1, /*bIgnoreFaster*/ true))
	{
		return;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/01/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                      */
/************************************************************************************************/
/* original bts code
	if (AI_group(UNITAI_ASSAULT_SEA, -1, 4, -1, true))
	{
		return;
	}
*/
	// Group only with large flotillas first
	if (AI_group(UNITAI_ASSAULT_SEA, -1, /*iMaxOwnUnitAI*/ 4, /*iMinUnitAI*/ 3, /*bIgnoreFaster*/ true))
	{
		return;
	}

	if (AI_shadow(UNITAI_SETTLER_SEA, 2, -1, false, true, 4))
	{
		return;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_heal())
	{
		return;
	}

	if (AI_travelToUpgradeCity())
	{
		return;
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 101))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/18/10                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
	// If nothing else useful to do, escort nearby large flotillas even if they're faster
	// Gives Caravel escorts something to do during the Galleon/pre-Frigate era
	if (AI_group(UNITAI_ASSAULT_SEA, -1, /*iMaxOwnUnitAI*/ 4, /*iMinUnitAI*/ 3, /*bIgnoreFaster*/ false, false, false, 4, false, true))
	{
		return;
	}

	if (AI_group(UNITAI_ASSAULT_SEA, -1, /*iMaxOwnUnitAI*/ 2, /*iMinUnitAI*/ -1, /*bIgnoreFaster*/ false, false, false, 1, false, true))
	{
		return;
	}

	// Pull back to primary area if it's not too far so primary area cities know you exist
	// and don't build more, unnecessary escorts
	if (AI_retreatToCity(true,false,6))
	{
		return;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_exploreSeaMove()
{
	PROFILE_FUNC();

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						10/21/08	Solver & jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	if (plot()->isCity(true)) //prioritize getting outta there
	{
		int iOurDefense = GET_TEAM(getTeam()).AI_getOurPlotStrength(plot(),0,true,false,true);
		int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),2,false,false);

		if( getDamage() > 0 )	// extra risk to leaving when wounded
		{
			iOurDefense *= 2;
		}

		if( iEnemyOffense > iOurDefense/4 || iOurDefense == 0 )
		{
			if (!isHuman())
			{
				if (AI_anyAttack(1, 60))
				{
					return;
				}
			}

			if (AI_retreatToCity())
			{
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	CvArea* pWaterArea = plot()->waterArea();

	if (!isHuman())
	{
		if (AI_anyAttack(1, 60))
		{
			return;
		}
/*************************************************************************************************/
/**	Tweak							04/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
		if (AI_rangeAttack(airRange(), 60))
		{
			return;
		}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
	}

	if (!isHuman() && !isBarbarian()) //XXX move some of this into a function? maybe useful elsewhere
	{
		//Obsolete?
		int iValue = GET_PLAYER(getOwnerINLINE()).AI_unitValue(getUnitType(), AI_getUnitAIType(), area());
		int iBestValue = GET_PLAYER(getOwnerINLINE()).AI_bestAreaUnitAIValue(AI_getUnitAIType(), area());

		if (iValue < iBestValue)
		{
			//Transform
			if (GET_PLAYER(getOwnerINLINE()).AI_unitValue(getUnitType(), UNITAI_WORKER_SEA, area()) > 0)
			{
				AI_setUnitAIType(UNITAI_WORKER_SEA);
				return;
			}

			if (GET_PLAYER(getOwnerINLINE()).AI_unitValue(getUnitType(), UNITAI_PIRATE_SEA, area()) > 0)
			{
				AI_setUnitAIType(UNITAI_PIRATE_SEA);
				return;
			}

			if (GET_PLAYER(getOwnerINLINE()).AI_unitValue(getUnitType(), UNITAI_MISSIONARY_SEA, area()) > 0)
			{
				AI_setUnitAIType(UNITAI_MISSIONARY_SEA);
				return;
			}

			if (GET_PLAYER(getOwnerINLINE()).AI_unitValue(getUnitType(), UNITAI_RESERVE_SEA, area()) > 0)
			{
				AI_setUnitAIType(UNITAI_RESERVE_SEA);
				return;
			}
			scrap();
		}
	}

	if (getDamage() > 0)
	{
		if ((plot()->getFeatureType() == NO_FEATURE) || (GC.getFeatureInfo(plot()->getFeatureType()).getTurnDamage() == 0))
		{
/*************************************************************************************************/
/**	HealRight								04/18/09								Xienwolf	**/
/**																								**/
/**		Units will not attempt to heal in a location which will make such a thing impossible	**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
			getGroup()->pushMission(MISSION_HEAL);
			return;
/**								----  End Original Code  ----									**/
/*************************************************************************************************/
/**	Speedup								11/02/12										Snarko	**/
/**																								**/
/**				Use the must faster getAnyPlotDanger where possible								**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
			if(GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 1) == 0)
/**								----  End Original Code  ----									**/
			if (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 1))
/*************************************************************************************************/
/**	Speedup									END													**/
/*************************************************************************************************/
			{
				if(healRate(plot()) > 5)
				{
					getGroup()->pushMission(MISSION_HEAL);
					return;
				}
				else
				{
					if(getDamage() > 40)
					{
						if (AI_retreatToCity())
						{
							return;
						}
						getGroup()->pushMission(MISSION_HEAL);
						return;
					}
				}
			}
			else
			{
				if(getDamage() > 20)
				{
					if (AI_retreatToCity())
					{
						return;
					}

					if (AI_safety())
					{
						return;
					}
				}
			}
/*************************************************************************************************/
/**	HealRight								END													**/
/*************************************************************************************************/
		}
	}

	if (!isHuman())
	{
		if (AI_pillageRange(1))
		{
			return;
		}
	}

/*************************************************************************************************/
/**	MISSION_CLAIM_FORT/MISSION_EXPLORE_LAIR	28/06/10									Snarko	**/
/**																								**/
/**						Adding a mission for the claim_fort action...							**/
/**							And teaching the AI how/when to do it								**/
/**									Likewise for explore_lair									**/
/*************************************************************************************************/
	if (AI_claimFort(4, 60))
	{
		return;
	}

	if (AI_exploreLair(4, 60))
	{
		return;
	}
/*************************************************************************************************/
/**	MISSION_CLAIM_FORT/MISSION_EXPLORE_LAIR				END										**/
/*************************************************************************************************/

	if (AI_exploreRange(4))
	{
		return;
	}

	if (!isHuman())
	{
		if (AI_pillageRange(4))
		{
			return;
		}
	}

	if (AI_explore())
	{
		return;
	}

	if (!isHuman())
	{
		if (AI_pillage())
		{
			return;
		}
	}

	if (!isHuman())
	{
		if (AI_travelToUpgradeCity())
		{
			return;
		}
	}

	if (!(isHuman()) && (AI_getUnitAIType() == UNITAI_EXPLORE_SEA))
	{
		pWaterArea = plot()->waterArea();

		if (pWaterArea != NULL)
		{
			if (GET_PLAYER(getOwnerINLINE()).AI_totalWaterAreaUnitAIs(pWaterArea, UNITAI_EXPLORE_SEA) > GET_PLAYER(getOwnerINLINE()).AI_neededExplorers(pWaterArea))
			{
				if (GET_PLAYER(getOwnerINLINE()).calculateUnitCost() > 0)
				{
					scrap();
					return;
				}
			}
		}
	}

	if (AI_patrol())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/18/10                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
void CvUnitAI::AI_assaultSeaMove()
{
	PROFILE_FUNC();

	FAssert(AI_getUnitAIType() == UNITAI_ASSAULT_SEA);

	bool bEmpty = !getGroup()->hasCargo();
	bool bFull = (getGroup()->AI_isFull() && (getGroup()->getCargo() > 0));

	if (plot()->isCity(true))
	{
		int iOurDefense = GET_TEAM(getTeam()).AI_getOurPlotStrength(plot(),0,true,false,true);
		int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),2,false,false);

		if( getDamage() > 0 )	// extra risk to leaving when wounded
		{
			iOurDefense *= 2;
		}

		if( iEnemyOffense > iOurDefense/8 || iOurDefense == 0 )
		{
			if( iEnemyOffense > iOurDefense/4 || iOurDefense == 0 ) //prioritize getting outta there
			{
				if( !bEmpty )
				{
					getGroup()->unloadAll();
				}

				if (AI_anyAttack(1, 65))
				{
					return;
				}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
				if (AI_rangeAttack(airRange(), 75))
				{
					return;
				}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

				// Retreat to primary area first
				if (AI_retreatToCity(true))
				{
					return;
				}

				if (AI_retreatToCity())
				{
					return;
				}

				if (AI_safety())
				{
					return;
				}
			}

			if( !bFull && !bEmpty )
			{
				getGroup()->unloadAll();
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}
		}
	}

	if (bEmpty)
	{
		if (AI_anyAttack(1, 65))
		{
			return;
		}
		if (AI_anyAttack(1, 45))
		{
			return;
		}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
		if (AI_rangeAttack(airRange(), 60))
		{
			return;
		}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
	}

	bool bReinforce = false;
	bool bAttack = false;
	bool bNoWarPlans = (GET_TEAM(getTeam()).getAnyWarPlanCount(true) == 0);
	bool bAttackBarbarian = false;
	bool bLandWar = false;
	bool bIsBarbarian = isBarbarian();

	// Count forts as cities
	bool bIsCity = plot()->isCity(true);

	// Cargo if already at war
	int iTargetReinforcementSize = (bIsBarbarian ? AI_stackOfDoomExtra() : 2);

	// Cargo to launch a new invasion
	int iTargetInvasionSize = 2*iTargetReinforcementSize;

	int iCargo = getGroup()->getCargo();
	int iEscorts = getGroup()->countNumUnitAIType(UNITAI_ESCORT_SEA) + getGroup()->countNumUnitAIType(UNITAI_ATTACK_SEA);

	AreaAITypes eAreaAIType = area()->getAreaAIType(getTeam());
	bLandWar = !bIsBarbarian && ((eAreaAIType == AREAAI_OFFENSIVE) || (eAreaAIType == AREAAI_DEFENSIVE) || (eAreaAIType == AREAAI_MASSING));

	// Plot danger case handled above

	if( hasCargo() && (getUnitAICargo(UNITAI_SETTLE) > 0 || getUnitAICargo(UNITAI_WORKER) > 0) )
	{
		// Dump inappropriate load at first oppurtunity after pick up
		if( bIsCity && (plot()->getOwnerINLINE() == getOwnerINLINE()) )
		{
			getGroup()->unloadAll();
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
		else
		{
			if( !isFull() )
			{
				if(AI_pickupStranded(NO_UNITAI, 1))
				{
					return;
				}
			}

			if (AI_retreatToCity(true))
			{
				return;
			}

			if (AI_retreatToCity())
			{
				return;
			}
		}
	}

	if (bIsCity)
	{
		CvCity* pCity = plot()->getPlotCity();

		if( pCity != NULL && (plot()->getOwnerINLINE() == getOwnerINLINE()) )
		{
			// split out galleys from stack of ocean capable ships
			if( GET_PLAYER(getOwnerINLINE()).AI_unitImpassableCount(getUnitType()) == 0 && getGroup()->getNumUnits() > 1 )
			{
				getGroup()->AI_separateImpassable();
			}

			// galleys with upgrade available should get that ASAP
			if( GET_PLAYER(getOwnerINLINE()).AI_unitImpassableCount(getUnitType()) > 0 )
			{
/*************************************************************************************************/
/**	Speedup								12/02/12										Snarko	**/
/**																								**/
/**			Only store the unitclasses we upgrade to, for faster looping						**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
				CvCity* pUpgradeCity = getUpgradeCity(false);
				if( pUpgradeCity != NULL && pUpgradeCity == pCity )
/**								----  End Original Code  ----									**/
				int iValue = GET_PLAYER(getOwnerINLINE()).AI_unitValue(getUnitType(), AI_getUnitAIType(), area()); //I assume we're on the same area...
				if ( AI_getBestUpgradeUnit(getUnitType(), &iValue, pCity) != NULL)
/*************************************************************************************************/
/**	Speedup									END													**/
/*************************************************************************************************/
				{
					// Wait for upgrade, this unit is top upgrade priority
					getGroup()->pushMission(MISSION_SKIP);
					return;
				}
			}
		}

		if( (iCargo > 0) )
		{
			if( pCity != NULL )
			{
				if( (GC.getGameINLINE().getGameTurn() - pCity->getGameTurnAcquired()) <= 1 )
				{
					if( pCity->getPreviousOwner() != NO_PLAYER )
					{
						// Just captured city, probably from naval invasion.  If area targets, drop cargo and leave so as to not to be lost in quick counter attack
						if( GET_TEAM(getTeam()).countEnemyPowerByArea(plot()->area()) > 0 )
						{
							getGroup()->unloadAll();

							if( iEscorts > 2 )
							{
								if( getGroup()->countNumUnitAIType(UNITAI_ESCORT_SEA) > 1 && getGroup()->countNumUnitAIType(UNITAI_ATTACK_SEA) > 0 )
								{
									getGroup()->AI_separateAI(UNITAI_ATTACK_SEA);
									getGroup()->AI_separateAI(UNITAI_RESERVE_SEA);

									iEscorts = getGroup()->countNumUnitAIType(UNITAI_ESCORT_SEA);
								}
							}
							iCargo = getGroup()->getCargo();
						}
					}
				}
			}
		}

		if( (iCargo > 0) && (iEscorts == 0) )
		{
			if (AI_group(UNITAI_ASSAULT_SEA,-1,-1,-1,/*bIgnoreFaster*/true,false,false,/*iMaxPath*/1,false,/*bCargoOnly*/true,false,MISSIONAI_ASSAULT))
			{
				return;
			}

			if( plot()->plotCount(PUF_isUnitAIType, UNITAI_ESCORT_SEA, -1, getOwnerINLINE(), NO_TEAM, PUF_isGroupHead, -1, -1) > 0 )
			{
				// Loaded but with no escort, wait for escorts in plot to join us
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}

			MissionAITypes eMissionAIType = MISSIONAI_GROUP;
			if( (GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, &eMissionAIType, 1, getGroup(), 3) > 0) || (GET_PLAYER(getOwnerINLINE()).AI_getWaterDanger(plot(), 4, false) > 0) )
			{
				// Loaded but with no escort, wait for others joining us soon or avoid dangerous waters
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}
		}

		if (bLandWar)
		{
			if ( iCargo > 0 )
			{
				if( (eAreaAIType == AREAAI_DEFENSIVE) || (pCity != NULL && pCity->AI_isDanger()))
				{
					// Unload cargo when on defense or if small load of troops and can reach enemy city over land (generally less risky)
					getGroup()->unloadAll();
					getGroup()->pushMission(MISSION_SKIP);
					return;
				}
			}

			if ((iCargo >= iTargetReinforcementSize))
			{
				getGroup()->AI_separateEmptyTransports();

				if( !(getGroup()->hasCargo()) )
				{
					// this unit was empty group leader
					getGroup()->pushMission(MISSION_SKIP);
					return;
				}

				// Send ready transports
				if (AI_assaultSeaReinforce(false))
				{
					return;
				}

				if( iCargo >= iTargetInvasionSize )
				{
					if (AI_assaultSeaTransport(false))
					{
						return;
					}
				}
			}
		}
		else
		{
			if ( (eAreaAIType == AREAAI_ASSAULT) )
			{
				if( iCargo >= iTargetInvasionSize )
				{
					bAttack = true;
				}
			}

			if( (eAreaAIType == AREAAI_ASSAULT) || (eAreaAIType == AREAAI_ASSAULT_ASSIST) )
			{
				if( (bFull && iCargo > cargoSpace()) || (iCargo >= iTargetReinforcementSize) )
				{
					bReinforce = true;
				}
			}
		}

		if( !bAttack && !bReinforce && (plot()->getTeam() == getTeam()) )
		{
			if( iEscorts > 3 && iEscorts > (2*getGroup()->countNumUnitAIType(UNITAI_ASSAULT_SEA)) )
			{
				// If we have too many escorts, try freeing some for others
				getGroup()->AI_separateAI(UNITAI_ATTACK_SEA);
				getGroup()->AI_separateAI(UNITAI_RESERVE_SEA);

				iEscorts = getGroup()->countNumUnitAIType(UNITAI_ESCORT_SEA);
				if( iEscorts > 3 && iEscorts > (2*getGroup()->countNumUnitAIType(UNITAI_ASSAULT_SEA)) )
				{
					getGroup()->AI_separateAI(UNITAI_ESCORT_SEA);
				}
			}
		}

		MissionAITypes eMissionAIType = MISSIONAI_GROUP;
		if( GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, &eMissionAIType, 1, getGroup(), 1) > 0 )
		{
			// Wait for units which are joining our group this turn
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}

		if( !bFull )
		{
			if( bAttack )
			{
				eMissionAIType = MISSIONAI_LOAD_ASSAULT;
				if( GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, &eMissionAIType, 1, getGroup(), 1) > 0 )
				{
					// Wait for cargo which will load this turn
					getGroup()->pushMission(MISSION_SKIP);
					return;
				}
			}
			else if( GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, MISSIONAI_LOAD_ASSAULT) > 0 )
			{
				// Wait for cargo which is on the way
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}
		}

		if( !bAttack && !bReinforce )
		{
			if ( iCargo > 0 )
			{
				if (AI_group(UNITAI_ASSAULT_SEA,-1,-1,-1,/*bIgnoreFaster*/true,false,false,/*iMaxPath*/5,false,/*bCargoOnly*/true,false,MISSIONAI_ASSAULT))
				{
					return;
				}
			}
			else if (plot()->getTeam() == getTeam() && getGroup()->getNumUnits() > 1)
			{
				CvCity* pCity = plot()->getPlotCity();
				if( pCity != NULL && (GC.getGameINLINE().getGameTurn() - pCity->getGameTurnAcquired()) > 10 )
				{
					if( pCity->plot()->plotCount(PUF_isAvailableUnitAITypeGroupie, UNITAI_ATTACK_CITY, -1, getOwnerINLINE()) < iTargetReinforcementSize )
					{
						// Not attacking, no cargo so release any escorts, attack ships, etc and split transports
						getGroup()->AI_makeForceSeparate();
					}
				}
			}
		}
	}

	if (!bIsCity)
	{
		if( iCargo >= iTargetInvasionSize )
		{
			bAttack = true;
		}

		if ((iCargo >= iTargetReinforcementSize) || (bFull && iCargo > cargoSpace()))
		{
			bReinforce = true;
		}

		CvPlot* pAdjacentPlot = NULL;
		for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
		{
			pAdjacentPlot = plotDirection(getX_INLINE(), getY_INLINE(), ((DirectionTypes)iI));
			if( pAdjacentPlot != NULL )
			{
				if( iCargo > 0 )
				{
					CvCity* pAdjacentCity = pAdjacentPlot->getPlotCity();
					if( pAdjacentCity != NULL && pAdjacentCity->getOwner() == getOwnerINLINE() && pAdjacentCity->getPreviousOwner() != NO_PLAYER )
					{
						if( (GC.getGameINLINE().getGameTurn() - pAdjacentCity->getGameTurnAcquired()) < 5 )
						{
							// If just captured city and we have some cargo, dump units in city
							getGroup()->pushMission(MISSION_MOVE_TO, pAdjacentPlot->getX_INLINE(), pAdjacentPlot->getY_INLINE(), 0, false, false, MISSIONAI_ASSAULT, pAdjacentPlot);
							return;
						}
					}
				}
				else
				{
					if (pAdjacentPlot->isOwned() && isEnemy(pAdjacentPlot->getTeam()))
					{
						if( pAdjacentPlot->getNumDefenders(getOwnerINLINE()) > 2 )
						{
							// if we just made a dropoff in enemy territory, release sea bombard units to support invaders
							if ((getGroup()->countNumUnitAIType(UNITAI_ATTACK_SEA) + getGroup()->countNumUnitAIType(UNITAI_RESERVE_SEA)) > 0)
							{
								bool bMissionPushed = false;

								if (AI_seaBombardRange(1))
								{
									bMissionPushed = true;
								}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
					if (AI_rangeAttack(airRange(), 101))
					{
						bMissionPushed = true;
					}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

								CvSelectionGroup* pOldGroup = getGroup();

								//Release any Warships to finish the job.
								getGroup()->AI_separateAI(UNITAI_ATTACK_SEA);
								getGroup()->AI_separateAI(UNITAI_RESERVE_SEA);

/************************************************************************************************/
/* UNOFFICIAL_PATCH                       05/11/09                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original bts code
					if (pOldGroup == getGroup() && getUnitType() == UNITAI_ASSAULT_SEA)
					{
						if (AI_retreatToCity(true))
						{
							bMissionPushed = true;
						}
					}
*/
								// Fixed bug in next line with checking unit type instead of unit AI
								if (pOldGroup == getGroup() && AI_getUnitAIType() == UNITAI_ASSAULT_SEA)
								{
									// Need to be sure all units can move
									if( getGroup()->canAllMove() )
									{
										if (AI_retreatToCity(true))
										{
											bMissionPushed = true;
										}
									}
								}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/


								if (bMissionPushed)
								{
									return;
								}
							}
						}
					}
				}
			}
		}

		if(iCargo > 0)
		{
			MissionAITypes eMissionAIType = MISSIONAI_GROUP;
			if( GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, &eMissionAIType, 1, getGroup(), 1) > 0 )
			{
				if( iEscorts < GET_PLAYER(getOwnerINLINE()).AI_getWaterDanger(plot(), 2, false) )
				{
					// Wait for units which are joining our group this turn (hopefully escorts)
					getGroup()->pushMission(MISSION_SKIP);
					return;
				}
			}
		}
	}

	if (bIsBarbarian)
	{
		if (getGroup()->isFull() || iCargo > iTargetInvasionSize)
		{
			if (AI_assaultSeaTransport(false))
			{
				return;
			}
		}
		else
		{
			if (AI_pickup(UNITAI_ATTACK_CITY, true, 5))
			{
				return;
			}

			if (AI_pickup(UNITAI_ATTACK, true, 5))
			{
				return;
			}

			if (AI_retreatToCity())
			{
				return;
			}

			if( !(getGroup()->getCargo()) )
			{
				AI_barbAttackSeaMove();
				return;
			}

			if( AI_safety() )
			{
				return;
			}

			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
	}
	else
	{
		if (bAttack || bReinforce)
		{
			if( bIsCity )
			{
				getGroup()->AI_separateEmptyTransports();
			}

			if( !(getGroup()->hasCargo()) )
			{
				// this unit was empty group leader
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}

			FAssert(getGroup()->hasCargo());

			//BBAI TODO: Check that group has escorts, otherwise usually wait

			if( bAttack )
			{
				if( bReinforce && (AI_getBirthmark()%2 == 0) )
				{
					if (AI_assaultSeaReinforce())
					{
						return;
					}
					bReinforce = false;
				}

				if (AI_assaultSeaTransport())
				{
					return;
				}
			}

			// If not enough troops for own invasion,
			if( bReinforce )
			{
				if (AI_assaultSeaReinforce())
				{
					return;
				}
			}
		}

		if( bNoWarPlans && (iCargo >= iTargetReinforcementSize) )
		{
			bAttackBarbarian = true;

			getGroup()->AI_separateEmptyTransports();

			if( !(getGroup()->hasCargo()) )
			{
				// this unit was empty group leader
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}

			FAssert(getGroup()->hasCargo());
			if (AI_assaultSeaReinforce(bAttackBarbarian))
			{
				return;
			}

			FAssert(getGroup()->hasCargo());
			if (AI_assaultSeaTransport(bAttackBarbarian))
			{
				return;
			}
		}
	}

	if ((bFull || bReinforce) && !bAttack)
	{
		// Group with nearby transports with units on board
		if (AI_group(UNITAI_ASSAULT_SEA, -1, /*iMaxOwnUnitAI*/ -1, -1, true, false, false, 2, false, true, false, MISSIONAI_ASSAULT))
		{
			return;
		}

		if (AI_group(UNITAI_ASSAULT_SEA, -1, -1, -1, true, false, false, 10, false, true, false, MISSIONAI_ASSAULT))
		{
			return;
		}
	}
	else if( !bFull )
	{
		bool bHasOneLoad = (getGroup()->getCargo() >= cargoSpace());
		bool bHasCargo = getGroup()->hasCargo();

		if (AI_pickup(UNITAI_ATTACK_CITY, !bHasCargo, (bHasOneLoad ? 3 : 7)))
		{
			return;
		}

		if (AI_pickup(UNITAI_ATTACK, !bHasCargo, (bHasOneLoad ? 3 : 7)))
		{
			return;
		}

		if (AI_pickup(UNITAI_COUNTER, !bHasCargo, (bHasOneLoad ? 3 : 7)))
		{
			return;
		}

		if (AI_pickup(UNITAI_ATTACK_CITY, !bHasCargo))
		{
			return;
		}

		if( !bHasCargo )
		{
			if(AI_pickupStranded(UNITAI_ATTACK_CITY))
			{
				return;
			}

			if(AI_pickupStranded(UNITAI_ATTACK))
			{
				return;
			}

			if(AI_pickupStranded(UNITAI_COUNTER))
			{
				return;
			}

			if( (getGroup()->countNumUnitAIType(AI_getUnitAIType()) == 1) )
			{
				// Try picking up any thing
				if(AI_pickupStranded())
				{
					return;
				}
			}
		}
	}

	if (bIsCity && bLandWar && getGroup()->hasCargo())
	{
		// Enemy units in this player's territory
		if( GET_PLAYER(getOwnerINLINE()).AI_countNumAreaHostileUnits(area(),true,false,false,false) > (getGroup()->getCargo()/2))
		{
			getGroup()->unloadAll();
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
	}

/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
				if (AI_rangeAttack(airRange(), 101))
				{
					return;
				}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (AI_retreatToCity(true))
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


void CvUnitAI::AI_settlerSeaMove()
{
	PROFILE_FUNC();

	bool bEmpty = !getGroup()->hasCargo();

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						10/21/08	Solver & jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	if (plot()->isCity(true))
	{
		int iOurDefense = GET_TEAM(getTeam()).AI_getOurPlotStrength(plot(),0,true,false,true);
		int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),2,false,false);

		if( getDamage() > 0 )	// extra risk to leaving when wounded
		{
			iOurDefense *= 2;
		}

		if( iEnemyOffense > iOurDefense/4 || iOurDefense == 0 ) //prioritize getting outta there
		{
			if( bEmpty )
			{
				if (AI_anyAttack(1, 65))
				{
					return;
				}
			}

			// Retreat to primary area first
			if (AI_retreatToCity(true))
			{
				return;
			}

			if (AI_retreatToCity())
			{
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	if (bEmpty)
	{
		if (AI_anyAttack(1, 65))
		{
			return;
		}
		if (AI_anyAttack(1, 40))
		{
			return;
		}
	}

	int iSettlerCount = getUnitAICargo(UNITAI_SETTLE);
	int iWorkerCount = getUnitAICargo(UNITAI_WORKER);

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/07/08                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
	if( hasCargo() && (iSettlerCount == 0) && (iWorkerCount == 0))
	{
		// Dump troop load at first oppurtunity after pick up
		if( plot()->isCity() && plot()->getOwnerINLINE() == getOwnerINLINE() )
		{
			getGroup()->unloadAll();
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
		else
		{
			if( !(isFull()) )
			{
				if(AI_pickupStranded(NO_UNITAI, 1))
				{
					return;
				}
			}

			if (AI_retreatToCity(true))
			{
				return;
			}

			if (AI_retreatToCity())
			{
				return;
			}
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/02/09                                jdog5000      */
/*                                                                                              */
/* Settler AI                                                                                   */
/************************************************************************************************/
	// Don't send transport with settler and no defense
	if( (iSettlerCount > 0) && (iSettlerCount + iWorkerCount == cargoSpace()) )
	{
		// No defenders for settler
		if( plot()->isCity() && plot()->getOwnerINLINE() == getOwnerINLINE() )
		{
			getGroup()->unloadAll();
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
	}

	if ((iSettlerCount > 0) && (isFull() ||
			((getUnitAICargo(UNITAI_CITY_DEFENSE) > 0) &&
			 (GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, MISSIONAI_LOAD_SETTLER) == 0))))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	{
		if (AI_settlerSeaTransport())
		{
			return;
		}
	}
	else if ((getTeam() != plot()->getTeam()) && bEmpty)
	{
		if (AI_pillageRange(3))
		{
			return;
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
	if (plot()->isCity() && plot()->getOwnerINLINE() == getOwnerINLINE() && !hasCargo()
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

//FfH: Added by Kael 02/13/2009
	&& !isHiddenNationality()
//FfH: End Add

	)
	{
		AreaAITypes eAreaAI = area()->getAreaAIType(getTeam());
		if ((eAreaAI == AREAAI_ASSAULT) || (eAreaAI == AREAAI_ASSAULT_MASSING))
		{
			CvArea* pWaterArea = plot()->waterArea();
			FAssert(pWaterArea != NULL);
			if (pWaterArea != NULL)
			{
				if (GET_PLAYER(getOwnerINLINE()).AI_totalWaterAreaUnitAIs(pWaterArea, UNITAI_SETTLER_SEA) > 1)
				{
					if (GET_PLAYER(getOwnerINLINE()).AI_unitValue(getUnitType(), UNITAI_ASSAULT_SEA, pWaterArea) > 0)
					{
						AI_setUnitAIType(UNITAI_ASSAULT_SEA);
						AI_assaultSeaMove();
						return;
					}
				}
			}
		}
	}

	if ((iWorkerCount > 0)
		&& GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, MISSIONAI_LOAD_SETTLER) == 0)
	{
		if (isFull() || (iSettlerCount == 0))
		{
			if (AI_settlerSeaFerry())
			{
				return;
			}
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Settler AI                                                                                   */
/************************************************************************************************/
/* original bts code
	if (AI_pickup(UNITAI_SETTLE))
	{
		return;
	}
*/
	if( !(getGroup()->hasCargo()) )
	{
		if(AI_pickupStranded(UNITAI_SETTLE))
		{
			return;
		}
	}

	if( !(getGroup()->isFull()) )
	{
		if( GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, MISSIONAI_LOAD_SETTLER) > 0 )
		{
			// Wait for units on the way
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}

		if( iSettlerCount > 0 )
		{
			if (AI_pickup(UNITAI_CITY_DEFENSE))
			{
				return;
			}
		}
		else if( cargoSpace() - 2 >= getCargo() + iWorkerCount )
		{
			if (AI_pickup(UNITAI_SETTLE, true))
			{
				return;
			}
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if ((GC.getGame().getGameTurn() - getGameTurnCreated()) < 8)
	{
		if ((plot()->getPlotCity() == NULL) || GET_PLAYER(getOwnerINLINE()).AI_totalAreaUnitAIs(plot()->area(), UNITAI_SETTLE) == 0)
		{
			if (AI_explore())
			{
				return;
			}
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
/* original bts code
	if (AI_pickup(UNITAI_WORKER))
	{
		return;
	}
*/
	if( !getGroup()->hasCargo() )
	{
		// Rescue stranded non-settlers
		if(AI_pickupStranded())
		{
			return;
		}
	}

	if( cargoSpace() - 2 < getCargo() + iWorkerCount )
	{
		// If full of workers and not going anywhere, dump them if a settler is available
		if( (iSettlerCount == 0) && (plot()->plotCount(PUF_isAvailableUnitAITypeGroupie, UNITAI_SETTLE, -1, getOwnerINLINE(), NO_TEAM, PUF_isFiniteRange) > 0) )
		{
			getGroup()->unloadAll();

			if (AI_pickup(UNITAI_SETTLE, true))
			{
				return;
			}

			return;
		}
	}

	if( !(getGroup()->isFull()) )
	{
		if (AI_pickup(UNITAI_WORKER))
		{
			return;
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	if (AI_retreatToCity(true))
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_missionarySeaMove()
{
	PROFILE_FUNC();

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						10/21/08	Solver & jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	if (plot()->isCity(true))
	{
		int iOurDefense = GET_TEAM(getTeam()).AI_getOurPlotStrength(plot(),0,true,false,true);
		int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),2,false,false);

		if( getDamage() > 0 )	// extra risk to leaving when wounded
		{
			iOurDefense *= 2;
		}

		if( iEnemyOffense > iOurDefense/4 || iOurDefense == 0 ) //prioritize getting outta there
		{
			// Retreat to primary area first
			if (AI_retreatToCity(true))
			{
				return;
			}

			if (AI_retreatToCity())
			{
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	if (getUnitAICargo(UNITAI_MISSIONARY) > 0)
	{
		if (AI_specialSeaTransportMissionary())
		{
			return;
		}
	}
	else if (!(getGroup()->hasCargo()))
	{
		if (AI_pillageRange(4))
		{
			return;
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/14/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
	if( !(getGroup()->isFull()) )
	{
		if( GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, MISSIONAI_LOAD_SPECIAL) > 0 )
		{
			// Wait for units on the way
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
	}

	if (AI_pickup(UNITAI_MISSIONARY, true))
	{
		return;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
/*************************************************************************************************/
/**	Tweak							26/01/12								Snarko				**/
/**																								**/
/**						Don't explore with cargo, it's too dangerous.							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	if (AI_explore())
	{
		return;
	}
/**								----  End Original Code  ----									**/
	if (!(getGroup()->hasCargo()))
	{
		if (AI_explore())
		{
			return;
		}
	}
/*************************************************************************************************/
/**	Tweak								END														**/
/*************************************************************************************************/

	if (AI_retreatToCity(true))
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_spySeaMove()
{
	PROFILE_FUNC();

	CvCity* pCity;

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						10/21/08	Solver & jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	if (plot()->isCity(true))
	{
		int iOurDefense = GET_TEAM(getTeam()).AI_getOurPlotStrength(plot(),0,true,false,true);
		int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),2,false,false);

		if( getDamage() > 0 )	// extra risk to leaving when wounded
		{
			iOurDefense *= 2;
		}

		if( iEnemyOffense > iOurDefense/4 || iOurDefense == 0 ) //prioritize getting outta there
		{
			// Retreat to primary area first
			if (AI_retreatToCity(true))
			{
				return;
			}

			if (AI_retreatToCity())
			{
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	if (getUnitAICargo(UNITAI_SPY) > 0)
	{
		if (AI_specialSeaTransportSpy())
		{
			return;
		}

		pCity = plot()->getPlotCity();

		if (pCity != NULL)
		{
			if (pCity->getOwnerINLINE() == getOwnerINLINE())
			{
				getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY, pCity->plot());
				return;
			}
		}
	}
	else if (!(getGroup()->hasCargo()))
	{
		if (AI_pillageRange(5))
		{
			return;
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/14/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
	if( !(getGroup()->isFull()) )
	{
		if( GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, MISSIONAI_LOAD_SPECIAL) > 0 )
		{
			// Wait for units on the way
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
	}

	if (AI_pickup(UNITAI_SPY, true))
	{
		return;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_retreatToCity(true))
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_carrierSeaMove()
{
	PROFILE_FUNC();

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						10/21/08	Solver & jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	if (plot()->isCity(true))
	{
		int iOurDefense = GET_TEAM(getTeam()).AI_getOurPlotStrength(plot(),0,true,false,true);
		int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),2,false,false);

		if( getDamage() > 0 )	// extra risk to leaving when wounded
		{
			iOurDefense *= 2;
		}

		if( iEnemyOffense > iOurDefense/4 || iOurDefense == 0 ) //prioritize getting outta there
		{
			if (AI_retreatToCity(true))
			{
				return;
			}

			if (AI_retreatToCity())
			{
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	if (AI_heal(50))
	{
		return;
	}

	if (!isEnemy(plot()->getTeam()))
	{
		if (GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, MISSIONAI_GROUP) > 0)
		{
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
	}
	else
	{
		if (AI_seaBombardRange(1))
		{
			return;
		}
	}

	if (AI_group(UNITAI_CARRIER_SEA, -1, /*iMaxOwnUnitAI*/ 1))
	{
		return;
	}

	if (getGroup()->countNumUnitAIType(UNITAI_ATTACK_SEA) + getGroup()->countNumUnitAIType(UNITAI_ESCORT_SEA) == 0)
	{
		if (plot()->isCity() && plot()->getOwnerINLINE() == getOwnerINLINE())
		{
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
		if (AI_retreatToCity())
		{
			return;
		}
	}

	if (getCargo() > 0)
	{
		if (AI_carrierSeaTransport())
		{
			return;
		}

		if (AI_blockade())
		{
			return;
		}

		if (AI_shadow(UNITAI_ASSAULT_SEA))
		{
			return;
		}
	}

	if (AI_travelToUpgradeCity())
	{
		return;
	}

	if (AI_retreatToCity(true))
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_missileCarrierSeaMove()
{
/*************************************************************************************************/
/**	CandyMan								04/04/09								Xienwolf	**/
/**																								**/
/**							Allows Multiple Invisible types on a Unit							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	bool bIsStealth = (getInvisibleType() != NO_INVISIBLE);
/**								----  End Original Code  ----									**/
	bool bIsStealth = (getNumInvisibleTypes() > 0);
/*************************************************************************************************/
/**	CandyMan								END													**/
/*************************************************************************************************/
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						06/14/09	Solver & jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	if (plot()->isCity(true))
	{
		int iOurDefense = GET_TEAM(getTeam()).AI_getOurPlotStrength(plot(),0,true,false,true);
		int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),2,false,false);

		if( getDamage() > 0 )	// extra risk to leaving when wounded
		{
			iOurDefense *= 2;
		}

		if( iEnemyOffense > iOurDefense/4 || iOurDefense == 0 ) //prioritize getting outta there
		{
			if (AI_shadow(UNITAI_ASSAULT_SEA, 1, 50, false, true, getMoves()))
			{
				return;
			}

			if (AI_retreatToCity())
			{
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	if (plot()->isCity() && plot()->getTeam() == getTeam())
	{
		if (AI_heal())
		{
			return;
		}
	}

	if (((plot()->getTeam() != getTeam()) && getGroup()->hasCargo()) || getGroup()->AI_isFull())
	{
		if (bIsStealth)
		{
			if (AI_carrierSeaTransport())
			{
				return;
			}
		}
		else
		{
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						06/14/09		jdog5000		*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
			if (AI_shadow(UNITAI_ASSAULT_SEA, 1, 50, true, false, 12))
			{
				return;
			}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

			if (AI_carrierSeaTransport())
			{
				return;
			}
		}
	}
//	if (AI_pickup(UNITAI_ICBM))
//	{
//		return;
//	}
//
//	if (AI_pickup(UNITAI_MISSILE_AIR))
//	{
//		return;
//	}
	if (AI_retreatToCity())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
}


void CvUnitAI::AI_attackAirMove()
{
	PROFILE_FUNC();

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						10/21/08	Solver & jdog5000	*/
/* 																			*/
/* 	Air AI																	*/
/********************************************************************************/
	CvCity* pCity = plot()->getPlotCity();
	bool bSkiesClear = true;
	int iDX, iDY;

	// Check for sufficient defenders to stay
	int iDefenders = plot()->plotCount(PUF_canDefend, -1, -1, plot()->getOwner());

	int iAttackAirCount = plot()->plotCount(PUF_canAirAttack, -1, -1, NO_PLAYER, getTeam());
	iAttackAirCount += 2 * plot()->plotCount(PUF_isUnitAIType, UNITAI_ICBM, -1, NO_PLAYER, getTeam());

	if( plot()->isCoastalLand(GC.getMIN_WATER_SIZE_FOR_OCEAN()) )
	{
		iDefenders -= 1;
	}

	if( pCity != NULL )
	{
		if( pCity->getDefenseModifier(true) < 40 )
		{
			iDefenders -= 1;
		}

		if( pCity->getOccupationTimer() > 1 )
		{
			iDefenders -= 1;
		}
	}

	if( iAttackAirCount > iDefenders )
	{
		if (AI_airOffensiveCity())
		{
			return;
		}
	}

	// Check for direct threat to current base
	if (plot()->isCity(true))
	{
		int iOurDefense = GET_TEAM(getTeam()).AI_getOurPlotStrength(plot(),0,true,false,true);
		int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),2,false,false);

		if (iEnemyOffense > iOurDefense || iOurDefense == 0)
		{
			// Too risky, pull back
			if (AI_airOffensiveCity())
			{
				return;
			}

			if( canAirDefend() )
			{
				if (AI_airDefensiveCity())
				{
					return;
				}
			}
		}
		else if( iEnemyOffense > iOurDefense/3 )
		{
			if( getDamage() == 0 )
			{
				if( collateralDamage() == 0 && canAirDefend() )
				{
					if (pCity != NULL)
					{
						// Check for whether city needs this unit to air defend
						if( !(pCity->AI_isAirDefended(true,-1)) )
						{
							getGroup()->pushMission(MISSION_AIRPATROL);
							return;
						}
					}
				}

				// Attack the invaders!
				if (AI_defendBaseAirStrike())
				{
					return;
				}

				if (AI_defensiveAirStrike())
				{
					return;
				}

				if (AI_airStrike())
				{
					return;
				}

				// If no targets, no sense staying in risky place
				if (AI_airOffensiveCity())
				{
					return;
				}

				if( canAirDefend() )
				{
					if (AI_airDefensiveCity())
					{
						return;
					}
				}
			}

			if( healTurns(plot()) > 1 )
			{
				// If very damaged, no sense staying in risky place
				if (AI_airOffensiveCity())
				{
					return;
				}

				if( canAirDefend() )
				{
					if (AI_airDefensiveCity())
					{
						return;
					}
				}
			}

		}
	}

	if( getDamage() > 0 )
	{
		if (((100*currHitPoints()) / maxHitPoints()) < 40)
		{
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
		else
		{
			CvPlot *pLoopPlot;
			int iSearchRange = airRange();
			for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
			{
				if (!bSkiesClear) break;
				for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
				{
					pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

					if (pLoopPlot != NULL)
					{
						if (bestInterceptor(pLoopPlot) != NULL)
						{
							bSkiesClear = false;
							break;
						}
					}
				}
			}

			if (!bSkiesClear)
			{
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());
	CvArea* pArea = area();
	int iAttackValue = kPlayer.AI_unitValue(getUnitType(), UNITAI_ATTACK_AIR, pArea);
	int iCarrierValue = kPlayer.AI_unitValue(getUnitType(), UNITAI_CARRIER_AIR, pArea);
	if (iCarrierValue > 0)
	{
		int iCarriers = kPlayer.AI_totalUnitAIs(UNITAI_CARRIER_SEA);
		if (iCarriers > 0)
		{
			UnitTypes eBestCarrierUnit = NO_UNIT;
			kPlayer.AI_bestAreaUnitAIValue(UNITAI_CARRIER_SEA, NULL, &eBestCarrierUnit);
			if (eBestCarrierUnit != NO_UNIT)
			{
				int iCarrierAirNeeded = iCarriers * GC.getUnitInfo(eBestCarrierUnit).getCargoSpace();
				if (kPlayer.AI_totalUnitAIs(UNITAI_CARRIER_AIR) < iCarrierAirNeeded)
				{
					AI_setUnitAIType(UNITAI_CARRIER_AIR);
					getGroup()->pushMission(MISSION_SKIP);
					return;
				}
			}
		}
	}

	int iDefenseValue = kPlayer.AI_unitValue(getUnitType(), UNITAI_DEFENSE_AIR, pArea);
	if (iDefenseValue > iAttackValue)
	{
		if (kPlayer.AI_bestAreaUnitAIValue(UNITAI_ATTACK_AIR, pArea) > iAttackValue)
		{
			AI_setUnitAIType(UNITAI_DEFENSE_AIR);
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
	}

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						10/6/08			jdog5000		*/
/* 																			*/
/* 	Air AI																	*/
/********************************************************************************/
	/* original BTS code
	if (AI_airBombDefenses())
	{
		return;
	}

	if (GC.getGameINLINE().getSorenRandNum(4, "AI Air Attack Move") == 0)
	{
		if (AI_airBombPlots())
		{
			return;
		}
	}

	if (AI_airStrike())
	{
		return;
	}

	if (canAirAttack())
	{
		if (AI_airOffensiveCity())
		{
			return;
		}
	}

	if (canRecon(plot()))
	{
		if (AI_exploreAir())
		{
			return;
		}
	}

	if (canAirDefend())
	{
		getGroup()->pushMission(MISSION_AIRPATROL);
		return;
	}
	*/
	bool bDefensive = false;
	if( pArea != NULL )
	{
		bDefensive = pArea->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE;
	}

	if (GC.getGameINLINE().getSorenRandNum(bDefensive ? 3 : 6, "AI Air Attack Move") == 0)
	{
		if( AI_defensiveAirStrike() )
		{
			return;
		}
	}

	if (GC.getGameINLINE().getSorenRandNum(4, "AI Air Attack Move") == 0)
	{
		// only moves unit in a fort
		if (AI_travelToUpgradeCity())
		{
			return;
		}
	}

	// Support ground attacks
	if (AI_airBombDefenses())
	{
		return;
	}

	if (GC.getGameINLINE().getSorenRandNum(bDefensive ? 6 : 4, "AI Air Attack Move") == 0)
	{
		if (AI_airBombPlots())
		{
			return;
		}
	}

	if (AI_airStrike())
	{
		return;
	}

	if (canAirAttack())
	{
		if (AI_airOffensiveCity())
		{
			return;
		}
	}
	else
	{
		if( canAirDefend() )
		{
			if (AI_airDefensiveCity())
			{
				return;
			}
		}
	}

	// BBAI TODO: Support friendly attacks on common enemies, if low risk?

	if (canAirDefend())
	{
		if( bDefensive || GC.getGameINLINE().getSorenRandNum(2, "AI Air Attack Move") == 0 )
		{
			getGroup()->pushMission(MISSION_AIRPATROL);
			return;
		}
	}

	if (canRecon(plot()))
	{
		if (AI_exploreAir())
		{
			return;
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/


	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_defenseAirMove()
{
	PROFILE_FUNC();

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						10/21/08	Solver & jdog5000	*/
/* 																			*/
/* 	Air AI																	*/
/********************************************************************************/
	CvCity* pCity = plot()->getPlotCity();

	int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),2,false,false);

	// includes forts
	if (plot()->isCity(true))
	{
		int iOurDefense = GET_TEAM(getTeam()).AI_getOurPlotStrength(plot(),0,true,false,true);

		if (3*iEnemyOffense > 4*iOurDefense || iOurDefense == 0)
		{
			// Too risky, pull out
			// AI_airDefensiveCity will leave some air defense, pull extras out
			if (AI_airDefensiveCity())
			{
				return;
			}
		}
		else if ( iEnemyOffense > iOurDefense/3 )
		{
			if (getDamage() > 0)
			{
				if( healTurns(plot()) > 1 + GC.getGameINLINE().getSorenRandNum(2, "AI Air Defense Move") )
				{
					// Can't help current situation, only risk losing unit
					if (AI_airDefensiveCity())
					{
						return;
					}
				}

				// Stay to defend in the future
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}

			if (canAirDefend() && pCity != NULL)
			{
				// Check for whether city needs this unit to air defend
				if( !(pCity->AI_isAirDefended(true,-1)) )
				{
					getGroup()->pushMission(MISSION_AIRPATROL);
					return;
				}

				// Consider adding extra defenders
				if( collateralDamage() == 0 && (!pCity->AI_isAirDefended(false,-2)) )
				{
					if( GC.getGameINLINE().getSorenRandNum(3, "AI Air Defense Move") == 0 )
					{
						getGroup()->pushMission(MISSION_AIRPATROL);
						return;
					}
				}
			}

			// Attack the invaders!
			if (AI_defendBaseAirStrike())
			{
				return;
			}

			if (AI_defensiveAirStrike())
			{
				return;
			}

			if (AI_airStrike())
			{
				return;
			}

			if (AI_airDefensiveCity())
			{
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	if (getDamage() > 0)
	{
		getGroup()->pushMission(MISSION_SKIP);
		return;
	}

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						10/17/08	Solver & jdog5000	*/
/* 																			*/
/* 	Air AI																	*/
/********************************************************************************/
	/* original BTS code
	if ((GC.getGameINLINE().getSorenRandNum(2, "AI Air Defense Move") == 0))
	{
		if ((pCity != NULL) && pCity->AI_isDanger())
		{
			if (AI_airStrike())
			{
				return;
			}
		}
		else
		{
			if (AI_airBombDefenses())
			{
				return;
			}

			if (AI_airStrike())
			{
				return;
			}

			if (AI_getBirthmark() % 2 == 0)
			{
				if (AI_airBombPlots())
				{
					return;
				}
			}
		}

		if (AI_travelToUpgradeCity())
		{
			return;
		}
	}

	bool bNoWar = (GET_TEAM(getTeam()).getAtWarCount(false) == 0);

	if (canRecon(plot()))
	{
		if (GC.getGame().getSorenRandNum(bNoWar ? 2 : 4, "AI defensive air recon") == 0)
		{
			if (AI_exploreAir())
			{
				return;
			}
		}
	}

	if (AI_airDefensiveCity())
	{
		return;
	}
	*/
	if((GC.getGameINLINE().getSorenRandNum(4, "AI Air Defense Move") == 0))
	{
		// only moves unit in a fort
		if (AI_travelToUpgradeCity())
		{
			return;
		}
	}

	if( canAirDefend() )
	{
		// Check for whether city needs this unit for base air defenses
		int iBaseAirDefenders = 0;

		if( iEnemyOffense > 0 )
		{
			iBaseAirDefenders++;
		}

		if( pCity != NULL )
		{
			iBaseAirDefenders += pCity->AI_neededAirDefenders()/2;
		}

		if( plot()->countAirInterceptorsActive(getTeam()) < iBaseAirDefenders )
		{
			getGroup()->pushMission(MISSION_AIRPATROL);
			return;
		}
	}

	CvArea* pArea = area();
	bool bDefensive = false;
	bool bOffensive = false;

	if( pArea != NULL )
	{
		bDefensive = (pArea->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE);
		bOffensive = (pArea->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE);
	}

	if( (iEnemyOffense > 0) || bDefensive )
	{
		if( canAirDefend() )
		{
			if( pCity != NULL )
			{
				// Consider adding extra defenders
				if( !(pCity->AI_isAirDefended(false,-1)) )
				{
					if ((GC.getGameINLINE().getSorenRandNum((bOffensive ? 3 : 2), "AI Air Defense Move") == 0))
					{
						getGroup()->pushMission(MISSION_AIRPATROL);
						return;
					}
				}
			}
			else
			{
				if ((GC.getGameINLINE().getSorenRandNum((bOffensive ? 3 : 2), "AI Air Defense Move") == 0))
				{
					getGroup()->pushMission(MISSION_AIRPATROL);
					return;
				}
			}
		}

		if((GC.getGameINLINE().getSorenRandNum(3, "AI Air Defense Move") > 0))
		{
			if (AI_defensiveAirStrike())
			{
				return;
			}

			if (AI_airStrike())
			{
				return;
			}
		}
	}
	else
	{
		if ((GC.getGameINLINE().getSorenRandNum(3, "AI Air Defense Move") > 0))
		{
			// Clear out any enemy fighters, support offensive units
			if (AI_airBombDefenses())
			{
				return;
			}

			if (GC.getGameINLINE().getSorenRandNum(3, "AI Air Defense Move") == 0)
			{
				// Hit enemy land stacks near our cities
				if (AI_defensiveAirStrike())
				{
					return;
				}
			}

			if (AI_airStrike())
			{
				return;
			}

			if (AI_getBirthmark() % 2 == 0 || bOffensive)
			{
				if (AI_airBombPlots())
				{
					return;
				}
			}
		}
	}

	if (AI_airDefensiveCity())
	{
		return;
	}

/*************************************************************************************************/
/**	Improved AI							06/05/11										Snarko	**/
/**						Getting AI to use air units correctly									**/
/*************************************************************************************************/
	if (AI_exploreAir())
	{
		return;
	}

	if (AI_exploreAirNear(airRange()))
	{
		return;
	}
/*************************************************************************************************/
/**	Improved AI									END													**/
/*************************************************************************************************/

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	if (canAirDefend())
	{
		getGroup()->pushMission(MISSION_AIRPATROL);
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_carrierAirMove()
{
	PROFILE_FUNC();

	// XXX maybe protect land troops?

	if (getDamage() > 0)
	{
		getGroup()->pushMission(MISSION_SKIP);
		return;
	}

	if (isCargo())
	{
		int iRand = GC.getGameINLINE().getSorenRandNum(3, "AI Air Missile plot bombing");
		if (iRand != 0)
		{
			if (AI_airBombPlots())
			{
				return;
			}
		}

		iRand = GC.getGameINLINE().getSorenRandNum(3, "AI Air Missile Carrier Move");

		if (iRand == 2 && canAirDefend())
		{
			getGroup()->pushMission(MISSION_AIRPATROL);
			return;
		}
		else if (AI_airBombDefenses())
		{
			return;
		}
		else if (iRand == 1)
		{
			if (AI_airBombPlots())
			{
				return;
			}

			if (AI_airStrike())
			{
				return;
			}
		}
		else
		{
			if (AI_airStrike())
			{
				return;
			}

			if (AI_airBombPlots())
			{
				return;
			}
		}

		if (AI_travelToUpgradeCity())
		{
			return;
		}

		if (canAirDefend())
		{
			getGroup()->pushMission(MISSION_AIRPATROL);
			return;
		}
		getGroup()->pushMission(MISSION_SKIP);
		return;
	}

	if (AI_airCarrier())
	{
		return;
	}

	if (AI_airDefensiveCity())
	{
		return;
	}

	if (canAirDefend())
	{
		getGroup()->pushMission(MISSION_AIRPATROL);
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_missileAirMove()
{
	PROFILE_FUNC();

	CvCity* pCity = plot()->getPlotCity();

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						10/21/08	Solver & jdog5000	*/
/* 																			*/
/* 	Air AI																	*/
/********************************************************************************/
	// includes forts
	if (!isCargo() && plot()->isCity(true))
	{
		int iOurDefense = GET_TEAM(getTeam()).AI_getOurPlotStrength(plot(),0,true,false,true);
		int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),2,false,false);

		if (iEnemyOffense > (iOurDefense/2) || iOurDefense == 0)
		{
			if (AI_airOffensiveCity())
			{
				return;
			}
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	if (isCargo())
	{
		int iRand = GC.getGameINLINE().getSorenRandNum(3, "AI Air Missile Carrier Move");

		if (iRand == 0)
		{
			if (AI_airBombDefenses())
			{
				return;
			}

			if (AI_airStrike())
			{
				return;
			}
		}
		else
		{
			if (AI_airStrike())
			{
				return;
			}

			if (AI_airBombDefenses())
			{
				return;
			}
		}

		if (AI_airBombPlots())
		{
			return;
		}

		getGroup()->pushMission(MISSION_SKIP);
		return;
	}

	if (AI_airStrike())
	{
		return;
	}

	if (AI_missileLoad(UNITAI_MISSILE_CARRIER_SEA))
	{
		return;
	}

	if (AI_missileLoad(UNITAI_RESERVE_SEA, 1))
	{
		return;
	}

	if (AI_missileLoad(UNITAI_ATTACK_SEA, 1))
	{
		return;
	}

	if (AI_airBombDefenses())
	{
		return;
	}

	if (!isCargo())
	{
		if (AI_airOffensiveCity())
		{
			return;
		}
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_networkAutomated()
{
	FAssertMsg(canBuildRoute(), "canBuildRoute is expected to be true");

	if (!(getGroup()->canDefend()))
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
		//if (GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot()) > 0)
		if (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot()))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		{
			if (AI_retreatToCity()) // XXX maybe not do this??? could be working productively somewhere else...
			{
				return;
			}
		}
	}

	if (AI_improveBonus(20))
	{
		return;
	}

	if (AI_improveBonus(10))
	{
		return;
	}

	if (AI_connectBonus())
	{
		return;
	}

	if (AI_connectCity())
	{
		return;
	}

	if (AI_improveBonus())
	{
		return;
	}

	if (AI_routeTerritory(true))
	{
		return;
	}

	if (AI_connectBonus(false))
	{
		return;
	}

	if (AI_routeCity())
	{
		return;
	}

	if (AI_routeTerritory())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_cityAutomated()
{
	CvCity* pCity;

	if (!(getGroup()->canDefend()))
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
		//if (GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot()) > 0)
		if (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot()))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		{
			if (AI_retreatToCity()) // XXX maybe not do this??? could be working productively somewhere else...
			{
				return;
			}
		}
	}

	pCity = NULL;

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		pCity = plot()->getWorkingCity();
	}

	if (pCity == NULL)
	{
		pCity = GC.getMapINLINE().findCity(getX_INLINE(), getY_INLINE(), getOwnerINLINE()); // XXX do team???
	}

	if (pCity != NULL)
	{
		if (AI_improveCity(pCity))
		{
			return;
		}
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


// XXX make sure we include any new UnitAITypes...
/*************************************************************************************************/
/**	Xienwolf Tweak							03/27/09											**/
/**	NOTE:  kPromotion method is faster, but not comment-friendly, so isn't commented			**/
/**						Chipotle Support for Decoding AI love of Promos							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
int CvUnitAI::AI_promotionValue(PromotionTypes ePromotion)
{
/**								----  End Original Code  ----									**/
int CvUnitAI::AI_promotionValue(PromotionTypes ePromotion, bool bSkipRandom, bool bSkipNegative) const
{
	CvPromotionInfo &kPromotion = GC.getPromotionInfo(ePromotion);
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
	int iValue;
	int iTemp;
	int iExtra;
	int iI;

	iValue = 0;

	if (kPromotion.isLeader())
	{
		// Don't consume the leader as a regular promotion
		return 0;
	}

	if (kPromotion.isBlitz())
	{
		if ((AI_getUnitAIType() == UNITAI_RESERVE  && baseMoves() > 1) ||
			AI_getUnitAIType() == UNITAI_PARADROP)
		{
			iValue += 10;
		}
		else
		{

//FfH: Modified by Kael 06/28/2008
//			iValue += 2;
			iValue += 3 * baseMoves();
//FfH: End Modify

		}
	}

	if (kPromotion.isAmphib())
	{
		if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
			  (AI_getUnitAIType() == UNITAI_ATTACK_CITY))
		{
			iValue += 5;
		}
		else
		{
			iValue++;
		}
	}

	if (kPromotion.isRiver())
	{
		if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
			  (AI_getUnitAIType() == UNITAI_ATTACK_CITY))
		{
			iValue += 5;
		}
		else
		{
			iValue++;
		}
	}

	if (kPromotion.isEnemyRoute())
	{
		if (AI_getUnitAIType() == UNITAI_PILLAGE)
		{
			iValue += 40;
		}
		else if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
				   (AI_getUnitAIType() == UNITAI_ATTACK_CITY))
		{
			iValue += 20;
		}
		else if (AI_getUnitAIType() == UNITAI_PARADROP)
		{
			iValue += 10;
		}
		else
		{
			iValue += 4;
		}
	}

	if (kPromotion.isAlwaysHeal())
	{
/*************************************************************************************************/
/**	Xienwolf Tweak							02/06/09											**/
/**																								**/
/**									Enhanced Barbarian Logic									**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
			  (AI_getUnitAIType() == UNITAI_ATTACK_CITY) ||
				(AI_getUnitAIType() == UNITAI_PILLAGE) ||
				(AI_getUnitAIType() == UNITAI_COUNTER) ||
				(AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
				(AI_getUnitAIType() == UNITAI_PIRATE_SEA) ||
				(AI_getUnitAIType() == UNITAI_ESCORT_SEA) ||
				(AI_getUnitAIType() == UNITAI_PARADROP))
		{
			iValue += 10;
		}
		else
		{
			iValue += 8;
		}
/**								----  End Original Code  ----									**/
		if (!isBarbarian())
		{
			if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
				  (AI_getUnitAIType() == UNITAI_ATTACK_CITY) ||
					(AI_getUnitAIType() == UNITAI_PILLAGE) ||
					(AI_getUnitAIType() == UNITAI_COUNTER) ||
					(AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
					(AI_getUnitAIType() == UNITAI_PIRATE_SEA) ||
					(AI_getUnitAIType() == UNITAI_ESCORT_SEA) ||
					(AI_getUnitAIType() == UNITAI_PARADROP))
			{
				iValue += 10;
			}
			else
			{
				iValue += 8;
			}
		}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
	}

	if (kPromotion.isHillsDoubleMove())
	{
		if (AI_getUnitAIType() == UNITAI_EXPLORE)
		{
			iValue += 20;
		}
		else
		{
			iValue += 10;
		}
	}

	if (kPromotion.isImmuneToFirstStrikes()
		&& !immuneToFirstStrikes())
	{
		if ((AI_getUnitAIType() == UNITAI_ATTACK_CITY))
		{
			iValue += 12;
		}
		else if ((AI_getUnitAIType() == UNITAI_ATTACK))
		{
			iValue += 8;
		}
		else
		{
			iValue += 4;
		}
	}

	iTemp = kPromotion.getVisibilityChange();
	if ((AI_getUnitAIType() == UNITAI_EXPLORE_SEA) ||
		(AI_getUnitAIType() == UNITAI_EXPLORE))
	{
		iValue += (iTemp * 40);
	}
	else if (AI_getUnitAIType() == UNITAI_PIRATE_SEA)
	{
		iValue += (iTemp * 20);
	}

	iTemp = kPromotion.getMovesChange();
	if ((AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
		(AI_getUnitAIType() == UNITAI_PIRATE_SEA) ||
		  (AI_getUnitAIType() == UNITAI_RESERVE_SEA) ||
		  (AI_getUnitAIType() == UNITAI_ESCORT_SEA) ||
			(AI_getUnitAIType() == UNITAI_EXPLORE_SEA) ||
			(AI_getUnitAIType() == UNITAI_ASSAULT_SEA) ||
			(AI_getUnitAIType() == UNITAI_SETTLER_SEA) ||
			(AI_getUnitAIType() == UNITAI_PILLAGE) ||
			(AI_getUnitAIType() == UNITAI_ATTACK) ||
			(AI_getUnitAIType() == UNITAI_PARADROP))
	{
		iValue += (iTemp * 20);
	}
	else
	{
		iValue += (iTemp * 4);
	}

	iTemp = kPromotion.getMoveDiscountChange();
	if (AI_getUnitAIType() == UNITAI_PILLAGE)
	{
		iValue += (iTemp * 10);
	}
	else
	{
		iValue += (iTemp * 2);
	}

	iTemp = kPromotion.getAirRangeChange();
	if (AI_getUnitAIType() == UNITAI_ATTACK_AIR ||
		AI_getUnitAIType() == UNITAI_CARRIER_AIR)
	{
		iValue += (iTemp * 20);
	}
	else if (AI_getUnitAIType() == UNITAI_DEFENSE_AIR)
	{
		iValue += (iTemp * 10);
	}

	iTemp = kPromotion.getInterceptChange();
	if (AI_getUnitAIType() == UNITAI_DEFENSE_AIR)
	{
		iValue += (iTemp * 3);
	}
	else if (AI_getUnitAIType() == UNITAI_CITY_SPECIAL || AI_getUnitAIType() == UNITAI_CARRIER_AIR)
	{
		iValue += (iTemp * 2);
	}
	else
	{
		iValue += (iTemp / 10);
	}

	iTemp = kPromotion.getEvasionChange();
	if (AI_getUnitAIType() == UNITAI_ATTACK_AIR || AI_getUnitAIType() == UNITAI_CARRIER_AIR)
	{
		iValue += (iTemp * 3);
	}
	else
	{
		iValue += (iTemp / 10);
	}

	iTemp = kPromotion.getFirstStrikesChange() * 2;
	iTemp += kPromotion.getChanceFirstStrikesChange();
	if ((AI_getUnitAIType() == UNITAI_RESERVE) ||
		  (AI_getUnitAIType() == UNITAI_COUNTER) ||
			(AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
			(AI_getUnitAIType() == UNITAI_CITY_COUNTER) ||
			(AI_getUnitAIType() == UNITAI_CITY_SPECIAL) ||
			(AI_getUnitAIType() == UNITAI_ATTACK))
	{
		iTemp *= 8;
		iExtra = getExtraChanceFirstStrikes() + getExtraFirstStrikes() * 2;
		iTemp *= 100 + iExtra * 15;
		iTemp /= 100;
/*************************************************************************************************/
/** Unofficial Patch  Fixed bug where some AI unit types mistakenly ignored the value of first-strike promotions. [ViterboKnight]   adopted for Orbis by Ahwaric      18.06.09   **/
/*************************************************************************************************/
		iValue += (iTemp * 1);
/*************************************************************************************************/
/** Unofficial Patch  END													**/
/*************************************************************************************************/
	}
	else
	{
		iValue += (iTemp * 5);
	}


	iTemp = kPromotion.getWithdrawalChange();
	if (iTemp != 0)
	{
		iExtra = (m_pUnitInfo->getWithdrawalProbability() + (getExtraWithdrawal() * 4));
		iTemp *= (100 + iExtra);
		iTemp /= 100;
		if ((AI_getUnitAIType() == UNITAI_ATTACK_CITY))
		{
			iValue += (iTemp * 4) / 3;
		}
		else if ((AI_getUnitAIType() == UNITAI_COLLATERAL) ||
			  (AI_getUnitAIType() == UNITAI_RESERVE) ||
			  (AI_getUnitAIType() == UNITAI_RESERVE_SEA) ||
			  getLeaderUnitType() != NO_UNIT)
		{
			iValue += iTemp * 1;
		}
		else
		{
			iValue += (iTemp / 4);
		}
	}
	iTemp = kPromotion.getEnemyWithdrawalChange();
	if (iTemp != 0)
	{
		iExtra = (m_pUnitInfo->getEnemyWithdrawalProbability() + (getExtraEnemyWithdrawal() * 4));
		iTemp *= (100 + iExtra);
		iTemp /= 100;
		if ((AI_getUnitAIType() == UNITAI_ATTACK_CITY))
		{
			iValue -= (iTemp * 4) / 3;
		}
		else if ((AI_getUnitAIType() == UNITAI_COLLATERAL) ||
			  (AI_getUnitAIType() == UNITAI_RESERVE) ||
			  (AI_getUnitAIType() == UNITAI_RESERVE_SEA) ||
			  getLeaderUnitType() != NO_UNIT)
		{
			iValue -= iTemp * 1;
		}
		else
		{
			iValue -= (iTemp / 4);
		}
	}
	iTemp = kPromotion.getCollateralDamageChange();
	if (iTemp != 0)
	{
		iExtra = (getExtraCollateralDamage());//collateral has no strong synergy (not like retreat)
		iTemp *= (100 + iExtra);
		iTemp /= 100;

		if (AI_getUnitAIType() == UNITAI_COLLATERAL)
		{
			iValue += (iTemp * 1);
		}
		else if (AI_getUnitAIType() == UNITAI_ATTACK_CITY)
		{
			iValue += ((iTemp * 2) / 3);
		}
		else
		{
			iValue += (iTemp / 8);
		}
	}

	iTemp = kPromotion.getBombardRateChange();
	if (AI_getUnitAIType() == UNITAI_ATTACK_CITY)
	{
		iValue += (iTemp * 2);
	}
	else
	{
		iValue += (iTemp / 8);
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/26/10                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	iTemp = kPromotion.getEnemyHealChange();
	if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
		(AI_getUnitAIType() == UNITAI_PILLAGE) ||
		(AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
		(AI_getUnitAIType() == UNITAI_PARADROP) ||
		(AI_getUnitAIType() == UNITAI_PIRATE_SEA))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	{
		iValue += (iTemp / 4);
	}
	else
	{
		iValue += (iTemp / 8);
	}

	iTemp = kPromotion.getNeutralHealChange();
	iValue += (iTemp / 8);

	iTemp = kPromotion.getFriendlyHealChange();
	if ((AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
		  (AI_getUnitAIType() == UNITAI_CITY_COUNTER) ||
		  (AI_getUnitAIType() == UNITAI_CITY_SPECIAL))
	{
		iValue += (iTemp / 4);
	}
	else
	{
		iValue += (iTemp / 8);
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/26/10                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	if ( getDamage() > 0 || ((AI_getBirthmark() % 8 == 0) && (AI_getUnitAIType() == UNITAI_COUNTER ||
															AI_getUnitAIType() == UNITAI_PILLAGE ||
															AI_getUnitAIType() == UNITAI_ATTACK_CITY ||
															AI_getUnitAIType() == UNITAI_RESERVE )) )
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		iTemp = kPromotion.getSameTileHealChange() + getSameTileHeal();
		iExtra = getSameTileHeal();

		iTemp *= (100 + iExtra * 5);
		iTemp /= 100;

		if (iTemp > 0)
		{
			if (healRate(plot()) < iTemp)
			{
				iValue += iTemp * ((getGroup()->getNumUnits() > 4) ? 4 : 2);
			}
			else
			{
				iValue += (iTemp / 8);
			}
		}

		iTemp = kPromotion.getAdjacentTileHealChange();
		iExtra = getAdjacentTileHeal();
		iTemp *= (100 + iExtra * 5);
		iTemp /= 100;
		if (getSameTileHeal() >= iTemp)
		{
			iValue += (iTemp * ((getGroup()->getNumUnits() > 9) ? 4 : 2));
		}
		else
		{
			iValue += (iTemp / 4);
		}
	}

/*************************************************************************************************/
/**	Cutting								30/01/12										Snarko	**/
/**																								**/
/**		Removing rarely used/redundant stuff to improve speed.									**/
/**		Checking every promotion is heavy stuff.												**/
/**		Checking every promotion for isLeader, which isn't used in RifE, is just silly			**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	// try to use Warlords to create super-medic units
	if (kPromotion.getAdjacentTileHealChange() > 0 || kPromotion.getSameTileHealChange() > 0)
	{
		PromotionTypes eLeader = NO_PROMOTION;
		for (iI = 0; iI < GC.getNumPromotionInfos(); iI++)
		{
			if (GC.getPromotionInfo((PromotionTypes)iI).isLeader())
			{
				eLeader = (PromotionTypes)iI;
			}
		}

		if (isHasPromotion(eLeader) && eLeader != NO_PROMOTION)
		{
			iValue += kPromotion.getAdjacentTileHealChange() + kPromotion.getSameTileHealChange();
		}
	}
/**								----  End Original Code  ----									**/
/*************************************************************************************************/
/**	Cutting									END													**/
/*************************************************************************************************/

	iTemp = kPromotion.getCombatPercent();
	if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
		(AI_getUnitAIType() == UNITAI_COUNTER) ||
		(AI_getUnitAIType() == UNITAI_CITY_COUNTER) ||
		  (AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
		  (AI_getUnitAIType() == UNITAI_RESERVE_SEA) ||
			(AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
			(AI_getUnitAIType() == UNITAI_PARADROP) ||
			(AI_getUnitAIType() == UNITAI_PIRATE_SEA) ||
			(AI_getUnitAIType() == UNITAI_RESERVE_SEA) ||
			(AI_getUnitAIType() == UNITAI_ESCORT_SEA) ||
			(AI_getUnitAIType() == UNITAI_CARRIER_SEA) ||
			(AI_getUnitAIType() == UNITAI_ATTACK_AIR) ||
			(AI_getUnitAIType() == UNITAI_CARRIER_AIR))
	{
		iValue += (iTemp * 2);
	}
	else
	{
		iValue += (iTemp * 1);
	}

/*************************************************************************************************/
/**	1.4										03/28/11								Valkrionn	**/
/**																								**/
/**									New tags required for 1.4									**/
/*************************************************************************************************/
	iTemp = kPromotion.getRangedCombatPercent();
	if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
		(AI_getUnitAIType() == UNITAI_COUNTER) ||
		(AI_getUnitAIType() == UNITAI_CITY_COUNTER) ||
		  (AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
		  (AI_getUnitAIType() == UNITAI_RESERVE_SEA) ||
			(AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
			(AI_getUnitAIType() == UNITAI_PARADROP) ||
			(AI_getUnitAIType() == UNITAI_PIRATE_SEA) ||
			(AI_getUnitAIType() == UNITAI_RESERVE_SEA) ||
			(AI_getUnitAIType() == UNITAI_ESCORT_SEA) ||
			(AI_getUnitAIType() == UNITAI_CARRIER_SEA) ||
			(AI_getUnitAIType() == UNITAI_ATTACK_AIR) ||
			(AI_getUnitAIType() == UNITAI_CARRIER_AIR))
	{
		iValue += (iTemp * 2);
	}
	else
	{
		iValue += (iTemp * 1);
	}
/*************************************************************************************************/
/**												END												**/
/*************************************************************************************************/

	iTemp = kPromotion.getCityAttackPercent();
	if (iTemp != 0)
	{
		if (m_pUnitInfo->getUnitAIType(UNITAI_ATTACK) || m_pUnitInfo->getUnitAIType(UNITAI_ATTACK_CITY) || m_pUnitInfo->getUnitAIType(UNITAI_ATTACK_CITY_LEMMING))
		{
			iExtra = (m_pUnitInfo->getCityAttackModifier() + (getExtraCityAttackPercent() * 2));
			iTemp *= (100 + iExtra);
			iTemp /= 100;
			if (AI_getUnitAIType() == UNITAI_ATTACK_CITY)
			{
				iValue += (iTemp * 1);
			}
			else
			{
				iValue -= iTemp / 4;
			}
		}
	}

	iTemp = kPromotion.getCityDefensePercent();
	if (iTemp != 0)
	{
		if ((AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
			  (AI_getUnitAIType() == UNITAI_CITY_SPECIAL))
		{
			iExtra = m_pUnitInfo->getCityDefenseModifier() + (getExtraCityDefensePercent() * 2);
			iValue += ((iTemp * (100 + iExtra)) / 100);
		}
		else
		{
			iValue += (iTemp / 4);
		}
	}

	iTemp = kPromotion.getHillsAttackPercent();
	if (iTemp != 0)
	{
		iExtra = getExtraHillsAttackPercent();
		iTemp *= (100 + iExtra * 2);
		iTemp /= 100;
		if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
			(AI_getUnitAIType() == UNITAI_COUNTER))
		{
			iValue += (iTemp / 4);
		}
		else
		{
			iValue += (iTemp / 16);
		}
	}

	iTemp = kPromotion.getHillsDefensePercent();
	if (iTemp != 0)
	{
		iExtra = (m_pUnitInfo->getHillsDefenseModifier() + (getExtraHillsDefensePercent() * 2));
		iTemp *= (100 + iExtra);
		iTemp /= 100;
		if (AI_getUnitAIType() == UNITAI_CITY_DEFENSE)
		{
			if (plot()->isCity() && plot()->isHills())
			{
				iValue += (iTemp * 4) / 3;
			}
		}
		else if (AI_getUnitAIType() == UNITAI_COUNTER)
		{
			if (plot()->isHills())
			{
				iValue += (iTemp / 4);
			}
			else
			{
				iValue++;
			}
		}
		else
		{
			iValue += (iTemp / 16);
		}
	}

	iTemp = kPromotion.getRevoltProtection();
	if ((AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
		(AI_getUnitAIType() == UNITAI_CITY_COUNTER) ||
		(AI_getUnitAIType() == UNITAI_CITY_SPECIAL))
	{
		if (iTemp > 0)
		{
			PlayerTypes eOwner = plot()->calculateCulturalOwner();
			if (eOwner != NO_PLAYER && GET_PLAYER(eOwner).getTeam() != GET_PLAYER(getOwnerINLINE()).getTeam())
			{
				iValue += (iTemp / 2);
			}
		}
	}

	iTemp = kPromotion.getCollateralDamageProtection();
	if ((AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
		(AI_getUnitAIType() == UNITAI_CITY_COUNTER) ||
		(AI_getUnitAIType() == UNITAI_CITY_SPECIAL))
	{
		iValue += (iTemp / 3);
	}
	else if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
		(AI_getUnitAIType() == UNITAI_COUNTER))
	{
		iValue += (iTemp / 4);
	}
	else
	{
		iValue += (iTemp / 8);
	}

	iTemp = kPromotion.getPillageChange();
	if (AI_getUnitAIType() == UNITAI_PILLAGE ||
		AI_getUnitAIType() == UNITAI_ATTACK_SEA ||
		AI_getUnitAIType() == UNITAI_PIRATE_SEA)
	{
		iValue += (iTemp / 4);
	}
	else
	{
		iValue += (iTemp / 16);
	}

	iTemp = kPromotion.getUpgradeDiscount();
	iValue += (iTemp / 16);

	iTemp = kPromotion.getExperiencePercent();
	if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
		(AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
		(AI_getUnitAIType() == UNITAI_PIRATE_SEA) ||
		(AI_getUnitAIType() == UNITAI_RESERVE_SEA) ||
		(AI_getUnitAIType() == UNITAI_ESCORT_SEA) ||
		(AI_getUnitAIType() == UNITAI_CARRIER_SEA) ||
		(AI_getUnitAIType() == UNITAI_MISSILE_CARRIER_SEA))
	{
		iValue += (iTemp * 1);
	}
	else
	{
		iValue += (iTemp / 2);
	}

	iTemp = kPromotion.getKamikazePercent();
	if (AI_getUnitAIType() == UNITAI_ATTACK_CITY)
	{
		iValue += (iTemp / 16);
	}
	else
	{
		iValue += (iTemp / 64);
	}

	for (iI = 0; iI < GC.getNumTerrainInfos(); iI++)
	{
		iTemp = kPromotion.getTerrainAttackPercent(iI);
		if (iTemp != 0)
		{
			iExtra = getExtraTerrainAttackPercent((TerrainTypes)iI);
			iTemp *= (100 + iExtra * 2);
			iTemp /= 100;
			if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
				(AI_getUnitAIType() == UNITAI_COUNTER))
			{
				iValue += (iTemp / 4);
			}
			else
			{
				iValue += (iTemp / 16);
			}
		}

		iTemp = kPromotion.getTerrainDefensePercent(iI);
		if (iTemp != 0)
		{
			iExtra =  getExtraTerrainDefensePercent((TerrainTypes)iI);
			iTemp *= (100 + iExtra);
			iTemp /= 100;
			if (AI_getUnitAIType() == UNITAI_COUNTER)
			{
				if (plot()->getTerrainType() == (TerrainTypes)iI)
				{
					iValue += (iTemp / 4);
				}
				else
				{
					iValue++;
				}
			}
			else
			{
				iValue += (iTemp / 16);
			}
		}

		if (kPromotion.getTerrainDoubleMove(iI))
		{
			if (AI_getUnitAIType() == UNITAI_EXPLORE)
			{
				iValue += 20;
			}
			else if ((AI_getUnitAIType() == UNITAI_ATTACK) || (AI_getUnitAIType() == UNITAI_PILLAGE))
			{
				iValue += 10;
			}
			else
			{
				iValue += 1;
			}
		}
	}

	for (iI = 0; iI < GC.getNumFeatureInfos(); iI++)
	{
		iTemp = kPromotion.getFeatureAttackPercent(iI);
		if (iTemp != 0)
		{
			iExtra = getExtraFeatureAttackPercent((FeatureTypes)iI);
			iTemp *= (100 + iExtra * 2);
			iTemp /= 100;
			if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
				(AI_getUnitAIType() == UNITAI_COUNTER))
			{
				iValue += (iTemp / 4);
			}
			else
			{
				iValue += (iTemp / 16);
			}
		}

		iTemp = kPromotion.getFeatureDefensePercent(iI);;
		if (iTemp != 0)
		{
			iExtra = getExtraFeatureDefensePercent((FeatureTypes)iI);
			iTemp *= (100 + iExtra * 2);
			iTemp /= 100;

			if (!noDefensiveBonus())
			{
				if (AI_getUnitAIType() == UNITAI_COUNTER)
				{
					if (plot()->getFeatureType() == (FeatureTypes)iI)
					{
						iValue += (iTemp / 4);
					}
					else
					{
						iValue++;
					}
				}
				else
				{
					iValue += (iTemp / 16);
				}
			}
		}

		if (kPromotion.getFeatureDoubleMove(iI))
		{
			if (AI_getUnitAIType() == UNITAI_EXPLORE)
			{
				iValue += 20;
			}
			else if ((AI_getUnitAIType() == UNITAI_ATTACK) || (AI_getUnitAIType() == UNITAI_PILLAGE))
			{
				iValue += 10;
			}
			else
			{
				iValue += 1;
			}
		}
	}
	for (iI = 0; iI < GC.getNumPlotEffectInfos(); iI++)
	{
		iTemp = kPromotion.getPlotEffectAttackPercent(iI);
		if (iTemp != 0)
		{
			iExtra = getExtraPlotEffectAttackPercent((PlotEffectTypes)iI);
			iTemp *= (100 + iExtra * 2);
			iTemp /= 100;
			if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
				(AI_getUnitAIType() == UNITAI_COUNTER))
			{
				iValue += (iTemp / 4);
			}
			else
			{
				iValue += (iTemp / 16);
			}
		}

		iTemp = kPromotion.getPlotEffectDefensePercent(iI);;
		if (iTemp != 0)
		{
			iExtra = getExtraPlotEffectDefensePercent((PlotEffectTypes)iI);
			iTemp *= (100 + iExtra * 2);
			iTemp /= 100;

			if (!noDefensiveBonus())
			{
				if (AI_getUnitAIType() == UNITAI_COUNTER)
				{
					if (plot()->getPlotEffectType() == (PlotEffectTypes)iI)
					{
						iValue += (iTemp / 4);
					}
					else
					{
						iValue++;
					}
				}
				else
				{
					iValue += (iTemp / 16);
				}
			}
		}

		if (kPromotion.getPlotEffectDoubleMove(iI))
		{
			if (AI_getUnitAIType() == UNITAI_EXPLORE)
			{
				iValue += 20;
			}
			else if ((AI_getUnitAIType() == UNITAI_ATTACK) || (AI_getUnitAIType() == UNITAI_PILLAGE))
			{
				iValue += 10;
			}
			else
			{
				iValue += 1;
			}
		}
	}

	int iOtherCombat = 0;
	int iSameCombat = 0;

	for (iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
	{
		if ((UnitCombatTypes)iI == getUnitCombatType())
		{
			iSameCombat += unitCombatModifier((UnitCombatTypes)iI);
		}
		else
		{
			iOtherCombat += unitCombatModifier((UnitCombatTypes)iI);
		}
	}

	for (iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
	{
		iTemp = kPromotion.getUnitCombatModifierPercent(iI);
		int iCombatWeight = 0;
		//Fighting their own kind
		if ((UnitCombatTypes)iI == getUnitCombatType())
		{
			if (iSameCombat >= iOtherCombat)
			{
				iCombatWeight = 70;//"axeman takes formation"
			}
			else
			{
				iCombatWeight = 30;
			}
		}
		else
		{
			//fighting other kinds
			if (unitCombatModifier((UnitCombatTypes)iI) > 10)
			{
				iCombatWeight = 70;//"spearman takes formation"
			}
			else
			{
				iCombatWeight = 30;
			}
		}

		iCombatWeight *= GET_PLAYER(getOwnerINLINE()).AI_getUnitCombatWeight((UnitCombatTypes)iI);
		iCombatWeight /= 100;

		if ((AI_getUnitAIType() == UNITAI_COUNTER) || (AI_getUnitAIType() == UNITAI_CITY_COUNTER))
		{
			iValue += (iTemp * iCombatWeight) / 50;
		}
		else if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
				   (AI_getUnitAIType() == UNITAI_RESERVE))
		{
			iValue += (iTemp * iCombatWeight) / 100;
		}
		else
		{
			iValue += (iTemp * iCombatWeight) / 200;
		}
	}

	for (iI = 0; iI < NUM_DOMAIN_TYPES; iI++)
	{
		//WTF? why float and cast to int?
		//iTemp = ((int)((GC.getPromotionInfo(ePromotion).getDomainModifierPercent(iI) + getExtraDomainModifier((DomainTypes)iI)) * 100.0f));
		iTemp = kPromotion.getDomainModifierPercent(iI);
		if (AI_getUnitAIType() == UNITAI_COUNTER)
		{
			iValue += (iTemp * 1);
		}
		else if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
				   (AI_getUnitAIType() == UNITAI_RESERVE))
		{
			iValue += (iTemp / 2);
		}
		else
		{
			iValue += (iTemp / 8);
		}
	}

//FfH: Added by Kael 07/30/2007
	iTemp = kPromotion.getDefensiveStrikeChance() + kPromotion.getDefensiveStrikeDamage();
	iTemp /= 2;
	if ((AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
		(AI_getUnitAIType() == UNITAI_CITY_COUNTER) ||
		(AI_getUnitAIType() == UNITAI_COUNTER))
	{
		iTemp *= 2;
	}
	iValue += iTemp;
	iValue += kPromotion.getAIWeight();
//FfH: End Add
/*************************************************************************************************/
/**	New Tag Defs	(PromotionInfos)		05/15/08								Xienwolf	**/
/**	New Tag Defs	(CityBonuses)			05/15/08											**/
/**									Sets AI Value Levels										**/
/*************************************************************************************************/
	int iJ;
	bool bValid;

	iTemp = kPromotion.getCommandLimit();
	if(iTemp > 0)
	{
		if ((getCommandLimit() > 0) ||
			(getLevel() > 4))
		{
			iValue += iTemp*getLevel()*5;
		}
	}

	iTemp = kPromotion.getCommandXPShareRate();
	if(iTemp > 0)
	{
		iValue += iTemp*getNumMinions()*3;
	}
	iTemp = kPromotion.getCommandRange();
	if(iTemp > 0)
	{
		int iCommLim = kPromotion.getCommandLimit() + getCommandLimit();
		if (iCommLim > 0)
		{
			iValue += iTemp*iCommLim*getLevel()*5;
		}
	}

	iTemp = kPromotion.getNumMinionPromotions();
	if(iTemp > 0)
	{
		int iNumMin = getNumMinions();
		for(iI = 0; iI < iNumMin; ++iI)
		{
			for(iJ = 0;iJ < iTemp;++iJ)
			{
				PromotionTypes eLoopPromotion = kPromotion.getMinionPromotion(iJ);
				iValue += getMinionUnit(iI)->AI_promotionValue(eLoopPromotion, true);
			}
		}
	}

	iTemp = kPromotion.getNumCommanderPromotions();
	if(iTemp > 0)
	{
		if (getCommanderUnit() != NULL)
		{
			for(iI = 0;iI < iTemp;++iI)
			{
				PromotionTypes eLoopPromotion = kPromotion.getCommanderPromotion(iI);
				iValue += getCommanderUnit()->AI_promotionValue(eLoopPromotion, true);
			}
		}
	}

	iValue += (isCommunalProperty() ? 0 : (kPromotion.isCommunalProperty() ? -25 : 0));
	iValue += (isBlind() ? 0 : (kPromotion.isBlind() ? -70 : 0));

	if(kPromotion.isNeverHostile() && !isNeverHostile())
	{
		if ((AI_getUnitAIType() == UNITAI_WORKER) ||
			(AI_getUnitAIType() == UNITAI_EXPLORE) ||
			(AI_getUnitAIType() == UNITAI_MISSIONARY) ||
			(AI_getUnitAIType() == UNITAI_PROPHET) ||
			(AI_getUnitAIType() == UNITAI_ARTIST) ||
			(AI_getUnitAIType() == UNITAI_SCIENTIST) ||
			(AI_getUnitAIType() == UNITAI_MERCHANT) ||
			(AI_getUnitAIType() == UNITAI_ENGINEER) ||
			(AI_getUnitAIType() == UNITAI_SPY) ||
			(AI_getUnitAIType() == UNITAI_WORKER_SEA) ||
			(AI_getUnitAIType() == UNITAI_EXPLORE_SEA) ||
			(AI_getUnitAIType() == UNITAI_MISSIONARY_SEA) ||
			(AI_getUnitAIType() == UNITAI_SETTLER_SEA) ||
			(AI_getUnitAIType() == UNITAI_SETTLE))
		{
			iValue += 100;
		}
		else
		{
			iValue -= 75;
		}
	}

	if (airCombatLimit() < 100)
	{
		iValue += (std::min(100, kPromotion.getAirCombatLimitBoost() + airCombatLimit()) - airCombatLimit()) * 5;
	}

	if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
		(AI_getUnitAIType() == UNITAI_ATTACK_CITY) ||
		(AI_getUnitAIType() == UNITAI_ATTACK_CITY_LEMMING))
	{
		iValue += (kPromotion.getExtraDropRange() * 8);
	}

	if (airRange() > 0)
	{
		iValue += (kPromotion.getAirCombat() * 4);
	}

	iValue += (isFreeUnit() ? 0 : (kPromotion.isFreeUnit() ? 5 : 0));

	iValue += ((isNoSupply() || isFreeUnit()) ? 0 : (kPromotion.isNoSupply() ? 3 : 0));

	iValue += (kPromotion.getExtraCombatDefense() * 10);

	iValue += (kPromotion.getExtraCombatStr() * 10);

	iValue -= (kPromotion.getExpireChance() / 10);

/*************************************************************************************************/
/**	Tweak							16/05/10								Snarko				**/
/**																								**/
/**				Makes no sense to remove 10 from iValue if the duration is zero.				**/
/**					Or add to the value if the duration is more than 20.						**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	iValue -= 10 - (kPromotion.getDuration() / 2);
/**								----  End Original Code  ----									**/
	if (kPromotion.getDuration() > 0)
	{
		iValue -= std::max(0, 10 - (kPromotion.getDuration() / 2));
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	iValue -= (kPromotion.getCombatPercentDefense() / 5);

	if (kPromotion.isInvisible())
	{
		if ((AI_getUnitAIType() == UNITAI_RESERVE) ||
			(AI_getUnitAIType() == UNITAI_COUNTER) ||
			(AI_getUnitAIType() == UNITAI_CITY_COUNTER) ||
			(AI_getUnitAIType() == UNITAI_RESERVE_SEA) ||
			(AI_getUnitAIType() == UNITAI_EXPLORE_SEA) ||
			(AI_getUnitAIType() == UNITAI_EXPLORE))
		{
			iValue += 15;
		}
	}

	if (kPromotion.getNumInvisibleTypes() > 0)
	{
		for (iI = 0; iI < kPromotion.getNumInvisibleTypes(); iI++)
		{
			bValid = true;
			for (iJ = 0; iJ < getNumInvisibleTypes(); iJ++)
				if (getInvisibleType(iJ) == kPromotion.getInvisibleType(iI))
					bValid = false;
			if (bValid)
			{
				if ((AI_getUnitAIType() == UNITAI_RESERVE) ||
					(AI_getUnitAIType() == UNITAI_COUNTER) ||
					(AI_getUnitAIType() == UNITAI_CITY_COUNTER) ||
					(AI_getUnitAIType() == UNITAI_RESERVE_SEA) ||
					(AI_getUnitAIType() == UNITAI_EXPLORE_SEA) ||
					(AI_getUnitAIType() == UNITAI_EXPLORE))
				{
					iValue += 5;
				}
				else
					iValue += 2;
			}
		}
	}

//	if (kPromotion.getNumSeeInvisibleTypes() > 0)
//	{
//		for (iI = 0; iI < kPromotion.getNumSeeInvisibleTypes(); iI++)
//		{
//			bValid = true;
//			for (iJ = 0; iJ < getNumSeeInvisibleTypes(); iJ++)
//				if (getSeeInvisibleType(iJ) == kPromotion.getSeeInvisibleType(iI))
//					bValid = false;
//			if (bValid)
//			{
//				if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
//					(AI_getUnitAIType() == UNITAI_COUNTER) ||
//					(AI_getUnitAIType() == UNITAI_ANIMAL) ||
//					(AI_getUnitAIType() == UNITAI_PILLAGE) ||
//					(AI_getUnitAIType() == UNITAI_RESERVE_SEA) ||
//					(AI_getUnitAIType() == UNITAI_EXPLORE_SEA) ||
//					(AI_getUnitAIType() == UNITAI_EXPLORE))
//				{
//					iValue += 5;
//				}
//				else
//					iValue += 2;
//			}
//		}
//	}
	if (kPromotion.getInvisibleLevel() > 0 && kPromotion.getInvisibleLevel()> getInvisibleLevel())
	{
		if ((AI_getUnitAIType() == UNITAI_RESERVE) ||
			(AI_getUnitAIType() == UNITAI_COUNTER) ||
			(AI_getUnitAIType() == UNITAI_CITY_COUNTER) ||
			(AI_getUnitAIType() == UNITAI_RESERVE_SEA) ||
			(AI_getUnitAIType() == UNITAI_EXPLORE_SEA) ||
			(AI_getUnitAIType() == UNITAI_EXPLORE))
		{
			iValue += 5*(kPromotion.getInvisibleLevel()-getInvisibleLevel());
		}
		else
			iValue += 2 * (kPromotion.getInvisibleLevel() - getInvisibleLevel());
	}
	if (kPromotion.getExtraPerception() != 0)
	{
		if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
			(AI_getUnitAIType() == UNITAI_COUNTER) ||
			(AI_getUnitAIType() == UNITAI_ANIMAL) ||
			(AI_getUnitAIType() == UNITAI_PILLAGE) ||
			(AI_getUnitAIType() == UNITAI_RESERVE_SEA) ||
			(AI_getUnitAIType() == UNITAI_EXPLORE_SEA) ||
			(AI_getUnitAIType() == UNITAI_EXPLORE))
		{
			iValue += 5* kPromotion.getExtraPerception();
		}
		else
		{
			iValue += 2* kPromotion.getExtraPerception();
		}

	}
	if (kPromotion.isRemovedByCasting())
		iValue -= 5;

	if (kPromotion.isRemovedByCombat())
		iValue -= 5;

	if (kPromotion.isImmuneToCapture())
		iValue += 15;

/*************************************************************************************************/
/**	Tweak							16/05/10								Snarko				**/
/**																								**/
/**						We're not interested in zero or LESS here...							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	iValue += std::min(0, 100 - getExperience()) * kPromotion.getFreeXPPerTurn();
/**								----  End Original Code  ----									**/
	iValue += std::max(0, 100 - getExperience()) * kPromotion.getFreeXPPerTurn();
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/


	iValue += kPromotion.getFreeXPFromCombat() / 10;

	iExtra = 0;
	if (m_pUnitInfo->isFreeXP() || isGetCasterXP())
/*************************************************************************************************/
/**	Tweak							18/05/10								Snarko				**/
/**																								**/
/**					Never reduce the value of the promotion here								**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		iValue += (getSpellCasterXP() + kPromotion.getSpellCasterXP() - (getLevel() * getLevel())) * 8;
/**								----  End Original Code  ----									**/
		iValue += std::max(0, (getSpellCasterXP() + kPromotion.getSpellCasterXP() - (getLevel() * getLevel())) * 8);
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	iTemp = kPromotion.getWorkRateModify();
	iExtra = 0;
	for (iI = 0; iI < GC.getNumBuildInfos(); iI++)
		if (m_pUnitInfo->getBuilds(iI))
			iExtra++;
	if (iExtra > 0)
		iValue += iTemp + iExtra / 5;

/**             Fields for Later Inclusion in AI Weighting
	bool m_bBoarding;
	bool m_bOnlyDefensive;
	bool m_bDoubleFortifyBonus;
	bool m_bFear;
	bool m_bFlying;
	bool m_bHiddenNationality;
	bool m_bIgnoreBuildingDefense;
	bool m_bImmortal;
	bool m_bImmuneToCapture;
	bool m_bImmuneToFear;
	bool m_bImmuneToMagic;
	bool m_bInvisible;
	bool m_bNotAlive;
	bool m_bRemovedWhenHealed;
	bool m_bTargetWeakestUnit;
	bool m_bTargetWeakestUnitCounter;
	bool m_bTwincast;
	bool m_bWaterWalking;
	int m_iBetrayalChance;
	int m_iBetterDefenderThanPercent;
	int m_iCasterResistModify;
	int m_iCombatCapturePercent;
	int m_iCombatHealPercent;
	int m_iCombatLimit;
	int m_iCombatPercentGlobalCounter;
	int m_iCombatPercentInBorders;
	int m_iResistMagic;
	int m_iSpellDamageModify;
	int m_iCaptureUnitCombat;
	int m_iPromotionCombatApply;
	int m_iPromotionRandomApply;
	int m_iPromotionSummonPerk;
	int m_iPromotionCombatType;
	int m_iPromotionCombatMod;
	int* m_piBonusAffinity;
	int* m_piDamageTypeCombat;
	int* m_piDamageTypeResist;
**/
	if (kPromotion.isNoXP())
	{
		iValue += 10;
	}

	if (kPromotion.isTerritorial())
	{
		if ((AI_getUnitAIType() == UNITAI_RESERVE) ||
			(AI_getUnitAIType() == UNITAI_COUNTER) ||
			(AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
			(AI_getUnitAIType() == UNITAI_CITY_COUNTER) ||
			(AI_getUnitAIType() == UNITAI_CITY_SPECIAL) ||
			(AI_getUnitAIType() == UNITAI_RESERVE_SEA) ||
			(AI_getUnitAIType() == UNITAI_ESCORT_SEA) ||
			(AI_getUnitAIType() == UNITAI_EXPLORE_SEA) ||
			(AI_getUnitAIType() == UNITAI_EXPLORE))
		{
			iValue += 2;
		}
	}

	if (kPromotion.isRivalTerritoryExplore())
	{
		if (!m_pUnitInfo->isRivalTerritory())
		{
			if ((AI_getUnitAIType() == UNITAI_SETTLE) ||
				(AI_getUnitAIType() == UNITAI_EXPLORE) ||
				(AI_getUnitAIType() == UNITAI_MISSIONARY) ||
				(AI_getUnitAIType() == UNITAI_MERCHANT) ||
				(AI_getUnitAIType() == UNITAI_SPY) ||
				(AI_getUnitAIType() == UNITAI_ESCORT_SEA) ||
				(AI_getUnitAIType() == UNITAI_EXPLORE_SEA) ||
				(AI_getUnitAIType() == UNITAI_SETTLER_SEA) ||
				(AI_getUnitAIType() == UNITAI_MISSIONARY_SEA))
			{
				iValue += 4;
			}
			else
			{
				iValue++;
			}
		}
	}

	if (kPromotion.isRivalTerritoryBlock())
		iValue -= 4;

	if (kPromotion.isPillageOnMove())
	{
		if (!m_pUnitInfo->isAutoRaze())
		{
			if ((AI_getUnitAIType() == UNITAI_ANIMAL) ||
				(AI_getUnitAIType() == UNITAI_ATTACK) ||
				(AI_getUnitAIType() == UNITAI_PILLAGE) ||
				(AI_getUnitAIType() == UNITAI_PIRATE_SEA) ||
				(AI_getUnitAIType() == UNITAI_ATTACK_SEA))
			{
				iValue += 4;
			}
			else if ((AI_getUnitAIType() == UNITAI_COUNTER) ||
					 (AI_getUnitAIType() == UNITAI_COLLATERAL) ||
					 (AI_getUnitAIType() == UNITAI_GENERAL) ||
					 (AI_getUnitAIType() == UNITAI_SPY) ||
					 (AI_getUnitAIType() == UNITAI_ASSAULT_SEA) ||
					 (AI_getUnitAIType() == UNITAI_SPY_SEA) ||
					 (AI_getUnitAIType() == UNITAI_ATTACK_CITY) ||
					 (AI_getUnitAIType() == UNITAI_ATTACK_CITY_LEMMING))
			{
				iValue += 2;
			}
			else
			{
				iValue++;
			}
		}
	}

	if (kPromotion.isSelfPillage())
	{
		if ((AI_getUnitAIType() == UNITAI_WORKER) ||
			(AI_getUnitAIType() == UNITAI_RESERVE) ||
			(AI_getUnitAIType() == UNITAI_EXPLORE) ||
			(AI_getUnitAIType() == UNITAI_MISSIONARY) ||
			(AI_getUnitAIType() == UNITAI_WORKER_SEA) ||
			(AI_getUnitAIType() == UNITAI_RESERVE_SEA) ||
			(AI_getUnitAIType() == UNITAI_ESCORT_SEA) ||
			(AI_getUnitAIType() == UNITAI_EXPLORE_SEA) ||
			(AI_getUnitAIType() == UNITAI_MISSIONARY_SEA))
		{
			iValue -= 20;
		}
		else
		{
			iValue -= 10;
		}
	}

	if (kPromotion.isGetCasterXP())
	{
		if (!m_pUnitInfo->isFreeXP())
		{
			iValue += 10;
		}
	}

	if (kPromotion.isNonWarWeariness())
		if (!m_pUnitInfo->isNoWarWeariness())
			iValue += 2;

	if (kPromotion.isNoMapReveal())
	{
		if (!m_pUnitInfo->isNoRevealMap())
		{
			if ((AI_getUnitAIType() == UNITAI_SETTLE) ||
				(AI_getUnitAIType() == UNITAI_EXPLORE) ||
				(AI_getUnitAIType() == UNITAI_EXPLORE_SEA) ||
				(AI_getUnitAIType() == UNITAI_SETTLER_SEA))
			{
				iValue -= 16;
			}
			else
			{
				iValue -= 4;
			}
		}
	}

	if (kPromotion.isCannotCapture())
	{
		if (!m_pUnitInfo->isCannotCapture())
		{
			if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
				(AI_getUnitAIType() == UNITAI_ATTACK_CITY) ||
				(AI_getUnitAIType() == UNITAI_ATTACK_CITY_LEMMING))
			{
				iValue -= 4;
			}
			else
			{
				iValue--;
			}
		}
	}

	if (kPromotion.isCityHappy())
	{
		if (!isMilitaryHappiness())
		{
			if ((AI_getUnitAIType() == UNITAI_WORKER) ||
				(AI_getUnitAIType() == UNITAI_RESERVE) ||
				(AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
				(AI_getUnitAIType() == UNITAI_CITY_COUNTER) ||
				(AI_getUnitAIType() == UNITAI_CITY_SPECIAL))
			{
				iValue += 4;
			}
			else
			{
				iValue++;
			}
		}
	}

	if (kPromotion.isCityNoHappy())
	{
		if (isMilitaryHappiness())
		{
			if ((AI_getUnitAIType() == UNITAI_WORKER) ||
				(AI_getUnitAIType() == UNITAI_RESERVE) ||
				(AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
				(AI_getUnitAIType() == UNITAI_CITY_COUNTER) ||
				(AI_getUnitAIType() == UNITAI_CITY_SPECIAL))
			{
				iValue -= 4;
			}
			else
			{
				iValue--;
			}
		}
	}

	if (kPromotion.isNoSupport())
	{
		if (m_pUnitInfo->isMilitarySupport())
		{
			iValue += 2;
		}
	}

	if (kPromotion.isCanPillage())
	{
		if (!m_pUnitInfo->isPillage())
		{
			if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
				(AI_getUnitAIType() == UNITAI_COLLATERAL) ||
				(AI_getUnitAIType() == UNITAI_PILLAGE) ||
				(AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
				(AI_getUnitAIType() == UNITAI_ASSAULT_SEA) ||
				(AI_getUnitAIType() == UNITAI_PIRATE_SEA))
			{
				iValue += 4;
			}
			else if ((AI_getUnitAIType() == UNITAI_EXPLORE) ||
					 (AI_getUnitAIType() == UNITAI_EXPLORE_SEA))
			{
				iValue += 2;
			}
			else
			{
				iValue++;
			}
		}
	}

	if (kPromotion.isCannotPillage())
	{
		if (m_pUnitInfo->isPillage())
		{
			if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
				(AI_getUnitAIType() == UNITAI_COLLATERAL) ||
				(AI_getUnitAIType() == UNITAI_PILLAGE) ||
				(AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
				(AI_getUnitAIType() == UNITAI_ASSAULT_SEA) ||
				(AI_getUnitAIType() == UNITAI_PIRATE_SEA))
			{
				iValue -= 4;
			}
			else if ((AI_getUnitAIType() == UNITAI_EXPLORE) ||
					 (AI_getUnitAIType() == UNITAI_EXPLORE_SEA))
			{
				iValue -= 2;
			}
			else
			{
				iValue--;
			}
		}
	}

	if (kPromotion.isCitySpy())
	{
		if (!m_pUnitInfo->isInvestigate())
		{
			if ((AI_getUnitAIType() == UNITAI_EXPLORE) ||
				(AI_getUnitAIType() == UNITAI_MISSIONARY) ||
				(AI_getUnitAIType() == UNITAI_SPY) ||
				(AI_getUnitAIType() == UNITAI_EXPLORE_SEA) ||
				(AI_getUnitAIType() == UNITAI_MISSIONARY_SEA) ||
				(AI_getUnitAIType() == UNITAI_SPY_SEA))
			{
				iValue += 4;
			}
			else
			{
				iValue++;
			}
		}
	}

	if (kPromotion.isStartGoldenAge())
	{
		if (!m_pUnitInfo->isGoldenAge())
		{
			iValue += 2;
		}
	}

	if (kPromotion.isNoDefenseBonus())
	{
		if (!m_pUnitInfo->isNoDefensiveBonus())
		{
			if ((AI_getUnitAIType() == UNITAI_WORKER) ||
				(AI_getUnitAIType() == UNITAI_RESERVE) ||
				(AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
				(AI_getUnitAIType() == UNITAI_CITY_SPECIAL) ||
				(AI_getUnitAIType() == UNITAI_WORKER_SEA) ||
				(AI_getUnitAIType() == UNITAI_RESERVE_SEA) ||
				(AI_getUnitAIType() == UNITAI_ESCORT_SEA) ||
				(AI_getUnitAIType() == UNITAI_CARRIER_SEA))
			{
				iValue -= 4;
			}
			else
			{
				iValue--;
			}
		}
	}

	if (kPromotion.isAllowDefenseBonuses())
	{
		if (m_pUnitInfo->isNoDefensiveBonus())
		{
			if ((AI_getUnitAIType() == UNITAI_WORKER) ||
				(AI_getUnitAIType() == UNITAI_RESERVE) ||
				(AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
				(AI_getUnitAIType() == UNITAI_CITY_SPECIAL) ||
				(AI_getUnitAIType() == UNITAI_WORKER_SEA) ||
				(AI_getUnitAIType() == UNITAI_RESERVE_SEA) ||
				(AI_getUnitAIType() == UNITAI_ESCORT_SEA) ||
				(AI_getUnitAIType() == UNITAI_CARRIER_SEA))
			{
				iValue += 4;
			}
			else
			{
				iValue++;
			}
		}
	}

	if (kPromotion.isMoveImpassable())
	{
		if (!m_pUnitInfo->isCanMoveImpassable())
		{
			if ((AI_getUnitAIType() == UNITAI_WORKER) ||
				(AI_getUnitAIType() == UNITAI_EXPLORE) ||
				(AI_getUnitAIType() == UNITAI_MISSIONARY) ||
				(AI_getUnitAIType() == UNITAI_SPY) ||
				(AI_getUnitAIType() == UNITAI_WORKER_SEA) ||
				(AI_getUnitAIType() == UNITAI_EXPLORE_SEA) ||
				(AI_getUnitAIType() == UNITAI_MISSIONARY_SEA))
			{
				iValue += 8;
			}
			else
			{
				iValue++;
			}
		}
	}

	if (kPromotion.isFlatMoveCost())
		if (!m_pUnitInfo->isFlatMovementCost() && !m_pUnitInfo->isIgnoreTerrainCost())
			iValue++;

	if (kPromotion.isIgnoreTerrainCosts())
	{
		if (!m_pUnitInfo->isIgnoreTerrainCost())
		{
			if ((AI_getUnitAIType() == UNITAI_EXPLORE))
			{
				iValue += 4;
			}
			else
			{
				iValue++;
			}
		}
	}

	if (kPromotion.isAttackNoWar())
	{
		if (!m_pUnitInfo->isAlwaysHostile())
		{
			if ((AI_getUnitAIType() == UNITAI_SETTLE) ||
				(AI_getUnitAIType() == UNITAI_WORKER) ||
				(AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
				(AI_getUnitAIType() == UNITAI_CITY_COUNTER) ||
				(AI_getUnitAIType() == UNITAI_CITY_SPECIAL) ||
				(AI_getUnitAIType() == UNITAI_EXPLORE) ||
				(AI_getUnitAIType() == UNITAI_SPY) ||
				(AI_getUnitAIType() == UNITAI_WORKER_SEA) ||
				(AI_getUnitAIType() == UNITAI_ESCORT_SEA) ||
				(AI_getUnitAIType() == UNITAI_EXPLORE_SEA) ||
				(AI_getUnitAIType() == UNITAI_SETTLER_SEA) ||
				(AI_getUnitAIType() == UNITAI_MISSIONARY_SEA) ||
				(AI_getUnitAIType() == UNITAI_SPY_SEA) ||
				(AI_getUnitAIType() == UNITAI_CARRIER_SEA) ||
				(AI_getUnitAIType() == UNITAI_MISSIONARY))
			{
				iValue -= 10;
			}
			else if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
					 (AI_getUnitAIType() == UNITAI_ATTACK_CITY) ||
					 (AI_getUnitAIType() == UNITAI_COLLATERAL) ||
					 (AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
					 (AI_getUnitAIType() == UNITAI_ASSAULT_SEA) ||
					 (AI_getUnitAIType() == UNITAI_PIRATE_SEA) ||
					 (AI_getUnitAIType() == UNITAI_ATTACK_CITY_LEMMING))
			{
				iValue += 5;
			}
		}
	}

	if (kPromotion.isAllowAttacks())
		if (m_pUnitInfo->isOnlyDefensive())
			iValue += 4;

	if (kPromotion.isFirstStrikeVulnerable())
	{
		if (m_pUnitInfo->isFirstStrikeImmune())
			iValue -= 8;
		else
			iValue--;
	}

	if (kPromotion.isNonAbandon())
		if (m_pUnitInfo->isAbandon())
			iValue += 18;

	if (kPromotion.isIndependant())
		if (isTeamUnitClass((UnitClassTypes)m_pUnitInfo->getUnitClassType()) || isNationalUnitClass((UnitClassTypes)m_pUnitInfo->getUnitClassType()))
			iValue += 20;


	iTemp = kPromotion.getSlaveGenerationChance();
	if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
		(AI_getUnitAIType() == UNITAI_ATTACK_CITY) ||
		(AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
		(AI_getUnitAIType() == UNITAI_PIRATE_SEA) ||
		(AI_getUnitAIType() == UNITAI_ATTACK_CITY_LEMMING))
	{
		iValue += (iTemp * 4);
	}
	else
	{
		iValue += (iTemp);
	}

	iTemp = kPromotion.getCombatExtraDuration();
	if (getDuration() > 0)
		iValue += (iTemp * 10);

	iTemp = kPromotion.getNoBadExplore();
	if ((AI_getUnitAIType() == UNITAI_EXPLORE_SEA) ||
		(AI_getUnitAIType() == UNITAI_EXPLORE))
	{
		iValue += iTemp * (100-getNoBadExplore())/10;
	}


	iTemp = kPromotion.getDurationPerTurn();
	if (getDuration() > 0)
		iValue += (iTemp * 20);

	iTemp = kPromotion.getDurationAlter();
	if (getDuration() > 0)
		iValue += (iTemp * 5);

	iTemp = kPromotion.getChangeDuration();
	if (getDuration() > 0)
		iValue += (iTemp * 5);
	else
		if (iTemp != 0)
			iValue -= (50 - iTemp * 3);
	if (!bSkipNegative)
	{
		iValue -= (kPromotion.getExtraSupport() * 2);

		iValue -= (kPromotion.getGoldCost() * 20);
		if (GET_PLAYER(getOwner()).AI_isFinancialTrouble())
			iValue -= (kPromotion.getGoldCost() * 50);
		iValue -= (kPromotion.getChanceMiscast());
	}
	iTemp = (kPromotion.getCombatDmgCapBoost());
	if (iTemp > 0)
/*************************************************************************************************/
/**	Higher hitpoints				28/01/11				Imported from wiser orcs by Snarko	**/
/**						Makes higher values than 100 HP possible.								**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		if (combatLimit() < 100 && ((AI_getUnitAIType() == UNITAI_ATTACK) ||
									(AI_getUnitAIType() == UNITAI_ATTACK_CITY) ||
									(AI_getUnitAIType() == UNITAI_COLLATERAL) ||
									(AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
									(AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
									(AI_getUnitAIType() == UNITAI_ASSAULT_SEA) ||
									(AI_getUnitAIType() == UNITAI_PIRATE_SEA) ||
									(AI_getUnitAIType() == UNITAI_ATTACK_CITY_LEMMING)))
			if (combatLimit() + iTemp > 99)
				iValue += 50;
			else
				iValue += iTemp * 3;
	else if (iTemp < 0)
		if (combatLimit() > 0 && ((AI_getUnitAIType() == UNITAI_ATTACK) ||
								(AI_getUnitAIType() == UNITAI_ATTACK_CITY) ||
								(AI_getUnitAIType() == UNITAI_COLLATERAL) ||
								(AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
								(AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
								(AI_getUnitAIType() == UNITAI_ASSAULT_SEA) ||
								(AI_getUnitAIType() == UNITAI_PIRATE_SEA) ||
								(AI_getUnitAIType() == UNITAI_ATTACK_CITY_LEMMING)))
			iValue -= iTemp * 5;
/**								----  End Original Code  ----									**/
		if (combatLimit() < GC.getMAX_HIT_POINTS() && ((AI_getUnitAIType() == UNITAI_ATTACK) ||
									(AI_getUnitAIType() == UNITAI_ATTACK_CITY) ||
									(AI_getUnitAIType() == UNITAI_COLLATERAL) ||
									(AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
									(AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
									(AI_getUnitAIType() == UNITAI_ASSAULT_SEA) ||
									(AI_getUnitAIType() == UNITAI_PIRATE_SEA) ||
									(AI_getUnitAIType() == UNITAI_ATTACK_CITY_LEMMING)))
			if (combatLimit() + iTemp > (99 * GC.getDefineINT("HIT_POINT_FACTOR")))
				iValue += 50;
			else
				iValue += iTemp * 3 / GC.getDefineINT("HIT_POINT_FACTOR");
	else if (iTemp < 0)
		if (combatLimit() > 0 && ((AI_getUnitAIType() == UNITAI_ATTACK) ||
								(AI_getUnitAIType() == UNITAI_ATTACK_CITY) ||
								(AI_getUnitAIType() == UNITAI_COLLATERAL) ||
								(AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
								(AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
								(AI_getUnitAIType() == UNITAI_ASSAULT_SEA) ||
								(AI_getUnitAIType() == UNITAI_PIRATE_SEA) ||
								(AI_getUnitAIType() == UNITAI_ATTACK_CITY_LEMMING)))
			iValue -= iTemp * 5 / GC.getDefineINT("HIT_POINT_FACTOR");
/*************************************************************************************************/
/**	Higher hitpoints						END													**/
/*************************************************************************************************/

	for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		iTemp = kPromotion.getYieldFromWin(iI) - kPromotion.getYieldForLoss(iI);
		if ((AI_getUnitAIType() == UNITAI_ANIMAL) ||
			(AI_getUnitAIType() == UNITAI_ATTACK) ||
			(AI_getUnitAIType() == UNITAI_ATTACK_CITY) ||
			(AI_getUnitAIType() == UNITAI_COLLATERAL) ||
			(AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
			(AI_getUnitAIType() == UNITAI_EXPLORE) ||
			(AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
			(AI_getUnitAIType() == UNITAI_EXPLORE_SEA) ||
			(AI_getUnitAIType() == UNITAI_ASSAULT_SEA) ||
			(AI_getUnitAIType() == UNITAI_PIRATE_SEA) ||
			(AI_getUnitAIType() == UNITAI_ATTACK_CITY_LEMMING))
		{
			iValue += (iTemp * 2);
		}
		else
		{
			iValue += (iTemp);
		}
	}

	for (iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		iTemp = kPromotion.getCommerceFromWin(iI) - kPromotion.getCommerceForLoss(iI);
		if ((AI_getUnitAIType() == UNITAI_ANIMAL) ||
			(AI_getUnitAIType() == UNITAI_ATTACK) ||
			(AI_getUnitAIType() == UNITAI_ATTACK_CITY) ||
			(AI_getUnitAIType() == UNITAI_COLLATERAL) ||
			(AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
			(AI_getUnitAIType() == UNITAI_EXPLORE) ||
			(AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
			(AI_getUnitAIType() == UNITAI_EXPLORE_SEA) ||
			(AI_getUnitAIType() == UNITAI_ASSAULT_SEA) ||
			(AI_getUnitAIType() == UNITAI_PIRATE_SEA) ||
			(AI_getUnitAIType() == UNITAI_ATTACK_CITY_LEMMING))
		{
			iValue += (iTemp * 2);
		}
		else
		{
			iValue += (iTemp);
		}
	}

	if (kPromotion.getNumCityBonuses() > 0)
		{for (iI = 0; iI < kPromotion.getNumCityBonuses(); iI++)
			{CityBonuses cbTemp = kPromotion.getCityBonus(iI);
			if (cbTemp.bApplyEnemy || cbTemp.bApplyRival)
				{if ((AI_getUnitAIType() == UNITAI_ATTACK) ||
					 (AI_getUnitAIType() == UNITAI_ATTACK_CITY) ||
					 (AI_getUnitAIType() == UNITAI_COLLATERAL) ||
					 (AI_getUnitAIType() == UNITAI_ATTACK_SEA) ||
					 (AI_getUnitAIType() == UNITAI_ASSAULT_SEA) ||
					 (AI_getUnitAIType() == UNITAI_PIRATE_SEA) ||
					 (AI_getUnitAIType() == UNITAI_ATTACK_CITY_LEMMING))
					 {iValue -= int(cbTemp.fCulture
								+ cbTemp.fDefense / 2
								- cbTemp.fDiplo * 5
								+ cbTemp.fFood * 2
								+ cbTemp.fFreeXP * 8
								+ cbTemp.fGold * 5
								+ cbTemp.fGPP
								+ cbTemp.fHappy * 3
								+ cbTemp.fHealth
								+ cbTemp.fProduction * 2
								+ cbTemp.fTradeRoutes
								+ cbTemp.iBonusRange * 3
								+ cbTemp.fCrime
								+ cbTemp.fDecayRate);}
					else
						iValue += 5;}
			if (cbTemp.bApplySelf || cbTemp.bApplyTeam)
				{
				if (AI_getUnitAIType() == UNITAI_CITY_DEFENSE)
				{
					if (plot()->isCity())
					{
						if (plot()->getPlotCity()->getCrimePerTurn() > 0)
						{
							iValue -= (int)cbTemp.fCrime * 20;
						}
						else
						{
							iValue -= (int)cbTemp.fCrime * 15;
						}
					}
					else
					{
						iValue -= (int)cbTemp.fCrime * 10;
					}

				}
				if ((AI_getUnitAIType() == UNITAI_SETTLE) ||
					(AI_getUnitAIType() == UNITAI_WORKER) ||
					(AI_getUnitAIType() == UNITAI_CITY_DEFENSE) ||
					(AI_getUnitAIType() == UNITAI_CITY_COUNTER) ||
					(AI_getUnitAIType() == UNITAI_CITY_SPECIAL) ||
					(AI_getUnitAIType() == UNITAI_EXPLORE) ||
					(AI_getUnitAIType() == UNITAI_SPY) ||
					(AI_getUnitAIType() == UNITAI_WORKER_SEA) ||
					(AI_getUnitAIType() == UNITAI_ESCORT_SEA) ||
					(AI_getUnitAIType() == UNITAI_EXPLORE_SEA) ||
					(AI_getUnitAIType() == UNITAI_SETTLER_SEA) ||
					(AI_getUnitAIType() == UNITAI_MISSIONARY_SEA) ||
					(AI_getUnitAIType() == UNITAI_SPY_SEA) ||
					(AI_getUnitAIType() == UNITAI_CARRIER_SEA) ||
					(AI_getUnitAIType() == UNITAI_MISSIONARY))
					 {iValue += int(cbTemp.fCulture
								+ cbTemp.fDefense / 2
								- cbTemp.fDiplo * 5
								+ cbTemp.fFood * 2
								+ cbTemp.fFreeXP * 8
								+ cbTemp.fGold * 5
								+ cbTemp.fGPP
								+ cbTemp.fHappy * 3
								+ cbTemp.fHealth
								+ cbTemp.fProduction * 2
								+ cbTemp.fTradeRoutes
								+ cbTemp.iBonusRange * 3
								+ cbTemp.fDecayRate);}
					else
						iValue += 5;}}}

	if (kPromotion.getNumPromotionOverwrites() > 0)
		for (iI = 0; iI < kPromotion.getNumPromotionOverwrites(); iI++)
			if (isHasPromotion(kPromotion.getPromotionOverwrites(iI)))
				iValue -= AI_promotionValue(kPromotion.getPromotionOverwrites(iI));

	for (iI = 0; iI < GC.getNumPromotionInfos(); iI++)
	{
		CvPromotionInfo& kLoopPromotion = GC.getPromotionInfo((PromotionTypes)iI);
		if (isHasPromotion((PromotionTypes)iI))
		{
			if (kLoopPromotion.getNumPromotionReplacedBy() > 0)
				for (int iJ = 0; iJ < kPromotion.getNumPromotionReplacedBy(); iJ++)
					if (kLoopPromotion.getPromotionReplacedBy(iJ) == ePromotion)
						iValue -= AI_promotionValue((PromotionTypes)iI);
			//PromotionClass
			if (kPromotion.getPromotionClass() == kLoopPromotion.getPromotionClass() && kPromotion.getPromotionClass() != NO_PROMOTIONCLASS && ePromotion != (PromotionTypes)iI) {
				iValue -= (AI_promotionValue((PromotionTypes)iI,false,true)*3)/2;
			}
		}
	}
/*************************************************************************************************/
/**	New Tag Defs							END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	Civilization Flavors				09/07/10										Snarko	**/
/**																								**/
/**							Making civilization flavors,										**/
/**			for helping AI with things we can't really add in a non-hardcoded way				**/
/*************************************************************************************************/
	iValue += 10 * kPromotion.getModifyGlobalCounter() * GC.getCivilizationInfo(getCivilizationType()).getCivFlavorValue(CIVFLAVOR_GLOBALCOUNTER);
/*************************************************************************************************/
/**	Civilization Flavors					END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
	if (kPromotion.isInquisition())
	{
		//Do this if we're going for religious victory but are not at an equal or better state for culture victory.
		//Since AI is only supposed to do one level 4 victory we don't need a check there
		//XXX Maybe make the AI only pick this promotion for a limited number of units?
		//XXX What about multiple promotions giving the same thing?
		if (GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_RELIGION, 2) && !GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE, 2))
			iValue += 10;
		else if (GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_RELIGION, 3) && !GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE, 3))
			iValue += 15;
		else if (GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_RELIGION, 4))
			iValue += 25;
		else
			iValue += 1;
	}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/

/*************************************************************************************************/
/**	Xienwolf Tweak							03/27/09											**/
/**																								**/
/**						Chipotle Support for Decoding AI love of Promos							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	if (iValue > 0)
/**								----  End Original Code  ----									**/
	if (iValue > 0 && !bSkipRandom)
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
	{
		iValue += GC.getGameINLINE().getSorenRandNum(15, "AI Promote");
	}

	return iValue;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_shadow(UnitAITypes eUnitAI, int iMax, int iMaxRatio, bool bWithCargoOnly, bool bOutsideCityOnly, int iMaxPath)
{
	PROFILE_FUNC();

	CvUnit* pLoopUnit;
	CvUnit* pBestUnit;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iLoop;

	iBestValue = 0;
	pBestUnit = NULL;

	for(pLoopUnit = GET_PLAYER(getOwnerINLINE()).firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = GET_PLAYER(getOwnerINLINE()).nextUnit(&iLoop))
	{
		if (pLoopUnit != this)
		{
			if (AI_plotValid(pLoopUnit->plot()))
			{
				if (pLoopUnit->isGroupHead())
				{
					if (!(pLoopUnit->isCargo()))
					{
						if (pLoopUnit->AI_getUnitAIType() == eUnitAI)
						{
							if (pLoopUnit->getGroup()->baseMoves() <= getGroup()->baseMoves())
							{
								if (!bWithCargoOnly || pLoopUnit->getGroup()->hasCargo())
								{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/08/08                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
									if( bOutsideCityOnly && pLoopUnit->plot()->isCity() )
									{
										continue;
									}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

									int iShadowerCount = GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(pLoopUnit, MISSIONAI_SHADOW, getGroup());
									if (((-1 == iMax) || (iShadowerCount < iMax)) &&
										 ((-1 == iMaxRatio) || (iShadowerCount == 0) || (((100 * iShadowerCount) / std::max(1, pLoopUnit->getGroup()->countNumUnitAIType(eUnitAI))) <= iMaxRatio)))
									{
										if (!(pLoopUnit->plot()->isVisibleEnemyUnit(this)))
										{
											if (generatePath(pLoopUnit->plot(), 0, true, &iPathTurns, iMaxPath))
											{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/08/08                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
/* original bts code
												//if (iPathTurns <= iMaxPath) XXX
*/
												if (iPathTurns <= iMaxPath)
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
												{
													iValue = 1 + pLoopUnit->getGroup()->getCargo();
													iValue *= 1000;
													iValue /= 1 + iPathTurns;

													if (iValue > iBestValue)
													{
														iBestValue = iValue;
														pBestUnit = pLoopUnit;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestUnit != NULL)
	{
		if (atPlot(pBestUnit->plot()))
		{
			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_SHADOW, NULL, pBestUnit);
			return true;
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO_UNIT, pBestUnit->getOwnerINLINE(), pBestUnit->getID(), 0, false, false, MISSIONAI_SHADOW, NULL, pBestUnit);
			return true;
		}
	}

	return false;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/22/10                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
// Added new options to aid transport grouping
// Returns true if a group was joined or a mission was pushed...
bool CvUnitAI::AI_group(UnitAITypes eUnitAI, int iMaxGroup, int iMaxOwnUnitAI, int iMinUnitAI, bool bIgnoreFaster, bool bIgnoreOwnUnitType, bool bStackOfDoom, int iMaxPath, bool bAllowRegrouping, bool bWithCargoOnly, bool bInCityOnly, MissionAITypes eIgnoreMissionAIType)
{
	PROFILE_FUNC();

	CvUnit* pLoopUnit;
	CvUnit* pBestUnit;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iLoop;

	// if we are on a transport, then do not regroup
	if (isCargo())
	{
		return false;
	}

	if (!bAllowRegrouping)
	{
		if (getGroup()->getNumUnits() > 1)
		{
			return false;
		}
	}

	if ((getDomainType() == DOMAIN_LAND) && !canMoveAllTerrain())
	{
		if (area()->getNumAIUnits(getOwnerINLINE(), eUnitAI) == 0)
		{
			return false;
		}
	}

	if (!AI_canGroupWithAIType(eUnitAI))
	{
		return false;
	}

	int iOurImpassableCount = 0;
	CLLNode<IDInfo>* pUnitNode = getGroup()->headUnitNode();
	while (pUnitNode != NULL)
	{
		CvUnit* pImpassUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = getGroup()->nextUnitNode(pUnitNode);

		iOurImpassableCount = std::max(iOurImpassableCount, GET_PLAYER(getOwnerINLINE()).AI_unitImpassableCount(pImpassUnit->getUnitType()));
	}

	iBestValue = MAX_INT;
	pBestUnit = NULL;

	// Loop over groups, ai_allowgroup blocks non-head units anyway
	CvSelectionGroup* pLoopGroup = NULL;
	for(pLoopGroup = GET_PLAYER(getOwnerINLINE()).firstSelectionGroup(&iLoop); pLoopGroup != NULL; pLoopGroup = GET_PLAYER(getOwnerINLINE()).nextSelectionGroup(&iLoop))
	{
		pLoopUnit = pLoopGroup->getHeadUnit();
		if( pLoopUnit == NULL )
		{
			continue;
		}

		CvPlot* pPlot = pLoopUnit->plot();
		if (AI_plotValid(pPlot))
		{
			if (iMaxPath > 0 || pPlot == plot())
			{
				if (!isEnemy(pPlot->getTeam()))
				{
					if (AI_allowGroup(pLoopUnit, eUnitAI))
					{
						if ((iMaxGroup == -1) || ((pLoopGroup->getNumUnits() + GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(pLoopUnit, MISSIONAI_GROUP, getGroup())) <= (iMaxGroup + ((bStackOfDoom) ? AI_stackOfDoomExtra() : 0))))
						{
							if ((iMaxOwnUnitAI == -1) || (pLoopGroup->countNumUnitAIType(AI_getUnitAIType()) <= (iMaxOwnUnitAI + ((bStackOfDoom) ? AI_stackOfDoomExtra() : 0))))
							{
								if ((iMinUnitAI == -1) || (pLoopGroup->countNumUnitAIType(eUnitAI) >= iMinUnitAI))
								{
									if (!bIgnoreFaster || (pLoopGroup->baseMoves() <= baseMoves()))
									{
										if (!bIgnoreOwnUnitType || (pLoopUnit->getUnitType() != getUnitType()))
										{
											if (!bWithCargoOnly || pLoopUnit->getGroup()->hasCargo())
											{
												if( !bInCityOnly || pLoopUnit->plot()->isCity() )
												{
													if( (eIgnoreMissionAIType == NO_MISSIONAI) || (eIgnoreMissionAIType != pLoopUnit->getGroup()->AI_getMissionAIType()) )
													{
														if (!(pPlot->isVisibleEnemyUnit(this)))
														{
															if( iOurImpassableCount > 0 || AI_getUnitAIType() == UNITAI_ASSAULT_SEA )
															{
																int iTheirImpassableCount = 0;
																pUnitNode = pLoopGroup->headUnitNode();
																while (pUnitNode != NULL)
																{
																	CvUnit* pImpassUnit = ::getUnit(pUnitNode->m_data);
																	pUnitNode = pLoopGroup->nextUnitNode(pUnitNode);

																	iTheirImpassableCount = std::max(iTheirImpassableCount, GET_PLAYER(getOwnerINLINE()).AI_unitImpassableCount(pImpassUnit->getUnitType()));
																}

																if( iOurImpassableCount != iTheirImpassableCount )
																{
																	continue;
																}
															}

															if (generatePath(pPlot, 0, true, &iPathTurns, iMaxPath))
															{
																if (iPathTurns <= iMaxPath)
																{
																	iValue = 1000 * (iPathTurns + 1);
																	iValue *= 4 + pLoopGroup->getCargo();
																	iValue /= pLoopGroup->getNumUnits();

																	if (iValue < iBestValue)
																	{
																		iBestValue = iValue;
																		pBestUnit = pLoopUnit;
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestUnit != NULL)
	{
		if (atPlot(pBestUnit->plot()))
		{
			joinGroup(pBestUnit->getGroup());
			return true;
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO_UNIT, pBestUnit->getOwnerINLINE(), pBestUnit->getID(), 0, false, false, MISSIONAI_GROUP, NULL, pBestUnit);
			return true;
		}
	}

	return false;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

bool CvUnitAI::AI_groupMergeRange(UnitAITypes eUnitAI, int iMaxRange, bool bBiggerOnly, bool bAllowRegrouping, bool bIgnoreFaster)
{
	PROFILE_FUNC();


	// if we are on a transport, then do not regroup
	if (isCargo())
	{
		return false;
	}

   if (!bAllowRegrouping)
	{
		if (getGroup()->getNumUnits() > 1)
		{
			return false;
		}
	}

	if ((getDomainType() == DOMAIN_LAND) && !canMoveAllTerrain())
	{
		if (area()->getNumAIUnits(getOwnerINLINE(), eUnitAI) == 0)
		{
			return false;
		}
	}

	if (!AI_canGroupWithAIType(eUnitAI))
	{
		return false;
	}

	// cached values
	CvPlot* pPlot = plot();
	CvSelectionGroup* pGroup = getGroup();

	// best match
	CvUnit* pBestUnit = NULL;
	int iBestValue = MAX_INT;
	// iterate over plots at each range
	for (int iDX = -(iMaxRange); iDX <= iMaxRange; iDX++)
	{
		for (int iDY = -(iMaxRange); iDY <= iMaxRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL && pLoopPlot->getArea() == pPlot->getArea())
			{
				CLLNode<IDInfo>* pUnitNode = pLoopPlot->headUnitNode();
				while (pUnitNode != NULL)
				{
					CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
					pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

					CvSelectionGroup* pLoopGroup = pLoopUnit->getGroup();

					if (AI_allowGroup(pLoopUnit, eUnitAI))
					{
						if (!bIgnoreFaster || (pLoopUnit->getGroup()->baseMoves() <= baseMoves()))
						{
							if (!bBiggerOnly || (pLoopGroup->getNumUnits() >= pGroup->getNumUnits()))
							{
								int iPathTurns;
								if (generatePath(pLoopPlot, 0, true, &iPathTurns, iMaxRange))
								{
									if (iPathTurns <= (iMaxRange + 2))
									{
										int iValue = 1000 * (iPathTurns + 1);
										iValue /= pLoopGroup->getNumUnits();

										if (iValue < iBestValue)
										{
											iBestValue = iValue;
											pBestUnit = pLoopUnit;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestUnit != NULL)
	{
		if (atPlot(pBestUnit->plot()))
		{
			pGroup->mergeIntoGroup(pBestUnit->getGroup());
			return true;
		}
		else
		{
			pGroup->pushMission(MISSION_MOVE_TO_UNIT, pBestUnit->getOwnerINLINE(), pBestUnit->getID(), 0, false, false, MISSIONAI_GROUP, NULL, pBestUnit);
			return true;
		}
	}

	return false;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/18/10                                jdog5000      */
/*                                                                                              */
/* War tactics AI, Unit AI                                                                      */
/************************************************************************************************/
// Returns true if we loaded onto a transport or a mission was pushed...
bool CvUnitAI::AI_load(UnitAITypes eUnitAI, MissionAITypes eMissionAI, UnitAITypes eTransportedUnitAI, int iMinCargo, int iMinCargoSpace, int iMaxCargoSpace, int iMaxCargoOurUnitAI, int iFlags, int iMaxPath, int iMaxTransportPath)
{
	PROFILE_FUNC();

	CvUnit* pLoopUnit;
	CvUnit* pBestUnit;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iLoop;

	if (getCargo() > 0)
	{
		return false;
	}

	if (isCargo())
	{
		getGroup()->pushMission(MISSION_SKIP);
		return true;
	}

	if ((getDomainType() == DOMAIN_LAND) && !canMoveAllTerrain())
	{
		if (area()->getNumAIUnits(getOwnerINLINE(), eUnitAI) == 0)
		{
			return false;
		}
	}

	iBestValue = MAX_INT;
	pBestUnit = NULL;

	const int iLoadMissionAICount = 4;
	MissionAITypes aeLoadMissionAI[iLoadMissionAICount] = {MISSIONAI_LOAD_ASSAULT, MISSIONAI_LOAD_SETTLER, MISSIONAI_LOAD_SPECIAL, MISSIONAI_ATTACK_SPY};

	int iCurrentGroupSize = getGroup()->getNumUnits();

	for(pLoopUnit = GET_PLAYER(getOwnerINLINE()).firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = GET_PLAYER(getOwnerINLINE()).nextUnit(&iLoop))
	{
		if (pLoopUnit != this)
		{
			if (AI_plotValid(pLoopUnit->plot()))
			{
				if (canLoadUnit(pLoopUnit, pLoopUnit->plot()))
				{
					// special case ASSAULT_SEA UnitAI, so that, if a unit is marked escort, but can load units, it will load them
					// transport units might have been built as escort, this most commonly happens with galleons
					UnitAITypes eLoopUnitAI = pLoopUnit->AI_getUnitAIType();
					if (eLoopUnitAI == eUnitAI)// || (eUnitAI == UNITAI_ASSAULT_SEA && eLoopUnitAI == UNITAI_ESCORT_SEA))
					{
						int iCargoSpaceAvailable = pLoopUnit->cargoSpaceAvailable(getSpecialUnitType(), getDomainType());
						iCargoSpaceAvailable -= GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(pLoopUnit, aeLoadMissionAI, iLoadMissionAICount, getGroup());
						if (iCargoSpaceAvailable > 0)
						{
							if ((eTransportedUnitAI == NO_UNITAI) || (pLoopUnit->getUnitAICargo(eTransportedUnitAI) > 0))
							{
								if ((iMinCargo == -1) || (pLoopUnit->getCargo() >= iMinCargo))
								{
									// Use existing count of cargo space available
									if ((iMinCargoSpace == -1) || (iCargoSpaceAvailable >= iMinCargoSpace))
									{
										if ((iMaxCargoSpace == -1) || (iCargoSpaceAvailable <= iMaxCargoSpace))
										{
											if ((iMaxCargoOurUnitAI == -1) || (pLoopUnit->getUnitAICargo(AI_getUnitAIType()) <= iMaxCargoOurUnitAI))
											{
												// Don't block city defense from getting on board
												if (true)
												{
													if (!(pLoopUnit->plot()->isVisibleEnemyUnit(this)))
													{
														CvPlot* pUnitTargetPlot = pLoopUnit->getGroup()->AI_getMissionAIPlot();
														if ((pUnitTargetPlot == NULL) || (pUnitTargetPlot->getTeam() == getTeam()) || (!pUnitTargetPlot->isOwned() || !isPotentialEnemy(pUnitTargetPlot->getTeam(), pUnitTargetPlot)))
														{
															if (generatePath(pLoopUnit->plot(), iFlags, true, &iPathTurns, iMaxPath))
															{
																if (iPathTurns <= iMaxPath || (iMaxPath == 0 && plot() == pLoopUnit->plot()))
																{
																	// prefer a transport that can hold as much of our group as possible
																	iValue = (std::max(0, iCurrentGroupSize - iCargoSpaceAvailable) * 5) + iPathTurns;

																	if (iValue < iBestValue)
																	{
																		iBestValue = iValue;
																		pBestUnit = pLoopUnit;
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if( pBestUnit != NULL && iMaxTransportPath < MAX_INT )
	{
		// Can transport reach enemy in requested time
		bool bFoundEnemyPlotInRange = false;
		int iPathTurns;
		int iRange = iMaxTransportPath * pBestUnit->baseMoves();
		CvPlot* pAdjacentPlot = NULL;

		for( int iDX = -iRange; (iDX <= iRange && !bFoundEnemyPlotInRange); iDX++ )
		{
			for( int iDY = -iRange; (iDY <= iRange && !bFoundEnemyPlotInRange); iDY++ )
			{
				CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

				if( pLoopPlot != NULL )
				{
					if( pLoopPlot->isCoastalLand() )
					{
						if( pLoopPlot->isOwned() )
						{
							if( isPotentialEnemy(pLoopPlot->getTeam(), pLoopPlot) && !isBarbarian() )
							{
								if( pLoopPlot->area()->getCitiesPerPlayer(pLoopPlot->getOwnerINLINE()) > 0 )
								{
									// Transport cannot enter land plot without cargo, so generate path only works properly if
									// land units are already loaded

									for( int iI = 0; (iI < NUM_DIRECTION_TYPES && !bFoundEnemyPlotInRange); iI++ )
									{
										pAdjacentPlot = plotDirection(getX_INLINE(), getY_INLINE(), (DirectionTypes)iI);
										if (pAdjacentPlot != NULL)
										{
											if( pAdjacentPlot->isWater() )
											{
												if( pBestUnit->generatePath(pAdjacentPlot, 0, true, &iPathTurns, iMaxTransportPath) )
												{
													if (pBestUnit->getPathLastNode()->m_iData1 == 0)
													{
														iPathTurns++;
													}

													if( iPathTurns <= iMaxTransportPath )
													{
														bFoundEnemyPlotInRange = true;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}

		if( !bFoundEnemyPlotInRange )
		{
			pBestUnit = NULL;
		}
	}

	if (pBestUnit != NULL)
	{
		if (atPlot(pBestUnit->plot()))
		{
			CvSelectionGroup* pOtherGroup = NULL;
			getGroup()->setTransportUnit(pBestUnit, &pOtherGroup); // XXX is this dangerous (not pushing a mission...) XXX air units?

			// If part of large group loaded, then try to keep loading the rest
			if( eUnitAI == UNITAI_ASSAULT_SEA && eMissionAI == MISSIONAI_LOAD_ASSAULT )
			{
				if( pOtherGroup != NULL && pOtherGroup->getNumUnits() > 0 )
				{
					if( pOtherGroup->getHeadUnitAI() == AI_getUnitAIType() )
					{
						pOtherGroup->getHeadUnit()->AI_load( eUnitAI, eMissionAI, eTransportedUnitAI, iMinCargo, iMinCargoSpace, iMaxCargoSpace, iMaxCargoOurUnitAI, iFlags, 0, iMaxTransportPath );
					}
					else if( eTransportedUnitAI == NO_UNITAI && iMinCargo < 0 && iMinCargoSpace < 0 && iMaxCargoSpace < 0 && iMaxCargoOurUnitAI < 0 )
					{
						pOtherGroup->getHeadUnit()->AI_load( eUnitAI, eMissionAI, NO_UNITAI, -1, -1, -1, -1, iFlags, 0, iMaxTransportPath );
					}
				}
			}

			return true;
		}
		else
		{
			// BBAI TODO: To split or not to split?
			int iCargoSpaceAvailable = pBestUnit->cargoSpaceAvailable(getSpecialUnitType(), getDomainType());
			FAssertMsg(iCargoSpaceAvailable > 0, "best unit has no space");

			// split our group to fit on the transport
			CvSelectionGroup* pOtherGroup = NULL;
			CvSelectionGroup* pSplitGroup = getGroup()->splitGroup(iCargoSpaceAvailable, this, &pOtherGroup);
			FAssertMsg(pSplitGroup != NULL, "splitGroup failed");
			FAssertMsg(m_iGroupID == pSplitGroup->getID(), "splitGroup failed to put unit in the new group");

			if (pSplitGroup != NULL)
			{
				CvPlot* pOldPlot = pSplitGroup->plot();
				pSplitGroup->pushMission(MISSION_MOVE_TO_UNIT, pBestUnit->getOwnerINLINE(), pBestUnit->getID(), iFlags, false, false, eMissionAI, NULL, pBestUnit);
				bool bMoved = (pSplitGroup->plot() != pOldPlot);
				if (!bMoved && pOtherGroup != NULL)
				{
					joinGroup(pOtherGroup);
				}
				return bMoved;
			}
		}
	}

	return false;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


// Returns true if a mission was pushed...
bool CvUnitAI::AI_guardCityBestDefender()
{
	CvCity* pCity;
	CvPlot* pPlot;

	pPlot = plot();
	pCity = pPlot->getPlotCity();

	if (pCity != NULL)
	{
		if (pCity->getOwnerINLINE() == getOwnerINLINE())
		{
			if (pPlot->getBestDefender(getOwnerINLINE()) == this)
			{
				getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GUARD_CITY, NULL);
				return true;
			}
		}
	}

	return false;
}

bool CvUnitAI::AI_guardCityMinDefender(bool bSearch)
{
	PROFILE_FUNC();

	CvCity* pPlotCity = plot()->getPlotCity();
	if ((pPlotCity != NULL) && (pPlotCity->getOwnerINLINE() == getOwnerINLINE()))
	{
		int iCityDefenderCount = pPlotCity->plot()->plotCount(PUF_isUnitAIType, UNITAI_CITY_DEFENSE, -1, getOwnerINLINE());
		if ((iCityDefenderCount - 1) < pPlotCity->AI_minDefenders())
		{
			if ((iCityDefenderCount <= 2) || (GC.getGame().getSorenRandNum(5, "AI shuffle defender") != 0))
			{
				getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GUARD_CITY, NULL);
				return true;
			}
		}
	}

	if (bSearch)
	{
		int iBestValue = 0;
		CvPlot* pBestPlot = NULL;
		CvPlot* pBestGuardPlot = NULL;

		CvCity* pLoopCity;
		int iLoop;

		int iCurrentTurn = GC.getGame().getGameTurn();
		for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
		{
			if (AI_plotValid(pLoopCity->plot()))
			{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
				// BBAI efficiency: check area for land units
				if( (getDomainType() == DOMAIN_LAND) && (pLoopCity->area() != area()) && !(getGroup()->canMoveAllTerrain()) )
				{
					continue;
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				int iDefendersHave = pLoopCity->plot()->plotCount(PUF_isUnitAIType, UNITAI_CITY_DEFENSE, -1, getOwnerINLINE());
				int iDefendersNeed = pLoopCity->AI_minDefenders();
				if (iDefendersHave < iDefendersNeed)
				{
					if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
					{
						iDefendersHave += GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_GUARD_CITY, getGroup());
						if (iDefendersHave < iDefendersNeed + 1)
						{
							int iPathTurns;
							if (!atPlot(pLoopCity->plot()) && generatePath(pLoopCity->plot(), 0, true, &iPathTurns, 10))
							{
								if (iPathTurns <= 10)
								{
									int iValue = (iDefendersNeed - iDefendersHave) * 20;
									iValue += 2 * std::min(15, iCurrentTurn - pLoopCity->getGameTurnAcquired());
									if (pLoopCity->isOccupation())
									{
										iValue += 5;
									}
									iValue -= iPathTurns;

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = getPathEndTurnPlot();
										pBestGuardPlot = pLoopCity->plot();
									}
								}
							}
						}
					}
				}
			}
		}
		if (pBestPlot != NULL)
		{
			if (atPlot(pBestGuardPlot))
			{
				FAssert(pBestGuardPlot == pBestPlot);
				getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GUARD_CITY, NULL);
				return true;
			}
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_GUARD_CITY, pBestGuardPlot);
			return true;
		}
	}

	return false;
}

// Returns true if a mission was pushed...
bool CvUnitAI::AI_guardCity(bool bLeave, bool bSearch, int iMaxPath)
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvCity* pCity;
	CvCity* pLoopCity;
	CvUnit* pLoopUnit;
	CvPlot* pPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestGuardPlot;
	bool bDefend;
	int iExtra;
	int iCount;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iLoop;

	FAssert(getDomainType() == DOMAIN_LAND);
	FAssert(canDefend());

	pPlot = plot();
	pCity = pPlot->getPlotCity();

	if ((pCity != NULL) && (pCity->getOwnerINLINE() == getOwnerINLINE())) // XXX check for other team?
	{
		if (bLeave && !(pCity->AI_isDanger()))
		{
			iExtra = 1;
		}
		else
		{
			iExtra = (bSearch ? 0 : -GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(pPlot, 2));
		}

		bDefend = false;

		if (pPlot->plotCount(PUF_canDefendGroupHead, -1, -1, getOwnerINLINE()) == 1) // XXX check for other team's units?
		{
			bDefend = true;
		}
		else if (!(pCity->AI_isDefended(((AI_isCityAIType()) ? -1 : 0) + iExtra))) // XXX check for other team's units?
		{
			if (AI_isCityAIType())
			{
				bDefend = true;
			}
			else
			{
				iCount = 0;

				pUnitNode = pPlot->headUnitNode();

				while (pUnitNode != NULL)
				{
					pLoopUnit = ::getUnit(pUnitNode->m_data);
					pUnitNode = pPlot->nextUnitNode(pUnitNode);

					if (pLoopUnit->getOwnerINLINE() == getOwnerINLINE())
					{
						if (pLoopUnit->isGroupHead())
						{
							if (!(pLoopUnit->isCargo()))
							{
								if (pLoopUnit->canDefend())
								{
									if (!(pLoopUnit->AI_isCityAIType()))
									{
										if (!(pLoopUnit->isHurt()))
										{
											if (pLoopUnit->isWaiting())
											{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/24/09                                jdog5000      */
/*                                                                                              */
/* Unit AI																						 */
/************************************************************************************************/
												//FAssert(pLoopUnit != this);
												if( pLoopUnit != this )
												{
													iCount++;
												}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
											}
										}
									}
									else
									{
										if (pLoopUnit->getGroup()->getMissionType(0) != MISSION_SKIP)
										{
											iCount++;
										}
									}
								}
							}
						}
					}
				}

				if (!(pCity->AI_isDefended(iCount + iExtra))) // XXX check for other team's units?
				{
					bDefend = true;
				}
			}
		}

		if (bDefend)
		{
			CvSelectionGroup* pOldGroup = getGroup();
			CvUnit* pEjectedUnit = getGroup()->AI_ejectBestDefender(pPlot);

			if (pEjectedUnit != NULL)
			{
				if (pPlot->plotCount(PUF_isCityAIType, -1, -1, getOwnerINLINE()) == 0)
				{
					if (pEjectedUnit->cityDefenseModifier() > 0)
					{
						pEjectedUnit->AI_setUnitAIType(UNITAI_CITY_DEFENSE);
					}
				}
				pEjectedUnit->getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GUARD_CITY, NULL);
				if (pEjectedUnit->getGroup() == pOldGroup || pEjectedUnit == this)
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				//This unit is not suited for defense, skip the mission
				//to protect this city but encourage others to defend instead.
				getGroup()->pushMission(MISSION_SKIP);
				if (!isHurt())
				{
					finishMoves();
				}
			}
			return true;
		}
	}

	if (bSearch)
	{
		iBestValue = MAX_INT;
		pBestPlot = NULL;
		pBestGuardPlot = NULL;

		for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
		{
			if (AI_plotValid(pLoopCity->plot()))
			{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
				// BBAI efficiency: check area for land units
				if( (getDomainType() == DOMAIN_LAND) && (pLoopCity->area() != area()) && !(getGroup()->canMoveAllTerrain()) )
				{
					continue;
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				if (!(pLoopCity->AI_isDefended((!AI_isCityAIType()) ? pLoopCity->plot()->plotCount(PUF_canDefendGroupHead, -1, -1, getOwnerINLINE(), NO_TEAM, PUF_isNotCityAIType) : 0)))	// XXX check for other team's units?
				{
					if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
					{
						if ((GC.getGame().getGameTurn() - pLoopCity->getGameTurnAcquired() < 10) || GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_GUARD_CITY, getGroup()) < 2)
						{
							if (!atPlot(pLoopCity->plot()) && generatePath(pLoopCity->plot(), 0, true, &iPathTurns, iMaxPath))
							{
								if (iPathTurns <= iMaxPath)
								{
									iValue = iPathTurns;

									if (iValue < iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = getPathEndTurnPlot();
										pBestGuardPlot = pLoopCity->plot();
										FAssert(!atPlot(pBestPlot));
									}
								}
							}
						}
					}
				}

				if (pBestPlot != NULL)
				{
					break;
				}
			}
		}

		if ((pBestPlot != NULL) && (pBestGuardPlot != NULL))
		{
			FAssert(!atPlot(pBestPlot));
			// split up group if we are going to defend, so rest of group has opportunity to do something else
//			if (getGroup()->getNumUnits() > 1)
//			{
//				getGroup()->AI_separate();	// will change group
//			}
//
//			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_GUARD_CITY, pBestGuardPlot);
//			return true;

			CvSelectionGroup* pOldGroup = getGroup();
			CvUnit* pEjectedUnit = getGroup()->AI_ejectBestDefender(pPlot);

			if (pEjectedUnit != NULL)
			{
				pEjectedUnit->getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_GUARD_CITY, NULL);
				if (pEjectedUnit->getGroup() == pOldGroup || pEjectedUnit == this)
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				//This unit is not suited for defense, skip the mission
				//to protect this city but encourage others to defend instead.
				if (atPlot(pBestGuardPlot))
				{
					getGroup()->pushMission(MISSION_SKIP);
				}
				else
				{
					getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_GUARD_CITY, NULL);
				}
				return true;
			}
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_guardCityAirlift()
{
	PROFILE_FUNC();

	CvCity* pCity;
	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	int iValue;
	int iBestValue;
	int iLoop;

	if (getGroup()->getNumUnits() > 1)
	{
		return false;
	}

	pCity = plot()->getPlotCity();

	if (pCity == NULL)
	{
		return false;
	}

	if (pCity->getMaxAirlift() == 0)
	{
		return false;
	}

	iBestValue = 0;
	pBestPlot = NULL;

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if (pLoopCity != pCity)
		{
			if (canAirliftAt(pCity->plot(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE()))
			{
				if (!(pLoopCity->AI_isDefended((!AI_isCityAIType()) ? pLoopCity->plot()->plotCount(PUF_canDefendGroupHead, -1, -1, getOwnerINLINE(), NO_TEAM, PUF_isNotCityAIType) : 0)))	// XXX check for other team's units?
				{
					iValue = pLoopCity->getPopulation();

					if (pLoopCity->AI_isDanger())
					{
						iValue *= 2;
					}

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pBestPlot = pLoopCity->plot();
						FAssert(pLoopCity != pCity);
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_AIRLIFT, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_guardBonus(int iMinValue)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestGuardPlot;
	ImprovementTypes eImprovement;
	BonusTypes eNonObsoleteBonus;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iI;

	iBestValue = 0;
	pBestPlot = NULL;
	pBestGuardPlot = NULL;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (AI_plotValid(pLoopPlot))
		{
			if (pLoopPlot->getOwnerINLINE() == getOwnerINLINE())
			{
				eNonObsoleteBonus = pLoopPlot->getNonObsoleteBonusType(getTeam());

//FfH: Modified by Kael 03/22/2008
//				if (eNonObsoleteBonus != NO_BONUS)
//				{
//					eImprovement = pLoopPlot->getImprovementType();
//
//					if ((eImprovement != NO_IMPROVEMENT) && GC.getImprovementInfo(eImprovement).isImprovementBonusTrade(eNonObsoleteBonus))
//					{
//						iValue = GET_PLAYER(getOwnerINLINE()).AI_bonusVal(eNonObsoleteBonus);
//						iValue += std::max(0, 200 * GC.getBonusInfo(eNonObsoleteBonus).getAIObjective());
//
//						if (pLoopPlot->getPlotGroupConnectedBonus(getOwnerINLINE(), eNonObsoleteBonus) == 1)
//						{
//							iValue *= 2;
//						}
				iValue = 0;
				eImprovement = pLoopPlot->getImprovementType();
				if (eImprovement != NO_IMPROVEMENT)
				{
					if (eNonObsoleteBonus != NO_BONUS && GC.getImprovementInfo(eImprovement).isImprovementBonusTrade(eNonObsoleteBonus))
					{
						iValue += GET_PLAYER(getOwnerINLINE()).AI_bonusVal(eNonObsoleteBonus);
						iValue += std::max(0, 200 * GC.getBonusInfo(eNonObsoleteBonus).getAIObjective());
						if (pLoopPlot->getPlotGroupConnectedBonus(getOwnerINLINE(), eNonObsoleteBonus) == 1)
						{
							iValue *= 2;
						}
					}
					iValue += GC.getImprovementInfo(eImprovement).getRangeDefenseModifier() * GC.getImprovementInfo(eImprovement).getRange() * 200;
//FfH: End Modify

						if (iValue > iMinValue)
						{
							if (!(pLoopPlot->isVisibleEnemyUnit(this)))
							{
								// BBAI TODO: Multiple defenders for higher value resources?
								if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_GUARD_BONUS, getGroup()) == 0)
								{
									if (generatePath(pLoopPlot, 0, true, &iPathTurns))
									{
										iValue *= 1000;

										iValue /= (iPathTurns + 1);

										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											pBestPlot = getPathEndTurnPlot();
											pBestGuardPlot = pLoopPlot;
										}
									}
								}
							}
						}
					}
				}
			}
		}

//FfH: Modified by Kael 10/29/2007
//	}
//FfH: End Modify

	if ((pBestPlot != NULL) && (pBestGuardPlot != NULL))
	{
		if (atPlot(pBestGuardPlot))
		{
			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GUARD_BONUS, pBestGuardPlot);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_GUARD_BONUS, pBestGuardPlot);
			return true;
		}
	}

	return false;
}

int CvUnitAI::AI_getPlotDefendersNeeded(CvPlot* pPlot, int iExtra)
{
	int iNeeded = iExtra;
	BonusTypes eNonObsoleteBonus = pPlot->getNonObsoleteBonusType(getTeam());
	if (eNonObsoleteBonus != NO_BONUS)
	{
		iNeeded += (GET_PLAYER(getOwnerINLINE()).AI_bonusVal(eNonObsoleteBonus) + 10) / 19;
	}

	int iDefense = pPlot->defenseModifier(getTeam(), true);

	iNeeded += (iDefense + 25) / 50;

	if (iNeeded == 0)
	{
		return 0;
	}

	iNeeded += GET_PLAYER(getOwnerINLINE()).AI_getPlotAirbaseValue(pPlot) / 50;

	int iNumHostiles = 0;
	int iNumPlots = 0;

	int iRange = 2;
	for (int iX = -iRange; iX <= iRange; iX++)
	{
		for (int iY = -iRange; iY <= iRange; iY++)
		{
			CvPlot* pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iX, iY);
			if (pLoopPlot != NULL)
			{
				iNumHostiles += pLoopPlot->getNumVisibleEnemyDefenders(this);
				if ((pLoopPlot->getTeam() != getTeam()) || pLoopPlot->isCoastalLand())
				{
					iNumPlots++;
					if (isEnemy(pLoopPlot->getTeam()))
					{
						iNumPlots += 4;
					}
				}
			}
		}
	}

	if ((iNumHostiles == 0) && (iNumPlots < 4))
	{
		if (iNeeded > 1)
		{
			iNeeded = 1;
		}
		else
		{
			iNeeded = 0;
		}
	}

	return iNeeded;
}

bool CvUnitAI::AI_guardFort(bool bSearch)
{
	PROFILE_FUNC();

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		ImprovementTypes eImprovement = plot()->getImprovementType();
		if (eImprovement != NO_IMPROVEMENT)
		{
			if (GC.getImprovementInfo(eImprovement).isActsAsCity())
			{
				if (plot()->plotCount(PUF_isCityAIType, -1, -1, getOwnerINLINE()) <= AI_getPlotDefendersNeeded(plot(), 0))
				{
					getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GUARD_BONUS, plot());
					return true;
				}
			}
		}
	}

	if (!bSearch)
	{
		return false;
	}

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	CvPlot* pBestGuardPlot = NULL;

	for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (AI_plotValid(pLoopPlot) && !atPlot(pLoopPlot))
		{
			if (pLoopPlot->getOwnerINLINE() == getOwnerINLINE())
			{
				ImprovementTypes eImprovement = pLoopPlot->getImprovementType();
				if (eImprovement != NO_IMPROVEMENT)
				{
					if (GC.getImprovementInfo(eImprovement).isActsAsCity())
					{
						int iValue = AI_getPlotDefendersNeeded(pLoopPlot, 0);

						if (iValue > 0)
						{
							if (!(pLoopPlot->isVisibleEnemyUnit(this)))
							{
								if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_GUARD_BONUS, getGroup()) < iValue)
								{
									int iPathTurns;
									if (generatePath(pLoopPlot, 0, true, &iPathTurns))
									{
										iValue *= 1000;

										iValue /= (iPathTurns + 2);

										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											pBestPlot = getPathEndTurnPlot();
											pBestGuardPlot = pLoopPlot;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestGuardPlot != NULL))
	{
		if (atPlot(pBestGuardPlot))
		{
			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GUARD_BONUS, pBestGuardPlot);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_GUARD_BONUS, pBestGuardPlot);
			return true;
		}
	}

	return false;
}
// Returns true if a mission was pushed...
bool CvUnitAI::AI_guardCitySite()
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestGuardPlot;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iI;

	iBestValue = 0;
	pBestPlot = NULL;
	pBestGuardPlot = NULL;

	for (iI = 0; iI < GET_PLAYER(getOwnerINLINE()).AI_getNumCitySites(); iI++)
	{
		pLoopPlot = GET_PLAYER(getOwnerINLINE()).AI_getCitySite(iI);
		if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_GUARD_CITY, getGroup()) == 0)
		{
			if (generatePath(pLoopPlot, 0, true, &iPathTurns))
			{
				iValue = pLoopPlot->getFoundValue(getOwnerINLINE());
				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					pBestPlot = getPathEndTurnPlot();
					pBestGuardPlot = pLoopPlot;
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestGuardPlot != NULL))
	{
		if (atPlot(pBestGuardPlot))
		{
			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GUARD_CITY, pBestGuardPlot);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_GUARD_CITY, pBestGuardPlot);
			return true;
		}
	}

	return false;
}



// Returns true if a mission was pushed...
bool CvUnitAI::AI_guardSpy(int iRandomPercent)
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	CvPlot* pBestGuardPlot;
	int iValue;
	int iBestValue;
	int iLoop;

	iBestValue = 0;
	pBestPlot = NULL;
	pBestGuardPlot = NULL;

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if (AI_plotValid(pLoopCity->plot()))
		{
			if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
			{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
				// BBAI efficiency: check area for land units
				if( (getDomainType() == DOMAIN_LAND) && (pLoopCity->area() != area()) && !(getGroup()->canMoveAllTerrain()) )
				{
					continue;
				}

				iValue = 0;

				if( GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_SPACE, 4) )
				{
					if( pLoopCity->isCapital() )
					{
						iValue += 30;
					}
					else if( pLoopCity->isProductionProject() )
					{
						iValue += 5;
					}
				}

				if( GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE, 3) )
				{
					if( pLoopCity->getCultureLevel() >= (GC.getNumCultureLevelInfos() - 2))
					{
						iValue += 10;
					}
				}

				if (pLoopCity->isProductionUnit())
				{
					if (isLimitedUnitClass((UnitClassTypes)(GC.getUnitInfo(pLoopCity->getProductionUnit()).getUnitClassType())))
					{
						iValue += 4;
					}
				}
				else if (pLoopCity->isProductionBuilding())
				{
					if (isLimitedWonderClass((BuildingClassTypes)(GC.getBuildingInfo(pLoopCity->getProductionBuilding()).getBuildingClassType())))
					{
						iValue += 5;
					}
				}
				else if (pLoopCity->isProductionProject())
				{
					if (isLimitedProject(pLoopCity->getProductionProject()))
					{
						iValue += 6;
					}
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

				if (iValue > 0)
				{
					if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_GUARD_SPY, getGroup()) == 0)
					{
						int iPathTurns;
						if (generatePath(pLoopCity->plot(), 0, true, &iPathTurns))
						{
							iValue *= 100 + GC.getGameINLINE().getSorenRandNum(iRandomPercent, "AI Guard Spy");
							//iValue /= 100;
							iValue /= iPathTurns + 1;

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = getPathEndTurnPlot();
								pBestGuardPlot = pLoopCity->plot();
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestGuardPlot != NULL))
	{
		if (atPlot(pBestGuardPlot))
		{
			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_GUARD_SPY, pBestGuardPlot);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_GUARD_SPY, pBestGuardPlot);
			return true;
		}
	}

	return false;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/25/09                                jdog5000      */
/*                                                                                              */
/* Espionage AI                                                                                 */
/************************************************************************************************/
/*
// Never used BTS functions ...

// Returns true if a mission was pushed...
bool CvUnitAI::AI_destroySpy()
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	CvCity* pBestCity;
	CvPlot* pBestPlot;
	int iValue;
	int iBestValue;
	int iLoop;
	int iI;

	iBestValue = 0;
	pBestPlot = NULL;
	pBestCity = NULL;

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() != getTeam())
			{
				if (GET_PLAYER(getOwnerINLINE()).AI_getAttitude((PlayerTypes)iI) <= ATTITUDE_ANNOYED)
				{
					for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
					{
						if (AI_plotValid(pLoopCity->plot()))
						{
							if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_ATTACK_SPY, getGroup()) == 0)
							{
								if (generatePath(pLoopCity->plot(), 0, true))
								{
									iValue = (pLoopCity->getPopulation() * 2);

									iValue += pLoopCity->getYieldRate(YIELD_PRODUCTION);

									if (atPlot(pLoopCity->plot()))
									{
										iValue *= 4;
										iValue /= 3;
									}

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = getPathEndTurnPlot();
										pBestCity = pLoopCity;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestCity != NULL))
	{
		if (atPlot(pBestCity->plot()))
		{
			if (canDestroy(pBestCity->plot()))
			{
				if (pBestCity->getProduction() > ((pBestCity->getProductionNeeded() * 2) / 3))
				{
					if (pBestCity->isProductionUnit())
					{
						if (isLimitedUnitClass((UnitClassTypes)(GC.getUnitInfo(pBestCity->getProductionUnit()).getUnitClassType())))
						{
							getGroup()->pushMission(MISSION_DESTROY);
							return true;
						}
					}
					else if (pBestCity->isProductionBuilding())
					{
						if (isLimitedWonderClass((BuildingClassTypes)(GC.getBuildingInfo(pBestCity->getProductionBuilding()).getBuildingClassType())))
						{
							getGroup()->pushMission(MISSION_DESTROY);
							return true;
						}
					}
					else if (pBestCity->isProductionProject())
					{
						if (isLimitedProject(pBestCity->getProductionProject()))
						{
							getGroup()->pushMission(MISSION_DESTROY);
							return true;
						}
					}
				}
			}

			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY, pBestCity->plot());
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_ATTACK_SPY, pBestCity->plot());
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_sabotageSpy()
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestSabotagePlot;
	bool abPlayerAngry[MAX_PLAYERS];
	ImprovementTypes eImprovement;
	BonusTypes eNonObsoleteBonus;
	int iValue;
	int iBestValue;
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		abPlayerAngry[iI] = false;

		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() != getTeam())
			{
				if (GET_PLAYER(getOwnerINLINE()).AI_getAttitude((PlayerTypes)iI) <= ATTITUDE_ANNOYED)
				{
					abPlayerAngry[iI] = true;
				}
			}
		}
	}

	iBestValue = 0;
	pBestPlot = NULL;
	pBestSabotagePlot = NULL;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (AI_plotValid(pLoopPlot))
		{
			if (pLoopPlot->isOwned())
			{
				if (pLoopPlot->getTeam() != getTeam())
				{
					if (abPlayerAngry[pLoopPlot->getOwnerINLINE()])
					{
						eNonObsoleteBonus = pLoopPlot->getNonObsoleteBonusType(pLoopPlot->getTeam());

						if (eNonObsoleteBonus != NO_BONUS)
						{
							eImprovement = pLoopPlot->getImprovementType();

							if ((eImprovement != NO_IMPROVEMENT) && GC.getImprovementInfo(eImprovement).isImprovementBonusTrade(eNonObsoleteBonus))
							{
								if (canSabotage(pLoopPlot))
								{
									iValue = GET_PLAYER(pLoopPlot->getOwnerINLINE()).AI_bonusVal(eNonObsoleteBonus);

									if (pLoopPlot->isConnectedToCapital() && (pLoopPlot->getPlotGroupConnectedBonus(pLoopPlot->getOwnerINLINE(), eNonObsoleteBonus) == 1))
									{
										iValue *= 3;
									}

									if (iValue > 25)
									{
										if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_ATTACK_SPY, getGroup()) == 0)
										{
											if (generatePath(pLoopPlot, 0, true))
											{
												if (iValue > iBestValue)
												{
													iBestValue = iValue;
													pBestPlot = getPathEndTurnPlot();
													pBestSabotagePlot = pLoopPlot;
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestSabotagePlot != NULL))
	{
		if (atPlot(pBestSabotagePlot))
		{
			getGroup()->pushMission(MISSION_SABOTAGE);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_ATTACK_SPY, pBestSabotagePlot);
			return true;
		}
	}

	return false;
}


bool CvUnitAI::AI_pickupTargetSpy()
{
	PROFILE_FUNC();

	CvCity* pCity;
	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	CvPlot* pBestPickupPlot;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iLoop;

	pCity = plot()->getPlotCity();

	if (pCity != NULL)
	{
		if (pCity->getOwnerINLINE() == getOwnerINLINE())
		{
			if (pCity->isCoastal(GC.getMIN_WATER_SIZE_FOR_OCEAN()))
			{
				getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY, pCity->plot());
				return true;
			}
		}
	}

	iBestValue = MAX_INT;
	pBestPlot = NULL;
	pBestPickupPlot = NULL;

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if (AI_plotValid(pLoopCity->plot()))
		{
			if (pLoopCity->isCoastal(GC.getMIN_WATER_SIZE_FOR_OCEAN()))
			{
				if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
				{
					if (!atPlot(pLoopCity->plot()) && generatePath(pLoopCity->plot(), 0, true, &iPathTurns))
					{
						iValue = iPathTurns;

						if (iValue < iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = getPathEndTurnPlot();
							pBestPickupPlot = pLoopCity->plot();
							FAssert(!atPlot(pBestPlot));
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestPickupPlot != NULL))
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_ATTACK_SPY, pBestPickupPlot);
		return true;
	}

	return false;
}
*/
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


// Returns true if a mission was pushed...
bool CvUnitAI::AI_chokeDefend()
{
	CvCity* pCity;
	int iPlotDanger;

	FAssert(AI_isCityAIType());

	// XXX what about amphib invasions?

	pCity = plot()->getPlotCity();

	if (pCity != NULL)
	{
		if (pCity->getOwnerINLINE() == getOwnerINLINE())
		{
			if (pCity->AI_neededDefenders() > 1)
			{
				if (pCity->AI_isDefended(pCity->plot()->plotCount(PUF_canDefendGroupHead, -1, -1, getOwnerINLINE(), NO_TEAM, PUF_isNotCityAIType)))
				{
					iPlotDanger = GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 3);

					if (iPlotDanger <= 4)
					{
						if (AI_anyAttack(1, 65, std::max(0, (iPlotDanger - 1))))
						{
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_heal(int iDamagePercent, int iMaxPath)
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pEntityNode;
	std::vector<CvUnit*> aeDamagedUnits;
	CvSelectionGroup* pGroup;
	CvUnit* pLoopUnit;
	int iTotalDamage;
	int iTotalHitpoints;
	int iHurtUnitCount;
	bool bRetreat;

	if (plot()->getFeatureType() != NO_FEATURE)
	{
		if (GC.getFeatureInfo(plot()->getFeatureType()).getTurnDamage() != 0)
		{
			//Pass through
			//(actively seeking a safe spot may result in unit getting stuck)
			return false;
		}
	}
	if (plot()->getPlotEffectType() != NO_PLOT_EFFECT)
	{
		if (GC.getPlotEffectInfo(plot()->getPlotEffectType()).getTurnDamage() != 0)
		{
			//Pass through
			//(actively seeking a safe spot may result in unit getting stuck)
			return false;
		}
	}
//FfH: Added by Kael 10/01/2007 (so the AI won't try to sit and heal in areas where they cant heal)
	if (healRate(plot()) <= 0)
	{
		return false;
	}
//FfH: End Add

	pGroup = getGroup();

	if (iDamagePercent == 0)
	{
		iDamagePercent = 10;
	}

	bRetreat = false;

	if (getGroup()->getNumUnits() == 1)
	{
		if (getDamage() > 0)
		{
			if (plot()->isCity() || (healTurns(plot()) == 1))
			{
				if (!(isAlwaysHeal()))
				{
					getGroup()->pushMission(MISSION_HEAL);
					return true;
				}
			}
		}
		return false;
	}

	iMaxPath = std::min(iMaxPath, 2);

	pEntityNode = getGroup()->headUnitNode();

	iTotalDamage = 0;
	iTotalHitpoints = 0;
	iHurtUnitCount = 0;
	while (pEntityNode != NULL)
	{
		pLoopUnit = ::getUnit(pEntityNode->m_data);
		FAssert(pLoopUnit != NULL);
		pEntityNode = pGroup->nextUnitNode(pEntityNode);

		int iDamageThreshold = (pLoopUnit->maxHitPoints() * iDamagePercent) / 100;

		if (NO_UNIT != getLeaderUnitType())
		{
			iDamageThreshold /= 2;
		}

		if (pLoopUnit->getDamage() > 0)
		{
			iHurtUnitCount++;
		}
/*************************************************************************************************/
/**	Higher hitpoints				01/02/11											Snarko	**/
/**						Makes higher values than 100 HP possible.								**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		iTotalDamage += pLoopUnit->getDamage();
		iTotalHitpoints += pLoopUnit->maxHitPoints();

		if (pLoopUnit->getDamage() > iDamageThreshold)
/**								----  End Original Code  ----									**/
		//iTotalDamage += pLoopUnit->getDamageReal(); //Isn't used?!?
		//iTotalHitpoints += pLoopUnit->maxHitPoints(); //Isn't used?!?

		if (pLoopUnit->getDamageReal() > iDamageThreshold)
/*************************************************************************************************/
/**	Higher hitpoints						END													**/
/*************************************************************************************************/
		{
			bRetreat = true;

			if (!(pLoopUnit->hasMoved()))
			{
				if (!(pLoopUnit->isAlwaysHeal()))
				{
					if (pLoopUnit->healTurns(pLoopUnit->plot()) <= iMaxPath)
					{
						aeDamagedUnits.push_back(pLoopUnit);
					}
				}
			}
		}
	}
	if (iHurtUnitCount == 0)
	{
		return false;
	}

	bool bPushedMission = false;
	if (plot()->isCity() && (plot()->getOwnerINLINE() == getOwnerINLINE()))
	{
		FAssertMsg(((int) aeDamagedUnits.size()) <= iHurtUnitCount, "damaged units array is larger than our hurt unit count");

		for (unsigned int iI = 0; iI < aeDamagedUnits.size(); iI++)
		{
			CvUnit* pUnitToHeal = aeDamagedUnits[iI];
			pUnitToHeal->joinGroup(NULL);
			pUnitToHeal->getGroup()->pushMission(MISSION_HEAL);

			// note, removing the head unit from a group will force the group to be completely split if non-human
			if (pUnitToHeal == this)
			{
				bPushedMission = true;
			}

			iHurtUnitCount--;
		}
	}

	if ((iHurtUnitCount * 2) > pGroup->getNumUnits())
	{
		FAssertMsg(pGroup->getNumUnits() > 0, "group now has zero units");

		if (AI_moveIntoCity(2))
		{
			return true;
		}
		else if (healRate(plot()) > 10)
		{
/*************************************************************************************************/
/**	HealRight								04/18/09								Xienwolf	**/
/**																								**/
/**		Units will not attempt to heal in a location which will make such a thing impossible	**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
			pGroup->pushMission(MISSION_HEAL);
			return true;
/**								----  End Original Code  ----									**/
/*************************************************************************************************/
/**	Speedup								11/02/12										Snarko	**/
/**																								**/
/**				Use the must faster getAnyPlotDanger where possible								**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
			if(GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 2) == 0)
/**								----  End Original Code  ----									**/
			if (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot()))
/*************************************************************************************************/
/**	Speedup									END													**/
/*************************************************************************************************/
			{
				pGroup->pushMission(MISSION_HEAL);
				return true;
			}
/*************************************************************************************************/
/**	HealRight								END													**/
/*************************************************************************************************/
		}
	}

	return bPushedMission;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_afterAttack()
{
	if (!isMadeAttack())
	{
		return false;
	}

	if (!canFight())
	{
		return false;
	}

	if (isBlitz())
	{
		return false;
	}

	if (getDomainType() == DOMAIN_LAND)
	{
		if (AI_guardCity(false, true, 1))
		{
			return true;
		}
	}

	if (AI_pillageRange(1))
	{
		return true;
	}

	if (AI_retreatToCity(false, false, 1))
	{
		return true;
	}

	if (AI_hide())
	{
		return true;
	}

	if (AI_goody(1))
	{
		return true;
	}

	if (AI_pillageRange(2))
	{
		return true;
	}

	if (AI_defend())
	{
		return true;
	}

	if (AI_safety())
	{
		return true;
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_goldenAge()
{
	if (canGoldenAge(plot()))
	{
		getGroup()->pushMission(MISSION_GOLDEN_AGE);
		return true;
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_spreadReligion()
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	CvPlot* pBestSpreadPlot;
	ReligionTypes eReligion;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iPlayerMultiplierPercent;
	int iLoop;
	int iI;

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/08/10                                jdog5000      */
/*                                                                                              */
/* Victory Strategy AI                                                                          */
/************************************************************************************************/
	bool bCultureVictory = GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE, 2);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	eReligion = NO_RELIGION;

	// BBAI TODO: Unnecessary with changes below ...
	if (eReligion == NO_RELIGION)
	{
		if (GET_PLAYER(getOwnerINLINE()).getStateReligion() != NO_RELIGION)
		{
			if (m_pUnitInfo->getReligionSpreads(GET_PLAYER(getOwnerINLINE()).getStateReligion()) > 0)
			{
				eReligion = GET_PLAYER(getOwnerINLINE()).getStateReligion();
			}
		}
	}

	if (eReligion == NO_RELIGION)
	{
		for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
		{
			//if (bCultureVictory || GET_TEAM(getTeam()).hasHolyCity((ReligionTypes)iI))
			{
				if (m_pUnitInfo->getReligionSpreads((ReligionTypes)iI) > 0)
				{
					eReligion = ((ReligionTypes)iI);
					break;
				}
			}
		}
	}

	if (eReligion == NO_RELIGION)
	{
		return false;
	}

	bool bHasHolyCity = GET_TEAM(getTeam()).hasHolyCity(eReligion);
	bool bHasAnyHolyCity = bHasHolyCity;
	if (!bHasAnyHolyCity)
	{
		for (iI = 0; !bHasAnyHolyCity && iI < GC.getNumReligionInfos(); iI++)
		{
			bHasAnyHolyCity = GET_TEAM(getTeam()).hasHolyCity((ReligionTypes)iI);
		}
	}

	iBestValue = 0;
	pBestPlot = NULL;
	pBestSpreadPlot = NULL;

	// BBAI TODO: Could also use CvPlayerAI::AI_missionaryValue to determine which player to target ...
	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			iPlayerMultiplierPercent = 0;

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      11/28/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
			//if (GET_PLAYER((PlayerTypes)iI).getTeam() != getTeam())
			if (GET_PLAYER((PlayerTypes)iI).getTeam() != getTeam() && canEnterTerritory(GET_PLAYER((PlayerTypes)iI).getTeam()))
			{
				if (bHasHolyCity)
				{
					iPlayerMultiplierPercent = 100;
					// BBAI TODO: If going for cultural victory, don't spread to other teams?  Sure, this might decrease the chance of
					// someone else winning by culture, but at the cost of $$ in holy city and diplomatic conversions (ie future wars!).
					// Doesn't seem to up our odds of winning by culture really.  Also, no foreign spread after Free Religion?  Still get
					// gold for city count.
					if (!bCultureVictory || (eReligion == GET_PLAYER(getOwnerINLINE()).getStateReligion()))
					{
						if (GET_PLAYER((PlayerTypes)iI).getStateReligion() == NO_RELIGION)
						{
							if (0 == (GET_PLAYER((PlayerTypes)iI).getNonStateReligionHappiness()))
							{
								iPlayerMultiplierPercent += 600;
							}
						}
						else if (GET_PLAYER((PlayerTypes)iI).getStateReligion() == eReligion)
						{
							iPlayerMultiplierPercent += 300;
						}
						else
						{
							if (GET_PLAYER((PlayerTypes)iI).hasHolyCity(GET_PLAYER((PlayerTypes)iI).getStateReligion()))
							{
								iPlayerMultiplierPercent += 50;
							}
							else
							{
								iPlayerMultiplierPercent += 300;
							}
						}

						int iReligionCount = GET_PLAYER((PlayerTypes)iI).countTotalHasReligion();
						int iCityCount = GET_PLAYER(getOwnerINLINE()).getNumCities();
						//magic formula to produce normalized adjustment factor based on religious infusion
						int iAdjustment = (100 * (iCityCount + 1));
						iAdjustment /= ((iCityCount + 1) + iReligionCount);
						iAdjustment = (((iAdjustment - 25) * 4) / 3);

						iAdjustment = std::max(10, iAdjustment);

						iPlayerMultiplierPercent *= iAdjustment;
						iPlayerMultiplierPercent /= 100;
					}
				}
			}
			else if (iI == getOwnerINLINE())
			{
				iPlayerMultiplierPercent = 100;
			}
			else if (bHasHolyCity && GET_PLAYER((PlayerTypes)iI).getTeam() == getTeam())
			{
				iPlayerMultiplierPercent = 80;
			}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

			if (iPlayerMultiplierPercent > 0)
			{
				for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
				{

					if (AI_plotValid(pLoopCity->plot()) && pLoopCity->area() == area())
					{
						if (canSpread(pLoopCity->plot(), eReligion))
						{
							if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
							{
								if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_SPREAD, getGroup()) == 0)
								{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/03/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
									if (generatePath(pLoopCity->plot(), MOVE_NO_ENEMY_TERRITORY, true, &iPathTurns))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
									{
										iValue = (7 + (pLoopCity->getPopulation() * 4));

										bool bOurCity = false;
										// BBAI TODO: Why not just use iPlayerMultiplier??
										if (pLoopCity->getOwnerINLINE() == getOwnerINLINE())
										{
											iValue *= (bCultureVictory ? 16 : 4);
											bOurCity = true;
										}
										else if (pLoopCity->getTeam() == getTeam())
										{
											iValue *= 3;
											bOurCity = true;
										}
										else
										{
											iValue *= iPlayerMultiplierPercent;
											iValue /= 100;
										}

										int iCityReligionCount = pLoopCity->getReligionCount();
										int iReligionCountFactor = iCityReligionCount;

										if (bOurCity)
										{
											// count cities with no religion the same as cities with 2 religions
											// prefer a city with exactly 1 religion already
											if (iCityReligionCount == 0)
											{
												iReligionCountFactor = 2;
											}
											else if (iCityReligionCount == 1)
											{
												iValue *= 2;
											}
										}
										else
										{
											// absolutely prefer cities with zero religions
											if (iCityReligionCount == 0)
											{
												iValue *= 2;
											}

											// not our city, so prefer the lowest number of religions (increment so no divide by zero)
											iReligionCountFactor++;
										}

										iValue /= iReligionCountFactor;

										FAssert(iPathTurns > 0);

										bool bForceMove = false;
										if (isHuman())
										{
											//If human, prefer to spread to the player where automated from.
											if (plot()->getOwnerINLINE() == pLoopCity->getOwnerINLINE())
											{
												iValue *= 10;
												if (pLoopCity->isRevealed(getTeam(), false))
												{
													bForceMove = true;
												}
											}
										}

										iValue *= 1000;

										iValue /= (iPathTurns + 2);

										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											pBestPlot = bForceMove ? pLoopCity->plot() : getPathEndTurnPlot();
											pBestSpreadPlot = pLoopCity->plot();
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestSpreadPlot != NULL))
	{
		if (atPlot(pBestSpreadPlot))
		{
			getGroup()->pushMission(MISSION_SPREAD, eReligion);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/09/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_NO_ENEMY_TERRITORY, false, false, MISSIONAI_SPREAD, pBestSpreadPlot);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_spreadCorporation()
{
	PROFILE_FUNC();

	CorporationTypes eCorporation = NO_CORPORATION;

	for (int iI = 0; iI < GC.getNumCorporationInfos(); ++iI)
	{
		if (m_pUnitInfo->getCorporationSpreads((CorporationTypes)iI) > 0)
		{
			eCorporation = ((CorporationTypes)iI);
			break;
		}
	}

	if (NO_CORPORATION == eCorporation)
	{
		return false;
	}
/*************************************************************************************************/
/**	Xienwolf Tweak							03/20/09											**/
/**																								**/
/**										Firaxis Typo Fix										**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	bool bHasHQ = (GET_TEAM(getTeam()).hasHeadquarters((CorporationTypes)iI));
/**								----  End Original Code  ----									**/
	bool bHasHQ = (GET_TEAM(getTeam()).hasHeadquarters(eCorporation));
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	CvPlot* pBestSpreadPlot = NULL;

	CvTeam& kTeam = GET_TEAM(getTeam());
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/21/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
		//if (kLoopPlayer.isAlive() && (bHasHQ || (getTeam() == kLoopPlayer.getTeam())))
		if (kLoopPlayer.isAlive() && ((bHasHQ && canEnterTerritory(GET_PLAYER((PlayerTypes)iI).getTeam())) || (getTeam() == kLoopPlayer.getTeam())))
		{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			int iLoopPlayerCorpCount = kLoopPlayer.countCorporations(eCorporation);
			CvTeam& kLoopTeam = GET_TEAM(kLoopPlayer.getTeam());
			int iLoop;
			for (CvCity* pLoopCity = kLoopPlayer.firstCity(&iLoop); NULL != pLoopCity; pLoopCity = kLoopPlayer.nextCity(&iLoop))
			{
				if (AI_plotValid(pLoopCity->plot()))
				{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
					// BBAI efficiency: check same area
					if ( pLoopCity->area() == area() && canSpreadCorporation(pLoopCity->plot(), eCorporation))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
					{
						if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
						{
							if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_SPREAD_CORPORATION, getGroup()) == 0)
							{
								int iPathTurns;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/03/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
								if (generatePath(pLoopCity->plot(), MOVE_NO_ENEMY_TERRITORY, true, &iPathTurns))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
								{
									// BBAI TODO: Serious need for more intelligent self spread, keep certain corps from
									// enemies based on their victory pursuits (culture ...)
									int iValue = (10 + pLoopCity->getPopulation() * 2);

									if (pLoopCity->getOwnerINLINE() == getOwnerINLINE())
									{
										iValue *= 4;
									}
									else if (kLoopTeam.isVassal(getTeam()))
									{
										iValue *= 3;
									}
									else if (kTeam.isVassal(kLoopTeam.getID()))
									{
										if (iLoopPlayerCorpCount == 0)
										{
											iValue *= 10;
										}
										else
										{
											iValue *= 3;
											iValue /= 2;
										}
									}
									else if (pLoopCity->getTeam() == getTeam())
									{
										iValue *= 2;
									}

									if (iLoopPlayerCorpCount == 0)
									{
										//Generally prefer to heavily target one player
										iValue /= 2;
									}

									bool bForceMove = false;
									if (isHuman())
									{
										//If human, prefer to spread to the player where automated from.
										if (plot()->getOwnerINLINE() == pLoopCity->getOwnerINLINE())
										{
											iValue *= 10;
											if (pLoopCity->isRevealed(getTeam(), false))
											{
												bForceMove = true;
											}
										}
									}

									FAssert(iPathTurns > 0);

									iValue *= 1000;

									iValue /= (iPathTurns + 1);

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = bForceMove ? pLoopCity->plot() : getPathEndTurnPlot();
										pBestSpreadPlot = pLoopCity->plot();
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestSpreadPlot != NULL))
	{
		if (atPlot(pBestSpreadPlot))
		{
			if (canSpreadCorporation(pBestSpreadPlot, eCorporation))
			{
				getGroup()->pushMission(MISSION_SPREAD_CORPORATION, eCorporation);
			}
			else
			{
				getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_SPREAD_CORPORATION, pBestSpreadPlot);
			}
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/09/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_NO_ENEMY_TERRITORY, false, false, MISSIONAI_SPREAD_CORPORATION, pBestSpreadPlot);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			return true;
		}
	}

	return false;
}

bool CvUnitAI::AI_spreadReligionAirlift()
{
	PROFILE_FUNC();

	CvPlot* pBestPlot;
	ReligionTypes eReligion;
	int iValue;
	int iBestValue;
	int iI;

	if (getGroup()->getNumUnits() > 1)
	{
		return false;
	}

	CvCity* pCity = plot()->getPlotCity();

	if (pCity == NULL)
	{
		return false;
	}

	if (pCity->getMaxAirlift() == 0)
	{
		return false;
	}

	//bool bCultureVictory = GET_PLAYER(getOwnerINLINE()).AI_isDoStrategy(AI_STRATEGY_CULTURE2);
	eReligion = NO_RELIGION;

	if (eReligion == NO_RELIGION)
	{
		if (GET_PLAYER(getOwnerINLINE()).getStateReligion() != NO_RELIGION)
		{
			if (m_pUnitInfo->getReligionSpreads(GET_PLAYER(getOwnerINLINE()).getStateReligion()) > 0)
			{
				eReligion = GET_PLAYER(getOwnerINLINE()).getStateReligion();
			}
		}
	}

	if (eReligion == NO_RELIGION)
	{
		for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
		{
			//if (bCultureVictory || GET_TEAM(getTeam()).hasHolyCity((ReligionTypes)iI))
			{
				if (m_pUnitInfo->getReligionSpreads((ReligionTypes)iI) > 0)
				{
					eReligion = ((ReligionTypes)iI);
					break;
				}
			}
		}
	}

	if (eReligion == NO_RELIGION)
	{
		return false;
	}

	iBestValue = 0;
	pBestPlot = NULL;

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kLoopPlayer.isAlive() && (getTeam() == kLoopPlayer.getTeam()))
		{
			int iLoop;
			for (CvCity* pLoopCity = kLoopPlayer.firstCity(&iLoop); NULL != pLoopCity; pLoopCity = kLoopPlayer.nextCity(&iLoop))
			{
				if (canAirliftAt(pCity->plot(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE()))
				{
					if (canSpread(pLoopCity->plot(), eReligion))
					{
						if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_SPREAD, getGroup()) == 0)
						{
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       08/04/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
							// Don't airlift where there's already one of our unit types (probably just airlifted)
							if( pLoopCity->plot()->plotCount(PUF_isUnitType, getUnitType(), -1, getOwnerINLINE()) > 0 )
							{
								continue;
							}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
							iValue = (7 + (pLoopCity->getPopulation() * 4));

							int iCityReligionCount = pLoopCity->getReligionCount();
							int iReligionCountFactor = iCityReligionCount;

							// count cities with no religion the same as cities with 2 religions
							// prefer a city with exactly 1 religion already
							if (iCityReligionCount == 0)
							{
								iReligionCountFactor = 2;
							}
							else if (iCityReligionCount == 1)
							{
								iValue *= 2;
							}

							iValue /= iReligionCountFactor;
							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = pLoopCity->plot();
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_AIRLIFT, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_SPREAD, pBestPlot);
		return true;
	}

	return false;
}

bool CvUnitAI::AI_spreadCorporationAirlift()
{
	PROFILE_FUNC();

	if (getGroup()->getNumUnits() > 1)
	{
		return false;
	}

	CvCity* pCity = plot()->getPlotCity();

	if (pCity == NULL)
	{
		return false;
	}

	if (pCity->getMaxAirlift() == 0)
	{
		return false;
	}

	CorporationTypes eCorporation = NO_CORPORATION;

	for (int iI = 0; iI < GC.getNumCorporationInfos(); ++iI)
	{
		if (m_pUnitInfo->getCorporationSpreads((CorporationTypes)iI) > 0)
		{
			eCorporation = ((CorporationTypes)iI);
			break;
		}
	}

	if (NO_CORPORATION == eCorporation)
	{
		return false;
	}

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kLoopPlayer.isAlive() && (getTeam() == kLoopPlayer.getTeam()))
		{
			int iLoop;
			for (CvCity* pLoopCity = kLoopPlayer.firstCity(&iLoop); NULL != pLoopCity; pLoopCity = kLoopPlayer.nextCity(&iLoop))
			{
				if (canAirliftAt(pCity->plot(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE()))
				{
					if (canSpreadCorporation(pLoopCity->plot(), eCorporation))
					{
						if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_SPREAD_CORPORATION, getGroup()) == 0)
						{
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       08/04/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
							// Don't airlift where there's already one of our unit types (probably just airlifted)
							if( pLoopCity->plot()->plotCount(PUF_isUnitType, getUnitType(), -1, getOwnerINLINE()) > 0 )
							{
								continue;
							}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/

							int iValue = (pLoopCity->getPopulation() * 4);

							if (pLoopCity->getOwnerINLINE() == getOwnerINLINE())
							{
								iValue *= 4;
							}
							else
							{
								iValue *= 3;
							}

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = pLoopCity->plot();
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_AIRLIFT, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_SPREAD, pBestPlot);
		return true;
	}

	return false;
}

// Returns true if a mission was pushed...
bool CvUnitAI::AI_discover(bool bThisTurnOnly, bool bFirstResearchOnly)
{
	TechTypes eDiscoverTech;
	bool bIsFirstTech;
	int iPercentWasted = 0;

	if (canDiscover(plot()))
	{
		eDiscoverTech = getDiscoveryTech();
		bIsFirstTech = (GET_PLAYER(getOwnerINLINE()).AI_isFirstTech(eDiscoverTech));

		if (bFirstResearchOnly && !bIsFirstTech)
		{
			return false;
		}

		iPercentWasted = (100 - ((getDiscoverResearch(eDiscoverTech) * 100) / getDiscoverResearch(NO_TECH)));
		FAssert(((iPercentWasted >= 0) && (iPercentWasted <= 100)));


		if (getDiscoverResearch(eDiscoverTech) >= GET_TEAM(getTeam()).getResearchLeft(eDiscoverTech))
		{
			if ((iPercentWasted < 51) && bFirstResearchOnly && bIsFirstTech)
			{
				getGroup()->pushMission(MISSION_DISCOVER);
				return true;
			}

			if (iPercentWasted < (bIsFirstTech ? 31 : 11))
			{
				//I need a good way to assess if the tech is actually valuable...
				//but don't have one.
				getGroup()->pushMission(MISSION_DISCOVER);
				return true;
			}
		}
		else if (bThisTurnOnly)
		{
			return false;
		}

		if (iPercentWasted <= 11)
		{
			if (GET_PLAYER(getOwnerINLINE()).getCurrentResearch() == eDiscoverTech)
			{
				getGroup()->pushMission(MISSION_DISCOVER);
				return true;
			}
		}
	}
	return false;
}

// Returns true if a mission was pushed...
bool CvUnitAI::AI_lead(std::vector<UnitAITypes>& aeUnitAITypes)
{
	PROFILE_FUNC();

	FAssertMsg(!isHuman(), "isHuman did not return false as expected");
	FAssertMsg(AI_getUnitAIType() != NO_UNITAI, "AI_getUnitAIType() is not expected to be equal with NO_UNITAI");
	FAssert(NO_PLAYER != getOwnerINLINE());

	CvPlayer& kOwner = GET_PLAYER(getOwnerINLINE());

	bool bNeedLeader = false;
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		CvTeamAI& kLoopTeam = GET_TEAM((TeamTypes)iI);
		if (isEnemy((TeamTypes)iI))
		{
			if (kLoopTeam.countNumUnitsByArea(area()) > 0)
			{
				bNeedLeader = true;
				break;
			}
		}
	}

	CvUnit* pBestUnit = NULL;
	CvPlot* pBestPlot = NULL;

	// AI may use Warlords to create super-medic units
	CvUnit* pBestStrUnit = NULL;
	CvPlot* pBestStrPlot = NULL;

	CvUnit* pBestHealUnit = NULL;
	CvPlot* pBestHealPlot = NULL;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/14/10                                jdog5000      */
/*                                                                                              */
/* Great People AI, Unit AI                                                                     */
/************************************************************************************************/
	if (bNeedLeader)
	{
		int iBestStrength = 0;
		int iBestHealing = 0;
		int iLoop;
		for (CvUnit* pLoopUnit = kOwner.firstUnit(&iLoop); pLoopUnit; pLoopUnit = kOwner.nextUnit(&iLoop))
		{
			bool bValid = isWorldUnitClass(pLoopUnit->getUnitClassType());

			if( !bValid )
			{
				for (uint iI = 0; iI < aeUnitAITypes.size(); iI++)
				{
					if (pLoopUnit->AI_getUnitAIType() == aeUnitAITypes[iI] || NO_UNITAI == aeUnitAITypes[iI])
					{
						bValid = true;
						break;
					}
				}
			}

			if( bValid )
			{
				if (canLead(pLoopUnit->plot(), pLoopUnit->getID()) > 0)
				{
					if (AI_plotValid(pLoopUnit->plot()))
					{
						if (!(pLoopUnit->plot()->isVisibleEnemyUnit(this)))
						{
							if( pLoopUnit->combatLimit() == 100 )
							{
								if (generatePath(pLoopUnit->plot(), MOVE_AVOID_ENEMY_WEIGHT_3, true))
								{
									// pick the unit with the highest current strength
									int iCombatStrength = pLoopUnit->currCombatStr(NULL, NULL);

									iCombatStrength *= 30 + pLoopUnit->getExperience();
									iCombatStrength /= 30;

									if( GC.getUnitClassInfo(pLoopUnit->getUnitClassType()).getMaxGlobalInstances() > -1 )
									{
										iCombatStrength *= 1 + GC.getUnitClassInfo(pLoopUnit->getUnitClassType()).getMaxGlobalInstances();
										iCombatStrength /= std::max(1, GC.getUnitClassInfo(pLoopUnit->getUnitClassType()).getMaxGlobalInstances());
									}

									if (iCombatStrength > iBestStrength)
									{
										iBestStrength = iCombatStrength;
										pBestStrUnit = pLoopUnit;
										pBestStrPlot = getPathEndTurnPlot();
									}

									// or the unit with the best healing ability
									int iHealing = pLoopUnit->getSameTileHeal() + pLoopUnit->getAdjacentTileHeal();
									if (iHealing > iBestHealing)
									{
										iBestHealing = iHealing;
										pBestHealUnit = pLoopUnit;
										pBestHealPlot = getPathEndTurnPlot();
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if( AI_getBirthmark() % 3 == 0 && pBestHealUnit != NULL )
	{
		pBestPlot = pBestHealPlot;
		pBestUnit = pBestHealUnit;
	}
	else
	{
		pBestPlot = pBestStrPlot;
		pBestUnit = pBestStrUnit;
	}

	if (pBestPlot)
	{
		if (atPlot(pBestPlot) && pBestUnit)
		{
			getGroup()->pushMission(MISSION_LEAD, pBestUnit->getID());
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_AVOID_ENEMY_WEIGHT_3);
			return true;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	return false;
}

// Returns true if a mission was pushed...
// iMaxCounts = 1 would mean join a city if there's no existing joined GP of that type.
bool CvUnitAI::AI_join(int iMaxCount)
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	SpecialistTypes eBestSpecialist;
	int iValue;
	int iBestValue;
	int iLoop;
	int iI;
	int iCount;

	iBestValue = 0;
	pBestPlot = NULL;
	eBestSpecialist = NO_SPECIALIST;
	iCount = 0;

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
		// BBAI efficiency: check same area
		if ((pLoopCity->area() == area()) && AI_plotValid(pLoopCity->plot()))
		{
			if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
			{
				if (generatePath(pLoopCity->plot(), MOVE_SAFE_TERRITORY, true))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				{
					for (iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
					{
						bool bDoesJoin = false;
						SpecialistTypes eSpecialist = (SpecialistTypes)iI;
						if (m_pUnitInfo->getGreatPeoples(eSpecialist))
						{
							bDoesJoin = true;
						}
						if (bDoesJoin)
						{
							iCount += pLoopCity->getSpecialistCount(eSpecialist);
							if (iCount >= iMaxCount)
							{
								return false;
							}
						}

						if (canJoin(pLoopCity->plot(), ((SpecialistTypes)iI)))
						{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
							//if (GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(pLoopCity->plot(), 2) == 0)
							if ( !(GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(pLoopCity->plot(), 2)) )
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
							{
								iValue = pLoopCity->AI_specialistValue(((SpecialistTypes)iI), pLoopCity->AI_avoidGrowth(), false);
								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									pBestPlot = getPathEndTurnPlot();
									eBestSpecialist = ((SpecialistTypes)iI);
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (eBestSpecialist != NO_SPECIALIST))
	{
		if (atPlot(pBestPlot))
		{
			getGroup()->pushMission(MISSION_JOIN, eBestSpecialist);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/09/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_SAFE_TERRITORY);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			return true;
		}
	}

	return false;
}

// Returns true if a mission was pushed...
// iMaxCount = 1 would mean construct only if there are no existing buildings
//   constructed by this GP type.
bool CvUnitAI::AI_construct(int iMaxCount, int iMaxSingleBuildingCount, int iThreshold)
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	CvPlot* pBestConstructPlot;
	BuildingTypes eBestBuilding;
	int iValue;
	int iBestValue;
	int iLoop;
	int iI;
	int iCount;

	iBestValue = 0;
	pBestPlot = NULL;
	pBestConstructPlot = NULL;
	eBestBuilding = NO_BUILDING;
	iCount = 0;

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if (AI_plotValid(pLoopCity->plot()) && pLoopCity->area() == area())
		{
			if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
			{
				if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_CONSTRUCT, getGroup()) == 0)
				{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/03/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
					if (generatePath(pLoopCity->plot(), MOVE_NO_ENEMY_TERRITORY, true))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
					{
						for (iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
						{
							BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI);

							if (NO_BUILDING != eBuilding)
							{
								bool bDoesBuild = false;
								if ((m_pUnitInfo->getForceBuildings(eBuilding))
									|| (m_pUnitInfo->getBuildings(eBuilding)))
								{
									bDoesBuild = true;
								}

								if (bDoesBuild && (pLoopCity->getNumBuilding(eBuilding) > 0))
								{
									iCount++;
									if (iCount >= iMaxCount)
									{
/*************************************************************************************************/
/**	Bugfix									23/01/12									Snarko	**/
/**																								**/
/**			We might still be able to build something else... or somewhere else					**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
										return false;
/**								----  End Original Code  ----									**/
										continue;
/*************************************************************************************************/
/**	Bugfix END																					**/
/*************************************************************************************************/
									}
								}

								if (bDoesBuild && GET_PLAYER(getOwnerINLINE()).getBuildingClassCount((BuildingClassTypes)GC.getBuildingInfo(eBuilding).getBuildingClassType()) < iMaxSingleBuildingCount)
								{
									if (canConstruct(pLoopCity->plot(), eBuilding))
									{
										iValue = pLoopCity->AI_buildingValue(eBuilding);

										if ((iValue > iThreshold) && (iValue > iBestValue))
										{
											iBestValue = iValue;
											pBestPlot = getPathEndTurnPlot();
											pBestConstructPlot = pLoopCity->plot();
											eBestBuilding = eBuilding;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestConstructPlot != NULL) && (eBestBuilding != NO_BUILDING))
	{
		if (atPlot(pBestConstructPlot))
		{
			getGroup()->pushMission(MISSION_CONSTRUCT, eBestBuilding);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/09/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_NO_ENEMY_TERRITORY, false, false, MISSIONAI_CONSTRUCT, pBestConstructPlot);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_switchHurry()
{
	CvCity* pCity;
	BuildingTypes eBestBuilding;
	int iValue;
	int iBestValue;
	int iI;

	pCity = plot()->getPlotCity();

/*************************************************************************************************/
/**	Xienwolf Notes							NOTES												**/
/**																								**/
/**We need to modify this function to account for Soldiers of Kilmorph, don't rush if they have XP*/
/*************************************************************************************************/
	if ((pCity == NULL) || (pCity->getOwnerINLINE() != getOwnerINLINE()))
	{
		return false;
	}

	iBestValue = 0;
	eBestBuilding = NO_BUILDING;

	for (iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		if (isWorldWonderClass((BuildingClassTypes)iI))
		{
			BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI);

			if (NO_BUILDING != eBuilding)
			{
				if (pCity->canConstruct(eBuilding))
				{
					if (pCity->getBuildingProduction(eBuilding) == 0)
					{
						if (getMaxHurryProduction(pCity) >= pCity->getProductionNeeded(eBuilding))
						{
							iValue = pCity->AI_buildingValue(eBuilding);

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								eBestBuilding = eBuilding;
							}
						}
					}
				}
			}
		}
	}

	if (eBestBuilding != NO_BUILDING)
	{
		pCity->pushOrder(ORDER_CONSTRUCT, eBestBuilding, -1, false, false, false);

		if (pCity->getProductionBuilding() == eBestBuilding)
		{
			if (canHurry(plot()))
			{
				getGroup()->pushMission(MISSION_HURRY);
				return true;
			}
		}

		FAssert(false);
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_hurry()
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	CvPlot* pBestHurryPlot;
	bool bHurry;
	int iTurnsLeft;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iLoop;

	iBestValue = 0;
	pBestPlot = NULL;
	pBestHurryPlot = NULL;

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
		// BBAI efficiency: check same area
		if ((pLoopCity->area() == area()) && AI_plotValid(pLoopCity->plot()))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		{
			if (canHurry(pLoopCity->plot()))
			{
				if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
				{
					if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_HURRY, getGroup()) == 0)
					{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/03/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
						if (generatePath(pLoopCity->plot(), MOVE_NO_ENEMY_TERRITORY, true, &iPathTurns))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
						{
							bHurry = false;

							if (pLoopCity->isProductionBuilding())
							{
								if (isWorldWonderClass((BuildingClassTypes)(GC.getBuildingInfo(pLoopCity->getProductionBuilding()).getBuildingClassType())))
								{
									bHurry = true;
								}
							}

							if (bHurry)
							{
								iTurnsLeft = pLoopCity->getProductionTurnsLeft();

								iTurnsLeft -= iPathTurns;

								if (iTurnsLeft > 8)
								{
									iValue = iTurnsLeft;

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = getPathEndTurnPlot();
										pBestHurryPlot = pLoopCity->plot();
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestHurryPlot != NULL))
	{
		if (atPlot(pBestHurryPlot))
		{
			getGroup()->pushMission(MISSION_HURRY);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/09/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_NO_ENEMY_TERRITORY, false, false, MISSIONAI_HURRY, pBestHurryPlot);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_greatWork()
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	CvPlot* pBestGreatWorkPlot;
	int iValue;
	int iBestValue;
	int iLoop;

	iBestValue = 0;
	pBestPlot = NULL;
	pBestGreatWorkPlot = NULL;

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
		// BBAI efficiency: check same area
		if ((pLoopCity->area() == area()) && AI_plotValid(pLoopCity->plot()))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		{
			if (canGreatWork(pLoopCity->plot()))
			{
				if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
				{
					if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_GREAT_WORK, getGroup()) == 0)
					{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/03/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
						if (generatePath(pLoopCity->plot(), MOVE_NO_ENEMY_TERRITORY, true))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
						{
							iValue = pLoopCity->AI_calculateCulturePressure(true);
							iValue -= ((100 * pLoopCity->getCulture(pLoopCity->getOwnerINLINE())) / std::max(1, getGreatWorkCulture(pLoopCity->plot())));
							if (iValue > 0)
							{
								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									pBestPlot = getPathEndTurnPlot();
									pBestGreatWorkPlot = pLoopCity->plot();
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestGreatWorkPlot != NULL))
	{
		if (atPlot(pBestGreatWorkPlot))
		{
			getGroup()->pushMission(MISSION_GREAT_WORK);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/09/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_NO_ENEMY_TERRITORY, false, false, MISSIONAI_GREAT_WORK, pBestGreatWorkPlot);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_offensiveAirlift()
{
	PROFILE_FUNC();

	CvCity* pCity;
	CvCity* pTargetCity;
	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	int iValue;
	int iBestValue;
	int iLoop;

	if (getGroup()->getNumUnits() > 1)
	{
		return false;
	}

	if (area()->getTargetCity(getOwnerINLINE()) != NULL)
	{
		return false;
	}

	pCity = plot()->getPlotCity();

	if (pCity == NULL)
	{
		return false;
	}

	if (pCity->getMaxAirlift() == 0)
	{
		return false;
	}

	iBestValue = 0;
	pBestPlot = NULL;

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if (pLoopCity->area() != pCity->area())
		{
			if (canAirliftAt(pCity->plot(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE()))
			{
				pTargetCity = pLoopCity->area()->getTargetCity(getOwnerINLINE());

				if (pTargetCity != NULL)
				{
					AreaAITypes eAreaAIType = pTargetCity->area()->getAreaAIType(getTeam());
					if (((eAreaAIType == AREAAI_OFFENSIVE) || (eAreaAIType == AREAAI_DEFENSIVE) || (eAreaAIType == AREAAI_MASSING))
						|| pTargetCity->AI_isDanger())
					{
						iValue = 10000;

						iValue *= (GET_PLAYER(getOwnerINLINE()).AI_militaryWeight(pLoopCity->area()) + 10);
						iValue /= (GET_PLAYER(getOwnerINLINE()).AI_totalAreaUnitAIs(pLoopCity->area(), AI_getUnitAIType()) + 10);

						iValue += std::max(1, ((GC.getMapINLINE().maxStepDistance() * 2) - GC.getMapINLINE().calculatePathDistance(pLoopCity->plot(), pTargetCity->plot())));

						if (AI_getUnitAIType() == UNITAI_PARADROP)
						{
							CvCity* pNearestEnemyCity = GC.getMapINLINE().findCity(pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(), NO_PLAYER, NO_TEAM, false, false, getTeam());

							if (pNearestEnemyCity != NULL)
							{
								int iDistance = plotDistance(pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(), pNearestEnemyCity->getX_INLINE(), pNearestEnemyCity->getY_INLINE());
								if (iDistance <= getDropRange())
								{
									iValue *= 5;
								}
							}
						}

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = pLoopCity->plot();
							FAssert(pLoopCity != pCity);
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_AIRLIFT, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_paradrop(int iRange)
{
	PROFILE_FUNC();

	if (getGroup()->getNumUnits() > 1)
	{
		return false;
	}
	int iParatrooperCount = plot()->plotCount(PUF_isUnitAIType, UNITAI_PARADROP, -1, getOwnerINLINE());
	FAssert(iParatrooperCount > 0);

	CvPlot* pPlot = plot();

	if (!canParadrop(pPlot))
	{
		return false;
	}

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;

	int iSearchRange = AI_searchRange(iRange);

	for (int iDX = -iSearchRange; iDX <= iSearchRange; ++iDX)
	{
		for (int iDY = -iSearchRange; iDY <= iSearchRange; ++iDY)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (isPotentialEnemy(pLoopPlot->getTeam(), pLoopPlot))
				{
					if (canParadropAt(pPlot, pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()))
					{
						int iValue = 0;

						PlayerTypes eTargetPlayer = pLoopPlot->getOwnerINLINE();
						FAssert(NO_PLAYER != eTargetPlayer);
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       08/01/08                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original BTS code
						if (NO_BONUS != pLoopPlot->getBonusType())
						{
							iValue += GET_PLAYER(eTargetPlayer).AI_bonusVal(pLoopPlot->getBonusType()) - 10;
						}
*/
						// Bonus values for bonuses the AI has are less than 10 for non-strategic resources... since this is
						// in the AI territory they probably have it
						if (NO_BONUS != pLoopPlot->getNonObsoleteBonusType(getTeam()))
						{
							iValue += std::max(1,GET_PLAYER(eTargetPlayer).AI_bonusVal(pLoopPlot->getBonusType()) - 10);
						}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/

						for (int i = -1; i <= 1; ++i)
						{
							for (int j = -1; j <= 1; ++j)
							{
								CvPlot* pAdjacentPlot = plotXY(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), i, j);
								if (NULL != pAdjacentPlot)
								{
									CvCity* pAdjacentCity = pAdjacentPlot->getPlotCity();

									if (NULL != pAdjacentCity)
									{
										if (pAdjacentCity->getOwnerINLINE() == eTargetPlayer)
										{
											int iAttackerCount = GET_PLAYER(getOwnerINLINE()).AI_adjacentPotentialAttackers(pAdjacentPlot, true);
											int iDefenderCount = pAdjacentPlot->getNumVisibleEnemyDefenders(this);
											iValue += 20 * (AI_attackOdds(pAdjacentPlot, true) - ((50 * iDefenderCount) / (iParatrooperCount + iAttackerCount)));
										}
									}
								}
							}
						}

						if (iValue > 0)
						{
							iValue += pLoopPlot->defenseModifier(getTeam(), ignoreBuildingDefense());

							CvUnit* pInterceptor = bestInterceptor(pLoopPlot);
							if (NULL != pInterceptor)
							{
								int iInterceptProb = isSuicide() ? 100 : pInterceptor->currInterceptionProbability();

								iInterceptProb *= std::max(0, (100 - evasionProbability()));
								iInterceptProb /= 100;

								iValue *= std::max(0, 100 - iInterceptProb / 2);
								iValue /= 100;
							}
						}

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = pLoopPlot;

							FAssert(pBestPlot != pPlot);
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_PARADROP, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}


/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/01/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
// Returns true if a mission was pushed...
bool CvUnitAI::AI_protect(int iOddsThreshold, int iMaxPathTurns)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	int iValue;
	int iBestValue;
	int iI;

	iBestValue = 0;
	pBestPlot = NULL;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (pLoopPlot->getOwnerINLINE() == getOwnerINLINE())
		{
			if (AI_plotValid(pLoopPlot))
			{
				if (pLoopPlot->isVisibleEnemyUnit(this))
				{
					if (!atPlot(pLoopPlot))
					{
						// BBAI efficiency: Check area for land units
						if( (getDomainType() != DOMAIN_LAND) || (pLoopPlot->area() == area()) || getGroup()->canMoveAllTerrain() )
						{
							// BBAI efficiency: Most of the time, path will exist and odds will be checked anyway.  When path doesn't exist, checking path
							// takes longer.  Therefore, check odds first.
							iValue = getGroup()->AI_attackOdds(pLoopPlot, true);

							if ((iValue >= AI_finalOddsThreshold(pLoopPlot, iOddsThreshold)) && (iValue*50 > iBestValue))
							{
								int iPathTurns;
								if( generatePath(pLoopPlot, 0, true, &iPathTurns, iMaxPathTurns) )
								{
									// BBAI TODO: Other units targeting this already (if path turns > 1 or 0)?
									if( iPathTurns <= iMaxPathTurns )
									{
										iValue *= 100;

										iValue /= (2 + iPathTurns);

										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											pBestPlot = getPathEndTurnPlot();
											FAssert(!atPlot(pBestPlot));
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


// Returns true if a mission was pushed...
bool CvUnitAI::AI_patrol()
{
	PROFILE_FUNC();

	CvPlot* pAdjacentPlot;
	CvPlot* pBestPlot;
	int iValue;
	int iBestValue;
	int iI;

	iBestValue = 0;
	pBestPlot = NULL;

	for (iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		pAdjacentPlot = plotDirection(getX_INLINE(), getY_INLINE(), ((DirectionTypes)iI));

		if (pAdjacentPlot != NULL)
		{
			if (AI_plotValid(pAdjacentPlot))
			{
				if (!(pAdjacentPlot->isVisibleEnemyUnit(this)))
				{
					if (generatePath(pAdjacentPlot, 0, true))
					{
/*************************************************************************************************/
/**	Xienwolf Tweak							12/13/08											**/
/**																								**/
/**					Reduction in massive Random Spam in Logger files by using Map				**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
						iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Patrol"));
/**								----  End Original Code  ----									**/
						iValue = (1 + GC.getGameINLINE().getMapRandNum(10000, "AI Patrol"));
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

						if (isBarbarian())
						{
							if (!(pAdjacentPlot->isOwned()))
							{
								iValue += 20000;
							}

							if (!(pAdjacentPlot->isAdjacentOwned()))
							{
								iValue += 10000;
							}
						}
						else
						{
							if (pAdjacentPlot->isRevealedGoody(getTeam()))
							{
/*************************************************************************************************/
/**	Xienwolf Tweak							02/01/09											**/
/**																								**/
/**			Gets AI and Automated Exploration units to seek Lairs, but not TOO heavily			**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
								iValue += 100000;
/**								----  End Original Code  ----									**/
								if (pAdjacentPlot->isGoody())
								{
									iValue += 100000;
								}
								else
								{
									iValue += 5000;
								}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
							}

							if (pAdjacentPlot->getOwnerINLINE() == getOwnerINLINE())
							{
								iValue += 10000;
							}
						}

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = getPathEndTurnPlot();
							FAssert(!atPlot(pBestPlot));
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_defend()
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	int iSearchRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iDX, iDY;

	if (AI_defendPlot(plot()))
	{
		getGroup()->pushMission(MISSION_SKIP);
		return true;
	}

	iSearchRange = AI_searchRange(1);

	iBestValue = 0;
	pBestPlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot	= plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (AI_plotValid(pLoopPlot))
				{
					if (AI_defendPlot(pLoopPlot))
					{
						if (!(pLoopPlot->isVisibleEnemyUnit(this)))
						{
							if (!atPlot(pLoopPlot) && generatePath(pLoopPlot, 0, true, &iPathTurns, 1))
							{
								if (iPathTurns <= 1)
								{
									iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Defend"));

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = pLoopPlot;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/06/08                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
		if( !(pBestPlot->isCity()) && (getGroup()->getNumUnits() > 1) )
		{
			getGroup()->AI_makeForceSeparate();
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_safety()
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvUnit* pHeadUnit;
	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	int iSearchRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iCount;
	int iPass;
	int iDX, iDY;

	iSearchRange = AI_searchRange(1);

	iBestValue = 0;
	pBestPlot = NULL;

/*************************************************************************************************/
/**	TipToe									04/04/09								Xienwolf	**/
/**																								**/
/**						Safe Movement is ideal situation where possible							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	for (iPass = 0; iPass < 2; iPass++)
	{
		for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
		{
			for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
			{
				pLoopPlot	= plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

				if (pLoopPlot != NULL)
				{
					if (AI_plotValid(pLoopPlot))
					{
						if (!(pLoopPlot->isVisibleEnemyUnit(this)))
						{
							if (generatePath(pLoopPlot, ((iPass > 0) ? MOVE_IGNORE_DANGER : 0), true, &iPathTurns))
/**								----  End Original Code  ----									**/
	for (iPass = 0; iPass < 3; iPass++)
	{
		for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
		{
			for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
			{
				pLoopPlot	= plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

				if (pLoopPlot != NULL)
				{
					if (AI_plotValid(pLoopPlot))
					{
						if (!(pLoopPlot->isVisibleEnemyUnit(this)))
						{
							if (generatePath(pLoopPlot, ((iPass == 2) ? MOVE_IGNORE_DANGER : (iPass == 1 ? MOVE_SAFE_TERRITORY : 0)), true, &iPathTurns, 1))
/*************************************************************************************************/
/**	TipToe									END													**/
/*************************************************************************************************/
							{
								if (iPathTurns <= 1)
								{
									iCount = 0;

									pUnitNode = pLoopPlot->headUnitNode();

									while (pUnitNode != NULL)
									{
										pLoopUnit = ::getUnit(pUnitNode->m_data);
										pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

										if (pLoopUnit->getOwnerINLINE() == getOwnerINLINE())
										{
											if (pLoopUnit->canDefend())
											{
												pHeadUnit = pLoopUnit->getGroup()->getHeadUnit();
												FAssert(pHeadUnit != NULL);
												FAssert(getGroup()->getHeadUnit() == this);

												if (pHeadUnit != this)
												{
													if (pHeadUnit->isWaiting() || !(pHeadUnit->canMove()))
													{
														FAssert(pLoopUnit != this);
														FAssert(pHeadUnit != getGroup()->getHeadUnit());
														iCount++;
													}
												}
											}
										}
									}

									iValue = (iCount * 100);

									iValue += pLoopPlot->defenseModifier(getTeam(), false);

									if (atPlot(pLoopPlot))
									{
										iValue += 50;
									}
									else
									{
										iValue += GC.getGameINLINE().getSorenRandNum(50, "AI Safety");
									}

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = pLoopPlot;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		if (atPlot(pBestPlot))
		{
			getGroup()->pushMission(MISSION_SKIP);
			return true;
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), ((iPass > 0) ? MOVE_IGNORE_DANGER : 0));
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_hide()
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvUnit* pHeadUnit;
	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	bool bValid;
	int iSearchRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iCount;
	int iDX, iDY;
	int iI;

/*************************************************************************************************/
/**	CandyMan								04/04/09								Xienwolf	**/
/**																								**/
/**							Allows Multiple Invisible types on a Unit							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	if (getInvisibleType() == NO_INVISIBLE)
/**								----  End Original Code  ----									**/
	if (getNumInvisibleTypes() == 0)
/*************************************************************************************************/
/**	CandyMan								END													**/
/*************************************************************************************************/
	{
		return false;
	}

	iSearchRange = AI_searchRange(1);

	iBestValue = 0;
	pBestPlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot	= plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (AI_plotValid(pLoopPlot))
				{
					bValid = true;

					for (iI = 0; iI < MAX_TEAMS; iI++)
					{
						if (GET_TEAM((TeamTypes)iI).isAlive())
						{
/*************************************************************************************************/
/**	CandyMan								04/04/09								Xienwolf	**/
/**																								**/
/**							Allows Multiple Invisible types on a Unit							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
							if (pLoopPlot->isInvisibleVisible(((TeamTypes)iI), getInvisibleType()))
							{
								bValid = false;
								break;
							}
/**								----  End Original Code  ----									**/
							bool bHidden = false;
							for (int iG = 0; iG < getNumInvisibleTypes(); iG++)
							{
								if (!pLoopPlot->isInvisibleVisible(((TeamTypes)iI), getInvisibleType(iG)))
								{
									bHidden = true;
								}
							}
							if (!bHidden)
							{
								bValid = false;
								break;
							}
/*************************************************************************************************/
/**	CandyMan								END													**/
/*************************************************************************************************/
						}
					}

					if (bValid)
					{
						if (!(pLoopPlot->isVisibleEnemyUnit(this)))
						{
							if (generatePath(pLoopPlot, 0, true, &iPathTurns, 1))
							{
								if (iPathTurns <= 1)
								{
									iCount = 1;

									pUnitNode = pLoopPlot->headUnitNode();

									while (pUnitNode != NULL)
									{
										pLoopUnit = ::getUnit(pUnitNode->m_data);
										pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

										if (pLoopUnit->getOwnerINLINE() == getOwnerINLINE())
										{
											if (pLoopUnit->canDefend())
											{
												pHeadUnit = pLoopUnit->getGroup()->getHeadUnit();
												FAssert(pHeadUnit != NULL);
												FAssert(getGroup()->getHeadUnit() == this);

												if (pHeadUnit != this)
												{
													if (pHeadUnit->isWaiting() || !(pHeadUnit->canMove()))
													{
														FAssert(pLoopUnit != this);
														FAssert(pHeadUnit != getGroup()->getHeadUnit());
														iCount++;
													}
												}
											}
										}
									}

									iValue = (iCount * 100);

									iValue += pLoopPlot->defenseModifier(getTeam(), false);

									if (atPlot(pLoopPlot))
									{
										iValue += 50;
									}
									else
									{
										iValue += GC.getGameINLINE().getSorenRandNum(50, "AI Hide");
									}

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = pLoopPlot;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		if (atPlot(pBestPlot))
		{
			getGroup()->pushMission(MISSION_SKIP);
			return true;
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_goody(int iRange)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
//	CvPlot* pAdjacentPlot;
	CvPlot* pBestPlot;
	int iSearchRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iDX, iDY;
//	int iI;

	if (isBarbarian())
	{
		return false;
	}

	iSearchRange = AI_searchRange(iRange);

	iBestValue = 0;
	pBestPlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot	= plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (AI_plotValid(pLoopPlot))
				{
					if (pLoopPlot->isRevealedGoody(getTeam()))
					{
						if (!(pLoopPlot->isVisibleEnemyUnit(this)))
						{
							if (!atPlot(pLoopPlot) && generatePath(pLoopPlot, 0, true, &iPathTurns, iRange))
							{
								if (iPathTurns <= iRange)
								{
									iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Goody"));

									iValue /= (iPathTurns + 1);

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = getPathEndTurnPlot();
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_explore()
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pAdjacentPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestExplorePlot;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iI, iJ;

	iBestValue = 0;
	pBestPlot = NULL;
	pBestExplorePlot = NULL;

	bool bNoContact = (GC.getGameINLINE().countCivTeamsAlive() > GET_TEAM(getTeam()).getHasMetCivCount(true));

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		PROFILE("AI_explore 1");

		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (AI_plotValid(pLoopPlot))
		{
			iValue = 0;

			if (pLoopPlot->isRevealedGoody(getTeam()))
			{
				iValue += 100000;
			}

/*************************************************************************************************/
/**	Xienwolf Tweak							12/13/08											**/
/**																								**/
/**					Reduction in massive Random Spam in Logger files by using Map				**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
			if (iValue > 0 || GC.getGameINLINE().getSorenRandNum(4, "AI make explore faster ;)") == 0)
/**								----  End Original Code  ----									**/
			if (iValue > 0 || GC.getGameINLINE().getMapRandNum(4, "AI make explore faster ;)") == 0)
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
			{
				if (!(pLoopPlot->isRevealed(getTeam(), false)))
				{
					iValue += 10000;
				}
				// XXX is this too slow?
				for (iJ = 0; iJ < NUM_DIRECTION_TYPES; iJ++)
				{
					PROFILE("AI_explore 2");

					pAdjacentPlot = plotDirection(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), ((DirectionTypes)iJ));

					if (pAdjacentPlot != NULL)
					{
						if (!(pAdjacentPlot->isRevealed(getTeam(), false)))
						{
							iValue += 1000;
						}
						else if (bNoContact)
						{
							if (pAdjacentPlot->getRevealedTeam(getTeam(), false) != pAdjacentPlot->getTeam())
							{
								iValue += 100;
							}
						}
					}
				}

				if (iValue > 0)
				{
					if (!(pLoopPlot->isVisibleEnemyUnit(this)))
					{
						if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_EXPLORE, getGroup(), 3) == 0)
						{
							if (!atPlot(pLoopPlot) && generatePath(pLoopPlot, MOVE_NO_ENEMY_TERRITORY, true, &iPathTurns))
							{
/*************************************************************************************************/
/**	Xienwolf Tweak							12/13/08											**/
/**																								**/
/**					Reduction in massive Random Spam in Logger files by using Map				**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
								iValue += GC.getGameINLINE().getSorenRandNum(250 * abs(xDistance(getX_INLINE(), pLoopPlot->getX_INLINE())) + abs(yDistance(getY_INLINE(), pLoopPlot->getY_INLINE())), "AI explore");
/**								----  End Original Code  ----									**/
								iValue += GC.getGameINLINE().getMapRandNum(250 * abs(xDistance(getX_INLINE(), pLoopPlot->getX_INLINE())) + abs(yDistance(getY_INLINE(), pLoopPlot->getY_INLINE())), "AI explore");
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

								if (pLoopPlot->isAdjacentToLand())
								{
									iValue += 10000;
								}

								if (pLoopPlot->isOwned())
								{
									iValue += 5000;
								}

								iValue /= 3 + std::max(1, iPathTurns);

								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									pBestPlot = pLoopPlot->isRevealedGoody(getTeam()) ? getPathEndTurnPlot() : pLoopPlot;
									pBestExplorePlot = pLoopPlot;
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestExplorePlot != NULL))
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_NO_ENEMY_TERRITORY, false, false, MISSIONAI_EXPLORE, pBestExplorePlot);
		return true;
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_exploreRange(int iRange)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pAdjacentPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestExplorePlot;
	int iSearchRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iDX, iDY;
	int iI;
/*************************************************************************************************/
/**	Tweak							26/01/12								Snarko				**/
/**																								**/
/**	Making explorers explore near their own territory, because anything else is too dangerous	**/
/*************************************************************************************************/
	CvCity* pNearestCity = NULL;
/*************************************************************************************************/
/**	Tweak								END														**/
/*************************************************************************************************/

	iSearchRange = AI_searchRange(iRange);

	iBestValue = 0;
	pBestPlot = NULL;
	pBestExplorePlot = NULL;

	int iImpassableCount = GET_PLAYER(getOwnerINLINE()).AI_unitImpassableCount(getUnitType());

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			PROFILE("AI_exploreRange 1");

			pLoopPlot	= plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (AI_plotValid(pLoopPlot))
				{
					iValue = 0;

					if (pLoopPlot->isRevealedGoody(getTeam()))
					{
						iValue += 100000;
					}

					if (!(pLoopPlot->isRevealed(getTeam(), false)))
					{
						iValue += 10000;
					}

					for (iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
					{
						PROFILE("AI_exploreRange 2");

						pAdjacentPlot = plotDirection(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), ((DirectionTypes)iI));

						if (pAdjacentPlot != NULL)
						{
							if (!(pAdjacentPlot->isRevealed(getTeam(), false)))
							{
								iValue += 1000;
							}
						}
					}

					if (iValue > 0)
					{
						if (!(pLoopPlot->isVisibleEnemyUnit(this)))
						{
							PROFILE("AI_exploreRange 3");

							if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_EXPLORE, getGroup(), 3) == 0)
							{
								PROFILE("AI_exploreRange 4");

								if (!atPlot(pLoopPlot) && generatePath(pLoopPlot, MOVE_NO_ENEMY_TERRITORY, true, &iPathTurns, iRange))
								{
									if (iPathTurns <= iRange)
									{
/*************************************************************************************************/
/**	Xienwolf Tweak							12/13/08											**/
/**																								**/
/**					Reduction in massive Random Spam in Logger files by using Map				**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
										iValue += GC.getGameINLINE().getSorenRandNum(10000, "AI Explore");
/**								----  End Original Code  ----									**/
										iValue += GC.getGameINLINE().getMapRandNum(10000, "AI Explore");
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

										if (pLoopPlot->isAdjacentToLand())
										{
											iValue += 10000;
										}

										if (pLoopPlot->isOwned())
										{
											iValue += 5000;
										}
/*************************************************************************************************/
/**	Tweak							26/01/12								Snarko				**/
/**																								**/
/**	Making explorers explore near their own territory, because anything else is too dangerous	**/
/*************************************************************************************************/
										pNearestCity = GC.getMapINLINE().findCity(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), getOwnerINLINE(), NO_TEAM, true);
										if (pNearestCity != NULL)
										{
											iValue += std::max(0, 10000 - 200 * plotDistance(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), pNearestCity->getX_INLINE(), pNearestCity->getY_INLINE()));
										}
										iValue += std::max(0, 1000 - 20 * iPathTurns);
/*************************************************************************************************/
/**	Tweak								END														**/
/*************************************************************************************************/

										if (!isHuman())
										{
											int iDirectionModifier = 100;

											if (AI_getUnitAIType() == UNITAI_EXPLORE_SEA && iImpassableCount == 0)
											{
												iDirectionModifier += (50 * (abs(iDX) + abs(iDY))) / iSearchRange;
												if (GC.getGame().circumnavigationAvailable())
												{
													if (GC.getMap().isWrapX())
													{
														if ((iDX * ((AI_getBirthmark() % 2 == 0) ? 1 : -1)) > 0)
														{
															iDirectionModifier *= 150 + ((iDX * 100) / iSearchRange);
														}
														else
														{
															iDirectionModifier /= 2;
														}
													}
													if (GC.getMap().isWrapY())
													{
														if ((iDY * (((AI_getBirthmark() >> 1) % 2 == 0) ? 1 : -1)) > 0)
														{
															iDirectionModifier *= 150 + ((iDY * 100) / iSearchRange);
														}
														else
														{
															iDirectionModifier /= 2;
														}
													}
												}
												iValue *= iDirectionModifier;
												iValue /= 100;
											}
										}

										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											if (getDomainType() == DOMAIN_LAND)
											{
												pBestPlot = getPathEndTurnPlot();
											}
											else
											{
												pBestPlot = pLoopPlot;
											}
											pBestExplorePlot = pLoopPlot;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestExplorePlot != NULL))
	{
		PROFILE("AI_exploreRange 5");

		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_NO_ENEMY_TERRITORY, false, false, MISSIONAI_EXPLORE, pBestExplorePlot);
		return true;
	}

	return false;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/29/10                                jdog5000      */
/*                                                                                              */
/* War tactics AI, Efficiency                                                                   */
/************************************************************************************************/
// Returns target city
CvCity* CvUnitAI::AI_pickTargetCity(int iFlags, int iMaxPathTurns, bool bHuntBarbs )
{
	PROFILE_FUNC();

	CvCity* pTargetCity;
	CvCity* pLoopCity;
	CvCity* pBestCity;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iLoop;
	int iI;

	iBestValue = 0;
	pBestCity = NULL;

	pTargetCity = area()->getTargetCity(getOwnerINLINE());

	// Don't always go after area target ... don't know how far away it is
	/*
	if (pTargetCity != NULL)
	{
		if (AI_potentialEnemy(pTargetCity->getTeam(), pTargetCity->plot()))
		{
			if (!atPlot(pTargetCity->plot()) && generatePath(pTargetCity->plot(), iFlags, true))
			{
				pBestCity = pTargetCity;
			}
		}
	}
	*/

	if (pBestCity == NULL)
	{
		for (iI = 0; iI < (bHuntBarbs ? MAX_PLAYERS : MAX_CIV_PLAYERS); iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive() && ::isPotentialEnemy(getTeam(), GET_PLAYER((PlayerTypes)iI).getTeam()))
			{
				for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
				{
					// BBAI efficiency: check area for land units before generating path
					if (AI_plotValid(pLoopCity->plot()) && (pLoopCity->area() == area()))
					{
						if(AI_potentialEnemy(GET_PLAYER((PlayerTypes)iI).getTeam(), pLoopCity->plot()))
						{
							if (!atPlot(pLoopCity->plot()) && generatePath(pLoopCity->plot(), iFlags, true, &iPathTurns, iMaxPathTurns))
							{
								if( iPathTurns <= iMaxPathTurns )
								{
									// If city is visible and our force already in position is dominantly powerful or we have a huge force
									// already on the way, pick a different target
									if( iPathTurns > 2 && pLoopCity->isVisible(getTeam(), false) )
									{
										/*
										int iOurOffense = GET_TEAM(getTeam()).AI_getOurPlotStrength(pLoopCity->plot(),2,false,false,true);
										int iEnemyDefense = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(pLoopCity->plot(),1,true,false);

										if( 100*iOurOffense >= GC.getBBAI_SKIP_BOMBARD_BASE_STACK_RATIO()*iEnemyDefense )
										{
											continue;
										}
										*/

										if( GET_PLAYER(getOwnerINLINE()).AI_cityTargetUnitsByPath(pLoopCity, getGroup(), iPathTurns) > std::max( 6, 3 * pLoopCity->plot()->getNumVisibleEnemyDefenders(this) ) )
										{
											continue;
										}
									}

									iValue = 0;
									if (AI_getUnitAIType() == UNITAI_ATTACK_CITY) //lemming?
									{
										iValue = GET_PLAYER(getOwnerINLINE()).AI_targetCityValue(pLoopCity, false, false);
									}
									else
									{
										iValue = GET_PLAYER(getOwnerINLINE()).AI_targetCityValue(pLoopCity, true, true);
									}

									if( pLoopCity == pTargetCity )
									{
										iValue *= 2;
									}

									if ((area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE))
									{
										iValue *= 50 + pLoopCity->calculateCulturePercent(getOwnerINLINE());
										iValue /= 50;
									}

									iValue *= 1000;

									// If city is minor civ, less interesting
									if( GET_PLAYER(pLoopCity->getOwnerINLINE()).isMinorCiv() || GET_PLAYER(pLoopCity->getOwnerINLINE()).isBarbarian() )
									{
										iValue /= 2;
									}

									// If stack has poor bombard, direct towards lower defense cities
									iPathTurns += std::min(12, getGroup()->getBombardTurns(pLoopCity)/4);

									iValue /= (4 + iPathTurns*iPathTurns);

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestCity = pLoopCity;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return pBestCity;
}

// Returns true if a mission was pushed...
bool CvUnitAI::AI_goToTargetCity(int iFlags, int iMaxPathTurns, CvCity* pTargetCity )
{
	PROFILE_FUNC();

	CvPlot* pAdjacentPlot;
	CvPlot* pBestPlot;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iI;

	if( pTargetCity == NULL )
	{
		pTargetCity = AI_pickTargetCity(iFlags, iMaxPathTurns);
	}

	if (pTargetCity != NULL)
	{
		PROFILE("CvUnitAI::AI_targetCity plot attack");
		iBestValue = 0;
		pBestPlot = NULL;

		if (0 == (iFlags & MOVE_THROUGH_ENEMY))
		{
			for (iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
			{
				pAdjacentPlot = plotDirection(pTargetCity->getX_INLINE(), pTargetCity->getY_INLINE(), ((DirectionTypes)iI));

				if (pAdjacentPlot != NULL)
				{
					if (AI_plotValid(pAdjacentPlot))
					{
						if (!(pAdjacentPlot->isVisibleEnemyUnit(this)))
						{
							if (generatePath(pAdjacentPlot, iFlags, true, &iPathTurns, iMaxPathTurns))
							{
								if( iPathTurns <= iMaxPathTurns )
								{
									iValue = std::max(0, (pAdjacentPlot->defenseModifier(getTeam(), false) + 100));

									if (!(pAdjacentPlot->isRiverCrossing(directionXY(pAdjacentPlot, pTargetCity->plot()))))
									{
										iValue += (12 * -(GC.getRIVER_ATTACK_MODIFIER()));
									}

									if (!isEnemy(pAdjacentPlot->getTeam(), pAdjacentPlot))
									{
										iValue += 100;
									}

									if( atPlot(pAdjacentPlot) )
									{
										iValue += 50;
									}

									iValue = std::max(1, iValue);

									iValue *= 1000;

									iValue /= (iPathTurns + 1);

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = getPathEndTurnPlot();
									}
								}
							}
						}
					}
				}
			}
		}
		else
		{
			pBestPlot =  pTargetCity->plot();
		}

		if (pBestPlot != NULL)
		{
			FAssert(!(pTargetCity->at(pBestPlot)) || 0 != (iFlags & MOVE_THROUGH_ENEMY)); // no suicide missions...
			if (!atPlot(pBestPlot))
			{
				getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), iFlags);
				return true;
			}
		}
	}

	return false;
}

// Returns true if a mission was pushed...
bool CvUnitAI::AI_goToTargetBarbCity(int iMaxPathTurns)
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	CvCity* pBestCity;
	CvPlot* pAdjacentPlot;
	CvPlot* pBestPlot;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iLoop;
	int iI;

/*************************************************************************************************/
/**	MultiBarb							12/23/08									Xienwolf	**/
/**																								**/
/**							Adds extra Barbarian Civilizations									**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	if (isBarbarian())
	{
		return false;
	}

	iBestValue = 0;
	pBestCity = NULL;

	for (pLoopCity = GET_PLAYER(BARBARIAN_PLAYER).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(BARBARIAN_PLAYER).nextCity(&iLoop))
	{
		if (AI_plotValid(pLoopCity->plot()))
		{
			if (pLoopCity->isRevealed(getTeam(), false))
			{
				if (!atPlot(pLoopCity->plot()) && generatePath(pLoopCity->plot(), 0, true, &iPathTurns))
				{
					if (iPathTurns < 10)
					{
						iValue = GET_PLAYER(getOwnerINLINE()).AI_targetCityValue(pLoopCity, false);

						iValue *= 1000;

						iValue /= (iPathTurns + 1);

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestCity = pLoopCity;
						}
					}
				}
			}
		}
	}
/**								----  End Original Code  ----									**/
	iBestValue = 0;
	pBestCity = NULL;

	if (GET_TEAM(getTeam()).isAtWar(ORC_TEAM))
	{
		for (pLoopCity = GET_PLAYER(ORC_PLAYER).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(ORC_PLAYER).nextCity(&iLoop))
		{
			if (AI_plotValid(pLoopCity->plot()))
			{
				// BBAI efficiency: check area for land units before generating path
				if( (getDomainType() == DOMAIN_LAND) && (pLoopCity->area() != area()) && !(getGroup()->canMoveAllTerrain()) )
				{
					continue;
				}
				if (pLoopCity->isRevealed(getTeam(), false))
				{
					if (!atPlot(pLoopCity->plot()) && generatePath(pLoopCity->plot(), 0, true, &iPathTurns, iMaxPathTurns))
					{
						if (iPathTurns < iMaxPathTurns)
						{
							iValue = GET_PLAYER(getOwnerINLINE()).AI_targetCityValue(pLoopCity, false);

							iValue *= 1000;

							iValue /= (iPathTurns + 1);

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestCity = pLoopCity;
							}
						}
					}
				}
			}
		}
	}
	if (GET_TEAM(getTeam()).isAtWar(ANIMAL_TEAM))
	{
		for (pLoopCity = GET_PLAYER(ANIMAL_PLAYER).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(ANIMAL_PLAYER).nextCity(&iLoop))
		{
			if (AI_plotValid(pLoopCity->plot()))
			{
				// BBAI efficiency: check area for land units before generating path
				if( (getDomainType() == DOMAIN_LAND) && (pLoopCity->area() != area()) && !(getGroup()->canMoveAllTerrain()) )
				{
					continue;
				}
				if (pLoopCity->isRevealed(getTeam(), false))
				{
					if (!atPlot(pLoopCity->plot()) && generatePath(pLoopCity->plot(), 0, true, &iPathTurns, iMaxPathTurns))
					{
						if (iPathTurns < iMaxPathTurns)
						{
							iValue = GET_PLAYER(getOwnerINLINE()).AI_targetCityValue(pLoopCity, false);

							iValue *= 1000;

							iValue /= (iPathTurns + 1);

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestCity = pLoopCity;
							}
						}
					}
				}
			}
		}
	}
	if (GET_TEAM(getTeam()).isAtWar(DEMON_TEAM))
	{
		for (pLoopCity = GET_PLAYER(DEMON_PLAYER).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(DEMON_PLAYER).nextCity(&iLoop))
		{
			if (AI_plotValid(pLoopCity->plot()))
			{
				// BBAI efficiency: check area for land units before generating path
				if( (getDomainType() == DOMAIN_LAND) && (pLoopCity->area() != area()) && !(getGroup()->canMoveAllTerrain()) )
				{
					continue;
				}
				if (pLoopCity->isRevealed(getTeam(), false))
				{
					if (!atPlot(pLoopCity->plot()) && generatePath(pLoopCity->plot(), 0, true, &iPathTurns, iMaxPathTurns))
					{
						if (iPathTurns < iMaxPathTurns)
						{
							iValue = GET_PLAYER(getOwnerINLINE()).AI_targetCityValue(pLoopCity, false);

							iValue *= 1000;

							iValue /= (iPathTurns + 1);

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestCity = pLoopCity;
							}
						}
					}
				}
			}
		}
	}
/*************************************************************************************************/
/**	MultiBarb								END													**/
/*************************************************************************************************/

	if (pBestCity != NULL)
	{
		iBestValue = 0;
		pBestPlot = NULL;

		for (iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
		{
			pAdjacentPlot = plotDirection(pBestCity->getX_INLINE(), pBestCity->getY_INLINE(), ((DirectionTypes)iI));

			if (pAdjacentPlot != NULL)
			{
				if (AI_plotValid(pAdjacentPlot))
				{
					if (!(pAdjacentPlot->isVisibleEnemyUnit(this)))
					{
						if (generatePath(pAdjacentPlot, 0, true, &iPathTurns, iMaxPathTurns))
						{
							if( iPathTurns <= iMaxPathTurns )
							{
								iValue = std::max(0, (pAdjacentPlot->defenseModifier(getTeam(), false) + 100));

								if (!(pAdjacentPlot->isRiverCrossing(directionXY(pAdjacentPlot, pBestCity->plot()))))
								{
									iValue += (10 * -(GC.getRIVER_ATTACK_MODIFIER()));
								}

								iValue = std::max(1, iValue);

								iValue *= 1000;

								iValue /= (iPathTurns + 1);

								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									pBestPlot = getPathEndTurnPlot();
								}
							}
						}
					}
				}
			}
		}

		if (pBestPlot != NULL)
		{
			FAssert(!(pBestCity->at(pBestPlot))); // no suicide missions...
			if (atPlot(pBestPlot))
			{
				getGroup()->pushMission(MISSION_SKIP);
				return true;
			}
			else
			{
				getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
				return true;
			}
		}
	}

	return false;
}

bool CvUnitAI::AI_pillageAroundCity(CvCity* pTargetCity, int iBonusValueThreshold, int iMaxPathTurns )
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestPillagePlot;
	int iPathTurns;
	int iValue;
	int iBestValue;

	iBestValue = 0;
	pBestPlot = NULL;
	pBestPillagePlot = NULL;

	for( int iI = 0; iI < NUM_CITY_PLOTS; iI++ )
	{
		pLoopPlot = pTargetCity->getCityIndexPlot(iI);

		if (pLoopPlot != NULL)
		{
			if (AI_plotValid(pLoopPlot) && !(pLoopPlot->isBarbarian()))
			{
				if (potentialWarAction(pLoopPlot) && (pLoopPlot->getTeam() == pTargetCity->getTeam()))
				{
					if (canPillage(pLoopPlot))
					{
						if (!(pLoopPlot->isVisibleEnemyUnit(this)))
						{
							if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_PILLAGE, getGroup()) == 0)
							{
								if (generatePath(pLoopPlot, 0, true, &iPathTurns, iMaxPathTurns))
								{
									if (getPathLastNode()->m_iData1 == 0)
									{
										iPathTurns++;
									}

									if ( iPathTurns <= iMaxPathTurns )
									{
										iValue = AI_pillageValue(pLoopPlot, iBonusValueThreshold);

										iValue *= 1000 + 30*(pLoopPlot->defenseModifier(getTeam(),false));

										iValue /= (iPathTurns + 1);

										// if not at war with this plot owner, then devalue plot if we already inside this owner's borders
										// (because declaring war will pop us some unknown distance away)
										if (!isEnemy(pLoopPlot->getTeam()) && plot()->getTeam() == pLoopPlot->getTeam())
										{
											iValue /= 10;
										}

										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											pBestPlot = getPathEndTurnPlot();
											pBestPillagePlot = pLoopPlot;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestPillagePlot != NULL))
	{
		if (atPlot(pBestPillagePlot) && !isEnemy(pBestPillagePlot->getTeam()))
		{
			//getGroup()->groupDeclareWar(pBestPillagePlot, true);
			// rather than declare war, just find something else to do, since we may already be deep in enemy territory
			return false;
		}

		if (atPlot(pBestPillagePlot))
		{
			if (isEnemy(pBestPillagePlot->getTeam()))
			{
				getGroup()->pushMission(MISSION_PILLAGE, -1, -1, 0, false, false, MISSIONAI_PILLAGE, pBestPillagePlot);
				return true;
			}
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_PILLAGE, pBestPillagePlot);
			return true;
		}
	}

	return false;
}

// Returns true if a mission was pushed...
bool CvUnitAI::AI_bombardCity()
{
	PROFILE_FUNC();

	CvCity* pBombardCity;

	if (canBombard(plot()))
	{
		pBombardCity = bombardTarget(plot());
		FAssertMsg(pBombardCity != NULL, "BombardCity is not assigned a valid value");

		// do not bombard cities with no defenders
		int iDefenderStrength = pBombardCity->plot()->AI_sumStrength(NO_PLAYER, getOwnerINLINE(), DOMAIN_LAND, /*bDefensiveBonuses*/ true, /*bTestAtWar*/ true, false);
		if (iDefenderStrength == 0)
		{
			return false;
		}

		// do not bombard cities if we have overwelming odds
		int iAttackOdds = getGroup()->AI_attackOdds(pBombardCity->plot(), /*bPotentialEnemy*/ true);
		if (iAttackOdds > 95)
		{
			return false;
		}

		// If we have reasonable odds, check for attacking without waiting for bombards
		if( (iAttackOdds >= GC.getDefineINT("BBAI_SKIP_BOMBARD_BEST_ATTACK_ODDS")) )
		{
			int iBase = std::max(150, GC.getDefineINT("BBAI_SKIP_BOMBARD_BASE_STACK_RATIO"));
			int iComparison = getGroup()->AI_compareStacks(pBombardCity->plot(), /*bPotentialEnemy*/ true, /*bCheckCanAttack*/ true, /*bCheckCanMove*/ true);

			// Big troop advantage plus pretty good starting odds, don't wait to allow reinforcements
			if( iComparison > (iBase - 4*iAttackOdds) )
			{
				return false;
			}

			int iMin = std::max(100, GC.getDefineINT("BBAI_SKIP_BOMBARD_MIN_STACK_RATIO"));
			bool bHasWaited = false;
			CLLNode<IDInfo>* pUnitNode = getGroup()->headUnitNode();
			while (pUnitNode != NULL)
			{
				CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);

				if( pLoopUnit->getFortifyTurns() > 0 )
				{
					bHasWaited = true;
					break;
				}

				pUnitNode = getGroup()->nextUnitNode(pUnitNode);
			}

			// Bombard at least one turn to allow bombers/ships to get some shots in too
			if( bHasWaited && (pBombardCity->getDefenseDamage() > 0) )
			{
				int iBombardTurns = getGroup()->getBombardTurns(pBombardCity);
				if( iComparison > std::max(iMin, iBase - 3*iAttackOdds - 3*iBombardTurns) )
				{
					return false;
				}
			}
		}

		//getGroup()->pushMission(MISSION_PILLAGE);
		getGroup()->pushMission(MISSION_BOMBARD);
		return true;
	}

	return false;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


// Returns true if a mission was pushed...
bool CvUnitAI::AI_cityAttack(int iRange, int iOddsThreshold, bool bFollow)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	int iSearchRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iDX, iDY;

	FAssert(canMove());

	if (bFollow)
	{
		iSearchRange = 1;
	}
	else
	{
		iSearchRange = AI_searchRange(iRange);
	}

	iBestValue = 0;
	pBestPlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot	= plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (AI_plotValid(pLoopPlot))
				{
					if (pLoopPlot->isCity() || (pLoopPlot->isCity(true, getTeam()) && pLoopPlot->isVisibleEnemyUnit(this)))
					{
						if (AI_potentialEnemy(pLoopPlot->getTeam(), pLoopPlot))
						{
/*************************************************************************************************/
/**	Xienwolf Tweak							02/14/09											**/
/**																								**/
/**					Prevents Chokepoint Exploitation against AI targetting						**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
							if (!atPlot(pLoopPlot) && ((bFollow) ? canMoveInto(pLoopPlot, true) : (generatePath(pLoopPlot, 0, true, &iPathTurns) && (iPathTurns <= iRange))))
/**								----  End Original Code  ----									**/
							if (!atPlot(pLoopPlot) && ((bFollow) ? canMoveInto(pLoopPlot, true) : (generatePath(pLoopPlot, MOVE_THROUGH_ENEMY | MOVE_IGNORE_DANGER, true, &iPathTurns, iRange) && (iPathTurns <= iRange))))
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
							{
								iValue = getGroup()->AI_attackOdds(pLoopPlot, true);

								if (iValue >= AI_finalOddsThreshold(pLoopPlot, iOddsThreshold))
								{
									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = ((bFollow) ? pLoopPlot : getPathEndTurnPlot());
										FAssert(!atPlot(pBestPlot));
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), ((bFollow) ? MOVE_DIRECT_ATTACK : 0));
		return true;
	}

	return false;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/01/10                                jdog5000      */
/*                                                                                              */
/* War tactics AI, Efficiency                                                                   */
/************************************************************************************************/
// Returns true if a mission was pushed...
bool CvUnitAI::AI_anyAttack(int iRange, int iOddsThreshold, int iMinStack, bool bAllowCities, bool bFollow)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	int iSearchRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iDX, iDY;

	FAssert(canMove());

/*************************************************************************************************/
/**	Xienwolf Tweak							04/09/09											**/
/**																								**/
/**			Since ranged attacks don't prevent other actions, don't stop processing the move	**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	if (AI_rangeAttack(iRange))
	{
		return true;
	}
/**								----  End Original Code  ----									**/
/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
//Since ranged attacks now do cost movement we might not even want to range attack... especially since it might prevent a normal attack which is what we're here for!
/**								---- Start Original Code ----									**
	AI_rangeAttack(iRange);
/**								----  End Original Code  ----									**/
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (bFollow)
	{
		iSearchRange = 1;
	}
	else
	{
		iSearchRange = AI_searchRange(iRange);
	}

	iBestValue = 0;
	pBestPlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot	= plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (AI_plotValid(pLoopPlot))
				{
					if( (bAllowCities) || !(pLoopPlot->isCity(false)) )
					{
						if (pLoopPlot->isVisibleEnemyUnit(this) || (pLoopPlot->isCity() && AI_potentialEnemy(pLoopPlot->getTeam())))
						{
							if (pLoopPlot->getNumVisibleEnemyDefenders(this) >= iMinStack)
							{
								if (!atPlot(pLoopPlot) && ((bFollow) ? canMoveInto(pLoopPlot, true) : (generatePath(pLoopPlot, 0, true, &iPathTurns, iRange) && (iPathTurns <= iRange))))
								{

									iValue = getGroup()->AI_attackOdds(pLoopPlot, true);

									if (iValue >= AI_finalOddsThreshold(pLoopPlot, iOddsThreshold))
									{
										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											pBestPlot = ((bFollow) ? pLoopPlot : getPathEndTurnPlot());
											FAssert(!atPlot(pBestPlot));
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), ((bFollow) ? MOVE_DIRECT_ATTACK : 0));
		return true;
	}

	return false;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

// Returns true if a mission was pushed...
/*************************************************************************************************/
/**	Tweak							04/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
bool CvUnitAI::AI_rangeAttack(int iRange)
{
	PROFILE_FUNC();

/*************************************************************************************************/
/**	Xienwolf Tweak							03/07/09											**/
/**																								**/
/**				Ranged Attacks no longer require ability to move, useless assert				**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	FAssert(canMove());
/**								----  End Original Code  ----									**/
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**

	if (!canRangeStrike())
	{
		return false;
	}

	int iSearchRange = AI_searchRange(iRange);

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;

	for (int iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (int iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->isVisibleEnemyUnit(this) || (pLoopPlot->isCity() && AI_potentialEnemy(pLoopPlot->getTeam())))
				{
					if (!atPlot(pLoopPlot) && canRangeStrikeAt(plot(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()))
					{
						int iValue = getGroup()->AI_attackOdds(pLoopPlot, true);

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = pLoopPlot;
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_RANGE_ATTACK, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0);
		return true;
	}

	return false;
}
/**								----  End Original Code  ----									**/
bool CvUnitAI::AI_rangeAttack(int iRange, int iOddsThreshold)
{
	PROFILE_FUNC();

	FAssert(canMove());

	if (!canRangeStrike())
	{
		return false;
	}

	CvPlot* pBestPlot = NULL;
	CvPlot* pLoopPlot;
	int iBestValue = MAX_INT;
	int iValue;
	int iDX, iDY;

	for (iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (iDY = -(iRange); iDY <= iRange; iDY++)
		{
			pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->isVisibleEnemyUnit(this) || (pLoopPlot->isCity() && AI_potentialEnemy(pLoopPlot->getTeam())))
				{
					if (!atPlot(pLoopPlot) && canRangeStrikeAt(plot(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()))
					{
						iValue = getGroup()->AI_attackOdds(pLoopPlot, true);

						//We only do ranged strikes if we have poor odds. Otherwise we're better off doing a normal attack.
						if (iValue < AI_finalOddsThreshold(pLoopPlot, iOddsThreshold))
						{
							if (iValue < iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = pLoopPlot;
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_RANGE_ATTACK, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0);
		return true;
	}

	return false;
}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

bool CvUnitAI::AI_leaveAttack(int iRange, int iOddsThreshold, int iStrengthThreshold)
{
	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	CvCity* pCity;
	int iSearchRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iDX, iDY;

	FAssert(canMove());

	iSearchRange = iRange;

	iBestValue = 0;
	pBestPlot = NULL;


	pCity = plot()->getPlotCity();

	if ((pCity != NULL) && (pCity->getOwnerINLINE() == getOwnerINLINE()))
	{
		int iOurStrength = GET_PLAYER(getOwnerINLINE()).AI_getOurPlotStrength(plot(), 0, false, false);
		int iEnemyStrength = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(), 2, false, false);
		if (iEnemyStrength > 0)
		{
			if (((iOurStrength * 100) / iEnemyStrength) < iStrengthThreshold)
			{
				return false;
			}
			if (plot()->plotCount(PUF_canDefendGroupHead, -1, -1, getOwnerINLINE()) <= getGroup()->getNumUnits())
			{
				return false;
			}
		}
	}

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (AI_plotValid(pLoopPlot))
				{
					if (pLoopPlot->isVisibleEnemyUnit(this) || (pLoopPlot->isCity() && AI_potentialEnemy(pLoopPlot->getTeam(), pLoopPlot)))
					{
						if (pLoopPlot->getNumVisibleEnemyDefenders(this) > 0)
						{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/27/10                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
							if (!atPlot(pLoopPlot) && (generatePath(pLoopPlot, 0, true, &iPathTurns, iRange) && (iPathTurns <= iRange)))
							{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

								iValue = getGroup()->AI_attackOdds(pLoopPlot, true);

								if (iValue >= AI_finalOddsThreshold(pLoopPlot, iOddsThreshold))
								{
									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = getPathEndTurnPlot();
										FAssert(!atPlot(pBestPlot));
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0);
		return true;
	}

	return false;

}

// Returns true if a mission was pushed...
bool CvUnitAI::AI_blockade()
{
	PROFILE_FUNC();

	CvCity* pCity;
	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestBlockadePlot;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iI;

	iBestValue = 0;
	pBestPlot = NULL;
	pBestBlockadePlot = NULL;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (AI_plotValid(pLoopPlot))
		{
			if (potentialWarAction(pLoopPlot))
			{
				pCity = pLoopPlot->getWorkingCity();

				if (pCity != NULL)
				{
					if (pCity->isCoastal(GC.getMIN_WATER_SIZE_FOR_OCEAN()))
					{
						if (!(pCity->isBarbarian()))
						{
							FAssert(isEnemy(pCity->getTeam()) || GET_TEAM(getTeam()).AI_getWarPlan(pCity->getTeam()) != NO_WARPLAN);

							if (!(pLoopPlot->isVisibleEnemyUnit(this)) && canPlunder(pLoopPlot))
							{
								if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_BLOCKADE, getGroup(), 2) == 0)
								{
									if (generatePath(pLoopPlot, 0, true, &iPathTurns))
									{
										iValue = 1;

										iValue += std::min(pCity->getPopulation(), pCity->countNumWaterPlots());

										iValue += GET_PLAYER(getOwnerINLINE()).AI_adjacentPotentialAttackers(pCity->plot());

										iValue += (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pCity->plot(), MISSIONAI_ASSAULT, getGroup(), 2) * 3);

										if (canBombard(pLoopPlot))
										{
											iValue *= 2;
										}

										iValue *= 1000;

										iValue /= (iPathTurns + 1);

										if (iPathTurns == 1)
										{
											//Prefer to have movement remaining to Bombard + Plunder
											iValue *= 1 + std::min(2, getPathLastNode()->m_iData1);
										}

										// if not at war with this plot owner, then devalue plot if we already inside this owner's borders
										// (because declaring war will pop us some unknown distance away)
										if (!isEnemy(pLoopPlot->getTeam()) && plot()->getTeam() == pLoopPlot->getTeam())
										{
											iValue /= 10;
										}

										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											pBestPlot = getPathEndTurnPlot();
											pBestBlockadePlot = pLoopPlot;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestBlockadePlot != NULL))
	{
		FAssert(canPlunder(pBestBlockadePlot));
		if (atPlot(pBestBlockadePlot) && !isEnemy(pBestBlockadePlot->getTeam(), pBestBlockadePlot))
		{
			getGroup()->groupDeclareWar(pBestBlockadePlot, true);
		}

		if (atPlot(pBestBlockadePlot))
		{
			if (canBombard(plot()))
			{
				getGroup()->pushMission(MISSION_BOMBARD, -1, -1, 0, false, false, MISSIONAI_BLOCKADE, pBestBlockadePlot);
			}

			getGroup()->pushMission(MISSION_PLUNDER, -1, -1, 0, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BLOCKADE, pBestBlockadePlot);

			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_BLOCKADE, pBestBlockadePlot);
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_pirateBlockade()
{
	PROFILE_FUNC();

	int iPathTurns;
	int iValue;
	int iI;

	std::vector<int> aiDeathZone(GC.getMapINLINE().numPlotsINLINE(), 0);

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
		if (AI_plotValid(pLoopPlot) || (pLoopPlot->isCity() && pLoopPlot->isAdjacentToArea(area())))
		{
			if (pLoopPlot->isOwned() && (pLoopPlot->getTeam() != getTeam()))
			{
				int iBestHostileMoves = 0;
				CLLNode<IDInfo>* pUnitNode = pLoopPlot->headUnitNode();
				while (pUnitNode != NULL)
				{
					CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
					pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);
					if (isEnemy(pLoopUnit->getTeam(), pLoopUnit->plot()))
					{
						if (pLoopUnit->getDomainType() == DOMAIN_SEA && !pLoopUnit->isInvisible(getTeam(), false))
						{
							if (pLoopUnit->canAttack())
							{
								if (pLoopUnit->currEffectiveStr(NULL, NULL, NULL) > currEffectiveStr(pLoopPlot, pLoopUnit, NULL))
								{
									iBestHostileMoves = std::max(iBestHostileMoves, pLoopUnit->getMoves());
								}
							}
						}
					}
				}
				if (iBestHostileMoves > 0)
				{
					for (int iX = -iBestHostileMoves; iX <= iBestHostileMoves; iX++)
					{
						for (int iY = -iBestHostileMoves; iY <= iBestHostileMoves; iY++)
						{
							CvPlot * pRangePlot = plotXY(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), iX, iY);
							if (pRangePlot != NULL)
							{
								aiDeathZone[GC.getMap().plotNumINLINE(pRangePlot->getX_INLINE(), pRangePlot->getY_INLINE())]++;
							}
						}
					}
				}
			}
		}
	}

	bool bIsInDanger = aiDeathZone[GC.getMap().plotNumINLINE(getX_INLINE(), getY_INLINE())] > 0;

	if (!bIsInDanger)
	{
		if (getDamage() > 0)
		{
			if (!plot()->isOwned() && !plot()->isAdjacentOwned())
			{
				if (AI_retreatToCity(false, false, 1 + getDamage() / 20))
				{
					return true;
				}
				getGroup()->pushMission(MISSION_SKIP);
				return true;
			}
		}
	}

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	CvPlot* pBestBlockadePlot = NULL;
	bool bBestIsForceMove = false;
	bool bBestIsMove = false;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (AI_plotValid(pLoopPlot))
		{
			if (!(pLoopPlot->isVisibleEnemyUnit(this)) && canPlunder(pLoopPlot))
			{
				if (GC.getGame().getSorenRandNum(4, "AI Pirate Blockade") == 0)
				{
					if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_BLOCKADE, getGroup(), 3) == 0)
					{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/17/09                                jdog5000      */
/*                                                                                              */
/* Pirate AI                                                                                    */
/************************************************************************************************/
/* original bts code
						if (generatePath(pLoopPlot, 0, true, &iPathTurns))
*/
						if (generatePath(pLoopPlot, MOVE_AVOID_ENEMY_WEIGHT_3, true, &iPathTurns))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
						{
							int iBlockadedCount = 0;
							int iPopulationValue = 0;
							int iRange = GC.getDefineINT("SHIP_BLOCKADE_RANGE") - 1;
							for (int iX = -iRange; iX <= iRange; iX++)
							{
								for (int iY = -iRange; iY <= iRange; iY++)
								{
									CvPlot* pRangePlot = plotXY(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), iX, iY);
									if (pRangePlot != NULL)
									{
										bool bPlotBlockaded = false;
										if (pRangePlot->isWater() && pRangePlot->isOwned() && isEnemy(pRangePlot->getTeam(), pLoopPlot))
										{
											bPlotBlockaded = true;
											iBlockadedCount += pRangePlot->getBlockadedCount(pRangePlot->getTeam());
										}

										if (!bPlotBlockaded)
										{
											CvCity* pPlotCity = pRangePlot->getPlotCity();
											if (pPlotCity != NULL)
											{
												if (isEnemy(pPlotCity->getTeam(), pLoopPlot))
												{
													int iCityValue = 3 + pPlotCity->getPopulation();
													iCityValue *= (atWar(getTeam(), pPlotCity->getTeam()) ? 1 : 3);
													if (GET_PLAYER(pPlotCity->getOwnerINLINE()).isNoForeignTrade())
													{
														iCityValue /= 2;
													}
													iPopulationValue += iCityValue;

												}
											}
										}
									}
								}
							}
							iValue = iPopulationValue;

							iValue *= 1000;

							iValue /= 16 + iBlockadedCount;

							bool bMove = ((getPathLastNode()->m_iData2 == 1) && getPathLastNode()->m_iData1 > 0);
							if (atPlot(pLoopPlot))
							{
								iValue *= 3;
							}
							else if (bMove)
							{
								iValue *= 2;
							}

							int iDeath = aiDeathZone[GC.getMap().plotNumINLINE(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE())];

							bool bForceMove = false;
							if (iDeath)
							{
								iValue /= 10;
							}
							else if (bIsInDanger && (iPathTurns <= 2) && (0 == iPopulationValue))
							{
								if (getPathLastNode()->m_iData1 == 0)
								{
									if (!pLoopPlot->isAdjacentOwned())
									{
										int iRand = GC.getGame().getSorenRandNum(2500, "AI Pirate Retreat");
										iValue += iRand;
										if (iRand > 1000)
										{
											iValue += GC.getGame().getSorenRandNum(2500, "AI Pirate Retreat");
											bForceMove = true;
										}
									}
								}
							}

							if (!bForceMove)
							{
								iValue /= iPathTurns + 1;
							}

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = bForceMove ? pLoopPlot : getPathEndTurnPlot();
								pBestBlockadePlot = pLoopPlot;
								bBestIsForceMove = bForceMove;
								bBestIsMove = bMove;
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestBlockadePlot != NULL))
	{
		FAssert(canPlunder(pBestBlockadePlot));

		if (atPlot(pBestBlockadePlot))
		{
			getGroup()->pushMission(MISSION_PLUNDER, -1, -1, 0, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BLOCKADE, pBestBlockadePlot);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			if (bBestIsForceMove)
			{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/01/09                                jdog5000      */
/*                                                                                              */
/* Pirate AI                                                                                    */
/************************************************************************************************/
/* original bts code
				getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
*/
				getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_AVOID_ENEMY_WEIGHT_3);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				return true;
			}
			else
			{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/01/09                                jdog5000      */
/*                                                                                              */
/* Pirate AI                                                                                    */
/************************************************************************************************/
/* original bts code
				getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_BLOCKADE, pBestBlockadePlot);
*/
				getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_AVOID_ENEMY_WEIGHT_3, false, false, MISSIONAI_BLOCKADE, pBestBlockadePlot);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				if (bBestIsMove)
				{
					getGroup()->pushMission(MISSION_PLUNDER, -1, -1, 0, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BLOCKADE, pBestBlockadePlot);
				}
				return true;
			}
		}
	}

	return false;
}

// Returns true if a mission was pushed...
bool CvUnitAI::AI_seaBombardRange(int iMaxRange)
{
	PROFILE_FUNC();

	// cached values
	CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());
	CvPlot* pPlot = plot();
	CvSelectionGroup* pGroup = getGroup();

	// can any unit in this group bombard?
	bool bHasBombardUnit = false;
	bool bBombardUnitCanBombardNow = false;
	CLLNode<IDInfo>* pUnitNode = pGroup->headUnitNode();
	while (pUnitNode != NULL && !bBombardUnitCanBombardNow)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = pGroup->nextUnitNode(pUnitNode);

		if (pLoopUnit->bombardRate() > 0)
		{
			bHasBombardUnit = true;

			if (pLoopUnit->canMove() && !pLoopUnit->isMadeAttack())
			{
				bBombardUnitCanBombardNow = true;
			}
		}
	}

	if (!bHasBombardUnit)
	{
		return false;
	}

	// best match
	CvPlot* pBestPlot = NULL;
	CvPlot* pBestBombardPlot = NULL;
	int iBestValue = 0;

	// iterate over plots at each range
	for (int iDX = -(iMaxRange); iDX <= iMaxRange; iDX++)
	{
		for (int iDY = -(iMaxRange); iDY <= iMaxRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL && AI_plotValid(pLoopPlot))
			{
				CvCity* pBombardCity = bombardTarget(pLoopPlot);

				if (pBombardCity != NULL && isEnemy(pBombardCity->getTeam(), pLoopPlot) && pBombardCity->getDefenseDamage() < GC.getMAX_CITY_DEFENSE_DAMAGE())
				{
					int iPathTurns;
					if (generatePath(pLoopPlot, 0, true, &iPathTurns, (1 + iMaxRange/std::max(1, baseMoves()))))
					{
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						6/24/08				jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
						// Loop construction doesn't guarantee we can get there anytime soon, could be on other side of narrow continent
						if( iPathTurns <= (1 + iMaxRange/std::max(1, baseMoves())) )
						{
							// Check only for supporting our own ground troops first, if none will look for another target
							int iValue = (kPlayer.AI_plotTargetMissionAIs(pBombardCity->plot(), MISSIONAI_ASSAULT, NULL, 2) * 3);
							iValue += (kPlayer.AI_adjacentPotentialAttackers(pBombardCity->plot(), true));

							if (iValue > 0)
							{
								iValue *= 1000;

								iValue /= (iPathTurns + 1);

								if (iPathTurns == 1)
								{
									//Prefer to have movement remaining to Bombard + Plunder
									iValue *= 1 + std::min(2, getPathLastNode()->m_iData1);
								}

								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									pBestPlot = getPathEndTurnPlot();
									pBestBombardPlot = pLoopPlot;
								}
							}
						}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD							END							*/
/********************************************************************************/
					}
				}
			}
		}
	}

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						6/24/08				jdog5000	*/
/* 																			*/
/* 	Naval AI																*/
/********************************************************************************/
	// If no troops of ours to support, check for other bombard targets
	if( (pBestPlot == NULL) && (pBestBombardPlot == NULL) )
	{
		if( (AI_getUnitAIType() != UNITAI_ASSAULT_SEA) )
		{
			for (int iDX = -(iMaxRange); iDX <= iMaxRange; iDX++)
			{
				for (int iDY = -(iMaxRange); iDY <= iMaxRange; iDY++)
				{
					CvPlot* pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

					if (pLoopPlot != NULL && AI_plotValid(pLoopPlot))
					{
						CvCity* pBombardCity = bombardTarget(pLoopPlot);

						// Consider city even if fully bombarded, causes ship to camp outside blockading instead of twitching between
						// cities after bombarding to 0
						if (pBombardCity != NULL && isEnemy(pBombardCity->getTeam(), pLoopPlot) && pBombardCity->getTotalDefense(false) > 0)
						{
							int iPathTurns;
							if (generatePath(pLoopPlot, 0, true, &iPathTurns, 1 + iMaxRange/std::max(1, baseMoves())))
							{
								// Loop construction doesn't guarantee we can get there anytime soon, could be on other side of narrow continent
								if( iPathTurns <= 1 + iMaxRange/std::max(1, baseMoves()) )
								{
									int iValue = std::min(20,pBombardCity->getDefenseModifier(false)/2);

									// Inclination to support attacks by others
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
									//if( GET_PLAYER(pBombardCity->getOwnerINLINE()).AI_getPlotDanger(pBombardCity->plot(), 2, false) > 0 )
									if( GET_PLAYER(pBombardCity->getOwnerINLINE()).AI_getAnyPlotDanger(pBombardCity->plot(), 2, false) )
									{
										iValue += 60;
									}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

									// Inclination to bombard a different nearby city to extend the reach of blockade
									if( GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pBombardCity->plot(), MISSIONAI_BLOCKADE, getGroup(), 3) == 0 )
									{
										iValue += 35 + pBombardCity->getPopulation();
									}

									// Small inclination to bombard area target, not too large so as not to tip our hand
									if( pBombardCity == pBombardCity->area()->getTargetCity(getOwnerINLINE()) )
									{
										iValue += 10;
									}

									if (iValue > 0)
									{
										iValue *= 1000;

										iValue /= (iPathTurns + 1);

										if (iPathTurns == 1)
										{
											//Prefer to have movement remaining to Bombard + Plunder
											iValue *= 1 + std::min(2, getPathLastNode()->m_iData1);
										}

										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											pBestPlot = getPathEndTurnPlot();
											pBestBombardPlot = pLoopPlot;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	/********************************************************************************/
	/* 	BETTER_BTS_AI_MOD							END							*/
	/********************************************************************************/

	if ((pBestPlot != NULL) && (pBestBombardPlot != NULL))
	{
		if (atPlot(pBestBombardPlot))
		{
			// if we are at the plot from which to bombard, and we have a unit that can bombard this turn, do it
			if (bBombardUnitCanBombardNow && pGroup->canBombard(pBestBombardPlot))
			{
				getGroup()->pushMission(MISSION_BOMBARD, -1, -1, 0, false, false, MISSIONAI_BLOCKADE, pBestBombardPlot);

				// if city bombarded enough, wake up any units that were waiting to bombard this city
				CvCity* pBombardCity = bombardTarget(pBestBombardPlot); // is NULL if city cannot be bombarded any more
				if (pBombardCity == NULL || pBombardCity->getDefenseDamage() < ((GC.getMAX_CITY_DEFENSE_DAMAGE()*5)/6))
				{
					kPlayer.AI_wakePlotTargetMissionAIs(pBestBombardPlot, MISSIONAI_BLOCKADE, getGroup());
				}
			}
			// otherwise, skip until next turn, when we will surely bombard
			else if (canPlunder(pBestBombardPlot))
			{
				getGroup()->pushMission(MISSION_PLUNDER, -1, -1, 0, false, false, MISSIONAI_BLOCKADE, pBestBombardPlot);
			}
			else
			{
				getGroup()->pushMission(MISSION_SKIP);
			}

			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_BLOCKADE, pBestBombardPlot);
			return true;
		}
	}

	return false;
}



// Returns true if a mission was pushed...
bool CvUnitAI::AI_pillage(int iBonusValueThreshold)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestPillagePlot;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iI;

	iBestValue = 0;
	pBestPlot = NULL;
	pBestPillagePlot = NULL;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (AI_plotValid(pLoopPlot) && !(pLoopPlot->isBarbarian()))
		{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/22/10                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
			//if (potentialWarAction(pLoopPlot))
			if( pLoopPlot->isOwned() && isEnemy(pLoopPlot->getTeam(),pLoopPlot) )
			{
				CvCity * pWorkingCity = pLoopPlot->getWorkingCity();

				if (pWorkingCity != NULL)
				{
					if (!(pWorkingCity == area()->getTargetCity(getOwnerINLINE())) && canPillage(pLoopPlot))
					{
						if (!(pLoopPlot->isVisibleEnemyUnit(this)))
						{
							if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_PILLAGE, getGroup(), 1) == 0)
							{
								iValue = AI_pillageValue(pLoopPlot, iBonusValueThreshold);
								iValue *= 1000;

								// if not at war with this plot owner, then devalue plot if we already inside this owner's borders
								// (because declaring war will pop us some unknown distance away)
								if (!isEnemy(pLoopPlot->getTeam()) && plot()->getTeam() == pLoopPlot->getTeam())
								{
									iValue /= 10;
								}

								if( iValue > iBestValue )
								{
									if (generatePath(pLoopPlot, 0, true, &iPathTurns))
									{
										iValue /= (iPathTurns + 1);

										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											pBestPlot = getPathEndTurnPlot();
											pBestPillagePlot = pLoopPlot;
										}
									}
								}
							}
						}
					}
				}
			}
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	}

	if ((pBestPlot != NULL) && (pBestPillagePlot != NULL))
	{
		if (atPlot(pBestPillagePlot) && !isEnemy(pBestPillagePlot->getTeam()))
		{
			//getGroup()->groupDeclareWar(pBestPillagePlot, true);
			// rather than declare war, just find something else to do, since we may already be deep in enemy territory
			return false;
		}

		if (atPlot(pBestPillagePlot))
		{
			if (isEnemy(pBestPillagePlot->getTeam()))
			{
				getGroup()->pushMission(MISSION_PILLAGE, -1, -1, 0, false, false, MISSIONAI_PILLAGE, pBestPillagePlot);
				return true;
			}
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_PILLAGE, pBestPillagePlot);
			return true;
		}
	}

	return false;
}

bool CvUnitAI::AI_canPillage(CvPlot& kPlot) const
{
	if (isEnemy(kPlot.getTeam(), &kPlot))
	{
		return true;
	}

	if (!kPlot.isOwned())
	{
		bool bPillageUnowned = true;

		for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS && bPillageUnowned; ++iPlayer)
		{
			int iIndx;
			CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
			if (!isEnemy(kLoopPlayer.getTeam(), &kPlot))
			{
				for (CvCity* pCity = kLoopPlayer.firstCity(&iIndx); NULL != pCity; pCity = kLoopPlayer.nextCity(&iIndx))
				{
					if (kPlot.getPlotGroup((PlayerTypes)iPlayer) == pCity->plot()->getPlotGroup((PlayerTypes)iPlayer))
					{
						bPillageUnowned = false;
						break;
					}

				}
			}
		}

		if (bPillageUnowned)
		{
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_pillageRange(int iRange, int iBonusValueThreshold)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestPillagePlot;
	int iSearchRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iDX, iDY;

	iSearchRange = AI_searchRange(iRange);

	iBestValue = 0;
	pBestPlot = NULL;
	pBestPillagePlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (AI_plotValid(pLoopPlot) && !(pLoopPlot->isBarbarian()))
				{
					if (potentialWarAction(pLoopPlot))
					{
						CvCity * pWorkingCity = pLoopPlot->getWorkingCity();

						if (pWorkingCity != NULL)
						{
							if (!(pWorkingCity == area()->getTargetCity(getOwnerINLINE())) && canPillage(pLoopPlot))
							{
								if (!(pLoopPlot->isVisibleEnemyUnit(this)))
								{
									if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_PILLAGE, getGroup()) == 0)
									{
										if (generatePath(pLoopPlot, 0, true, &iPathTurns, iRange))
										{
											if (getPathLastNode()->m_iData1 == 0)
											{
												iPathTurns++;
											}

											if (iPathTurns <= iRange)
											{
												iValue = AI_pillageValue(pLoopPlot, iBonusValueThreshold);

												iValue *= 1000;

												iValue /= (iPathTurns + 1);

												// if not at war with this plot owner, then devalue plot if we already inside this owner's borders
												// (because declaring war will pop us some unknown distance away)
												if (!isEnemy(pLoopPlot->getTeam()) && plot()->getTeam() == pLoopPlot->getTeam())
												{
													iValue /= 10;
												}

												if (iValue > iBestValue)
												{
													iBestValue = iValue;
													pBestPlot = getPathEndTurnPlot();
													pBestPillagePlot = pLoopPlot;
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestPillagePlot != NULL))
	{
		if (atPlot(pBestPillagePlot) && !isEnemy(pBestPillagePlot->getTeam()))
		{
			//getGroup()->groupDeclareWar(pBestPillagePlot, true);
			// rather than declare war, just find something else to do, since we may already be deep in enemy territory
			return false;
		}

		if (atPlot(pBestPillagePlot))
		{
			if (isEnemy(pBestPillagePlot->getTeam()))
			{
				getGroup()->pushMission(MISSION_PILLAGE, -1, -1, 0, false, false, MISSIONAI_PILLAGE, pBestPillagePlot);
				return true;
			}
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_PILLAGE, pBestPillagePlot);
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_found()
{
	PROFILE_FUNC();

	int iPathTurns;
	int iValue;
	int iBestFoundValue = 0;
	CvPlot* pBestPlot = NULL;
	CvPlot* pBestFoundPlot = NULL;

	for (int iI = 0; iI < GET_PLAYER(getOwnerINLINE()).AI_getNumCitySites(); iI++)
	{
		CvPlot* pCitySitePlot = GET_PLAYER(getOwnerINLINE()).AI_getCitySite(iI);

		// Settler AI : BETTER_BTS_AI_MOD jdog5000 10/23/09

		if (pCitySitePlot->getArea() != getArea() && !canMoveAllTerrain())
			continue;

		if (!canFound(pCitySitePlot))
			continue;

		if (pCitySitePlot->isVisibleEnemyUnit(this))
			continue;

		// Disallow if we have another AI doing the same mission to the same plot
		if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pCitySitePlot, MISSIONAI_FOUND, getGroup()) > 0)
			continue;

		// Disallow if current group can't defend and there aren't any other city guard missions enroute to that plot. Also allow faster AI settling : Blazenclaw AI_SpeedySettle
		if (!(getGroup()->canDefend() || isHasPromotion((PromotionTypes)GC.getDefineINT("STARTING_SETTLER_PROMOTION"))) && GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pCitySitePlot, MISSIONAI_GUARD_CITY) == 0)
			continue;

		// Must be able to actually generate a path
		if (!generatePath(pCitySitePlot, MOVE_SAFE_TERRITORY, true, &iPathTurns))
			continue;

		iValue = pCitySitePlot->getFoundValue(getOwnerINLINE());
		iValue *= 1000;
		iValue /= (iPathTurns + 1);
		if (iValue > iBestFoundValue)
		{
			iBestFoundValue = iValue;
			pBestPlot = getPathEndTurnPlot();
			pBestFoundPlot = pCitySitePlot;
		}
	}

	if ((pBestPlot != NULL) && (pBestFoundPlot != NULL))
	{
		if (atPlot(pBestFoundPlot))
		{
			getGroup()->pushMission(MISSION_FOUND, -1, -1, 0, false, false, MISSIONAI_FOUND, pBestFoundPlot);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_SAFE_TERRITORY, false, false, MISSIONAI_FOUND, pBestFoundPlot);
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_foundRange(int iRange, bool bFollow)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestFoundPlot;
	int iSearchRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iDX, iDY;

	iSearchRange = AI_searchRange(iRange);

	iBestValue = 0;
	pBestPlot = NULL;
	pBestFoundPlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot	= plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
/*************************************************************************************************/
/**	Speedup								11/02/12										Snarko	**/
/**																								**/
/**		For the few cases where getPlotDanger is still used, use a threshold where possible		**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
				if (AI_plotValid(pLoopPlot) && (pLoopPlot != plot() || GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(pLoopPlot, 1) <= pLoopPlot->plotCount(PUF_canDefend, -1, -1, getOwnerINLINE())))
/**								----  End Original Code  ----									**/
				if (AI_plotValid(pLoopPlot) && (pLoopPlot != plot() || !GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(pLoopPlot, 1, true, pLoopPlot->plotCount(PUF_canDefend, -1, -1, getOwnerINLINE()))))
/*************************************************************************************************/
/**	Speedup									END													**/
/*************************************************************************************************/
				{
					if (canFound(pLoopPlot))
					{
						iValue = pLoopPlot->getFoundValue(getOwnerINLINE());

						if (iValue > iBestValue)
						{
							if (!(pLoopPlot->isVisibleEnemyUnit(this)))
							{
								if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_FOUND, getGroup(), 3) == 0)
								{
									if (generatePath(pLoopPlot, MOVE_SAFE_TERRITORY, true, &iPathTurns, iRange))
									{
										if (iPathTurns <= iRange)
										{
											iBestValue = iValue;
											pBestPlot = getPathEndTurnPlot();
											pBestFoundPlot = pLoopPlot;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestFoundPlot != NULL))
	{
		if (atPlot(pBestFoundPlot))
		{
			getGroup()->pushMission(MISSION_FOUND, -1, -1, 0, false, false, MISSIONAI_FOUND, pBestFoundPlot);
			return true;
		}
		else if (!bFollow)
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_SAFE_TERRITORY, false, false, MISSIONAI_FOUND, pBestFoundPlot);
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_assaultSeaTransport(bool bBarbarian)
{
	PROFILE_FUNC();

	bool bIsAttackCity = (getUnitAICargo(UNITAI_ATTACK_CITY) > 0);

	FAssert(getGroup()->hasCargo());
	//FAssert(bIsAttackCity || getGroup()->getUnitAICargo(UNITAI_ATTACK) > 0);

	if (!canCargoAllMove())
	{
		return false;
	}

	std::vector<CvUnit*> aGroupCargo;
	CLLNode<IDInfo>* pUnitNode = plot()->headUnitNode();
	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = plot()->nextUnitNode(pUnitNode);
		CvUnit* pTransport = pLoopUnit->getTransportUnit();
		if (pTransport != NULL && pTransport->getGroup() == getGroup())
		{
			aGroupCargo.push_back(pLoopUnit);
		}
	}

	int iCargo = getGroup()->getCargo();
	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	CvPlot* pBestAssaultPlot = NULL;

	for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (pLoopPlot->isCoastalLand())
		{
			if (pLoopPlot->isOwned())
			{
				if (((bBarbarian || !pLoopPlot->isBarbarian())) || GET_PLAYER(getOwnerINLINE()).isMinorCiv())
				{
					if (isPotentialEnemy(pLoopPlot->getTeam(), pLoopPlot))
					{
						int iTargetCities = pLoopPlot->area()->getCitiesPerPlayer(pLoopPlot->getOwnerINLINE());
						if (iTargetCities > 0)
						{
							bool bCanCargoAllUnload = true;
							int iVisibleEnemyDefenders = pLoopPlot->getNumVisibleEnemyDefenders(this);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      11/30/08                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
							if (iVisibleEnemyDefenders > 0 || pLoopPlot->isCity())
							{
								for (uint i = 0; i < aGroupCargo.size(); ++i)
								{
									CvUnit* pAttacker = aGroupCargo[i];
									if( iVisibleEnemyDefenders > 0 )
									{
										CvUnit* pDefender = pLoopPlot->getBestDefender(NO_PLAYER, pAttacker->getOwnerINLINE(), pAttacker, true);
										if (pDefender == NULL || !pAttacker->canAttack(*pDefender))
										{
											bCanCargoAllUnload = false;
											break;
										}
									}
									else if( pLoopPlot->isCity() && !(pLoopPlot->isVisible(getTeam(),false)) )
									{
										// Assume city is defended, artillery can't naval invade
										if( pAttacker->combatLimit() < 100 )
										{
											bCanCargoAllUnload = false;
											break;
										}
									}
								}
							}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

							if (bCanCargoAllUnload)
							{
								int iPathTurns;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/17/09                                jdog5000      */
/*                                                                                              */
/* War tactics AI                                                                               */
/************************************************************************************************/
/* original bts code
								if (generatePath(pLoopPlot, 0, true, &iPathTurns))
*/
								if (generatePath(pLoopPlot, MOVE_AVOID_ENEMY_WEIGHT_3, true, &iPathTurns))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
								{
									int iValue = 1;

									if (!bIsAttackCity)
									{
										iValue += (AI_pillageValue(pLoopPlot, 15) * 10);
									}

									int iAssaultsHere = GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_ASSAULT, getGroup());

									iValue += (iAssaultsHere * 100);

									CvCity* pCity = pLoopPlot->getPlotCity();

									if (pCity == NULL)
									{
										for (int iJ = 0; iJ < NUM_DIRECTION_TYPES; iJ++)
										{
											CvPlot* pAdjacentPlot = plotDirection(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), ((DirectionTypes)iJ));

											if (pAdjacentPlot != NULL)
											{
												pCity = pAdjacentPlot->getPlotCity();

												if (pCity != NULL)
												{
													if (pCity->getOwnerINLINE() == pLoopPlot->getOwnerINLINE())
													{
														break;
													}
													else
													{
														pCity = NULL;
													}
												}
											}
										}
									}

									if (pCity != NULL)
									{
										FAssert(isPotentialEnemy(pCity->getTeam(), pLoopPlot));

										if (!(pLoopPlot->isRiverCrossing(directionXY(pLoopPlot, pCity->plot()))))
										{
											iValue += (50 * -(GC.getRIVER_ATTACK_MODIFIER()));
										}

										iValue += 15 * (pLoopPlot->defenseModifier(getTeam(), false));
										iValue += 1000;
										iValue += (GET_PLAYER(getOwnerINLINE()).AI_adjacentPotentialAttackers(pCity->plot()) * 200);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/26/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
										// Continue attacking in area we have already captured cities
										if( pCity->area()->getCitiesPerPlayer(getOwnerINLINE()) > 0 )
										{
											if( pCity->AI_playerCloseness(getOwnerINLINE()) > 5 )
											{
												iValue *= 3;
												iValue /= 2;
											}
										}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

										if (iPathTurns == 1)
										{
											iValue += GC.getGameINLINE().getSorenRandNum(50, "AI Assault");
										}
									}

									FAssert(iPathTurns > 0);

									if (iPathTurns == 1)
									{
										if (pCity != NULL)
										{
											if (pCity->area()->getNumCities() > 1)
											{
												iValue *= 2;
											}
										}
									}

									iValue *= 1000;

									if (iTargetCities <= iAssaultsHere)
									{
										iValue /= 2;
									}

									if (iTargetCities == 1)
									{
										if (iCargo > 7)
										{
											iValue *= 3;
											iValue /= iCargo - 4;
										}
									}

									if (pLoopPlot->isCity())
									{
										if (iVisibleEnemyDefenders * 3 > iCargo)
										{
											iValue /= 10;
										}
										else
										{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      11/30/08                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
/*
// original bts code
											iValue *= iCargo;
											iValue /= std::max(1, (iVisibleEnemyDefenders * 3));
*/
											// Assume non-visible city is properly defended
											iValue *= iCargo;
											iValue /= std::max(1,std::max(pLoopPlot->getPlotCity()->AI_neededDefenders(), (iVisibleEnemyDefenders * 3)));
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
										}
									}
									else
									{
										if (0 == iVisibleEnemyDefenders)
										{
											iValue *= 4;
											iValue /= 3;
										}
										else
										{
											iValue /= iVisibleEnemyDefenders;
										}
									}

									// if more than 3 turns to get there, then put some randomness into our preference of distance
									// +/- 33%
									if (iPathTurns > 3)
									{
										int iPathAdjustment = GC.getGameINLINE().getSorenRandNum(67, "AI Assault Target");

										iPathTurns *= 66 + iPathAdjustment;
										iPathTurns /= 100;
									}

									iValue /= (iPathTurns + 1);

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = getPathEndTurnPlot();
										pBestAssaultPlot = pLoopPlot;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestAssaultPlot != NULL))
	{
		FAssert(!(pBestPlot->isImpassable()));

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/11/10                                jdog5000      */
/*                                                                                              */
/* War tactics AI                                                                               */
/************************************************************************************************/
		// Cancel missions of all those coming to join departing transport
		CvSelectionGroup* pLoopGroup = NULL;
		int iLoop = 0;
		CvPlayer& kPlayer = GET_PLAYER(getOwnerINLINE());

		for(pLoopGroup = kPlayer.firstSelectionGroup(&iLoop); pLoopGroup != NULL; pLoopGroup = kPlayer.nextSelectionGroup(&iLoop))
		{
			if( pLoopGroup != getGroup() )
			{
				if( pLoopGroup->AI_getMissionAIType() == MISSIONAI_GROUP && pLoopGroup->getHeadUnitAI() == AI_getUnitAIType() )
				{
					CvUnit* pMissionUnit = pLoopGroup->AI_getMissionAIUnit();

					if( pMissionUnit != NULL && pMissionUnit->getGroup() == getGroup() )
					{
						pLoopGroup->clearMissionQueue();
					}
				}
			}
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

		if ((pBestPlot == pBestAssaultPlot) || (stepDistance(pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), pBestAssaultPlot->getX_INLINE(), pBestAssaultPlot->getY_INLINE()) == 1))
		{
			if (atPlot(pBestAssaultPlot))
			{
				getGroup()->unloadAll(); // XXX is this dangerous (not pushing a mission...) XXX air units?
				return true;
			}
			else
			{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/01/09                                jdog5000      */
/*                                                                                              */
/* War tactics AI                                                                               */
/************************************************************************************************/
/* original bts code
				getGroup()->pushMission(MISSION_MOVE_TO, pBestAssaultPlot->getX_INLINE(), pBestAssaultPlot->getY_INLINE(), 0, false, false, MISSIONAI_ASSAULT, pBestAssaultPlot);
*/
				getGroup()->pushMission(MISSION_MOVE_TO, pBestAssaultPlot->getX_INLINE(), pBestAssaultPlot->getY_INLINE(), MOVE_AVOID_ENEMY_WEIGHT_3, false, false, MISSIONAI_ASSAULT, pBestAssaultPlot);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				return true;
			}
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/01/09                                jdog5000      */
/*                                                                                              */
/* War tactics AI                                                                               */
/************************************************************************************************/
/* original bts code
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_ASSAULT, pBestAssaultPlot);
*/
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_AVOID_ENEMY_WEIGHT_3, false, false, MISSIONAI_ASSAULT, pBestAssaultPlot);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			return true;
		}
	}

	return false;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/07/10                                jdog5000      */
/*                                                                                              */
/* Naval AI, Efficiency                                                                         */
/************************************************************************************************/
// Returns true if a mission was pushed...
bool CvUnitAI::AI_assaultSeaReinforce(bool bBarbarian)
{
	PROFILE_FUNC();

	bool bIsAttackCity = (getUnitAICargo(UNITAI_ATTACK_CITY) > 0);

	FAssert(getGroup()->hasCargo());

	if (!canCargoAllMove())
	{
		return false;
	}

	if( !(getGroup()->canAllMove()) )
	{
		return false;
	}

	std::vector<CvUnit*> aGroupCargo;
	CLLNode<IDInfo>* pUnitNode = plot()->headUnitNode();
	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = plot()->nextUnitNode(pUnitNode);
		CvUnit* pTransport = pLoopUnit->getTransportUnit();
		if (pTransport != NULL && pTransport->getGroup() == getGroup())
		{
			aGroupCargo.push_back(pLoopUnit);
		}
	}

	int iCargo = getGroup()->getCargo();
	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	CvPlot* pBestAssaultPlot = NULL;
	CvArea* pWaterArea = plot()->waterArea();
	bool bCity = plot()->isCity(true,getTeam());
	bool bCanMoveAllTerrain = getGroup()->canMoveAllTerrain();

	int iTargetCities;
	int iOurFightersHere;
	int iPathTurns;
	int iValue;

	// Loop over nearby plots for groups in enemy territory to reinforce
	int iRange = 2*baseMoves();
	int iDX, iDY;
	for (iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (iDY = -(iRange); iDY <= iRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if( pLoopPlot != NULL )
			{
				if (pLoopPlot->isOwned())
				{
					if (isEnemy(pLoopPlot->getTeam(), pLoopPlot))
					{
						if ( bCanMoveAllTerrain || (pWaterArea != NULL && pLoopPlot->isAdjacentToArea(pWaterArea)) )
						{
							iTargetCities = pLoopPlot->area()->getCitiesPerPlayer(pLoopPlot->getOwnerINLINE());

							if (iTargetCities > 0)
							{
								iOurFightersHere = pLoopPlot->getNumDefenders(getOwnerINLINE());

								if( iOurFightersHere > 2 )
								{
									iPathTurns;
									if (generatePath(pLoopPlot, MOVE_AVOID_ENEMY_WEIGHT_3, true, &iPathTurns, 2))
									{
										if( iPathTurns <= 2 )
										{
											CvPlot* pEndTurnPlot = getPathEndTurnPlot();

											iValue = 10*iTargetCities;
											iValue += 8*iOurFightersHere;
											iValue += 3*GET_PLAYER(getOwnerINLINE()).AI_adjacentPotentialAttackers(pLoopPlot);

											iValue *= 100;

											iValue /= (iPathTurns + 1);

											if (iValue > iBestValue)
											{
												iBestValue = iValue;
												pBestPlot = pEndTurnPlot;
												pBestAssaultPlot = pLoopPlot;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// Loop over other transport groups, looking for synchronized landing
	if ((pBestPlot == NULL) && (pBestAssaultPlot == NULL))
	{
		int iLoop;
		for(CvSelectionGroup* pLoopSelectionGroup = GET_PLAYER(getOwnerINLINE()).firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = GET_PLAYER(getOwnerINLINE()).nextSelectionGroup(&iLoop))
		{
			if (pLoopSelectionGroup != getGroup())
			{
				if (pLoopSelectionGroup->AI_getMissionAIType() == MISSIONAI_ASSAULT)
				{
					CvPlot* pLoopPlot = pLoopSelectionGroup->AI_getMissionAIPlot();

					if( pLoopPlot != NULL )
					{
						if (pLoopPlot->isOwned())
						{
							if (isPotentialEnemy(pLoopPlot->getTeam(), pLoopPlot))
							{
								if ( bCanMoveAllTerrain || (pWaterArea != NULL && pLoopPlot->isAdjacentToArea(pWaterArea)) )
								{
									iTargetCities = pLoopPlot->area()->getCitiesPerPlayer(pLoopPlot->getOwnerINLINE());
									if (iTargetCities > 0)
									{
										int iAssaultsHere = pLoopSelectionGroup->getCargo();

										if( iAssaultsHere > 2 )
										{
											iPathTurns;
											if (generatePath(pLoopPlot, MOVE_AVOID_ENEMY_WEIGHT_3, true, &iPathTurns))
											{
												CvPlot* pEndTurnPlot = getPathEndTurnPlot();

												int iOtherPathTurns = MAX_INT;
												if (pLoopSelectionGroup->generatePath(pLoopSelectionGroup->plot(), pLoopPlot, MOVE_AVOID_ENEMY_WEIGHT_3, true, &iOtherPathTurns))
												{
													// We need to get there the turn after they do, +1 required whether
													// they move first or we do
													iOtherPathTurns += 1;
												}
												else
												{
													// Should never happen ...
													continue;
												}

												if( (iPathTurns >= iOtherPathTurns) && (iPathTurns < iOtherPathTurns + 5) )
												{
													bool bCanCargoAllUnload = true;
													int iVisibleEnemyDefenders = pLoopPlot->getNumVisibleEnemyDefenders(this);
													if (iVisibleEnemyDefenders > 0 || pLoopPlot->isCity())
													{
														for (uint i = 0; i < aGroupCargo.size(); ++i)
														{
															CvUnit* pAttacker = aGroupCargo[i];
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/21/10                                jdog5000      */
/*                                                                                              */
/* Efficiency                                                                                   */
/************************************************************************************************/
															// From Lead From Behind by UncutDragon
															// original
															//CvUnit* pDefender = pLoopPlot->getBestDefender(NO_PLAYER, pAttacker->getOwnerINLINE(), pAttacker, true);
															//if (pDefender == NULL || !pAttacker->canAttack(*pDefender))
															// modified
															if (!pLoopPlot->hasDefender(true, NO_PLAYER, pAttacker->getOwnerINLINE(), pAttacker, true))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
															{
																bCanCargoAllUnload = false;
																break;
															}
															else if( pLoopPlot->isCity() && !(pLoopPlot->isVisible(getTeam(),false)) )
															{
																// Artillery can't naval invade, so don't try
																if( pAttacker->combatLimit() < 100 )
																{
																	bCanCargoAllUnload = false;
																	break;
																}
															}
														}
													}

													iValue = (iAssaultsHere * 5);
													iValue += iTargetCities*10;

													iValue *= 100;

													// if more than 3 turns to get there, then put some randomness into our preference of distance
													// +/- 33%
													if (iPathTurns > 3)
													{
														int iPathAdjustment = GC.getGameINLINE().getSorenRandNum(67, "AI Assault Target");

														iPathTurns *= 66 + iPathAdjustment;
														iPathTurns /= 100;
													}

													iValue /= (iPathTurns + 1);

													if (iValue > iBestValue)
													{
														iBestValue = iValue;
														pBestPlot = pEndTurnPlot;
														pBestAssaultPlot = pLoopPlot;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// Reinforce our cities in need
	if ((pBestPlot == NULL) && (pBestAssaultPlot == NULL))
	{
		int iLoop;
		CvCity* pLoopCity;

		for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
		{
			if( bCanMoveAllTerrain || (pWaterArea != NULL && (pLoopCity->waterArea(true) == pWaterArea || pLoopCity->secondWaterArea() == pWaterArea)) )
			{
				iValue = 0;
				if(pLoopCity->area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE)
				{
					iValue = 3;
				}
				else if(pLoopCity->area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE)
				{
					iValue = 2;
				}
				else if(pLoopCity->area()->getAreaAIType(getTeam()) == AREAAI_MASSING)
				{
					iValue = 1;
				}
				else if( bBarbarian && (pLoopCity->area()->getCitiesPerPlayer(BARBARIAN_PLAYER) > 0) )
				{
					iValue = 1;
				}

				if( iValue > 0 )
				{
					bool bCityDanger = pLoopCity->AI_isDanger();
					if( (bCity && pLoopCity->area() != area()) || bCityDanger || ((GC.getGameINLINE().getGameTurn() - pLoopCity->getGameTurnAcquired()) < 10 && pLoopCity->getPreviousOwner() != NO_PLAYER) )
					{
						int iOurPower = std::max(1, pLoopCity->area()->getPower(getOwnerINLINE()));
						// Enemy power includes barb power
						int iEnemyPower = GET_TEAM(getTeam()).countEnemyPowerByArea(pLoopCity->area());

						// Don't send troops to areas we are dominating already
						// Don't require presence of enemy cities, just a dangerous force
						if( iOurPower < (3*iEnemyPower) )
						{
							iPathTurns;
							if (generatePath(pLoopCity->plot(), MOVE_AVOID_ENEMY_WEIGHT_3, true, &iPathTurns))
							{
								iValue *= 10*pLoopCity->AI_cityThreat();

								iValue += 20 * GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_ASSAULT, getGroup());

								iValue *= std::min(iEnemyPower, 3*iOurPower);
								iValue /= iOurPower;

								iValue *= 100;

								// if more than 3 turns to get there, then put some randomness into our preference of distance
								// +/- 33%
								if (iPathTurns > 3)
								{
									int iPathAdjustment = GC.getGameINLINE().getSorenRandNum(67, "AI Assault Target");

									iPathTurns *= 66 + iPathAdjustment;
									iPathTurns /= 100;
								}

								iValue /= (iPathTurns + 6);

								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									pBestPlot = (bCityDanger ? getPathEndTurnPlot() : pLoopCity->plot());
									pBestAssaultPlot = pLoopCity->plot();
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot == NULL) && (pBestAssaultPlot == NULL))
	{
		if( bCity )
		{
			if( GET_TEAM(getTeam()).isAVassal() )
			{
				TeamTypes eMasterTeam = NO_TEAM;

				for( int iI = 0; iI < MAX_CIV_TEAMS; iI++ )
				{
					if( GET_TEAM(getTeam()).isVassal((TeamTypes)iI) )
					{
						eMasterTeam = (TeamTypes)iI;
					}
				}

				if( (eMasterTeam != NO_TEAM) && GET_TEAM(getTeam()).isOpenBorders(eMasterTeam) )
				{
					for( int iI = 0; iI < MAX_CIV_PLAYERS; iI++ )
					{
						if( GET_PLAYER((PlayerTypes)iI).getTeam() == eMasterTeam )
						{
							int iLoop;
							CvCity* pLoopCity;

							for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
							{
								if( pLoopCity->area() != area() )
								{
									iValue = 0;
									if(pLoopCity->area()->getAreaAIType(eMasterTeam) == AREAAI_OFFENSIVE)
									{
										iValue = 2;
									}
									else if(pLoopCity->area()->getAreaAIType(eMasterTeam) == AREAAI_MASSING)
									{
										iValue = 1;
									}

									if( iValue > 0 )
									{
										if( bCanMoveAllTerrain || (pWaterArea != NULL && (pLoopCity->waterArea(true) == pWaterArea || pLoopCity->secondWaterArea() == pWaterArea)) )
										{
											int iOurPower = std::max(1, pLoopCity->area()->getPower(getOwnerINLINE()));
											iOurPower += GET_TEAM(eMasterTeam).countPowerByArea(pLoopCity->area());
											// Enemy power includes barb power
											int iEnemyPower = GET_TEAM(eMasterTeam).countEnemyPowerByArea(pLoopCity->area());

											// Don't send troops to areas we are dominating already
											// Don't require presence of enemy cities, just a dangerous force
											if( iOurPower < (2*iEnemyPower) )
											{
												int iPathTurns;
												if (generatePath(pLoopCity->plot(), MOVE_AVOID_ENEMY_WEIGHT_3, true, &iPathTurns))
												{
													iValue *= pLoopCity->AI_cityThreat();

													iValue += 10 * GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_ASSAULT, getGroup());

													iValue *= std::min(iEnemyPower, 3*iOurPower);
													iValue /= iOurPower;

													iValue *= 100;

													// if more than 3 turns to get there, then put some randomness into our preference of distance
													// +/- 33%
													if (iPathTurns > 3)
													{
														int iPathAdjustment = GC.getGameINLINE().getSorenRandNum(67, "AI Assault Target");

														iPathTurns *= 66 + iPathAdjustment;
														iPathTurns /= 100;
													}

													iValue /= (iPathTurns + 1);

													if (iValue > iBestValue)
													{
														iBestValue = iValue;
														pBestPlot = getPathEndTurnPlot();
														pBestAssaultPlot = pLoopCity->plot();
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestAssaultPlot != NULL))
	{
		FAssert(!(pBestPlot->isImpassable()));

		// Cancel missions of all those coming to join departing transport
		CvSelectionGroup* pLoopGroup = NULL;
		int iLoop = 0;
		CvPlayer& kPlayer = GET_PLAYER(getOwnerINLINE());

		for(pLoopGroup = kPlayer.firstSelectionGroup(&iLoop); pLoopGroup != NULL; pLoopGroup = kPlayer.nextSelectionGroup(&iLoop))
		{
			if( pLoopGroup != getGroup() )
			{
				if( pLoopGroup->AI_getMissionAIType() == MISSIONAI_GROUP && pLoopGroup->getHeadUnitAI() == AI_getUnitAIType() )
				{
					CvUnit* pMissionUnit = pLoopGroup->AI_getMissionAIUnit();

					if( pMissionUnit != NULL && pMissionUnit->getGroup() == getGroup() )
					{
						pLoopGroup->clearMissionQueue();
					}
				}
			}
		}

		if ((pBestPlot == pBestAssaultPlot) || (stepDistance(pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), pBestAssaultPlot->getX_INLINE(), pBestAssaultPlot->getY_INLINE()) == 1))
		{
			if (atPlot(pBestAssaultPlot))
			{
				getGroup()->unloadAll(); // XXX is this dangerous (not pushing a mission...) XXX air units?
				return true;
			}
			else
			{
				getGroup()->pushMission(MISSION_MOVE_TO, pBestAssaultPlot->getX_INLINE(), pBestAssaultPlot->getY_INLINE(), MOVE_AVOID_ENEMY_WEIGHT_3, false, false, MISSIONAI_ASSAULT, pBestAssaultPlot);
				return true;
			}
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_AVOID_ENEMY_WEIGHT_3, false, false, MISSIONAI_ASSAULT, pBestAssaultPlot);
			return true;
		}
	}

	return false;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


// Returns true if a mission was pushed...
bool CvUnitAI::AI_settlerSeaTransport()
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvPlot* pLoopPlot;
	CvPlot* pPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestFoundPlot;
	CvArea* pWaterArea;
	bool bValid;
	int iValue;
	int iBestValue;
	int iI;

	FAssert(getCargo() > 0);
	FAssert(getUnitAICargo(UNITAI_SETTLE) > 0);

	if (!canCargoAllMove())
	{
		return false;
	}

	//New logic should allow some new tricks like
	//unloading settlers when a better site opens up locally
	//and delivering settlers
	//to inland sites

	pWaterArea = plot()->waterArea();
	FAssertMsg(pWaterArea != NULL, "Ship out of water?");

	CvUnit* pSettlerUnit = NULL;
	pPlot = plot();
	pUnitNode = pPlot->headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = pPlot->nextUnitNode(pUnitNode);

		if (pLoopUnit->getTransportUnit() == this)
		{
			if (pLoopUnit->AI_getUnitAIType() == UNITAI_SETTLE)
			{
				pSettlerUnit = pLoopUnit;
				break;
			}
		}
	}

	FAssert(pSettlerUnit != NULL);

	int iAreaBestFoundValue = 0;
	CvPlot* pAreaBestPlot = NULL;

	int iOtherAreaBestFoundValue = 0;
	CvPlot* pOtherAreaBestPlot = NULL;

	for (iI = 0; iI < GET_PLAYER(getOwnerINLINE()).AI_getNumCitySites(); iI++)
	{
		CvPlot* pCitySitePlot = GET_PLAYER(getOwnerINLINE()).AI_getCitySite(iI);
		if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pCitySitePlot, MISSIONAI_FOUND, getGroup()) == 0)
		{
			iValue = pCitySitePlot->getFoundValue(getOwnerINLINE());
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/13/09                                jdog5000      */
/*                                                                                              */
/* Settler AI                                                                                   */
/************************************************************************************************/
/* original bts code
			if (pCitySitePlot->getArea() == getArea())
			{
				if (iValue > iAreaBestFoundValue)
				{
*/
			// Only count city sites we can get to
			if (pCitySitePlot->getArea() == getArea() && pSettlerUnit->generatePath(pCitySitePlot, MOVE_SAFE_TERRITORY, true))
			{
				if (iValue > iAreaBestFoundValue)
				{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
					iAreaBestFoundValue = iValue;
					pAreaBestPlot = pCitySitePlot;
				}
			}
			else
			{
				if (iValue > iOtherAreaBestFoundValue)
				{
					iOtherAreaBestFoundValue = iValue;
					pOtherAreaBestPlot = pCitySitePlot;
				}
			}
		}
	}
	if ((0 == iAreaBestFoundValue) && (0 == iOtherAreaBestFoundValue))
	{
		return false;
	}

	if (iAreaBestFoundValue > iOtherAreaBestFoundValue)
	{
		//let the settler walk.
		getGroup()->unloadAll();
		getGroup()->pushMission(MISSION_SKIP);
		return true;
	}

	iBestValue = 0;
	pBestPlot = NULL;
	pBestFoundPlot = NULL;

	for (iI = 0; iI < GET_PLAYER(getOwnerINLINE()).AI_getNumCitySites(); iI++)
	{
		CvPlot* pCitySitePlot = GET_PLAYER(getOwnerINLINE()).AI_getCitySite(iI);
		if (!(pCitySitePlot->isVisibleEnemyUnit(this)))
		{
			if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pCitySitePlot, MISSIONAI_FOUND, getGroup(), 4) == 0)
			{
				int iPathTurns;
				// BBAI TODO: Nearby plots too if much shorter (settler walk from there)
				// also, if plots are in area player already has cities, then may not be coastal ... (see Earth 1000 AD map for Inca)
				if (generatePath(pCitySitePlot, 0, true, &iPathTurns))
				{
					iValue = pCitySitePlot->getFoundValue(getOwnerINLINE());
					iValue *= 1000;
					iValue /= (2 + iPathTurns);

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pBestPlot = getPathEndTurnPlot();
						pBestFoundPlot = pCitySitePlot;
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestFoundPlot != NULL))
	{
		FAssert(!(pBestPlot->isImpassable()));

		if ((pBestPlot == pBestFoundPlot) || (stepDistance(pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), pBestFoundPlot->getX_INLINE(), pBestFoundPlot->getY_INLINE()) == 1))
		{
			if (atPlot(pBestFoundPlot))
			{
				unloadAll(); // XXX is this dangerous (not pushing a mission...) XXX air units?
				return true;
			}
			else
			{
				getGroup()->pushMission(MISSION_MOVE_TO, pBestFoundPlot->getX_INLINE(), pBestFoundPlot->getY_INLINE(), 0, false, false, MISSIONAI_FOUND, pBestFoundPlot);
				return true;
			}
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_FOUND, pBestFoundPlot);
			return true;
		}
	}

	//Try original logic
	//(sometimes new logic breaks)
	pPlot = plot();

	iBestValue = 0;
	pBestPlot = NULL;
	pBestFoundPlot = NULL;

	int iMinFoundValue = GET_PLAYER(getOwnerINLINE()).AI_getMinFoundValue();

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (pLoopPlot->isCoastalLand())
		{
			iValue = pLoopPlot->getFoundValue(getOwnerINLINE());

			if ((iValue > iBestValue) && (iValue >= iMinFoundValue))
			{
				bValid = false;

				pUnitNode = pPlot->headUnitNode();

				while (pUnitNode != NULL)
				{
					pLoopUnit = ::getUnit(pUnitNode->m_data);
					pUnitNode = pPlot->nextUnitNode(pUnitNode);

					if (pLoopUnit->getTransportUnit() == this)
					{
						if (pLoopUnit->canFound(pLoopPlot))
						{
							bValid = true;
							break;
						}
					}
				}

				if (bValid)
				{
					if (!(pLoopPlot->isVisibleEnemyUnit(this)))
					{
						if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_FOUND, getGroup(), 4) == 0)
						{
							if (generatePath(pLoopPlot, 0, true))
							{
								iBestValue = iValue;
								pBestPlot = getPathEndTurnPlot();
								pBestFoundPlot = pLoopPlot;
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestFoundPlot != NULL))
	{
		FAssert(!(pBestPlot->isImpassable()));

		if ((pBestPlot == pBestFoundPlot) || (stepDistance(pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), pBestFoundPlot->getX_INLINE(), pBestFoundPlot->getY_INLINE()) == 1))
		{
			if (atPlot(pBestFoundPlot))
			{
				unloadAll(); // XXX is this dangerous (not pushing a mission...) XXX air units?
				return true;
			}
			else
			{
				getGroup()->pushMission(MISSION_MOVE_TO, pBestFoundPlot->getX_INLINE(), pBestFoundPlot->getY_INLINE(), 0, false, false, MISSIONAI_FOUND, pBestFoundPlot);
				return true;
			}
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_FOUND, pBestFoundPlot);
			return true;
		}
	}
	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_settlerSeaFerry()
{
	PROFILE_FUNC();

	FAssert(getCargo() > 0);
	FAssert(getUnitAICargo(UNITAI_WORKER) > 0);

	if (!canCargoAllMove())
	{
		return false;
	}

	CvArea* pWaterArea = plot()->waterArea();
	FAssertMsg(pWaterArea != NULL, "Ship out of water?");

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;

	CvCity* pLoopCity;
	int iLoop;
	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		int iValue = pLoopCity->AI_getWorkersNeeded();
		if (iValue > 0)
		{
			iValue -= GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_FOUND, getGroup());
			if (iValue > 0)
			{
				int iPathTurns;
				if (generatePath(pLoopCity->plot(), 0, true, &iPathTurns))
				{
					iValue += std::max(0, (GET_PLAYER(getOwnerINLINE()).AI_neededWorkers(pLoopCity->area()) - GET_PLAYER(getOwnerINLINE()).AI_totalAreaUnitAIs(pLoopCity->area(), UNITAI_WORKER)));
					iValue *= 1000;
					iValue /= 4 + iPathTurns;
					if (atPlot(pLoopCity->plot()))
					{
						iValue += 100;
					}
					else
					{
						iValue += GC.getGame().getSorenRandNum(100, "AI settler sea ferry");
					}
					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pBestPlot = pLoopCity->plot();
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		if (atPlot(pBestPlot))
		{
			unloadAll(); // XXX is this dangerous (not pushing a mission...) XXX air units?
			return true;
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_FOUND, pBestPlot);
			return true;
		}
	}
	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_specialSeaTransportMissionary()
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvCity* pCity;
	CvUnit* pMissionaryUnit;
	CvUnit* pLoopUnit;
	CvPlot* pLoopPlot;
	CvPlot* pPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestSpreadPlot;
	int iPathTurns;
	int iValue;
	int iCorpValue;
	int iBestValue;
	int iI, iJ;
	bool bExecutive = false;

	FAssert(getCargo() > 0);
	FAssert(getUnitAICargo(UNITAI_MISSIONARY) > 0);

	if (!canCargoAllMove())
	{
		return false;
	}

	pPlot = plot();

	pMissionaryUnit = NULL;

	pUnitNode = pPlot->headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = pPlot->nextUnitNode(pUnitNode);

		if (pLoopUnit->getTransportUnit() == this)
		{
			if (pLoopUnit->AI_getUnitAIType() == UNITAI_MISSIONARY)
			{
				pMissionaryUnit = pLoopUnit;
				break;
			}
		}
	}

	if (pMissionaryUnit == NULL)
	{
		return false;
	}

	iBestValue = 0;
	pBestPlot = NULL;
	pBestSpreadPlot = NULL;

	// XXX what about non-coastal cities?
	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (pLoopPlot->isCoastalLand())
		{
			pCity = pLoopPlot->getPlotCity();

			if (pCity != NULL)
			{
				iValue = 0;
				iCorpValue = 0;

				for (iJ = 0; iJ < GC.getNumReligionInfos(); iJ++)
				{
					if (pMissionaryUnit->canSpread(pLoopPlot, ((ReligionTypes)iJ)))
					{
						if (GET_PLAYER(getOwnerINLINE()).getStateReligion() == ((ReligionTypes)iJ))
						{
							iValue += 3;
						}

						if (GET_PLAYER(getOwnerINLINE()).hasHolyCity((ReligionTypes)iJ))
						{
							iValue++;
						}
					}
				}

				for (iJ = 0; iJ < GC.getNumCorporationInfos(); iJ++)
				{
					if (pMissionaryUnit->canSpreadCorporation(pLoopPlot, ((CorporationTypes)iJ)))
					{
						if (GET_PLAYER(getOwnerINLINE()).hasHeadquarters((CorporationTypes)iJ))
						{
							iCorpValue += 3;
						}
					}
				}

				if (iValue > 0)
				{
					if (!(pLoopPlot->isVisibleEnemyUnit(this)))
					{
						if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_SPREAD, getGroup()) == 0)
						{
							if (generatePath(pLoopPlot, 0, true, &iPathTurns))
							{
								iValue *= pCity->getPopulation();

								if (pCity->getOwnerINLINE() == getOwnerINLINE())
								{
									iValue *= 4;
								}
								else if (pCity->getTeam() == getTeam())
								{
									iValue *= 3;
								}

								if (pCity->getReligionCount() == 0)
								{
									iValue *= 2;
								}

								iValue /= (pCity->getReligionCount() + 1);

								FAssert(iPathTurns > 0);

								if (iPathTurns == 1)
								{
									iValue *= 2;
								}

								iValue *= 1000;

								iValue /= (iPathTurns + 1);

								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									pBestPlot = getPathEndTurnPlot();
									pBestSpreadPlot = pLoopPlot;
									bExecutive = false;
								}
							}
						}
					}
				}

				if (iCorpValue > 0)
				{
					if (!(pLoopPlot->isVisibleEnemyUnit(this)))
					{
						if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_SPREAD_CORPORATION, getGroup()) == 0)
						{
							if (generatePath(pLoopPlot, 0, true, &iPathTurns))
							{
								iCorpValue *= pCity->getPopulation();

								FAssert(iPathTurns > 0);

								if (iPathTurns == 1)
								{
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       02/22/10                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original bts code
									iValue *= 2;
*/
									iCorpValue *= 2;
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
								}

								iCorpValue *= 1000;

								iCorpValue /= (iPathTurns + 1);

								if (iCorpValue > iBestValue)
								{
									iBestValue = iCorpValue;
									pBestPlot = getPathEndTurnPlot();
									pBestSpreadPlot = pLoopPlot;
									bExecutive = true;
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestSpreadPlot != NULL))
	{
		FAssert(!(pBestPlot->isImpassable()) || canMoveImpassable());

		if ((pBestPlot == pBestSpreadPlot) || (stepDistance(pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), pBestSpreadPlot->getX_INLINE(), pBestSpreadPlot->getY_INLINE()) == 1))
		{
			if (atPlot(pBestSpreadPlot))
			{
				unloadAll(); // XXX is this dangerous (not pushing a mission...) XXX air units?
				return true;
			}
			else
			{
				getGroup()->pushMission(MISSION_MOVE_TO, pBestSpreadPlot->getX_INLINE(), pBestSpreadPlot->getY_INLINE(), 0, false, false, bExecutive ? MISSIONAI_SPREAD_CORPORATION : MISSIONAI_SPREAD, pBestSpreadPlot);
				return true;
			}
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, bExecutive ? MISSIONAI_SPREAD_CORPORATION : MISSIONAI_SPREAD, pBestSpreadPlot);
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_specialSeaTransportSpy()
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	CvPlot* pBestSpyPlot;
	PlayerTypes eBestPlayer;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iI;

	FAssert(getCargo() > 0);
	FAssert(getUnitAICargo(UNITAI_SPY) > 0);

	if (!canCargoAllMove())
	{
		return false;
	}

	iBestValue = 0;
	eBestPlayer = NO_PLAYER;

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() != getTeam())
			{
				if (GET_PLAYER(getOwnerINLINE()).AI_getAttitude((PlayerTypes)iI) <= ATTITUDE_ANNOYED)
				{
					iValue = GET_PLAYER((PlayerTypes)iI).getTotalPopulation();

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						eBestPlayer = ((PlayerTypes)iI);
					}
				}
			}
		}
	}

	if (eBestPlayer == NO_PLAYER)
	{
		return false;
	}

	pBestPlot = NULL;
	pBestSpyPlot = NULL;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (pLoopPlot->isCoastalLand())
		{
			if (pLoopPlot->getOwnerINLINE() == eBestPlayer)
			{
				iValue = pLoopPlot->area()->getCitiesPerPlayer(eBestPlayer);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/23/10                                jdog5000      */
/*                                                                                              */
/* Efficiency                                                                                   */
/************************************************************************************************/
				iValue *= 1000;

				if (iValue > iBestValue)
				{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
					if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_ATTACK_SPY, getGroup(), 4) == 0)
					{
						if (generatePath(pLoopPlot, 0, true, &iPathTurns))
						{
							iValue /= (iPathTurns + 1);

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = getPathEndTurnPlot();
								pBestSpyPlot = pLoopPlot;
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestSpyPlot != NULL))
	{
		FAssert(!(pBestPlot->isImpassable()));

		if ((pBestPlot == pBestSpyPlot) || (stepDistance(pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), pBestSpyPlot->getX_INLINE(), pBestSpyPlot->getY_INLINE()) == 1))
		{
			if (atPlot(pBestSpyPlot))
			{
				unloadAll(); // XXX is this dangerous (not pushing a mission...) XXX air units?
				return true;
			}
			else
			{
				getGroup()->pushMission(MISSION_MOVE_TO, pBestSpyPlot->getX_INLINE(), pBestSpyPlot->getY_INLINE(), 0, false, false, MISSIONAI_ATTACK_SPY, pBestSpyPlot);
				return true;
			}
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_ATTACK_SPY, pBestSpyPlot);
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_carrierSeaTransport()
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pLoopPlotAir;
	CvPlot* pBestPlot;
	CvPlot* pBestCarrierPlot;
	int iMaxAirRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iDX, iDY;
	int iI;

	iMaxAirRange = 0;

	std::vector<CvUnit*> aCargoUnits;
	getCargoUnits(aCargoUnits);
	for (uint i = 0; i < aCargoUnits.size(); ++i)
	{
		iMaxAirRange = std::max(iMaxAirRange, aCargoUnits[i]->airRange());
	}

	if (iMaxAirRange == 0)
	{
		return false;
	}

	iBestValue = 0;
	pBestPlot = NULL;
	pBestCarrierPlot = NULL;

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/22/10                                jdog5000      */
/*                                                                                              */
/* Naval AI, War tactics, Efficiency                                                            */
/************************************************************************************************/
	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (AI_plotValid(pLoopPlot))
		{
			if (pLoopPlot->isAdjacentToLand())
			{
				if (!(pLoopPlot->isVisibleEnemyUnit(this)))
				{
					iValue = 0;

					for (iDX = -(iMaxAirRange); iDX <= iMaxAirRange; iDX++)
					{
						for (iDY = -(iMaxAirRange); iDY <= iMaxAirRange; iDY++)
						{
							pLoopPlotAir = plotXY(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), iDX, iDY);

							if (pLoopPlotAir != NULL)
							{
								if (plotDistance(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), pLoopPlotAir->getX_INLINE(), pLoopPlotAir->getY_INLINE()) <= iMaxAirRange)
								{
									if (!(pLoopPlotAir->isBarbarian()))
									{
										if (potentialWarAction(pLoopPlotAir))
										{
											if (pLoopPlotAir->isCity())
											{
												iValue += 3;

												// BBAI: Support invasions
												iValue += (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlotAir, MISSIONAI_ASSAULT, getGroup(), 2) * 6);
											}

											if (pLoopPlotAir->getImprovementType() != NO_IMPROVEMENT)
											{
												iValue += 2;
											}

											if (plotDistance(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), pLoopPlotAir->getX_INLINE(), pLoopPlotAir->getY_INLINE()) <= iMaxAirRange/2)
											{
												// BBAI: Support/air defense for land troops
												iValue += pLoopPlotAir->plotCount(PUF_canDefend, -1, -1, getOwnerINLINE());
											}
										}
									}
								}
							}
						}
					}

					if( iValue > 0 )
					{
						iValue *= 1000;

						for (int iDirection = 0; iDirection < NUM_DIRECTION_TYPES; iDirection++)
						{
							CvPlot* pDirectionPlot = plotDirection(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), (DirectionTypes)iDirection);
							if (pDirectionPlot != NULL)
							{
								if (pDirectionPlot->isCity() && isEnemy(pDirectionPlot->getTeam(), pLoopPlot))
								{
									iValue /= 2;
									break;
								}
							}
						}

						if (iValue > iBestValue)
						{
/*************************************************************************************************/
/**	CandyMan								04/04/09								Xienwolf	**/
/**																								**/
/**							Allows Multiple Invisible types on a Unit							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
							bool bStealth = (getInvisibleType() != NO_INVISIBLE);
/**								----  End Original Code  ----									**/
							bool bStealth = (getNumInvisibleTypes() > 0);
/*************************************************************************************************/
/**	CandyMan								END													**/
/*************************************************************************************************/
							if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_CARRIER, getGroup(), bStealth ? 5 : 3) <= (bStealth ? 0 : 3))
							{
								if (generatePath(pLoopPlot, 0, true, &iPathTurns))
								{
									iValue /= (iPathTurns + 1);

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = getPathEndTurnPlot();
										pBestCarrierPlot = pLoopPlot;
									}
								}
							}
						}
					}
				}
			}
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if ((pBestPlot != NULL) && (pBestCarrierPlot != NULL))
	{
		if (atPlot(pBestCarrierPlot))
		{
			if (getGroup()->hasCargo())
			{
				CvPlot* pPlot = plot();

				int iNumUnits = pPlot->getNumUnits();

				for (int i = 0; i < iNumUnits; ++i)
				{
					bool bDone = true;
					CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();
					while (pUnitNode != NULL)
					{
						CvUnit* pCargoUnit = ::getUnit(pUnitNode->m_data);
						pUnitNode = pPlot->nextUnitNode(pUnitNode);

						if (pCargoUnit->isCargo())
						{
							FAssert(pCargoUnit->getTransportUnit() != NULL);
							if (pCargoUnit->getOwnerINLINE() == getOwnerINLINE() && (pCargoUnit->getTransportUnit()->getGroup() == getGroup()) && (pCargoUnit->getDomainType() == DOMAIN_AIR))
							{
								if (pCargoUnit->canMove() && pCargoUnit->isGroupHead())
								{
									// careful, this might kill the cargo group
									if (pCargoUnit->getGroup()->AI_update())
									{
										bDone = false;
										break;
									}
								}
							}
						}
					}

					if (bDone)
					{
						break;
					}
				}
			}

			if (canPlunder(pBestCarrierPlot))
			{
				getGroup()->pushMission(MISSION_PLUNDER, -1, -1, 0, false, false, MISSIONAI_CARRIER, pBestCarrierPlot);
			}
			else
			{
				getGroup()->pushMission(MISSION_SKIP);
			}
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_CARRIER, pBestCarrierPlot);
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_connectPlot(CvPlot* pPlot, int iRange)
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	int iLoop;

	FAssert(canBuildRoute());
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
	// BBAI efficiency: check area for land units before generating paths
	if( (getDomainType() == DOMAIN_LAND) && (pPlot->area() != area()) && !(getGroup()->canMoveAllTerrain()) )
	{
		return false;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

/*************************************************************************************************/
/**	Improved AI							02/07/10										Snarko	**/
/**																								**/
/**								Making the AI build forts										**/
/**			If you change your mind in the middle of nowhere don't build a road back home		**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	if (!(pPlot->isVisibleEnemyUnit(this)))
/**								----  End Original Code  ----									**/
	if (!(pPlot->isVisibleEnemyUnit(this)) && plot()->getTeam() == getTeam())
/*************************************************************************************************/
/**	Improved AI								END													**/
/*************************************************************************************************/
	{
		if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pPlot, MISSIONAI_BUILD, getGroup(), iRange) == 0)
		{
			if (generatePath(pPlot, MOVE_SAFE_TERRITORY, true))
			{
				for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
				{
					if (!(pPlot->isConnectedTo(pLoopCity)))
					{
						FAssertMsg(pPlot->getPlotCity() != pLoopCity, "pPlot->getPlotCity() is not expected to be equal with pLoopCity");

						if (plot()->getPlotGroup(getOwnerINLINE()) == pLoopCity->plot()->getPlotGroup(getOwnerINLINE()))
						{
							getGroup()->pushMission(MISSION_ROUTE_TO, pPlot->getX_INLINE(), pPlot->getY_INLINE(), MOVE_SAFE_TERRITORY, false, false, MISSIONAI_BUILD, pPlot);
							return true;
						}
					}
				}

				for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
				{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
					// BBAI efficiency: check same area
					if( (pLoopCity->area() != pPlot->area()) )
					{
						continue;
					}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

					if (!(pPlot->isConnectedTo(pLoopCity)))
					{
						FAssertMsg(pPlot->getPlotCity() != pLoopCity, "pPlot->getPlotCity() is not expected to be equal with pLoopCity");

						if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
						{
							if (generatePath(pLoopCity->plot(), MOVE_SAFE_TERRITORY, true))
							{
								if (atPlot(pPlot)) // need to test before moving...
								{
									getGroup()->pushMission(MISSION_ROUTE_TO, pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(), MOVE_SAFE_TERRITORY, false, false, MISSIONAI_BUILD, pPlot);
								}
								else
								{
									getGroup()->pushMission(MISSION_ROUTE_TO, pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(), MOVE_SAFE_TERRITORY, false, false, MISSIONAI_BUILD, pPlot);
									getGroup()->pushMission(MISSION_ROUTE_TO, pPlot->getX_INLINE(), pPlot->getY_INLINE(), MOVE_SAFE_TERRITORY, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BUILD, pPlot);
								}

								return true;
							}
						}
					}
				}
			}
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_improveCity(CvCity* pCity)
{
	PROFILE_FUNC();

	CvPlot* pBestPlot;
	BuildTypes eBestBuild;
	MissionTypes eMission;

	if (AI_bestCityBuild(pCity, &pBestPlot, &eBestBuild, NULL, this))
	{
		FAssertMsg(pBestPlot != NULL, "BestPlot is not assigned a valid value");
		FAssertMsg(eBestBuild != NO_BUILD, "BestBuild is not assigned a valid value");
		FAssertMsg(eBestBuild < GC.getNumBuildInfos(), "BestBuild is assigned a corrupt value");
/*************************************************************************************************/
/**	Improved AI							02/07/10										Snarko	**/
/**																								**/
/**								Making the AI build forts										**/
/**			If you change your mind in the middle of nowhere don't build a road back home		**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		if ((plot()->getWorkingCity() != pCity) || (GC.getBuildInfo(eBestBuild).getRoute() != NO_ROUTE))
/**								----  End Original Code  ----									**/
/*************************************************************************************************/
/**	Improved AI								END													**/
/*************************************************************************************************/
		if ((plot()->getTeam() == getTeam()) && ((plot()->getWorkingCity() != pCity) || (GC.getBuildInfo(eBestBuild).getRoute() != NO_ROUTE)))
		{
			eMission = MISSION_ROUTE_TO;
		}
		else
		{
			eMission = MISSION_MOVE_TO;
/*************************************************************************************************/
/**	TipToe									04/04/09								Xienwolf	**/
/**																								**/
/**						Safe Movement is ideal situation where possible							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
			if (NULL != pBestPlot && generatePath(pBestPlot) && (getPathLastNode()->m_iData2 == 1) && (getPathLastNode()->m_iData1 == 0))
/**								----  End Original Code  ----									**/
			if (NULL != pBestPlot && generatePath(pBestPlot, MOVE_SAFE_TERRITORY) && (getPathLastNode()->m_iData2 == 1) && (getPathLastNode()->m_iData1 == 0))
/*************************************************************************************************/
/**	TipToe									END													**/
/*************************************************************************************************/
			{
				if (pBestPlot->getRouteType() != NO_ROUTE)
				{
					eMission = MISSION_ROUTE_TO;
				}
			}
/*************************************************************************************************/
/**	Improved AI							02/07/10										Snarko	**/
/**																								**/
/**								Making the AI build forts										**/
/**			If you change your mind in the middle of nowhere don't build a road back home		**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
			else if (plot()->getRouteType() == NO_ROUTE)
/**								----  End Original Code  ----									**/
			else if ((plot()->getTeam() == getTeam()) && (plot()->getRouteType() == NO_ROUTE))
/*************************************************************************************************/
/**	Improved AI								END													**/
/*************************************************************************************************/
			{
				int iPlotMoveCost = 0;
/*************************************************************************************************/
/**	Xienwolf Tweak							03/18/09											**/
/**																								**/
/**						Don't bother building routes you don't really need						**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
				iPlotMoveCost = ((plot()->getFeatureType() == NO_FEATURE) ? GC.getTerrainInfo(plot()->getTerrainType()).getMovementCost() : GC.getFeatureInfo(plot()->getFeatureType()).getMovementCost());

				if (plot()->isHills())
/**								----  End Original Code  ----									**/
				iPlotMoveCost = ((plot()->getFeatureType() == NO_FEATURE) ? (isTerrainDoubleMove(plot()->getTerrainType()) ? 1 : GC.getTerrainInfo(plot()->getTerrainType()).getMovementCost()) : (isFeatureDoubleMove(plot()->getFeatureType()) ? 1 : GC.getFeatureInfo(plot()->getFeatureType()).getMovementCost()));

				if (plot()->isHills() && !isHillsDoubleMove())
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
				{
					iPlotMoveCost += GC.getHILLS_EXTRA_MOVEMENT();
				}

/*************************************************************************************************/
/**	Mountain Mod by NeverMind 		imported by Ahwaric	19.09.09		**/
/*************************************************************************************************/
				if (plot()->isPeak())
				{
					iPlotMoveCost += GC.getPEAK_EXTRA_MOVEMENT();
				}
/*************************************************************************************************/
/**	Mountain Mod	END									**/
/*************************************************************************************************/

				if (iPlotMoveCost > 1)
				{
					eMission = MISSION_ROUTE_TO;
				}
			}
		}

		eBestBuild = AI_betterPlotBuild(pBestPlot, eBestBuild);

		getGroup()->pushMission(eMission, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_BUILD, pBestPlot);
		getGroup()->pushMission(MISSION_BUILD, eBestBuild, -1, 0, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BUILD, pBestPlot);

		return true;
	}

	return false;
}

bool CvUnitAI::AI_improveLocalPlot(int iRange, CvCity* pIgnoreCity)
{

	int iX, iY;

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	BuildTypes eBestBuild = NO_BUILD;

	for (iX = -iRange; iX <= iRange; iX++)
	{
		for (iY = -iRange; iY <= iRange; iY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iX, iY);
			if ((pLoopPlot != NULL) && (pLoopPlot->isCityRadius()))
			{
				CvCity* pCity = pLoopPlot->getWorkingCity();
				if ((NULL != pCity) && (pCity->getOwnerINLINE() == getOwnerINLINE()))
				{
					if ((NULL == pIgnoreCity) || (pCity != pIgnoreCity))
					{
						if (AI_plotValid(pLoopPlot))
						{
							int iIndex = pCity->getCityPlotIndex(pLoopPlot);
							if (iIndex != CITY_HOME_PLOT)
							{
								if (((NULL == pIgnoreCity) || ((pCity->AI_getWorkersNeeded() > 0) && (pCity->AI_getWorkersHave() < (1 + pCity->AI_getWorkersNeeded() * 2 / 3)))) && (pCity->AI_getBestBuild(iIndex) != NO_BUILD))
								{
									if (canBuild(pLoopPlot, pCity->AI_getBestBuild(iIndex)))
									{
										bool bAllowed = true;

/*************************************************************************************************/
/**	Xienwolf Tweak							03/18/09											**/
/**																								**/
/**									Can't build if you can't clear...							**/
/*************************************************************************************************/
										if (pLoopPlot->getImprovementType() != NO_IMPROVEMENT && GC.getImprovementInfo(pLoopPlot->getImprovementType()).isPermanent())
										{
											bAllowed = false;
										}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
										if (GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_SAFE_AUTOMATION))
										{
											if (pLoopPlot->getImprovementType() != NO_IMPROVEMENT && pLoopPlot->getImprovementType() != GC.getDefineINT("RUINS_IMPROVEMENT"))
											{
												bAllowed = false;
											}
										}

										if (bAllowed)
										{
/*************************************************************************************************/
/**	Xienwolf Tweak							03/18/09											**/
/**																								**/
/**		Already checked for Safe_Automation, let them rebuild over something if they want to	**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
											if (pLoopPlot->getImprovementType() != NO_IMPROVEMENT && GC.getBuildInfo(pCity->AI_getBestBuild(iIndex)).getImprovement() != NO_IMPROVEMENT)
/**								----  End Original Code  ----									**/
											if (pLoopPlot->getImprovementType() != NO_IMPROVEMENT && GC.getBuildInfo(pCity->AI_getBestBuild(iIndex)).getImprovement() == pLoopPlot->getImprovementType())
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
											{
												bAllowed = false;
											}
										}

										if (bAllowed)
										{
											int iValue = pCity->AI_getBestBuildValue(iIndex);
											int iPathTurns;
/*************************************************************************************************/
/**	TipToe									04/04/09								Xienwolf	**/
/**																								**/
/**						Safe Movement is ideal situation where possible							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
											if (generatePath(pLoopPlot, 0, true, &iPathTurns))
/**								----  End Original Code  ----									**/
											if (generatePath(pLoopPlot, MOVE_SAFE_TERRITORY, true, &iPathTurns))
/*************************************************************************************************/
/**	TipToe									END													**/
/*************************************************************************************************/
											{
												int iMaxWorkers = 1;
												if (plot() == pLoopPlot)
												{
													iValue *= 3;
													iValue /= 2;
												}
												else if (getPathLastNode()->m_iData1 == 0)
												{
													iPathTurns++;
												}
												else if (iPathTurns <= 1)
												{
													iMaxWorkers = AI_calculatePlotWorkersNeeded(pLoopPlot, pCity->AI_getBestBuild(iIndex));
												}

												if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_BUILD, getGroup()) < iMaxWorkers)
												{
													iValue *= 1000;
													iValue /= 1 + iPathTurns;

													if (iValue > iBestValue)
													{
														iBestValue = iValue;
														pBestPlot = pLoopPlot;
														eBestBuild = pCity->AI_getBestBuild(iIndex);
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssertMsg(eBestBuild != NO_BUILD, "BestBuild is not assigned a valid value");
		FAssertMsg(eBestBuild < GC.getNumBuildInfos(), "BestBuild is assigned a corrupt value");

		FAssert(pBestPlot->getWorkingCity() != NULL);
		if (NULL != pBestPlot->getWorkingCity())
		{
			pBestPlot->getWorkingCity()->AI_changeWorkersHave(+1);

			if (plot()->getWorkingCity() != NULL)
			{
				plot()->getWorkingCity()->AI_changeWorkersHave(-1);
			}
		}
		MissionTypes eMission = MISSION_MOVE_TO;

		int iPathTurns;
/*************************************************************************************************/
/**	TipToe									04/04/09								Xienwolf	**/
/**																								**/
/**						Safe Movement is ideal situation where possible							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		if (generatePath(pBestPlot, 0, true, &iPathTurns) && (getPathLastNode()->m_iData2 == 1) && (getPathLastNode()->m_iData1 == 0))
/**								----  End Original Code  ----									**/
		if (generatePath(pBestPlot, MOVE_SAFE_TERRITORY, true, &iPathTurns) && (getPathLastNode()->m_iData2 == 1) && (getPathLastNode()->m_iData1 == 0))
/*************************************************************************************************/
/**	TipToe									END													**/
/*************************************************************************************************/
		{
			if (pBestPlot->getRouteType() != NO_ROUTE)
			{
				eMission = MISSION_ROUTE_TO;
			}
		}
		else if (plot()->getRouteType() == NO_ROUTE)
		{
			int iPlotMoveCost = 0;
/*************************************************************************************************/
/**	Xienwolf Tweak							03/18/09											**/
/**																								**/
/**						Don't bother building routes you don't really need						**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
			iPlotMoveCost = ((plot()->getFeatureType() == NO_FEATURE) ? GC.getTerrainInfo(plot()->getTerrainType()).getMovementCost() : GC.getFeatureInfo(plot()->getFeatureType()).getMovementCost());

			if (plot()->isHills())
/**								----  End Original Code  ----									**/
			iPlotMoveCost = ((plot()->getFeatureType() == NO_FEATURE) ? (isTerrainDoubleMove(plot()->getTerrainType()) ? 1 : GC.getTerrainInfo(plot()->getTerrainType()).getMovementCost()) : (isFeatureDoubleMove(plot()->getFeatureType()) ? 1 : GC.getFeatureInfo(plot()->getFeatureType()).getMovementCost()));

			if (plot()->isHills() && !isHillsDoubleMove())
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
			{
				iPlotMoveCost += GC.getHILLS_EXTRA_MOVEMENT();
			}

/*************************************************************************************************/
/**	Mountain Mod by NeverMind 		imported by Ahwaric	19.09.09		**/
/*************************************************************************************************/
			if (plot()->isPeak())
			{
				iPlotMoveCost += GC.getPEAK_EXTRA_MOVEMENT();
			}
/*************************************************************************************************/
/**	Mountain Mod	END									**/
/*************************************************************************************************/

			if (iPlotMoveCost > 1)
			{
				eMission = MISSION_ROUTE_TO;
			}
		}

		eBestBuild = AI_betterPlotBuild(pBestPlot, eBestBuild);

		getGroup()->pushMission(eMission, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_BUILD, pBestPlot);
		getGroup()->pushMission(MISSION_BUILD, eBestBuild, -1, 0, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BUILD, pBestPlot);
		return true;
	}

	return false;
}

// Returns true if a mission was pushed...
bool CvUnitAI::AI_nextCityToImprove(CvCity* pCity)
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	CvPlot* pPlot;
	CvPlot* pBestPlot;
	BuildTypes eBuild;
	BuildTypes eBestBuild;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iLoop;

	iBestValue = 0;
	eBestBuild = NO_BUILD;
	pBestPlot = NULL;

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if (pLoopCity != pCity)
		{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/22/10                                jdog5000      */
/*                                                                                              */
/* Worker AI, Efficiency                                                                        */
/************************************************************************************************/
			// BBAI efficiency: check area for land units before path generation
			if( (getDomainType() == DOMAIN_LAND) && (pLoopCity->area() != area()) && !(getGroup()->canMoveAllTerrain()) )
			{
				continue;
			}

			//iValue = pLoopCity->AI_totalBestBuildValue(area());
			int iWorkersNeeded = pLoopCity->AI_getWorkersNeeded();
			int iWorkersHave = pLoopCity->AI_getWorkersHave();

			iValue = std::max(0, iWorkersNeeded - iWorkersHave) * 100;
			iValue += iWorkersNeeded * 10;
			iValue *= (iWorkersNeeded + 1);
			iValue /= (iWorkersHave + 1);

			if (iValue > 0)
			{
				if (AI_bestCityBuild(pLoopCity, &pPlot, &eBuild, NULL, this))
				{
					FAssert(pPlot != NULL);
					FAssert(eBuild != NO_BUILD);

					if( AI_plotValid(pPlot) )
					{
						iValue *= 1000;

						if (pLoopCity->isCapital())
						{
							iValue *= 2;
						}

						if( iValue > iBestValue )
						{
/*************************************************************************************************/
/**	TipToe									04/04/09								Xienwolf	**/
/**																								**/
/**						Safe Movement is ideal situation where possible							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
							generatePath(pPlot, 0, true, &iPathTurns);
/**								----  End Original Code  ----									**/
							generatePath(pPlot, MOVE_SAFE_TERRITORY, true, &iPathTurns);
/*************************************************************************************************/
/**	TipToe									END													**/
/*************************************************************************************************/
							{
								iValue /= (iPathTurns + 1);

/*************************************************************************************************/
/**	JohnHenry								04/04/09								Xienwolf	**/
/**																								**/
/**			Allows checking of properly scaled BuildTimes without going through a unit			**/
/*************************************************************************************************/
								iValue *= 100;
								iValue /= pPlot->getBuildTurnsLeft(eBuild, 0, 0, true, getTeam());
/*************************************************************************************************/
/**	JohnHenry								END													**/
/*************************************************************************************************/
								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									eBestBuild = eBuild;
									pBestPlot = pPlot;
									FAssert(!atPlot(pBestPlot) || NULL == pCity || pCity->AI_getWorkersNeeded() == 0 || pCity->AI_getWorkersHave() > pCity->AI_getWorkersNeeded() + 1);
								}
							}
						}
					}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssertMsg(eBestBuild != NO_BUILD, "BestBuild is not assigned a valid value");
		FAssertMsg(eBestBuild < GC.getNumBuildInfos(), "BestBuild is assigned a corrupt value");
		if (plot()->getWorkingCity() != NULL)
		{
			plot()->getWorkingCity()->AI_changeWorkersHave(-1);
		}

		FAssert(pBestPlot->getWorkingCity() != NULL || GC.getBuildInfo(eBestBuild).getImprovement() == NO_IMPROVEMENT);
		if (NULL != pBestPlot->getWorkingCity())
		{
			pBestPlot->getWorkingCity()->AI_changeWorkersHave(+1);
		}

		eBestBuild = AI_betterPlotBuild(pBestPlot, eBestBuild);

/*************************************************************************************************/
/**	Improved AI							02/07/10										Snarko	**/
/**																								**/
/**								Making the AI build forts										**/
/**			If you change your mind in the middle of nowhere don't build a road back home		**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		getGroup()->pushMission(MISSION_ROUTE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_BUILD, pBestPlot);
/**								----  End Original Code  ----									**/
		if (plot()->getTeam() == getTeam())
		{
			getGroup()->pushMission(MISSION_ROUTE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_BUILD, pBestPlot);
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_BUILD, pBestPlot);
		}
/*************************************************************************************************/
/**	Improved AI								END													**/
/*************************************************************************************************/
		getGroup()->pushMission(MISSION_BUILD, eBestBuild, -1, 0, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BUILD, pBestPlot);
		return true;
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_nextCityToImproveAirlift()
{
	PROFILE_FUNC();

	CvCity* pCity;
	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	int iValue;
	int iBestValue;
	int iLoop;

	if (getGroup()->getNumUnits() > 1)
	{
		return false;
	}

	pCity = plot()->getPlotCity();

	if (pCity == NULL)
	{
		return false;
	}

	if (pCity->getMaxAirlift() == 0)
	{
		return false;
	}

	iBestValue = 0;
	pBestPlot = NULL;

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if (pLoopCity != pCity)
		{
			if (canAirliftAt(pCity->plot(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE()))
			{
				iValue = pLoopCity->AI_totalBestBuildValue(pLoopCity->area());

				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					pBestPlot = pLoopCity->plot();
					FAssert(pLoopCity != pCity);
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_AIRLIFT, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_irrigateTerritory()
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	ImprovementTypes eImprovement;
	BuildTypes eBuild;
	BuildTypes eBestBuild;
	BuildTypes eBestTempBuild;
	BonusTypes eNonObsoleteBonus;
	bool bValid;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iBestTempBuildValue;
	int iI, iJ;

	iBestValue = 0;
	eBestBuild = NO_BUILD;
	pBestPlot = NULL;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (AI_plotValid(pLoopPlot))
		{
			if (pLoopPlot->getOwnerINLINE() == getOwnerINLINE()) // XXX team???
			{
				if (pLoopPlot->getWorkingCity() == NULL)
				{
					eImprovement = pLoopPlot->getImprovementType();

					if ((eImprovement == NO_IMPROVEMENT) || !(GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_SAFE_AUTOMATION) && !(eImprovement == (GC.getDefineINT("RUINS_IMPROVEMENT")))))
					{
						if ((eImprovement == NO_IMPROVEMENT) || !(GC.getImprovementInfo(eImprovement).isCarriesIrrigation()))
						{
							eNonObsoleteBonus = pLoopPlot->getNonObsoleteBonusType(getTeam());

							if ((eImprovement == NO_IMPROVEMENT) || (eNonObsoleteBonus == NO_BONUS) || !(GC.getImprovementInfo(eImprovement).isImprovementBonusTrade(eNonObsoleteBonus)))
							{
								if (pLoopPlot->isIrrigationAvailable(true))
								{
									iBestTempBuildValue = MAX_INT;
									eBestTempBuild = NO_BUILD;

									for (iJ = 0; iJ < GC.getNumBuildInfos(); iJ++)
									{
										eBuild = ((BuildTypes)iJ);
										FAssertMsg(eBuild < GC.getNumBuildInfos(), "Invalid Build");

										if (GC.getBuildInfo(eBuild).getImprovement() != NO_IMPROVEMENT)
										{
											if (GC.getImprovementInfo((ImprovementTypes)(GC.getBuildInfo(eBuild).getImprovement())).isCarriesIrrigation())
											{
												if (canBuild(pLoopPlot, eBuild))
												{
/*************************************************************************************************/
/**	Choppers							12/07/08									Xienwolf	**/
/**																								**/
/**					Allows Discounting of Build Times based on known Technologies				**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
													iValue = 10000;

													iValue /= (GC.getBuildInfo(eBuild).getTime() + 1);
/**								----  End Original Code  ----									**/
													iValue = pLoopPlot->getBuildTurnsLeft(eBuild, 0, 0, true, getTeam());
/*************************************************************************************************/
/**	Choppers								END													**/
/*************************************************************************************************/

													// XXX feature production???

													if (iValue < iBestTempBuildValue)
													{
														iBestTempBuildValue = iValue;
														eBestTempBuild = eBuild;
													}
												}
											}
										}
									}

									if (eBestTempBuild != NO_BUILD)
									{
										bValid = true;

										if (GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_LEAVE_FORESTS))
										{
											if (pLoopPlot->getFeatureType() != NO_FEATURE)
											{
												if (GC.getBuildInfo(eBestTempBuild).isFeatureRemove(pLoopPlot->getFeatureType())

//FfH: Added by Kael 04/24/2008
												  && !GC.getCivilizationInfo(getCivilizationType()).isMaintainFeatures(pLoopPlot->getFeatureType())
//FfH: End Add

												)
												{
/*************************************************************************************************/
/**	CivPlotMods								03/31/09								Jean Elcard	**/
/**																								**/
/**						Consider Player-specific Feature Yield Changes.							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
													if (GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange(YIELD_PRODUCTION) > 0)
/**								----  End Original Code  ----									**/
													if ((GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange(YIELD_PRODUCTION) + GET_PLAYER(getOwnerINLINE()).getFeatureYieldChange(pLoopPlot->getFeatureType(), YIELD_PRODUCTION)) > 0)
/*************************************************************************************************/
/**	CivPlotMods								END													**/
/*************************************************************************************************/
													{
														bValid = false;
													}
												}
											}
										}

										if (bValid)
										{
											if (!(pLoopPlot->isVisibleEnemyUnit(this)))
											{
												if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_BUILD, getGroup(), 1) == 0)
												{
/*************************************************************************************************/
/**	TipToe									04/04/09								Xienwolf	**/
/**																								**/
/**						Safe Movement is ideal situation where possible							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
													if (generatePath(pLoopPlot, 0, true, &iPathTurns)) // XXX should this actually be at the top of the loop? (with saved paths and all...)
/**								----  End Original Code  ----									**/
													if (generatePath(pLoopPlot, MOVE_SAFE_TERRITORY, true, &iPathTurns)) // XXX should this actually be at the top of the loop? (with saved paths and all...)
/*************************************************************************************************/
/**	TipToe									END													**/
/*************************************************************************************************/
													{
														iValue = 10000;

														iValue /= (iPathTurns + 1);

														if (iValue > iBestValue)
														{
															iBestValue = iValue;
															eBestBuild = eBestTempBuild;
															pBestPlot = pLoopPlot;
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssertMsg(eBestBuild != NO_BUILD, "BestBuild is not assigned a valid value");
		FAssertMsg(eBestBuild < GC.getNumBuildInfos(), "BestBuild is assigned a corrupt value");

/*************************************************************************************************/
/**	Improved AI							02/07/10										Snarko	**/
/**																								**/
/**								Making the AI build forts										**/
/**			If you change your mind in the middle of nowhere don't build a road back home		**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		getGroup()->pushMission(MISSION_ROUTE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_BUILD, pBestPlot);
/**								----  End Original Code  ----									**/
		if (plot()->getTeam() == getTeam())
		{
			getGroup()->pushMission(MISSION_ROUTE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_BUILD, pBestPlot);
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_BUILD, pBestPlot);
		}
/*************************************************************************************************/
/**	Improved AI								END													**/
/*************************************************************************************************/
		getGroup()->pushMission(MISSION_BUILD, eBestBuild, -1, 0, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BUILD, pBestPlot);

		return true;
	}

	return false;
}

bool CvUnitAI::AI_fortTerritory(bool bCanal, bool bAirbase)
{
	PROFILE_FUNC();

	int iBestValue = 0;
	BuildTypes eBestBuild = NO_BUILD;
	CvPlot* pBestPlot = NULL;

/*************************************************************************************************/
/**	Xienwolf Tweak							03/18/09											**/
/**																								**/
/**		No Forts on Workable Tiles unless requested by BestBuild.  And no Canals/Airports!		**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (AI_plotValid(pLoopPlot))
		{
			if (pLoopPlot->getOwnerINLINE() == getOwnerINLINE()) // XXX team???
			{
				if (pLoopPlot->getImprovementType() == NO_IMPROVEMENT)
				{
					int iValue = 0;
					iValue += bCanal ? kOwner.AI_getPlotCanalValue(pLoopPlot) : 0;
					iValue += bAirbase ? kOwner.AI_getPlotAirbaseValue(pLoopPlot) : 0;

					if (iValue > 0)
					{
						int iBestTempBuildValue = MAX_INT;
						BuildTypes eBestTempBuild = NO_BUILD;

						for (int iJ = 0; iJ < GC.getNumBuildInfos(); iJ++)
						{
							BuildTypes eBuild = ((BuildTypes)iJ);
							FAssertMsg(eBuild < GC.getNumBuildInfos(), "Invalid Build");

							if (GC.getBuildInfo(eBuild).getImprovement() != NO_IMPROVEMENT)
							{
								if (GC.getImprovementInfo((ImprovementTypes)(GC.getBuildInfo(eBuild).getImprovement())).isActsAsCity())
								{
									if (GC.getImprovementInfo((ImprovementTypes)(GC.getBuildInfo(eBuild).getImprovement())).getDefenseModifier() > 0)
									{
										if (canBuild(pLoopPlot, eBuild))
										{
											iValue = 10000;

											iValue /= (GC.getBuildInfo(eBuild).getTime() + 1);

											if (iValue < iBestTempBuildValue)
											{
												iBestTempBuildValue = iValue;
												eBestTempBuild = eBuild;
											}
										}
									}
								}
							}
						}

						if (eBestTempBuild != NO_BUILD)
						{
							if (!(pLoopPlot->isVisibleEnemyUnit(this)))
							{
								bool bValid = true;

								if (GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_LEAVE_FORESTS))
								{
									if (pLoopPlot->getFeatureType() != NO_FEATURE)
									{
										if (GC.getBuildInfo(eBestTempBuild).isFeatureRemove(pLoopPlot->getFeatureType())

//FfH: Added by Kael 04/24/2008
										  && !GC.getCivilizationInfo(getCivilizationType()).isMaintainFeatures(pLoopPlot->getFeatureType())
//FfH: End Add

										)
										{
											if (GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange(YIELD_PRODUCTION) > 0)
											{
												bValid = false;
											}
										}
									}
								}

								if (bValid)
								{
									if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_BUILD, getGroup(), 3) == 0)
									{
										int iPathTurns;
										if (generatePath(pLoopPlot, 0, true, &iPathTurns))
										{
											iValue *= 1000;
											iValue /= (iPathTurns + 1);

											if (iValue > iBestValue)
											{
												iBestValue = iValue;
												eBestBuild = eBestTempBuild;
												pBestPlot = pLoopPlot;
											}
/**								----  End Original Code  ----									**/
	int iValue;

	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (AI_plotValid(pLoopPlot))
		{
			if (pLoopPlot->getOwnerINLINE() == getOwnerINLINE()) // XXX team???
			{
				if (pLoopPlot->getImprovementType() == NO_IMPROVEMENT && pLoopPlot->getWorkingCity() == NULL)
				{
					int iBestTempBuildValue = MAX_INT;
					BuildTypes eBestTempBuild = NO_BUILD;

					for (int iJ = 0; iJ < GC.getNumBuildInfos(); iJ++)
					{
						BuildTypes eBuild = ((BuildTypes)iJ);
						FAssertMsg(eBuild < GC.getNumBuildInfos(), "Invalid Build");

						if (GC.getBuildInfo(eBuild).getImprovement() != NO_IMPROVEMENT)
						{
							iValue = GC.getImprovementInfo((ImprovementTypes)(GC.getBuildInfo(eBuild).getImprovement())).getDefenseModifier();
							if (iValue > 0)
							{
								if (canBuild(pLoopPlot, eBuild))
								{
									iValue *= 100;
									iValue /= std::min(1, pLoopPlot->getBuildTurnsLeft(eBuild, 0, 0, true, getTeam()));

									if (iValue > iBestTempBuildValue)
									{
										iBestTempBuildValue = iValue;
										eBestTempBuild = eBuild;
									}
								}
							}
						}
					}

					if (eBestTempBuild != NO_BUILD)
					{
						if (!(pLoopPlot->isVisibleEnemyUnit(this)))
						{
							bool bValid = true;

							if (GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_LEAVE_FORESTS))
							{
								if (pLoopPlot->getFeatureType() != NO_FEATURE)
								{
									if (GC.getBuildInfo(eBestTempBuild).isFeatureRemove(pLoopPlot->getFeatureType()) && !GC.getCivilizationInfo(getCivilizationType()).isMaintainFeatures(pLoopPlot->getFeatureType()))
									{
/*************************************************************************************************/
/**	CivPlotMods								03/31/09								Jean Elcard	**/
/**																								**/
/**						Consider Player-specific Feature Yield Changes.							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
										if (GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange(YIELD_PRODUCTION) > 0)
/**								----  End Original Code  ----									**/
										if ((GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange(YIELD_PRODUCTION) + GET_PLAYER(getOwnerINLINE()).getFeatureYieldChange(pLoopPlot->getFeatureType(), YIELD_PRODUCTION)) > 0)
/*************************************************************************************************/
/**	CivPlotMods								END													**/
/*************************************************************************************************/
										{
											bValid = false;
										}
									}
								}
							}

							if (bValid)
							{
								if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_BUILD, getGroup(), 3) == 0)
								{
									int iPathTurns;
/*************************************************************************************************/
/**	TipToe									04/04/09								Xienwolf	**/
/**																								**/
/**						Safe Movement is ideal situation where possible							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
									if (generatePath(pLoopPlot, 0, true, &iPathTurns))
/**								----  End Original Code  ----									**/
									if (generatePath(pLoopPlot, MOVE_SAFE_TERRITORY, true, &iPathTurns))
/*************************************************************************************************/
/**	TipToe									END													**/
/*************************************************************************************************/
									{
										iValue = iBestTempBuildValue*1000;
										iValue /= (iPathTurns + 1);

										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											eBestBuild = eBestTempBuild;
											pBestPlot = pLoopPlot;
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssertMsg(eBestBuild != NO_BUILD, "BestBuild is not assigned a valid value");
		FAssertMsg(eBestBuild < GC.getNumBuildInfos(), "BestBuild is assigned a corrupt value");

/*************************************************************************************************/
/**	Improved AI							02/07/10										Snarko	**/
/**																								**/
/**								Making the AI build forts										**/
/**			If you change your mind in the middle of nowhere don't build a road back home		**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		getGroup()->pushMission(MISSION_ROUTE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_BUILD, pBestPlot);
/**								----  End Original Code  ----									**/
		if (plot()->getTeam() == getTeam())
		{
			getGroup()->pushMission(MISSION_ROUTE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_BUILD, pBestPlot);
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_BUILD, pBestPlot);
		}
/*************************************************************************************************/
/**	Improved AI								END													**/
/*************************************************************************************************/
		getGroup()->pushMission(MISSION_BUILD, eBestBuild, -1, 0, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BUILD, pBestPlot);

		return true;
	}
	return false;
}

// Returns true if a mission was pushed...
bool CvUnitAI::AI_improveBonus(int iMinValue, CvPlot** ppBestPlot, BuildTypes* peBestBuild, int* piBestValue)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	ImprovementTypes eImprovement;
	BuildTypes eBuild;
	BuildTypes eBestBuild;
	BuildTypes eBestTempBuild;
	BonusTypes eNonObsoleteBonus;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iBestTempBuildValue;
	int iBestResourceValue;
	int iI, iJ;
	bool bBestBuildIsRoute = false;

	bool bCanRoute;
	bool bIsConnected;

	iBestValue = 0;
	iBestResourceValue = 0;
	eBestBuild = NO_BUILD;
	pBestPlot = NULL;

	bCanRoute = canBuildRoute();

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (pLoopPlot->getOwnerINLINE() == getOwnerINLINE() && AI_plotValid(pLoopPlot))
		{
			bool bCanImprove = (pLoopPlot->area() == area());
			if (!bCanImprove)
			{
				if (DOMAIN_SEA == getDomainType() && pLoopPlot->isWater() && plot()->isAdjacentToArea(pLoopPlot->area()))
				{
					bCanImprove = true;
				}
			}

//FfH: Added by Kael 12/20/2008
			if (pLoopPlot->isVisibleEnemyUnit(this))
			{
				bCanImprove = false;
			}
			if (!atPlot(pLoopPlot))
			{
				if (!canMoveInto(pLoopPlot))
				{
					bCanImprove = false;
				}
			}
//FfH: End Add

			if (bCanImprove)
			{
				eNonObsoleteBonus = pLoopPlot->getNonObsoleteBonusType(getTeam());

				if (eNonObsoleteBonus != NO_BONUS)
				{
					bIsConnected = pLoopPlot->isConnectedToCapital(getOwnerINLINE());
					if ((pLoopPlot->getWorkingCity() != NULL) || (bIsConnected || bCanRoute))
					{
						eImprovement = pLoopPlot->getImprovementType();

						bool bDoImprove = false;

						if (eImprovement == NO_IMPROVEMENT)
						{
							bDoImprove = true;
						}
						else if (GC.getImprovementInfo(eImprovement).isActsAsCity() || GC.getImprovementInfo(eImprovement).isImprovementBonusTrade(eNonObsoleteBonus))
						{
							bDoImprove = false;
						}
						else if (eImprovement == (ImprovementTypes)(GC.getDefineINT("RUINS_IMPROVEMENT")))
						{
							bDoImprove = true;
						}
						else if (!GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_SAFE_AUTOMATION))
						{
							bDoImprove = true;
						}

						iBestTempBuildValue = MAX_INT;
						eBestTempBuild = NO_BUILD;

						if (bDoImprove)
						{
							for (iJ = 0; iJ < GC.getNumBuildInfos(); iJ++)
							{
								eBuild = ((BuildTypes)iJ);

								if (GC.getBuildInfo(eBuild).getImprovement() != NO_IMPROVEMENT)
								{
									if (GC.getImprovementInfo((ImprovementTypes) GC.getBuildInfo(eBuild).getImprovement()).isImprovementBonusTrade(eNonObsoleteBonus) || (!pLoopPlot->isCityRadius() && GC.getImprovementInfo((ImprovementTypes) GC.getBuildInfo(eBuild).getImprovement()).isActsAsCity()))
									{
										if (canBuild(pLoopPlot, eBuild))
										{
											if ((pLoopPlot->getFeatureType() == NO_FEATURE) || !GC.getBuildInfo(eBuild).isFeatureRemove(pLoopPlot->getFeatureType()) || !GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_LEAVE_FORESTS)

//FfH: Added by Kael 04/24/2008
											  || GC.getCivilizationInfo(getCivilizationType()).isMaintainFeatures(pLoopPlot->getFeatureType())
//FfH: End Add

											)
											{
												iValue = 10000;

/*************************************************************************************************/
/**	Choppers							12/07/08									Xienwolf	**/
/**					Allows Discounting of Build Times based on known Technologies				**/
/**	Removes random from AI selection of Improvement to build, should eliminate silly workers	**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
												iValue /= (GC.getBuildInfo(eBuild).getTime() + 1);

/*FfH: Added by Chalid AiManaAndBonus 06/10/2006*
												if (!isHuman())
												{
													iValue /= 100;
													iValue *= std::max(0, (100-GC.getLeaderHeadInfo(GET_PLAYER(getOwnerINLINE()).getPersonalityType()).getImprovementWeightModifier((ImprovementTypes) GC.getBuildInfo(eBuild).getImprovement())));
												}
												iValue -= GC.getGameINLINE().getSorenRandNum(4000, "AIBonus");
//FfH: End Add
/**								----  End Original Code  ----									**/
												iValue *= std::max(1, pLoopPlot->getBuildTurnsLeft(eBuild, 0, 0, true, getTeam()));
												if (!isHuman())
												{
													iValue /= 100;
													iValue *= std::max(0, (100-GC.getLeaderHeadInfo(GET_PLAYER(getOwnerINLINE()).getPersonalityType()).getImprovementWeightModifier((ImprovementTypes) GC.getBuildInfo(eBuild).getImprovement())));
												}
												BonusTypes eBonus = (BonusTypes)GC.getImprovementInfo((ImprovementTypes)GC.getBuildInfo(eBuild).getImprovement()).getBonusConvert();
												if (eBonus != NO_BONUS && GC.getBonusInfo(eBonus).getBonusClassType() == GC.getDefineINT("BONUSCLASS_MANA"))
												{
													iValue -= GET_PLAYER(getOwnerINLINE()).AI_bonusVal(eBonus);
												}
/*************************************************************************************************/
/**	Choppers								END													**/
/*************************************************************************************************/

												// XXX feature production???

												if (iValue < iBestTempBuildValue)
												{
													iBestTempBuildValue = iValue;
													eBestTempBuild = eBuild;
												}
											}
										}
									}
								}
							}
						}
						if (eBestTempBuild == NO_BUILD)
						{
							bDoImprove = false;
						}

						if ((eBestTempBuild != NO_BUILD) || (bCanRoute && !bIsConnected))
						{
/*************************************************************************************************/
/**	TipToe									04/04/09								Xienwolf	**/
/**																								**/
/**						Safe Movement is ideal situation where possible							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
							if (generatePath(pLoopPlot, 0, true, &iPathTurns))
/**								----  End Original Code  ----									**/
/*************************************************************************************************/
/**	Improved AI							10/02/12										Snarko	**/
/**																								**/
/**				MOVE_SAFE_TERRITORY is too strict when trying to improve a bonus				**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
							if (generatePath(pLoopPlot, MOVE_SAFE_TERRITORY, true, &iPathTurns))
/**								----  End Original Code  ----									**/
							if (generatePath(pLoopPlot, MOVE_NO_ENEMY_TERRITORY, true, &iPathTurns))
/*************************************************************************************************/
/**	Improved AI								END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	TipToe									END													**/
/*************************************************************************************************/
							{
								iValue = GET_PLAYER(getOwnerINLINE()).AI_bonusVal(eNonObsoleteBonus);

								if (bDoImprove)
								{
									eImprovement = (ImprovementTypes)GC.getBuildInfo(eBestTempBuild).getImprovement();
									FAssert(eImprovement != NO_IMPROVEMENT);
									//iValue += (GC.getImprovementInfo((ImprovementTypes) GC.getBuildInfo(eBestTempBuild).getImprovement()))
									iValue += 5 * pLoopPlot->calculateImprovementYieldChange(eImprovement, YIELD_FOOD, getOwnerINLINE(), false);
/*************************************************************************************************/
/**	CivPlotMods								04/02/09								Jean Elcard	**/
/**																								**/
/**							Calculate Player-specific Nature Yields.							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
									iValue += 5 * pLoopPlot->calculateNatureYield(YIELD_FOOD, getTeam(), (pLoopPlot->getFeatureType() == NO_FEATURE) ? true : (GC.getBuildInfo(eBestTempBuild).isFeatureRemove(pLoopPlot->getFeatureType())
/**								----  End Original Code  ----									**/
									iValue += 5 * pLoopPlot->calculateNatureYield(YIELD_FOOD, getOwnerINLINE(), (pLoopPlot->getFeatureType() == NO_FEATURE) ? true : (GC.getBuildInfo(eBestTempBuild).isFeatureRemove(pLoopPlot->getFeatureType())
/*************************************************************************************************/
/**	CivPlotMods								END													**/
/*************************************************************************************************/
//FfH: Added by Kael 04/24/2008
									  && !GC.getCivilizationInfo(getCivilizationType()).isMaintainFeatures(pLoopPlot->getFeatureType()))
//FfH: End Add

									);
								}

								iValue += std::max(0, 100 * GC.getBonusInfo(eNonObsoleteBonus).getAIObjective());

								if (GET_PLAYER(getOwnerINLINE()).getNumTradeableBonuses(eNonObsoleteBonus) == 0)
								{
									iValue *= 2;
								}

								int iMaxWorkers = 1;
								if ((eBestTempBuild != NO_BUILD) && (!GC.getBuildInfo(eBestTempBuild).isKill()))
								{
									//allow teaming.
									iMaxWorkers = AI_calculatePlotWorkersNeeded(pLoopPlot, eBestTempBuild);
									if (getPathLastNode()->m_iData1 == 0)
									{
										iMaxWorkers = std::min((iMaxWorkers + 1) / 2, 1 + GET_PLAYER(getOwnerINLINE()).AI_baseBonusVal(eNonObsoleteBonus) / 20);
									}
								}

								if ((GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_BUILD, getGroup()) < iMaxWorkers)
/*************************************************************************************************/
/**	JohnHenry								04/04/09								Xienwolf	**/
/**																								**/
/**			Allows checking of properly scaled BuildTimes without going through a unit			**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
									&& (!bDoImprove || (pLoopPlot->getBuildTurnsLeft(eBestTempBuild, 0, 0) > (iPathTurns * 2 - 1))))
/**								----  End Original Code  ----									**/
									&& (!bDoImprove || (GC.getBuildInfo(eBestTempBuild).isKill() || (pLoopPlot->getBuildTurnsLeft(eBestTempBuild, 0, 0, true, getTeam()) > (iPathTurns * 2 - 1)))))
/*************************************************************************************************/
/**	JohnHenry								END													**/
/*************************************************************************************************/
								{
									if (bDoImprove)
									{
										iValue *= 1000;

										if (atPlot(pLoopPlot))
										{
											iValue *= 3;
										}

										iValue /= (iPathTurns + 1);

										if (pLoopPlot->isCityRadius())
										{
											iValue *= 2;
										}

										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											eBestBuild = eBestTempBuild;
											pBestPlot = pLoopPlot;
											bBestBuildIsRoute = false;
											iBestResourceValue = iValue;
										}
									}
									else
									{
										FAssert(bCanRoute && !bIsConnected);
										eImprovement = pLoopPlot->getImprovementType();
										if ((eImprovement != NO_IMPROVEMENT) && (GC.getImprovementInfo(eImprovement).isImprovementBonusTrade(eNonObsoleteBonus)))
										{
											iValue *= 1000;
											iValue /= (iPathTurns + 1);

											if (iValue > iBestValue)
											{
												iBestValue = iValue;
												eBestBuild = NO_BUILD;
												pBestPlot = pLoopPlot;
												bBestBuildIsRoute = true;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	//Should this be checking if we are UNDER the min Value?  We just spent so long looking for the HIGHEST value we can find...
	if ((iBestValue < iMinValue) && (NULL != ppBestPlot))
	{
		FAssert(NULL != peBestBuild);
		FAssert(NULL != piBestValue);

		*ppBestPlot = pBestPlot;
		*peBestBuild = eBestBuild;
		*piBestValue = iBestResourceValue;
	}

	if (pBestPlot != NULL)
	{
		if (eBestBuild != NO_BUILD)
		{
			FAssertMsg(!bBestBuildIsRoute, "BestBuild should not be a route");
			FAssertMsg(eBestBuild < GC.getNumBuildInfos(), "BestBuild is assigned a corrupt value");


			MissionTypes eBestMission = MISSION_MOVE_TO;

/*************************************************************************************************/
/**	Improved AI							02/07/10										Snarko	**/
/**																								**/
/**								Making the AI build forts										**/
/**			If you change your mind in the middle of nowhere don't build a road back home		**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
			if ((pBestPlot->getWorkingCity() == NULL) || !pBestPlot->getWorkingCity()->isConnectedToCapital())
/**								----  End Original Code  ----									**/
			if ((plot()->getTeam() == getTeam()) && ((pBestPlot->getWorkingCity() == NULL) || !pBestPlot->getWorkingCity()->isConnectedToCapital()))
/*************************************************************************************************/
/**	Improved AI								END													**/
/*************************************************************************************************/
			{
				eBestMission = MISSION_ROUTE_TO;
			}
/*************************************************************************************************/
/**	Improved AI							02/07/10										Snarko	**/
/**																								**/
/**								Making the AI build forts										**/
/**			If you change your mind in the middle of nowhere don't build a road back home		**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
			else
/**								----  End Original Code  ----									**/
			else if (plot()->getTeam() == getTeam())
/*************************************************************************************************/
/**	Improved AI								END													**/
/*************************************************************************************************/
			{
				int iDistance = stepDistance(getX_INLINE(), getY_INLINE(), pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
				int iPathTurns;
/*************************************************************************************************/
/**	TipToe									04/04/09								Xienwolf	**/
/**																								**/
/**						Safe Movement is ideal situation where possible							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
				if (generatePath(pBestPlot, 0, false, &iPathTurns))
/**								----  End Original Code  ----									**/
				if (generatePath(pBestPlot, MOVE_SAFE_TERRITORY, false, &iPathTurns))
/*************************************************************************************************/
/**	TipToe									END													**/
/*************************************************************************************************/
				{
					if (iPathTurns >= iDistance)
					{
						eBestMission = MISSION_ROUTE_TO;
					}
				}
			}

			eBestBuild = AI_betterPlotBuild(pBestPlot, eBestBuild);
			getGroup()->pushMission(eBestMission, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_BUILD, pBestPlot);
			getGroup()->pushMission(MISSION_BUILD, eBestBuild, -1, 0, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BUILD, pBestPlot);

			return true;
		}
		else if (bBestBuildIsRoute)
		{
			if (AI_connectPlot(pBestPlot))
			{
				return true;
			}
			/*else
			{
				// the plot may be connected, but not connected to capital, if capital is not on same area, or if civ has no capital (like barbarians)
				FAssertMsg(false, "Expected that a route could be built to eBestPlot");
			}*/
		}
		else
		{
			FAssert(false);
		}
	}

	return false;
}

//returns true if a mission is pushed
//if eBuild is NO_BUILD, assumes a route is desired.
bool CvUnitAI::AI_improvePlot(CvPlot* pPlot, BuildTypes eBuild)
{
	FAssert(pPlot != NULL);

	if (eBuild != NO_BUILD)
	{
		FAssertMsg(eBuild < GC.getNumBuildInfos(), "BestBuild is assigned a corrupt value");

		eBuild = AI_betterPlotBuild(pPlot, eBuild);
		if (!atPlot(pPlot))
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pPlot->getX_INLINE(), pPlot->getY_INLINE(), 0, false, false, MISSIONAI_BUILD, pPlot);
		}
		getGroup()->pushMission(MISSION_BUILD, eBuild, -1, 0, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BUILD, pPlot);

		return true;
	}
	else if (canBuildRoute())
	{
		if (AI_connectPlot(pPlot))
		{
			return true;
		}
	}

	return false;

}

BuildTypes CvUnitAI::AI_betterPlotBuild(CvPlot* pPlot, BuildTypes eBuild)
{
	FAssert(pPlot != NULL);
	FAssert(eBuild != NO_BUILD);
	bool bBuildRoute = false;
	bool bClearFeature = false;

	FeatureTypes eFeature = pPlot->getFeatureType();

	CvBuildInfo& kOriginalBuildInfo = GC.getBuildInfo(eBuild);

	if (kOriginalBuildInfo.getRoute() != NO_ROUTE)
	{
		return eBuild;	//Always prioritize Routes
	}

	int iWorkersNeeded = AI_calculatePlotWorkersNeeded(pPlot, eBuild);

	/********************************************************************************/
	/* 	BETTER_BTS_AI_MOD						7/31/08				jdog5000	*/
	/* 																			*/
	/* 	Bugfix																	*/
	/********************************************************************************/
	//if ((pPlot->getBonusType() == NO_BONUS) && (pPlot->getWorkingCity() != NULL))
	if ((pPlot->getNonObsoleteBonusType(getTeam()) == NO_BONUS) && (pPlot->getWorkingCity() != NULL))
	{
		iWorkersNeeded = std::max(1, std::min(iWorkersNeeded, pPlot->getWorkingCity()->AI_getWorkersHave()));
	}
	/********************************************************************************/
	/* 	BETTER_BTS_AI_MOD						END								*/
	/********************************************************************************/

	if (eFeature != NO_FEATURE)
	{
		CvFeatureInfo& kFeatureInfo = GC.getFeatureInfo(eFeature);
		if (kOriginalBuildInfo.isFeatureRemove(eFeature) && !GC.getCivilizationInfo(getCivilizationType()).isMaintainFeatures(pPlot->getFeatureType()))
		{
/*************************************************************************************************/
/**	CivPlotMods								03/31/09								Jean Elcard	**/
/**																								**/
/**						Consider Player-specific Feature Yield Changes.							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
			if ((kOriginalBuildInfo.getImprovement() == NO_IMPROVEMENT) || (!pPlot->isBeingWorked() || (kFeatureInfo.getYieldChange(YIELD_FOOD) + kFeatureInfo.getYieldChange(YIELD_PRODUCTION)) <= 0))
/**								----  End Original Code  ----									**/
			if ((kOriginalBuildInfo.getImprovement() == NO_IMPROVEMENT) || (!pPlot->isBeingWorked() ||
				(kFeatureInfo.getYieldChange(YIELD_FOOD) + GET_PLAYER(getOwnerINLINE()).getFeatureYieldChange(eFeature , YIELD_FOOD)
			   + kFeatureInfo.getYieldChange(YIELD_PRODUCTION) + GET_PLAYER(getOwnerINLINE()).getFeatureYieldChange(eFeature , YIELD_PRODUCTION)) <= 0))
/*************************************************************************************************/
/**	CivPlotMods								END													**/
/*************************************************************************************************/
			{
/*************************************************************************************************/
/**	Xienwolf Tweak							07/03/09											**/
/**																								**/
/**				Doing the check outside of the loop may enhance performance minimally			**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
				bClearFeature = true;
/**								----  End Original Code  ----									**/
				if(!GC.getCivilizationInfo(getCivilizationType()).isMaintainFeatures(pPlot->getFeatureType()))
				{
					bClearFeature = true;
				}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
			}
		}

/*************************************************************************************************/
/**	Xienwolf Tweak							03/18/09											**/
/**																								**/
/**						Don't bother building routes you don't really need						**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		if ((kFeatureInfo.getMovementCost() > 1) && (iWorkersNeeded > 1))
/**								----  End Original Code  ----									**/
		if ((kFeatureInfo.getMovementCost() > 1 //Feature slows travel (normally)
			//Racial Promotion doesn't offset this fact
			&& !(GC.getCivilizationInfo(getCivilizationType()).getDefaultRace() != NO_PROMOTION && GC.getPromotionInfo((PromotionTypes)GC.getCivilizationInfo(getCivilizationType()).getDefaultRace()).getFeatureDoubleMove(eFeature)))
			//And nobody is doing anything about it yet
			&& (iWorkersNeeded > 1))
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
		{
			bBuildRoute = true;
		}
	}
	/********************************************************************************/
	/* 	BETTER_BTS_AI_MOD						7/31/08				jdog5000	*/
	/* 																			*/
	/* 	Bugfix																	*/
	/********************************************************************************/
	//if (pPlot->getBonusType() != NO_BONUS)
	if (pPlot->getNonObsoleteBonusType(getTeam()) != NO_BONUS)
	/********************************************************************************/
	/* 	BETTER_BTS_AI_MOD						END								*/
	/********************************************************************************/
	{
		bBuildRoute = true;
	}
	else if (pPlot->isHills())
	{
/*************************************************************************************************/
/**	Xienwolf Tweak							03/18/09											**/
/**																								**/
/**						Don't bother building routes you don't really need						**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		if ((GC.getHILLS_EXTRA_MOVEMENT() > 0) && (iWorkersNeeded > 1))
/**								----  End Original Code  ----									**/
		if ((GC.getHILLS_EXTRA_MOVEMENT() > 0) //Normally slows travel
		//But race might fix this fact...
		&& !(GC.getCivilizationInfo(getCivilizationType()).getDefaultRace() != NO_PROMOTION && GC.getPromotionInfo((PromotionTypes)GC.getCivilizationInfo(getCivilizationType()).getDefaultRace()).isHillsDoubleMove())
		&& (iWorkersNeeded > 1))
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
		{
			bBuildRoute = true;
		}

/*************************************************************************************************/
/**	Mountain Mod by NeverMind 		imported by Ahwaric	19.09.09		**/
/*************************************************************************************************/
		else if (pPlot->isPeak())
		{
			if ((GC.getPEAK_EXTRA_MOVEMENT() > 0) && (iWorkersNeeded > 1))
			{
				bBuildRoute = true;
			}
		}
/*************************************************************************************************/
/**	Mountain Mod	END									**/
/*************************************************************************************************/
	}

	if (pPlot->getRouteType() != NO_ROUTE)
	{
		bBuildRoute = false;
	}

	BuildTypes eBestBuild = NO_BUILD;
	int iBestValue = 0;
/*************************************************************************************************/
/**	Xienwolf Tweak							07/03/09											**/
/**				If both of these are false, there is no need to do this loop...					**/
/**	NOTE:  Everything gets indented, but I don't feel that is really worth commenting it all	**/
/*************************************************************************************************/
	if(bBuildRoute || bClearFeature)	//
	{
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
		for (int iBuild = 0; iBuild < GC.getNumBuildInfos(); iBuild++)
		{
			BuildTypes eBuild = ((BuildTypes)iBuild);
			CvBuildInfo& kBuildInfo = GC.getBuildInfo(eBuild);


			RouteTypes eRoute = (RouteTypes)kBuildInfo.getRoute();

/*************************************************************************************************/
/**	Xienwolf Tweak							07/03/09											**/
/**																								**/
/**				Doing the check outside of the loop may enhance performance minimally			**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
			if ((bBuildRoute && (eRoute != NO_ROUTE)) || (bClearFeature && kBuildInfo.isFeatureRemove(eFeature)

//FfH: Added by Kael 04/24/2008
			&& !GC.getCivilizationInfo(getCivilizationType()).isMaintainFeatures(pPlot->getFeatureType())
//FfH: End Add

			))
/**								----  End Original Code  ----									**/
			if ((bBuildRoute && (eRoute != NO_ROUTE)) || (bClearFeature && kBuildInfo.isFeatureRemove(eFeature)))
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

			{
				if (canBuild(pPlot, eBuild))
				{
					int iValue = 10000;
					if (bBuildRoute && (eRoute != NO_ROUTE))
					{
						iValue *= (1 + GC.getRouteInfo(eRoute).getValue());
						iValue /= 2;

					/********************************************************************************/
					/* 	BETTER_BTS_AI_MOD						7/31/08				jdog5000	*/
					/* 																			*/
					/* 	Bugfix																	*/
					/********************************************************************************/
					//if if (pPlot->getBonusType() != NO_BONUS)
					if (pPlot->getNonObsoleteBonusType(getTeam()) != NO_BONUS)
					{
						iValue *= 2;
					}
					/********************************************************************************/
					/* 	BETTER_BTS_AI_MOD						END								*/
					/********************************************************************************/

/*************************************************************************************************/
/**	BuildXP									7/17/10									Valkrionn	**/
/**																								**/
/**									Allows Builds to grant XP									**/
/*************************************************************************************************/
						if (!GC.getGameINLINE().isOption(GAMEOPTION_NO_WORKER_XP))
						{
							iValue *= (1 + GC.getBuildInfo(eBuild).getXPValue() * (workRate(false, eBuild, plot()->getFeatureType())));
						}
/*************************************************************************************************/
/**	New Tag Defs							END													**/
/*************************************************************************************************/

						if (pPlot->getWorkingCity() != NULL)
						{
/*************************************************************************************************/
/**	Mountain Mod by NeverMind 		imported by Ahwaric	19.09.09		**/
/*************************************************************************************************/
/**					---- Start Original Code ----				**
							iValue *= 2 + iWorkersNeeded + ((pPlot->isHills() && (iWorkersNeeded > 1)) ? 2 * GC.getHILLS_EXTRA_MOVEMENT() : 0);
/**					----  End Original Code  ----				**/
							iValue *= 2 + iWorkersNeeded + ((pPlot->isHills() && (iWorkersNeeded > 1)) ? 2 * GC.getHILLS_EXTRA_MOVEMENT() : 0) + ((pPlot->isPeak() && (iWorkersNeeded > 1)) ? 2 * GC.getPEAK_EXTRA_MOVEMENT() : 0);
/*************************************************************************************************/
/**	Mountain Mod	END									**/
/*************************************************************************************************/
							iValue /= 3;
						}
						ImprovementTypes eImprovement = (ImprovementTypes)kOriginalBuildInfo.getImprovement();

						if (eImprovement != NO_IMPROVEMENT)
						{
							int iRouteMultiplier = ((GC.getImprovementInfo(eImprovement).getRouteYieldChanges(eRoute, YIELD_FOOD)) * 100);
							iRouteMultiplier += ((GC.getImprovementInfo(eImprovement).getRouteYieldChanges(eRoute, YIELD_PRODUCTION)) * 100);
							iRouteMultiplier += ((GC.getImprovementInfo(eImprovement).getRouteYieldChanges(eRoute, YIELD_COMMERCE)) * 60);
							iValue *= 100 + iRouteMultiplier;
							iValue /= 100;
						}

						int iPlotGroupId = -1;
						for (int iDirection = 0; iDirection < NUM_DIRECTION_TYPES; iDirection++)
						{
							CvPlot* pLoopPlot = plotDirection(pPlot->getX_INLINE(), pPlot->getY_INLINE(), (DirectionTypes)iDirection);
							if (pLoopPlot != NULL)
							{
								if (pPlot->isRiver() || (pLoopPlot->getRouteType() != NO_ROUTE))
								{
									CvPlotGroup* pLoopGroup = pLoopPlot->getPlotGroup(getOwnerINLINE());
									if (pLoopGroup != NULL)
									{
										if (pLoopGroup->getID() != -1)
										{
											if (pLoopGroup->getID() != iPlotGroupId)
											{
												//This plot bridges plot groups, so route it.
												iValue *= 4;
												break;
											}
											else
											{
												iPlotGroupId = pLoopGroup->getID();
											}
										}
									}
								}
							}
						}
/*************************************************************************************************/
/**	LinkedBuilds							7/18/10									Valkrionn	**/
/**																								**/
/**		Creates a list of Builds, which are executed before the main build if reqs are met		**/
/*************************************************************************************************/
						if (GC.getBuildInfo(eBuild).getNumLinkedBuilds() > 0)
						{
							int iNumLinkedBuilds = GC.getBuildInfo(eBuild).getNumLinkedBuilds();
							for (int iJ=0; iJ < iNumLinkedBuilds; iJ++)
							{
								BuildTypes eLinkedBuild = (BuildTypes)GC.getBuildInfo(eBuild).getLinkedBuilds(iJ);

								if (canBuild(plot(), eLinkedBuild, false, true))
								{
									bool bBuildLinkedRoute = false;
									bool bClearLinkedFeature = false;

									CvBuildInfo& kOriginalLinkedBuildInfo = GC.getBuildInfo(eLinkedBuild);

									int iLinkedWorkersNeeded = AI_calculatePlotWorkersNeeded(pPlot, eLinkedBuild);

									if ((pPlot->getBonusType() == NO_BONUS) && (pPlot->getWorkingCity() != NULL))
									{
										iLinkedWorkersNeeded = std::max(1, std::min(iLinkedWorkersNeeded, pPlot->getWorkingCity()->AI_getWorkersHave()));
									}

									if (eFeature != NO_FEATURE)
									{
										CvFeatureInfo& kLinkedFeatureInfo = GC.getFeatureInfo(eFeature);
										if (kOriginalLinkedBuildInfo.isFeatureRemove(eFeature) && !GC.getCivilizationInfo(getCivilizationType()).isMaintainFeatures(pPlot->getFeatureType()))
										{
											if ((kOriginalLinkedBuildInfo.getImprovement() == NO_IMPROVEMENT) || (!pPlot->isBeingWorked() ||
												(kLinkedFeatureInfo.getYieldChange(YIELD_FOOD) + GET_PLAYER(getOwnerINLINE()).getFeatureYieldChange(eFeature , YIELD_FOOD)
											   + kLinkedFeatureInfo.getYieldChange(YIELD_PRODUCTION) + GET_PLAYER(getOwnerINLINE()).getFeatureYieldChange(eFeature , YIELD_PRODUCTION)) <= 0))
											{
												if(!GC.getCivilizationInfo(getCivilizationType()).isMaintainFeatures(pPlot->getFeatureType()))
												{
													bClearLinkedFeature = true;
												}
											}
										}

										if ((kLinkedFeatureInfo.getMovementCost() > 1 //Feature slows travel (normally)
											//Racial Promotion doesn't offset this fact
											&& !(GC.getCivilizationInfo(getCivilizationType()).getDefaultRace() != NO_PROMOTION && GC.getPromotionInfo((PromotionTypes)GC.getCivilizationInfo(getCivilizationType()).getDefaultRace()).getFeatureDoubleMove(eFeature)))
											//And nobody is doing anything about it yet
											&& (iLinkedWorkersNeeded > 1))
										{
											bBuildLinkedRoute = true;
										}
									}

									if (pPlot->getBonusType() != NO_BONUS)
									{
										bBuildLinkedRoute = true;
									}
									else if (pPlot->isHills())
									{
										if ((GC.getHILLS_EXTRA_MOVEMENT() > 0) //Normally slows travel
										//But race might fix this fact...
										&& !(GC.getCivilizationInfo(getCivilizationType()).getDefaultRace() != NO_PROMOTION && GC.getPromotionInfo((PromotionTypes)GC.getCivilizationInfo(getCivilizationType()).getDefaultRace()).isHillsDoubleMove())
										&& (iLinkedWorkersNeeded > 1))
										{
											bBuildLinkedRoute = true;
										}

										else if (pPlot->isPeak())
										{
											if ((GC.getPEAK_EXTRA_MOVEMENT() > 0) && (iLinkedWorkersNeeded > 1))
											{
												bBuildLinkedRoute = true;
											}
										}
									}

									if (pPlot->getRouteType() != NO_ROUTE)
									{
										bBuildLinkedRoute = false;
									}


									if(bBuildLinkedRoute || bClearLinkedFeature)
									{
										for (int iLinkedBuild = 0; iLinkedBuild < GC.getNumBuildInfos(); iLinkedBuild++)
										{
											CvBuildInfo& kBuildInfo = GC.getBuildInfo(eLinkedBuild);

											RouteTypes eRoute = (RouteTypes)kBuildInfo.getRoute();


											if ((bBuildLinkedRoute && (eRoute != NO_ROUTE)) || (bClearLinkedFeature && kBuildInfo.isFeatureRemove(eFeature)))

											{
												if (canBuild(pPlot, eLinkedBuild))
												{
													int iLinkedValue = 10000;
													if (bBuildLinkedRoute && (eRoute != NO_ROUTE))
													{
														iLinkedValue *= (1 + GC.getRouteInfo(eRoute).getValue());
														iLinkedValue /= 2;

														if (pPlot->getBonusType() != NO_BONUS)
														{
															iLinkedValue *= 2;
														}

														if (!GC.getGameINLINE().isOption(GAMEOPTION_NO_WORKER_XP))
														{
															iLinkedValue *= (1 + GC.getBuildInfo(eLinkedBuild).getXPValue() * (workRate(false, eLinkedBuild, plot()->getFeatureType())));
														}

														if (pPlot->getWorkingCity() != NULL)
														{
															iLinkedValue *= 2 + iLinkedWorkersNeeded + ((pPlot->isHills() && (iLinkedWorkersNeeded > 1)) ? 2 * GC.getHILLS_EXTRA_MOVEMENT() : 0) + ((pPlot->isPeak() && (iLinkedWorkersNeeded > 1)) ? 2 * GC.getPEAK_EXTRA_MOVEMENT() : 0);

															iLinkedValue /= 3;
														}
														ImprovementTypes eImprovement = (ImprovementTypes)kOriginalLinkedBuildInfo.getImprovement();

														if (eImprovement != NO_IMPROVEMENT)
														{
															int iRouteMultiplier = ((GC.getImprovementInfo(eImprovement).getRouteYieldChanges(eRoute, YIELD_FOOD)) * 100);
															iRouteMultiplier += ((GC.getImprovementInfo(eImprovement).getRouteYieldChanges(eRoute, YIELD_PRODUCTION)) * 100);
															iRouteMultiplier += ((GC.getImprovementInfo(eImprovement).getRouteYieldChanges(eRoute, YIELD_COMMERCE)) * 60);
															iLinkedValue *= 100 + iRouteMultiplier;
															iLinkedValue /= 100;
														}

														iPlotGroupId = -1;
														for (int iDirection = 0; iDirection < NUM_DIRECTION_TYPES; iDirection++)
														{
															CvPlot* pLoopPlot = plotDirection(pPlot->getX_INLINE(), pPlot->getY_INLINE(), (DirectionTypes)iDirection);
															if (pLoopPlot != NULL)
															{
																if (pPlot->isRiver() || (pLoopPlot->getRouteType() != NO_ROUTE))
																{
																	CvPlotGroup* pLoopGroup = pLoopPlot->getPlotGroup(getOwnerINLINE());
																	if (pLoopGroup != NULL)
																	{
																		if (pLoopGroup->getID() != -1)
																		{
																			if (pLoopGroup->getID() != iPlotGroupId)
																			{
																				//This plot bridges plot groups, so route it.
																				iLinkedValue *= 4;
																				break;
																			}
																			else
																			{
																				iPlotGroupId = pLoopGroup->getID();
																			}
																		}
																	}
																}
															}
														}
													}
													iLinkedValue /= std::max(1, pPlot->getBuildTurnsLeft(eLinkedBuild, 0, 0, true, getTeam()));
													iValue += iLinkedValue;
												}
											}
										}
									}
								}
							}
						}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
					}

/*************************************************************************************************/
/**	Choppers							12/07/08									Xienwolf	**/
/**																								**/
/**					Allows Discounting of Build Times based on known Technologies				**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
					iValue /= (kBuildInfo.getTime() + 1);
/**								----  End Original Code  ----									**/
					iValue /= std::max(1, pPlot->getBuildTurnsLeft(eBuild, 0, 0, true, getTeam()));
/*************************************************************************************************/
/**	Choppers								END													**/
/*************************************************************************************************/

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						eBestBuild = eBuild;
					}
				}
			}
		}
	}

	if (eBestBuild == NO_BUILD)
	{
		return eBuild;
	}
	return eBestBuild;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_connectBonus(bool bTestTrade)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	BonusTypes eNonObsoleteBonus;
	int iI;

	// XXX how do we make sure that we can build roads???
/*************************************************************************************************/
/**	Xienwolf Tweak							03/18/09											**/
/**																								**/
/**									Ask a silly question....									**/
/*************************************************************************************************/
	if (!canBuildRoute())
	{
		return false;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (AI_plotValid(pLoopPlot))
		{
			if (pLoopPlot->getOwnerINLINE() == getOwnerINLINE()) // XXX team???
			{
				eNonObsoleteBonus = pLoopPlot->getNonObsoleteBonusType(getTeam());

				if (eNonObsoleteBonus != NO_BONUS)
				{
					if (!(pLoopPlot->isConnectedToCapital()))
					{
						if (!bTestTrade || ((pLoopPlot->getImprovementType() != NO_IMPROVEMENT) && (GC.getImprovementInfo(pLoopPlot->getImprovementType()).isImprovementBonusTrade(eNonObsoleteBonus))))
						{
							if (AI_connectPlot(pLoopPlot))
							{
								return true;
							}
						}
					}
				}
			}
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_connectCity()
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	int iLoop;

	// XXX how do we make sure that we can build roads???
/*************************************************************************************************/
/**	Xienwolf Tweak							03/18/09											**/
/**																								**/
/**									Ask a silly question....									**/
/*************************************************************************************************/
	if (!canBuildRoute())
	{
		return false;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	pLoopCity = plot()->getWorkingCity();
	if (pLoopCity != NULL)
	{
		if (AI_plotValid(pLoopCity->plot()))
		{
			if (!(pLoopCity->isConnectedToCapital()))
			{
				if (AI_connectPlot(pLoopCity->plot(), 1))
				{
					return true;
				}
			}
		}
	}

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if (AI_plotValid(pLoopCity->plot()))
		{
			if (!(pLoopCity->isConnectedToCapital()))
			{
				if (AI_connectPlot(pLoopCity->plot(), 1))
				{
					return true;
				}
			}
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_routeCity()
{
	PROFILE_FUNC();

	CvCity* pRouteToCity;
	CvCity* pLoopCity;
	int iLoop;

/*************************************************************************************************/
/**	Xienwolf Tweak							03/18/09											**/
/**																								**/
/**						Better to prevent an error than to cry about it...						**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	FAssert(canBuildRoute());
/**								----  End Original Code  ----									**/
/*************************************************************************************************/
/**	Improved AI							02/07/10										Snarko	**/
/**																								**/
/**								Making the AI build forts										**/
/**			If you change your mind in the middle of nowhere don't build a road back home		**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	if (!canBuildRoute())
/**								----  End Original Code  ----									**/
	if (!canBuildRoute() || (plot()->getTeam() != getTeam()))
	{
		return false;
	}
/*************************************************************************************************/
/**	Improved AI								END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if (AI_plotValid(pLoopCity->plot()))
		{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/22/10                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
			// BBAI efficiency: check area for land units before generating path
			if( (getDomainType() == DOMAIN_LAND) && (pLoopCity->area() != area()) && !(getGroup()->canMoveAllTerrain()) )
			{
				continue;
			}

			pRouteToCity = pLoopCity->AI_getRouteToCity();

			if (pRouteToCity != NULL)
			{
				if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
				{
					if (!(pRouteToCity->plot()->isVisibleEnemyUnit(this)))
					{
						if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pRouteToCity->plot(), MISSIONAI_BUILD, getGroup()) == 0)
						{
							if (generatePath(pLoopCity->plot(), MOVE_SAFE_TERRITORY, true))
							{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
								if (generatePath(pRouteToCity->plot(), MOVE_SAFE_TERRITORY, true))
								{
									getGroup()->pushMission(MISSION_ROUTE_TO, pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(), MOVE_SAFE_TERRITORY, false, false, MISSIONAI_BUILD, pRouteToCity->plot());
									getGroup()->pushMission(MISSION_ROUTE_TO, pRouteToCity->getX_INLINE(), pRouteToCity->getY_INLINE(), MOVE_SAFE_TERRITORY, (getGroup()->getLengthMissionQueue() > 0), false, MISSIONAI_BUILD, pRouteToCity->plot());

									return true;
								}
							}
						}
					}
				}
			}
		}
	}

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_routeTerritory(bool bImprovementOnly)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	ImprovementTypes eImprovement;
	RouteTypes eBestRoute;
	bool bValid;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iI, iJ;

	// XXX how do we make sure that we can build roads???

/*************************************************************************************************/
/**	Xienwolf Tweak							03/18/09											**/
/**		Oh look!  Now we ask a silly question with the answer staring us in the face!			**/
/**						Better to prevent an error than to cry about it...						**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	FAssert(canBuildRoute());
/**								----  End Original Code  ----									**/
/*************************************************************************************************/
/**	Improved AI							02/07/10										Snarko	**/
/**																								**/
/**								Making the AI build forts										**/
/**			If you change your mind in the middle of nowhere don't build a road back home		**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	if (!canBuildRoute())
/**								----  End Original Code  ----									**/
	if (!canBuildRoute() || (plot()->getTeam() != getTeam()))
	{
		return false;
	}
/*************************************************************************************************/
/**	Improved AI								END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	iBestValue = 0;
	pBestPlot = NULL;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (AI_plotValid(pLoopPlot))
		{
			if (pLoopPlot->getOwnerINLINE() == getOwnerINLINE()) // XXX team???
			{
				eBestRoute = GET_PLAYER(getOwnerINLINE()).getBestRoute(pLoopPlot);

				if (eBestRoute != NO_ROUTE)
				{
					if (eBestRoute != pLoopPlot->getRouteType())
					{
						if (bImprovementOnly)
						{
							bValid = false;

							eImprovement = pLoopPlot->getImprovementType();

							if (eImprovement != NO_IMPROVEMENT)
							{
								for (iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
								{
									if (GC.getImprovementInfo(eImprovement).getRouteYieldChanges(eBestRoute, iJ) > 0)
									{
										bValid = true;
										break;
									}
								}
							}
						}
						else
						{
							bValid = true;
						}

						if (bValid)
						{
							if (!(pLoopPlot->isVisibleEnemyUnit(this)))
							{
								if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_BUILD, getGroup(), 1) == 0)
								{
									if (generatePath(pLoopPlot, MOVE_SAFE_TERRITORY, true, &iPathTurns))
									{
										iValue = 10000;

										iValue /= (iPathTurns + 1);

										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											pBestPlot = pLoopPlot;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_ROUTE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_SAFE_TERRITORY, false, false, MISSIONAI_BUILD, pBestPlot);
		return true;
	}

	return false;
}

/*************************************************************************************************/
/**	Speedup								14/02/12										Snarko	**/
/**																								**/
/**			Only store the unitclasses we upgrade to, for faster looping						**/
/**	This function was NOT altered. It's rarely called. Don't start to abuse it and we're fine.	**/
/*************************************************************************************************/
// Returns true if a mission was pushed...
bool CvUnitAI::AI_travelToUpgradeCity()
{
	PROFILE_FUNC();

	// is there a city which can upgrade us?
	CvCity* pUpgradeCity = getUpgradeCity(/*bSearch*/ true);
	if (pUpgradeCity != NULL)
	{
		// cache some stuff
		CvPlot* pPlot = plot();
		bool bSeaUnit = (getDomainType() == DOMAIN_SEA);
		bool bCanAirliftUnit = (getDomainType() == DOMAIN_LAND);
		bool bShouldSkipToUpgrade = (getDomainType() != DOMAIN_AIR);

		// if we at the upgrade city, stop, wait to get upgraded
		if (pUpgradeCity->plot() == pPlot)
		{
			if (!bShouldSkipToUpgrade)
			{
				return false;
			}

			getGroup()->pushMission(MISSION_SKIP);
			return true;
		}

		if (DOMAIN_AIR == getDomainType())
		{
			FAssert(!atPlot(pUpgradeCity->plot()));
			getGroup()->pushMission(MISSION_MOVE_TO, pUpgradeCity->getX_INLINE(), pUpgradeCity->getY_INLINE());
			return true;
		}

		// find the closest city
		CvCity* pClosestCity = pPlot->getPlotCity();
		bool bAtClosestCity = (pClosestCity != NULL);
		if (pClosestCity == NULL)
		{
			pClosestCity = pPlot->getWorkingCity();
		}
		if (pClosestCity == NULL)
		{
			pClosestCity = GC.getMapINLINE().findCity(getX_INLINE(), getY_INLINE(), NO_PLAYER, getTeam(), true, bSeaUnit);
		}

		// can we path to the upgrade city?
		int iUpgradeCityPathTurns;
		CvPlot* pThisTurnPlot = NULL;
		bool bCanPathToUpgradeCity = generatePath(pUpgradeCity->plot(), 0, true, &iUpgradeCityPathTurns);
		if (bCanPathToUpgradeCity)
		{
			pThisTurnPlot = getPathEndTurnPlot();
		}

		// if we close to upgrade city, head there
		if (NULL != pThisTurnPlot && NULL != pClosestCity && (pClosestCity == pUpgradeCity || iUpgradeCityPathTurns < 4))
		{
			FAssert(!atPlot(pThisTurnPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pThisTurnPlot->getX_INLINE(), pThisTurnPlot->getY_INLINE());
			return true;
		}

		// check for better airlift choice
		if (bCanAirliftUnit && NULL != pClosestCity && pClosestCity->getMaxAirlift() > 0)
		{
			// if we at the closest city, then do the airlift, or wait
			if (bAtClosestCity)
			{
				// can we do the airlift this turn?
				if (canAirliftAt(pClosestCity->plot(), pUpgradeCity->getX_INLINE(), pUpgradeCity->getY_INLINE()))
				{
					getGroup()->pushMission(MISSION_AIRLIFT, pUpgradeCity->getX_INLINE(), pUpgradeCity->getY_INLINE());
					return true;
				}
				// wait to do it next turn
				else
				{
					getGroup()->pushMission(MISSION_SKIP);
					return true;
				}
			}

			int iClosestCityPathTurns;
			CvPlot* pThisTurnPlotForAirlift = NULL;
			bool bCanPathToClosestCity = generatePath(pClosestCity->plot(), 0, true, &iClosestCityPathTurns);
			if (bCanPathToClosestCity)
			{
				pThisTurnPlotForAirlift = getPathEndTurnPlot();
			}

			// is the closest city closer pathing? If so, move toward closest city
			if (NULL != pThisTurnPlotForAirlift && (!bCanPathToUpgradeCity || iClosestCityPathTurns < iUpgradeCityPathTurns))
			{
				FAssert(!atPlot(pThisTurnPlotForAirlift));
				getGroup()->pushMission(MISSION_MOVE_TO, pThisTurnPlotForAirlift->getX_INLINE(), pThisTurnPlotForAirlift->getY_INLINE());
				return true;
			}
		}

		// did not have better airlift choice, go ahead and path to the upgrade city
		if (NULL != pThisTurnPlot)
		{
			FAssert(!atPlot(pThisTurnPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pThisTurnPlot->getX_INLINE(), pThisTurnPlot->getY_INLINE());
			return true;
		}
	}

	return false;
}

// Returns true if a mission was pushed...
bool CvUnitAI::AI_retreatToCity(bool bPrimary, bool bAirlift, int iMaxPath)
{
	PROFILE_FUNC();

	CvCity* pCity;
	CvCity* pLoopCity;
	CvPlot* pBestPlot = NULL;
	int iPathTurns;
	int iValue;
	int iBestValue = MAX_INT;
	int iPass;
	int iLoop;
	int iCurrentDanger = GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot());

	pCity = plot()->getPlotCity();


	if (0 == iCurrentDanger)
	{
		if (pCity != NULL)
		{
			if (pCity->getOwnerINLINE() == getOwnerINLINE())
			{
				if (!bPrimary || GET_PLAYER(getOwnerINLINE()).AI_isPrimaryArea(pCity->area()))
				{
					if (!bAirlift || (pCity->getMaxAirlift() > 0))
					{
						if (!(pCity->plot()->isVisibleEnemyUnit(this)))
						{
							getGroup()->pushMission(MISSION_SKIP);
							return true;
						}
					}
				}
			}
		}
	}

	for (iPass = 0; iPass < 4; iPass++)
	{
		for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
		{
			if (AI_plotValid(pLoopCity->plot()))
			{
				if (!bPrimary || GET_PLAYER(getOwnerINLINE()).AI_isPrimaryArea(pLoopCity->area()))
				{
					if (!bAirlift || (pLoopCity->getMaxAirlift() > 0))
					{
						if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
						{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
							// BBAI efficiency: check area for land units before generating path
							if( !bAirlift && (getDomainType() == DOMAIN_LAND) && (pLoopCity->area() != area()) && !(getGroup()->canMoveAllTerrain()) )
							{
								continue;
							}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
/*************************************************************************************************/
/**	TipToe									04/04/09								Xienwolf	**/
/**																								**/
/**						Safe Movement is ideal situation where possible							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
							if (!atPlot(pLoopCity->plot()) && generatePath(pLoopCity->plot(), ((iPass > 1) ? MOVE_IGNORE_DANGER : 0), true, &iPathTurns))
/**								----  End Original Code  ----									**/
							if (!atPlot(pLoopCity->plot()) && generatePath(pLoopCity->plot(), ((iPass > 1) ? MOVE_IGNORE_DANGER : ((iPass == 0) ? MOVE_SAFE_TERRITORY :0)), true, &iPathTurns, iMaxPath))
/*************************************************************************************************/
/**	TipToe									END													**/
/*************************************************************************************************/
							{
								if (iPathTurns <= ((iPass == 2) ? 1 : iMaxPath))
								{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
/* original bts code
									if ((iPass > 0) || (getGroup()->canFight() || GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(pLoopCity->plot()) < iCurrentDanger))
*/
									// Water units can't defend a city
									// Any unthreatened city acceptable on 0th pass, solves problem where sea units
									// would oscillate in and out of threatened city because they had iCurrentDanger = 0
									// on turns outside city

									bool bCheck = (iPass > 0) || (getGroup()->canDefend());
									if( !bCheck )
									{
/*************************************************************************************************/
/**	Speedup								11/02/12										Snarko	**/
/**																								**/
/**		For the few cases where getPlotDanger is still used, use a threshold where possible		**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
										int iLoopDanger = GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(pLoopCity->plot());
										bCheck = (iLoopDanger == 0) || (iLoopDanger < iCurrentDanger);
/**								----  End Original Code  ----									**/
										bCheck = !GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(pLoopCity->plot(), -1, true, iCurrentDanger);
/*************************************************************************************************/
/**	Speedup									END													**/
/*************************************************************************************************/
									}

									if( bCheck )
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
									{
										iValue = iPathTurns;

										if (AI_getUnitAIType() == UNITAI_SETTLER_SEA)
										{
											iValue *= 1 + std::max(0, GET_PLAYER(getOwnerINLINE()).AI_totalAreaUnitAIs(pLoopCity->area(), UNITAI_SETTLE) - GET_PLAYER(getOwnerINLINE()).AI_totalAreaUnitAIs(pLoopCity->area(), UNITAI_SETTLER_SEA));
										}

										if (iValue < iBestValue)
										{
											iBestValue = iValue;
											pBestPlot = getPathEndTurnPlot();
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/27/08                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
											// Not sure what can go wrong here, it seems somehow m_iData1 (moves) was set to 0
											// for first node in path so m_iData2 (turns) incremented
											if( atPlot(pBestPlot) )
											{
												//FAssert(false);
												pBestPlot = getGroup()->getPathFirstPlot();
												FAssert(!atPlot(pBestPlot));
											}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
										}
									}
								}
							}
						}
					}
				}
			}
		}

		if (pBestPlot != NULL)
		{
			break;
		}
		else if (iPass == 0)
		{
			if (pCity != NULL)
			{
				if (pCity->getOwnerINLINE() == getOwnerINLINE())
				{
					if (!bPrimary || GET_PLAYER(getOwnerINLINE()).AI_isPrimaryArea(pCity->area()))
					{
						if (!bAirlift || (pCity->getMaxAirlift() > 0))
						{
							if (!(pCity->plot()->isVisibleEnemyUnit(this)))
							{
								getGroup()->pushMission(MISSION_SKIP);
								return true;
							}
						}
					}
				}
			}
		}

		if (getGroup()->alwaysInvisible())
		{
			break;
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), ((iPass > 1) ? MOVE_IGNORE_DANGER : ((iPass == 0) ? MOVE_SAFE_TERRITORY : 0)));
		return true;
	}

	if (pCity != NULL)
	{
		if (pCity->getTeam() == getTeam())
		{
			getGroup()->pushMission(MISSION_SKIP);
			return true;
		}
	}

	return false;
}


// Returns true if a mission was pushed...
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/15/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
/* original bts code
bool CvUnitAI::AI_pickup(UnitAITypes eUnitAI)
*/
bool CvUnitAI::AI_pickup(UnitAITypes eUnitAI, bool bCountProduction, int iMaxPath)
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
{
	PROFILE_FUNC();

	CvCity* pCity;
	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	CvPlot* pBestPickupPlot;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iLoop;

	FAssert(cargoSpace() > 0);
	if (0 == cargoSpace())
	{
		return false;
	}

	pCity = plot()->getPlotCity();

	if (pCity != NULL)
	{
		if (pCity->getOwnerINLINE() == getOwnerINLINE())
		{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/23/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
/* original bts code
			if (pCity->plot()->plotCount(PUF_isUnitAIType, eUnitAI, -1, getOwnerINLINE()) > 0)
			{
				if ((AI_getUnitAIType() != UNITAI_ASSAULT_SEA) || pCity->AI_isDefended(-1))
				{
*/
			if( (GC.getGameINLINE().getGameTurn() - pCity->getGameTurnAcquired()) > 15 || (GET_TEAM(getTeam()).countEnemyPowerByArea(pCity->area()) == 0) )
			{
				bool bConsider = false;

				if(AI_getUnitAIType() == UNITAI_ASSAULT_SEA)
				{
					// Improve island hopping
					if( pCity->area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE )
					{
						bConsider = false;
					}
					else if( eUnitAI == UNITAI_ATTACK_CITY && !(pCity->AI_isDanger()) )
					{
						bConsider = (pCity->plot()->plotCount(PUF_canDefend, -1, -1, getOwnerINLINE(), NO_TEAM, PUF_isDomainType, DOMAIN_LAND) > pCity->AI_neededDefenders());
					}
					else
					{
						bConsider = pCity->AI_isDefended(-1);
					}
				}
				else if(AI_getUnitAIType() == UNITAI_SETTLER_SEA)
				{
					if( eUnitAI == UNITAI_CITY_DEFENSE )
					{
						bConsider = (pCity->plot()->plotCount(PUF_canDefendGroupHead, -1, -1, getOwnerINLINE(), NO_TEAM, PUF_isCityAIType) > 1);
					}
					else
					{
						bConsider = true;
					}
				}
				else
				{
					bConsider = true;
				}

				if ( bConsider )
				{
					// only count units which are available to load
					int iCount = pCity->plot()->plotCount(PUF_isAvailableUnitAITypeGroupie, eUnitAI, -1, getOwnerINLINE(), NO_TEAM, PUF_isFiniteRange);

					if (bCountProduction && (pCity->getProductionUnitAI() == eUnitAI))
					{
						if( pCity->getProductionTurnsLeft() < 4 )
						{
							CvUnitInfo& kUnitInfo = GC.getUnitInfo(pCity->getProductionUnit());
							if ((kUnitInfo.getDomainType() != DOMAIN_AIR) || kUnitInfo.getAirRange() > 0)
							{
								iCount++;
							}
						}
					}

					if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pCity->plot(), MISSIONAI_PICKUP, getGroup()) < ((iCount + (cargoSpace() - 1)) / cargoSpace()))
					{
						getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_PICKUP, pCity->plot());
						return true;
					}
				}
			}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		}
	}

	iBestValue = 0;
	pBestPlot = NULL;
	pBestPickupPlot = NULL;

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if (AI_plotValid(pLoopCity->plot()))
		{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/23/09                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
			if( (GC.getGameINLINE().getGameTurn() - pLoopCity->getGameTurnAcquired()) > 15 || (GET_TEAM(getTeam()).countEnemyPowerByArea(pLoopCity->area()) == 0) )
			{
				bool bConsider = false;

				if(AI_getUnitAIType() == UNITAI_ASSAULT_SEA)
				{
					if( pLoopCity->area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE )
					{
						bConsider = false;
					}
					else if( eUnitAI == UNITAI_ATTACK_CITY && !(pLoopCity->AI_isDanger()) )
					{
						// Improve island hopping
						bConsider = (pLoopCity->plot()->plotCount(PUF_canDefend, -1, -1, getOwnerINLINE(), NO_TEAM, PUF_isDomainType, DOMAIN_LAND) > pLoopCity->AI_neededDefenders());
					}
					else
					{
						bConsider = pLoopCity->AI_isDefended(-1);
					}
				}
				else if(AI_getUnitAIType() == UNITAI_SETTLER_SEA)
				{
					if( eUnitAI == UNITAI_CITY_DEFENSE )
					{
						bConsider = (pLoopCity->plot()->plotCount(PUF_canDefendGroupHead, -1, -1, getOwnerINLINE(), NO_TEAM, PUF_isCityAIType) > 1);
					}
					else
					{
						bConsider = true;
					}
				}
				else
				{
					bConsider = true;
				}

				if ( bConsider )
				{
					// only count units which are available to load, have had a chance to move since being built
					int iCount = pLoopCity->plot()->plotCount(PUF_isAvailableUnitAITypeGroupie, eUnitAI, -1, getOwnerINLINE(), NO_TEAM, (bCountProduction ? PUF_isFiniteRange : PUF_isFiniteRangeAndNotJustProduced));

					iValue = iCount * 10;

					if (bCountProduction && (pLoopCity->getProductionUnitAI() == eUnitAI))
					{
						CvUnitInfo& kUnitInfo = GC.getUnitInfo(pLoopCity->getProductionUnit());
						if ((kUnitInfo.getDomainType() != DOMAIN_AIR) || kUnitInfo.getAirRange() > 0)
						{
							iValue++;
							iCount++;
						}
					}

					if (iValue > 0)
					{
						iValue += pLoopCity->getPopulation();

						if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
						{
							if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_PICKUP, getGroup()) < ((iCount + (cargoSpace() - 1)) / cargoSpace()))
							{
								if( !(pLoopCity->AI_isDanger()) )
								{
									if (!atPlot(pLoopCity->plot()) && generatePath(pLoopCity->plot(), 0, true, &iPathTurns))
									{
										if( AI_getUnitAIType() == UNITAI_ASSAULT_SEA )
										{
											if( pLoopCity->area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT )
											{
												iValue *= 4;
											}
											else if( pLoopCity->area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT_ASSIST )
											{
												iValue *= 2;
											}
										}

										iValue *= 1000;

										iValue /= (iPathTurns + 3);

										if( (iValue > iBestValue) && (iPathTurns <= iMaxPath) )
										{
											iBestValue = iValue;
											// Do one turn along path, then reevaluate
											// Causes update of destination based on troop movement
											//pBestPlot = pLoopCity->plot();
											pBestPlot = getPathEndTurnPlot();
											pBestPickupPlot = pLoopCity->plot();

											if( pBestPlot == NULL || atPlot(pBestPlot) )
											{
												//FAssert(false);
												pBestPlot = pBestPickupPlot;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if ((pBestPlot != NULL) && (pBestPickupPlot != NULL))
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_AVOID_ENEMY_WEIGHT_3, false, false, MISSIONAI_PICKUP, pBestPickupPlot);
		return true;
	}

	return false;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/22/10                                jdog5000      */
/*                                                                                              */
/* Naval AI                                                                                     */
/************************************************************************************************/
// Returns true if a mission was pushed...
bool CvUnitAI::AI_pickupStranded(UnitAITypes eUnitAI, int iMaxPath)
{
	PROFILE_FUNC();

	CvUnit* pBestUnit=NULL;
	int iPathTurns=0;
	int iValue=-1;
	int iBestValue=-1;
	int iLoop=0;
	int iCount=0;

	FAssert(cargoSpace() > 0);
	if (0 == cargoSpace())
	{
		return false;
	}

	if( isBarbarian() )
	{
		return false;
	}

	iBestValue = 0;
	pBestUnit = NULL;

	int iI;
	CvSelectionGroup* pLoopGroup = NULL;
	CvUnit* pHeadUnit = NULL;
	CvPlot* pLoopPlot = NULL;
	CvPlot* pPickupPlot = NULL;
	CvPlot* pAdjacentPlot = NULL;
	CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());

	for(pLoopGroup = kPlayer.firstSelectionGroup(&iLoop); pLoopGroup != NULL; pLoopGroup = kPlayer.nextSelectionGroup(&iLoop))
	{
		if( pLoopGroup->isStranded() )
		{
			pHeadUnit = pLoopGroup->getHeadUnit();
			if( pHeadUnit == NULL )
			{
				continue;
			}

			if( (eUnitAI != NO_UNITAI) && (pHeadUnit->AI_getUnitAIType() != eUnitAI) )
			{
				continue;
			}

			pLoopPlot = pHeadUnit->plot();
			if( pLoopPlot == NULL  )
			{
				continue;
			}

			if( !(pLoopPlot->isCoastalLand())  && !canMoveAllTerrain() )
			{
				continue;
			}

			// Units are stranded, attempt rescue

			iCount = pLoopGroup->getNumUnits();

			if( 1000*iCount > iBestValue )
			{
				pPickupPlot = NULL;
				if( atPlot(pLoopPlot) || (AI_plotValid(pLoopPlot) && generatePath(pLoopPlot, 0, true, &iPathTurns)) )
				{
					pPickupPlot = pLoopPlot;
				}
				else
				{
					for (iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
					{
						pAdjacentPlot = plotDirection(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), ((DirectionTypes)iI));

						if (pAdjacentPlot != NULL && AI_plotValid(pAdjacentPlot))
						{
							if( generatePath(pAdjacentPlot, 0, true, &iPathTurns) )
							{
								pPickupPlot = pAdjacentPlot;
								break;
							}
						}
					}
				}

				if( pPickupPlot != NULL && iPathTurns <= iMaxPath )
				{
					MissionAITypes eMissionAIType = MISSIONAI_PICKUP;
					iCount -= GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(pHeadUnit, &eMissionAIType, 1, getGroup(), iPathTurns) * cargoSpace();

					iValue = 1000*iCount;

					iValue /= (iPathTurns + 1);

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pBestUnit = pHeadUnit;
					}
				}
			}
		}
	}

	if ((pBestUnit != NULL))
	{
		if( atPlot(pBestUnit->plot()) )
		{
			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_PICKUP, pBestUnit->plot());
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestUnit->plot()));
			getGroup()->pushMission(MISSION_MOVE_TO_UNIT, pBestUnit->getOwnerINLINE(), pBestUnit->getID(), MOVE_AVOID_ENEMY_WEIGHT_3, false, false, MISSIONAI_PICKUP, NULL, pBestUnit);
			return true;
		}
	}

	return false;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


// Returns true if a mission was pushed...
bool CvUnitAI::AI_airOffensiveCity()
{
	PROFILE_FUNC();

	CvPlot* pBestPlot;
	int iValue;
	int iBestValue;
	int iI;

	FAssert(canAirAttack() || nukeRange() >= 0);

	iBestValue = 0;
	pBestPlot = NULL;

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						04/25/08			jdog5000		*/
/* 																				*/
/* 	Air AI																		*/
/********************************************************************************/
	/* original BTS code

	*/
	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		// Limit to cities and forts, true for any city but only this team's forts
		if (pLoopPlot->isCity(true, getTeam()))
		{
			if (pLoopPlot->getTeam() == getTeam() || (pLoopPlot->isOwned() && GET_TEAM(pLoopPlot->getTeam()).isVassal(getTeam())))
			{
				if (atPlot(pLoopPlot) || canMoveInto(pLoopPlot))
				{
					iValue = AI_airOffenseBaseValue( pLoopPlot );

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pBestPlot = pLoopPlot;
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		if (!atPlot(pBestPlot))
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_SAFE_TERRITORY);
			return true;
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END									*/
/********************************************************************************/

	return false;
}

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						04/25/10			jdog5000		*/
/* 																				*/
/* 	Air AI																		*/
/********************************************************************************/
// Function for ranking the value of a plot as a base for offensive air units
int CvUnitAI::AI_airOffenseBaseValue( CvPlot* pPlot )
{
	if( pPlot == NULL || pPlot->area() == NULL )
	{
		return 0;
	}

	CvCity* pNearestEnemyCity = NULL;
	int iRange = 0;
	int iTempValue = 0;
	int iOurDefense = 0;
	int iOurOffense = 0;
	int iEnemyOffense = 0;
	int iEnemyDefense = 0;
	int iDistance = 0;

	CvPlot* pLoopPlot = NULL;
	CvCity* pCity = pPlot->getPlotCity();

	int iDefenders = pPlot->plotCount(PUF_canDefend, -1, -1, pPlot->getOwner());

	int iAttackAirCount = pPlot->plotCount(PUF_canAirAttack, -1, -1, NO_PLAYER, getTeam());
	iAttackAirCount += 2 * pPlot->plotCount(PUF_isUnitAIType, UNITAI_ICBM, -1, NO_PLAYER, getTeam());
	if (atPlot(pPlot))
	{
		iAttackAirCount += canAirAttack() ? -1 : 0;
		iAttackAirCount += (nukeRange() >= 0) ? -2 : 0;
	}

	if( pPlot->isCoastalLand(GC.getMIN_WATER_SIZE_FOR_OCEAN()) )
	{
		iDefenders -= 1;
	}

	if( pCity != NULL )
	{
		if( pCity->getDefenseModifier(true) < 40 )
		{
			iDefenders -= 1;
		}

		if( pCity->getOccupationTimer() > 1 )
		{
			iDefenders -= 1;
		}
	}

	// Consider threat from nearby enemy territory
	iRange = 1;
	int iBorderDanger = 0;

	for (int iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (int iDY = -(iRange); iDY <= iRange; iDY++)
		{
			pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->area() == pPlot->area() && pLoopPlot->isOwned())
				{
					iDistance = stepDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());
					if( pLoopPlot->getTeam() != getTeam() && !(GET_TEAM(pLoopPlot->getTeam()).isVassal(getTeam())) )
					{
						if( iDistance == 1 )
						{
							iBorderDanger++;
						}

						if (atWar(pLoopPlot->getTeam(), getTeam()))
						{
							if (iDistance == 1)
							{
								iBorderDanger += 2;
							}
							else if ((iDistance == 2) && (pLoopPlot->isRoute()))
							{
								iBorderDanger += 2;
							}
						}
					}
				}
			}
		}
	}

	iDefenders -= std::min(2,(iBorderDanger + 1)/3);

	// Don't put more attack air units on plot than effective land defenders ... too large a risk
	if (iAttackAirCount >= (iDefenders) || iDefenders <= 0)
	{
		return 0;
	}

	bool bAnyWar = (GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0);

	int iValue = 0;

	if( bAnyWar )
	{
		// Don't count assault assist, don't want to weight defending colonial coasts when homeland might be under attack
		bool bAssault = (pPlot->area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT) || (pPlot->area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT_MASSING);

		// Loop over operational range
		iRange = airRange();

		for (int iDX = -(iRange); iDX <= iRange; iDX++)
		{
			for (int iDY = -(iRange); iDY <= iRange; iDY++)
			{
				pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

				if ((pLoopPlot != NULL && pLoopPlot->area() != NULL))
				{
					iDistance = plotDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());

					if( iDistance <= iRange )
					{
						bool bDefensive = pLoopPlot->area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE;
						bool bOffensive = pLoopPlot->area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE;

						// Value system is based around 1 enemy military unit in our territory = 10 pts
						iTempValue = 0;

						if( pLoopPlot->isWater() )
						{
							if( pLoopPlot->isVisible(getTeam(),false) && !pLoopPlot->area()->isLake()  )
							{
								// Defend ocean
								iTempValue = 1;

								if( pLoopPlot->isOwned() )
								{
									if( pLoopPlot->getTeam() == getTeam() )
									{
										iTempValue += 1;
									}
									else if ((pLoopPlot->getTeam() != getTeam()) && GET_TEAM(getTeam()).AI_getWarPlan(pLoopPlot->getTeam()) != NO_WARPLAN)
									{
										iTempValue += 1;
									}
								}

								// Low weight for visible ships cause they will probably move
								iTempValue += 2*pLoopPlot->getNumVisibleEnemyDefenders(this);

								if( bAssault )
								{
									iTempValue *= 2;
								}
							}
						}
						else
						{
							if( !(pLoopPlot->isOwned()) )
							{
								if( iDistance < (iRange - 2) )
								{
									// Target enemy troops in neutral territory
									iTempValue += 4*pLoopPlot->getNumVisibleEnemyDefenders(this);
								}
							}
							else if( pLoopPlot->getTeam() == getTeam() )
							{
								iTempValue = 0;

								if( iDistance < (iRange - 2) )
								{
									// Target enemy troops in our territory
									iTempValue += 5*pLoopPlot->getNumVisibleEnemyDefenders(this);

									if( pLoopPlot->getOwnerINLINE() == getOwnerINLINE() )
									{
										if( GET_PLAYER(getOwnerINLINE()).AI_isPrimaryArea(pLoopPlot->area()) )
										{
											iTempValue *= 3;
										}
										else
										{
											iTempValue *= 2;
										}
									}

									if( bDefensive )
									{
										iTempValue *= 2;
									}
								}
							}
							else if ((pLoopPlot->getTeam() != getTeam()) && GET_TEAM(getTeam()).AI_getWarPlan(pLoopPlot->getTeam()) != NO_WARPLAN)
							{
								// Attack opponents land territory
								iTempValue = 3;

								CvCity* pLoopCity = pLoopPlot->getPlotCity();

								if (pLoopCity != NULL)
								{
									// Target enemy cities
									iTempValue += (3*pLoopCity->getPopulation() + 30);

									if( canAirBomb(pPlot) && pLoopCity->isBombardable(this) )
									{
										iTempValue *= 2;
									}

									if( pLoopPlot->area()->getTargetCity(getOwnerINLINE()) == pLoopCity )
									{
										iTempValue *= 2;
									}

									if( pLoopCity->AI_isDanger() )
									{
										// Multiplier for nearby troops, ours, teammate's, and any other enemy of city
										iTempValue *= 3;
									}
								}
								else
								{
									if( iDistance < (iRange - 2) )
									{
										// Support our troops in enemy territory
										iTempValue += 15*pLoopPlot->getNumDefenders(getOwnerINLINE());

										// Target enemy troops adjacent to our territory
										if( pLoopPlot->isAdjacentTeam(getTeam(),true) )
										{
											iTempValue += 7*pLoopPlot->getNumVisibleEnemyDefenders(this);
										}
									}

									// Weight resources
									if (canAirBombAt(pPlot, pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()))
									{
										if (pLoopPlot->getBonusType(getTeam()) != NO_BONUS)
										{
											iTempValue += 8*std::max(2, GET_PLAYER(pLoopPlot->getOwnerINLINE()).AI_bonusVal(pLoopPlot->getBonusType(getTeam()))/10);
										}
									}
								}

								if( (pLoopPlot->area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE) )
								{
									// Extra weight for enemy territory in offensive areas
									iTempValue *= 2;
								}

								if( GET_PLAYER(getOwnerINLINE()).AI_isPrimaryArea(pLoopPlot->area()) )
								{
									iTempValue *= 3;
									iTempValue /= 2;
								}

								if( pLoopPlot->isBarbarian() )
								{
									iTempValue /= 2;
								}
							}
						}

						iValue += iTempValue;
					}
				}
			}
		}

		// Consider available defense, direct threat to potential base
		iOurDefense = GET_TEAM(getTeam()).AI_getOurPlotStrength(pPlot,0,true,false,true);
		iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(pPlot,2,false,false);

		if( 3*iEnemyOffense > iOurDefense || iOurDefense == 0 )
		{
			iValue *= iOurDefense;
			iValue /= std::max(1,3*iEnemyOffense);
		}

		// Value forts less, they are generally riskier bases
		if( pCity == NULL )
		{
			iValue *= 2;
			iValue /= 3;
		}
	}
	else
	{
		if( pPlot->getOwnerINLINE() != getOwnerINLINE() )
		{
			// Keep planes at home when not in real wars
			return 0;
		}

		// If no wars, use prior logic with added value to keeping planes safe from sneak attack
		if (pCity != NULL)
		{
			iValue = (pCity->getPopulation() + 20);
			iValue += pCity->AI_cityThreat();
		}
		else
		{
			if( iDefenders > 0 )
			{
				iValue = (pCity != NULL) ? 0 : GET_PLAYER(getOwnerINLINE()).AI_getPlotAirbaseValue(pPlot);
				iValue /= 6;
			}
		}

		iValue += std::min(24, 3*(iDefenders - iAttackAirCount));

		if( GET_PLAYER(getOwnerINLINE()).AI_isPrimaryArea(pPlot->area()) )
		{
			iValue *= 4;
			iValue /= 3;
		}

		// No real enemies, check for minor civ or barbarian cities where attacks could be supported
		pNearestEnemyCity = GC.getMapINLINE().findCity(pPlot->getX_INLINE(), pPlot->getY_INLINE(), NO_PLAYER, NO_TEAM, false, false, getTeam());

		if (pNearestEnemyCity != NULL)
		{
			iDistance = plotDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pNearestEnemyCity->getX_INLINE(), pNearestEnemyCity->getY_INLINE());
			if (iDistance > airRange())
			{
				iValue /= 10 * (2 + airRange());
			}
			else
			{
				iValue /= 2 + iDistance;
			}
		}
	}

	if (pPlot->getOwnerINLINE() == getOwnerINLINE())
	{
		// Bases in our territory better than teammate's
		iValue *= 2;
	}
	else if( pPlot->getTeam() == getTeam() )
	{
		// Our team's bases are better than vassal plots
		iValue *= 3;
		iValue /= 2;
	}

	return iValue;
}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

// Returns true if a mission was pushed...
bool CvUnitAI::AI_airDefensiveCity()
{
	PROFILE_FUNC();

	CvCity* pCity;
	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	int iValue;
	int iBestValue;
	int iLoop;

	FAssert(getDomainType() == DOMAIN_AIR);
	FAssert(canAirDefend());

	/********************************************************************************/
	/* 	BETTER_BTS_AI_MOD						10/26/08			jdog5000	*/
	/* 																			*/
	/* 	Air AI																	*/
	/********************************************************************************/
	if (canAirDefend() && getDamage() == 0)
	{
		pCity = plot()->getPlotCity();

		if (pCity != NULL)
		{
			if (pCity->getOwnerINLINE() == getOwnerINLINE())
			{
				if ( !(pCity->AI_isAirDefended(false,+1)) )
				{
					// Stay if very short on planes, regardless of situation
					getGroup()->pushMission(MISSION_AIRPATROL);
					return true;
				}

				if( !(pCity->AI_isAirDefended(true,-1)) )
				{
					// Stay if city is threatened but not seriously threatened
					int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),2,false,false);

					if (iEnemyOffense > 0)
					{
						int iOurDefense = GET_TEAM(getTeam()).AI_getOurPlotStrength(plot(),0,true,false,true);
						if( 3*iEnemyOffense < 4*iOurDefense )
						{
							getGroup()->pushMission(MISSION_AIRPATROL);
							return true;
						}
					}
				}
			}
		}
	}

	iBestValue = 0;
	pBestPlot = NULL;

	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		if (canAirDefend(pLoopCity->plot()))
		{
			if (atPlot(pLoopCity->plot()) || canMoveInto(pLoopCity->plot()))
			{
				int iExistingAirDefenders = pLoopCity->plot()->plotCount(PUF_canAirDefend, -1, -1, pLoopCity->getOwnerINLINE(), NO_TEAM, PUF_isDomainType, DOMAIN_AIR);
				if( atPlot(pLoopCity->plot()) )
				{
					iExistingAirDefenders -= 1;
				}
				int iNeedAirDefenders = pLoopCity->AI_neededAirDefenders();

				if ( iNeedAirDefenders > iExistingAirDefenders )
				{
					iValue = pLoopCity->getPopulation() + pLoopCity->AI_cityThreat();

					int iOurDefense = GET_TEAM(getTeam()).AI_getOurPlotStrength(pLoopCity->plot(),0,true,false,true);
					int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(pLoopCity->plot(),2,false,false);

					iValue *= 100;

					// Increase value of cities needing air defense more
					iValue *= std::max(1, 3 + iNeedAirDefenders - iExistingAirDefenders);

					if( GET_PLAYER(getOwnerINLINE()).AI_isPrimaryArea(pLoopCity->area()) )
					{
						iValue *= 4;
						iValue /= 3;
					}

					// Reduce value of endangered city, it may be too late to help
					if (3*iEnemyOffense > iOurDefense || iOurDefense == 0)
					{
						iValue *= iOurDefense;
						iValue /= std::max(1,3*iEnemyOffense);
					}

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pBestPlot = pLoopCity->plot();
					}
				}
			}
		}
	}

	if (pBestPlot != NULL && !atPlot(pBestPlot))
	{
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}
	/********************************************************************************/
	/* 	BETTER_BTS_AI_MOD						END								*/
	/********************************************************************************/

	return false;
}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_airCarrier()
{
	PROFILE_FUNC();

	CvUnit* pLoopUnit;
	CvUnit* pBestUnit;
	int iValue;
	int iBestValue;
	int iLoop;

	if (getCargo() > 0)
	{
		return false;
	}

	if (isCargo())
	{
		if (canAirDefend())
		{
			getGroup()->pushMission(MISSION_AIRPATROL);
			return true;
		}
		else
		{
			getGroup()->pushMission(MISSION_SKIP);
			return true;
		}
	}

	iBestValue = 0;
	pBestUnit = NULL;

	for(pLoopUnit = GET_PLAYER(getOwnerINLINE()).firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = GET_PLAYER(getOwnerINLINE()).nextUnit(&iLoop))
	{
		if (canLoadUnit(pLoopUnit, pLoopUnit->plot()))
		{
			iValue = 10;

			if (!(pLoopUnit->plot()->isCity()))
			{
				iValue += 20;
			}

			if (pLoopUnit->plot()->isOwned())
			{
				if (isEnemy(pLoopUnit->plot()->getTeam(), pLoopUnit->plot()))
				{
					iValue += 20;
				}
			}
			else
			{
				iValue += 10;
			}

			iValue /= (pLoopUnit->getCargo() + 1);

			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				pBestUnit = pLoopUnit;
			}
		}
	}

	if (pBestUnit != NULL)
	{
		if (atPlot(pBestUnit->plot()))
		{
			setTransportUnit(pBestUnit); // XXX is this dangerous (not pushing a mission...) XXX air units?
			return true;
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestUnit->getX_INLINE(), pBestUnit->getY_INLINE());
			return true;
		}
	}

	return false;
}

bool CvUnitAI::AI_missileLoad(UnitAITypes eTargetUnitAI, int iMaxOwnUnitAI, bool bStealthOnly)
{
	PROFILE_FUNC();

	CvUnit* pBestUnit = NULL;
	int iBestValue = 0;
	int iLoop;
	CvUnit* pLoopUnit;
	for(pLoopUnit = GET_PLAYER(getOwnerINLINE()).firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = GET_PLAYER(getOwnerINLINE()).nextUnit(&iLoop))
	{
/*************************************************************************************************/
/**	CandyMan								04/04/09								Xienwolf	**/
/**																								**/
/**							Allows Multiple Invisible types on a Unit							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		if (!bStealthOnly || pLoopUnit->getInvisibleType() != NO_INVISIBLE)
/**								----  End Original Code  ----									**/
		if (!bStealthOnly || pLoopUnit->getNumInvisibleTypes() > 0)
/*************************************************************************************************/
/**	CandyMan								END													**/
/*************************************************************************************************/
		{
			if (pLoopUnit->AI_getUnitAIType() == eTargetUnitAI)
			{
				if ((iMaxOwnUnitAI == -1) || (pLoopUnit->getUnitAICargo(AI_getUnitAIType()) <= iMaxOwnUnitAI))
				{
					if (canLoadUnit(pLoopUnit, pLoopUnit->plot()))
					{
						int iValue = 100;

						iValue += GC.getGame().getSorenRandNum(100, "AI missile load");

						iValue *= 1 + pLoopUnit->getCargo();

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestUnit = pLoopUnit;
						}
					}
				}
			}
		}
	}

	if (pBestUnit != NULL)
	{
		if (atPlot(pBestUnit->plot()))
		{
			setTransportUnit(pBestUnit); // XXX is this dangerous (not pushing a mission...) XXX air units?
			return true;
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestUnit->getX_INLINE(), pBestUnit->getY_INLINE());
			setTransportUnit(pBestUnit);
			return true;
		}
	}

	return false;

}


// Returns true if a mission was pushed...
bool CvUnitAI::AI_airStrike()
{
	PROFILE_FUNC();

	CvUnit* pDefender;
	CvUnit* pInterceptor;
	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	int iSearchRange;
	int iDamage;
	int iPotentialAttackers;
	int iInterceptProb;
	int iValue;
	int iBestValue;
	int iDX, iDY;

	iSearchRange = airRange();

	iBestValue = (isSuicide() && m_pUnitInfo->getProductionCost() > 0) ? (5 * m_pUnitInfo->getProductionCost()) / 6 : 0;
	pBestPlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (canMoveInto(pLoopPlot, true))
				{
					iValue = 0;
					iPotentialAttackers = GET_PLAYER(getOwnerINLINE()).AI_adjacentPotentialAttackers(pLoopPlot);
					if (pLoopPlot->isCity())
					{
						iPotentialAttackers += GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_ASSAULT, getGroup(), 1) * 2;
					}
					/********************************************************************************/
					/* 	BETTER_BTS_AI_MOD						10/13/08		jdog5000		*/
					/* 																			*/
					/* 	Air AI																	*/
					/********************************************************************************/
					/* original BTS code
					if (pLoopPlot->isWater() || (iPotentialAttackers > 0) || pLoopPlot->isAdjacentTeam(getTeam()))
					*/
					// Bombers will always consider striking units adjacent to this team's territory
					// to soften them up for potential attack.  This situation doesn't apply if this team's adjacent
					// territory is water, land units won't be able to reach easily for attack
					if (pLoopPlot->isWater() || (iPotentialAttackers > 0) || pLoopPlot->isAdjacentTeam(getTeam(),true))
					/********************************************************************************/
					/* 	BETTER_BTS_AI_MOD						END								*/
					/********************************************************************************/
					{
						pDefender = pLoopPlot->getBestDefender(NO_PLAYER, getOwnerINLINE(), this, true);

						FAssert(pDefender != NULL);
						FAssert(pDefender->canDefend());

						// XXX factor in air defenses...

						iDamage = airCombatDamage(pDefender);

						iValue = std::max(0, (std::min((pDefender->getDamage() + iDamage), airCombatLimit()) - pDefender->getDamage()));

						iValue += ((((iDamage * collateralDamage()) / 100) * std::min((pLoopPlot->getNumVisibleEnemyDefenders(this) - 1), collateralDamageMaxUnits())) / 2);

						iValue *= (3 + iPotentialAttackers);
						iValue /= 4;

						pInterceptor = bestInterceptor(pLoopPlot);

						if (pInterceptor != NULL)
						{
							iInterceptProb = isSuicide() ? 100 : pInterceptor->currInterceptionProbability();

							iInterceptProb *= std::max(0, (100 - evasionProbability()));
							iInterceptProb /= 100;

							iValue *= std::max(0, 100 - iInterceptProb / 2);
							iValue /= 100;
						}

						if (pLoopPlot->isWater())
						{
							iValue *= 3;
						}

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = pLoopPlot;
							FAssert(!atPlot(pBestPlot));
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						9/16/08			jdog5000		*/
/* 																			*/
/* 	Air AI																	*/
/********************************************************************************/
// Air strike focused on weakening enemy stacks threatening our cities
// Returns true if a mission was pushed...
bool CvUnitAI::AI_defensiveAirStrike()
{
	PROFILE_FUNC();

	CvUnit* pDefender;
	CvUnit* pInterceptor;
	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	int iSearchRange;
	int iDamage;
	int iInterceptProb;
	int iValue;
	int iBestValue;
	int iDX, iDY;

	iSearchRange = airRange();

	iBestValue = (isSuicide() && m_pUnitInfo->getProductionCost() > 0) ? (60 * m_pUnitInfo->getProductionCost()) : 0;
	pBestPlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (canMoveInto(pLoopPlot, true)) // Only true of plots this unit can airstrike
				{
					// Only attack enemy land units near our cities
					if( pLoopPlot->isPlayerCityRadius(getOwnerINLINE()) && !pLoopPlot->isWater() )
					{
						CvCity* pClosestCity = GC.getMapINLINE().findCity(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), getOwnerINLINE(), getTeam(), true, false);

						if( pClosestCity != NULL )
						{
							// City and pLoopPlot forced to be in same area, check they're still close
							int iStepDist = plotDistance(pClosestCity->getX_INLINE(), pClosestCity->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());

							if( iStepDist < 3 )
							{
								iValue = 0;

								pDefender = pLoopPlot->getBestDefender(NO_PLAYER, getOwnerINLINE(), this, true);

								FAssert(pDefender != NULL);
								FAssert(pDefender->canDefend());

								iDamage = airCombatDamage(pDefender);

								iValue = std::max(0, (std::min((pDefender->getDamage() + iDamage), airCombatLimit()) - pDefender->getDamage()));

								iValue += ((((iDamage * collateralDamage()) / 100) * std::min((pLoopPlot->getNumVisibleEnemyDefenders(this) - 1), collateralDamageMaxUnits())) / 2);

								iValue *= GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(pClosestCity->plot(),2,false,false);
								iValue /= std::max(1, GET_TEAM(getTeam()).AI_getOurPlotStrength(pClosestCity->plot(),0,true,false,true));

								if( iStepDist == 1 )
								{
									iValue *= 5;
									iValue /= 4;
								}

								pInterceptor = bestInterceptor(pLoopPlot);

								if (pInterceptor != NULL)
								{
									iInterceptProb = isSuicide() ? 100 : pInterceptor->currInterceptionProbability();

									iInterceptProb *= std::max(0, (100 - evasionProbability()));
									iInterceptProb /= 100;

									iValue *= std::max(0, 100 - iInterceptProb / 2);
									iValue /= 100;
								}

								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									pBestPlot = pLoopPlot;
									FAssert(!atPlot(pBestPlot));
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}

// Air strike around base city
// Returns true if a mission was pushed...
bool CvUnitAI::AI_defendBaseAirStrike()
{
	PROFILE_FUNC();

	CvUnit* pDefender;
	CvUnit* pInterceptor;
	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	int iDamage;
	int iInterceptProb;
	int iValue;
	int iBestValue;
	int iDX, iDY;

	// Only search around base
	int iSearchRange = 2;

	iBestValue = (isSuicide() && m_pUnitInfo->getProductionCost() > 0) ? (15 * m_pUnitInfo->getProductionCost()) : 0;
	pBestPlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (canMoveInto(pLoopPlot, true) && !pLoopPlot->isWater()) // Only true of plots this unit can airstrike
				{
					if( plot()->area() == pLoopPlot->area() )
					{
						iValue = 0;

						pDefender = pLoopPlot->getBestDefender(NO_PLAYER, getOwnerINLINE(), this, true);

						FAssert(pDefender != NULL);
						FAssert(pDefender->canDefend());

						iDamage = airCombatDamage(pDefender);

						iValue = std::max(0, (std::min((pDefender->getDamage() + iDamage), airCombatLimit()) - pDefender->getDamage()));

						iValue += ((iDamage * collateralDamage()) * std::min((pLoopPlot->getNumVisibleEnemyDefenders(this) - 1), collateralDamageMaxUnits())) / (2*100);

						// Weight towards stronger units
						iValue *= (pDefender->currCombatStr(NULL,NULL,NULL) + 2000);
						iValue /= 2000;

						// Weight towards adjacent stacks
						if( plotDistance(plot()->getX_INLINE(), plot()->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()) == 1 )
						{
							iValue *= 5;
							iValue /= 4;
						}

						pInterceptor = bestInterceptor(pLoopPlot);

						if (pInterceptor != NULL)
						{
							iInterceptProb = isSuicide() ? 100 : pInterceptor->currInterceptionProbability();

							iInterceptProb *= std::max(0, (100 - evasionProbability()));
							iInterceptProb /= 100;

							iValue *= std::max(0, 100 - iInterceptProb / 2);
							iValue /= 100;
						}

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = pLoopPlot;
							FAssert(!atPlot(pBestPlot));
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

bool CvUnitAI::AI_airBombPlots()
{
	PROFILE_FUNC();

	CvUnit* pInterceptor;
	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	int iSearchRange;
	int iInterceptProb;
	int iValue;
	int iBestValue;
	int iDX, iDY;

	iSearchRange = airRange();

	iBestValue = 0;
	pBestPlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot	= plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (!pLoopPlot->isCity() && pLoopPlot->isOwned() && pLoopPlot != plot())
				{
					if (canAirBombAt(plot(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()))
					{
						iValue = 0;

						if (pLoopPlot->getBonusType(pLoopPlot->getTeam()) != NO_BONUS)
						{
							iValue += AI_pillageValue(pLoopPlot, 15);

							iValue += GC.getGameINLINE().getSorenRandNum(10, "AI Air Bomb");
						}
						else if (isSuicide())
						{
							//This should only be reached when the unit is desperate to die
							iValue += AI_pillageValue(pLoopPlot);
							// Guided missiles lean towards destroying resource-producing tiles as opposed to improvements like Towns
							if (pLoopPlot->getBonusType(pLoopPlot->getTeam()) != NO_BONUS)
							{
								//and even more so if it's a resource
								iValue += GET_PLAYER(pLoopPlot->getOwnerINLINE()).AI_bonusVal(pLoopPlot->getBonusType(pLoopPlot->getTeam()));
							}
						}

						if (iValue > 0)
						{

							pInterceptor = bestInterceptor(pLoopPlot);

							if (pInterceptor != NULL)
							{
								iInterceptProb = isSuicide() ? 100 : pInterceptor->currInterceptionProbability();

								iInterceptProb *= std::max(0, (100 - evasionProbability()));
								iInterceptProb /= 100;

								iValue *= std::max(0, 100 - iInterceptProb / 2);
								iValue /= 100;
							}

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = pLoopPlot;
								FAssert(!atPlot(pBestPlot));
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_AIRBOMB, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}


bool CvUnitAI::AI_airBombDefenses()
{
	PROFILE_FUNC();

	CvCity* pCity;
	CvUnit* pInterceptor;
	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	int iSearchRange;
	int iPotentialAttackers;
	int iInterceptProb;
	int iValue;
	int iBestValue;
	int iDX, iDY;

	iSearchRange = airRange();

	iBestValue = 0;
	pBestPlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot	= plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				pCity = pLoopPlot->getPlotCity();
				if (pCity != NULL)
				{
					iValue = 0;

					if (canAirBombAt(plot(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()))
					{
						iPotentialAttackers = GET_PLAYER(getOwnerINLINE()).AI_adjacentPotentialAttackers(pLoopPlot);
						iPotentialAttackers += std::max(0, GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pCity->plot(), NO_MISSIONAI, getGroup(), 2) - 4);

						if (iPotentialAttackers > 1)
						{
							iValue += std::max(0, (std::min((pCity->getDefenseDamage() + airBombCurrRate()), GC.getMAX_CITY_DEFENSE_DAMAGE()) - pCity->getDefenseDamage()));

							iValue *= 4 + iPotentialAttackers;

							if (pCity->AI_isDanger())
							{
								iValue *= 2;
							}

							if (pCity == pCity->area()->getTargetCity(getOwnerINLINE()))
							{
								iValue *= 2;
							}
						}

						if (iValue > 0)
						{
							pInterceptor = bestInterceptor(pLoopPlot);

							if (pInterceptor != NULL)
							{
								iInterceptProb = isSuicide() ? 100 : pInterceptor->currInterceptionProbability();

								iInterceptProb *= std::max(0, (100 - evasionProbability()));
								iInterceptProb /= 100;

								iValue *= std::max(0, 100 - iInterceptProb / 2);
								iValue /= 100;
							}

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = pLoopPlot;
								FAssert(!atPlot(pBestPlot));
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_AIRBOMB, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;

}

bool CvUnitAI::AI_exploreAir()
{
	PROFILE_FUNC();

	CvPlayer& kPlayer = GET_PLAYER(getOwnerINLINE());
	int iLoop;
	CvCity* pLoopCity;
	CvPlot* pBestPlot = NULL;
	int iBestValue = 0;

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive() && !GET_PLAYER((PlayerTypes)iI).isBarbarian())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() != getTeam())
			{
				for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
				{
					if (!pLoopCity->isVisible(getTeam(), false))
					{
						if (canReconAt(plot(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE()))
						{
							int iValue = 1 + GC.getGame().getSorenRandNum(15, "AI explore air");
							if (isEnemy(GET_PLAYER((PlayerTypes)iI).getTeam()))
							{
								iValue += 10;
								iValue += std::min(10,  pLoopCity->area()->getNumAIUnits(getOwnerINLINE(), UNITAI_ATTACK_CITY));
								iValue += 10 * kPlayer.AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_ASSAULT);
							}

							iValue *= plotDistance(getX_INLINE(), getY_INLINE(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE());

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = pLoopCity->plot();
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_RECON, pBestPlot->getX(), pBestPlot->getY());
		return true;
	}

	return false;
}

/*************************************************************************************************/
/**	Improved AI							06/05/11										Snarko	**/
/**						Getting AI to use air units correctly									**/
/*************************************************************************************************/
bool CvUnitAI::AI_exploreAirNear(int iRange)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot = NULL;
	CvPlot* pBestPlot = NULL;
	int iBestValue = 0;
	int iX, iY;
	if (!canRecon(plot()))
	{
		return false;
	}

	for (iX = -(iRange); iX <= iRange; iX++)
	{
		for (iY = -(iRange); iY <= iRange; iY++)
		{
			pLoopPlot = plotXY(plot()->getX_INLINE(), plot()->getY_INLINE(), iX, iY);
			if (canReconAt(plot(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()))
			{
				int iValue = 1 + GC.getGame().getSorenRandNum(5, "AI explore air near");
				if (pLoopPlot->getOwner() != NO_PLAYER)
				{
					if (isEnemy(GET_PLAYER(pLoopPlot->getOwner()).getTeam()))
					{
						iValue += 2;
					}
				}

				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					pBestPlot = pLoopPlot;
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_RECON, pBestPlot->getX(), pBestPlot->getY());
		return true;
	}

	return false;
}
/*************************************************************************************************/
/**	Improved AI									END												**/
/*************************************************************************************************/
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/02/09                                jdog5000      */
/*                                                                                              */
/* Player Interface                                                                             */
/************************************************************************************************/
int CvUnitAI::AI_exploreAirPlotValue( CvPlot* pPlot )
{
	int iValue = 0;
	if (pPlot->isVisible(getTeam(), false))
	{
		iValue++;

		if (!pPlot->isOwned())
		{
			iValue++;
		}

		if (!pPlot->isImpassable())
		{
			iValue *= 4;

			if (pPlot->isWater() || pPlot->getArea() == getArea())
			{
				iValue *= 2;
			}
		}
	}

	return iValue;
}

bool CvUnitAI::AI_exploreAir2()
{
	PROFILE_FUNC();

	CvPlayer& kPlayer = GET_PLAYER(getOwner());
	CvPlot* pLoopPlot = NULL;
	CvPlot* pBestPlot = NULL;
	int iBestValue = 0;

	int iDX, iDY;
	int iSearchRange = airRange();
	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if( pLoopPlot != NULL )
			{
				if( !pLoopPlot->isVisible(getTeam(),false) )
				{
					if (canReconAt(plot(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()))
					{
						int iValue = AI_exploreAirPlotValue( pLoopPlot );

						for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
						{
							DirectionTypes eDirection = (DirectionTypes) iI;
							CvPlot* pAdjacentPlot = plotDirection(getX_INLINE(), getY_INLINE(), eDirection);
							if (pAdjacentPlot != NULL)
							{
								if( !pAdjacentPlot->isVisible(getTeam(),false) )
								{
									iValue += AI_exploreAirPlotValue( pAdjacentPlot );
								}
							}
						}

						iValue += GC.getGame().getSorenRandNum(25, "AI explore air");
						iValue *= std::min(7, plotDistance(getX_INLINE(), getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()));

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = pLoopPlot;
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_RECON, pBestPlot->getX(), pBestPlot->getY());
		return true;
	}

	return false;
}

void CvUnitAI::AI_exploreAirMove()
{
	if( AI_exploreAir() )
	{
		return;
	}

	if( AI_exploreAir2() )
	{
		return;
	}

	if( canAirDefend() )
	{
		getGroup()->pushMission(MISSION_AIRPATROL);
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

// Returns true if a mission was pushed...
bool CvUnitAI::AI_nuke()
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	CvCity* pBestCity;
	int iValue;
	int iBestValue;
	int iLoop;
	int iI;

	pBestCity = NULL;

	iBestValue = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive() && !GET_PLAYER((PlayerTypes)iI).isBarbarian())
		{
			if (isEnemy(GET_PLAYER((PlayerTypes)iI).getTeam()))
			{
				if (GET_PLAYER(getOwnerINLINE()).AI_getAttitude((PlayerTypes)iI) == ATTITUDE_FURIOUS)
				{
					for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
					{
						if (canNukeAt(plot(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE()))
						{
							iValue = AI_nukeValue(pLoopCity);

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestCity = pLoopCity;
								FAssert(pBestCity->getTeam() != getTeam());
							}
						}
					}
				}
			}
		}
	}

	if (pBestCity != NULL)
	{
		getGroup()->pushMission(MISSION_NUKE, pBestCity->getX_INLINE(), pBestCity->getY_INLINE());
		return true;
	}

	return false;
}

bool CvUnitAI::AI_nukeRange(int iRange)
{
	CvPlot* pBestPlot = NULL;
	int iBestValue = 0;
	for (int iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (int iDY = -(iRange); iDY <= iRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (canNukeAt(plot(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()))
				{
					int iValue = -99;

					for (int iDX2 = -(nukeRange()); iDX2 <= nukeRange(); iDX2++)
					{
						for (int iDY2 = -(nukeRange()); iDY2 <= nukeRange(); iDY2++)
						{
							CvPlot* pLoopPlot2 = plotXY(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), iDX2, iDY2);

							if (pLoopPlot2 != NULL)
							{
								int iEnemyCount = 0;
								int iTeamCount = 0;
								int iNeutralCount = 0;
								int iDamagedEnemyCount = 0;

								CLLNode<IDInfo>* pUnitNode;
								CvUnit* pLoopUnit;
								pUnitNode = pLoopPlot2->headUnitNode();
								while (pUnitNode != NULL)
								{
									pLoopUnit = ::getUnit(pUnitNode->m_data);
									pUnitNode = pLoopPlot2->nextUnitNode(pUnitNode);

									if (!pLoopUnit->isNukeImmune())
									{
										if (pLoopUnit->getTeam() == getTeam())
										{
											iTeamCount++;
										}
										else if (!pLoopUnit->isInvisible(getTeam(), false))
										{
											if (isEnemy(pLoopUnit->getTeam()))
											{
												iEnemyCount++;
/*************************************************************************************************/
/**	Higher hitpoints				01/02/11											Snarko	**/
/**						Makes higher values than 100 HP possible.								**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
												if (pLoopUnit->getDamage() * 2 > pLoopUnit->maxHitPoints())
/**								----  End Original Code  ----									**/
												if (pLoopUnit->getDamageReal() * 2 > pLoopUnit->maxHitPoints())
/*************************************************************************************************/
/**	Higher hitpoints						END													**/
/*************************************************************************************************/
												{
													iDamagedEnemyCount++;
												}
											}
											else
											{
												iNeutralCount++;
											}
										}
									}
								}

								iValue += (iEnemyCount + iDamagedEnemyCount) * (pLoopPlot2->isWater() ? 25 : 12);
								iValue -= iTeamCount * 15;
								iValue -= iNeutralCount * 20;


								int iMultiplier = 1;
								if (pLoopPlot2->getTeam() == getTeam())
								{
									iMultiplier = -2;
								}
								else if (isEnemy(pLoopPlot2->getTeam()))
								{
									iMultiplier = 1;
								}
								else if (!pLoopPlot2->isOwned())
								{
									iMultiplier = 0;
								}
								else
								{
									iMultiplier = -10;
								}

								if (pLoopPlot2->getImprovementType() != NO_IMPROVEMENT)
								{
									iValue += iMultiplier * 10;
								}

								/********************************************************************************/
								/* 	BETTER_BTS_AI_MOD						7/31/08				jdog5000	*/
								/* 																			*/
								/* 	Bugfix																	*/
								/********************************************************************************/
								// This could also have been considered a minor AI cheat
								//if (pLoopPlot2->getBonusType() != NO_BONUS)
								if (pLoopPlot2->getNonObsoleteBonusType(getTeam()) != NO_BONUS)
								{
									iValue += iMultiplier * 20;
								}
								/********************************************************************************/
								/* 	BETTER_BTS_AI_MOD						END								*/
								/********************************************************************************/

								if (pLoopPlot2->isCity())
								{
									iValue += std::max(0, iMultiplier * (-20 + 15 * pLoopPlot2->getPlotCity()->getPopulation()));
								}
							}
						}
					}

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pBestPlot = pLoopPlot;
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		getGroup()->pushMission(MISSION_NUKE, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}

bool CvUnitAI::AI_trade(int iValueThreshold)
{
	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	CvPlot* pBestTradePlot;

	int iPathTurns;
	int iValue;
	int iBestValue;
	int iLoop;
	int iI;


	iBestValue = 0;
	pBestPlot = NULL;
	pBestTradePlot = NULL;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
			{
				if (AI_plotValid(pLoopCity->plot()))
				{
					if (getTeam() != pLoopCity->getTeam())
					{
						iValue = getTradeGold(pLoopCity->plot());

						if ((iValue >= iValueThreshold) && canTrade(pLoopCity->plot(), true))
						{
							if (!(pLoopCity->plot()->isVisibleEnemyUnit(this)))
							{
								if (generatePath(pLoopCity->plot(), 0, true, &iPathTurns))
								{
									FAssert(iPathTurns > 0);

									iValue /= (4 + iPathTurns);

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = getPathEndTurnPlot();
										pBestTradePlot = pLoopCity->plot();
									}
								}

							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestTradePlot != NULL))
	{
		if (atPlot(pBestTradePlot))
		{
			getGroup()->pushMission(MISSION_TRADE);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
			return true;
		}
	}

	return false;
}

bool CvUnitAI::AI_infiltrate()
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	CvPlot* pBestPlot;

	int iPathTurns;
	int iValue;
	int iBestValue;
	int iLoop;
	int iI;

	iBestValue = 0;
	pBestPlot = NULL;

	if (canInfiltrate(plot()))
	{
		getGroup()->pushMission(MISSION_INFILTRATE);
		return true;
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if ((GET_PLAYER((PlayerTypes)iI).isAlive()) && GET_PLAYER((PlayerTypes)iI).getTeam() != getTeam())
		{
			for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
			{
				if (canInfiltrate(pLoopCity->plot()))
				{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/22/10                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
					// BBAI efficiency: check area for land units before generating path
					if( (getDomainType() == DOMAIN_LAND) && (pLoopCity->area() != area()) && !(getGroup()->canMoveAllTerrain()) )
					{
						continue;
					}

					iValue = getEspionagePoints(pLoopCity->plot());

					if (iValue > iBestValue)
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
					{
						if (generatePath(pLoopCity->plot(), 0, true, &iPathTurns))
						{
							FAssert(iPathTurns > 0);

							if (getPathLastNode()->m_iData1 == 0)
							{
								iPathTurns++;
							}

							iValue /= 1 + iPathTurns;

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = pLoopCity->plot();
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL))
	{
		if (atPlot(pBestPlot))
		{
			getGroup()->pushMission(MISSION_INFILTRATE);
			return true;
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
			getGroup()->pushMission(MISSION_INFILTRATE, -1, -1, 0, (getGroup()->getLengthMissionQueue() > 0));
			return true;
		}
	}

	return false;
}

bool CvUnitAI::AI_reconSpy(int iRange)
{
	PROFILE_FUNC();
	CvPlot* pLoopPlot;
	int iX, iY;

	CvPlot* pBestPlot = NULL;
	CvPlot* pBestTargetPlot = NULL;
	int iBestValue = 0;

	int iSearchRange = AI_searchRange(iRange);

	for (iX = -iSearchRange; iX <= iSearchRange; iX++)
	{
		for (iY = -iSearchRange; iY <= iSearchRange; iY++)
		{
			pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iX, iY);
			int iDistance = stepDistance(0, 0, iX, iY);
			if ((iDistance > 0) && (pLoopPlot != NULL) && AI_plotValid(pLoopPlot))
			{
				int iValue = 0;
				if (pLoopPlot->getPlotCity() != NULL)
				{
					iValue += GC.getGameINLINE().getSorenRandNum(4000, "AI Spy Scout City");
				}

				if (pLoopPlot->getBonusType(getTeam()) != NO_BONUS)
				{
					iValue += GC.getGameINLINE().getSorenRandNum(1000, "AI Spy Recon Bonus");
				}

				for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
				{
					CvPlot* pAdjacentPlot = plotDirection(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), ((DirectionTypes)iI));

					if (pAdjacentPlot != NULL)
					{
						if (!pAdjacentPlot->isRevealed(getTeam(), false))
						{
							iValue += 500;
						}
						else if (!pAdjacentPlot->isVisible(getTeam(), false))
						{
							iValue += 200;
						}
					}
				}


				if (iValue > 0)
				{
					int iPathTurns;
					if (generatePath(pLoopPlot, 0, true, &iPathTurns, iRange))
					{
						if (iPathTurns <= iRange)
						{
							// don't give each and every plot in range a value before generating the patch (performance hit)
							iValue += GC.getGameINLINE().getSorenRandNum(250, "AI Spy Scout Best Plot");

							iValue *= iDistance;

							/* Can no longer perform missions after having moved
							if (getPathLastNode()->m_iData2 == 1)
							{
								if (getPathLastNode()->m_iData1 > 0)
								{
									//Prefer to move and have movement remaining to perform a kill action.
									iValue *= 2;
								}
							} */

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestTargetPlot = getPathEndTurnPlot();
								pBestPlot = pLoopPlot;
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestTargetPlot != NULL))
	{
		if (atPlot(pBestTargetPlot))
		{
			getGroup()->pushMission(MISSION_SKIP);
			return true;
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestTargetPlot->getX_INLINE(), pBestTargetPlot->getY_INLINE());
			getGroup()->pushMission(MISSION_SKIP);
			return true;
		}
	}

	return false;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/25/09                               jdog5000        */
/*                                                                                               */
/* Espionage AI                                                                                  */
/************************************************************************************************/
/// \brief Spy decision on whether to cause revolt in besieged city
///
/// Have spy breakdown city defenses if we have troops in position to capture city this turn.
bool CvUnitAI::AI_revoltCitySpy()
{
	PROFILE_FUNC();

	CvCity* pCity = plot()->getPlotCity();

	FAssert(pCity != NULL);

	if( pCity == NULL )
	{
		return false;
	}

	if( !(GET_TEAM(getTeam()).isAtWar(pCity->getTeam())) )
	{
		return false;
	}

	if( pCity->isDisorder() )
	{
		return false;
	}

	int iOurPower = GET_PLAYER(getOwnerINLINE()).AI_getOurPlotStrength(plot(),1,false,true);
	int iEnemyDefensePower = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),0,true,false);
	int iEnemyPostPower = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),0,false,false);

	if( iOurPower > 2*iEnemyDefensePower )
	{
		return false;
	}

	if( iOurPower < iEnemyPostPower )
	{
		return false;
	}

	if( 10*iEnemyDefensePower < 11*iEnemyPostPower )
	{
		return false;
	}

	for (int iMission = 0; iMission < GC.getNumEspionageMissionInfos(); ++iMission)
	{
		CvEspionageMissionInfo& kMissionInfo = GC.getEspionageMissionInfo((EspionageMissionTypes)iMission);
		if ((kMissionInfo.getCityRevoltCounter() > 0) || (kMissionInfo.getPlayerAnarchyCounter() > 0))
		{
			if (!GET_PLAYER(getOwnerINLINE()).canDoEspionageMission((EspionageMissionTypes)iMission, pCity->getOwnerINLINE(), pCity->plot(), -1, this))
			{
				continue;
			}

			if (!espionage((EspionageMissionTypes)iMission, -1))
			{
				continue;
			}

			return true;
		}
	}

	return false;
}

int CvUnitAI::AI_getEspionageTargetValue(CvPlot* pPlot, int iMaxPath)
{
	PROFILE_FUNC();

	CvTeamAI& kTeam = GET_TEAM(getTeam());
	int iValue = 0;

	if (pPlot->isOwned() && pPlot->getTeam() != getTeam() && !GET_TEAM(getTeam()).isVassal(pPlot->getTeam()))
	{
		if (AI_plotValid(pPlot))
		{
			CvCity* pCity = pPlot->getPlotCity();
			if (pCity != NULL)
			{
				iValue += pCity->getPopulation();
				iValue += pCity->plot()->calculateCulturePercent(getOwnerINLINE())/8;

				// BBAI TODO: Should go to cities where missions will be cheaper ...

				int iRand = GC.getGame().getSorenRandNum(6, "AI spy choose city");
				iValue += iRand * iRand;

				if( area()->getTargetCity(getOwnerINLINE()) == pCity )
				{
					iValue += 30;
				}

				if( GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pPlot, MISSIONAI_ASSAULT, getGroup()) > 0 )
				{
					iValue += 30;
				}

				// BBAI TODO: What else?  If can see production, go for wonders and space race ...
			}
			else
			{
				BonusTypes eBonus = pPlot->getNonObsoleteBonusType(getTeam());
				if (eBonus != NO_BONUS)
				{
					iValue += GET_PLAYER(pPlot->getOwnerINLINE()).AI_baseBonusVal(eBonus) - 10;
				}
			}

			int iPathTurns;
			if (generatePath(pPlot, 0, true, &iPathTurns, iMaxPath))
			{
				if (iPathTurns <= iMaxPath)
				{
					if (kTeam.AI_getWarPlan(pPlot->getTeam()) == NO_WARPLAN)
					{
						iValue *= 1;
					}
					else if (kTeam.AI_isSneakAttackPreparing(pPlot->getTeam()))
					{
						iValue *= (pPlot->isCity()) ? 15 : 10;
					}
					else
					{
						iValue *= 3;
					}

					iValue *= 3;
					iValue /= (3 + GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pPlot, MISSIONAI_ATTACK_SPY, getGroup()));
				}
			}
		}
	}

	return iValue;
}


bool CvUnitAI::AI_cityOffenseSpy(int iMaxPath, CvCity* pSkipCity)
{
	PROFILE_FUNC();

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;

	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kLoopPlayer.isAlive() && kLoopPlayer.getTeam() != getTeam() && !GET_TEAM(getTeam()).isVassal(kLoopPlayer.getTeam()))
		{
			// Only move to cities where we will run missions
			if (GET_PLAYER(getOwnerINLINE()).AI_getAttitudeWeight((PlayerTypes)iPlayer) < (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 51 : 1)
				|| GET_TEAM(getTeam()).AI_getWarPlan(kLoopPlayer.getTeam()) != NO_WARPLAN
				|| GET_TEAM(getTeam()).getBestKnownTechScorePercent() < 85 )
			{
				int iLoop;
				for (CvCity* pLoopCity = kLoopPlayer.firstCity(&iLoop); NULL != pLoopCity; pLoopCity = kLoopPlayer.nextCity(&iLoop))
				{
					if( pLoopCity == pSkipCity )
					{
						continue;
					}

					if (pLoopCity->area() == area() || canMoveAllTerrain())
					{
						CvPlot* pLoopPlot = pLoopCity->plot();
						if (AI_plotValid(pLoopPlot))
						{
							int iValue = AI_getEspionageTargetValue(pLoopPlot, iMaxPath);
							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = pLoopPlot;
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		if (atPlot(pBestPlot))
		{
			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY);
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_ATTACK_SPY );
			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY);
		}
		return true;
	}

	return false;
}

bool CvUnitAI::AI_bonusOffenseSpy(int iRange)
{
	PROFILE_FUNC();

	CvPlot* pBestPlot = NULL;

	int iBestValue = 10;

	int iSearchRange = AI_searchRange(iRange);

	for (int iX = -iSearchRange; iX <= iSearchRange; iX++)
	{
		for (int iY = -iSearchRange; iY <= iSearchRange; iY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iX, iY);

			if (NULL != pLoopPlot && pLoopPlot->getBonusType(getTeam()) != NO_BONUS)
			{
				if( pLoopPlot->isOwned() && pLoopPlot->getTeam() != getTeam() )
				{
					// Only move to plots where we will run missions
					if (GET_PLAYER(getOwnerINLINE()).AI_getAttitudeWeight(pLoopPlot->getOwner()) < (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 51 : 1)
						|| GET_TEAM(getTeam()).AI_getWarPlan(pLoopPlot->getTeam()) != NO_WARPLAN )
					{
						int iValue = AI_getEspionageTargetValue(pLoopPlot, iRange);
						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = pLoopPlot;
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		if (atPlot(pBestPlot))
		{
			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY);
			return true;
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), 0, false, false, MISSIONAI_ATTACK_SPY);
			getGroup()->pushMission(MISSION_SKIP, -1, -1, 0, false, false, MISSIONAI_ATTACK_SPY);
			return true;
		}
	}

	return false;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

//Returns true if the spy performs espionage.
bool CvUnitAI::AI_espionageSpy()
{
	PROFILE_FUNC();

	if (!canEspionage(plot()))
	{
		return false;
	}

	EspionageMissionTypes eBestMission = NO_ESPIONAGEMISSION;
	CvPlot* pTargetPlot = NULL;
	PlayerTypes eTargetPlayer = NO_PLAYER;
	int iExtraData = -1;

	eBestMission = GET_PLAYER(getOwnerINLINE()).AI_bestPlotEspionage(plot(), eTargetPlayer, pTargetPlot, iExtraData);
	if (NO_ESPIONAGEMISSION == eBestMission)
	{
		return false;
	}

	if (!GET_PLAYER(getOwnerINLINE()).canDoEspionageMission(eBestMission, eTargetPlayer, pTargetPlot, iExtraData, this))
	{
		return false;
	}

	if (!espionage(eBestMission, iExtraData))
	{
		return false;
	}

	return true;
}

bool CvUnitAI::AI_moveToStagingCity()
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	CvPlot* pBestPlot;

	int iPathTurns;
	int iValue;
	int iBestValue;
	int iLoop;

	iBestValue = 0;
	pBestPlot = NULL;

	int iWarCount = 0;
	TeamTypes eTargetTeam = NO_TEAM;
	CvTeam& kTeam = GET_TEAM(getTeam());
	for (int iI = 0; iI < MAX_TEAMS; iI++)
	{
		if ((iI != getTeam()) && GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (kTeam.AI_isSneakAttackPreparing((TeamTypes)iI))
			{
				eTargetTeam = (TeamTypes)iI;
				iWarCount++;
			}
		}
	}
	if (iWarCount > 1)
	{
		eTargetTeam = NO_TEAM;
	}


	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/22/10                                jdog5000      */
/*                                                                                              */
/* War tactics AI, Efficiency                                                                   */
/************************************************************************************************/
		// BBAI efficiency: check same area
		if ((pLoopCity->area() == area()) && AI_plotValid(pLoopCity->plot()))
		{
			// BBAI TODO: Need some knowledge of whether this is a good city to attack from ... only get that
			// indirectly from threat.
			iValue = pLoopCity->AI_cityThreat();

			// Have attack stacks in assault areas move to coastal cities for faster loading
			if( (area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT) || (area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT_MASSING) )
			{
				CvArea* pWaterArea = pLoopCity->waterArea();
				if( pWaterArea != NULL && GET_TEAM(getTeam()).AI_isWaterAreaRelevant(pWaterArea) )
				{
					// BBAI TODO:  Need a better way to determine which cities should serve as invasion launch locations

					// Inertia so units don't just chase transports around the map
					iValue = iValue/2;
					if( pLoopCity->area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT )
					{
						// If in assault, transports may be at sea ... tend to stay where they left from
						// to speed reinforcement
						iValue += pLoopCity->plot()->plotCount(PUF_isAvailableUnitAITypeGroupie, UNITAI_ATTACK_CITY, -1, getOwnerINLINE());
					}

					// Attraction to cities which are serving as launch/pickup points
					iValue += 3*pLoopCity->plot()->plotCount(PUF_isUnitAIType, UNITAI_ASSAULT_SEA, -1, getOwnerINLINE());
					iValue += 2*pLoopCity->plot()->plotCount(PUF_isUnitAIType, UNITAI_ESCORT_SEA, -1, getOwnerINLINE());
					iValue += 5*GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopCity->plot(), MISSIONAI_PICKUP);
				}
				else
				{
					iValue = iValue/8;
				}
			}

			if (iValue*200 > iBestValue)
			{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				if (generatePath(pLoopCity->plot(), 0, true, &iPathTurns))
				{
					iValue *= 1000;
					iValue /= (5 + iPathTurns);
					if ((pLoopCity->plot() != plot()) && pLoopCity->isVisible(eTargetTeam, false))
					{
						iValue /= 2;
					}

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pBestPlot = getPathEndTurnPlot();
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		if (atPlot(pBestPlot))
		{
			getGroup()->pushMission(MISSION_SKIP);
			return true;
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
			return true;
		}
	}

	return false;
}

/*
bool CvUnitAI::AI_seaRetreatFromCityDanger()
{
	if (plot()->isCity(true) && GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 2) > 0) //prioritize getting outta there
	{
		if (AI_anyAttack(2, 40))
		{
			return true;
		}

		if (AI_anyAttack(4, 50))
		{
			return true;
		}

		if (AI_retreatToCity())
		{
			return true;
		}

		if (AI_safety())
		{
			return true;
		}
	}
	return false;
}

bool CvUnitAI::AI_airRetreatFromCityDanger()
{
	if (plot()->isCity(true))
	{
		CvCity* pCity = plot()->getPlotCity();
		if (GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 2) > 0 || (pCity != NULL && !pCity->AI_isDefended()))
		{
			if (AI_airOffensiveCity())
			{
				return true;
			}

			if (canAirDefend() && AI_airDefensiveCity())
			{
				return true;
			}
		}
	}
	return false;
}

bool CvUnitAI::AI_airAttackDamagedSkip()
{
	if (getDamage() == 0)
	{
		return false;
	}

	bool bSkip = (currHitPoints() * 100 / maxHitPoints() < 40);
	if (!bSkip)
	{
		int iSearchRange = airRange();
		bool bSkiesClear = true;
		for (int iDX = -iSearchRange; iDX <= iSearchRange && bSkiesClear; iDX++)
		{
			for (int iDY = -iSearchRange; iDY <= iSearchRange && bSkiesClear; iDY++)
			{
				CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);
				if (pLoopPlot != NULL)
				{
					if (bestInterceptor(pLoopPlot) != NULL)
					{
						bSkiesClear = false;
						break;
					}
				}
			}
		}
		bSkip = !bSkiesClear;
	}

	if (bSkip)
	{
		getGroup()->pushMission(MISSION_SKIP);
		return true;
	}

	return false;
}
*/

// Returns true if a mission was pushed or we should wait for another unit to bombard...
bool CvUnitAI::AI_followBombard()
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvPlot* pAdjacentPlot1;
	CvPlot* pAdjacentPlot2;
	int iI, iJ;

	if (canBombard(plot()))
	{
		getGroup()->pushMission(MISSION_BOMBARD);
		return true;
	}

	if (getDomainType() == DOMAIN_LAND)
	{
		for (iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
		{
			pAdjacentPlot1 = plotDirection(getX_INLINE(), getY_INLINE(), ((DirectionTypes)iI));

			if (pAdjacentPlot1 != NULL)
			{
				if (pAdjacentPlot1->isCity())
				{
					if (AI_potentialEnemy(pAdjacentPlot1->getTeam(), pAdjacentPlot1))
					{
						for (iJ = 0; iJ < NUM_DIRECTION_TYPES; iJ++)
						{
							pAdjacentPlot2 = plotDirection(pAdjacentPlot1->getX_INLINE(), pAdjacentPlot1->getY_INLINE(), ((DirectionTypes)iJ));

							if (pAdjacentPlot2 != NULL)
							{
								pUnitNode = pAdjacentPlot2->headUnitNode();

								while (pUnitNode != NULL)
								{
									pLoopUnit = ::getUnit(pUnitNode->m_data);
									pUnitNode = pAdjacentPlot2->nextUnitNode(pUnitNode);

									if (pLoopUnit->getOwnerINLINE() == getOwnerINLINE())
									{
										if (pLoopUnit->canBombard(pAdjacentPlot2))
										{
											if (pLoopUnit->isGroupHead())
											{
												if (pLoopUnit->getGroup() != getGroup())
												{
													if (pLoopUnit->getGroup()->readyToMove())
													{
														return true;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return false;
}


// Returns true if the unit has found a potential enemy...
bool CvUnitAI::AI_potentialEnemy(TeamTypes eTeam, const CvPlot* pPlot)
{
	PROFILE_FUNC();

	if (getGroup()->AI_isDeclareWar(pPlot))
	{
		return isPotentialEnemy(eTeam, pPlot);
	}
	else
	{
		return isEnemy(eTeam, pPlot);
	}
}


// Returns true if this plot needs some defense...
bool CvUnitAI::AI_defendPlot(CvPlot* pPlot)
{
	CvCity* pCity;

	if (!canDefend(pPlot))
	{
		return false;
	}

	pCity = pPlot->getPlotCity();

	if (pCity != NULL)
	{
		if (pCity->getOwnerINLINE() == getOwnerINLINE())
		{
			if (pCity->AI_isDanger())
			{
				return true;
			}
		}
	}
	else
	{
		if (pPlot->plotCount(PUF_canDefendGroupHead, -1, -1, getOwnerINLINE()) <= ((atPlot(pPlot)) ? 1 : 0))
		{
			if (pPlot->plotCount(PUF_cannotDefend, -1, -1, getOwnerINLINE()) > 0)
			{
				return true;
			}

//			if (pPlot->defenseModifier(getTeam(), false) >= 50 && pPlot->isRoute() && pPlot->getTeam() == getTeam())
//			{
//				return true;
//			}
		}
	}

	return false;
}


int CvUnitAI::AI_pillageValue(CvPlot* pPlot, int iBonusValueThreshold)
{
	CvPlot* pAdjacentPlot;
	ImprovementTypes eImprovement;
	BonusTypes eNonObsoleteBonus;
	int iValue;
	int iTempValue;
	int iBonusValue;
	int iI;

	FAssert(canPillage(pPlot) || canAirBombAt(plot(), pPlot->getX_INLINE(), pPlot->getY_INLINE()) || (getGroup()->getCargo() > 0));

	if (!(pPlot->isOwned()))
	{
		return 0;
	}

	iBonusValue = 0;
	eNonObsoleteBonus = pPlot->getNonObsoleteBonusType(pPlot->getTeam());
	if (eNonObsoleteBonus != NO_BONUS)
	{
		iBonusValue = (GET_PLAYER(pPlot->getOwnerINLINE()).AI_bonusVal(eNonObsoleteBonus));
	}

	if (iBonusValueThreshold > 0)
	{
		if (eNonObsoleteBonus == NO_BONUS)
		{
			return 0;
		}
		else if (iBonusValue < iBonusValueThreshold)
		{
			return 0;
		}
	}

	iValue = 0;

	if (getDomainType() != DOMAIN_AIR)
	{
		if (pPlot->isRoute())
		{
			iValue++;
			if (eNonObsoleteBonus != NO_BONUS)
			{
				iValue += iBonusValue * 4;
			}

			for (iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
			{
				pAdjacentPlot = plotDirection(getX_INLINE(), getY_INLINE(), ((DirectionTypes)iI));

				if (pAdjacentPlot != NULL && pAdjacentPlot->getTeam() == pPlot->getTeam())
				{
					if (pAdjacentPlot->isCity())
					{
						iValue += 10;
					}

					if (!(pAdjacentPlot->isRoute()))
					{
						if (!(pAdjacentPlot->isWater()) && !(pAdjacentPlot->isImpassable()))
						{
							iValue += 2;
						}
					}
				}
			}
		}
	}

	if (pPlot->getImprovementDuration() > ((pPlot->isWater()) ? 20 : 5))
	{
		eImprovement = pPlot->getImprovementType();
	}
	else
	{
		eImprovement = pPlot->getRevealedImprovementType(getTeam(), false);
	}

	if (eImprovement != NO_IMPROVEMENT)
	{
		if (pPlot->getWorkingCity() != NULL)
		{
			iValue += (pPlot->calculateImprovementYieldChange(eImprovement, YIELD_FOOD, pPlot->getOwnerINLINE()) * 5);
			iValue += (pPlot->calculateImprovementYieldChange(eImprovement, YIELD_PRODUCTION, pPlot->getOwnerINLINE()) * 4);
			iValue += (pPlot->calculateImprovementYieldChange(eImprovement, YIELD_COMMERCE, pPlot->getOwnerINLINE()) * 3);
		}

		if (getDomainType() != DOMAIN_AIR)
		{
			iValue += GC.getImprovementInfo(eImprovement).getPillageGold();
		}

		if (eNonObsoleteBonus != NO_BONUS)
		{
			if (GC.getImprovementInfo(eImprovement).isImprovementBonusTrade(eNonObsoleteBonus))
			{
				iTempValue = iBonusValue * 4;

				if (pPlot->isConnectedToCapital() && (pPlot->getPlotGroupConnectedBonus(pPlot->getOwnerINLINE(), eNonObsoleteBonus) == 1))
				{
					iTempValue *= 2;
				}

				iValue += iTempValue;
			}
		}
	}

	return iValue;
}


int CvUnitAI::AI_nukeValue(CvCity* pCity)
{
	PROFILE_FUNC();
	FAssertMsg(pCity != NULL, "City is not assigned a valid value");

	for (int iI = 0; iI < MAX_TEAMS; iI++)
	{
		CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iI);
		if (kLoopTeam.isAlive() && !isEnemy((TeamTypes)iI))
		{
			if (isNukeVictim(pCity->plot(), ((TeamTypes)iI)))
			{
				// Don't start wars with neutrals
				return 0;
			}
		}
	}

	int iValue = 1;

	iValue += GC.getGameINLINE().getSorenRandNum((pCity->getPopulation() + 1), "AI Nuke City Value");
	iValue += std::max(0, pCity->getPopulation() - 10);

	iValue += ((pCity->getPopulation() * (100 + pCity->calculateCulturePercent(pCity->getOwnerINLINE()))) / 100);

	iValue += -(GET_PLAYER(getOwnerINLINE()).AI_getAttitudeVal(pCity->getOwnerINLINE()) / 3);

	for (int iDX = -(nukeRange()); iDX <= nukeRange(); iDX++)
	{
		for (int iDY = -(nukeRange()); iDY <= nukeRange(); iDY++)
		{
			CvPlot* pLoopPlot = plotXY(pCity->getX_INLINE(), pCity->getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->getImprovementType() != NO_IMPROVEMENT)
				{
					iValue++;
				}

				/********************************************************************************/
				/* 	BETTER_BTS_AI_MOD						7/31/08				jdog5000	*/
				/* 																			*/
				/* 	Bugfix																	*/
				/********************************************************************************/
				//if (pLoopPlot->getBonusType() != NO_BONUS)
				if (pLoopPlot->getNonObsoleteBonusType(getTeam()) != NO_BONUS)
				{
					iValue++;
				}
				/********************************************************************************/
				/* 	BETTER_BTS_AI_MOD						END								*/
				/********************************************************************************/
			}
		}
	}

	if (!(pCity->isEverOwned(getOwnerINLINE())))
	{
		iValue *= 3;
		iValue /= 2;
	}

	if (!GET_TEAM(pCity->getTeam()).isAVassal())
	{
		iValue *= 2;
	}

	if (pCity->plot()->isVisible(getTeam(), false))
	{
		iValue += 2 * pCity->plot()->getNumVisibleEnemyDefenders(this);
	}
	else
	{
		iValue += 6;
	}

	return iValue;
}


int CvUnitAI::AI_searchRange(int iRange)
{
	if (iRange == 0)
	{
		return 0;
	}

	if (flatMovementCost() || (getDomainType() == DOMAIN_SEA))
	{
		return (iRange * baseMoves());
	}
	else
	{
		return ((iRange + 1) * (baseMoves() + 1));
	}
}


// XXX at some point test the game with and without this function...
bool CvUnitAI::AI_plotValid(CvPlot* pPlot)
{
	PROFILE_FUNC();

/*************************************************************************************************/
/**	Stumbler								04/04/09								Xienwolf	**/
/**																								**/
/**									Blocks Movement of Marked Units								**/
/*************************************************************************************************/
	if ((!pPlot->isVisible(getTeam(), false) || !pPlot->isRevealed(getTeam(), false)) && isBlind())
	{
		return false;
	}
/*************************************************************************************************/
/**	Stumbler								END													**/
/*************************************************************************************************/

/*************************************************************************************************/
/**	No Reveal 								05/15/08								Xienwolf	**/
/**																								**/
/**							Adds Promotion Value to the Unit Logic Check						**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	if (m_pUnitInfo->isNoRevealMap() && willRevealByMove(pPlot))
/**								----  End Original Code  ----									**/
	if ((m_pUnitInfo->isNoRevealMap() || isNoMapReveal()) && willRevealByMove(pPlot))
/*************************************************************************************************/
/**	No Reveal 									END												**/
/*************************************************************************************************/
	{
		return false;
	}
/*************************************************************************************************/
/**	MobileCage								 6/17/2009								Cyther		**/
/**	Expanded by Valkrionn					01/28/2010											**/
/**	Moved here by Snarko					16/02/2012											**/
/**										Leashes	a unit to a plot								**/
/*************************************************************************************************/
	if (isLeashed())
	{
		if (getLeashRange() >= 0)
		{
			if (getLeashUnit() != NULL)
			{
				if ((getLeashUnit()->getX() != INVALID_PLOT_COORD) && (getLeashUnit()->getY() != INVALID_PLOT_COORD))
				{
					int iLeashRange = getLeashRange();
					int iX = getLeashUnit()->getX();
					int iY = getLeashUnit()->getY();
					int iNewPlotDistance = (plotDistance(iX, iY, pPlot->getX_INLINE(), pPlot->getY_INLINE()));
					int iPlotDistance = (plotDistance(iX, iY, getX(), getY()));
					if (iNewPlotDistance > iLeashRange)
					{
						if (iNewPlotDistance > iPlotDistance)
						{
							return false;
						}
					}
				}
			}
			else if ((getLeashX() != INVALID_PLOT_COORD) && (getLeashY() != INVALID_PLOT_COORD))
			{
				int iLeashRange = getLeashRange();
				int iNewPlotDistance = (plotDistance(getLeashX(), getLeashY(), pPlot->getX_INLINE(), pPlot->getY_INLINE()));
				int iPlotDistance = (plotDistance(getLeashX(), getLeashY(), getX(), getY()));
				if (iNewPlotDistance > iLeashRange)
				{
					if (iNewPlotDistance > iPlotDistance)
					{
						return false;
					}
				}
			}
		}
	}
/*************************************************************************************************/
/**	MobileCage									END												**/
/*************************************************************************************************/

	switch (getDomainType())
	{
	case DOMAIN_SEA:
		if (pPlot->isWater() || canMoveAllTerrain())
		{
			return true;
		}
		else if (pPlot->isFriendlyCity(*this, true) && pPlot->isCoastalLand())
		{
			return true;
		}
		break;

	case DOMAIN_AIR:
		FAssert(false);
		break;

	case DOMAIN_LAND:
		if (pPlot->getArea() == getArea() || canMoveAllTerrain())
		{
			return true;
		}
		break;

	case DOMAIN_IMMOBILE:
		FAssert(false);
		break;

	default:
		FAssert(false);
		break;
	}

	return false;
}


int CvUnitAI::AI_finalOddsThreshold(CvPlot* pPlot, int iOddsThreshold)
{
	PROFILE_FUNC();

	CvCity* pCity;

	int iFinalOddsThreshold;

	iFinalOddsThreshold = iOddsThreshold;
/*************************************************************************************************/
/**	Xienwolf Tweak							12/22/08											**/
/**																								**/
/**					Catchall function to make the AI protect valuable units						**/
/*************************************************************************************************/
	if (getDuration() > 0)
	{
		return 0;
	}
	if (isLimitedUnitClass(getUnitClassType()))
	{
		iFinalOddsThreshold = iOddsThreshold + 10;
	}
	iFinalOddsThreshold += getExperience()/200;
	iFinalOddsThreshold = range(iFinalOddsThreshold, 0, std::max(iOddsThreshold, 95));
	if(isBarbarian())
	{
		iFinalOddsThreshold -= getGroup()->getNumUnits()*5;
	}
	if (getCommandLimit() > 0)
	{
		iFinalOddsThreshold += getCommandLimit()*5;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	pCity = pPlot->getPlotCity();

	if (pCity != NULL)
	{
		if (pCity->getDefenseDamage() < ((GC.getMAX_CITY_DEFENSE_DAMAGE() * 3) / 4))
		{
			iFinalOddsThreshold += std::max(0, (pCity->getDefenseDamage() - pCity->getLastDefenseDamage() - (GC.getDefineINT("CITY_DEFENSE_DAMAGE_HEAL_RATE") * 2)));
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/29/10                                jdog5000      */
/*                                                                                              */
/* War tactics AI                                                                               */
/************************************************************************************************/
/* original bts code
	if (pPlot->getNumVisiblePotentialEnemyDefenders(this) == 1)
	{
		if (pCity != NULL)
		{
			iFinalOddsThreshold *= 2;
			iFinalOddsThreshold /= 3;
		}
		else
		{
			iFinalOddsThreshold *= 7;
			iFinalOddsThreshold /= 8;
		}
	}

	if ((getDomainType() == DOMAIN_SEA) && !getGroup()->hasCargo())
	{
		iFinalOddsThreshold *= 3;
		iFinalOddsThreshold /= 2 + getGroup()->getNumUnits();
	}
	else
	{
		iFinalOddsThreshold *= 6;
		iFinalOddsThreshold /= (3 + GET_PLAYER(getOwnerINLINE()).AI_adjacentPotentialAttackers(pPlot, true) + ((stepDistance(getX_INLINE(), getY_INLINE(), pPlot->getX_INLINE(), pPlot->getY_INLINE()) > 1) ? 1 : 0) + ((AI_isCityAIType()) ? 2 : 0));
	}
*/
	int iDefenders = pPlot->getNumVisiblePotentialEnemyDefenders(this);

	// More aggressive if only one enemy defending city
	if (iDefenders == 1 && pCity != NULL)
	{
		iFinalOddsThreshold *= 2;
		iFinalOddsThreshold /= 3;
	}

	if ((getDomainType() == DOMAIN_SEA) && !getGroup()->hasCargo())
	{
		iFinalOddsThreshold *= 3 + (iDefenders/2);
		iFinalOddsThreshold /= 2 + getGroup()->getNumUnits();
	}
	else
	{
		iFinalOddsThreshold *= 6 + (iDefenders/((pCity != NULL) ? 1 : 2));
		int iDivisor = 3;
		iDivisor += GET_PLAYER(getOwnerINLINE()).AI_adjacentPotentialAttackers(pPlot, true);
		iDivisor += ((stepDistance(getX_INLINE(), getY_INLINE(), pPlot->getX_INLINE(), pPlot->getY_INLINE()) > 1) ? getGroup()->getNumUnits() : 0);
		iDivisor += (AI_isCityAIType() ? 2 : 0);
		iFinalOddsThreshold /= iDivisor;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	return range(iFinalOddsThreshold, 1, 99);
}


int CvUnitAI::AI_stackOfDoomExtra()
{
	return ((AI_getBirthmark() % (1 + GET_PLAYER(getOwnerINLINE()).getCurrentEra())) + 4);
}

bool CvUnitAI::AI_stackAttackCity(int iRange, int iPowerThreshold, bool bFollow)
{
	PROFILE_FUNC();
	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	int iSearchRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iDX, iDY;

	FAssert(canMove());

	if (bFollow)
	{
		iSearchRange = 1;
	}
	else
	{
		iSearchRange = AI_searchRange(iRange);
	}

	iBestValue = 0;
	pBestPlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (AI_plotValid(pLoopPlot))
				{
					if (pLoopPlot->isCity() || (pLoopPlot->isCity(true) && pLoopPlot->isVisibleEnemyUnit(this)))
					{
						if (AI_potentialEnemy(pLoopPlot->getTeam(), pLoopPlot))
						{
							if (!atPlot(pLoopPlot) && ((bFollow) ? canMoveInto(pLoopPlot, /*bAttack*/ true, /*bDeclareWar*/ true) : (generatePath(pLoopPlot, 0, true, &iPathTurns) && (iPathTurns <= iRange))))
							{
								iValue = getGroup()->AI_compareStacks(pLoopPlot, /*bPotentialEnemy*/ true, /*bCheckCanAttack*/ true, /*bCheckCanMove*/ true);

								if (iValue >= iPowerThreshold)
								{
									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = ((bFollow) ? pLoopPlot : getPathEndTurnPlot());
										FAssert(!atPlot(pBestPlot));
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), ((bFollow) ? MOVE_DIRECT_ATTACK : 0));
		return true;
	}

	return false;
}

bool CvUnitAI::AI_moveIntoCity(int iRange)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	int iSearchRange = iRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iDX, iDY;

	FAssert(canMove());

	iBestValue = 0;
	pBestPlot = NULL;

	if (plot()->isCity())
	{
		return false;
	}

	iSearchRange = AI_searchRange(iRange);

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (AI_plotValid(pLoopPlot) && (!isEnemy(pLoopPlot->getTeam(), pLoopPlot)))
				{
					if (pLoopPlot->isCity() || (pLoopPlot->isCity(true)))
					{
						if (canMoveInto(pLoopPlot, false) && (generatePath(pLoopPlot, 0, true, &iPathTurns) && (iPathTurns <= 1)))
						{
							iValue = 1;
							if (pLoopPlot->getPlotCity() != NULL)
							{
								 iValue += pLoopPlot->getPlotCity()->getPopulation();
							}

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = getPathEndTurnPlot();
								FAssert(!atPlot(pBestPlot));
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}

	return false;
}

//bolsters the culture of the weakest city.
//returns true if a mission is pushed.
bool CvUnitAI::AI_artistCultureVictoryMove()
{
	bool bGreatWork = false;
	bool bJoin = true;

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/08/10                                jdog5000      */
/*                                                                                              */
/* Victory Strategy AI                                                                          */
/************************************************************************************************/
	if (!(GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE, 1)))
	{
		return false;
	}

	if (GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE, 3))
	{
		//Great Work
		bGreatWork = true;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	int iCultureCitiesNeeded = GC.getGameINLINE().culturalVictoryNumCultureCities();
	FAssertMsg(iCultureCitiesNeeded > 0, "CultureVictory Strategy should not be true");

	CvCity* pLoopCity;
	CvPlot* pBestPlot;
	CvCity* pBestCity;
	SpecialistTypes eBestSpecialist;
	int iLoop, iValue, iBestValue;

	pBestPlot = NULL;
	eBestSpecialist = NO_SPECIALIST;

	pBestCity = NULL;

	iBestValue = 0;
	iLoop = 0;

	int iTargetCultureRank = iCultureCitiesNeeded;
	while (iTargetCultureRank > 0 && pBestCity == NULL)
	{
		for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
		{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
			// BBAI efficiency: check same area
			if ((pLoopCity->area() == area()) && AI_plotValid(pLoopCity->plot()))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			{
				// instead of commerce rate rank should use the culture on tile...
				if (pLoopCity->findCommerceRateRank(COMMERCE_CULTURE) == iTargetCultureRank)
				{
					// if the city is a fledgling, probably building culture, try next higher city
					if (pLoopCity->getCultureLevel() < 2)
					{
						break;
					}

					// if we cannot path there, try the next higher culture city
					if (!generatePath(pLoopCity->plot(), 0, true))
					{
						break;
					}

					pBestCity = pLoopCity;
					pBestPlot = pLoopCity->plot();
					if (bGreatWork)
					{
						if (canGreatWork(pBestPlot))
						{
							break;
						}
					}

					for (int iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
					{
						if (canJoin(pBestPlot, ((SpecialistTypes)iI)))
						{
							iValue = pLoopCity->AI_specialistValue(((SpecialistTypes)iI), pLoopCity->AI_avoidGrowth(), false);

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								eBestSpecialist = ((SpecialistTypes)iI);
							}
						}
					}

					if (eBestSpecialist == NO_SPECIALIST)
					{
						bJoin = false;
						if (canGreatWork(pBestPlot))
						{
							bGreatWork = true;
							break;
						}
						bGreatWork = false;
					}
					break;
				}
			}
		}

		iTargetCultureRank--;
	}


	FAssertMsg(bGreatWork || bJoin, "This wasn't a Great Artist");

	if (pBestCity == NULL)
	{
		//should try to airlift there...
		return false;
	}


	if (atPlot(pBestPlot))
	{
		if (bGreatWork)
		{
			getGroup()->pushMission(MISSION_GREAT_WORK);
			return true;
		}
		if (bJoin)
		{
			getGroup()->pushMission(MISSION_JOIN, eBestSpecialist);
			return true;
		}
		FAssert(false);
		return false;
	}
	else
	{
		FAssert(!atPlot(pBestPlot));
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
		return true;
	}
}

bool CvUnitAI::AI_poach()
{
	CvPlot* pLoopPlot;
	int iX, iY;

	int iBestPoachValue = 0;
	CvPlot* pBestPoachPlot = NULL;
	TeamTypes eBestPoachTeam = NO_TEAM;

	if (!GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI))
	{
		return false;
	}

	if (GET_TEAM(getTeam()).getNumMembers() > 1)
	{
		return false;
	}

	int iNoPoachRoll = GET_PLAYER(getOwnerINLINE()).AI_totalUnitAIs(UNITAI_WORKER);
	iNoPoachRoll += GET_PLAYER(getOwnerINLINE()).getNumCities();
	iNoPoachRoll = std::max(0, (iNoPoachRoll - 1) / 2);
	if (GC.getGameINLINE().getSorenRandNum(iNoPoachRoll, "AI Poach") > 0)
	{
		return false;
	}

	if (GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0)
	{
		return false;
	}

	FAssert(canAttack());



	int iRange = 1;
	//Look for a unit which is non-combat
	//and has a capture unit type
	for (iX = -iRange; iX <= iRange; iX++)
	{
		for (iY = -iRange; iY <= iRange; iY++)
		{
			if (iX != 0 && iY != 0)
			{
				pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iX, iY);
				if ((pLoopPlot != NULL) && (pLoopPlot->getTeam() != getTeam()) && pLoopPlot->isVisible(getTeam(), false))
				{
					int iPoachCount = 0;
					int iDefenderCount = 0;
					CvUnit* pPoachUnit = NULL;
					CLLNode<IDInfo>* pUnitNode = pLoopPlot->headUnitNode();
					while (pUnitNode != NULL)
					{
						CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
						pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);
						if ((pLoopUnit->getTeam() != getTeam())
							&& GET_TEAM(getTeam()).canDeclareWar(pLoopUnit->getTeam()))
						{
							if (!pLoopUnit->canDefend())
							{
								if (pLoopUnit->getCaptureUnitType(getCivilizationType()) != NO_UNIT)
								{
									iPoachCount++;
									pPoachUnit = pLoopUnit;
								}
							}
							else
							{
								iDefenderCount++;
							}
						}
					}

					if (pPoachUnit != NULL)
					{
						if (iDefenderCount == 0)
						{
							int iValue = iPoachCount * 100;
							iValue -= iNoPoachRoll * 25;
							if (iValue > iBestPoachValue)
							{
								iBestPoachValue = iValue;
								pBestPoachPlot = pLoopPlot;
								eBestPoachTeam = pPoachUnit->getTeam();
							}
						}
					}
				}
			}
		}
	}

	if (pBestPoachPlot != NULL)
	{
		//No war roll.
		if (!GET_TEAM(getTeam()).AI_performNoWarRolls(eBestPoachTeam))
		{
			GET_TEAM(getTeam()).declareWar(eBestPoachTeam, true, WARPLAN_LIMITED);

			FAssert(!atPlot(pBestPoachPlot));
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPoachPlot->getX_INLINE(), pBestPoachPlot->getY_INLINE(), MOVE_DIRECT_ATTACK);
			return true;
		}

	}

	return false;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/31/10                                jdog5000      */
/*                                                                                              */
/* War tactics AI                                                                               */
/************************************************************************************************/
bool CvUnitAI::AI_choke(int iRange, bool bDefensive)
{
	PROFILE_FUNC();

	bool bNoDefensiveBonus = noDefensiveBonus();
	if( getGroup()->getNumUnits() > 1 )
	{
		CLLNode<IDInfo>* pUnitNode = getGroup()->headUnitNode();
		CvUnit* pLoopUnit = NULL;

		while( pUnitNode != NULL )
		{
			pLoopUnit = ::getUnit(pUnitNode->m_data);
			bNoDefensiveBonus = (bNoDefensiveBonus && pLoopUnit->noDefensiveBonus());

			pUnitNode = getGroup()->nextUnitNode(pUnitNode);
		}
	}

	CvPlot* pBestPlot = NULL;
	int iBestValue = 0;
	for (int iX = -iRange; iX <= iRange; iX++)
	{
		for (int iY = -iRange; iY <= iRange; iY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iX, iY);
			if (pLoopPlot != NULL)
			{
				if (isEnemy(pLoopPlot->getTeam()) && !(pLoopPlot->isVisibleEnemyUnit(this)))
				{
					CvCity* pWorkingCity = pLoopPlot->getWorkingCity();
					if ((pWorkingCity != NULL) && (pWorkingCity->getTeam() == pLoopPlot->getTeam()))
					{
						int iValue = (bDefensive ? pLoopPlot->defenseModifier(getTeam(), false) : -15);
						if (pLoopPlot->getBonusType(getTeam()) != NO_BONUS)
						{
							iValue += GET_PLAYER(pLoopPlot->getOwnerINLINE()).AI_bonusVal(pLoopPlot->getBonusType(), 0);
						}

						iValue += pLoopPlot->getYield(YIELD_PRODUCTION) * 10;
						iValue += pLoopPlot->getYield(YIELD_FOOD) * 10;
						iValue += pLoopPlot->getYield(YIELD_COMMERCE) * 5;

						if (bNoDefensiveBonus)
						{
							iValue *= std::max(0, ((baseCombatStr() * 120) - GC.getGame().getBestLandUnitCombat()));
						}
						else
						{
							iValue *= pLoopPlot->defenseModifier(getTeam(), false);
						}

						if (iValue > 0)
						{
							if( !bDefensive )
							{
								iValue *= 10;
								iValue /= std::max(1, (pLoopPlot->getNumDefenders(getOwnerINLINE()) + ((pLoopPlot == plot()) ? 0 : getGroup()->getNumUnits())));
							}

							if (generatePath(pLoopPlot))
							{
								pBestPlot = getPathEndTurnPlot();
								iBestValue = iValue;
							}
						}
					}
				}
			}
		}
	}
	if (pBestPlot != NULL)
	{
		if (atPlot(pBestPlot))
		{
			if(canPillage(plot())) getGroup()->pushMission(MISSION_PILLAGE);
			getGroup()->pushMission(MISSION_SKIP);
			return true;
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX(), pBestPlot->getY());
			return true;
		}
	}

	return false;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

bool CvUnitAI::AI_solveBlockageProblem(CvPlot* pDestPlot, bool bDeclareWar)
{
	PROFILE_FUNC();

	FAssert(pDestPlot != NULL);


	if (pDestPlot != NULL)
	{
		FAStarNode* pStepNode;

		CvPlot* pSourcePlot = plot();

		if (gDLL->getFAStarIFace()->GeneratePath(&GC.getStepFinder(), pSourcePlot->getX_INLINE(), pSourcePlot->getY_INLINE(), pDestPlot->getX_INLINE(), pDestPlot->getY_INLINE(), false, 0, true))
		{
			pStepNode = gDLL->getFAStarIFace()->GetLastNode(&GC.getStepFinder());

			while (pStepNode != NULL)
			{
				CvPlot* pStepPlot = GC.getMapINLINE().plotSorenINLINE(pStepNode->m_iX, pStepNode->m_iY);
				if (canMoveOrAttackInto(pStepPlot) && generatePath(pStepPlot, 0, true))
				{
					if (bDeclareWar && pStepNode->m_pPrev != NULL)
					{
						CvPlot* pPlot = GC.getMapINLINE().plotSorenINLINE(pStepNode->m_pPrev->m_iX, pStepNode->m_pPrev->m_iY);
						if (pPlot->getTeam() != NO_TEAM)
						{
							if (!canMoveInto(pPlot, true, true))
							{
								if (!isPotentialEnemy(pPlot->getTeam(), pPlot))
								{
									CvTeamAI& kTeam = GET_TEAM(getTeam());
									if (kTeam.canDeclareWar(pPlot->getTeam()))
									{
										WarPlanTypes eWarPlan = WARPLAN_LIMITED;
										WarPlanTypes eExistingWarPlan = kTeam.AI_getWarPlan(pDestPlot->getTeam());
										if (eExistingWarPlan != NO_WARPLAN)
										{
											if ((eExistingWarPlan == WARPLAN_TOTAL) || (eExistingWarPlan == WARPLAN_PREPARING_TOTAL))
											{
												eWarPlan = WARPLAN_TOTAL;
											}

											if (!kTeam.isAtWar(pDestPlot->getTeam()))
											{
												kTeam.AI_setWarPlan(pDestPlot->getTeam(), NO_WARPLAN);
											}
										}
										kTeam.AI_setWarPlan(pPlot->getTeam(), eWarPlan, true);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/29/10                                jdog5000      */
/*                                                                                              */
/* War tactics AI                                                                               */
/************************************************************************************************/
/* original bts code
										return (AI_targetCity());
*/
										return (AI_goToTargetCity(MOVE_AVOID_ENEMY_WEIGHT_2));
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

									}
								}
							}
						}
					}
					if (pStepPlot->isVisibleEnemyUnit(this))
					{
						FAssert(canAttack());
						CvPlot* pBestPlot = pStepPlot;
						//To prevent puppeteering attempt to barge through
						//if quite close
						if (getPathLastNode()->m_iData2 > 3)
						{
							pBestPlot = getPathEndTurnPlot();
						}

						FAssert(!atPlot(pBestPlot));
						getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_DIRECT_ATTACK);
						return true;
					}
				}
				pStepNode = pStepNode->m_pParent;
			}
		}
	}

	return false;
}

int CvUnitAI::AI_calculatePlotWorkersNeeded(CvPlot* pPlot, BuildTypes eBuild)
{
	int iBuildTime = pPlot->getBuildTime(eBuild) - pPlot->getBuildProgress(eBuild);
/*************************************************************************************************/
/**	JohnHenry								04/04/09								Xienwolf	**/
/**																								**/
/**		Require to know the Build Order and Features present to calculate the Workrate properly	**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	int iWorkRate = workRate(true);
/**								----  End Original Code  ----									**/
	int iWorkRate = workRate(true, eBuild, pPlot->getFeatureType());
/*************************************************************************************************/
/**	JohnHenry								END													**/
/*************************************************************************************************/

	if (iWorkRate <= 0)
	{
	//	FAssert(false);
		return 1;
	}
	int iTurns = iBuildTime / iWorkRate;

	if (iBuildTime > (iTurns * iWorkRate))
	{
		iTurns++;
	}

	int iNeeded = std::max(1, (iTurns + 2) / 3);

	/********************************************************************************/
	/* 	BETTER_BTS_AI_MOD						7/31/08				jdog5000	*/
	/* 																			*/
	/* 	Bugfix																	*/
	/********************************************************************************/
	//if (pPlot->getBonusType() != NO_BONUS)
	if (pPlot->getNonObsoleteBonusType(getTeam()) != NO_BONUS)
	{
		iNeeded *= 2;
	}
	/********************************************************************************/
	/* 	BETTER_BTS_AI_MOD						END								*/
	/********************************************************************************/

	return iNeeded;

}

bool CvUnitAI::AI_canGroupWithAIType(UnitAITypes eUnitAI) const
{
	if (eUnitAI != AI_getUnitAIType())
	{
		switch (eUnitAI)
		{
		case (UNITAI_ATTACK_CITY):
			if (plot()->isCity() && (GC.getGame().getGameTurn() - plot()->getPlotCity()->getGameTurnAcquired()) <= 1)
			{
				return false;
			}
			break;
		default:
			break;
		}
	}

	return true;
}



bool CvUnitAI::AI_allowGroup(const CvUnit* pUnit, UnitAITypes eUnitAI) const
{
	CvSelectionGroup* pGroup = pUnit->getGroup();
	CvPlot* pPlot = pUnit->plot();

	if (pUnit == this)
	{
		return false;
	}

	if (!pUnit->isGroupHead())
	{
		return false;
	}

	if (pGroup == getGroup())
	{
		return false;
	}

	if (pUnit->isCargo())
	{
		return false;
	}

	if (pUnit->AI_getUnitAIType() != eUnitAI)
	{
		return false;
	}

//FfH: Added by Kael 08/18/2008
	if ((plot() != pPlot) && (pPlot->isVisibleEnemyUnit((PlayerTypes)getOwnerINLINE())))
	{
		return false;
	}
//FfH: End Add

	switch (pGroup->AI_getMissionAIType())
	{
	case MISSIONAI_GUARD_CITY:
		// do not join groups that are guarding cities
		// intentional fallthrough
	case MISSIONAI_LOAD_SETTLER:
	case MISSIONAI_LOAD_ASSAULT:
	case MISSIONAI_LOAD_SPECIAL:
		// do not join groups that are loading into transports (we might not fit and get stuck in loop forever)
		return false;
		break;
	default:
		break;
	}

	if (pGroup->getActivityType() == ACTIVITY_HEAL)
	{
		// do not attempt to join groups which are healing this turn
		// (healing is cleared every turn for automated groups, so we know we pushed a heal this turn)
		return false;
	}

	if (!canJoinGroup(pPlot, pGroup))
	{
		return false;
	}

	if (eUnitAI == UNITAI_SETTLE)
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* Unit AI, Efficiency                                                                          */
/************************************************************************************************/
		//if (GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(pPlot, 3) > 0)
		if (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(pPlot, 3))
		{
			return false;
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	}
	else if (eUnitAI == UNITAI_ASSAULT_SEA)
	{
		if (!pGroup->hasCargo())
		{
			return false;
		}
	}

	if ((getGroup()->getHeadUnitAI() == UNITAI_CITY_DEFENSE))
	{
		if (plot()->isCity() && (plot()->getTeam() == getTeam()) && plot()->getBestDefender(getOwnerINLINE())->getGroup() == getGroup())
		{
			return false;
		}
	}

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		CvPlot* pTargetPlot = pGroup->AI_getMissionAIPlot();

		if (pTargetPlot != NULL)
		{
			if (pTargetPlot->isOwned())
			{
				if (isPotentialEnemy(pTargetPlot->getTeam(), pTargetPlot))
				{
					//Do not join groups which have debarked on an offensive mission
					return false;
				}
			}
		}
	}

/*************************************************************************************************/
/**	CandyMan								04/04/09								Xienwolf	**/
/**																								**/
/**							Allows Multiple Invisible types on a Unit							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	if (pUnit->getInvisibleType() != NO_INVISIBLE)
	{
		if (getInvisibleType() == NO_INVISIBLE)
		{
			return false;
		}
	}
/**								----  End Original Code  ----									**/
	int iI, iJ;
	bool bValid;
	for (iI = 0; iI < pUnit->getNumInvisibleTypes(); iI++)
	{
		bValid = false;
		for (iJ = 0; iJ < getNumInvisibleTypes(); iJ++)
		{
			if (getInvisibleType(iJ) == pUnit->getInvisibleType(iI))
			{
				bValid = true;
			}
		}
		if (!bValid)
		{
			return false;
		}
	}
/*************************************************************************************************/
/**	CandyMan								END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	Improved AI								10/02/12									Snarko	**/
/**																								**/
/**					Don't allow grouping with immobile, or leashed, units						**/
/*************************************************************************************************/
	if ((getImmobileTimer() > 0) || (baseMoves() == 0) || (isLeashed() && getLeashUnit() == NULL)) //Held automatically make immobiletimer > 0
	{
		return false;
	}
	if ((pUnit->getImmobileTimer() > 0) || (pUnit->baseMoves() == 0) || (pUnit->isLeashed() && pUnit->getLeashUnit() == NULL))
	{
		return false;
	}
/*************************************************************************************************/
/**	Improved AI END																				**/
/*************************************************************************************************/

	return true;
}


void CvUnitAI::read(FDataStreamBase* pStream)
{
	CvUnit::read(pStream);

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	pStream->Read(&m_iBirthmark);

	pStream->Read((int*)&m_eUnitAIType);
	pStream->Read(&m_iAutomatedAbortTurn);
}


void CvUnitAI::write(FDataStreamBase* pStream)
{
	CvUnit::write(pStream);

	uint uiFlag=0;
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(m_iBirthmark);

	pStream->Write(m_eUnitAIType);
	pStream->Write(m_iAutomatedAbortTurn);
}

// Private Functions...

//FfH: Added by Kael 09/19/2007
void CvUnitAI::AI_summonAttackMove()
{
	PROFILE_FUNC();

	if (AI_anyAttack(baseMoves(), 5))
	{
		return;
	}

/*************************************************************************************************/
/**	Tweak							04/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 101))
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	if (AI_patrol())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}
//FfH: End Add


// Teaching AI how/when to claim forts : Snarko MISSION_CLAIM_FORT 15/06/10
bool CvUnitAI::AI_claimFort(int iRange, int iOddsThreshold)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;

	int iSearchRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iDX, iDY;
	ImprovementTypes eRevealedImprovement;
	FAssert(canMove());

	iSearchRange = AI_searchRange(iRange);

	iBestValue = 0;
	pBestPlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot	= plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (AI_plotValid(pLoopPlot))
				{
					eRevealedImprovement = pLoopPlot->getRevealedImprovementType(getTeam(), false);
					if ((eRevealedImprovement != NO_IMPROVEMENT) && GC.getImprovementInfo(eRevealedImprovement).isFort())
					{
						if (AI_canClaimFort(pLoopPlot))
						{
							if (generatePath(pLoopPlot, 0, true, &iPathTurns, iRange) && (iPathTurns <= iRange))
							{
								iValue = getGroup()->AI_attackOdds(pLoopPlot, true);

								if (iValue >= AI_finalOddsThreshold(pLoopPlot, iOddsThreshold))
								{
									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = getPathEndTurnPlot();
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		if (atPlot(pBestPlot))
		{
			//FAssert(canClaimFort(pBestPlot));
			getGroup()->pushMission(MISSION_CLAIM_FORT);
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
			return true;
		}
	}

	return false;
}

bool CvUnitAI::AI_canClaimFort(CvPlot* pPlot)
{
	if (pPlot == NULL)
	{
		pPlot = plot();
	}

	// Factor of 3 safety; ai shouldn't be out of gold when trekking out to claim a fort (unitai: will always claim if on tile, though)
	if (!isBarbarian() && (GET_PLAYER(getOwnerINLINE())).getGold() < 3 * GET_PLAYER(getOwnerINLINE()).getClaimFortCost())
		return false;

	if (NO_IMPROVEMENT == pPlot->getRevealedImprovementType(getTeam(), false) || !GC.getImprovementInfo(pPlot->getRevealedImprovementType(getTeam(), false)).isFort())
		return false;

	// Barbs can't claim unowned naval forts.
	else if (isBarbarian() && pPlot->isWater() && !pPlot->isOwned())
		return false;

	// If we don't think there's an owner, go for it
	PlayerTypes eRevealedOwner = pPlot->getRevealedOwner(getTeam(), false);
	if (eRevealedOwner == NO_PLAYER)
		return true;

	// If we're not at war with owner, can't claim
	if (eRevealedOwner != getOwnerINLINE() && !GET_TEAM(getTeam()).isAtWar(pPlot->getRevealedTeam(getTeam(), false)))
		return false;

	// Check if we need to reappoint a commander to our fort. Can be expensive, but don't see how else to avoid.
	if (pPlot->getImprovementOwner() == getOwnerINLINE())
	{
		CvUnit* pLoopUnit;
		CLLNode<IDInfo>* pUnitNode;
		pUnitNode = pPlot->headUnitNode();
		while (pUnitNode != NULL)
		{
			pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = pPlot->nextUnitNode(pUnitNode);
			if (pLoopUnit->getUnitClassType() == GC.getDefineINT("FORT_COMMANDER_UNITCLASS"))
			{
				return false;
			}
		}
		return true;
	}

	return false;
}


/*************************************************************************************************/
/**	Improved AI							16/06/10										Snarko	**/
/**																								**/
/**								Making the AI build forts										**/
/*************************************************************************************************/
// Returns true if a mission was pushed...
bool CvUnitAI::AI_fort()
{
	PROFILE_FUNC();

	int iPathTurns=0;
	int iValue=-1;
	int iBestFortValue = MAX_INT;
	CvPlot* pBestPlot = NULL;
	CvPlot* pBestFortPlot = NULL;
	int iBestBuildValue=-1;
	BuildTypes eBestBuild=NO_BUILD;
	BuildTypes eBuild=NO_BUILD;

	for (int iI = 0; iI < GET_PLAYER(getOwnerINLINE()).AI_getNumCitySites(); iI++)
	{
		CvPlot* pCitySitePlot = GET_PLAYER(getOwnerINLINE()).AI_getCitySite(iI);
		if (pCitySitePlot->getArea() == getArea())
		{
			if (AI_canFort(pCitySitePlot))
			{
				if (!(pCitySitePlot->isVisibleEnemyUnit(this)))
				{
					if (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pCitySitePlot, MISSIONAI_FOUND, getGroup()) == 0 && GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pCitySitePlot, MISSIONAI_BUILD, getGroup()) == 0)
					{
						if (getGroup()->canDefend() || GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pCitySitePlot, MISSIONAI_GUARD_CITY) > 0)
						{
							iBestBuildValue = 0;
							eBestBuild = NO_BUILD;

							for (int iJ = 0; iJ < GC.getNumBuildInfos(); iJ++)
							{
								eBuild = ((BuildTypes)iJ);
								FAssertMsg(eBuild < GC.getNumBuildInfos(), "Invalid Build");
								if (GC.getBuildInfo(eBuild).getImprovement() != NO_IMPROVEMENT)
								{
									if (GC.getImprovementInfo((ImprovementTypes)(GC.getBuildInfo(eBuild).getImprovement())).isFort())
									{
										if (canBuild(pCitySitePlot, eBuild))
										{
											iValue = GC.getImprovementInfo((ImprovementTypes)(GC.getBuildInfo(eBuild).getImprovement())).getDefenseModifier();
											if (GC.getImprovementInfo((ImprovementTypes)(GC.getBuildInfo(eBuild).getImprovement())).getImprovementUpgrade() == NO_IMPROVEMENT)
											{
												iValue *= 3 / 2; //Bonus if it's the final upgrade
											}

											if (iValue > iBestBuildValue)
											{
												iBestBuildValue = iValue;
												eBestBuild = eBuild;
											}
										}
									}
								}
							}
							if (eBestBuild != NO_BUILD)
							{
								if (generatePath(pCitySitePlot, MOVE_SAFE_TERRITORY, true, &iPathTurns))
								{
									iValue = pCitySitePlot->getFoundValue(getOwnerINLINE());
									iValue *= (iPathTurns + 1);
									if (iValue < iBestFortValue)
									{
										iBestFortValue = iValue;
										pBestPlot = getPathEndTurnPlot();
										pBestFortPlot = pCitySitePlot;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ((pBestPlot != NULL) && (pBestFortPlot != NULL))
	{
		FAssertMsg(eBestBuild != NO_BUILD, "BestBuild is not assigned a valid value");
		FAssertMsg(eBestBuild < GC.getNumBuildInfos(), "BestBuild is assigned a corrupt value");
		if (!atPlot(pBestFortPlot))
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_SAFE_TERRITORY, false, false, MISSIONAI_BUILD, pBestFortPlot);
		}
		else
		{
			FAssert(!atPlot(pBestPlot));
			getGroup()->pushMission(MISSION_BUILD, eBestBuild, -1, 0, false, false, MISSIONAI_BUILD, pBestFortPlot);
		}
		return true;
	}

	return false;
}


bool CvUnitAI::AI_canFort(CvPlot* pPlot)
{
	int iBestFoundValue;
	int iCitySites = GET_PLAYER(getOwnerINLINE()).AI_getNumAreaCitySites(getArea(), iBestFoundValue);

	if (pPlot->getOwner() == getOwnerINLINE())
	{
		return false; //We own it, why fort it?
	}

	//Ignore this if we cannot build settlers!
	if (iCitySites <= 2) //Don't fort if we're running out of city sites
	{
		return false;
	}

	if (pPlot->findHighestCultureControlPlayer() == getOwnerINLINE())
	{
		return false; //We already have culture control, no need to fort it
	}

	if (pPlot->getFoundValue(getOwnerINLINE()) == iBestFoundValue)
	{
		return false; //Don't take the best city site
	}

	return true;
}

/*************************************************************************************************/
/**	Improved AI								END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	MISSION_EXPLORE_LAIR				20/06/10										Snarko	**/
/**																								**/
/**							Moving explore lair results into SDK								**/
/*************************************************************************************************/
bool CvUnitAI::AI_exploreLair(int iRange, int iOddsThreshold)
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;

	int iSearchRange;
	int iPathTurns;
	int iValue;
	int iBestValue;
	int iDX, iDY;
	ImprovementTypes eRevealedImprovement;
	FAssert(canMove());

	iSearchRange = AI_searchRange(iRange);

	iBestValue = 0;
	pBestPlot = NULL;

	for (iDX = -(iSearchRange); iDX <= iSearchRange; iDX++)
	{
		for (iDY = -(iSearchRange); iDY <= iSearchRange; iDY++)
		{
			pLoopPlot	= plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (AI_plotValid(pLoopPlot))
				{
					eRevealedImprovement = pLoopPlot->getRevealedImprovementType(getTeam(), false);
					if ((eRevealedImprovement != NO_IMPROVEMENT) && GC.getImprovementInfo(eRevealedImprovement).isExplorable())
					{
						if (AI_canExploreLair(pLoopPlot))
						{
							if (generatePath(pLoopPlot, 0, true, &iPathTurns, iRange) && (iPathTurns <= iRange))
							{
								iValue = getGroup()->AI_attackOdds(pLoopPlot, true);
								if (GC.getImprovementInfo(eRevealedImprovement).getSpawnUnitType() != -1)
									iValue *= 10;

								if (iValue >= AI_finalOddsThreshold(pLoopPlot, iOddsThreshold))
								{
									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										pBestPlot = getPathEndTurnPlot();
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		if (atPlot(pBestPlot))
		{
			FAssert(AI_canExploreLair(pBestPlot));
			getGroup()->pushMission(MISSION_EXPLORE_LAIR);
			return true;
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
			return true;
		}
	}

	return false;
}

bool CvUnitAI::AI_canExploreLair(CvPlot* pPlot)
{
	// This does true check, not 'visible' check
	// if (!canExploreLair(pPlot))
	// {
	// 	return false;
	// }

	if (pPlot == NULL)
		return false;

	if (pPlot->getRevealedImprovementType(getTeam(), false) == NO_IMPROVEMENT
		|| isBarbarian()
		|| !canFight()
		|| getUnitCombatType() == GC.getInfoTypeForString("UNITCOMBAT_SIEGE")
		|| getSpecialUnitType() == GC.getDefineINT("SPECIALUNIT_SPELL")
		|| getSpecialUnitType() == GC.getDefineINT("SPECIALUNIT_BIRD")
		|| isOnlyDefensive())
		return false;

	if (!GC.getImprovementInfo(pPlot->getRevealedImprovementType(getTeam(), false)).isExplorable()
	|| !(pPlot->getExploreNextTurn() <= GC.getGame().getGameTurn()))
		return false;

	bool bGoodyClass = false;
	for (int i = 0; i < GC.getNumGoodyClassTypes(); i++)
	{
		if (GC.getImprovementInfo(pPlot->getRevealedImprovementType(getTeam(), false)).isGoodyClassType(i))
		{
			bGoodyClass = true;
			break;
		}
	}
	if (!bGoodyClass)
		return false;

	if (GET_PLAYER(getOwnerINLINE()).getNumCities() == 0)
		return false;

	// The higher tier of lair, the higher tier of unit required. Modulate effective unit tier with noBadExplore; accounts for hypothetical negative luck, if added.
	if (!(getUnitInfo().getTier() * 25 + getNoBadExplore() + getNoBadExploreImprovement(pPlot->getRevealedImprovementType(getTeam(), false))
			>= 20 * GC.getImprovementInfo(pPlot->getRevealedImprovementType(getTeam(), false)).getLairTier()))
		return false;

	// Need minimum 2, maximum 5 defenders on nearest city to explore a lair
	CvCity* pNearestCity = GC.getMap().findCity(getX_INLINE(), getY_INLINE(), NO_PLAYER, getTeam());
	if (pNearestCity == NULL)
		return true; //No city on this area
	int iCityDist = plotDistance(getX_INLINE(), getY_INLINE(), pNearestCity->getX(), pNearestCity->getY());
	int iNumDefenders = pNearestCity->plot()->getNumDefenders(pNearestCity->getOwner()); //XXX take into account other players units? Blaze: Shouldn't be necessary given the #s involved
	if (range(18/std::max(1,iCityDist), 2, 5) > iNumDefenders)
		return false;

	return true;

}
/*************************************************************************************************/
/**	MISSION_EXPLORE_LAIR					END													**/
/*************************************************************************************************/
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/21/10                                jdog5000      */
/*                                                                                              */
/* Lead From Behind                                                                             */
/************************************************************************************************/
// From Lead From Behind by UncutDragon

void CvUnitAI::LFBgetBetterAttacker(CvUnit** ppAttacker, const CvPlot* pPlot, bool bPotentialEnemy, int& iAIAttackOdds, int& iAttackerValue) const
{
	CvUnit* pThis = (CvUnit*)this;
	CvUnit* pAttacker = (*ppAttacker);
	CvUnit* pDefender;
	int iOdds;
	int iValue;
	int iAIOdds;

	pDefender = pPlot->getBestDefender(NO_PLAYER, getOwnerINLINE(), this, !bPotentialEnemy, bPotentialEnemy);
	iValue = LFBgetAttackerRank(pDefender, iOdds);

	// Combat odds are out of 1000, but the AI routines need odds out of 100, and when called from AI_getBestGroupAttacker
	// we return this value. Note that I'm not entirely sure if/how that return value is actually used ... but just in case I
	// want to make sure I'm returning something consistent with what was there before
	iAIOdds = (iOdds + 5) / 10;
	iAIOdds += GET_PLAYER(getOwnerINLINE()).AI_getAttackOddsChange();
	iAIOdds = std::max(1, std::min(iAIOdds, 99));

	if (collateralDamage() > 0)
	{
		int iPossibleTargets = std::min((pPlot->getNumVisibleEnemyDefenders(pThis) - 1), collateralDamageMaxUnits());

		if (iPossibleTargets > 0)
		{
			iValue *= (100 + ((collateralDamage() * iPossibleTargets) / 5));
			iValue /= 100;
		}
	}

	// Nothing to compare against - we're obviously better
	if (!pAttacker)
	{
		(*ppAttacker) = pThis;
		iAIAttackOdds = iAIOdds;
		iAttackerValue = iValue;
		return;
	}

	// Compare our adjusted value with the current best
	if (iValue >= iAttackerValue)
	{
		(*ppAttacker) = pThis;
		iAIAttackOdds = iAIOdds;
		iAttackerValue = iValue;
	}
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
/*************************************************************************************************/
/**	MISSION_INQUISITION						13/01/12									Snarko	**/
/**																								**/
/**			Adding a mission for inquisition and teaching the AI proper use						**/
/*************************************************************************************************/
bool CvUnitAI::AI_inquisitionMove(int iMinThreshold)
{
	int iThreshold = 0;

	//Don't let human (automated) units do inquisition.
	if (isHuman())
		return false;

	//Culture and inquisition doesn't mix well
	if (GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE, 3))
		return false;
	else if (GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE, 2))
		iThreshold -= 5;

	if (GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_RELIGION, 3))
		iThreshold += 40;
	else if (GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_RELIGION, 2))
		iThreshold += 20;
	else if (GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_RELIGION, 1))
		iThreshold += 10;

	if (GET_PLAYER(getOwnerINLINE()).getNonStateReligionHappiness() < 0)
		iThreshold += 10;

	if (iThreshold <= iMinThreshold)
		return false;

	if (plot()->getPlotCity() != NULL)
	{
		if (inquisition())
			return true;
	}

	CvPlot* pLoopPlot = NULL;
	int iPathTurns = 0;
	int iBestValue = MAX_INT;
	CvPlot* pBestPlot = NULL;
	int iLoop;
	//XXX Make the AI value cities depending on number of religions?

	for (CvCity* pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
	{
		pLoopPlot = pLoopCity->plot();
		if (canInquisition(pLoopPlot))
		{
			if (generatePath(pLoopPlot, MOVE_NO_ENEMY_TERRITORY, true, &iPathTurns))
			{
				if (iPathTurns < iBestValue)
				{
					iBestValue = iPathTurns;
					pBestPlot = pLoopPlot;
				}
			}
		}
	}

	if (pBestPlot == NULL)
	{
		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive() && !(::isPotentialEnemy(getTeam(), GET_PLAYER((PlayerTypes)iI).getTeam())))
			{
				for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
				{
					pLoopPlot = pLoopCity->plot();
					if (AI_plotValid(pLoopPlot) && (pLoopCity->area() == area() || canMoveImpassable() || isWaterWalking()))
					{
						if (canInquisition(pLoopPlot))
						{
							if (generatePath(pLoopPlot, MOVE_NO_ENEMY_TERRITORY, true, &iPathTurns))
							{
								if (iPathTurns < iBestValue)
								{
									iBestValue = iPathTurns;
									pBestPlot = pLoopPlot;
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(pBestPlot != plot()) //Handled before
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE()); //XXX check for other units doing the same thing?
		return true;
	}

	return false;
}
/*************************************************************************************************/
/**	MISSION_INQUISITION END																		**/
/*************************************************************************************************/
void CvUnitAI::AI_upgrademanaMove()
{

	
	if (GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_TOWERMASTERY, 1))
	{
		if (GET_PLAYER(getOwnerINLINE()).countOwnedBonuses((BonusTypes)GC.getDefineINT("BONUSCLASS_MANA_RAW")) == 1)
		{
			if (GET_PLAYER(getOwnerINLINE()).countOwnedBonuses((BonusTypes)GC.getInfoTypeForString("BONUS_MANA_METAMAGIC")) == 0)
			{
				if (plot()->isCity())
				{
					getGroup()->pushMission(MISSION_FORTIFY);
					return;
				}
				else
				{
					if (AI_retreatToCity())
						return;
				}

			}
		}
	}

	bool bDanger = (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 3));

	if (bDanger)
	{
		if (AI_retreatToCity())
		{
			return;
		}
	}

	if (AI_heal())
	{
		return;
	}

	int iValue = 0;
	int iBestValue = 0;
	int iPathTurns;
	bool bBonusRawMana = false;
	bool bBonusMana = false;
	//int iRange = 15;

	CvPlot* pBestPlot = NULL;
	BuildTypes eBuild = NO_BUILD;
	BuildTypes eBestBuild = NO_BUILD;
	CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());

	//ToDo - keep one spare raw mana node if we have more than X mana and are pursuing Tower victory (for metamagic node)
//	bool bReadytoBuild = false;
	// loop through plots in range
	// ToDo - make this a map search?
	/*
	for (int iX = -iRange; iX <= iRange; iX++)
	{
		for (int iY = -iRange; iY <= iRange; iY++)
		{
		*/
	{
		for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
		{
			CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

			//CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iX, iY);
			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->getOwner() == getOwner())
				{
					if (!pLoopPlot->isVisibleEnemyDefender(this) && !kPlayer.AI_getAnyPlotDanger(pLoopPlot))
					{
						if (pLoopPlot->getBonusType() != NO_BONUS)
						{
							bBonusRawMana = false;
							bBonusMana = false;
							// HARDCODE - should have some sort of global variable to let the AI know about mana - XML tag?
							if (GC.getBonusInfo(pLoopPlot->getBonusType()).getBonusClassType() == GC.getDefineINT("BONUSCLASS_MANA"))
								//if (GC.getBonusInfo(pLoopPlot->getBonusType()).isMana())
							{
								// check to make sure we don't check existing nodes
								if (pLoopPlot->getImprovementType() == NO_IMPROVEMENT)
								{
									bBonusMana = true;
								}
								else
								{
									if (GC.getImprovementInfo(pLoopPlot->getImprovementType()).getBonusConvert() == NO_BONUS)
									{
										bBonusMana = true;
									}
								}
							}

							// HARDCODE
							if (GC.getBonusInfo(pLoopPlot->getBonusType()).getBonusClassType() == GC.getDefineINT("BONUSCLASS_MANA_RAW"))
							{
								bBonusRawMana = true;
							}

							if (bBonusMana || bBonusRawMana)
							{
								// found mana, now check path
								if (generatePath(pLoopPlot, 0, true, &iPathTurns))
								{
									// we can reach the mana, now make sure we have a build
									bool bFoundBuild = false;
									for (int iJ = 0; iJ < GC.getNumBuildInfos(); iJ++)
									{
										eBuild = ((BuildTypes)iJ);
										if (canBuild(pLoopPlot, eBuild))
										{
											bFoundBuild = true;
											break;
										}
									}

									if (bFoundBuild)
									{
										iValue = 100 / (iPathTurns + 1);

										if (iValue > iBestValue)
										{
											pBestPlot = pLoopPlot;
											iBestValue = iValue;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBestPlot != NULL)
	{
		if (atPlot(pBestPlot))
		{
			if (isHasCasted()) // casting blocks mana builds
			{
				getGroup()->pushMission(MISSION_FORTIFY);
				return;
			}
			iBestValue = 0;
			eBestBuild = NO_BUILD;

			// loop through various builds and find the best one
			for (int iJ = 0; iJ < GC.getNumBuildInfos(); iJ++)
			{
				eBuild = ((BuildTypes)iJ);
				if (canBuild(plot(), eBuild))
				{
					// we have to first get the improvement, then find out what mana this node will be converted to
					ImprovementTypes eImprovement = (ImprovementTypes)GC.getBuildInfo(eBuild).getImprovement();
					BonusTypes eNewBonus = (BonusTypes)GC.getImprovementInfo(eImprovement).getBonusConvert();

					iValue = kPlayer.AI_bonusVal(eNewBonus) + 1;

					


					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						eBestBuild = eBuild;
					}
				}
			}

			if (eBestBuild != NO_BUILD)
			{

				getGroup()->pushMission(MISSION_BUILD, eBestBuild, -1, 0, false, false, MISSIONAI_BUILD, plot());
				return;
			}
		}
		else
		{

			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_AVOID_ENEMY_WEIGHT_2);
			return;
		}
	}

	CyUnit* pyUnit1 = new CyUnit(this);
	CyArgsList argsList1;
	argsList1.add(gDLL->getPythonIFace()->makePythonObject(pyUnit1));	// pass in unit class
	long lResult = 0;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "AI_Mage_UPGRADE_MANA", argsList1.makeFunctionArgs(), &lResult);
	delete pyUnit1;	// python fxn must not hold on to this pointer

	if (lResult == 1)
	{
		getGroup()->pushMission(MISSION_SKIP);
		return;
	}

	if (AI_moveIntoCity(5))
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}


void CvUnitAI::AI_ConquestMove()
{
	CvSelectionGroup* pLoopSelectionGroup;
	CvUnit* pBestUnit;
	CvPlot* pLoopPlot;
	//CvPlot* pBestPlot;
	int iPathTurns;
	//int iDX, iDY;
	int iLoop;
	int iValue;
	int iBestValue;
	//int iSearchRange;
//    bool bFollow=false;
	CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());


	AreaAITypes eAreaAIType = area()->getAreaAIType(getTeam());
	//bool bLandWar = !isBarbarian() && ((eAreaAIType == AREAAI_OFFENSIVE) || (eAreaAIType == AREAAI_DEFENSIVE) || (eAreaAIType == AREAAI_MASSING));
	bool bLandWar = !isBarbarian() && kPlayer.AI_isLandWar(area());
	bool bAssault = !isBarbarian() && ((eAreaAIType == AREAAI_ASSAULT) || (eAreaAIType == AREAAI_ASSAULT_ASSIST) || (eAreaAIType == AREAAI_ASSAULT_MASSING));

	bool bTurtle = kPlayer.AI_isDoStrategy(AI_STRATEGY_TURTLE);
	bool bAlert1 = kPlayer.AI_isDoStrategy(AI_STRATEGY_ALERT1);
	bool bIgnoreFaster = false;

	

	if (kPlayer.AI_isDoStrategy(AI_STRATEGY_LAND_BLITZ))
	{
		if (!bAssault && area()->getCitiesPerPlayer(getOwnerINLINE()) > 0)
		{
			bIgnoreFaster = true;
		}
	}

	bool bFinancialTrouble = kPlayer.AI_isFinancialTrouble();

	//int bStackSize = getGroup()->getNumUnits();
	//bool bLargeGroup = (bStackSize > (kPlayer.getNumCities() *6));


	bool bHero = false;
	bool bWizard = false;

	switch (AI_getUnitAIType())
	{
	case UNITAI_HERO:
		bHero = true;
		break;
	case UNITAI_WARWIZARD:
		bWizard = true;
		break;
	case UNITAI_RESERVE:
		AI_setUnitAIType(UNITAI_ATTACK_CITY);
		break;
	case UNITAI_ATTACK:
		if ((getLevel() > 4)
			|| (kPlayer.AI_getNumAIUnits(UNITAI_ATTACK) > (kPlayer.getNumCities() * (bLandWar ? 2 : 4)))
			|| (kPlayer.AI_getNumAIUnits(UNITAI_ATTACK_CITY) < kPlayer.getNumCities()))
		{
			AI_setUnitAIType(UNITAI_ATTACK_CITY);
			break;
		}
	default:
		break;
	}


	if (isHiddenNationality())// || isInvisibleFromPromotion())
	{
		if (!bHero && !bWizard)
		{
			joinGroup(NULL);
			AI_setUnitAIType(UNITAI_ATTACK);
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}
	}

	

	bool bInCity = plot()->isCity();

	if (bInCity && plot()->getOwnerINLINE() == getOwnerINLINE())
	{
		if ((getGroup()->getNumUnits() == 1) && (getDamage() > 0))
		{
			if (AI_heal())
			{
				return;
			}
		}

		if (AI_leaveAttack(1, 70, 175))
		{
			return;
		}

		if (bIgnoreFaster)
		{
			// BBAI TODO: split out slow units ... will need to test to make sure this doesn't cause loops
		}

		if (AI_guardCity(false)) // note. this will eject a unit to defend the city rather then using the whole group
		{
			return;
		}

		if (bAssault)
		{
			if (AI_offensiveAirlift())
			{
				return;
			}
		}
	}


	// Opportunistic attacks
	if (getGroup()->getNumUnits() == 1 && !bHero && !bWizard)
	{
		if (AI_anyAttack(1, 80))
		{
			return;
		}
		if (AI_anyAttack(2, 90))
		{
			return;
		}
	}

	// Heroes and Casters should seek larger groups
	if (bHero || bWizard)
	{
		//if (getGroup()->getNumUnits() < ((getLevel() / 2) +1))
		{
			if (AI_pickupEquipment(3))
			{
				return;
			}
			/*
			int iRange = 0;
			if (plot()->getOwner() == getOwner())
			{
				iRange = 5;
			}
			else
			{
				iRange = 10;
			}

			if (GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, MISSIONAI_GROUP) == 0)
			{
				if (AI_groupMergeRange(UNITAI_ATTACK_CITY, iRange, false, true, false))
				{
					return;
				}
			}

			if (AI_moveToStagingCity())
			{
				return;
			}
			*/
		}
	}
	else // Mainly affects Clan and the units they get from their World Spell
	{
		if (getGroup()->isStranded() && (getLevel() < 3) && bFinancialTrouble)
		{
			kill(true);
			return;
		}
	}

	bool bHuntBarbs = false;
	if (area()->getCitiesPerPlayer(ORC_PLAYER) > 0 && !GET_TEAM(getTeam()).isBarbarianAlly())
	{
		if ((eAreaAIType != AREAAI_OFFENSIVE) && (eAreaAIType != AREAAI_DEFENSIVE) && !bAlert1 && !bTurtle)
		{
			bHuntBarbs = true;
		}
	}

	bool bReadyToAttack = false;
	if (!bTurtle)
	{
		bReadyToAttack = ((getGroup()->getNumUnits() >= ((bHuntBarbs) ? 3 : AI_stackOfDoomExtra())));
	}

	if (AI_guardCity(false, false))
	{
		return;
	}

	//ToDo - better incorporation of this section into rest of code
	iValue = 0;
	iBestValue = 0;
	if (plot()->getOwnerINLINE() == getOwnerINLINE() && !isCargo())
	{
		iBestValue = getGroup()->getNumUnits();
		pBestUnit = NULL;

		//if (getGroup()->getNumUnits() == 1)
		{
			if (GET_PLAYER(getOwnerINLINE()).AI_unitTargetMissionAIs(this, MISSIONAI_GROUP) == 0)
			{
				for (pLoopSelectionGroup = kPlayer.firstSelectionGroup(&iLoop); pLoopSelectionGroup != NULL; pLoopSelectionGroup = kPlayer.nextSelectionGroup(&iLoop))
				{
					if (pLoopSelectionGroup->getHeadUnit() != NULL)
					{
						
							if (pLoopSelectionGroup != getGroup() && !pLoopSelectionGroup->getHeadUnit()->isCargo())
							{
								pLoopPlot = pLoopSelectionGroup->getHeadUnit()->plot();
								if (AI_plotValid(pLoopPlot))
								{
									if (!isEnemy(pLoopPlot->getTeam()))
									{
										if (AI_allowGroup(pLoopSelectionGroup->getHeadUnit(), UNITAI_UNKNOWN))
										{
											if (!(pLoopPlot->isVisibleEnemyUnit(this)))
											{
												if ((getGroup()->getNumUnits() == 1) ||
													atPlot(pLoopPlot))
												{
													if (generatePath(pLoopPlot, 0, true, &iPathTurns))
													{
														iValue = 10 * pLoopSelectionGroup->getHeadUnit()->getLevel();
														iValue += pLoopSelectionGroup->getNumUnits();
														if (pLoopSelectionGroup->getHeadUnit()->AI_getUnitAIType() == UNITAI_HERO)
														{
															iValue *= 2;
														}

														//	if (pLoopSelectionGroup->getHeadUnit()->isAvatarOfCivLeader())
														//	{
														//		iValue *= 5;
														//	}

														if (atPlot(pLoopPlot))
														{
															iValue *= 2;
														}
														else
														{
															iValue /= (iPathTurns + 1);
														}

														if (iValue > iBestValue)
														{
															iBestValue = iValue;
															pBestUnit = pLoopSelectionGroup->getHeadUnit();
														}
													}
												}
											}
										}
									}
								}
							}
						
					}
				}
			}
			if (pBestUnit != NULL)
			{
				if (atPlot(pBestUnit->plot()))
				{
					if (getGroup()->getNumUnits() == 1)
					{
						
						joinGroup(pBestUnit->getGroup());
						return;
					}
					else
					{
						
						getGroup()->mergeIntoGroup(pBestUnit->getGroup());
						return;
					}
				}
				else
				{
					
					getGroup()->pushMission(MISSION_MOVE_TO_UNIT, pBestUnit->getOwnerINLINE(), pBestUnit->getID(), 0, false, false, MISSIONAI_GROUP, NULL, pBestUnit);
					return;
				}
			}
		}
		/*
		else
		{
			bool bMerge = false;
			pBestUnit = NULL;
			for(pLoopSelectionGroup = kPlayer.firstSelectionGroup(&iLoop); pLoopSelectionGroup != NULL; pLoopSelectionGroup = kPlayer.nextSelectionGroup(&iLoop))
			{
				if (pLoopSelectionGroup->getHeadUnit() != NULL)
				{
					if (pLoopSelectionGroup->getHeadUnit()->AI_getGroupflag()==GROUPFLAG_CONQUEST)
					{
						if (pLoopSelectionGroup != getGroup() && !pLoopSelectionGroup->getHeadUnit()->isCargo())
						{
							CvPlot* pPlot = pLoopSelectionGroup->getHeadUnit()->plot();
							if (AI_plotValid(pPlot))
							{
								if (!isEnemy(pPlot->getTeam()))
								{
									if (!(pPlot->isVisibleEnemyUnit(this)))
									{
										if (AI_allowGroup(pLoopSelectionGroup->getHeadUnit(), UNITAI_UNKNOWN))
										{
											if (generatePath(pPlot, 0, true, &iPathTurns))
											{
												if (pLoopSelectionGroup->getNumUnits() > (getGroup()->getNumUnits() * 2))
												{
													if (iPathTurns < 5)
													{
														bMerge = true;
														pBestUnit = pLoopSelectionGroup->getHeadUnit();
														break;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}


			if (bMerge)
			{
				if (atPlot(pBestUnit->plot()))
				{
					getGroup()->mergeIntoGroup(pBestUnit->getGroup());
					return;
				}
				else
				{
					getGroup()->pushMission(MISSION_MOVE_TO_UNIT, pBestUnit->getOwnerINLINE(), pBestUnit->getID(), 0, false, false, MISSIONAI_GROUP, NULL, pBestUnit);
					return;
				}
			}
		}
		*/
	}

	bool bAtWar = isEnemy(plot()->getTeam());

	// Look for local threats - mainly meant to deal with early barbarian or HN threats
//	bool bDanger = (kPlayer.AI_getAnyPlotDanger(plot(), 3, false));

	if (bReadyToAttack)
	{
		// Check that stack has units which can capture cities
		bReadyToAttack = false;
		int iCityCaptureCount = 0;

		CLLNode<IDInfo>* pUnitNode = getGroup()->headUnitNode();
		while (pUnitNode != NULL && !bReadyToAttack)
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = getGroup()->nextUnitNode(pUnitNode);

			if (!pLoopUnit->isOnlyDefensive())
			{
				if (!(pLoopUnit->isCannotCapture(true)) && (pLoopUnit->combatLimit() >= 100))
				{
					iCityCaptureCount++;

					if (iCityCaptureCount > 5 || 3 * iCityCaptureCount > getGroup()->getNumUnits())
					{
						bReadyToAttack = true;
					}
				}
			}
		}
	}

	CvCity* pTargetCity = NULL;
	if (bReadyToAttack)
	{
		//if (AI_cityAttack(1, 80, true))
		if (AI_stackAttackCity(1, 160, true))
		{
			
			return;
		}
	}

	if (AI_guardCity(false, false))
	{
		/*
		if( bReadyToAttack && (eAreaAIType != AREAAI_DEFENSIVE))
		{
			CvSelectionGroup* pOldGroup = getGroup();

			pOldGroup->AI_separateNonAI(UNITAI_ATTACK_CITY);
		}
		*/

		return;
	}

	if (AI_groupMergeRange(UNITAI_HERO, 0, false, true, bIgnoreFaster))
	{
		
		return;
	}

	if (AI_groupMergeRange(UNITAI_WARWIZARD, 0, false, true, bIgnoreFaster))
	{
		
		return;
	}

	if (AI_groupMergeRange(UNITAI_ATTACK_CITY, 0, true, true, bIgnoreFaster))
	{
		
		return;
	}

	// BBAI TODO: Find some way of reliably targetting nearby cities with less defense ...
	pTargetCity = AI_pickTargetCity(0, MAX_INT, bHuntBarbs);

	if (pTargetCity != NULL && (getGroup()->getNumUnits() > 1))
	{
		

		int iStepDistToTarget = stepDistance(pTargetCity->getX_INLINE(), pTargetCity->getY_INLINE(), getX_INLINE(), getY_INLINE());
		//int iAttackRatio = std::max(100, GC.getBBAI_ATTACK_CITY_STACK_RATIO());
		int iAttackRatio = 150; //Todo - find better way to come up with an AttackRatio - should vary based on situation
		// loop through units. Add level Subtract if melee or collateral or explodeincombat. Add if mage or UNITAI_HERO

		int iComparePostBombard = 0;
		// AI gets a 1-tile sneak peak to compensate for lack of memory
		if (iStepDistToTarget <= 2 || pTargetCity->isVisible(getTeam(), false))
		{
			iComparePostBombard = getGroup()->AI_compareStacks(pTargetCity->plot(), true, true, true);

			int iDefenseModifier = pTargetCity->getDefenseModifier(true);
			int iBombardTurns = getGroup()->getBombardTurns(pTargetCity);
			iDefenseModifier *= std::max(0, 20 - iBombardTurns);
			iDefenseModifier /= 20;
			iComparePostBombard *= 100 + std::max(0, iDefenseModifier);
			iComparePostBombard /= 100;
		}
		if (iStepDistToTarget <= 2)
		{
			if (iComparePostBombard < iAttackRatio)
			{
				if (AI_groupMergeRange(UNITAI_ATTACK_CITY, 2, true, true, bIgnoreFaster))
				{
					return;
				}

				int iOurOffense = GET_TEAM(getTeam()).AI_getOurPlotStrength(plot(), 1, false, false, true);
				int iEnemyOffense = kPlayer.AI_getEnemyPlotStrength(pTargetCity->plot(), 2, false, false);

				// If in danger, seek defensive ground
				if (4 * iOurOffense < 3 * iEnemyOffense)
				{
					if (AI_choke(1, true))
					{
						return;
					}
				}
			}

			if (iStepDistToTarget == 1)
			{
				// temp hack - Tholal ToDo: need to figure out why the AI is reluctant to attack empty cities
				if (pTargetCity->plot()->getNumDefenders(pTargetCity->getOwner()) == 0)
				{
					getGroup()->pushMission(MISSION_MOVE_TO, pTargetCity->getX_INLINE(), pTargetCity->getY_INLINE(), MOVE_DIRECT_ATTACK);
					return;
				}
				// If next to target city and we would attack after bombarding down defenses,
				// or if defenses have crept up past half
				if ((iComparePostBombard >= iAttackRatio) || (pTargetCity->getDefenseDamage() < ((GC.getMAX_CITY_DEFENSE_DAMAGE() * 1) / 2)))
				{
					if ((iComparePostBombard < std::max(150, GC.getDefineINT("BBAI_SKIP_BOMBARD_MIN_STACK_RATIO"))))
					{

						// Tholal Note: this section wasn't doing much useful
						// Move to good tile to attack from unless we're way more powerful
						/*
						if (plot()->defenseModifier(getTeam(), false) <= 0)
						{
							if( AI_goToTargetCity(0,1,pTargetCity) )
							{
								return;
							}
						}
						*/
					}

					// Bombard may skip if stack is powerful enough
					if (AI_bombardCity())
					{
						return;
					}

					//stack attack
					if (getGroup()->getNumUnits() > 1)
					{
						// BBAI TODO: What is right ratio?
						//if (AI_stackAttackCity(1, iAttackRatio, true))
						//if (AI_cityAttack(1, iAttackRatio, true))
						if (AI_stackAttackCity(1, iAttackRatio, true))
						{
							return;
						}
					}

					// If not strong enough alone, merge if another stack is nearby
					if (AI_groupMergeRange(UNITAI_ATTACK_CITY, 2, true, true, bIgnoreFaster))
					{
						return;
					}

					if (getGroup()->getNumUnits() == 1)
					{
						if (AI_cityAttack(1, 70))
						{
							return;
						}
					}
				}
			}
		}

		if (iComparePostBombard < iAttackRatio && GET_TEAM(getTeam()).isAtWar(pTargetCity->getTeam()))
		{
			// If not strong enough, pillage around target city without exposing ourselves
			if (AI_pillageRange(0))
			{
				return;
			}

			if (AI_anyAttack(1, 80, 0, false))
			{
				return;
			}

			if (AI_heal(30, 1))
			{
				return;
			}

			// Pillage around enemy city
			if (AI_pillageAroundCity(pTargetCity, 11, 3))
			{
				return;
			}

			if (AI_pillageAroundCity(pTargetCity, 0, 5))
			{
				return;
			}

			if (getGroup()->getNumUnits() > pTargetCity->plot()->getNumDefenders(pTargetCity->getOwner()))
			{
				if (AI_choke(1))
				{
					return;
				}
			}

			if (!pTargetCity->isVisible(getTeam(), false))
			{
				if (AI_goToTargetCity(0, 10, pTargetCity))
				{
					return;
				}
			}
		}
		else
		{
			if (AI_goToTargetCity(0, 6, pTargetCity))
			{
				return;
			}
		}

		// Tholal Note - seems that sometimes we have to force the AI to attack their targets
		// Note: This section can cause the AI to declare War before stack is near borders
		if (iComparePostBombard >= 120)
		{
			if (GET_TEAM(getTeam()).isAtWar(pTargetCity->getTeam()) || plot()->isAdjacentPlayer(pTargetCity->getOwner()))
			{
				if (AI_goToTargetCity(MOVE_THROUGH_ENEMY, 10, pTargetCity))
				{
					return;
				}
			}
			else
			{
				if (AI_goToTargetCity(0, 10, pTargetCity))
				{
					return;
				}
			}
		}
	}

	if (AI_groupMergeRange(UNITAI_ATTACK_CITY, 2, true, true, bIgnoreFaster))
	{
		return;
	}


	if (AI_heal(30, 1))
	{
		return;
	}

	// BBAI TODO: Stack v stack combat ... definitely want to do in own territory, but what about enemy territory?
	if (collateralDamage() > 0 && plot()->getOwnerINLINE() == getOwnerINLINE()) // note: this requires that the group leader has collateralDamage; doesnt check whole stack
	{
		if (AI_anyAttack(1, 45, 3, false))
		{
			return;
		}

		if (!bReadyToAttack)
		{
			if (AI_anyAttack(1, 25, 5, false))
			{
				return;
			}
		}
	}

	if (AI_anyAttack(1, 60, 0, false))
	{
		return;
	}

	if (bAtWar && (getGroup()->getNumUnits() <= 2))
	{
		if (AI_pillageRange(3, 11))
		{
			return;
		}

		if (AI_pillageRange(1))
		{
			return;
		}
	}

	bHuntBarbs = false;
	if ((area()->getCitiesPerPlayer(ORC_PLAYER) > 0) && !GET_TEAM(getTeam()).isBarbarianAlly())
	{
		if ((area()->getAreaAIType(getTeam()) != AREAAI_OFFENSIVE) && (area()->getAreaAIType(getTeam()) != AREAAI_DEFENSIVE))
		{
			bHuntBarbs = true;
		}
	}

	if (plot()->getOwnerINLINE() == getOwnerINLINE())
	{

		if (!bLandWar)
		{
			if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4))
			{
				return;
			}
		}

		if (bReadyToAttack)
		{
			// Wait for units about to join our group
			MissionAITypes eMissionAIType = MISSIONAI_GROUP;
			int iJoiners = kPlayer.AI_unitTargetMissionAIs(this, &eMissionAIType, 1, getGroup(), 2);

			if ((iJoiners * 5) > getGroup()->getNumUnits())
			{
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}
		}
		else
		{
			if (eAreaAIType == AREAAI_DEFENSIVE)
			{
				// Use smaller attack city stacks on defenses
				if (AI_guardCity(false, true, 3))
				{
					return;
				}
			}

			if (bTurtle)
			{
				if (AI_guardCity(false, true, 7))
				{
					return;
				}
			}

			int iTargetCount = kPlayer.AI_unitTargetMissionAIs(this, MISSIONAI_GROUP);
			if ((iTargetCount * 5) > getGroup()->getNumUnits())
			{
				MissionAITypes eMissionAIType = MISSIONAI_GROUP;
				int iJoiners = kPlayer.AI_unitTargetMissionAIs(this, &eMissionAIType, 1, getGroup(), 2);

				if ((iJoiners * 5) > getGroup()->getNumUnits())
				{
					getGroup()->pushMission(MISSION_SKIP);
					return;
				}

				if (AI_moveToStagingCity())
				{
					return;
				}
			}
		}
	}

	if (AI_heal(50, 3))
	{
		return;
	}

	if (!bAtWar)
	{
		if (AI_heal())
		{
			return;
		}

		if ((getGroup()->getNumUnits() == 1) && (getTeam() != plot()->getTeam()))
		{
			if (AI_retreatToCity())
			{
				return;
			}
		}
	}

	if (!bReadyToAttack && !noDefensiveBonus())
	{
		if (AI_guardCity(false, false))
		{
			return;
		}
	}

	bool bAnyWarPlan = (GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0);

	if (bReadyToAttack)
	{
		if (bHuntBarbs || pTargetCity == NULL)
		{
			if (AI_goToTargetBarbCity((bAnyWarPlan ? 7 : 15)))
			{
				return;
			}
		}
		else if (bLandWar && pTargetCity != NULL && (getGroup()->getNumUnits() > 1))
		{
			// Before heading out, check whether to wait to allow unit upgrades
			if (bInCity && plot()->getOwnerINLINE() == getOwnerINLINE())
			{
				if (!(kPlayer.AI_isFinancialTrouble()))
				{
					// Check if stack has units which can upgrade
					int iNeedUpgradeCount = 0;

					CLLNode<IDInfo>* pUnitNode = getGroup()->headUnitNode();
					while (pUnitNode != NULL)
					{
						CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
						pUnitNode = getGroup()->nextUnitNode(pUnitNode);

						if (pLoopUnit->getUpgradeCity(false) != NULL)
						{
							iNeedUpgradeCount++;

							if ((2 * iNeedUpgradeCount) > getGroup()->getNumUnits()) // was 8
							{
								if (getGroup()->getNumUnits() < (kPlayer.getNumCities() * 8))
								{
									getGroup()->pushMission(MISSION_SKIP);
									return;
								}
							}
						}
					}
				}
			}

			if (AI_goToTargetCity(MOVE_AVOID_ENEMY_WEIGHT_2, 5, pTargetCity))
			{
				return;
			}

			if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 2, 2))
			{
				return;
			}

			if (AI_goToTargetCity(MOVE_AVOID_ENEMY_WEIGHT_2, 8, pTargetCity))
			{
				return;
			}

			// Load stack if walking will take a long time
			if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4, 3))
			{
				return;
			}

			if (AI_goToTargetCity(MOVE_AVOID_ENEMY_WEIGHT_2, 12, pTargetCity))
			{
				return;
			}

			if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4, 7))
			{
				return;
			}


			if (AI_goToTargetCity(MOVE_AVOID_ENEMY_WEIGHT_2, MAX_INT, pTargetCity))
			{
				return;
			}


			if (bAnyWarPlan)
			{
				CvCity* pTargetCity = area()->getTargetCity(getOwnerINLINE());

				if (pTargetCity != NULL)
				{

					if (AI_solveBlockageProblem(pTargetCity->plot(), (GET_TEAM(getTeam()).getAtWarCount(true) == 0)))
					{
						return;
					}
				}
			}
		}
	}
	else
	{
		int iTargetCount = kPlayer.AI_unitTargetMissionAIs(this, MISSIONAI_GROUP);
		if (((iTargetCount * 4) > getGroup()->getNumUnits()) || ((getGroup()->getNumUnits() + iTargetCount) >= (bHuntBarbs ? 5 : AI_stackOfDoomExtra())))
		{
			MissionAITypes eMissionAIType = MISSIONAI_GROUP;
			int iJoiners = kPlayer.AI_unitTargetMissionAIs(this, &eMissionAIType, 1, getGroup(), 2);

			if ((iJoiners * 6) > getGroup()->getNumUnits())
			{
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}

			if (AI_safety())
			{
				return;
			}
		}

		if ((bombardRate() > 0) && noDefensiveBonus())
		{
			// BBAI Notes: Add this stack lead by bombard unit to stack probably not lead by a bombard unit
			// BBAI TODO: Some sense of minimum stack size?  Can have big stack moving 10 turns to merge with tiny stacks
			if (AI_group(UNITAI_ATTACK_CITY, -1, -1, -1, bIgnoreFaster, true, true, /*iMaxPath*/ 10, /*bAllowRegrouping*/ false))
			{
				return;
			}
		}
		else
		{
			if (AI_group(UNITAI_ATTACK_CITY, AI_stackOfDoomExtra() * 2, -1, -1, bIgnoreFaster, true, true, /*iMaxPath*/ 10, /*bAllowRegrouping*/ false))
			{
				return;
			}
		}
	}

	if (plot()->getOwnerINLINE() == getOwnerINLINE() && bLandWar)
	{
		if ((GET_TEAM(getTeam()).getAtWarCount(true) > 0))
		{
			// if no land path to enemy cities, try getting there another way
			if (AI_offensiveAirlift())
			{
				return;
			}

			if (pTargetCity != NULL)
			{
				if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_SAFE_TERRITORY, 4))
				{
					return;
				}
			}
		}
	}

	if (AI_moveToStagingCity())
	{
		return;
	}

	if (AI_offensiveAirlift())
	{
		return;
	}

	if (AI_guardFort(true))
	{
		return;
	}

	if (AI_travelToUpgradeCity())
	{
		return;
	}

	if (AI_retreatToCity())
	{
		return;
	}

	if (getGroup()->isStranded())
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_NO_ENEMY_TERRITORY, 1))
		{
			return;
		}

		if (!isHuman() && plot()->isCoastalLand() && kPlayer.AI_unitTargetMissionAIs(this, MISSIONAI_PICKUP) > 0)
		{
			// If no other desireable actions, wait for pickup
			getGroup()->pushMission(MISSION_SKIP);
			return;
		}

		if (AI_patrol())
		{
			return;
		}
	}

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}
void CvUnitAI::AI_terraformerMove()
{

	

	if (!isChanneler())
	{
		
		AI_setUnitAIType((UnitAITypes)m_pUnitInfo->getDefaultUnitAIType());
		return;
	}

	if (GET_PLAYER(getOwnerINLINE()).getDisableSpellcasting() > 0)
	{
		if (AI_retreatToCity())
		{
			return;
		}
	}

	if (plot()->isCity() && (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 3)))
	{
		getGroup()->pushMission(MISSION_SKIP);
		return;
	}

	CyUnit* pyUnit1 = new CyUnit(this);
	CyArgsList argsList1;
	argsList1.add(gDLL->getPythonIFace()->makePythonObject(pyUnit1));	// pass in unit class
	long lResult = -1;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "AI_MageTurn", argsList1.makeFunctionArgs(), &lResult);
	delete pyUnit1;	// python fxn must not hold on to this pointer
	

	if (lResult != 1)
	{
		// lResult of 2 means that the unit has been told to move somewhere or 
		// that the unit cannot move (either it already has or something is preventing it from moving)
		if (lResult == 2)
		{
			if (isHasCasted())
			{
				getGroup()->pushMission(MISSION_SKIP);
				return;
			}
			else
			{
				int iSpell = chooseSpell();
				if (iSpell != NO_SPELL)
				{
					cast(iSpell);
					return;
				}
				else // we have someplace to go but no useful spell to cast at the moment
				{
					getGroup()->pushMission(MISSION_SKIP);
					return;
				}
			}
		}

		if (GET_TEAM(getTeam()).getAtWarCount(false) > 0) //nothing to do and we're at war
		{
			if (getUnitCombatType() == GC.getInfoTypeForString("UNITCOMBAT_ADEPT"))
			{
			//	if (isDeBuffer() || isBuffer() || isDirectDamageCaster())
			//	{
					AI_setUnitAIType(UNITAI_WARWIZARD);
			//	}
			//	else
			//	{
			//		AI_setUnitAIType(UNITAI_MAGE);
			//	}
			}
			else
			{
				AI_setUnitAIType((UnitAITypes)m_pUnitInfo->getDefaultUnitAIType());
			}
		}

		if (AI_retreatToCity())
		{
			return;
		}

		getGroup()->pushMission(MISSION_SKIP);
		return;
	}
	// Tholal note: terraformers can get stuck in loop, if they've casted, are at a terraformable plot and have movement left

	if (isHasCasted())
	{
		getGroup()->pushMission(MISSION_SENTRY);
		//finishMoves();
	}

	return;
}

bool CvUnitAI::isChanneler()
{

	if (isHasPromotion((PromotionTypes)GC.getInfoTypeForString("PROMOTION_CHANNELING1")))
		return true;

	if (isHasPromotion((PromotionTypes)GC.getInfoTypeForString("PROMOTION_CHANNELING2")))
		return true;

	if (isHasPromotion((PromotionTypes)GC.getInfoTypeForString("PROMOTION_CHANNELING3")))
		return true;

	return false;
}
bool CvUnitAI::AI_pickupEquipment(int iRange)
{
	CvUnit* pLoopUnit;
	CvUnit* pBestUnit = NULL;
	CvPlot* pBestPlot;
	int iValue = 0;
	int iBestValue = iRange + 1;
	int iLoop;
	int iPathTurns;
	CvPlayer& kPlayer = GET_PLAYER(getOwnerINLINE());

	if (getMoves() > 1)
	{
		iRange++;
	}

	// First, look for our equipment and treasure
	for (pLoopUnit = kPlayer.firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = kPlayer.nextUnit(&iLoop))
	{
		if (pLoopUnit->getUnitInfo().isCommunalProperty() && hasInterestingEquipment(pLoopUnit))
		{
			if (generatePath(pLoopUnit->plot(), 0, true, &iPathTurns))
			{
				if (iPathTurns <= iRange)
				{
					iValue = iPathTurns;
					if (iValue < iBestValue)
					{
						iBestValue = iValue;
						pBestUnit = pLoopUnit;
					}
				}
			}
		}
	}

	if (pBestUnit != NULL)
	{


		pBestPlot = pBestUnit->plot();
		if (atPlot(pBestPlot))
		{
			int ispell = chooseSpell();
			if (ispell != NO_SPELL)
			{
				cast(ispell);
			}
			getGroup()->pushMission(MISSION_SKIP);
			return true;
		}
		else
		{
			getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), MOVE_AVOID_ENEMY_WEIGHT_2);
			return true;
		}
	}

	// TODO - look for ENEMY equipment nearby
	/*
	CvPlot* pBestPlot = NULL;
	for (int iX = -iRange; iX <= iRange; iX++)
	{
		for (int iY = -iRange; iY <= iRange; iY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iX, iY);
			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->isVisibleEnemy(GET_TEAM(getTeam())))
				{

				}
			}
		}
	}
	*/

	return false;
}
void CvUnitAI::AI_fortcommanderMove()
{
	PROFILE_FUNC();



	if (AI_heal(30, 1))
	{
		return;
	}



	/*************************************************************************************************/
	/**	Tweak							05/05/11								Snarko				**/
	/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
	/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 30))
	{
		return;
	}
	/*************************************************************************************************/
	/**	Tweak									END													**/
	/*************************************************************************************************/



	/*************************************************************************************************/
	/**	Tweak							05/05/11								Snarko				**/
	/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
	/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 50))
	{
		return;
	}
	/*************************************************************************************************/
	/**	Tweak									END													**/
	/*************************************************************************************************/


	if (AI_heal())
	{
		return;
	}


	/*************************************************************************************************/
	/**	Tweak							05/05/11								Snarko				**/
	/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
	/*************************************************************************************************/
	if (AI_rangeAttack(airRange(), 101))
	{
		return;
	}
	/*************************************************************************************************/
	/**	Tweak									END													**/
	/*************************************************************************************************/


	getGroup()->pushMission(MISSION_SKIP);
	return;
}
void CvUnitAI::AI_healerMove()
{
	PROFILE_FUNC();

	if (AI_join())
	{
		return;
	}

	if (AI_discover(true, true))
	{
		return;
	}



	int iGoldenAgeValue = (GET_PLAYER(getOwnerINLINE()).AI_calculateGoldenAgeValue() / (GET_PLAYER(getOwnerINLINE()).unitsRequiredForGoldenAge()));
	int iDiscoverValue = std::max(1, getDiscoverResearch(NO_TECH));

	if (((iGoldenAgeValue * 100) / iDiscoverValue) > 60)
	{
		if (AI_goldenAge())
		{
			return;
		}

		if (iDiscoverValue > iGoldenAgeValue)
		{
			if (AI_discover())
			{
				return;
			}
		}
	}
	else
	{
		if (AI_discover())
		{
			return;
		}

		if (AI_construct(3))
		{
			return;
		}
	}

	/************************************************************************************************/
	/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
	/*                                                                                              */
	/* Unit AI, Efficiency                                                                          */
	/************************************************************************************************/
		//if ((GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 2) > 0) ||
	if ((GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2)) ||
		/************************************************************************************************/
		/* BETTER_BTS_AI_MOD                       END                                                  */
		/************************************************************************************************/
		(getGameTurnCreated() < (GC.getGameINLINE().getGameTurn() - 25)))
	{
		if (AI_discover())
		{
			return;
		}
	}

	if (AI_retreatToCity())
	{
		return;
	}

	/************************************************************************************************/
	/* BETTER_BTS_AI_MOD                      09/18/09                                jdog5000      */
	/*                                                                                              */
	/* Unit AI                                                                                      */
	/************************************************************************************************/
	if (getGroup()->isStranded())
	{
		if (AI_load(UNITAI_ASSAULT_SEA, MISSIONAI_LOAD_ASSAULT, NO_UNITAI, -1, -1, -1, -1, MOVE_NO_ENEMY_TERRITORY, 1))
		{
			return;
		}
	}
	/************************************************************************************************/
	/* BETTER_BTS_AI_MOD                       END                                                  */
	/************************************************************************************************/

	if (AI_safety())
	{
		return;
	}

	getGroup()->pushMission(MISSION_SKIP);
	return;
}
bool CvUnitAI::hasInterestingEquipment(CvUnit* pTargetUnit)
{
	int iBestValue = -1;
	for (int iI = 0; iI < GC.getNumPromotionInfos(); iI++)
	{
		if (pTargetUnit->isHasPromotion((PromotionTypes)iI))
		{
			if (GC.getPromotionInfo((PromotionTypes)iI).isEquipment()) {
				int iLoopValue = AI_promotionValue((PromotionTypes)iI);
					if (iLoopValue > iBestValue)
					{
						iBestValue = iLoopValue;
					}
			}
		}


	}
	return (iBestValue > 0);
}

bool CvUnitAI::AI_AddPopToCity()
{
	PROFILE_FUNC();

	CvCity* pCity;
	CvCity* pLoopCity;
	CvPlot* pBestPlot = NULL;
	int iPathTurns;
	int iValue;
	int iBestValue = MAX_INT;
	int iPass;
	int iLoop;
	CvString szError;

	pCity = plot()->getPlotCity();

	if (!canAddPop())
		return false;
	//szError.Format("starting Ai add poptocity ");			
	//gDLL->logMsg("scionai.log", szError);


	if (pCity != NULL)
	{
		if (pCity->getOwnerINLINE() == getOwnerINLINE())
		{
			if (canAddPopToCity(pCity))
			{
				//	szError.Format("Adding pop to current city is possible, missionjoinpop is pushed ");			
				//	gDLL->logMsg("scionai.log", szError);

				joinPop();
				return true;
			}
		}
	}


	for (iPass = 0; iPass < 4; iPass++)
	{
		for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
		{
			if (AI_plotValid(pLoopCity->plot()))
			{
				if (canAddPopToCity(pLoopCity))
				{
					//		szError.Format("found a loop city needing pop, looking for path");			
					//		gDLL->logMsg("scionai.log", szError);
		/************************************************************************************************/
		/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
		/*                                                                                              */
		/* Unit AI, Efficiency                                                                          */
		/************************************************************************************************/
									// BBAI efficiency: check area for land units before generating path
					if ((getDomainType() == DOMAIN_LAND) && (pLoopCity->area() != area()) && !(getGroup()->canMoveAllTerrain()))
					{
						continue;
					}
					/************************************************************************************************/
					/* BETTER_BTS_AI_MOD                       END                                                  */
					/************************************************************************************************/
					/*************************************************************************************************/
					/**	TipToe									04/04/09								Xienwolf	**/
					/**																								**/
					/**						Safe Movement is ideal situation where possible							**/
					/*************************************************************************************************/
					/**								---- Start Original Code ----									**
												if (!atPlot(pLoopCity->plot()) && generatePath(pLoopCity->plot(), ((iPass > 1) ? MOVE_IGNORE_DANGER : 0), true, &iPathTurns))
					/**								----  End Original Code  ----									**/
					if (!atPlot(pLoopCity->plot()) && generatePath(pLoopCity->plot(), ((iPass > 1) ? MOVE_IGNORE_DANGER : ((iPass == 0) ? MOVE_SAFE_TERRITORY : 0)), true, &iPathTurns))
						/*************************************************************************************************/
						/**	TipToe									END													**/
						/*************************************************************************************************/
					{
						if (iPathTurns <= 10)
						{
							/************************************************************************************************/
							/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
							/*                                                                                              */
							/* Naval AI                                                                                     */
							/************************************************************************************************/
							/* original bts code
																if ((iPass > 0) || (getGroup()->canFight() || GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(pLoopCity->plot()) < iCurrentDanger))
							*/
							// Water units can't defend a city
							// Any unthreatened city acceptable on 0th pass, solves problem where sea units
							// would oscillate in and out of threatened city because they had iCurrentDanger = 0
							// on turns outside city

							bool bCheck = (iPass > 0) || (getGroup()->canDefend());
							if (!bCheck)
							{
								/*************************************************************************************************/
								/**	Speedup								11/02/12										Snarko	**/
								/**																								**/
								/**		For the few cases where getPlotDanger is still used, use a threshold where possible		**/
								/*************************************************************************************************/
								/**								---- Start Original Code ----									**
																		int iLoopDanger = GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(pLoopCity->plot());
																		bCheck = (iLoopDanger == 0) || (iLoopDanger < iCurrentDanger);
								/**								----  End Original Code  ----									**/
								bCheck = !GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(pLoopCity->plot(), -1, true);
								/*************************************************************************************************/
								/**	Speedup									END													**/
								/*************************************************************************************************/
							}

							if (bCheck)
								/************************************************************************************************/
								/* BETTER_BTS_AI_MOD                       END                                                  */
								/************************************************************************************************/
							{
								iValue = iPathTurns;

								if (AI_getUnitAIType() == UNITAI_SETTLER_SEA)
								{
									iValue *= 1 + std::max(0, GET_PLAYER(getOwnerINLINE()).AI_totalAreaUnitAIs(pLoopCity->area(), UNITAI_SETTLE) - GET_PLAYER(getOwnerINLINE()).AI_totalAreaUnitAIs(pLoopCity->area(), UNITAI_SETTLER_SEA));
								}

								if (iValue < iBestValue)
								{
									iBestValue = iValue;
									pBestPlot = getPathEndTurnPlot();
									//					szError.Format("found a path ");			
									//					gDLL->logMsg("scionai.log", szError);
					/************************************************************************************************/
					/* BETTER_BTS_AI_MOD                      01/27/08                                jdog5000      */
					/*                                                                                              */
					/* Bugfix                                                                                       */
					/************************************************************************************************/
																// Not sure what can go wrong here, it seems somehow m_iData1 (moves) was set to 0
																// for first node in path so m_iData2 (turns) incremented
									if (atPlot(pBestPlot))
									{
										//FAssert(false);
										pBestPlot = getGroup()->getPathFirstPlot();
										FAssert(!atPlot(pBestPlot));
									}
									/************************************************************************************************/
									/* BETTER_BTS_AI_MOD                       END                                                  */
									/************************************************************************************************/
								}
							}
						}
					}
				}
			}
		}

		if (pBestPlot != NULL)
		{
			break;
		}
		if (getGroup()->alwaysInvisible())
		{
			break;
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(!atPlot(pBestPlot));
		//	szError.Format("pushed move mission ");			
		//	gDLL->logMsg("scionai.log", szError);
		getGroup()->pushMission(MISSION_MOVE_TO, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), ((iPass > 0) ? MOVE_IGNORE_DANGER : 0));
		return true;
	}
	return false;
}
