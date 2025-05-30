// teamAI.cpp

#include "CvGameCoreDLL.h"
#include "CvTeamAI.h"
#include "CvPlayerAI.h"
#include "CvRandom.h"
#include "CvGlobals.h"
#include "CvGameCoreUtils.h"
#include "CvMap.h"
#include "CvPlot.h"
#include "CvDLLInterfaceIFaceBase.h"
#include "CvGameAI.h"
#include "CvInfos.h"
#include "FProfiler.h"
#include "CyArgsList.h"
#include "CvDLLPythonIFaceBase.h"

// statics

CvTeamAI* CvTeamAI::m_aTeams = NULL;

void CvTeamAI::initStatics()
{
	m_aTeams = new CvTeamAI[MAX_TEAMS];
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		m_aTeams[iI].m_eID = ((TeamTypes)iI);
	}
}

void CvTeamAI::freeStatics()
{
	SAFE_DELETE_ARRAY(m_aTeams);
}

// inlined for performance reasons
DllExport CvTeamAI& CvTeamAI::getTeamNonInl(TeamTypes eTeam)
{
	return getTeam(eTeam);
}


// Public Functions...

CvTeamAI::CvTeamAI()
{
	m_aiWarPlanStateCounter = new int[MAX_TEAMS];
	m_aiAtWarCounter = new int[MAX_TEAMS];
	m_aiAtPeaceCounter = new int[MAX_TEAMS];
	m_aiHasMetCounter = new int[MAX_TEAMS];
	m_aiOpenBordersCounter = new int[MAX_TEAMS];
	m_aiDefensivePactCounter = new int[MAX_TEAMS];
	m_aiShareWarCounter = new int[MAX_TEAMS];
	m_aiWarSuccess = new int[MAX_TEAMS];
	m_aiEnemyPeacetimeTradeValue = new int[MAX_TEAMS];
	m_aiEnemyPeacetimeGrantValue = new int[MAX_TEAMS];
	m_aeWarPlan = new WarPlanTypes[MAX_TEAMS];


	AI_reset(true);
}


CvTeamAI::~CvTeamAI()
{
	AI_uninit();

	SAFE_DELETE_ARRAY(m_aiWarPlanStateCounter);
	SAFE_DELETE_ARRAY(m_aiAtWarCounter);
	SAFE_DELETE_ARRAY(m_aiAtPeaceCounter);
	SAFE_DELETE_ARRAY(m_aiHasMetCounter);
	SAFE_DELETE_ARRAY(m_aiOpenBordersCounter);
	SAFE_DELETE_ARRAY(m_aiDefensivePactCounter);
	SAFE_DELETE_ARRAY(m_aiShareWarCounter);
	SAFE_DELETE_ARRAY(m_aiWarSuccess);
	SAFE_DELETE_ARRAY(m_aiEnemyPeacetimeTradeValue);
	SAFE_DELETE_ARRAY(m_aiEnemyPeacetimeGrantValue);
	SAFE_DELETE_ARRAY(m_aeWarPlan);
}


void CvTeamAI::AI_init()
{
	AI_reset(false);

	//--------------------------------
	// Init other game data
}


void CvTeamAI::AI_uninit()
{
}


void CvTeamAI::AI_reset(bool bConstructor)
{
	AI_uninit();

	m_eWorstEnemy = NO_TEAM;

	for (int iI = 0; iI < MAX_TEAMS; iI++)
	{
		m_aiWarPlanStateCounter[iI] = 0;
		m_aiAtWarCounter[iI] = 0;
		m_aiAtPeaceCounter[iI] = 0;
		m_aiHasMetCounter[iI] = 0;
		m_aiOpenBordersCounter[iI] = 0;
		m_aiDefensivePactCounter[iI] = 0;
		m_aiShareWarCounter[iI] = 0;
		m_aiWarSuccess[iI] = 0;
		m_aiEnemyPeacetimeTradeValue[iI] = 0;
		m_aiEnemyPeacetimeGrantValue[iI] = 0;
		m_aeWarPlan[iI] = NO_WARPLAN;

		if (!bConstructor && getID() != NO_TEAM)
		{
			TeamTypes eLoopTeam = (TeamTypes) iI;
			CvTeamAI& kLoopTeam = GET_TEAM(eLoopTeam);
			kLoopTeam.m_aiWarPlanStateCounter[getID()] = 0;
			kLoopTeam.m_aiAtWarCounter[getID()] = 0;
			kLoopTeam.m_aiAtPeaceCounter[getID()] = 0;
			kLoopTeam.m_aiHasMetCounter[getID()] = 0;
			kLoopTeam.m_aiOpenBordersCounter[getID()] = 0;
			kLoopTeam.m_aiDefensivePactCounter[getID()] = 0;
			kLoopTeam.m_aiShareWarCounter[getID()] = 0;
			kLoopTeam.m_aiWarSuccess[getID()] = 0;
			kLoopTeam.m_aiEnemyPeacetimeTradeValue[getID()] = 0;
			kLoopTeam.m_aiEnemyPeacetimeGrantValue[getID()] = 0;
			kLoopTeam.m_aeWarPlan[getID()] = NO_WARPLAN;
		}
	}
}


void CvTeamAI::AI_doTurnPre()
{
	AI_doCounter();

	if (isHuman())
	{
		return;
	}

	if (isBarbarian())
	{
		return;
	}

	if (isMinorCiv())
	{
		return;
	}
}


void CvTeamAI::AI_doTurnPost()
{
	AI_updateWorstEnemy();

	AI_updateAreaStragies(false);

	if (isHuman())
	{
		return;
	}

	if (isBarbarian())
	{
		return;
	}

	if (isMinorCiv())
	{
		return;
	}

	AI_doWar();
}


void CvTeamAI::AI_makeAssignWorkDirty()
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				GET_PLAYER((PlayerTypes)iI).AI_makeAssignWorkDirty();
			}
		}
	}
}

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						10/6/08				jdog5000	*/
/* 																			*/
/* 	General AI																*/
/********************************************************************************/
// Find plot strength of teammates and potentially vassals
int CvTeamAI::AI_getOurPlotStrength(CvPlot* pPlot, int iRange, bool bDefensiveBonuses, bool bTestMoves, bool bIncludeVassals)
{
	int iI;
	int iPlotStrength = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID() || (bIncludeVassals && GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).isVassal(getID())) )
			{
				iPlotStrength += GET_PLAYER((PlayerTypes)iI).AI_getOurPlotStrength(pPlot,iRange,bDefensiveBonuses,bTestMoves);
			}
		}
	}

	return iPlotStrength;
}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/


void CvTeamAI::AI_updateAreaStragies(bool bTargets)
{
	CvArea* pLoopArea;
	int iLoop;

	if (!(GC.getGameINLINE().isFinalInitialized()))
	{
		return;
	}

	for(pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
	{
		pLoopArea->setAreaAIType(getID(), AI_calculateAreaAIType(pLoopArea));
	}

	if (bTargets)
	{
		AI_updateAreaTargets();
	}
}


void CvTeamAI::AI_updateAreaTargets()
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				GET_PLAYER((PlayerTypes)iI).AI_updateAreaTargets();
			}
		}
	}
}


int CvTeamAI::AI_countFinancialTrouble() const
{
	int iCount;
	int iI;

	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				if (GET_PLAYER((PlayerTypes)iI).AI_isFinancialTrouble())
				{
					iCount++;
				}
			}
		}
	}

	return iCount;
}


int CvTeamAI::AI_countMilitaryWeight(CvArea* pArea) const
{
	int iCount;
	int iI;

	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iCount += GET_PLAYER((PlayerTypes)iI).AI_militaryWeight(pArea);
			}
		}
	}

	return iCount;
}


bool CvTeamAI::AI_isAnyCapitalAreaAlone() const
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				if (GET_PLAYER((PlayerTypes)iI).AI_isCapitalAreaAlone())
				{
					return true;
				}
			}
		}
	}

	return false;
}


bool CvTeamAI::AI_isPrimaryArea(CvArea* pArea) const
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				if (GET_PLAYER((PlayerTypes)iI).AI_isPrimaryArea(pArea))
				{
					return true;
				}
			}
		}
	}

	return false;
}


bool CvTeamAI::AI_hasCitiesInPrimaryArea(TeamTypes eTeam) const
{
	CvArea* pLoopArea;
	int iLoop;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	for(pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
	{
		if (AI_isPrimaryArea(pLoopArea))
		{
			if (GET_TEAM(eTeam).countNumCitiesByArea(pLoopArea))
			{
				return true;
			}
		}
	}

	return false;
}


AreaAITypes CvTeamAI::AI_calculateAreaAIType(CvArea* pArea, bool bPreparingTotal) const
{
	PROFILE_FUNC();

	bool bRecentAttack;
	bool bTargets;
	bool bChosenTargets;
	bool bDeclaredTargets;
	int iOffensiveThreshold;
	int iAreaCities;
	int iI;

	if (!(pArea->isWater()))
	{
		if (isBarbarian())
		{
/*************************************************************************************************/
/**	MultiBarb							12/23/08									Xienwolf	**/
/**																								**/
/**							Adds extra Barbarian Civilizations									**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
			if ((pArea->getNumCities() - pArea->getCitiesPerPlayer(BARBARIAN_PLAYER)) == 0)
/**								----  End Original Code  ----									**/
			if ((pArea->getNumCities() - pArea->getCitiesPerPlayer(ORC_PLAYER) - pArea->getCitiesPerPlayer(ANIMAL_PLAYER) - pArea->getCitiesPerPlayer(DEMON_PLAYER)) == 0)
/*************************************************************************************************/
/**	MultiBarb								END													**/
/*************************************************************************************************/
			{
				return AREAAI_ASSAULT;
			}
			if ((countNumAIUnitsByArea(pArea, UNITAI_ATTACK) + countNumAIUnitsByArea(pArea, UNITAI_ATTACK_CITY) + countNumAIUnitsByArea(pArea, UNITAI_PILLAGE) + countNumAIUnitsByArea(pArea, UNITAI_ATTACK_AIR)) > (((AI_countMilitaryWeight(pArea) * 20) / 100) + 1))
			{
/*************************************************************************************************/
/**	Xienwolf Tweak							12/22/08											**/
/**																								**/
/**					This should get the AI to spread out who it attacks better					**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
				return AREAAI_OFFENSIVE; // XXX does this ever happen?
			}

			return AREAAI_MASSING;
/**								----  End Original Code  ----									**/
				return AREAAI_MASSING;
			}

			return AREAAI_OFFENSIVE;
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
		}

		bRecentAttack = false;
		bTargets = false;
		bChosenTargets = false;
		bDeclaredTargets = false;

		bool bAssault = false;
		bool bPreparingAssault = false;

		if (bPreparingTotal)
		{
			iOffensiveThreshold = 25;
		}
		else
		{
			iOffensiveThreshold = 20;
		}

		iAreaCities = countNumCitiesByArea(pArea);

		for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
			{
				if (AI_getWarPlan((TeamTypes)iI) != NO_WARPLAN)
				{
					FAssert(((TeamTypes)iI) != getID());
					FAssert(isHasMet((TeamTypes)iI) || GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_WAR));

					if (AI_getWarPlan((TeamTypes)iI) == WARPLAN_ATTACKED_RECENT)
					{
						FAssert(isAtWar((TeamTypes)iI));
						bRecentAttack = true;
					}

					if ((GET_TEAM((TeamTypes)iI).countNumCitiesByArea(pArea) > 0) || (GET_TEAM((TeamTypes)iI).countNumUnitsByArea(pArea) > 4))
					{
						bTargets = true;

						if (AI_isChosenWar((TeamTypes)iI))
						{
							bChosenTargets = true;

							if ((isAtWar((TeamTypes)iI)) ? (AI_getAtWarCounter((TeamTypes)iI) < 10) : AI_isSneakAttackReady((TeamTypes)iI))
							{
								bDeclaredTargets = true;
							}
						}
					}
					else
					{
						bAssault = true;
						if (AI_isSneakAttackPreparing((TeamTypes)iI))
						{
							bPreparingAssault = true;
						}
					}
				}
			}
		}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                         01/12/10                             jdog5000      */
/*                                                                                              */
/* War strategy AI                                                                              */
/************************************************************************************************/
		if( bTargets )
		{
			if(iAreaCities > 0 && getAtWarCount(true) > 0)
			{
				int iPower = countPowerByArea(pArea);
				int iEnemyPower = countEnemyPowerByArea(pArea);

				if( AI_isPrimaryArea(pArea) && AI_getWarSuccessCapitulationRatio() < -50 )
				{
					// Use defensive stance if enemy has strong presence, we've been losing badly
					if( iPower < iEnemyPower )
					{
						return AREAAI_DEFENSIVE;
					}
				}
				else
				{
					// Use defensive area AI right after invasions, to force vulnerable captured cities to be better defended
					if( 2*iPower < iEnemyPower )
					{
						return AREAAI_DEFENSIVE;
					}
				}
			}
		}

		if (bDeclaredTargets)
		{
			return AREAAI_OFFENSIVE;
		}

		if (bTargets)
		{
			// AI_countMilitaryWeight is based on this team's pop and cities ... if this team is the biggest, it will over estimate needed units
			int iMilitaryWeight = AI_countMilitaryWeight(pArea);
			int iCount = 1;

			for( int iJ = 0; iJ < MAX_CIV_TEAMS; iJ++ )
			{
				if( iJ != getID() && GET_TEAM((TeamTypes)iJ).isAlive() )
				{
					if( !(GET_TEAM((TeamTypes)iJ).isBarbarian() || GET_TEAM((TeamTypes)iJ).isMinorCiv()) )
					{
						if( AI_getWarPlan((TeamTypes)iJ) != NO_WARPLAN )
						{
							iMilitaryWeight += GET_TEAM((TeamTypes)iJ).AI_countMilitaryWeight(pArea);
							iCount++;

							if( GET_TEAM((TeamTypes)iJ).isAVassal() )
							{
								for( int iK = 0; iK < MAX_CIV_TEAMS; iK++ )
								{
									if( iK != getID() && GET_TEAM((TeamTypes)iK).isAlive() )
									{
										if( GET_TEAM((TeamTypes)iJ).isVassal((TeamTypes)iK) )
										{
											iMilitaryWeight += GET_TEAM((TeamTypes)iK).AI_countMilitaryWeight(pArea);
										}
									}
								}
							}
						}
					}
				}
			}

			iMilitaryWeight /= iCount;

			if ((countNumAIUnitsByArea(pArea, UNITAI_ATTACK) + countNumAIUnitsByArea(pArea, UNITAI_ATTACK_CITY) + countNumAIUnitsByArea(pArea, UNITAI_PILLAGE) + countNumAIUnitsByArea(pArea, UNITAI_ATTACK_AIR)) > (((iMilitaryWeight * iOffensiveThreshold) / 100) + 1))
			{
				return AREAAI_OFFENSIVE;
			}
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

		if (bTargets)
		{
			for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
			{
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       09/17/09                      cephalo & jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original BTS code
				CvPlayerAI& kPlayer = GET_PLAYER((PlayerTypes)iI);
*/
				CvPlayerAI& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/

				if (kPlayer.isAlive())
				{
					if (kPlayer.getTeam() == getID())
					{
						if (kPlayer.AI_isDoStrategy(AI_STRATEGY_DAGGER) || kPlayer.AI_isDoStrategy(AI_STRATEGY_FINAL_WAR))
						{
							if (pArea->getCitiesPerPlayer((PlayerTypes)iPlayer) > 0)
							{
								return AREAAI_MASSING;
							}
						}
					}
				}
			}
			if (bRecentAttack)
			{
				int iPower = countPowerByArea(pArea);
				int iEnemyPower = countEnemyPowerByArea(pArea);
				if (iPower > iEnemyPower)
				{
					return AREAAI_MASSING;
				}
				return AREAAI_DEFENSIVE;
			}
		}

		if (iAreaCities > 0)
		{
			if (countEnemyDangerByArea(pArea) > iAreaCities)
			{
				return AREAAI_DEFENSIVE;
			}
		}

		if (bChosenTargets)
		{
			return AREAAI_MASSING;
		}

		if (bTargets)
		{
			if (iAreaCities > (getNumMembers() * 3))
			{
				if (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) || GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_WAR) || (countPowerByArea(pArea) > ((countEnemyPowerByArea(pArea) * 3) / 2)))
				{
					return AREAAI_MASSING;
				}
			}
			return AREAAI_DEFENSIVE;
		}
		else
		{
			if (bAssault)
			{
				if (AI_isPrimaryArea(pArea))
				{
					if (bPreparingAssault)
					{
						return AREAAI_ASSAULT_MASSING;
					}
				}
				else if (countNumCitiesByArea(pArea) > 0)
				{
					return AREAAI_ASSAULT_ASSIST;
				}

				return AREAAI_ASSAULT;
			}
		}
	}

	return AREAAI_NEUTRAL;
}

int CvTeamAI::AI_calculateAdjacentLandPlots(TeamTypes eTeam) const
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	int iCount;
	int iI;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	iCount = 0;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (!(pLoopPlot->isWater()))
		{
			if ((pLoopPlot->getTeam() == eTeam) && pLoopPlot->isAdjacentTeam(getID(), true))
			{
				iCount++;
			}
		}
	}

	return iCount;
}


int CvTeamAI::AI_calculatePlotWarValue(TeamTypes eTeam) const
{
	FAssert(eTeam != getID());

	int iValue = 0;

	for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (pLoopPlot->getTeam() == eTeam)
		{
			if (!pLoopPlot->isWater() && pLoopPlot->isAdjacentTeam(getID(), true))
			{
				iValue += 4;
			}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/17/08                                jdog5000      */
/*                                                                                              */
/* Notes                                                                                        */
/************************************************************************************************/
			// This section of code does nothing without XML modding as AIObjective is 0 for all bonuses
			// Left alone for mods to use
			// Resource driven war in BBAI is done with CvTeamAI::AI_calculateBonusWarValue
			// without using the XML variable AIObjective
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			BonusTypes eBonus = pLoopPlot->getBonusType(getID());
			if (NO_BONUS != eBonus)
			{
				iValue += 40 * GC.getBonusInfo(eBonus).getAIObjective();
			}
		}
	}

	return iValue;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      07/21/08                                jdog5000      */
/*                                                                                              */
/* War Strategy AI                                                                              */
/************************************************************************************************/
int CvTeamAI::AI_calculateBonusWarValue(TeamTypes eTeam) const
{
	FAssert(eTeam != getID());

	int iValue = 0;

	for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (pLoopPlot->getTeam() == eTeam)
		{
			BonusTypes eNonObsoleteBonus = pLoopPlot->getNonObsoleteBonusType(getID());
			if (NO_BONUS != eNonObsoleteBonus)
			{
				int iThisValue = 0;
				for( int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++ )
				{
					if( getID() == GET_PLAYER((PlayerTypes)iJ).getTeam() && GET_PLAYER((PlayerTypes)iJ).isAlive() )
					{
						// 10 seems like a typical value for a health/happiness resource the AI doesn't have
						// Values for strategic resources can be 60 or higher
						iThisValue += GET_PLAYER((PlayerTypes)iJ).AI_bonusVal(eNonObsoleteBonus);
					}
				}
				iThisValue /= getAliveCount();

				if (!pLoopPlot->isWater())
				{
					if( pLoopPlot->isAdjacentTeam(getID(), true))
					{
						iThisValue *= 3;
					}
					else
					{
						CvCity* pWorkingCity = pLoopPlot->getWorkingCity();
						if( pWorkingCity != NULL )
						{
							for( int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++ )
							{
								if( getID() == GET_PLAYER((PlayerTypes)iJ).getTeam() && GET_PLAYER((PlayerTypes)iJ).isAlive() )
								{
									if( pWorkingCity->AI_playerCloseness((PlayerTypes)iJ ) > 0 )
									{
										iThisValue *= 2;
										break;
									}
								}
							}
						}
					}
				}

				iThisValue = std::max(0, iThisValue - 4);
				iThisValue /= 5;

				iValue += iThisValue;
			}
		}
	}

	return iValue;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

int CvTeamAI::AI_calculateCapitalProximity(TeamTypes eTeam) const
{
	CvCity* pOurCapitalCity;
	CvCity* pTheirCapitalCity;
	int iTotalDistance;
	int iCount;
	int iI, iJ;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	iTotalDistance = 0;
	iCount = 0;

	int iMinDistance = MAX_INT;
	int iMaxDistance = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				pOurCapitalCity = GET_PLAYER((PlayerTypes)iI).getCapitalCity();

				if (pOurCapitalCity != NULL)
				{
					for (iJ = 0; iJ < MAX_PLAYERS; iJ++)
					{
						if (GET_PLAYER((PlayerTypes)iJ).isAlive())
						{
							if (GET_PLAYER((PlayerTypes)iJ).getTeam() != getID())
							{
								pTheirCapitalCity = GET_PLAYER((PlayerTypes)iJ).getCapitalCity();

								if (pTheirCapitalCity != NULL)
								{
									int iDistance = (plotDistance(pOurCapitalCity->getX_INLINE(), pOurCapitalCity->getY_INLINE(), pTheirCapitalCity->getX_INLINE(), pTheirCapitalCity->getY_INLINE()) * (pOurCapitalCity->area() != pTheirCapitalCity->area() ? 3 : 2));
									if (GET_PLAYER((PlayerTypes)iJ).getTeam() == eTeam)
									{
										iTotalDistance += iDistance;
										iCount++;
									}
									iMinDistance = std::min(iDistance, iMinDistance);
									iMaxDistance = std::max(iDistance, iMaxDistance);
								}
							}
						}
					}
				}
			}
		}
	}

	if (iCount > 0)
	{
		FAssert(iMaxDistance > 0);
		return ((GC.getMapINLINE().maxPlotDistance() * (iMaxDistance - ((iTotalDistance / iCount) - iMinDistance))) / iMaxDistance);
	}

	return 0;
}


bool CvTeamAI::AI_isWarPossible() const
{
/*************************************************************************************************/
/**	ForLife								11/25/08									Xienwolf	**/
/**																								**/
/**					Improves AI Defensive Strategy, especially against Barbarians				**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	if (getAtWarCount(false) > 0)
/**								----  End Original Code  ----									**/
	if (getAtWarCount(false) > 0 || (isAtWar(GET_PLAYER(ORC_PLAYER).getTeam()) && (GC.getGameINLINE().isOption(GAMEOPTION_BARBARIAN_WORLD) || GC.getGameINLINE().isOption(GAMEOPTION_RAGING_BARBARIANS))))
/*************************************************************************************************/
/**	ForLife										END												**/
/*************************************************************************************************/
	{
		return true;
	}

	if (GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_WAR))
	{
		return true;
	}

	if (!(GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_PEACE)) && !(GC.getGameINLINE().isOption(GAMEOPTION_NO_CHANGING_WAR_PEACE)))
	{
		return true;
	}

	return false;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/12/10                         Fuyu & jdog5000      */
/*                                                                                              */
/* War Strategy AI                                                                              */
/************************************************************************************************/
bool CvTeamAI::AI_isLandTarget(TeamTypes eTeam, bool bNeighborsOnly) const
{
	if (!AI_hasCitiesInPrimaryArea(eTeam))
	{
		return false;
	}

	// If shared capital area is largely unclaimed, then we can reach over land
	int iModifier = 100;

	if( !bNeighborsOnly )
	{
		for( int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++ )
		{
			if( GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID() && GET_PLAYER((PlayerTypes)iPlayer).getNumCities() > 0 )
			{
				CvCity* pCapital = GET_PLAYER((PlayerTypes)iPlayer).getCapitalCity();
				if( pCapital != NULL )
				{
					if( GET_TEAM(eTeam).AI_isPrimaryArea(pCapital->area()) )
					{
						iModifier *= pCapital->area()->getNumOwnedTiles();
						iModifier /= pCapital->area()->getNumTiles();
					}
				}
			}
		}
	}

	if (AI_calculateAdjacentLandPlots(eTeam) < range((8 * (iModifier - 40))/40, 0, 8))
	{
		return false;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	return true;
}

// this determines if eTeam or any of its allies are land targets of us
bool CvTeamAI::AI_isAllyLandTarget(TeamTypes eTeam) const
{
	for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
	{
		CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iTeam);
		if (iTeam != getID())
		{
			if (iTeam == eTeam || kLoopTeam.isVassal(eTeam) || GET_TEAM(eTeam).isVassal((TeamTypes)iTeam) || kLoopTeam.isDefensivePact(eTeam))
			{
				if (AI_isLandTarget((TeamTypes)iTeam))
				{
					return true;
				}
			}
		}
	}

	return false;
}


bool CvTeamAI::AI_shareWar(TeamTypes eTeam) const
{
	int iI;

	for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive() && !GET_TEAM((TeamTypes)iI).isMinorCiv())
		{
			if ((iI != getID()) && (iI != eTeam))
			{
				if (isAtWar((TeamTypes)iI) && GET_TEAM(eTeam).isAtWar((TeamTypes)iI))
				{
					return true;
				}
			}
		}
	}

	return false;
}


AttitudeTypes CvTeamAI::AI_getAttitude(TeamTypes eTeam, bool bForced) const
{
	int iAttitude;
	int iCount;
	int iI, iJ;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	iAttitude = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				for (iJ = 0; iJ < MAX_PLAYERS; iJ++)
				{
					if (GET_PLAYER((PlayerTypes)iJ).isAlive() && iI != iJ)
					{
						// BBAI TODO: Attitude averaging ... what to do?
						TeamTypes eTeamLoop = GET_PLAYER((PlayerTypes)iJ).getTeam();
						if (eTeamLoop == eTeam || GET_TEAM(eTeamLoop).isVassal(eTeam) || GET_TEAM(eTeam).isVassal(eTeamLoop))
						{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/25/09                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
/*
							iAttitude += GET_PLAYER((PlayerTypes)iI).AI_getAttitude((PlayerTypes)iJ, bForced);
*/
							iAttitude += GET_PLAYER((PlayerTypes)iI).AI_getAttitudeVal((PlayerTypes)iJ, bForced);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
							iCount++;
						}
					}
				}
			}
		}
	}

	if (iCount > 0)
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/25/09                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
/*
		return ((AttitudeTypes)(iAttitude / iCount));
*/
		// This function is the same for all players, regardless of leader or whatever
		// so it's fine to use it for the team's attitude
		return GET_PLAYER(getLeaderID()).AI_getAttitudeFromValue(iAttitude/iCount);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	}

	return ATTITUDE_CAUTIOUS;
}


int CvTeamAI::AI_getAttitudeVal(TeamTypes eTeam, bool bForced) const
{
	int iAttitudeVal;
	int iCount;
	int iI, iJ;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	iAttitudeVal = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				for (iJ = 0; iJ < MAX_PLAYERS; iJ++)
				{
					if (GET_PLAYER((PlayerTypes)iJ).isAlive())
					{
						if (GET_PLAYER((PlayerTypes)iJ).getTeam() == eTeam)
						{
							iAttitudeVal += GET_PLAYER((PlayerTypes)iI).AI_getAttitudeVal((PlayerTypes)iJ, bForced);
							iCount++;
						}
					}
				}
			}
		}
	}

	if (iCount > 0)
	{
		return (iAttitudeVal / iCount);
	}

	return 0;
}


int CvTeamAI::AI_getMemoryCount(TeamTypes eTeam, MemoryTypes eMemory) const
{
	int iMemoryCount;
	int iCount;
	int iI, iJ;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	iMemoryCount = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				for (iJ = 0; iJ < MAX_PLAYERS; iJ++)
				{
					if (GET_PLAYER((PlayerTypes)iJ).isAlive())
					{
						if (GET_PLAYER((PlayerTypes)iJ).getTeam() == eTeam)
						{
							iMemoryCount += GET_PLAYER((PlayerTypes)iI).AI_getMemoryCount(((PlayerTypes)iJ), eMemory);
							iCount++;
						}
					}
				}
			}
		}
	}

	if (iCount > 0)
	{
		return (iMemoryCount / iCount);
	}

	return 0;
}


int CvTeamAI::AI_chooseElection(const VoteSelectionData& kVoteSelectionData) const
{
	VoteSourceTypes eVoteSource = kVoteSelectionData.eVoteSource;

	FAssert(!isHuman());
	FAssert(GC.getGameINLINE().getSecretaryGeneral(eVoteSource) == getID());

	int iBestVote = -1;
	int iBestValue = 0;

	for (int iI = 0; iI < (int)kVoteSelectionData.aVoteOptions.size(); ++iI)
	{
		VoteTypes eVote = kVoteSelectionData.aVoteOptions[iI].eVote;
		CvVoteInfo& kVoteInfo = GC.getVoteInfo(eVote);

		FAssert(kVoteInfo.isVoteSourceType(eVoteSource));

		FAssert(GC.getGameINLINE().isChooseElection(eVote));
		bool bValid = true;

		if (!GC.getGameINLINE().isTeamVote(eVote))
		{
			for (int iJ = 0; iJ < MAX_PLAYERS; iJ++)
			{
				if (GET_PLAYER((PlayerTypes)iJ).isAlive())
				{
					if (GET_PLAYER((PlayerTypes)iJ).getTeam() == getID())
					{
						PlayerVoteTypes eVote = GET_PLAYER((PlayerTypes)iJ).AI_diploVote(kVoteSelectionData.aVoteOptions[iI], eVoteSource, true);

						if (eVote != PLAYER_VOTE_YES || eVote == GC.getGameINLINE().getVoteOutcome((VoteTypes)iI))
						{
							bValid = false;
							break;
						}
					}
				}
			}
		}

		if (bValid)
		{
			int iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Choose Vote"));

			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				iBestVote = iI;
			}
		}
	}

	return iBestVote;
}

/// \brief Relative value of starting a war against eTeam.
///
/// This function computes the value of starting a war against eTeam.
/// The returned value should be compared against other possible targets
/// to pick the best target.
int CvTeamAI::AI_startWarVal(TeamTypes eTeam) const
{
	PROFILE_FUNC();

	int iValue;

	iValue = AI_calculatePlotWarValue(eTeam);

	iValue += (3 * AI_calculateCapitalProximity(eTeam)) / ((iValue > 0) ? 2 : 3);

	int iClosenessValue = AI_teamCloseness(eTeam);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/16/10                                jdog5000      */
/*                                                                                              */
/* War Strategy AI, Victory Strategy AI                                                         */
/************************************************************************************************/
/* original code
	if (iClosenessValue == 0)
	{
		iValue /= 4;
	}
	iValue += iClosenessValue / 4;
*/
	// Dividing iValue by 4 is a drastic move, will result in more backstabbing between friendly neighbors
	// which is appropriate for Aggressive
	// Closeness values are much smaller after the fix to CvPlayerAI::AI_playerCloseness, no need to divide by 4
	if (iClosenessValue == 0)
	{
		iValue /= (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 4 : 2);
	}
	iValue += iClosenessValue;

	iValue += AI_calculateBonusWarValue(eTeam);

	// Target other teams close to victory
	if( GET_TEAM(eTeam).AI_isAnyMemberDoVictoryStrategyLevel3() )
	{
		iValue += 10;

		bool bAggressive = GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI);
		bool bConq4 = AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST, 4);

		// Prioritize targets closer to victory
		if( bConq4 || bAggressive )
		{
			iValue *= 3;
			iValue /= 2;
		}

		if( GET_TEAM(eTeam).AI_isAnyMemberDoVictoryStrategyLevel4() )
		{
			if( GET_TEAM(eTeam).AI_getLowestVictoryCountdown() >= 0 )
			{
				iValue += 50;
			}

			iValue *= 2;

			if( bConq4 || bAggressive )
			{
				iValue *= 4;
			}
			else if( AI_isAnyMemberDoVictoryStrategyLevel3() )
			{
				iValue *= 2;
			}
		}
	}

	// This adapted legacy code just makes us more willing to enter a war in a trade deal
	// as boost applies to all rivals
	if( AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_DOMINATION, 3) )
	{
		iValue *= (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 3 : 2);
	}

	// If occupied or conquest inclined and early/not strong, value weak opponents
	if( getAnyWarPlanCount(true) > 0 ||
		(AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST, 2) && !(AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST, 3))) )
	{
		int iMultiplier = (75 * getPower(false))/std::max(1, GET_TEAM(eTeam).getDefensivePower());

		iValue *= range(iMultiplier, 50, 400);
		iValue /= 100;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	switch (AI_getAttitude(eTeam))
	{
	case ATTITUDE_FURIOUS:
		iValue *= 16;
		break;

	case ATTITUDE_ANNOYED:
		iValue *= 8;
		break;

	case ATTITUDE_CAUTIOUS:
		iValue *= 4;
		break;

	case ATTITUDE_PLEASED:
		iValue *= 2;
		break;

	case ATTITUDE_FRIENDLY:
		iValue *= 1;
		break;

	default:
		FAssert(false);
		break;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/21/10                                jdog5000      */
/*                                                                                              */
/* Victory Strategy AI                                                                          */
/************************************************************************************************/
	// Make it harder to bribe player to start a war
	if ( AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CULTURE, 4))
	{
		iValue /= 8;
	}
	else if ( AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_SPACE, 4))
	{
		iValue /= 4;
	}
	else if ( AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CULTURE, 3))
	{
		iValue /= 3;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	return iValue;
}


// XXX this should consider area power...
int CvTeamAI::AI_endWarVal(TeamTypes eTeam) const
{
	int iValue;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(isAtWar(eTeam), "Current AI Team instance is expected to be at war with eTeam");

	iValue = 100;

	iValue += (getNumCities() * 3);
	iValue += (GET_TEAM(eTeam).getNumCities() * 3);

	iValue += getTotalPopulation();
	iValue += GET_TEAM(eTeam).getTotalPopulation();

	iValue += (GET_TEAM(eTeam).AI_getWarSuccess(getID()) * 20);

	int iOurPower = std::max(1, getPower(true));
	int iTheirPower = std::max(1, GET_TEAM(eTeam).getDefensivePower());

	iValue *= iTheirPower + 10;

//FfH: Modified by Kael 04/23/2009
//	iValue /= std::max(1, iOurPower + iTheirPower + 10);
	iValue /= std::max(1, iOurPower + 10);
	if ((iOurPower * 100) > (iTheirPower * 120))
	{
		iValue /= 5;
	}
//FfH: End Modify

	WarPlanTypes eWarPlan = AI_getWarPlan(eTeam);

	// if we not human, do we want to continue war for strategic reasons?
	// only check if our power is at least 120% of theirs
	if (!isHuman() && iOurPower > ((120 * iTheirPower) / 100))
	{
		bool bDagger = false;

		bool bAnyFinancialTrouble = false;
		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
				{
					if (GET_PLAYER((PlayerTypes)iI).AI_isDoStrategy(AI_STRATEGY_DAGGER))
					{
						bDagger = true;
					}

					if (GET_PLAYER((PlayerTypes)iI).AI_isFinancialTrouble())
					{
						bAnyFinancialTrouble = true;
					}
				}
			}
		}

		// if dagger, value peace at 90% * power ratio
		if (bDagger)
		{
			iValue *= 9 * iTheirPower;
			iValue /= 10 * iOurPower;
		}

		// for now, we will always do the land mass check for domination
		// if we have more than half the land, then value peace at 90% * land ratio
		int iLandRatio = ((getTotalLand(true) * 100) / std::max(1, GET_TEAM(eTeam).getTotalLand(true)));
		if (iLandRatio > 120)
		{
			iValue *= 9 * 100;
			iValue /= 10 * iLandRatio;
		}

		// if in financial trouble, warmongers will continue the fight to make more money
		if (bAnyFinancialTrouble)
		{
			switch (eWarPlan)
			{
				case WARPLAN_TOTAL:
					// if we total warmonger, value peace at 70% * power ratio factor
					if (bDagger || AI_maxWarRand() < 100)
					{
						iValue *= 7 * (5 * iTheirPower);
						iValue /= 10 * (iOurPower + (4 * iTheirPower));
					}
					break;

				case WARPLAN_LIMITED:
					// if we limited warmonger, value peace at 70% * power ratio factor
					if (AI_limitedWarRand() < 100)
					{
						iValue *= 7 * (5 * iTheirPower);
						iValue /= 10 * (iOurPower + (4 * iTheirPower));
					}
					break;

				case WARPLAN_DOGPILE:
					// if we dogpile warmonger, value peace at 70% * power ratio factor
					if (AI_dogpileWarRand() < 100)
					{
						iValue *= 7 * (5 * iTheirPower);
						iValue /= 10 * (iOurPower + (4 * iTheirPower));
					}
					break;

			}
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/19/10                                jdog5000      */
/*                                                                                              */
/* War strategy AI, Victory Strategy AI                                                         */
/************************************************************************************************/
	if( AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CULTURE, 4) )
	{
		iValue *= 4;
	}
	else if( AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CULTURE, 3) || AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_SPACE, 4) )
	{
		iValue *= 2;
	}

	// XXX count units in enemy territory...

	if ((!(isHuman()) && (eWarPlan == WARPLAN_TOTAL)) ||
		  (!(GET_TEAM(eTeam).isHuman()) && (GET_TEAM(eTeam).AI_getWarPlan(getID()) == WARPLAN_TOTAL)))
	{
		iValue *= 2;
	}
	else if ((!(isHuman()) && (eWarPlan == WARPLAN_DOGPILE) && (GET_TEAM(eTeam).getAtWarCount(true) > 1)) ||
			   (!(GET_TEAM(eTeam).isHuman()) && (GET_TEAM(eTeam).AI_getWarPlan(getID()) == WARPLAN_DOGPILE) && (getAtWarCount(true) > 1)))
	{
		iValue *= 3;
		iValue /= 2;
	}

	// Do we have a big stack en route?
	int iOurAttackers = 0;
	for( int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++ )
	{
		if( GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID() )
		{
			iOurAttackers += GET_PLAYER((PlayerTypes)iPlayer).AI_enemyTargetMissions(eTeam);
		}
	}
	int iTheirAttackers = 0;
	CvArea* pLoopArea = NULL;
	int iLoop;
	for(pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
	{
		iTheirAttackers += countEnemyDangerByArea(pLoopArea, eTeam);
	}

	int iAttackerRatio = (100 * iOurAttackers) / std::max(1 + GC.getGameINLINE().getCurrentEra(), iTheirAttackers);

	if( GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) )
	{
		iValue *= 150;
		iValue /= range(iAttackerRatio, 150, 900);
	}
	else
	{
		iValue *= 200;
		iValue /= range(iAttackerRatio, 200, 600);
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	iValue -= (iValue % GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));

	if (isHuman())
	{
		return std::max(iValue, GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));
	}
	else
	{
		return iValue;
	}
}


int CvTeamAI::AI_techTradeVal(TechTypes eTech, TeamTypes eTeam) const
{
	FAssert(eTeam != getID());
	int iKnownCount;
	int iPossibleKnownCount;
	int iCost;
	int iValue;
	int iI;

	iCost = std::max(0, (getResearchCost(eTech) - getResearchProgress(eTech)));

	iValue = ((iCost * 3) / 2);

	iKnownCount = 0;
	iPossibleKnownCount = 0;

	for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (iI != getID())
			{
				if (isHasMet((TeamTypes)iI))
				{
					if (GET_TEAM((TeamTypes)iI).isHasTech(eTech))
					{
						iKnownCount++;
					}

					iPossibleKnownCount++;
				}
			}
		}
	}

	iValue += (((iCost / 2) * (iPossibleKnownCount - iKnownCount)) / iPossibleKnownCount);
	if (iKnownCount == 0)
		iValue += (iCost / 5);

	iValue *= std::max(0, (GC.getTechInfo(eTech).getAITradeModifier() + 100));
	iValue /= 100;

	iValue -= (iValue % GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));

	if (isHuman())
	{
		return std::max(iValue, GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));
	}
	else
	{
		return iValue;
	}
}


DenialTypes CvTeamAI::AI_techTrade(TechTypes eTech, TeamTypes eTeam) const
{
	PROFILE_FUNC();

	AttitudeTypes eAttitude;
	int iNoTechTradeThreshold;

//FfH: Modified by Kael 04/23/2009
//	int iTechTradeKnownPercent;
//	int iKnownCount;
//	int iPossibleKnownCount;
//FfH: End Modify

	int iI, iJ;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");


	if (GC.getGameINLINE().isOption(GAMEOPTION_NO_TECH_BROKERING))
	{
		CvTeam& kTeam = GET_TEAM(eTeam);

		if (!kTeam.isHasTech(eTech))
		{
			if (!kTeam.isHuman())
			{
				if (2 * kTeam.getResearchProgress(eTech) > kTeam.getResearchCost(eTech))
				{
					return DENIAL_NO_GAIN;
				}
			}
		}
	}

	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (isVassal(eTeam))
	{
		return NO_DENIAL;
	}

	if (isAtWar(eTeam))
	{
		return NO_DENIAL;
	}

	if (AI_getWorstEnemy() == eTeam)
	{
		return DENIAL_WORST_ENEMY;
	}

	eAttitude = AI_getAttitude(eTeam);

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				if (eAttitude <= GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getTechRefuseAttitudeThreshold())
				{
					return DENIAL_ATTITUDE;
				}
			}
		}
	}

	if (eAttitude < ATTITUDE_FRIENDLY)
	{
		if ((GC.getGameINLINE().getTeamRank(getID()) < (GC.getGameINLINE().countCivTeamsEverAlive() / 2)) ||
			  (GC.getGameINLINE().getTeamRank(eTeam) < (GC.getGameINLINE().countCivTeamsEverAlive() / 2)))
		{
			iNoTechTradeThreshold = AI_noTechTradeThreshold();

			iNoTechTradeThreshold *= std::max(0, (GC.getHandicapInfo(GET_TEAM(eTeam).getHandicapType()).getNoTechTradeModifier() + 100));
			iNoTechTradeThreshold /= 100;

			if (AI_getMemoryCount(eTeam, MEMORY_RECEIVED_TECH_FROM_ANY) > iNoTechTradeThreshold)
			{
				return DENIAL_TECH_WHORE;
			}
		}

//FfH: Modified by Kael 04/22/2009
//		iKnownCount = 0;
//		iPossibleKnownCount = 0;
//		for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
//		{
//			if (GET_TEAM((TeamTypes)iI).isAlive())
//			{
//				if ((iI != getID()) && (iI != eTeam))
//				{
//					if (isHasMet((TeamTypes)iI))
//					{
//						if (GET_TEAM((TeamTypes)iI).isHasTech(eTech))
//						{
//							iKnownCount++;
//						}
//						iPossibleKnownCount++;
//					}
//				}
//			}
//		}
//		iTechTradeKnownPercent = AI_techTradeKnownPercent();
//		iTechTradeKnownPercent *= std::max(0, (GC.getHandicapInfo(GET_TEAM(eTeam).getHandicapType()).getTechTradeKnownModifier() + 100));
//		iTechTradeKnownPercent /= 100;
//		iTechTradeKnownPercent *= AI_getTechMonopolyValue(eTech, eTeam);
//		iTechTradeKnownPercent /= 100;
//		if ((iPossibleKnownCount > 0) ? (((iKnownCount * 100) / iPossibleKnownCount) < iTechTradeKnownPercent) : (iTechTradeKnownPercent > 0))
//		{
//			return DENIAL_TECH_MONOPOLY;
//		}
//FfH: End Modify

	}

	for (iI = 0; iI < GC.getNumUnitInfos(); iI++)
	{
		if (isTechRequiredForUnit(eTech, ((UnitTypes)iI)))
		{
			if (isWorldUnitClass((UnitClassTypes)(GC.getUnitInfo((UnitTypes)iI).getUnitClassType())))
			{
				if (getUnitClassMaking((UnitClassTypes)(GC.getUnitInfo((UnitTypes)iI).getUnitClassType())) > 0)
				{
					return DENIAL_MYSTERY;
				}
			}
		}
	}

	for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		if (isTechRequiredForBuilding(eTech, ((BuildingTypes)iI)))
		{
			if (isWorldWonderClass((BuildingClassTypes)(GC.getBuildingInfo((BuildingTypes)iI).getBuildingClassType())))
			{
				if (getBuildingClassMaking((BuildingClassTypes)(GC.getBuildingInfo((BuildingTypes)iI).getBuildingClassType())) > 0)
				{
					return DENIAL_MYSTERY;
				}
			}
		}
	}

	for (iI = 0; iI < GC.getNumProjectInfos(); iI++)
	{
		if (GC.getProjectInfo((ProjectTypes)iI).getTechPrereq() == eTech)
		{
			if (isWorldProject((ProjectTypes)iI))
			{
				if (getProjectMaking((ProjectTypes)iI) > 0)
				{
					return DENIAL_MYSTERY;
				}
			}

			for (iJ = 0; iJ < GC.getNumVictoryInfos(); iJ++)
			{
				if (GC.getGameINLINE().isVictoryValid((VictoryTypes)iJ))
				{
					if (GC.getProjectInfo((ProjectTypes)iI).getVictoryThreshold((VictoryTypes)iJ))
					{
						return DENIAL_VICTORY;
					}
				}
			}
		}
	}

	return NO_DENIAL;
}


int CvTeamAI::AI_mapTradeVal(TeamTypes eTeam) const
{
	CvPlot* pLoopPlot;
	int iValue;
	int iI;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	iValue = 0;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (!(pLoopPlot->isRevealed(getID(), false)) && pLoopPlot->isRevealed(eTeam, false))
		{
			if (pLoopPlot->isWater())
			{
				iValue++;
			}
			else
			{
				iValue += 5;
			}
		}
	}

	iValue /= 10;

	if (GET_TEAM(eTeam).isVassal(getID()))
	{
		iValue /= 2;
	}

	iValue -= (iValue % GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));

	if (isHuman())
	{
		return std::max(iValue, GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));
	}
	else
	{
		return iValue;
	}
}


DenialTypes CvTeamAI::AI_mapTrade(TeamTypes eTeam) const
{
	PROFILE_FUNC();

	AttitudeTypes eAttitude;
	int iI;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (isVassal(eTeam))
	{
		return NO_DENIAL;
	}

	if (isAtWar(eTeam))
	{
		return NO_DENIAL;
	}

	if (AI_getWorstEnemy() == eTeam)
	{
		return DENIAL_WORST_ENEMY;
	}

	eAttitude = AI_getAttitude(eTeam);

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				if (eAttitude <= GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getMapRefuseAttitudeThreshold())
				{
					return DENIAL_ATTITUDE;
				}
			}
		}
	}

	return NO_DENIAL;
}


int CvTeamAI::AI_vassalTradeVal(TeamTypes eTeam) const
{
	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	return AI_surrenderTradeVal(eTeam);
}


DenialTypes CvTeamAI::AI_vassalTrade(TeamTypes eTeam) const
{
	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	CvTeamAI& kMasterTeam = GET_TEAM(eTeam);

	for (int iLoopTeam = 0; iLoopTeam < MAX_TEAMS; iLoopTeam++)
	{
		CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iLoopTeam);
		if (kLoopTeam.isAlive() && iLoopTeam != getID() && iLoopTeam != kMasterTeam.getID())
		{
			if (!kLoopTeam.isAtWar(kMasterTeam.getID()) && kLoopTeam.isAtWar(getID()))
			{
				if (kMasterTeam.isForcePeace((TeamTypes)iLoopTeam) || !kMasterTeam.canChangeWarPeace((TeamTypes)iLoopTeam))
				{
					if (!kLoopTeam.isAVassal())
					{
						return DENIAL_WAR_NOT_POSSIBLE_YOU;
					}
				}

				if (!kMasterTeam.isHuman())
				{
					DenialTypes eWarDenial = kMasterTeam.AI_declareWarTrade((TeamTypes)iLoopTeam, getID(), true);
					if (NO_DENIAL != eWarDenial)
					{
						return DENIAL_WAR_NOT_POSSIBLE_YOU;
					}
				}
			}
			else if (kLoopTeam.isAtWar(kMasterTeam.getID()) && !kLoopTeam.isAtWar(getID()))
			{
				if (!kMasterTeam.canChangeWarPeace((TeamTypes)iLoopTeam))
				{
					if (!kLoopTeam.isAVassal())
					{
						return DENIAL_PEACE_NOT_POSSIBLE_YOU;
					}
				}

				if (!kMasterTeam.isHuman())
				{
					DenialTypes ePeaceDenial = kMasterTeam.AI_makePeaceTrade((TeamTypes)iLoopTeam, getID());
					if (NO_DENIAL != ePeaceDenial)
					{
						return DENIAL_PEACE_NOT_POSSIBLE_YOU;
					}
				}
			}
		}
	}

	return AI_surrenderTrade(eTeam);
}


int CvTeamAI::AI_surrenderTradeVal(TeamTypes eTeam) const
{
	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	return 0;
}


DenialTypes CvTeamAI::AI_surrenderTrade(TeamTypes eTeam, int iPowerMultiplier) const
{
	PROFILE_FUNC();

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	CvTeam& kMasterTeam = GET_TEAM(eTeam);

	for (int iLoopTeam = 0; iLoopTeam < MAX_TEAMS; iLoopTeam++)
	{
		CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iLoopTeam);
		if (kLoopTeam.isAlive() && iLoopTeam != getID() && iLoopTeam != kMasterTeam.getID())
		{
			if (kLoopTeam.isAtWar(kMasterTeam.getID()) && !kLoopTeam.isAtWar(getID()))
			{
				if (isForcePeace((TeamTypes)iLoopTeam) || !canChangeWarPeace((TeamTypes)iLoopTeam))
				{
					if (!kLoopTeam.isAVassal())
					{
						return DENIAL_WAR_NOT_POSSIBLE_US;
					}
				}
			}
			else if (!kLoopTeam.isAtWar(kMasterTeam.getID()) && kLoopTeam.isAtWar(getID()))
			{
				if (!canChangeWarPeace((TeamTypes)iLoopTeam))
				{
					if (!kLoopTeam.isAVassal())
					{
						return DENIAL_PEACE_NOT_POSSIBLE_US;
					}
				}
			}
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/07/09                                jdog5000      */
/*                                                                                              */
/* Diplomacy                                                                                    */
/************************************************************************************************/
	if (isHuman() && kMasterTeam.isHuman())
	{
		return NO_DENIAL;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	int iAttitudeModifier = 0;

	if (!GET_TEAM(eTeam).isParent(getID()))
	{
		int iPersonalityModifier = 0;
		int iMembers = 0;
		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
				{
					iPersonalityModifier += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getVassalPowerModifier();
					++iMembers;
				}
			}
		}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/03/09                                jdog5000      */
/*                                                                                              */
/* War Strategy AI                                                                              */
/************************************************************************************************/
/* original BTS code
		int iTotalPower = GC.getGameINLINE().countTotalCivPower();
		int iAveragePower = iTotalPower / std::max(1, GC.getGameINLINE().countCivTeamsAlive());
*/

		int iTotalPower = 0;
		int iNumNonVassals = 0;
		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			CvTeam& kTeam = GET_TEAM((TeamTypes) iI);
			if (kTeam.isAlive() && !(kTeam.isMinorCiv()))
			{
				if( kTeam.isAVassal() && kTeam.isCapitulated() )
				{
					// Count capitulated vassals as a fractional add to their master's power
					iTotalPower += (2*kTeam.getPower(false))/5;
				}
				else
				{
					iTotalPower += kTeam.getPower(false);
					iNumNonVassals++;
				}
			}
		}
		int iAveragePower = iTotalPower / std::max(1, iNumNonVassals);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		int iMasterPower = GET_TEAM(eTeam).getPower(false);
		int iVassalPower = (getPower(true) * (iPowerMultiplier + iPersonalityModifier / std::max(1, iMembers))) / 100;

		if (isAtWar(eTeam))
		{
			int iTheirSuccess = std::max(10, GET_TEAM(eTeam).AI_getWarSuccess(getID()));
			int iOurSuccess = std::max(10, AI_getWarSuccess(eTeam));
			int iOthersBestSuccess = 0;
			for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
		{
				if (iTeam != eTeam && iTeam != getID())
				{
					CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iTeam);

					if (kLoopTeam.isAlive() && kLoopTeam.isAtWar(getID()))
					{
						int iSuccess = kLoopTeam.AI_getWarSuccess(getID());
						if (iSuccess > iOthersBestSuccess)
						{
							iOthersBestSuccess = iSuccess;
						}
					}
				}
			}

			// Discourage capitulation to a team that has not done the most damage
			if (iTheirSuccess < iOthersBestSuccess)
			{
				iOurSuccess += iOthersBestSuccess - iTheirSuccess;
			}

			iMasterPower = (2 * iMasterPower * iTheirSuccess) / (iTheirSuccess + iOurSuccess);

			if (AI_getWorstEnemy() == eTeam)
			{
				iMasterPower *= 3;
				iMasterPower /= 4;
			}
		}
		else
		{
			if (!GET_TEAM(eTeam).AI_isLandTarget(getID()))
			{
				iMasterPower /= 2;
			}
		}


		for (int iLoopTeam = 0; iLoopTeam < MAX_CIV_TEAMS; iLoopTeam++)
		{
			if (iLoopTeam != getID())
			{
				CvTeamAI& kLoopTeam = GET_TEAM((TeamTypes)iLoopTeam);

				if (kLoopTeam.isAlive())
				{
					if (kLoopTeam.AI_isLandTarget(getID()))
					{
						if (iLoopTeam != eTeam)
						{
							if (kLoopTeam.getPower(true) > getPower(true))
							{
								if (kLoopTeam.isAtWar(eTeam) && !kLoopTeam.isAtWar(getID()))
								{
									return DENIAL_POWER_YOUR_ENEMIES;
								}

								iAveragePower = (2 * iAveragePower * kLoopTeam.getPower(true)) / std::max(1, kLoopTeam.getPower(true) + getPower(true));

								iAttitudeModifier += (3 * kLoopTeam.getPower(true)) / std::max(1, getPower(true)) - 2;
							}

							if (!kLoopTeam.isAtWar(eTeam) && kLoopTeam.isAtWar(getID()))
							{
								iAveragePower = (iAveragePower * (getPower(true) + GET_TEAM(eTeam).getPower(false))) / std::max(1, getPower(true));
							}
						}
					}

					if (!atWar(getID(), eTeam))
					{
						if (kLoopTeam.isAtWar(eTeam) && !kLoopTeam.isAtWar(getID()))
						{
							DenialTypes eDenial = AI_declareWarTrade((TeamTypes)iLoopTeam, eTeam, false);
							if (eDenial != NO_DENIAL)
							{
								return eDenial;
							}
						}
					}
				}
			}
		}

		if (!isVassal(eTeam) && canVassalRevolt(eTeam))
		{
			return DENIAL_POWER_US;
		}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/02/09                                jdog5000      */
/*                                                                                              */
/* War Strategy AI                                                                              */
/************************************************************************************************/
/* original bts code
		if (iVassalPower > iAveragePower || 3 * iVassalPower > 2 * iMasterPower)
*/
		if (5*iVassalPower > 4*iAveragePower || 3 * iVassalPower > 2 * iMasterPower)
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		{
			return DENIAL_POWER_US;
		}

		for (int i = 0; i < GC.getNumVictoryInfos(); i++)
		{
			bool bPopulationThreat = true;
			if (GC.getGameINLINE().getAdjustedPopulationPercent((VictoryTypes)i) > 0)
			{
				bPopulationThreat = false;

				int iThreshold = GC.getGameINLINE().getTotalPopulation() * GC.getGameINLINE().getAdjustedPopulationPercent((VictoryTypes)i);
				if (400 * getTotalPopulation(!isAVassal()) > 3 * iThreshold)
				{
					return DENIAL_VICTORY;
				}

				if (!atWar(getID(), eTeam))
				{
					if (400 * (getTotalPopulation(isAVassal()) + GET_TEAM(eTeam).getTotalPopulation()) > 3 * iThreshold)
					{
						bPopulationThreat = true;
					}
				}
			}

			bool bLandThreat = true;
			if (GC.getGameINLINE().getAdjustedLandPercent((VictoryTypes)i) > 0)
			{
				bLandThreat = false;

				int iThreshold = GC.getMapINLINE().getLandPlots() * GC.getGameINLINE().getAdjustedLandPercent((VictoryTypes)i);
				if (400 * getTotalLand(!isAVassal()) > 3 * iThreshold)
				{
					return DENIAL_VICTORY;
				}

				if (!atWar(getID(), eTeam))
				{
					if (400 * (getTotalLand(isAVassal()) + GET_TEAM(eTeam).getTotalLand()) > 3 * iThreshold)
					{
						bLandThreat = true;
					}
				}
			}

			if (GC.getGameINLINE().getAdjustedPopulationPercent((VictoryTypes)i) > 0 || GC.getGameINLINE().getAdjustedLandPercent((VictoryTypes)i) > 0)
			{
				if (bLandThreat && bPopulationThreat)
				{
					return DENIAL_POWER_YOU;
				}
			}
		}
	}

	if (!isAtWar(eTeam))
	{
		if (!GET_TEAM(eTeam).isParent(getID()))
		{
			if (AI_getWorstEnemy() == eTeam)
			{
				return DENIAL_WORST_ENEMY;
			}

			if (!AI_hasCitiesInPrimaryArea(eTeam) && AI_calculateAdjacentLandPlots(eTeam) == 0)
			{
				return DENIAL_TOO_FAR;
			}
		}

		AttitudeTypes eAttitude = AI_getAttitude(eTeam, false);

		AttitudeTypes eModifiedAttitude = CvPlayerAI::AI_getAttitudeFromValue(AI_getAttitudeVal(eTeam, false) + iAttitudeModifier);

		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
				{
					if (eAttitude <= ATTITUDE_FURIOUS)
					{
						return DENIAL_ATTITUDE;
					}

					if (eModifiedAttitude <= GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getVassalRefuseAttitudeThreshold())
					{
						return DENIAL_ATTITUDE;
					}
				}
			}
		}
	}
	else
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/07/09                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
/* original BTS code
		if (AI_getWarSuccess(eTeam) + 4 * GC.getDefineINT("WAR_SUCCESS_CITY_CAPTURING") > GET_TEAM(eTeam).AI_getWarSuccess(getID()))
		{
			return DENIAL_JOKING;
		}
*/
		// Scale better for small empires, particularly necessary if WAR_SUCCESS_CITY_CAPTURING > 10
		if (AI_getWarSuccess(eTeam) + std::min(getNumCities(), 4) * GC.getWAR_SUCCESS_CITY_CAPTURING() > GET_TEAM(eTeam).AI_getWarSuccess(getID()))
		{
			return DENIAL_JOKING;
		}

		if( !kMasterTeam.isHuman() )
		{
			if( !(GET_TEAM(kMasterTeam.getID()).AI_acceptSurrender(getID())) )
			{
				return DENIAL_JOKING;
			}
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	}

	return NO_DENIAL;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/20/10                                jdog5000      */
/*                                                                                              */
/* Victory Strategy AI                                                                          */
/************************************************************************************************/
bool CvTeamAI::AI_isAnyMemberDoVictoryStrategy( int iVictoryStrategy, int iLevel ) const
{
	for( int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++ )
	{
		if( GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID() )
		{
			if( GET_PLAYER((PlayerTypes)iPlayer).isAlive() )
			{
				if( GET_PLAYER((PlayerTypes)iPlayer).AI_isDoVictoryStrategy(iVictoryStrategy, iLevel) )
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool CvTeamAI::AI_isAnyMemberDoVictoryStrategyLevel4() const
{
	for( int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++ )
	{
		if( GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID() )
		{
			if( GET_PLAYER((PlayerTypes)iPlayer).isAlive() )
			{
				if( GET_PLAYER((PlayerTypes)iPlayer).AI_isDoVictoryStrategyLevel4() )
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool CvTeamAI::AI_isAnyMemberDoVictoryStrategyLevel3() const
{
	for( int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++ )
	{
		if( GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID() )
		{
			if( GET_PLAYER((PlayerTypes)iPlayer).isAlive() )
			{
				if( GET_PLAYER((PlayerTypes)iPlayer).AI_isDoVictoryStrategyLevel3() )
				{
					return true;
				}
			}
		}
	}

	return false;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/20/10                                jdog5000      */
/*                                                                                              */
/* War Strategy AI                                                                              */
/************************************************************************************************/
/// \brief Compute how close our wars are to capitulation.
///
/// At -99, this means we're losing on net enough to capitulate.  At +99, we're winning enough
/// to perhaps have another player capitulate to us.
int CvTeamAI::AI_getWarSuccessCapitulationRatio() const
{
	int iSumWarSuccess = 0;

	for( int iI = 0; iI < MAX_CIV_TEAMS; iI++ )
	{
		if( iI != getID() )
		{
			if( GET_TEAM((TeamTypes)iI).isAlive() && isAtWar((TeamTypes)iI) )
			{
				iSumWarSuccess += AI_getWarSuccess((TeamTypes)iI);
				iSumWarSuccess -= GET_TEAM((TeamTypes)iI).AI_getWarSuccess(getID());
			}
		}
	}

	int iDivisor = std::max(25, std::min(getNumCities(), 4) * GC.getWAR_SUCCESS_CITY_CAPTURING());

	iSumWarSuccess = range((100*iSumWarSuccess)/iDivisor, -99, 99);

	return iSumWarSuccess;
}

/// \brief Compute power of enemies as percentage of our power.
///
///
int CvTeamAI::AI_getEnemyPowerPercent( bool bConsiderOthers ) const
{
	int iEnemyPower = 0;

	for( int iI = 0; iI < MAX_CIV_TEAMS; iI++ )
	{
		if( iI != getID() )
		{
			if( GET_TEAM((TeamTypes)iI).isAlive() && isHasMet((TeamTypes)iI) )
			{
				if( isAtWar((TeamTypes)iI) )
				{
					int iTempPower = 220 * GET_TEAM((TeamTypes)iI).getPower(false);
					iTempPower /= (AI_hasCitiesInPrimaryArea((TeamTypes)iI) ? 2 : 3);
					iTempPower /= (GET_TEAM((TeamTypes)iI).isMinorCiv() ? 3 : 1);
					iTempPower /= std::max(1, (bConsiderOthers ? GET_TEAM((TeamTypes)iI).getAtWarCount(true,true) : 1));
					iEnemyPower += iTempPower;
				}
				else if( AI_isChosenWar((TeamTypes)iI) )
				{
					// Haven't declared war yet
					int iTempPower = 240 * GET_TEAM((TeamTypes)iI).getDefensivePower();
					iTempPower /= (AI_hasCitiesInPrimaryArea((TeamTypes)iI) ? 2 : 3);
					iTempPower /= 1 + (bConsiderOthers ? GET_TEAM((TeamTypes)iI).getAtWarCount(true,true) : 0);
					iEnemyPower += iTempPower;
				}
			}
		}
	}

	return (iEnemyPower/std::max(1, (isAVassal() ? getCurrentMasterPower(true) : getPower(true))));
}

/// \brief Sum up air power of enemies plus average of other civs we've met.
///
int CvTeamAI::AI_getRivalAirPower( ) const
{
	// Count enemy air units, not just those visible to us
	int iRivalAirPower = 0;
	int iEnemyAirPower = 0;

	for (int iI = 0; iI < GC.getNumUnitInfos(); iI++)
	{
		CvUnitInfo& kUnitInfo = GC.getUnitInfo((UnitTypes)iI);

		if( kUnitInfo.getDomainType() == DOMAIN_AIR )
		{
			if( kUnitInfo.getAirCombat() > 0 )
			{
				for( int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++ )
				{
					if( iTeam != getID() )
					{
						if( GET_TEAM((TeamTypes)iTeam).isAlive() && isHasMet((TeamTypes)iTeam) )
						{
							int iUnitPower = GET_TEAM((TeamTypes)iTeam).getUnitClassCount((UnitClassTypes)kUnitInfo.getUnitClassType());

							if( iUnitPower > 0 )
							{
/*************************************************************************************************/
/**	Unit power						17/02/12										Snarko		**/
/**																								**/
/**	I have not changed this much, because air units can't use the new power system anyway.		**/
/**	So if you add air units to the game make sure to set powerAdd like PowerValue of old		**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
								iUnitPower *= kUnitInfo.getPowerValue();
/**								----  End Original Code  ----									**/
								iUnitPower *= kUnitInfo.getPowerAdd();
/*************************************************************************************************/
/**	Unit power						END															**/
/*************************************************************************************************/

								if( AI_getWarPlan((TeamTypes)iTeam) == NO_WARPLAN )
								{
									iRivalAirPower += iUnitPower;
								}
								else
								{
									iEnemyAirPower += iUnitPower;
								}
							}
						}
					}
				}
			}
		}
	}

	return (iEnemyAirPower + (iRivalAirPower / std::max(1,getHasMetCivCount(true))));
}

bool CvTeamAI::AI_acceptSurrender( TeamTypes eSurrenderTeam )
{
	PROFILE_FUNC();

	if( isHuman() )
	{
		return true;
	}

	if( !isAtWar(eSurrenderTeam) )
	{
		return true;
	}

	if( GET_TEAM(eSurrenderTeam).AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_SPACE, 3) )
	{
		// Capturing capital or Apollo city will stop space
		return false;
	}

	if( GET_TEAM(eSurrenderTeam).AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CULTURE, 3) )
	{
		// Capturing top culture cities will stop culture
		return false;
	}

	// Check for whether another team has won enough to cause capitulation
	for( int iI = 0; iI < MAX_CIV_TEAMS; iI++ )
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (iI != getID() && !(GET_TEAM((TeamTypes)iI).isVassal(getID())) )
			{
				if (GET_TEAM(eSurrenderTeam).isAtWar((TeamTypes)iI))
				{
					if (GET_TEAM(eSurrenderTeam).AI_getAtWarCounter((TeamTypes)iI) >= 10)
					{
						if( (GET_TEAM(eSurrenderTeam).AI_getWarSuccess((TeamTypes)iI) + std::min(GET_TEAM(eSurrenderTeam).getNumCities(), 4) * GC.getWAR_SUCCESS_CITY_CAPTURING()) < GET_TEAM((TeamTypes)iI).AI_getWarSuccess(eSurrenderTeam))
						{
							return true;
						}
					}
				}
			}
		}
	}

	int iValuableCities = 0;
	int iCitiesThreatenedByUs = 0;
	int iValuableCitiesThreatenedByUs = 0;
	int iCitiesThreatenedByOthers = 0;

	CvCity* pLoopCity;
	int iLoop;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if( GET_PLAYER((PlayerTypes)iI).getTeam() == eSurrenderTeam && GET_PLAYER((PlayerTypes)iI).isAlive() )
		{
			for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
			{
				bool bValuable = false;

				if( pLoopCity->isHolyCity() )
				{
					bValuable = true;
				}
				else if( pLoopCity->isHeadquarters() )
				{
					bValuable = true;
				}
				else if( pLoopCity->hasActiveWorldWonder() )
				{
					bValuable = true;
				}
				else if( AI_isPrimaryArea(pLoopCity->area()) && (GET_TEAM(eSurrenderTeam).countNumCitiesByArea(pLoopCity->area()) < 3) )
				{
					bValuable = true;
				}
				else if( pLoopCity->isCapital() && (GET_TEAM(eSurrenderTeam).getNumCities() > GET_TEAM(eSurrenderTeam).getNumMembers() || countNumCitiesByArea(pLoopCity->area()) > 0) )
				{
					bValuable = true;
				}
				else
				{
					// Valuable terrain bonuses
					CvPlot* pLoopPlot = NULL;
					for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
					{
						pLoopPlot = plotCity(pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(), iJ);

						if (pLoopPlot != NULL)
						{
							BonusTypes eBonus = pLoopPlot->getNonObsoleteBonusType(getID());
							if ( eBonus != NO_BONUS)
							{
								if(GET_PLAYER(getLeaderID()).AI_bonusVal(eBonus) > 15)
								{
									bValuable = true;
									break;
								}
							}
						}
					}
				}

				int iOwnerPower = GET_PLAYER((PlayerTypes)iI).AI_getOurPlotStrength(pLoopCity->plot(), 2, true, false);
				int iOurPower = AI_getOurPlotStrength(pLoopCity->plot(), 2, false, false, true);
				int iOtherPower = GET_PLAYER((PlayerTypes)iI).AI_getEnemyPlotStrength(pLoopCity->plot(), 2, false, false) - iOurPower;

				if( iOtherPower > iOwnerPower )
				{
					iCitiesThreatenedByOthers++;
				}

				if (iOurPower > iOwnerPower)
				{
					iCitiesThreatenedByUs++;
					if( bValuable )
					{
						iValuableCities++;
						iValuableCitiesThreatenedByUs++;
						continue;
					}
				}

				if( bValuable && pLoopCity->getHighestPopulation() < 5 )
				{
					bValuable = false;
				}

				if( bValuable )
				{
					if( AI_isPrimaryArea(pLoopCity->area()) )
					{
						iValuableCities++;
					}
					else
					{
						for (int iJ = 0; iJ < MAX_PLAYERS; iJ++)
						{
							if (GET_PLAYER((PlayerTypes)iJ).isAlive())
							{
								if (GET_PLAYER((PlayerTypes)iJ).getTeam() == getID())
								{
									if( pLoopCity->AI_playerCloseness((PlayerTypes)iJ) > 5 )
									{
										iValuableCities++;
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

	if( iValuableCitiesThreatenedByUs > 0 )
	{
		// Press for capture of valuable city
		return false;
	}

	if( iCitiesThreatenedByOthers > (1 + iCitiesThreatenedByUs/2) )
	{
		// Keep others from capturing spoils, but let it go if surrender civ is too small
		// to care about
		if( 6*(iValuableCities + GET_TEAM(eSurrenderTeam).getNumCities()) > getNumCities() )
		{
			return true;
		}
	}

	// If we're low on the totem poll, accept so enemies don't drag anyone else into war with us
	// Top rank is 0, second is 1, etc.
	int iTeamRank = GC.getGameINLINE().getTeamRank(getID());
	if( iTeamRank > (1 + GC.getGameINLINE().countCivTeamsAlive()/3) )
	{
		return true;
	}

	int iOurWarSuccessRatio = AI_getWarSuccessCapitulationRatio();
	if( iOurWarSuccessRatio < -30 )
	{
		// We're doing badly overall, need to be done with this war and gain an ally
		return true;
	}

	int iWarCount = 0;
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive() && !(GET_TEAM((TeamTypes)iI).isMinorCiv()))
		{
			if ((TeamTypes)iI != eSurrenderTeam && !(GET_TEAM((TeamTypes)iI).isVassal(eSurrenderTeam)))
			{
				if (isAtWar((TeamTypes)iI))
				{
					if( GET_TEAM((TeamTypes)iI).AI_getWarSuccess(getID()) > 5*GC.getDefineINT("WAR_SUCCESS_ATTACKING") )
					{
						iWarCount++;
					}
				}
			}
		}
	}

	if( iWarCount > 0 && iOurWarSuccessRatio < 50 )
	{
		// Accept if we have other wars to fight
		return true;
	}

	// War weariness
	int iWearinessThreshold = (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 300 : 240);
	iWearinessThreshold += 10*iValuableCities + 20*iCitiesThreatenedByUs;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID() )
			{
				int iWarWearinessPercentAnger = (getWarWeariness(eSurrenderTeam) * std::max(0, 100 + GET_TEAM(eSurrenderTeam).getEnemyWarWearinessModifier())) / 10000;
				iWarWearinessPercentAnger = GET_PLAYER((PlayerTypes)iI).getModifiedWarWearinessPercentAnger(iWarWearinessPercentAnger);

				// Significant war weariness from eSurrenderTeam, 1000 = 100%
				if( iWarWearinessPercentAnger > 50 )
				{
					if( GET_PLAYER((PlayerTypes)iI).getWarWearinessPercentAnger() > iWearinessThreshold )
					{
						return true;
					}
				}
			}
		}
	}

	if( (iValuableCities + iCitiesThreatenedByUs) >= (AI_maxWarRand()/100) )
	{
		// Continue conquest
		return false;
	}

	if( GET_TEAM(eSurrenderTeam).getNumCities() < (getNumCities()/4 - (AI_maxWarRand()/100)) )
	{
		// Too small to bother leaving alive
		return false;
	}

	return true;
}

void CvTeamAI::AI_getWarRands( int &iMaxWarRand, int &iLimitedWarRand, int &iDogpileWarRand ) const
{
	iMaxWarRand = AI_maxWarRand();
	iLimitedWarRand = AI_limitedWarRand();
	iDogpileWarRand = AI_dogpileWarRand();

	bool bCult4 = false;
	bool bSpace4 = false;
	bool bCult3 = false;
	bool bFinalWar = false;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).AI_isDoStrategy(AI_STRATEGY_FINAL_WAR))
				{
					bFinalWar = true;
				}

				if( GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE, 4))
				{
					bCult4 = true;
				}
				if( GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE, 3))
				{
					bCult3 = true;
				}
				if(GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_SPACE, 4))
				{
					bSpace4 = true;
				}
			}
		}
	}

	if( bCult4 )
	{
		iMaxWarRand *= 4;
		iLimitedWarRand *= 3;
		iDogpileWarRand *= 2;
	}
	else if( bSpace4 )
	{
		iMaxWarRand *= 3;

		iLimitedWarRand *= 2;

		iDogpileWarRand *= 3;
		iDogpileWarRand /= 2;
	}
	else if( bCult3 )
	{
		iMaxWarRand *= 2;

		iLimitedWarRand *= 3;
		iLimitedWarRand /= 2;

		iDogpileWarRand *= 3;
		iDogpileWarRand /= 2;
	}

	int iNumMembers = getNumMembers();
	int iNumVassals = getVassalCount();

	iMaxWarRand *= (2 + iNumMembers);
	iMaxWarRand /= (2 + iNumMembers + iNumVassals);

	if (bFinalWar)
	{
		iMaxWarRand /= 4;
	}

	iLimitedWarRand *= (2 + iNumMembers);
	iLimitedWarRand /= (2 + iNumMembers + iNumVassals);

	iDogpileWarRand *= (2 + iNumMembers);
	iDogpileWarRand /= (2 + iNumMembers + iNumVassals);
}


void CvTeamAI::AI_getWarThresholds( int &iTotalWarThreshold, int &iLimitedWarThreshold, int &iDogpileWarThreshold ) const
{
	iTotalWarThreshold = 0;
	iLimitedWarThreshold = 0;
	iDogpileWarThreshold = 0;

	int iHighUnitSpendingPercent = 0;
	bool bConq2 = false;
	bool bDom3 = false;
	bool bAggressive = GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI);
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				int iUnitSpendingPercent = (GET_PLAYER((PlayerTypes)iI).calculateUnitCost() * 100) / std::max(1, GET_PLAYER((PlayerTypes)iI).calculatePreInflatedCosts());
				iHighUnitSpendingPercent += (std::max(0, iUnitSpendingPercent - 7) / 2);

				if( GET_PLAYER((PlayerTypes)iI).AI_isDoStrategy(AI_STRATEGY_DAGGER))
				{
					bAggressive = true;
				}
				if( GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST, 4))
				{
					bAggressive = true;
				}
				if( GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION, 4))
				{
					bAggressive = true;
				}
				if( GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST, 2))
				{
					bConq2 = true;
				}
				if(GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION, 3))
				{
					bDom3 = true;
				}
			}
		}
	}

	// BBAI TODO: Current UU, up aggression?

	iHighUnitSpendingPercent /= std::max(1, getNumMembers());

	iTotalWarThreshold = iHighUnitSpendingPercent * (bAggressive ? 3 : 2);
	if( bDom3 )
	{
		iTotalWarThreshold *= 3;

		iDogpileWarThreshold += 5;
	}
	else if( bConq2 )
	{
		iTotalWarThreshold *= 2;

		iDogpileWarThreshold += 2;
	}
	iTotalWarThreshold /= 3;
	iTotalWarThreshold += bAggressive ? 1 : 0;

	if( bAggressive && GET_PLAYER(getLeaderID()).getCurrentEra() < 3 )
	{
		iLimitedWarThreshold += 2;
	}
}

// Returns odds of player declaring total war times 100
int CvTeamAI::AI_getTotalWarOddsTimes100( ) const
{
	int iTotalWarRand;
	int iLimitedWarRand;
	int iDogpileWarRand;
	AI_getWarRands( iTotalWarRand, iLimitedWarRand, iDogpileWarRand );

	int iTotalWarThreshold;
	int iLimitedWarThreshold;
	int iDogpileWarThreshold;
	AI_getWarThresholds( iTotalWarThreshold, iLimitedWarThreshold, iDogpileWarThreshold );

	return ((100 * 100 * iTotalWarThreshold) / std::max(1, iTotalWarRand));
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

int CvTeamAI::AI_makePeaceTradeVal(TeamTypes ePeaceTeam, TeamTypes eTeam) const
{
	int iModifier;
	int iValue;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(ePeaceTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(GET_TEAM(ePeaceTeam).isAlive(), "GET_TEAM(ePeaceTeam).isAlive is expected to be true");
	FAssertMsg(atWar(ePeaceTeam, eTeam), "eTeam should be at war with ePeaceTeam");

	iValue = (50 + GC.getGameINLINE().getGameTurn());
	iValue += ((GET_TEAM(eTeam).getNumCities() + GET_TEAM(ePeaceTeam).getNumCities()) * 8);

	iModifier = 0;

	switch ((GET_TEAM(eTeam).AI_getAttitude(ePeaceTeam) + GET_TEAM(ePeaceTeam).AI_getAttitude(eTeam)) / 2)
	{
	case ATTITUDE_FURIOUS:
		iModifier += 400;
		break;

	case ATTITUDE_ANNOYED:
		iModifier += 200;
		break;

	case ATTITUDE_CAUTIOUS:
		iModifier += 100;
		break;

	case ATTITUDE_PLEASED:
		iModifier += 50;
		break;

	case ATTITUDE_FRIENDLY:
		break;

	default:
		FAssert(false);
		break;
	}

	iValue *= std::max(0, (iModifier + 100));
	iValue /= 100;

	iValue *= 40;
	iValue /= (GET_TEAM(eTeam).AI_getAtWarCounter(ePeaceTeam) + 10);

	iValue -= (iValue % GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));

	if (isHuman())
	{
		return std::max(iValue, GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));
	}
	else
	{
		return iValue;
	}
}


DenialTypes CvTeamAI::AI_makePeaceTrade(TeamTypes ePeaceTeam, TeamTypes eTeam) const
{
	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(ePeaceTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(GET_TEAM(ePeaceTeam).isAlive(), "GET_TEAM(ePeaceTeam).isAlive is expected to be true");
	FAssertMsg(isAtWar(ePeaceTeam), "should be at war with ePeaceTeam");

	if (GET_TEAM(ePeaceTeam).isHuman())
	{
		return DENIAL_CONTACT_THEM;
	}

	if (GET_TEAM(ePeaceTeam).isAVassal())
	{
		return DENIAL_VASSAL;
	}

	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (!canChangeWarPeace(ePeaceTeam))
	{
		return DENIAL_VASSAL;
	}

	if (AI_endWarVal(ePeaceTeam) > (GET_TEAM(ePeaceTeam).AI_endWarVal(getID()) * 2))
	{
		return DENIAL_CONTACT_THEM;
	}

	int iLandRatio = ((getTotalLand(true) * 100) / std::max(20, GET_TEAM(eTeam).getTotalLand(true)));
	if (iLandRatio > 250)
	{
		return DENIAL_VICTORY;
	}

	return NO_DENIAL;
}


int CvTeamAI::AI_declareWarTradeVal(TeamTypes eWarTeam, TeamTypes eTeam) const
{
	PROFILE_FUNC();

	int iModifier;
	int iValue;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(eWarTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(GET_TEAM(eWarTeam).isAlive(), "GET_TEAM(eWarTeam).isAlive is expected to be true");
	FAssertMsg(!atWar(eWarTeam, eTeam), "eTeam should be at peace with eWarTeam");

	iValue = 0;
	iValue += (GET_TEAM(eWarTeam).getNumCities() * 10);
	iValue += (GET_TEAM(eWarTeam).getTotalPopulation(true) * 2);

	iModifier = 0;

	switch (GET_TEAM(eTeam).AI_getAttitude(eWarTeam))
	{
	case ATTITUDE_FURIOUS:
		break;

	case ATTITUDE_ANNOYED:
		iModifier += 25;
		break;

	case ATTITUDE_CAUTIOUS:
		iModifier += 50;
		break;

	case ATTITUDE_PLEASED:
		iModifier += 150;
		break;

	case ATTITUDE_FRIENDLY:
		iModifier += 400;
		break;

	default:
		FAssert(false);
		break;
	}

	iValue *= std::max(0, (iModifier + 100));
	iValue /= 100;

	int iTheirPower = GET_TEAM(eTeam).getPower(true);
	int iWarTeamPower = GET_TEAM(eWarTeam).getPower(true);

	iValue *= 50 + ((100 * iWarTeamPower) / (iTheirPower + iWarTeamPower + 1));
	iValue /= 100;

	if (!(GET_TEAM(eTeam).AI_isAllyLandTarget(eWarTeam)))
	{
		iValue *= 2;
	}

	if (!isAtWar(eWarTeam))
	{
		iValue *= 3;
	}
	else
	{
		iValue *= 150;
		iValue /= 100 + ((50 * std::min(100, (100 * AI_getWarSuccess(eWarTeam)) / (8 + getTotalPopulation(false)))) / 100);
	}

	iValue += (GET_TEAM(eTeam).getNumCities() * 20);
	iValue += (GET_TEAM(eTeam).getTotalPopulation(true) * 15);

	if (isAtWar(eWarTeam))
	{
		switch (GET_TEAM(eTeam).AI_getAttitude(getID()))
		{
		case ATTITUDE_FURIOUS:
		case ATTITUDE_ANNOYED:
		case ATTITUDE_CAUTIOUS:
			iValue *= 100;
			break;

		case ATTITUDE_PLEASED:
			iValue *= std::max(75, 100 - getAtWarCount(true) * 10);
			break;

		case ATTITUDE_FRIENDLY:
			iValue *= std::max(50, 100 - getAtWarCount(true) * 20);
			break;

		default:
			FAssert(false);
			break;
		}
		iValue /= 100;
	}

	iValue += GET_TEAM(eWarTeam).getNumNukeUnits() * 250;//Don't want to get nuked
	iValue += GET_TEAM(eTeam).getNumNukeUnits() * 150;//Don't want to use nukes on another's behalf

	if (GET_TEAM(eWarTeam).getAtWarCount(false) == 0)
	{
		iValue *= 2;

		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
			{
				if (iI != getID() && iI != eWarTeam && iI != eTeam)
				{
					if (GET_TEAM(eWarTeam).isDefensivePact((TeamTypes)iI))
					{
						iValue += (GET_TEAM((TeamTypes)iI).getNumCities() * 30);
						iValue += (GET_TEAM((TeamTypes)iI).getTotalPopulation(true) * 20);
					}
				}
			}
		}
	}

	iValue *= 60 + (140 * GC.getGameINLINE().getGameTurn()) / std::max(1, GC.getGameINLINE().getEstimateEndTurn());
	iValue /= 100;

	iValue -= (iValue % GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));

	if (isHuman())
	{
		return std::max(iValue, GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));
	}
	else
	{
		return iValue;
	}
}


DenialTypes CvTeamAI::AI_declareWarTrade(TeamTypes eWarTeam, TeamTypes eTeam, bool bConsiderPower) const
{
	PROFILE_FUNC();

	AttitudeTypes eAttitude;
	AttitudeTypes eAttitudeThem;
	bool bLandTarget;
	int iI;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(eWarTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(GET_TEAM(eWarTeam).isAlive(), "GET_TEAM(eWarTeam).isAlive is expected to be true");
	FAssertMsg(!isAtWar(eWarTeam), "should be at peace with eWarTeam");

	if (GET_TEAM(eWarTeam).isVassal(eTeam) || GET_TEAM(eWarTeam).isDefensivePact(eTeam))
	{
		return DENIAL_JOKING;
	}

	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (!canDeclareWar(eWarTeam))
	{
		return DENIAL_VASSAL;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/06/09                                jdog5000      */
/*                                                                                              */
/* Diplomacy                                                                                    */
/************************************************************************************************/
/* original BTS code
	if (getAnyWarPlanCount(true) > 0)
	{
		return DENIAL_TOO_MANY_WARS;
	}
*/
	// Hide WHEOOHRN revealing war plans
	if( getAtWarCount(true) > 0 )
	{
		return DENIAL_TOO_MANY_WARS;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (bConsiderPower)
	{
		bLandTarget = AI_isAllyLandTarget(eWarTeam);

		if ((GET_TEAM(eWarTeam).getDefensivePower() / ((bLandTarget) ? 2 : 1)) >
			(getPower(true) + ((atWar(eWarTeam, eTeam)) ? GET_TEAM(eTeam).getPower(true) : 0)))
		{
			if (bLandTarget)
			{
				return DENIAL_POWER_THEM;
			}
			else
			{
				return DENIAL_NO_GAIN;
			}
		}
	}

	eAttitude = AI_getAttitude(eTeam);

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				if (eAttitude <= GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getDeclareWarRefuseAttitudeThreshold())
				{
					return DENIAL_ATTITUDE;
				}
			}
		}
	}

	eAttitudeThem = AI_getAttitude(eWarTeam);

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				if (eAttitudeThem > GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getDeclareWarThemRefuseAttitudeThreshold())
				{
					return DENIAL_ATTITUDE_THEM;
				}
			}
		}
	}

	if (!atWar(eWarTeam, eTeam))
	{
		if (GET_TEAM(eWarTeam).getNumNukeUnits() > 0)
		{
			return DENIAL_JOKING;
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/06/09                                jdog5000      */
/*                                                                                              */
/* Diplomacy                                                                                    */
/************************************************************************************************/
	if (getAnyWarPlanCount(true) > 0)
	{
		return DENIAL_TOO_MANY_WARS;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	return NO_DENIAL;
}


int CvTeamAI::AI_openBordersTradeVal(TeamTypes eTeam) const
{
	return (getNumCities() + GET_TEAM(eTeam).getNumCities());
}


DenialTypes CvTeamAI::AI_openBordersTrade(TeamTypes eTeam) const
{
	PROFILE_FUNC();

	AttitudeTypes eAttitude;
	int iI;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (isVassal(eTeam))
	{
		return NO_DENIAL;
	}

	if (AI_shareWar(eTeam))
	{
		return NO_DENIAL;
	}

	if (AI_getMemoryCount(eTeam, MEMORY_CANCELLED_OPEN_BORDERS) > 0)
	{
		return DENIAL_RECENT_CANCEL;
	}

	if (AI_getWorstEnemy() == eTeam)
	{
		return DENIAL_WORST_ENEMY;
	}

	eAttitude = AI_getAttitude(eTeam);

/*************************************************************************************************/
/**	Victory AI								23/01/12									Snarko	**/
/**																								**/
/**						Making the AI go for different victories.								**/
/*************************************************************************************************/
	int iDenialAttitude;
/*************************************************************************************************/
/**	Victory AI END																				**/
/*************************************************************************************************/

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
/*************************************************************************************************/
/**	Victory AI								23/01/12									Snarko	**/
/**																								**/
/**						Making the AI go for different victories.								**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
				if (eAttitude <= GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getOpenBordersRefuseAttitudeThreshold())
/**								----  End Original Code  ----									**/
				iDenialAttitude = GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getOpenBordersRefuseAttitudeThreshold();
				if (GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_RELIGION, 2))
					iDenialAttitude = std::max((int)NO_ATTITUDE, (iDenialAttitude - 1));
				if (eAttitude <= iDenialAttitude)
/*************************************************************************************************/
/**	Victory AI END																				**/
/*************************************************************************************************/
				{
					return DENIAL_ATTITUDE;
				}
			}
		}
	}

	return NO_DENIAL;
}


int CvTeamAI::AI_defensivePactTradeVal(TeamTypes eTeam) const
{
	return ((getNumCities() + GET_TEAM(eTeam).getNumCities()) * 3);
}


DenialTypes CvTeamAI::AI_defensivePactTrade(TeamTypes eTeam) const
{
	PROFILE_FUNC();

	AttitudeTypes eAttitude;
	int iI;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (GC.getGameINLINE().countCivTeamsAlive() == 2)
	{
		return DENIAL_NO_GAIN;
	}

	if (AI_getWorstEnemy() == eTeam)
	{
		return DENIAL_WORST_ENEMY;
	}

	eAttitude = AI_getAttitude(eTeam);

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				if (eAttitude <= GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getDefensivePactRefuseAttitudeThreshold())
				{
					return DENIAL_ATTITUDE;
				}
			}
		}
	}

	return NO_DENIAL;
}


DenialTypes CvTeamAI::AI_permanentAllianceTrade(TeamTypes eTeam) const
{
	PROFILE_FUNC();

	AttitudeTypes eAttitude;
	int iI;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (AI_getWorstEnemy() == eTeam)
	{
		return DENIAL_WORST_ENEMY;
	}

	if ((getPower(true) + GET_TEAM(eTeam).getPower(true)) > (GC.getGameINLINE().countTotalCivPower() / 2))
	{
		if (getPower(true) > GET_TEAM(eTeam).getPower(true))
		{
			return DENIAL_POWER_US;
		}
		else
		{
			return DENIAL_POWER_YOU;
		}
	}

	if ((AI_getDefensivePactCounter(eTeam) + AI_getShareWarCounter(eTeam)) < 40)
	{
		return DENIAL_NOT_ALLIED;
	}

	eAttitude = AI_getAttitude(eTeam);

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				if (eAttitude <= GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getPermanentAllianceRefuseAttitudeThreshold())
				{
					return DENIAL_ATTITUDE;
				}
			}
		}
	}

	return NO_DENIAL;
}


TeamTypes CvTeamAI::AI_getWorstEnemy() const
{
	return m_eWorstEnemy;
}


void CvTeamAI::AI_updateWorstEnemy()
{
	PROFILE_FUNC();

	TeamTypes eBestTeam = NO_TEAM;
	int iBestValue = MAX_INT;

	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		TeamTypes eLoopTeam = (TeamTypes) iI;
		CvTeam& kLoopTeam = GET_TEAM(eLoopTeam);
		if (kLoopTeam.isAlive())
		{
			if (iI != getID() && !kLoopTeam.isVassal(getID()))
			{
				if (isHasMet(eLoopTeam))
				{
					if (AI_getAttitude(eLoopTeam) < ATTITUDE_CAUTIOUS)
					{
						int iValue = AI_getAttitudeVal(eLoopTeam);
						if (iValue < iBestValue)
						{
							iBestValue = iValue;
							eBestTeam = eLoopTeam;
						}
					}
				}
			}
		}
	}

	m_eWorstEnemy = eBestTeam;
}


int CvTeamAI::AI_getWarPlanStateCounter(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiWarPlanStateCounter[eIndex];
}


void CvTeamAI::AI_setWarPlanStateCounter(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiWarPlanStateCounter[eIndex] = iNewValue;
	FAssert(AI_getWarPlanStateCounter(eIndex) >= 0);
}


void CvTeamAI::AI_changeWarPlanStateCounter(TeamTypes eIndex, int iChange)
{
	AI_setWarPlanStateCounter(eIndex, (AI_getWarPlanStateCounter(eIndex) + iChange));
}


int CvTeamAI::AI_getAtWarCounter(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiAtWarCounter[eIndex];
}


void CvTeamAI::AI_setAtWarCounter(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiAtWarCounter[eIndex] = iNewValue;
	FAssert(AI_getAtWarCounter(eIndex) >= 0);
}


void CvTeamAI::AI_changeAtWarCounter(TeamTypes eIndex, int iChange)
{
	AI_setAtWarCounter(eIndex, (AI_getAtWarCounter(eIndex) + iChange));
}


int CvTeamAI::AI_getAtPeaceCounter(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiAtPeaceCounter[eIndex];
}


void CvTeamAI::AI_setAtPeaceCounter(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiAtPeaceCounter[eIndex] = iNewValue;
	FAssert(AI_getAtPeaceCounter(eIndex) >= 0);
}


void CvTeamAI::AI_changeAtPeaceCounter(TeamTypes eIndex, int iChange)
{
	AI_setAtPeaceCounter(eIndex, (AI_getAtPeaceCounter(eIndex) + iChange));
}


int CvTeamAI::AI_getHasMetCounter(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiHasMetCounter[eIndex];
}


void CvTeamAI::AI_setHasMetCounter(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiHasMetCounter[eIndex] = iNewValue;
	FAssert(AI_getHasMetCounter(eIndex) >= 0);
}


void CvTeamAI::AI_changeHasMetCounter(TeamTypes eIndex, int iChange)
{
	AI_setHasMetCounter(eIndex, (AI_getHasMetCounter(eIndex) + iChange));
}


int CvTeamAI::AI_getOpenBordersCounter(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiOpenBordersCounter[eIndex];
}


void CvTeamAI::AI_setOpenBordersCounter(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiOpenBordersCounter[eIndex] = iNewValue;
	FAssert(AI_getOpenBordersCounter(eIndex) >= 0);
}


void CvTeamAI::AI_changeOpenBordersCounter(TeamTypes eIndex, int iChange)
{
	AI_setOpenBordersCounter(eIndex, (AI_getOpenBordersCounter(eIndex) + iChange));
}


int CvTeamAI::AI_getDefensivePactCounter(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiDefensivePactCounter[eIndex];
}


void CvTeamAI::AI_setDefensivePactCounter(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiDefensivePactCounter[eIndex] = iNewValue;
	FAssert(AI_getDefensivePactCounter(eIndex) >= 0);
}


void CvTeamAI::AI_changeDefensivePactCounter(TeamTypes eIndex, int iChange)
{
	AI_setDefensivePactCounter(eIndex, (AI_getDefensivePactCounter(eIndex) + iChange));
}


int CvTeamAI::AI_getShareWarCounter(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiShareWarCounter[eIndex];
}


void CvTeamAI::AI_setShareWarCounter(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiShareWarCounter[eIndex] = iNewValue;
	FAssert(AI_getShareWarCounter(eIndex) >= 0);
}


void CvTeamAI::AI_changeShareWarCounter(TeamTypes eIndex, int iChange)
{
	AI_setShareWarCounter(eIndex, (AI_getShareWarCounter(eIndex) + iChange));
}


int CvTeamAI::AI_getWarSuccess(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiWarSuccess[eIndex];
}


void CvTeamAI::AI_setWarSuccess(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/03/09                       poyuzhe & jdog5000     */
/*                                                                                              */
/* Efficiency                                                                                   */
/************************************************************************************************/
	// From Sanguo Mod Performance, ie the CAR Mod
	// Attitude cache
	if (m_aiWarSuccess[eIndex] != iNewValue)
	{
		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if( GET_PLAYER((PlayerTypes)iI).isAlive() )
			{
				if( GET_PLAYER((PlayerTypes)iI).getTeam() == getID() || GET_PLAYER((PlayerTypes)iI).getTeam() == eIndex )
				{
					for (int iJ = 0; iJ < MAX_PLAYERS; iJ++)
					{
						if( GET_PLAYER((PlayerTypes)iJ).isAlive() && GET_PLAYER((PlayerTypes)iJ).getTeam() != GET_PLAYER((PlayerTypes)iI).getTeam() )
						{
							if( GET_PLAYER((PlayerTypes)iJ).getTeam() == getID() || GET_PLAYER((PlayerTypes)iJ).getTeam() == eIndex )
							{
								GET_PLAYER((PlayerTypes)iJ).AI_invalidateAttitudeCache((PlayerTypes)iI);
								GET_PLAYER((PlayerTypes)iI).AI_invalidateAttitudeCache((PlayerTypes)iJ);
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
	m_aiWarSuccess[eIndex] = iNewValue;
	FAssert(AI_getWarSuccess(eIndex) >= 0);
}


void CvTeamAI::AI_changeWarSuccess(TeamTypes eIndex, int iChange)
{
	AI_setWarSuccess(eIndex, (AI_getWarSuccess(eIndex) + iChange));
}


int CvTeamAI::AI_getEnemyPeacetimeTradeValue(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiEnemyPeacetimeTradeValue[eIndex];
}


void CvTeamAI::AI_setEnemyPeacetimeTradeValue(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiEnemyPeacetimeTradeValue[eIndex] = iNewValue;
	FAssert(AI_getEnemyPeacetimeTradeValue(eIndex) >= 0);
}


void CvTeamAI::AI_changeEnemyPeacetimeTradeValue(TeamTypes eIndex, int iChange)
{
	AI_setEnemyPeacetimeTradeValue(eIndex, (AI_getEnemyPeacetimeTradeValue(eIndex) + iChange));
}


int CvTeamAI::AI_getEnemyPeacetimeGrantValue(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiEnemyPeacetimeGrantValue[eIndex];
}


void CvTeamAI::AI_setEnemyPeacetimeGrantValue(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiEnemyPeacetimeGrantValue[eIndex] = iNewValue;
	FAssert(AI_getEnemyPeacetimeGrantValue(eIndex) >= 0);
}


void CvTeamAI::AI_changeEnemyPeacetimeGrantValue(TeamTypes eIndex, int iChange)
{
	AI_setEnemyPeacetimeGrantValue(eIndex, (AI_getEnemyPeacetimeGrantValue(eIndex) + iChange));
}


WarPlanTypes CvTeamAI::AI_getWarPlan(TeamTypes eIndex) const
{
	FAssert(eIndex >= 0);
	FAssert(eIndex < MAX_TEAMS);
	FAssert(eIndex != getID() || m_aeWarPlan[eIndex] == NO_WARPLAN);
	return m_aeWarPlan[eIndex];
}


bool CvTeamAI::AI_isChosenWar(TeamTypes eIndex) const
{
	switch (AI_getWarPlan(eIndex))
	{
	case WARPLAN_ATTACKED_RECENT:
	case WARPLAN_ATTACKED:
		return false;
		break;
	case WARPLAN_PREPARING_LIMITED:
	case WARPLAN_PREPARING_TOTAL:
	case WARPLAN_LIMITED:
	case WARPLAN_TOTAL:
	case WARPLAN_DOGPILE:
		return true;
		break;
	}

	return false;
}


bool CvTeamAI::AI_isSneakAttackPreparing(TeamTypes eIndex) const
{
	return ((AI_getWarPlan(eIndex) == WARPLAN_PREPARING_LIMITED) || (AI_getWarPlan(eIndex) == WARPLAN_PREPARING_TOTAL));
}


bool CvTeamAI::AI_isSneakAttackReady(TeamTypes eIndex) const
{
	return (AI_isChosenWar(eIndex) && !(AI_isSneakAttackPreparing(eIndex)));
}

/*************************************************************************************************/
/**	K-mod merger								16/02/12								Snarko	**/
/**																								**/
/**					Merging in features of K-mod, most notably the pathfinder					**/
/*************************************************************************************************/
// K-Mod
bool CvTeamAI::AI_isSneakAttackReady() const
{
	for (int i = 0; i < MAX_CIV_TEAMS; i++)
	{
		if (AI_isSneakAttackReady((TeamTypes)i))
			return true;
	}
	return false;
}
// K-Mod end
/*************************************************************************************************/
/**	K-mod merger								END												**/
/*************************************************************************************************/

void CvTeamAI::AI_setWarPlan(TeamTypes eIndex, WarPlanTypes eNewValue, bool bWar)
{
	int iI;

	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (AI_getWarPlan(eIndex) != eNewValue)
	{
		if (bWar || !isAtWar(eIndex))
		{
			m_aeWarPlan[eIndex] = eNewValue;

			AI_setWarPlanStateCounter(eIndex, 0);

			AI_updateAreaStragies();

			for (iI = 0; iI < MAX_PLAYERS; iI++)
			{
				if (GET_PLAYER((PlayerTypes)iI).isAlive())
				{
					if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
					{
						if (!(GET_PLAYER((PlayerTypes)iI).isHuman()))
						{
							GET_PLAYER((PlayerTypes)iI).AI_makeProductionDirty();
						}
					}
				}
			}
		}
	}
}

//if this number is over 0 the teams are "close"
//this may be expensive to run, kinda O(N^2)...
int CvTeamAI::AI_teamCloseness(TeamTypes eIndex, int iMaxDistance) const
{
	PROFILE_FUNC();
	int iI, iJ;

	if (iMaxDistance == -1)
	{
		iMaxDistance = DEFAULT_PLAYER_CLOSENESS;
	}

	FAssert(eIndex != getID());
	int iValue = 0;
	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				for (iJ = 0; iJ < MAX_PLAYERS; iJ++)
				{
					if (GET_PLAYER((PlayerTypes)iJ).isAlive())
					{
						if (GET_PLAYER((PlayerTypes)iJ).getTeam() == eIndex)
						{
							iValue += GET_PLAYER((PlayerTypes)iI).AI_playerCloseness((PlayerTypes)iJ, iMaxDistance);
						}
					}
				}
			}
		}
	}

	return iValue;
}


void CvTeamAI::read(FDataStreamBase* pStream)
{
	CvTeam::read(pStream);

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	pStream->Read(MAX_TEAMS, m_aiWarPlanStateCounter);
	pStream->Read(MAX_TEAMS, m_aiAtWarCounter);
	pStream->Read(MAX_TEAMS, m_aiAtPeaceCounter);
	pStream->Read(MAX_TEAMS, m_aiHasMetCounter);
	pStream->Read(MAX_TEAMS, m_aiOpenBordersCounter);
	pStream->Read(MAX_TEAMS, m_aiDefensivePactCounter);
	pStream->Read(MAX_TEAMS, m_aiShareWarCounter);
	pStream->Read(MAX_TEAMS, m_aiWarSuccess);
	pStream->Read(MAX_TEAMS, m_aiEnemyPeacetimeTradeValue);
	pStream->Read(MAX_TEAMS, m_aiEnemyPeacetimeGrantValue);

	pStream->Read(MAX_TEAMS, (int*)m_aeWarPlan);
	pStream->Read((int*)&m_eWorstEnemy);
/*************************************************************************************************/
/**	MultiBarb								12/23/08								Xienwolf	**/
/**																								**/
/**				Workaround required until source of this call for Barbarians is located			**/
/*************************************************************************************************/
	if (getID() == ORC_TEAM)
	{
		GET_TEAM(ANIMAL_TEAM).read(pStream);
		GET_TEAM(DEMON_TEAM).read(pStream);
	}
/*************************************************************************************************/
/**	MultiBarb								END													**/
/*************************************************************************************************/
}


void CvTeamAI::write(FDataStreamBase* pStream)
{
	CvTeam::write(pStream);

	uint uiFlag=0;
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(MAX_TEAMS, m_aiWarPlanStateCounter);
	pStream->Write(MAX_TEAMS, m_aiAtWarCounter);
	pStream->Write(MAX_TEAMS, m_aiAtPeaceCounter);
	pStream->Write(MAX_TEAMS, m_aiHasMetCounter);
	pStream->Write(MAX_TEAMS, m_aiOpenBordersCounter);
	pStream->Write(MAX_TEAMS, m_aiDefensivePactCounter);
	pStream->Write(MAX_TEAMS, m_aiShareWarCounter);
	pStream->Write(MAX_TEAMS, m_aiWarSuccess);
	pStream->Write(MAX_TEAMS, m_aiEnemyPeacetimeTradeValue);
	pStream->Write(MAX_TEAMS, m_aiEnemyPeacetimeGrantValue);

	pStream->Write(MAX_TEAMS, (int*)m_aeWarPlan);
	pStream->Write(m_eWorstEnemy);
/*************************************************************************************************/
/**	MultiBarb								12/23/08								Xienwolf	**/
/**																								**/
/**				Workaround required until source of this call for Barbarians is located			**/
/*************************************************************************************************/
	if (getID() == ORC_TEAM)
	{
		GET_TEAM(ANIMAL_TEAM).write(pStream);
		GET_TEAM(DEMON_TEAM).write(pStream);
	}
/*************************************************************************************************/
/**	MultiBarb								END													**/
/*************************************************************************************************/
}

// Protected Functions...

int CvTeamAI::AI_noTechTradeThreshold() const
{
	int iRand;
	int iCount;
	int iI;

	iRand = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getNoTechTradeThreshold();
				iCount++;
			}
		}
	}

	if (iCount > 0)
	{
		iRand /= iCount;
	}

	return iRand;
}


int CvTeamAI::AI_techTradeKnownPercent() const
{
	int iRand;
	int iCount;
	int iI;

	iRand = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getTechTradeKnownPercent();
				iCount++;
			}
		}
	}

	if (iCount > 0)
	{
		iRand /= iCount;
	}

	return iRand;
}


int CvTeamAI::AI_maxWarRand() const
{
	int iRand;
	int iCount;
	int iI;

	iRand = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getMaxWarRand();
				iCount++;
			}
		}
	}

	if (iCount > 0)
	{
		iRand /= iCount;
	}

	return iRand;
}


int CvTeamAI::AI_maxWarNearbyPowerRatio() const
{
	int iRand;
	int iCount;
	int iI;

	iRand = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getMaxWarNearbyPowerRatio();
				iCount++;
			}
		}
	}

	if (iCount > 1)
	{
		iRand /= iCount;
	}

	return iRand;
}


int CvTeamAI::AI_maxWarDistantPowerRatio() const
{
	int iRand;
	int iCount;
	int iI;

	iRand = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getMaxWarDistantPowerRatio();
				iCount++;
			}
		}
	}

	if (iCount > 1)
	{
		iRand /= iCount;
	}

	return iRand;
}


int CvTeamAI::AI_maxWarMinAdjacentLandPercent() const
{
	int iRand;
	int iCount;
	int iI;

	iRand = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getMaxWarMinAdjacentLandPercent();
				iCount++;
			}
		}
	}

	if (iCount > 0)
	{
		iRand /= iCount;
	}

	return iRand;
}


int CvTeamAI::AI_limitedWarRand() const
{
	int iRand;
	int iCount;
	int iI;

	iRand = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getLimitedWarRand();
				iCount++;
			}
		}
	}

	if (iCount > 0)
	{
		iRand /= iCount;
	}

	return iRand;
}


int CvTeamAI::AI_limitedWarPowerRatio() const
{
	int iRand;
	int iCount;
	int iI;

	iRand = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getLimitedWarPowerRatio();
				iCount++;
			}
		}
	}

	if (iCount > 0)
	{
		iRand /= iCount;
	}

	return iRand;
}


int CvTeamAI::AI_dogpileWarRand() const
{
	int iRand;
	int iCount;
	int iI;

	iRand = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getDogpileWarRand();
				iCount++;
			}
		}
	}

	if (iCount > 0)
	{
		iRand /= iCount;
	}

	return iRand;
}


int CvTeamAI::AI_makePeaceRand() const
{
	int iRand;
	int iCount;
	int iI;

	iRand = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getMakePeaceRand();
				iCount++;
			}
		}
	}

	if (iCount > 0)
	{
		iRand /= iCount;
	}

	return iRand;
}


int CvTeamAI::AI_noWarAttitudeProb(AttitudeTypes eAttitude) const
{
	int iProb;
	int iCount;
	int iI;

	iProb = 0;
	iCount = 0;

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/20/10                                jdog5000      */
/*                                                                                              */
/* War Strategy AI                                                                              */
/************************************************************************************************/
	int iVictoryStrategyAdjust = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iProb += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getNoWarAttitudeProb(eAttitude);
				iCount++;

				// In final stages of miltaristic victory, AI may turn on its friends!
				if( GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST, 4) )
				{
					iVictoryStrategyAdjust += 30;
				}
				else if( GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION, 4) )
				{
					iVictoryStrategyAdjust += 20;
				}
			}
		}
	}

	if (iCount > 1)
	{
		iProb /= iCount;
		iVictoryStrategyAdjust /= iCount;
	}

	iProb = std::max( 0, iProb - iVictoryStrategyAdjust );
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	return iProb;
}


void CvTeamAI::AI_doCounter()
{
	int iI;

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (iI != getID())
			{
				AI_changeWarPlanStateCounter(((TeamTypes)iI), 1);

				if (isAtWar((TeamTypes)iI))
				{
					AI_changeAtWarCounter(((TeamTypes)iI), 1);
				}
				else
				{
/*************************************************************************************************/
/** LeadersEnhanced DiplomacySettings       Opera       01.06.09                                **/
/*************************************************************************************************/
/** Increases the AtPeaceCounter only if the civ has met the target civ. I think it is silly that
/** you can get the +AtPeace points even though you just met a civ at turn 300.
/*************************************************************************************************/
/** -- Start Original Code --                                                                   **
					AI_changeAtPeaceCounter(((TeamTypes)iI), 1);
/** -- End Original Code --                                                                     **/
/*************************************************************************************************/
					if (isHasMet((TeamTypes)iI))
					{
						AI_changeAtPeaceCounter(((TeamTypes)iI), 1);
					}
/*************************************************************************************************/
/** End                                                                                         **/
/*************************************************************************************************/
				}

				if (isHasMet((TeamTypes)iI))
				{
					AI_changeHasMetCounter(((TeamTypes)iI), 1);
				}

				if (isOpenBorders((TeamTypes)iI))
				{
					AI_changeOpenBordersCounter(((TeamTypes)iI), 1);
				}

				if (isDefensivePact((TeamTypes)iI))
				{
					AI_changeDefensivePactCounter(((TeamTypes)iI), 1);
				}
				else
				{
					if (AI_getDefensivePactCounter((TeamTypes)iI) > 0)
					{
						AI_changeDefensivePactCounter(((TeamTypes)iI), -1);
					}
				}

				if (isHasMet((TeamTypes)iI))
				{
					if (AI_shareWar((TeamTypes)iI))
					{
						AI_changeShareWarCounter(((TeamTypes)iI), 1);
					}
				}
			}
		}
	}
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/26/10                                jdog5000      */
/*                                                                                              */
/* War Strategy AI                                                                              */
/************************************************************************************************/
// Block AI from declaring war on a distant vassal if it shares an area with the master
bool CvTeamAI::AI_isOkayVassalTarget( TeamTypes eTeam )
{
	if( GET_TEAM(eTeam).isAVassal() )
	{
		if( !(AI_hasCitiesInPrimaryArea(eTeam)) || AI_calculateAdjacentLandPlots(eTeam) == 0 )
		{
			for( int iI = 0; iI < MAX_CIV_TEAMS; iI++ )
			{
				if( GET_TEAM(eTeam).isVassal((TeamTypes)iI) )
				{
					if( AI_hasCitiesInPrimaryArea((TeamTypes)iI) && AI_calculateAdjacentLandPlots((TeamTypes)iI) > 0)
					{
						return false;
					}
				}
			}
		}
	}

	return true;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/25/10                                jdog5000      */
/*                                                                                              */
/* War Strategy, AI logging                                                                     */
/************************************************************************************************/
/// \brief Make war decisions, mainly for starting or switching war plans.
///
///
void CvTeamAI::AI_doWar()
{
	PROFILE_FUNC();

	CvArea* pLoopArea;
	TeamTypes eBestTeam;
	bool bAreaValid;
	bool bShareValid;
	int iNoWarRoll;
	int iOurPower;
	int iDogpilePower;
	int iValue;
	int iBestValue;
	int iPass;
	int iLoop;
	int iI, iJ;

	FAssert(!isHuman());
	FAssert(!isBarbarian());
	FAssert(!isMinorCiv());

	if (isAVassal())
	{
		return;
	}

	// allow python to handle it
	CyArgsList argsList;
	argsList.add(getID());
	long lResult=0;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "AI_doWar", argsList.makeFunctionArgs(), &lResult);
	if (lResult == 1)
	{
		return;
	}

	int iEnemyPowerPercent = AI_getEnemyPowerPercent();

	for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive() && isHasMet((TeamTypes)iI))
		{
			if (AI_getWarPlan((TeamTypes)iI) != NO_WARPLAN)
			{
				int iTimeModifier = 100;
				{
					int iThreshold = (80*AI_maxWarNearbyPowerRatio())/100;

					if( iEnemyPowerPercent < iThreshold )
					{
						iTimeModifier *= iEnemyPowerPercent;
						iTimeModifier /= iThreshold;
					}

					iTimeModifier *= 50 + GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getTrainPercent();
					iTimeModifier /= 150;

					FAssert(iTimeModifier >= 0);
				}

				int iAbandonTimeModifier = 100;
				iAbandonTimeModifier *= 50 + GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getTrainPercent();
				iAbandonTimeModifier /= 150;

				bool bEnemyVictoryLevel4 = GET_TEAM((TeamTypes)iI).AI_isAnyMemberDoVictoryStrategyLevel4();

				if (AI_getWarPlan((TeamTypes)iI) == WARPLAN_ATTACKED_RECENT)
				{
					FAssert(isAtWar((TeamTypes)iI));

					if (AI_getAtWarCounter((TeamTypes)iI) > ((GET_TEAM((TeamTypes)iI).AI_isLandTarget(getID())) ? 9 : 3))
					{
						AI_setWarPlan(((TeamTypes)iI), WARPLAN_ATTACKED);
					}
				}
				else if (AI_getWarPlan((TeamTypes)iI) == WARPLAN_PREPARING_LIMITED)
				{
					FAssert(canEventuallyDeclareWar((TeamTypes)iI));

					if (AI_getWarPlanStateCounter((TeamTypes)iI) > ((5 * iTimeModifier) / (bEnemyVictoryLevel4 ? 400 : 100)))
					{
						AI_setWarPlan(((TeamTypes)iI), WARPLAN_LIMITED);
					}
				}
				else if (AI_getWarPlan((TeamTypes)iI) == WARPLAN_LIMITED || AI_getWarPlan((TeamTypes)iI) == WARPLAN_DOGPILE)
				{
					if( !isAtWar((TeamTypes)iI) )
					{
						FAssert(canEventuallyDeclareWar((TeamTypes)iI));

						bool bActive = false;
						for( int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++ )
						{
							if( GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID() )
							{
								if( GET_PLAYER((PlayerTypes)iPlayer).AI_enemyTargetMissions((TeamTypes)iI) > 0 )
								{
									bActive = true;
									break;
								}
							}
						}

						if( !bActive )
						{
							if (AI_getWarPlanStateCounter((TeamTypes)iI) > ((15 * iAbandonTimeModifier) / (100)))
							{
								AI_setWarPlan(((TeamTypes)iI), NO_WARPLAN);
							}
						}

						if( AI_getWarPlan((TeamTypes)iI) == WARPLAN_DOGPILE )
						{
							if( GET_TEAM((TeamTypes)iI).getAtWarCount(true) == 0 )
							{
								AI_setWarPlan(((TeamTypes)iI), NO_WARPLAN);
							}
						}
					}
				}
				else if (AI_getWarPlan((TeamTypes)iI) == WARPLAN_PREPARING_TOTAL)
				{
					FAssert(canEventuallyDeclareWar((TeamTypes)iI));

					if (AI_getWarPlanStateCounter((TeamTypes)iI) > ((10 * iTimeModifier) / (bEnemyVictoryLevel4 ? 400 : 100)))
					{
						bAreaValid = false;
						bShareValid = false;

						for(pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
						{
							if (AI_isPrimaryArea(pLoopArea))
							{
								if (GET_TEAM((TeamTypes)iI).countNumCitiesByArea(pLoopArea) > 0)
								{
									bShareValid = true;

									AreaAITypes eAreaAI = AI_calculateAreaAIType(pLoopArea, true);

									if ( eAreaAI == AREAAI_DEFENSIVE)
									{
										bAreaValid = false;
									}
									else if( eAreaAI == AREAAI_OFFENSIVE )
									{
										bAreaValid = true;
									}
								}
							}
						}

						if ( (bAreaValid && (iEnemyPowerPercent < 140)) || (!bShareValid && (iEnemyPowerPercent < 110)) || (GET_TEAM((TeamTypes)iI).AI_getLowestVictoryCountdown() >= 0) )
						{
							AI_setWarPlan(((TeamTypes)iI), WARPLAN_TOTAL);
						}
						else if (AI_getWarPlanStateCounter((TeamTypes)iI) > ((20 * iAbandonTimeModifier) / 100))
						{
							AI_setWarPlan(((TeamTypes)iI), NO_WARPLAN);
						}
					}
				}
				else if (AI_getWarPlan((TeamTypes)iI) == WARPLAN_TOTAL)
				{
					if( !isAtWar((TeamTypes)iI) )
					{
						FAssert(canEventuallyDeclareWar((TeamTypes)iI));

						bool bActive = false;
						for( int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++ )
						{
							if( GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID() )
							{
								if( GET_PLAYER((PlayerTypes)iPlayer).AI_enemyTargetMissions((TeamTypes)iI) > 0 )
								{
									bActive = true;
									break;
								}
							}
						}

						if( !bActive )
						{
							if (AI_getWarPlanStateCounter((TeamTypes)iI) > ((25 * iAbandonTimeModifier) / (100)))
							{
								AI_setWarPlan(((TeamTypes)iI), NO_WARPLAN);
							}
						}
					}
				}
			}
		}
	}

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				GET_PLAYER((PlayerTypes)iI).AI_doPeace();
			}
		}
	}

	int iNumMembers = getNumMembers();
	int iHighUnitSpendingPercent = 0;
	int iLowUnitSpendingPercent = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				int iUnitSpendingPercent = (GET_PLAYER((PlayerTypes)iI).calculateUnitCost() * 100) / std::max(1, GET_PLAYER((PlayerTypes)iI).calculatePreInflatedCosts());
				iHighUnitSpendingPercent += (std::max(0, iUnitSpendingPercent - 7) / 2);
				iLowUnitSpendingPercent += iUnitSpendingPercent;
			}
		}
	}


	// if at war, check for making peace
	if (getAtWarCount(true) > 0) // XXX
	{
		if (GC.getGameINLINE().getSorenRandNum(AI_makePeaceRand(), "AI Make Peace") == 0)
		{
			for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
			{
				if (GET_TEAM((TeamTypes)iI).isAlive())
				{
					if (iI != getID())
					{
						if (!(GET_TEAM((TeamTypes)iI).isHuman()))
						{
							if (canContact((TeamTypes)iI))
							{
								FAssert(!(GET_TEAM((TeamTypes)iI).isMinorCiv()));

								if (isAtWar((TeamTypes)iI))
								{
									if (AI_isChosenWar((TeamTypes)iI))
									{
										if( AI_getAtWarCounter((TeamTypes)iI) > std::max(10, (14 * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getVictoryDelayPercent())/100) )
										{
											// If nothing is happening in war
											if( AI_getWarSuccess((TeamTypes)iI) + GET_TEAM((TeamTypes)iI).AI_getWarSuccess(getID()) < 2*GC.getDefineINT("WAR_SUCCESS_ATTACKING") )
											{
												if( (GC.getGameINLINE().getSorenRandNum(8, "AI Make Peace 1") == 0) )
												{
													bool bValid = true;

													for( int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++ )
													{
														if( GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID() )
														{
															if( GET_PLAYER((PlayerTypes)iPlayer).AI_enemyTargetMissions((TeamTypes)iI) > 0 )
															{
																bValid = false;
																break;
															}
														}

														if( GET_PLAYER((PlayerTypes)iPlayer).getTeam() == iI )
														{
															MissionAITypes eMissionAI = MISSIONAI_ASSAULT;
															if( GET_PLAYER((PlayerTypes)iPlayer).AI_enemyTargetMissions(getID()) > 0 )
															{
																bValid = false;
																break;
															}
														}
													}

													if( bValid )
													{
														makePeace((TeamTypes)iI);

														break;
													}
												}
											}

											// Fought to a long draw
											if (AI_getAtWarCounter((TeamTypes)iI) > ((((AI_getWarPlan((TeamTypes)iI) == WARPLAN_TOTAL) ? 40 : 30) * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getVictoryDelayPercent())/100) )
											{
												int iOurValue = AI_endWarVal((TeamTypes)iI);
												int iTheirValue = GET_TEAM((TeamTypes)iI).AI_endWarVal(getID());
												if ((iOurValue > (iTheirValue / 2)) && (iTheirValue > (iOurValue / 2)))
												{
													makePeace((TeamTypes)iI);
													break;
												}
											}

											// All alone in what we thought was a dogpile
											if (AI_getWarPlan((TeamTypes)iI) == WARPLAN_DOGPILE)
											{
												if (GET_TEAM((TeamTypes)iI).getAtWarCount(true) == 1)
												{
													int iOurValue = AI_endWarVal((TeamTypes)iI);
													int iTheirValue = GET_TEAM((TeamTypes)iI).AI_endWarVal(getID());
													if ((iTheirValue > (iOurValue / 2)))
													{
														makePeace((TeamTypes)iI);
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
		}
	}

	// if no war plans, consider starting one!
	if (getAnyWarPlanCount(true) == 0 || iEnemyPowerPercent < 45)
	{
		bool bAggressive = GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI);

		int iFinancialTroubleCount = 0;
		int iDaggerCount = 0;
		int iGetBetterUnitsCount = 0;
		for (iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
				{
					if ( GET_PLAYER((PlayerTypes)iI).AI_isDoStrategy(AI_STRATEGY_DAGGER)
						|| GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST, 3)
						|| GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION, 4) )
					{
						iDaggerCount++;
						bAggressive = true;
					}

					if (GET_PLAYER((PlayerTypes)iI).AI_isDoStrategy(AI_STRATEGY_GET_BETTER_UNITS))
					{
						iGetBetterUnitsCount++;
					}

					if (GET_PLAYER((PlayerTypes)iI).AI_isFinancialTrouble())
					{
						iFinancialTroubleCount++;
					}
				}
			}
		}

		// if random in this range is 0, we go to war of this type (so lower numbers are higher probablity)
		// average of everyone on our team
		int iTotalWarRand;
		int iLimitedWarRand;
		int iDogpileWarRand;
		AI_getWarRands( iTotalWarRand, iLimitedWarRand, iDogpileWarRand );

		int iTotalWarThreshold;
		int iLimitedWarThreshold;
		int iDogpileWarThreshold;
		AI_getWarThresholds( iTotalWarThreshold, iLimitedWarThreshold, iDogpileWarThreshold );

		// we oppose war if half the non-dagger teammates in financial trouble
		bool bFinancesOpposeWar = false;
		if ((iFinancialTroubleCount - iDaggerCount) >= std::max(1, getNumMembers() / 2 ))
		{
			// this can be overridden by by the pro-war booleans
			bFinancesOpposeWar = true;
		}

		// if agressive, we may start a war to get money
		bool bFinancesProTotalWar = false;
		bool bFinancesProLimitedWar = false;
		bool bFinancesProDogpileWar = false;
		if (iFinancialTroubleCount > 0)
		{
			// do we like all out wars?
			if (iDaggerCount > 0 || iTotalWarRand < 100)
			{
				bFinancesProTotalWar = true;
			}

			// do we like limited wars?
			if (iLimitedWarRand < 100)
			{
				bFinancesProLimitedWar = true;
			}

			// do we like dogpile wars?
			if (iDogpileWarRand < 100)
			{
				bFinancesProDogpileWar = true;
			}
		}
		bool bFinancialProWar = (bFinancesProTotalWar || bFinancesProLimitedWar || bFinancesProDogpileWar);

		// overall war check (quite frequently true)
		bool bMakeWarChecks = false;
		if ((iGetBetterUnitsCount - iDaggerCount) * 3 < iNumMembers * 2)
		{
			if (bFinancialProWar || !bFinancesOpposeWar)
			{
				// random overall war chance (at noble+ difficulties this is 100%)
				if (GC.getGameINLINE().getSorenRandNum(100, "AI Declare War 1") < GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getAIDeclareWarProb())
				{
					bMakeWarChecks = true;
				}
			}
		}

		if (bMakeWarChecks)
		{
			iOurPower = getPower(true);

			if (bAggressive && (getAnyWarPlanCount(true) == 0))
			{
				iOurPower *= 4;
				iOurPower /= 3;
			}

			iOurPower *= (100 - iEnemyPowerPercent);
			iOurPower /= 100;

			if ((bFinancesProTotalWar || !bFinancesOpposeWar) &&
				(GC.getGameINLINE().getSorenRandNum(iTotalWarRand, "AI Maximum War") <= iTotalWarThreshold))
			{
				iNoWarRoll = GC.getGameINLINE().getSorenRandNum(100, "AI No War");
				iNoWarRoll = range(iNoWarRoll + (bAggressive ? 10 : 0) + (bFinancesProTotalWar ? 10 : 0) - (20*iGetBetterUnitsCount)/iNumMembers, 0, 99);

				iBestValue = 0;
				eBestTeam = NO_TEAM;

				for (iPass = 0; iPass < 3; iPass++)
				{
					for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
					{
						if (GET_TEAM((TeamTypes)iI).isAlive())
						{
							if (iI != getID())
							{
								if (isHasMet((TeamTypes)iI))
								{
									if (canEventuallyDeclareWar((TeamTypes)iI))
									{
										if( GET_TEAM((TeamTypes)iI).isAVassal() && !AI_isOkayVassalTarget((TeamTypes)iI) )
										{
											continue;
										}

										if (iNoWarRoll >= AI_noWarAttitudeProb(AI_getAttitude((TeamTypes)iI)))
										{
											int iDefensivePower = (GET_TEAM((TeamTypes)iI).getDefensivePower() * 2) / 3;

											if (iDefensivePower < ((iOurPower * ((iPass > 1) ? AI_maxWarDistantPowerRatio() : AI_maxWarNearbyPowerRatio())) / 100))
											{
												// XXX make sure they share an area....

												FAssertMsg(!(GET_TEAM((TeamTypes)iI).isBarbarian()), "Expected to not be declaring war on the barb civ");
												FAssertMsg(iI != getID(), "Expected not to be declaring war on self (DOH!)");

												if ((iPass > 1) || AI_isLandTarget((TeamTypes)iI) || AI_isAnyCapitalAreaAlone() || GET_TEAM((TeamTypes)iI).AI_isAnyMemberDoVictoryStrategyLevel4())
												{
													if ((iPass > 0) || (AI_calculateAdjacentLandPlots((TeamTypes)iI) >= ((getTotalLand() * AI_maxWarMinAdjacentLandPercent()) / 100)) || GET_TEAM((TeamTypes)iI).AI_isAnyMemberDoVictoryStrategyLevel4())
													{
														iValue = AI_startWarVal((TeamTypes)iI);

														if (iValue > iBestValue)
														{
															iBestValue = iValue;
															eBestTeam = ((TeamTypes)iI);
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

					if (eBestTeam != NO_TEAM)
					{
						AI_setWarPlan(eBestTeam, (iDaggerCount > 0) ? WARPLAN_TOTAL : WARPLAN_PREPARING_TOTAL);
						break;
					}
				}
			}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       01/02/09                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
			else if ((bFinancesProLimitedWar || !bFinancesOpposeWar) &&
				(GC.getGameINLINE().getSorenRandNum(iLimitedWarRand, "AI Limited War") <= iLimitedWarThreshold))
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
			{
				iNoWarRoll = GC.getGameINLINE().getSorenRandNum(100, "AI No War") - 10;
				iNoWarRoll = range(iNoWarRoll + (bAggressive ? 10 : 0) + (bFinancesProLimitedWar ? 10 : 0), 0, 99);

				iBestValue = 0;
				eBestTeam = NO_TEAM;

				for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
				{
					if (GET_TEAM((TeamTypes)iI).isAlive())
					{
						if (iI != getID())
						{
							if (isHasMet((TeamTypes)iI))
							{
								if (canEventuallyDeclareWar((TeamTypes)iI))
								{
									if( GET_TEAM((TeamTypes)iI).isAVassal() && !AI_isOkayVassalTarget((TeamTypes)iI) )
									{
										continue;
									}

									if (iNoWarRoll >= AI_noWarAttitudeProb(AI_getAttitude((TeamTypes)iI)))
									{
										if (AI_isLandTarget((TeamTypes)iI) || (AI_isAnyCapitalAreaAlone() && GET_TEAM((TeamTypes)iI).AI_isAnyCapitalAreaAlone()))
										{
											if (GET_TEAM((TeamTypes)iI).getDefensivePower() < ((iOurPower * AI_limitedWarPowerRatio()) / 100))
											{
												iValue = AI_startWarVal((TeamTypes)iI);

												if (iValue > iBestValue)
												{
													FAssert(!AI_shareWar((TeamTypes)iI));
													iBestValue = iValue;
													eBestTeam = ((TeamTypes)iI);
												}
											}
										}
									}
								}
							}
						}
					}
				}

				if (eBestTeam != NO_TEAM)
				{
					AI_setWarPlan(eBestTeam, (iDaggerCount > 0) ? WARPLAN_LIMITED : WARPLAN_PREPARING_LIMITED);
				}
			}
			else if ((bFinancesProDogpileWar || !bFinancesOpposeWar) &&
				(GC.getGameINLINE().getSorenRandNum(iDogpileWarRand, "AI Dogpile War") <= iDogpileWarThreshold))
			{
				iNoWarRoll = GC.getGameINLINE().getSorenRandNum(100, "AI No War") - 20;
				iNoWarRoll = range(iNoWarRoll + (bAggressive ? 10 : 0) + (bFinancesProDogpileWar ? 10 : 0), 0, 99);

				iBestValue = 0;
				eBestTeam = NO_TEAM;

				for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
				{
					if (GET_TEAM((TeamTypes)iI).isAlive())
					{
						if (iI != getID())
						{
							if (isHasMet((TeamTypes)iI))
							{
								if (canDeclareWar((TeamTypes)iI))
								{
									if( GET_TEAM((TeamTypes)iI).isAVassal() && !AI_isOkayVassalTarget((TeamTypes)iI) )
									{
										continue;
									}

									if (iNoWarRoll >= AI_noWarAttitudeProb(AI_getAttitude((TeamTypes)iI)))
									{
										if (GET_TEAM((TeamTypes)iI).getAtWarCount(true) > 0)
										{
											if (AI_isLandTarget((TeamTypes)iI) || GET_TEAM((TeamTypes)iI).AI_isAnyMemberDoVictoryStrategyLevel4())
											{
												iDogpilePower = iOurPower;

												for (iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
												{
													if (GET_TEAM((TeamTypes)iJ).isAlive())
													{
														if (iJ != iI)
														{
															if (atWar(((TeamTypes)iJ), ((TeamTypes)iI)))
															{
																iDogpilePower += GET_TEAM((TeamTypes)iJ).getPower(false);
															}
														}
													}
												}

												FAssert(GET_TEAM((TeamTypes)iI).getPower(true) == GET_TEAM((TeamTypes)iI).getDefensivePower() || GET_TEAM((TeamTypes)iI).isAVassal());

												if (((GET_TEAM((TeamTypes)iI).getDefensivePower() * 3) / 2) < iDogpilePower)
												{
													iValue = AI_startWarVal((TeamTypes)iI);

													if (iValue > iBestValue)
													{
														FAssert(!AI_shareWar((TeamTypes)iI));
														iBestValue = iValue;
														eBestTeam = ((TeamTypes)iI);
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

				if (eBestTeam != NO_TEAM)
				{
					AI_setWarPlan(eBestTeam, WARPLAN_DOGPILE);
				}
			}
		}
	}
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

//returns true if war is veto'd by rolls.
bool CvTeamAI::AI_performNoWarRolls(TeamTypes eTeam)
{

	if (GC.getGameINLINE().getSorenRandNum(100, "AI Declare War 1") > GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getAIDeclareWarProb())
	{
		return true;
	}

	if (GC.getGameINLINE().getSorenRandNum(100, "AI No War") <= AI_noWarAttitudeProb(AI_getAttitude(eTeam)))
	{
		return true;
	}



	return false;
}

int CvTeamAI::AI_getAttitudeWeight(TeamTypes eTeam)
{
	int iAttitudeWeight = 0;
	switch (AI_getAttitude(eTeam))
	{
	case ATTITUDE_FURIOUS:
		iAttitudeWeight = -100;
		break;
	case ATTITUDE_ANNOYED:
		iAttitudeWeight = -40;
		break;
	case ATTITUDE_CAUTIOUS:
		iAttitudeWeight = -5;
		break;
	case ATTITUDE_PLEASED:
		iAttitudeWeight = 50;
		break;
	case ATTITUDE_FRIENDLY:
		iAttitudeWeight = 100;
		break;
	}

	return iAttitudeWeight;
}

int CvTeamAI::AI_getLowestVictoryCountdown() const
{
	int iBestVictoryCountdown = MAX_INT;
	for (int iVictory = 0; iVictory < GC.getNumVictoryInfos(); iVictory++)
	{
		 int iCountdown = getVictoryCountdown((VictoryTypes)iVictory);
		 if (iCountdown > 0)
		 {
			iBestVictoryCountdown = std::min(iBestVictoryCountdown, iCountdown);
		 }
	}
	if (MAX_INT == iBestVictoryCountdown)
	{
		iBestVictoryCountdown = -1;
	}
	return iBestVictoryCountdown;
}

int CvTeamAI::AI_getTechMonopolyValue(TechTypes eTech, TeamTypes eTeam) const
{
	int iValue = 0;
	int iI;

	bool bWarPlan = (getAnyWarPlanCount(eTeam) > 0);

	for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		UnitTypes eLoopUnit = ((UnitTypes)GC.getUnitClassInfo((UnitClassTypes)iI).getDefaultUnitIndex());

		if (eLoopUnit != NO_UNIT)
		{
			if (isTechRequiredForUnit((eTech), eLoopUnit))
			{
				if (isWorldUnitClass((UnitClassTypes)iI))
				{
					iValue += 50;
				}

				if (GC.getUnitInfo(eLoopUnit).getPrereqAndTech() == eTech)
				{
					int iNavalValue = 0;

					int iCombatRatio = (GC.getUnitInfo(eLoopUnit).getCombat() * 100) / std::max(1, GC.getGameINLINE().getBestLandUnitCombat());
					if (iCombatRatio > 50)
					{
						iValue += ((bWarPlan ? 100 : 50) * (iCombatRatio - 40)) / 50;
					}

					switch (GC.getUnitInfo(eLoopUnit).getDefaultUnitAIType())
					{
					case UNITAI_UNKNOWN:
					case UNITAI_ANIMAL:
					case UNITAI_SETTLE:
					case UNITAI_WORKER:
					break;

					case UNITAI_ATTACK:
					case UNITAI_ATTACK_CITY:
					case UNITAI_COLLATERAL:
/*************************************************************************************************/
/**	Xienwolf Tweak							01/04/09											**/
/**																								**/
/**					Possibly wasn't intended to ever be a default AI type, added				**/
/*************************************************************************************************/
					case UNITAI_ATTACK_CITY_LEMMING:
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
						iValue += bWarPlan ? 50 : 20;
						break;

					case UNITAI_PILLAGE:
					case UNITAI_RESERVE:
					case UNITAI_COUNTER:
					case UNITAI_PARADROP:
					case UNITAI_CITY_DEFENSE:
					case UNITAI_CITY_COUNTER:
					case UNITAI_CITY_SPECIAL:
						iValue += bWarPlan ? 40 : 15;
						break;


					case UNITAI_EXPLORE:
					case UNITAI_MISSIONARY:
						break;

					case UNITAI_PROPHET:
					case UNITAI_ARTIST:
					case UNITAI_SCIENTIST:
					case UNITAI_GENERAL:
					case UNITAI_MERCHANT:
					case UNITAI_ENGINEER:
						break;

					case UNITAI_SPY:
						break;

					case UNITAI_ICBM:
						iValue += bWarPlan ? 80 : 40;
						break;

					case UNITAI_WORKER_SEA:
						break;

					case UNITAI_ATTACK_SEA:
						iNavalValue += 50;
						break;

					case UNITAI_RESERVE_SEA:
					case UNITAI_ESCORT_SEA:
						iNavalValue += 30;
						break;

					case UNITAI_EXPLORE_SEA:
						iValue += GC.getGame().circumnavigationAvailable() ? 100 : 0;
						break;

					case UNITAI_ASSAULT_SEA:
						iNavalValue += 60;
						break;

					case UNITAI_SETTLER_SEA:
					case UNITAI_MISSIONARY_SEA:
					case UNITAI_SPY_SEA:
						break;

					case UNITAI_CARRIER_SEA:
					case UNITAI_MISSILE_CARRIER_SEA:
						iNavalValue += 40;
						break;

					case UNITAI_PIRATE_SEA:
						iNavalValue += 20;
						break;

					case UNITAI_ATTACK_AIR:
					case UNITAI_DEFENSE_AIR:
						iValue += bWarPlan ? 60 : 30;
						break;

					case UNITAI_CARRIER_AIR:
						iNavalValue += 40;
						break;

					case UNITAI_MISSILE_AIR:
						iValue += bWarPlan ? 40 : 20;
						break;

					default:
						FAssert(false);
						break;
					}

					if (iNavalValue > 0)
					{
						if (AI_isAnyCapitalAreaAlone())
						{
							iValue += iNavalValue / 2;
						}
						if (bWarPlan && !AI_isLandTarget(eTeam))
						{
							iValue += iNavalValue / 2;
						}
					}
				}
			}
		}
	}

	for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		if (isTechRequiredForBuilding(eTech, ((BuildingTypes)iI)))
		{
			CvBuildingInfo& kLoopBuilding = GC.getBuildingInfo((BuildingTypes)iI);
			if (kLoopBuilding.getReligionType() == NO_RELIGION)
			{
				iValue += 30;
			}
			if (isWorldWonderClass((BuildingClassTypes)kLoopBuilding.getBuildingClassType()))
			{
				if (!(GC.getGameINLINE().isBuildingClassMaxedOut((BuildingClassTypes)kLoopBuilding.getBuildingClassType())))
				{
					iValue += 50;
				}
			}
		}
	}

	for (iI = 0; iI < GC.getNumProjectInfos(); iI++)
	{
		if (GC.getProjectInfo((ProjectTypes)iI).getTechPrereq() == eTech)
		{
			if (isWorldProject((ProjectTypes)iI))
			{
				if (!(GC.getGameINLINE().isProjectMaxedOut((ProjectTypes)iI)))
				{
					iValue += 100;
				}
			}
			else
			{
				iValue += 50;
			}
		}
	}

	return iValue;


}

bool CvTeamAI::AI_isWaterAreaRelevant(CvArea* pArea)
{
	int iTeamCities = 0;
	int iOtherTeamCities = 0;

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/15/09                                jdog5000      */
/*                                                                                              */
/* City AI                                                                                      */
/************************************************************************************************/
	CvArea* pBiggestArea = GC.getMap().findBiggestArea(true);
	if (pBiggestArea == pArea)
	{
		return true;
	}

	// An area is deemed relevant if it has at least 2 cities of our and different teams.
	// Also count lakes which are connected to ocean by a bridge city
	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
	{
		CvPlayerAI& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);

		if ((iTeamCities < 2 && (kPlayer.getTeam() == getID())) || (iOtherTeamCities < 2 && (kPlayer.getTeam() != getID())))
		{
			int iLoop;
			CvCity* pLoopCity;

			for (pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
			{
				if (pLoopCity->plot()->isAdjacentToArea(pArea->getID()))
				{
					if (kPlayer.getTeam() == getID())
					{
						iTeamCities++;

						if( pLoopCity->waterArea() == pBiggestArea )
						{
							return true;
						}
					}
					else
					{
						iOtherTeamCities++;
						if (iOtherTeamCities >= 2)
						{
							break;
						}
					}
				}
			}
		}
		if (iTeamCities >= 2 && iOtherTeamCities >= 2)
		{
			return true;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	return false;
}

// Private Functions...
