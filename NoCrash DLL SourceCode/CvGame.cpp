// game.cpp

#include "CvGameCoreDLL.h"
#include "CvGameCoreUtils.h"
#include "CvGame.h"
#include "CvGameAI.h"
#include "CvMap.h"
#include "CvPlot.h"
#include "CvPlayerAI.h"
#include "CvRandom.h"
#include "CvTeamAI.h"
#include "CvGlobals.h"
#include "CvInitCore.h"
#include "CvMapGenerator.h"
#include "CvArtFileMgr.h"
#include "CvDiploParameters.h"
#include "CvReplayMessage.h"
#include "CyArgsList.h"
#include "CvInfos.h"
#include "CvPopupInfo.h"
#include "FProfiler.h"
#include "CvReplayInfo.h"
#include "CvGameTextMgr.h"
/*************************************************************************************************/
/**	Xienwolf Tweak							02/01/09											**/
/**																								**/
/**				Required to pass a PyPlot object out to Python in Barbarian Spawning			**/
/*************************************************************************************************/
#include "CyPlot.h"
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
#include <set>
#include "CvEventReporter.h"
#include "CvMessageControl.h"

// interface uses
#include "CvDLLInterfaceIFaceBase.h"
#include "CvDLLEngineIFaceBase.h"
#include "CvDLLPythonIFaceBase.h"

// Public Functions...

CvGame::CvGame()
{
	m_aiRankPlayer = new int[MAX_PLAYERS];        // Ordered by rank...
	m_aiPlayerRank = new int[MAX_PLAYERS];        // Ordered by player ID...
	m_aiPlayerScore = new int[MAX_PLAYERS];       // Ordered by player ID...
	m_aiRankTeam = new int[MAX_TEAMS];						// Ordered by rank...
	m_aiTeamRank = new int[MAX_TEAMS];						// Ordered by team ID...
	m_aiTeamScore = new int[MAX_TEAMS];						// Ordered by team ID...

//FfH: Added by Kael 11/14/2007
	m_pabEventTriggered = NULL;
	m_pabGamblingRing = NULL;
	m_pabNoBonus = NULL;
	m_pabNoOutsideTechTrades = NULL;
	m_pabSlaveTrade = NULL;
	m_pabSmugglingRing = NULL;
//FfH: End Add

	m_paiUnitCreatedCount = NULL;
	m_paiUnitClassCreatedCount = NULL;
/*************************************************************************************************/
/**	Spawn Groups						08/05/10									Valkrionn	**/
/**																								**/
/**					New spawn mechanic, allowing us to customize stacks							**/
/*************************************************************************************************/
	m_paiSpawnGroupCreatedCount = NULL;
/*************************************************************************************************/
/**	Spawn Groups							END													**/
/*************************************************************************************************/
	m_paiBuildingClassCreatedCount = NULL;
	m_paiProjectCreatedCount = NULL;
	m_paiForceCivicCount = NULL;
	m_paiVoteOutcome = NULL;
	m_paiReligionGameTurnFounded = NULL;
	m_paiCorporationGameTurnFounded = NULL;
	m_aiSecretaryGeneralTimer = NULL;
	m_aiVoteTimer = NULL;
	m_aiDiploVote = NULL;

	m_pabSpecialUnitValid = NULL;
	m_pabSpecialBuildingValid = NULL;
	m_abReligionSlotTaken = NULL;
	m_pabTriggeredGoodies = NULL;

	m_paHolyCity = NULL;
	m_paHeadquarters = NULL;

	m_pReplayInfo = NULL;

	m_aiShrineBuilding = NULL;
	m_aiShrineReligion = NULL;
/*************************************************************************************************/
/**	New Tag Defs	(ProjectInfos)			10/01/08								Xienwolf	**/
/**	New Tag Defs	(GameInfos)				10/01/08								Xienwolf	**/
/**										Allocate Memory											**/
/*************************************************************************************************/
	m_ppaaiProjectTimers = NULL;
/*************************************************************************************************/
/**	New Tag Defs							END													**/
/*************************************************************************************************/

	reset(NO_HANDICAP, true);
}


CvGame::~CvGame()
{
	uninit();

	SAFE_DELETE_ARRAY(m_aiRankPlayer);
	SAFE_DELETE_ARRAY(m_aiPlayerRank);
	SAFE_DELETE_ARRAY(m_aiPlayerScore);
	SAFE_DELETE_ARRAY(m_aiRankTeam);
	SAFE_DELETE_ARRAY(m_aiTeamRank);
	SAFE_DELETE_ARRAY(m_aiTeamScore);
}

void CvGame::init(HandicapTypes eHandicap)
{
	bool bValid;
	int iStartTurn;
	int iEstimateEndTurn;
	int iI;

	//--------------------------------
	// Init saved data
	reset(eHandicap);

	//--------------------------------
	// Init containers
	m_deals.init();
	m_voteSelections.init();
	m_votesTriggered.init();

	m_mapRand.init(GC.getInitCore().getMapRandSeed() % 73637381);
	m_sorenRand.init(GC.getInitCore().getSyncRandSeed() % 52319761);

	//--------------------------------
	// Init non-saved data

	//--------------------------------
	// Init other game data

	// Turn off all MP options if it's a single player game
	if (GC.getInitCore().getType() == GAME_SP_NEW ||
		GC.getInitCore().getType() == GAME_SP_SCENARIO)
	{
		for (iI = 0; iI < NUM_MPOPTION_TYPES; ++iI)
		{
			setMPOption((MultiplayerOptionTypes)iI, false);
		}
	}

	// If this is a hot seat game, simultaneous turns is always off
	if (isHotSeat() || isPbem())
	{
		setMPOption(MPOPTION_SIMULTANEOUS_TURNS, false);
	}
	// If we didn't set a time in the Pitboss, turn timer off
	if (isPitboss() && getPitbossTurnTime() == 0)
	{
		setMPOption(MPOPTION_TURN_TIMER, false);
	}

	if (isMPOption(MPOPTION_SHUFFLE_TEAMS))
	{
		int aiTeams[MAX_CIV_PLAYERS];

		int iNumPlayers = 0;
		for (int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			if (GC.getInitCore().getSlotStatus((PlayerTypes)i) == SS_TAKEN)
			{
				aiTeams[iNumPlayers] = GC.getInitCore().getTeam((PlayerTypes)i);
				++iNumPlayers;
			}
		}

		for (int i = 0; i < iNumPlayers; i++)
		{
			int j = (getSorenRand().get(iNumPlayers - i, NULL) + i);

			if (i != j)
			{
				int iTemp = aiTeams[i];
				aiTeams[i] = aiTeams[j];
				aiTeams[j] = iTemp;
			}
		}

		iNumPlayers = 0;
		for (int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			if (GC.getInitCore().getSlotStatus((PlayerTypes)i) == SS_TAKEN)
			{
				GC.getInitCore().setTeam((PlayerTypes)i, (TeamTypes)aiTeams[iNumPlayers]);
				++iNumPlayers;
			}
		}
	}

//FfH: Added by Kael 05/28/2008
	int iRndCiv = GC.getDefineINT("RANDOM_CIVILIZATION");
	if (iRndCiv != -1)
	{
		int iAlignment = 0;
		int iBestLeader = -1;
		int iBestCiv = -1;
		int iBestValue = -1;
		int iValue = 0;

		for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
		{
			if ((GC.getInitCore().getSlotStatus((PlayerTypes)iPlayer) == SS_COMPUTER) || (GC.getInitCore().getSlotStatus((PlayerTypes)iPlayer) == SS_TAKEN))
			{
				iAlignment = 0;
				if (GC.getInitCore().getCiv((PlayerTypes)iPlayer) == NO_CIVILIZATION)
				{
					GC.getInitCore().setCiv((PlayerTypes)iPlayer, (CivilizationTypes)iRndCiv);
					iAlignment = -1;
				}
				if (GC.getInitCore().getCiv((PlayerTypes)iPlayer) == iRndCiv)
				{
					if (iAlignment != -1)
					{
						iAlignment = GC.getLeaderHeadInfo((LeaderHeadTypes)GC.getInitCore().getLeader((PlayerTypes)iPlayer)).getAlignment();
					}
					for (int iCiv = 0; iCiv < GC.getNumCivilizationInfos(); iCiv++)
					{
/*************************************************************************************************/
/**	CivSelector							10/22/08									Xienwolf	**/
/**																								**/
/**				Restricts AI Selection and Human Selection seperately							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
						if (GC.getCivilizationInfo((CivilizationTypes)iCiv).isAIPlayable())
/**								----  End Original Code  ----									**/
						if ((GC.getInitCore().getSlotStatus((PlayerTypes)iPlayer) == SS_COMPUTER && GC.getCivilizationInfo((CivilizationTypes)iCiv).isAIPlayable()) || (GC.getInitCore().getSlotStatus((PlayerTypes)iPlayer) == SS_TAKEN && GC.getCivilizationInfo((CivilizationTypes)iCiv).isPlayable()))
/*************************************************************************************************/
/**	CivSelector									END												**/
/*************************************************************************************************/
						{
							for (int iLeader = 0; iLeader < GC.getNumLeaderHeadInfos(); iLeader++)
							{
								if (GC.getCivilizationInfo((CivilizationTypes)iCiv).isLeaders(iLeader))
								{
									if (iAlignment == -1 || GC.getLeaderHeadInfo((LeaderHeadTypes)iLeader).getAlignment() == iAlignment)
									{
										iValue = 40000 + getSorenRandNum(1000, "Random Leader");
										for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
										{
											if (GC.getInitCore().getLeader((PlayerTypes)iI) == iLeader)
											{
												iValue -= 2000;
											}
											if (GC.getInitCore().getCiv((PlayerTypes)iI) == iCiv)
											{
												iValue -= 1000;
											}
										}
/*************************************************************************************************/
/**	LeaderStatus Infos      				10/02/09								Valkrionn	**/
/**                                         Originally                                          **/
/** Minor Leaders Options       Opera       08.06.09         imported by Valkrionn	09.26.09    **/
/*************************************************************************************************/
										if (isOption(GAMEOPTION_THE_ACKNOWLEDGEMENT))
										{
											if (GC.getLeaderClassInfo((LeaderClassTypes)GC.getLeaderHeadInfo((LeaderHeadTypes)iLeader).getLeaderClass()).getLeaderStatus() != HISTORICAL_STATUS)
											{
												iValue += 50000;
											}
										}
										if (isOption(GAMEOPTION_WELLKNOWN_FACES))
										{
										 	if (GC.getLeaderClassInfo((LeaderClassTypes)GC.getLeaderHeadInfo((LeaderHeadTypes)iLeader).getLeaderClass()).getLeaderStatus() == HISTORICAL_STATUS)
											{
												iValue += 50000;
											}
											if (GC.getLeaderClassInfo((LeaderClassTypes)GC.getLeaderHeadInfo((LeaderHeadTypes)iLeader).getLeaderClass()).getLeaderStatus() == NO_STATUS)
											{
												iValue += 50000;
											}
										}
/*************************************************************************************************/
/** End                                                                                         **/
/*************************************************************************************************/
										if (iValue > iBestValue)
										{
											iBestCiv = iCiv;
											iBestLeader = iLeader;
											iBestValue = iValue;
										}
									}
								}
							}
						}
					}
					GC.getInitCore().setCiv((PlayerTypes)iPlayer, (CivilizationTypes)iBestCiv);
					GC.getInitCore().setLeader((PlayerTypes)iPlayer, (LeaderHeadTypes)iBestLeader);
					iBestCiv = -1;
					iBestLeader = -1;
					iBestValue = -1;
				}
			}
		}
	}
//FfH: End Add

	if (isOption(GAMEOPTION_LOCK_MODS))
	{
		if (isGameMultiPlayer())
		{
			setOption(GAMEOPTION_LOCK_MODS, false);
		}
		else
		{
			static const int iPasswordSize = 8;
			char szRandomPassword[iPasswordSize];
			for (int i = 0; i < iPasswordSize-1; i++)
			{
				szRandomPassword[i] = getSorenRandNum(128, NULL);
			}
			szRandomPassword[iPasswordSize-1] = 0;

			GC.getInitCore().setAdminPassword(szRandomPassword);
		}
	}

	if (getGameTurn() == 0)
	{
		iStartTurn = 0;

		for (iI = 0; iI < GC.getGameSpeedInfo(getGameSpeedType()).getNumTurnIncrements(); iI++)
		{
			iStartTurn += GC.getGameSpeedInfo(getGameSpeedType()).getGameTurnInfo(iI).iNumGameTurnsPerIncrement;
		}

		iStartTurn *= GC.getEraInfo(getStartEra()).getStartPercent();
		iStartTurn /= 100;

		setGameTurn(iStartTurn);
	}

	setStartTurn(getGameTurn());

	if (getMaxTurns() == 0)
	{
		iEstimateEndTurn = 0;

		for (iI = 0; iI < GC.getGameSpeedInfo(getGameSpeedType()).getNumTurnIncrements(); iI++)
		{
			iEstimateEndTurn += GC.getGameSpeedInfo(getGameSpeedType()).getGameTurnInfo(iI).iNumGameTurnsPerIncrement;
		}

		setEstimateEndTurn(iEstimateEndTurn);

		if (getEstimateEndTurn() > getGameTurn())
		{
			bValid = false;

			for (iI = 0; iI < GC.getNumVictoryInfos(); iI++)
			{
				if (isVictoryValid((VictoryTypes)iI))
				{
					if (GC.getVictoryInfo((VictoryTypes)iI).isEndScore())
					{
						bValid = true;
						break;
					}
				}
			}

			if (bValid)
			{
				setMaxTurns(getEstimateEndTurn() - getGameTurn());
			}
		}
	}
	else
	{
		setEstimateEndTurn(getGameTurn() + getMaxTurns());
	}

	setStartYear(GC.getDefineINT("START_YEAR"));

	for (iI = 0; iI < GC.getNumSpecialUnitInfos(); iI++)
	{
		if (GC.getSpecialUnitInfo((SpecialUnitTypes)iI).isValid())
		{
			makeSpecialUnitValid((SpecialUnitTypes)iI);
		}
	}

	for (iI = 0; iI < GC.getNumSpecialBuildingInfos(); iI++)
	{
		if (GC.getSpecialBuildingInfo((SpecialBuildingTypes)iI).isValid())
		{
			makeSpecialBuildingValid((SpecialBuildingTypes)iI);
		}
	}

	AI_init();

	doUpdateCacheOnTurn();
}

//
// Set initial items (units, techs, etc...)
//
void CvGame::setInitialItems()
{
	PROFILE_FUNC();

	initFreeState();
	assignStartingPlots();
	normalizeStartingPlots();
/*************************************************************************************************/
/**	Flavour Mod								06/23/08								Jean Elcard **/
/**																								**/
/**					Activates the Python Programming to control selection						**/
/*************************************************************************************************/
	if (isOption(GAMEOPTION_FLAVOUR_START))
	{
		gDLL->getPythonIFace()->callFunction(PYFlavourModule, "generateFlavour");
	}
/*************************************************************************************************/
/**	Flavour Mod								END													**/
/*************************************************************************************************/

//FfH: Added by Kael 09/16/2008
//	if (isOption(GAMEOPTION_BARBARIAN_WORLD))
//	{
//		for (int iI = 0; iI < MAX_PLAYERS; iI++)
//		{
///*************************************************************************************************/
///**	MultiBarb							12/23/08									Xienwolf	**/
///**																								**/
///**							Adds extra Barbarian Civilizations									**/
///*************************************************************************************************/
///**								---- Start Original Code ----									**
//			if (GET_PLAYER((PlayerTypes)iI).isAlive() && iI != BARBARIAN_PLAYER)
///**								----  End Original Code  ----									**/
//			if (GET_PLAYER((PlayerTypes)iI).isAlive() && iI != ORC_PLAYER && iI != ANIMAL_PLAYER && iI != DEMON_PLAYER)
///*************************************************************************************************/
///**	MultiBarb								END													**/
///*************************************************************************************************/
//			{
//				foundBarbarianCity();
//			}
//		}
//	}
//FfH: End Add

	initFreeUnits();

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)i);
		if (kPlayer.isAlive())
		{
			kPlayer.AI_updateFoundValues();
		}
	}
}


void CvGame::regenerateMap()
{
	int iI;

	if (GC.getInitCore().getWBMapScript())
	{
		return;
	}

	setFinalInitialized(false);

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		GET_PLAYER((PlayerTypes)iI).killUnits();
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		GET_PLAYER((PlayerTypes)iI).killCities();
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		GET_PLAYER((PlayerTypes)iI).killAllDeals();
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		GET_PLAYER((PlayerTypes)iI).setFoundedFirstCity(false);
		GET_PLAYER((PlayerTypes)iI).setStartingPlot(NULL, false);
	}

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		GC.getMapINLINE().setRevealedPlots(((TeamTypes)iI), false);
	}

	gDLL->getEngineIFace()->clearSigns();

	GC.getMapINLINE().erasePlots();

	CvMapGenerator::GetInstance().generateRandomMap();
	CvMapGenerator::GetInstance().addGameElements();

	gDLL->getEngineIFace()->RebuildAllPlots();

	CvEventReporter::getInstance().resetStatistics();

	setInitialItems();

	initScoreCalculation();
	setFinalInitialized(true);

	GC.getMapINLINE().setupGraphical();
	gDLL->getEngineIFace()->SetDirty(GlobeTexture_DIRTY_BIT, true);
	gDLL->getEngineIFace()->SetDirty(MinimapTexture_DIRTY_BIT, true);
	gDLL->getInterfaceIFace()->setDirty(ColoredPlots_DIRTY_BIT, true);

	gDLL->getInterfaceIFace()->setCycleSelectionCounter(1);

	gDLL->getEngineIFace()->AutoSave(true);

	if (NO_PLAYER != getActivePlayer())
	{
		CvPlot* pPlot = GET_PLAYER(getActivePlayer()).getStartingPlot();

		if (NULL != pPlot)
		{
			gDLL->getInterfaceIFace()->lookAt(pPlot->getPoint(), CAMERALOOKAT_NORMAL);
		}
	}
}


void CvGame::uninit()
{

//FfH: Added by Kael 11/14/2007
	SAFE_DELETE_ARRAY(m_pabEventTriggered);
	SAFE_DELETE_ARRAY(m_pabGamblingRing);
	SAFE_DELETE_ARRAY(m_pabNoBonus);
	SAFE_DELETE_ARRAY(m_pabNoOutsideTechTrades);
	SAFE_DELETE_ARRAY(m_pabSlaveTrade);
	SAFE_DELETE_ARRAY(m_pabSmugglingRing);
//FfH: End Add

	SAFE_DELETE_ARRAY(m_aiShrineBuilding);
	SAFE_DELETE_ARRAY(m_aiShrineReligion);
	SAFE_DELETE_ARRAY(m_paiUnitCreatedCount);
	SAFE_DELETE_ARRAY(m_paiUnitClassCreatedCount);
/*************************************************************************************************/
/**	Spawn Groups						08/05/10									Valkrionn	**/
/**																								**/
/**					New spawn mechanic, allowing us to customize stacks							**/
/*************************************************************************************************/
	SAFE_DELETE_ARRAY(m_paiSpawnGroupCreatedCount);
/*************************************************************************************************/
/**	Spawn Groups							END													**/
/*************************************************************************************************/
	SAFE_DELETE_ARRAY(m_paiBuildingClassCreatedCount);
	SAFE_DELETE_ARRAY(m_paiProjectCreatedCount);
	SAFE_DELETE_ARRAY(m_paiForceCivicCount);
	SAFE_DELETE_ARRAY(m_paiVoteOutcome);
	SAFE_DELETE_ARRAY(m_paiReligionGameTurnFounded);
	SAFE_DELETE_ARRAY(m_paiCorporationGameTurnFounded);
	SAFE_DELETE_ARRAY(m_aiSecretaryGeneralTimer);
	SAFE_DELETE_ARRAY(m_aiVoteTimer);
	SAFE_DELETE_ARRAY(m_aiDiploVote);

	SAFE_DELETE_ARRAY(m_pabSpecialUnitValid);
	SAFE_DELETE_ARRAY(m_pabSpecialBuildingValid);
	SAFE_DELETE_ARRAY(m_abReligionSlotTaken);
	SAFE_DELETE_ARRAY(m_pabTriggeredGoodies);

	SAFE_DELETE_ARRAY(m_paHolyCity);
	SAFE_DELETE_ARRAY(m_paHeadquarters);

	m_aszDestroyedCities.clear();
	m_aszGreatPeopleBorn.clear();

	m_deals.uninit();
	m_voteSelections.uninit();
	m_votesTriggered.uninit();

	m_mapRand.uninit();
	m_sorenRand.uninit();

	clearReplayMessageMap();
	SAFE_DELETE(m_pReplayInfo);

	m_aPlotExtraYields.clear();
	m_aPlotExtraCosts.clear();
	m_mapVoteSourceReligions.clear();
	m_aeInactiveTriggers.clear();
/*************************************************************************************************/
/**	New Tag Defs	(ProjectInfos)			10/01/08								Xienwolf	**/
/**	New Tag Defs	(GameInfos)				10/01/08								Xienwolf	**/
/**										Clear Arrays											**/
/*************************************************************************************************/
	if (m_ppaaiProjectTimers != NULL)
	{
		for (int iI = 0; iI < GC.getNumProjectInfos(); iI++)
		{
			SAFE_DELETE_ARRAY(m_ppaaiProjectTimers[iI]);
		}
		SAFE_DELETE_ARRAY(m_ppaaiProjectTimers);
	}
/*************************************************************************************************/
/**	New Tag Defs							END													**/
/*************************************************************************************************/
}


// FUNCTION: reset()
// Initializes data members that are serialized.
void CvGame::reset(HandicapTypes eHandicap, bool bConstructorCall)
{
	int iI;

	//--------------------------------
	// Uninit class
	uninit();

	m_iElapsedGameTurns = 0;
	m_iStartTurn = 0;
	m_iStartYear = 0;
	m_iEstimateEndTurn = 0;
	m_iTurnSlice = 0;
	m_iTurnSliceSinceBeginning = 0;
	m_iCutoffSlice = 0;
	m_iNumGameTurnActive = 0;
	m_iNumCities = 0;
	m_iTotalPopulation = 0;
	m_iTradeRoutes = 0;
	m_iFreeTradeCount = 0;
	m_iNoNukesCount = 0;
	m_iNukesExploded = 0;
	m_iMaxPopulation = 0;
	m_iMaxLand = 0;
	m_iMaxTech = 0;
	m_iMaxWonders = 0;
	m_iInitPopulation = 0;
	m_iInitLand = 0;
	m_iInitTech = 0;
	m_iInitWonders = 0;
	m_iAIAutoPlay = 0;
/*************************************************************************************************/
/**	xUPT								02/08/11									Afforess	**/
/**																								**/
/**						xUPT mechanic, ported and modified by Valkrionn							**/
/*************************************************************************************************/
	m_iUPT = 0;
	m_bUPTLock = false;
/*************************************************************************************************/
/**	xUPT									END													**/
/*************************************************************************************************/

	m_uiInitialTime = 0;

	m_bScoreDirty = false;
	m_bCircumnavigated = false;
	m_bDebugMode = false;
	m_bDebugModeCache = false;
	m_bFinalInitialized = false;
	m_bPbemTurnSent = false;
	m_bHotPbemBetweenTurns = false;
	m_bPlayerOptionsSent = false;
	m_bNukesValid = false;

	m_eHandicap = eHandicap;
	m_ePausePlayer = NO_PLAYER;
	m_eBestLandUnit = NO_UNIT;
	m_eWinner = NO_TEAM;
	m_eVictory = NO_VICTORY;
	m_eGameState = GAMESTATE_ON;

	m_szScriptData = "";

//FfH: Added by Kael 08/07/2007
	m_iCrime = 10;
	m_iCutLosersCounter = 100;
	m_iFlexibleDifficultyCounter = 0;
	m_iGlobalCounter = 0;
	m_iHighToLowCounter = 0;
	m_iIncreasingDifficultyCounter = 0;
	m_iMaxGlobalCounter = 0;
	m_iGlobalCounterLimit = GC.getDefineINT("GLOBAL_COUNTER_LIMIT_DEFAULT");
	m_iScenarioCounter = 0;
//FfH: End Add

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		m_aiRankPlayer[iI] = 0;
		m_aiPlayerRank[iI] = 0;
		m_aiPlayerScore[iI] = 0;
	}

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		m_aiRankTeam[iI] = 0;
		m_aiTeamRank[iI] = 0;
		m_aiTeamScore[iI] = 0;
	}

	if (!bConstructorCall)
	{

//FfH: Added by Kael 11/14/2007
		m_pabEventTriggered = new bool[GC.getNumEventTriggerInfos()];
		for (iI = 0; iI < GC.getNumEventTriggerInfos(); iI++)
		{
			m_pabEventTriggered[iI] = false;
		}
		m_pabGamblingRing = new bool[GC.getNumVoteSourceInfos()];
		for (iI = 0; iI < GC.getNumVoteSourceInfos(); iI++)
		{
			m_pabGamblingRing[iI] = false;
		}
		m_pabNoBonus = new bool[GC.getNumBonusInfos()];
		for (iI = 0; iI < GC.getNumBonusInfos(); iI++)
		{
			m_pabNoBonus[iI] = false;
		}
		m_pabNoOutsideTechTrades = new bool[GC.getNumVoteSourceInfos()];
		for (iI = 0; iI < GC.getNumVoteSourceInfos(); iI++)
		{
			m_pabNoOutsideTechTrades[iI] = false;
		}
		m_pabSlaveTrade = new bool[GC.getNumVoteSourceInfos()];
		for (iI = 0; iI < GC.getNumVoteSourceInfos(); iI++)
		{
			m_pabSlaveTrade[iI] = false;
		}
		m_pabSmugglingRing = new bool[GC.getNumVoteSourceInfos()];
		for (iI = 0; iI < GC.getNumVoteSourceInfos(); iI++)
		{
			m_pabSmugglingRing[iI] = false;
		}
//FfH: End Add

		FAssertMsg(m_paiUnitCreatedCount==NULL, "about to leak memory, CvGame::m_paiUnitCreatedCount");
		m_paiUnitCreatedCount = new int[GC.getNumUnitInfos()];
		for (iI = 0; iI < GC.getNumUnitInfos(); iI++)
		{
			m_paiUnitCreatedCount[iI] = 0;
		}

		FAssertMsg(m_paiUnitClassCreatedCount==NULL, "about to leak memory, CvGame::m_paiUnitClassCreatedCount");
		m_paiUnitClassCreatedCount = new int[GC.getNumUnitClassInfos()];
		for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
		{
			m_paiUnitClassCreatedCount[iI] = 0;
		}

/*************************************************************************************************/
/**	Spawn Groups						08/05/10									Valkrionn	**/
/**																								**/
/**					New spawn mechanic, allowing us to customize stacks							**/
/*************************************************************************************************/
		FAssertMsg(m_paiSpawnGroupCreatedCount==NULL, "about to leak memory, CvGame::m_paiSpawnGroupCreatedCount");
		m_paiSpawnGroupCreatedCount = new int[GC.getNumSpawnGroupInfos()];
		for (iI = 0; iI < GC.getNumSpawnGroupInfos(); iI++)
		{
			m_paiSpawnGroupCreatedCount[iI] = 0;
		}
/*************************************************************************************************/
/**	Spawn Groups							END													**/
/*************************************************************************************************/

		FAssertMsg(m_paiBuildingClassCreatedCount==NULL, "about to leak memory, CvGame::m_paiBuildingClassCreatedCount");
		m_paiBuildingClassCreatedCount = new int[GC.getNumBuildingClassInfos()];
		for (iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
		{
			m_paiBuildingClassCreatedCount[iI] = 0;
		}

		FAssertMsg(m_paiProjectCreatedCount==NULL, "about to leak memory, CvGame::m_paiProjectCreatedCount");
		m_paiProjectCreatedCount = new int[GC.getNumProjectInfos()];
		for (iI = 0; iI < GC.getNumProjectInfos(); iI++)
		{
			m_paiProjectCreatedCount[iI] = 0;
		}

		FAssertMsg(m_paiForceCivicCount==NULL, "about to leak memory, CvGame::m_paiForceCivicCount");
		m_paiForceCivicCount = new int[GC.getNumCivicInfos()];
		for (iI = 0; iI < GC.getNumCivicInfos(); iI++)
		{
			m_paiForceCivicCount[iI] = 0;
		}

		FAssertMsg(0 < GC.getNumVoteInfos(), "GC.getNumVoteInfos() is not greater than zero in CvGame::reset");
		FAssertMsg(m_paiVoteOutcome==NULL, "about to leak memory, CvGame::m_paiVoteOutcome");
		m_paiVoteOutcome = new PlayerVoteTypes[GC.getNumVoteInfos()];
		for (iI = 0; iI < GC.getNumVoteInfos(); iI++)
		{
			m_paiVoteOutcome[iI] = NO_PLAYER_VOTE;
		}

		FAssertMsg(0 < GC.getNumVoteSourceInfos(), "GC.getNumVoteSourceInfos() is not greater than zero in CvGame::reset");
		FAssertMsg(m_aiDiploVote==NULL, "about to leak memory, CvGame::m_aiDiploVote");
		m_aiDiploVote = new int[GC.getNumVoteSourceInfos()];
		for (iI = 0; iI < GC.getNumVoteSourceInfos(); iI++)
		{
			m_aiDiploVote[iI] = 0;
		}

		FAssertMsg(m_pabSpecialUnitValid==NULL, "about to leak memory, CvGame::m_pabSpecialUnitValid");
		m_pabSpecialUnitValid = new bool[GC.getNumSpecialUnitInfos()];
		for (iI = 0; iI < GC.getNumSpecialUnitInfos(); iI++)
		{
			m_pabSpecialUnitValid[iI] = false;
		}

		FAssertMsg(m_pabSpecialBuildingValid==NULL, "about to leak memory, CvGame::m_pabSpecialBuildingValid");
		m_pabSpecialBuildingValid = new bool[GC.getNumSpecialBuildingInfos()];
		for (iI = 0; iI < GC.getNumSpecialBuildingInfos(); iI++)
		{
			m_pabSpecialBuildingValid[iI] = false;
		}

		FAssertMsg(m_pabTriggeredGoodies == NULL, "about to leak memory, CvGame::m_pabTriggeredGoodies");
		m_pabTriggeredGoodies = new bool[GC.getNumGoodyInfos()];
		for (iI = 0; iI < GC.getNumGoodyInfos(); iI++)
		{
			m_pabTriggeredGoodies[iI] = false;
		}

		FAssertMsg(m_paiReligionGameTurnFounded==NULL, "about to leak memory, CvGame::m_paiReligionGameTurnFounded");
		m_paiReligionGameTurnFounded = new int[GC.getNumReligionInfos()];
		FAssertMsg(m_abReligionSlotTaken==NULL, "about to leak memory, CvGame::m_abReligionSlotTaken");
		m_abReligionSlotTaken = new bool[GC.getNumReligionInfos()];
		FAssertMsg(m_paHolyCity==NULL, "about to leak memory, CvGame::m_paHolyCity");
		m_paHolyCity = new IDInfo[GC.getNumReligionInfos()];
		for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
		{
			m_paiReligionGameTurnFounded[iI] = -1;
			m_paHolyCity[iI].reset();
			m_abReligionSlotTaken[iI] = false;
		}

		FAssertMsg(m_paiCorporationGameTurnFounded==NULL, "about to leak memory, CvGame::m_paiCorporationGameTurnFounded");
		m_paiCorporationGameTurnFounded = new int[GC.getNumCorporationInfos()];
		m_paHeadquarters = new IDInfo[GC.getNumCorporationInfos()];
		for (iI = 0; iI < GC.getNumCorporationInfos(); iI++)
		{
			m_paiCorporationGameTurnFounded[iI] = -1;
			m_paHeadquarters[iI].reset();
		}

		FAssertMsg(m_aiShrineBuilding==NULL, "about to leak memory, CvGame::m_aiShrineBuilding");
		FAssertMsg(m_aiShrineReligion==NULL, "about to leak memory, CvGame::m_aiShrineReligion");
		m_aiShrineBuilding = new int[GC.getNumBuildingInfos()];
		m_aiShrineReligion = new int[GC.getNumBuildingInfos()];
		for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
		{
			m_aiShrineBuilding[iI] = (int) NO_BUILDING;
			m_aiShrineReligion[iI] = (int) NO_RELIGION;
		}

		FAssertMsg(m_aiSecretaryGeneralTimer==NULL, "about to leak memory, CvGame::m_aiSecretaryGeneralTimer");
		FAssertMsg(m_aiVoteTimer==NULL, "about to leak memory, CvGame::m_aiVoteTimer");
		m_aiSecretaryGeneralTimer = new int[GC.getNumVoteSourceInfos()];
		m_aiVoteTimer = new int[GC.getNumVoteSourceInfos()];
		for (iI = 0; iI < GC.getNumVoteSourceInfos(); iI++)
		{
			m_aiSecretaryGeneralTimer[iI] = 0;
			m_aiVoteTimer[iI] = 0;
		}
	}

	m_deals.removeAll();
	m_voteSelections.removeAll();
	m_votesTriggered.removeAll();

	m_mapRand.reset();
	m_sorenRand.reset();

	m_iNumSessions = 1;

	m_iShrineBuildingCount = 0;
	m_iNumCultureVictoryCities = 0;
	m_eCultureVictoryCultureLevel = NO_CULTURELEVEL;
/*************************************************************************************************/
/**	New Tag Defs	(ProjectInfos)			10/01/08								Xienwolf	**/
/**	New Tag Defs	(GameInfos)				10/01/08								Xienwolf	**/
/**										Initial Values											**/
/*************************************************************************************************/
	// First lair cycle is at a speed-fixed turn
	m_iNextLairCycle = 0;
	if (!bConstructorCall)
	{
		m_ppaaiProjectTimers = new int*[GC.getNumProjectInfos()];
		for (iI = 0; iI < GC.getNumProjectInfos(); iI++)
		{
			m_ppaaiProjectTimers[iI] = new int[GC.getMaxNumProjectsAllowed()];
			for (int iJ = 0; iJ < GC.getMaxNumProjectsAllowed(); iJ++)
			{
				m_ppaaiProjectTimers[iI][iJ] = -1;
			}
		}
	}
/*************************************************************************************************/
/**	New Tag Defs							END													**/
/*************************************************************************************************/

	if (!bConstructorCall)
	{
		AI_reset();
	}
}


void CvGame::initDiplomacy()
{
	PROFILE_FUNC();

	int iI, iJ;

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		GET_TEAM((TeamTypes)iI).meet(((TeamTypes)iI), false);

		if (GET_TEAM((TeamTypes)iI).isBarbarian() || GET_TEAM((TeamTypes)iI).isMinorCiv())
		{
			for (iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
			{
				if (iI != iJ)
				{

/*************************************************************************************************/
/**	MultiBarb								12/23/08								Xienwolf	**/
/**																								**/
/**							Adds extra Barbarian Civilizations									**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
//FfH: Modified by Kael 08/02/2007
//					GET_TEAM((TeamTypes)iI).declareWar(((TeamTypes)iJ), false, NO_WARPLAN);
					if (!GET_TEAM((TeamTypes)iJ).isBarbarianAlly())
/**								----  End Original Code  ----									**/
					if (!(GET_TEAM((TeamTypes)iJ).isBarbarianAlly() && iI == ORC_TEAM))
/*************************************************************************************************/
/**	MultiBarb								END													**/
/*************************************************************************************************/
					{
						GET_TEAM((TeamTypes)iI).declareWar(((TeamTypes)iJ), false, NO_WARPLAN);
					}
//FfH: End Add

				}
			}
		}
	}

	// Forced peace at the beginning of Advanced starts
	if (isOption(GAMEOPTION_ADVANCED_START))
	{
		CLinkList<TradeData> player1List;
		CLinkList<TradeData> player2List;
		TradeData kTradeData;
		setTradeItem(&kTradeData, TRADE_PEACE_TREATY);
		player1List.insertAtEnd(kTradeData);
		player2List.insertAtEnd(kTradeData);

		for (int iPlayer1 = 0; iPlayer1 < MAX_CIV_PLAYERS; ++iPlayer1)
		{
			CvPlayer& kLoopPlayer1 = GET_PLAYER((PlayerTypes)iPlayer1);

			if (kLoopPlayer1.isAlive())
			{
				for (int iPlayer2 = iPlayer1 + 1; iPlayer2 < MAX_CIV_PLAYERS; ++iPlayer2)
				{
					CvPlayer& kLoopPlayer2 = GET_PLAYER((PlayerTypes)iPlayer2);

					if (kLoopPlayer2.isAlive())
					{
						if (GET_TEAM(kLoopPlayer1.getTeam()).canChangeWarPeace(kLoopPlayer2.getTeam()))
						{
							implementDeal((PlayerTypes)iPlayer1, (PlayerTypes)iPlayer2, &player1List, &player2List);
						}
					}
				}
			}
		}
	}
}


void CvGame::initFreeState()
{
	bool bValid;
	int iI, iJ, iK;

	for (iI = 0; iI < GC.getNumTechInfos(); iI++)
	{
		for (iJ = 0; iJ < MAX_TEAMS; iJ++)
		{
			if (GET_TEAM((TeamTypes)iJ).isAlive())
			{
				bValid = false;

				if (!bValid)
				{
					if ((GC.getHandicapInfo(getHandicapType()).isFreeTechs(iI)) ||
						  (!(GET_TEAM((TeamTypes)iJ).isHuman())&& GC.getHandicapInfo(getHandicapType()).isAIFreeTechs(iI)) ||
						  (GC.getTechInfo((TechTypes)iI).getEra() < getStartEra()

//FfH: Added by Kael 09/13/2007
						  && GC.getTechInfo((TechTypes)iI).getResearchCost() != -1
//FfH: End Add

						  ))
					{
						bValid = true;
					}
				}

				if (!bValid)
				{
					for (iK = 0; iK < MAX_PLAYERS; iK++)
					{
						if (GET_PLAYER((PlayerTypes)iK).isAlive())
						{
							if (GET_PLAYER((PlayerTypes)iK).getTeam() == iJ)
							{
								if (GC.getCivilizationInfo(GET_PLAYER((PlayerTypes)iK).getCivilizationType()).isCivilizationFreeTechs(iI))
								{
									bValid = true;
									break;
								}
							}
						}
					}
				}

				GET_TEAM((TeamTypes)iJ).setHasTech(((TechTypes)iI), bValid, NO_PLAYER, false, false);
				if (bValid && GC.getTechInfo((TechTypes)iI).isMapVisible())
				{
					GC.getMapINLINE().setRevealedPlots((TeamTypes)iJ, true, true);
				}
			}
		}
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			GET_PLAYER((PlayerTypes)iI).initFreeState();
		}
	}
}


void CvGame::initFreeUnits()
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if ((GET_PLAYER((PlayerTypes)iI).getNumUnits() == 0) && (GET_PLAYER((PlayerTypes)iI).getNumCities() == 0))
			{
				GET_PLAYER((PlayerTypes)iI).initFreeUnits();
			}
		}
	}
}


void CvGame::assignStartingPlots()
{
	PROFILE_FUNC();

	CvPlot* pPlot;
	CvPlot* pBestPlot;
	bool bStartFound;
	bool bValid;
	int iRandOffset;
	int iLoopTeam;
	int iLoopPlayer;
	int iHumanSlot;
	int iValue;
	int iBestValue;
	int iI, iJ, iK;

	std::vector<int> playerOrder;
	std::vector<int>::iterator playerOrderIter;

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getStartingPlot() == NULL)
			{
				iBestValue = 0;
				pBestPlot = NULL;

				for (iJ = 0; iJ < GC.getMapINLINE().numPlotsINLINE(); iJ++)
				{
					gDLL->callUpdater();	// allow window updates during launch

					pPlot = GC.getMapINLINE().plotByIndexINLINE(iJ);

					if (pPlot->isStartingPlot())
					{
						bValid = true;

						for (iK = 0; iK < MAX_CIV_PLAYERS; iK++)
						{
							if (GET_PLAYER((PlayerTypes)iK).isAlive())
							{
								if (GET_PLAYER((PlayerTypes)iK).getStartingPlot() == pPlot)
								{
									bValid = false;
									break;
								}
							}
						}

						if (bValid)
						{
							iValue = (1 + getSorenRandNum(1000, "Starting Plot"));

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = pPlot;
							}
						}
					}
				}

				if (pBestPlot != NULL)
				{
					GET_PLAYER((PlayerTypes)iI).setStartingPlot(pBestPlot, true);
				}
			}
		}
	}

	if (gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "assignStartingPlots"))
	{
		if (!gDLL->getPythonIFace()->pythonUsingDefaultImpl())
		{
			// Python override
			return;
		}
	}

	if (isTeamGame())
	{
		for (int iPass = 0; iPass < 2 * MAX_PLAYERS; ++iPass)
		{
			bStartFound = false;

			iRandOffset = getSorenRandNum(countCivTeamsAlive(), "Team Starting Plot");

			for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
			{
				iLoopTeam = ((iI + iRandOffset) % MAX_CIV_TEAMS);

				if (GET_TEAM((TeamTypes)iLoopTeam).isAlive())
				{
					for (iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
					{
						if (GET_PLAYER((PlayerTypes)iJ).isAlive())
						{
							if (GET_PLAYER((PlayerTypes)iJ).getTeam() == iLoopTeam)
							{
								if (GET_PLAYER((PlayerTypes)iJ).getStartingPlot() == NULL)
								{
									CvPlot* pStartingPlot = GET_PLAYER((PlayerTypes)iJ).findStartingPlot();

									if (NULL != pStartingPlot)
									{
										GET_PLAYER((PlayerTypes)iJ).setStartingPlot(pStartingPlot, true);
										playerOrder.push_back(iJ);
									}
									bStartFound = true;
									break;
								}
							}
						}
					}
				}
			}

			if (!bStartFound)
			{
				break;
			}
		}

		//check all players have starting plots
		for (iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
		{
			FAssertMsg(!GET_PLAYER((PlayerTypes)iJ).isAlive() || GET_PLAYER((PlayerTypes)iJ).getStartingPlot() != NULL, "Player has no starting plot");
		}
	}
	else if (isGameMultiPlayer())
	{
		iRandOffset = getSorenRandNum(countCivPlayersAlive(), "Player Starting Plot");

		for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			iLoopPlayer = ((iI + iRandOffset) % MAX_CIV_PLAYERS);

			if (GET_PLAYER((PlayerTypes)iLoopPlayer).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iLoopPlayer).isHuman())
				{
					if (GET_PLAYER((PlayerTypes)iLoopPlayer).getStartingPlot() == NULL)
					{
						GET_PLAYER((PlayerTypes)iLoopPlayer).setStartingPlot(GET_PLAYER((PlayerTypes)iLoopPlayer).findStartingPlot(), true);
						playerOrder.push_back(iLoopPlayer);
					}
				}
			}
		}

		for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (!(GET_PLAYER((PlayerTypes)iI).isHuman()))
				{
					if (GET_PLAYER((PlayerTypes)iI).getStartingPlot() == NULL)
					{
						GET_PLAYER((PlayerTypes)iI).setStartingPlot(GET_PLAYER((PlayerTypes)iI).findStartingPlot(), true);
						playerOrder.push_back(iI);
					}
				}
			}
		}
	}
	else
	{
		iHumanSlot = range((((countCivPlayersAlive() - 1) * GC.getHandicapInfo(getHandicapType()).getStartingLocationPercent()) / 100), 0, (countCivPlayersAlive() - 1));

		for (iI = 0; iI < iHumanSlot; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (!(GET_PLAYER((PlayerTypes)iI).isHuman()))
				{
					if (GET_PLAYER((PlayerTypes)iI).getStartingPlot() == NULL)
					{
						GET_PLAYER((PlayerTypes)iI).setStartingPlot(GET_PLAYER((PlayerTypes)iI).findStartingPlot(), true);
						playerOrder.push_back(iI);
					}
				}
			}
		}

		for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).isHuman())
				{
					if (GET_PLAYER((PlayerTypes)iI).getStartingPlot() == NULL)
					{
						GET_PLAYER((PlayerTypes)iI).setStartingPlot(GET_PLAYER((PlayerTypes)iI).findStartingPlot(), true);
						playerOrder.push_back(iI);
					}
				}
			}
		}

		for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).getStartingPlot() == NULL)
				{
					GET_PLAYER((PlayerTypes)iI).setStartingPlot(GET_PLAYER((PlayerTypes)iI).findStartingPlot(), true);
					playerOrder.push_back(iI);
				}
			}
		}
	}

	//Now iterate over the player starts in the original order and re-place them.
	for (playerOrderIter = playerOrder.begin(); playerOrderIter != playerOrder.end(); ++playerOrderIter)
	{
		GET_PLAYER((PlayerTypes)(*playerOrderIter)).setStartingPlot(GET_PLAYER((PlayerTypes)(*playerOrderIter)).findStartingPlot(), true);
	}
}

// Swaps starting locations until we have reached the optimal closeness between teams
// (caveat: this isn't quite "optimal" because we could get stuck in local minima, but it's pretty good)

void CvGame::normalizeStartingPlotLocations()
{
	CvPlot* apNewStartPlots[MAX_CIV_PLAYERS];
	int* aaiDistances[MAX_CIV_PLAYERS];
	int aiStartingLocs[MAX_CIV_PLAYERS];
	int iI, iJ;

	// Precalculate distances between all starting positions:
	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			gDLL->callUpdater();	// allow window to update during launch
			aaiDistances[iI] = new int[iI];
			for (iJ = 0; iJ < iI; iJ++)
			{
				aaiDistances[iI][iJ] = 0;
			}
			CvPlot *pPlotI = GET_PLAYER((PlayerTypes)iI).getStartingPlot();
			if (pPlotI != NULL)
			{
				for (iJ = 0; iJ < iI; iJ++)
				{
					if (GET_PLAYER((PlayerTypes)iJ).isAlive())
					{
						CvPlot *pPlotJ = GET_PLAYER((PlayerTypes)iJ).getStartingPlot();
						if (pPlotJ != NULL)
						{
							int iDist = GC.getMapINLINE().calculatePathDistance(pPlotI, pPlotJ);
							if (iDist == -1)
							{
								// 5x penalty for not being on the same area, or having no passable route
								iDist = 5*plotDistance(pPlotI->getX_INLINE(), pPlotI->getY_INLINE(), pPlotJ->getX_INLINE(), pPlotJ->getY_INLINE());
							}
							aaiDistances[iI][iJ] = iDist;
						}
					}
				}
			}
		}
		else
		{
			aaiDistances[iI] = NULL;
		}
	}

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		aiStartingLocs[iI] = iI; // each player starting in own location
	}

	int iBestScore = getTeamClosenessScore(aaiDistances, aiStartingLocs);
	bool bFoundSwap = true;
	while (bFoundSwap)
	{
		bFoundSwap = false;
		for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				for (iJ = 0; iJ < iI; iJ++)
				{
					if (GET_PLAYER((PlayerTypes)iJ).isAlive())
					{
						int iTemp = aiStartingLocs[iI];
						aiStartingLocs[iI] = aiStartingLocs[iJ];
						aiStartingLocs[iJ] = iTemp;
						int iScore = getTeamClosenessScore(aaiDistances, aiStartingLocs);
						if (iScore < iBestScore)
						{
							iBestScore = iScore;
							bFoundSwap = true;
						}
						else
						{
							// Swap them back:
							iTemp = aiStartingLocs[iI];
							aiStartingLocs[iI] = aiStartingLocs[iJ];
							aiStartingLocs[iJ] = iTemp;
						}
					}
				}
			}
		}
	}

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		apNewStartPlots[iI] = NULL;
	}

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (aiStartingLocs[iI] != iI)
			{
				apNewStartPlots[iI] = GET_PLAYER((PlayerTypes)aiStartingLocs[iI]).getStartingPlot();
			}
		}
	}

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (apNewStartPlots[iI] != NULL)
			{
				GET_PLAYER((PlayerTypes)iI).setStartingPlot(apNewStartPlots[iI], false);
			}
		}
	}

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		SAFE_DELETE_ARRAY(aaiDistances[iI]);
	}
}


void CvGame::normalizeAddRiver()
{
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			CvPlot* pStartingPlot = GET_PLAYER((PlayerTypes)iI).getStartingPlot();

			if (pStartingPlot != NULL)
			{
				if (!pStartingPlot->isFreshWater())
				{
					// if we will be able to add a lake, then use old river code
					if (normalizeFindLakePlot((PlayerTypes)iI) != NULL)
					{
						CvMapGenerator::GetInstance().doRiver(pStartingPlot);
					}
					// otherwise, use new river code which is much more likely to succeed
					else
					{
						CvMapGenerator::GetInstance().addRiver(pStartingPlot);
					}

					// add floodplains to any desert tiles the new river passes through
					for (int iK = 0; iK < GC.getMapINLINE().numPlotsINLINE(); iK++)
					{
						CvPlot* pPlot = GC.getMapINLINE().plotByIndexINLINE(iK);
						FAssert(pPlot != NULL);

						for (int iJ = 0; iJ < GC.getNumFeatureInfos(); iJ++)
						{
							if (GC.getFeatureInfo((FeatureTypes)iJ).isRequiresRiver())
							{
								if (pPlot->canHaveFeature((FeatureTypes)iJ))
								{
									if (GC.getFeatureInfo((FeatureTypes)iJ).getAppearanceProbability() == 10000)
									{
										if (pPlot->getBonusType() != NO_BONUS)
										{
											pPlot->setBonusType(NO_BONUS);
										}
										pPlot->setFeatureType((FeatureTypes)iJ);
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


void CvGame::normalizeRemovePeaks()
{
	CvPlot* pStartingPlot;
	CvPlot* pLoopPlot;
	int iRange;
	int iDX, iDY;
	int iI;

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			pStartingPlot = GET_PLAYER((PlayerTypes)iI).getStartingPlot();

			if (pStartingPlot != NULL)
			{
/*************************************************************************************************/
/**	Flavour Mod								06/23/08								Jean Elcard **/
/**																								**/
/**		Removes Peaks only if adjacent to the starting Plot, rather than a 7x7 chunk of map		**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
				iRange = 3;
/**								----  End Original Code  ----									**/
				if (isOption(GAMEOPTION_FLAVOUR_START))
				{
					iRange = 0;
				}
				else
				{
					iRange = 1;
				}
/*************************************************************************************************/
/**	Flavour Mod								END													**/
/*************************************************************************************************/

				for (iDX = -(iRange); iDX <= iRange; iDX++)
				{
					for (iDY = -(iRange); iDY <= iRange; iDY++)
					{
						pLoopPlot = plotXY(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), iDX, iDY);

						if (pLoopPlot != NULL)
						{
							if (pLoopPlot->isPeak())
							{
								pLoopPlot->setPlotType(PLOT_HILLS);
							}
						}
					}
				}
			}
		}
	}
}

void CvGame::normalizeAddLakes()
{
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			CvPlot* pLakePlot = normalizeFindLakePlot((PlayerTypes)iI);
			if (pLakePlot != NULL)
			{
				pLakePlot->setPlotType(PLOT_OCEAN);
			}
		}
	}
}

CvPlot* CvGame::normalizeFindLakePlot(PlayerTypes ePlayer)
{
	if (!GET_PLAYER(ePlayer).isAlive())
	{
		return NULL;
	}

	CvPlot* pStartingPlot = GET_PLAYER(ePlayer).getStartingPlot();
	if (pStartingPlot != NULL)
	{
		if (!(pStartingPlot->isFreshWater()))
		{
			for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
			{
				CvPlot* pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), iJ);

				if (pLoopPlot != NULL)
				{
					if (!(pLoopPlot->isWater()))
					{
						if (!(pLoopPlot->isCoastalLand()))
						{
							if (!(pLoopPlot->isRiver()))
							{
								if (pLoopPlot->getBonusType() == NO_BONUS)
								{
									bool bStartingPlot = false;

									for (int iK = 0; iK < MAX_CIV_PLAYERS; iK++)
									{
										if (GET_PLAYER((PlayerTypes)iK).isAlive())
										{
											if (GET_PLAYER((PlayerTypes)iK).getStartingPlot() == pLoopPlot)
											{
												bStartingPlot = true;
												break;
											}
										}
									}

									if (!bStartingPlot)
									{
										return pLoopPlot;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return NULL;
}


void CvGame::normalizeRemoveBadFeatures()
{
	CvPlot* pStartingPlot;
	CvPlot* pLoopPlot;
	int iI, iJ;

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			pStartingPlot = GET_PLAYER((PlayerTypes)iI).getStartingPlot();

			if (pStartingPlot != NULL)
			{
				for (iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
				{
					pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), iJ);

					if (pLoopPlot != NULL)
					{
						if (pLoopPlot->getFeatureType() != NO_FEATURE)
						{
/*************************************************************************************************/
/**	CivPlotMods								03/31/09								Jean Elcard	**/
/**																								**/
/**						Consider Civilization-specific Feature Yield Changes.					**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
							if ((GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange(YIELD_FOOD) <= 0) &&
								(GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange(YIELD_PRODUCTION) <= 0))
/**								----  End Original Code  ----									**/
							if (((GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange(YIELD_FOOD) + GC.getCivilizationInfo(GET_PLAYER((PlayerTypes)iI).getCivilizationType()).getFeatureYieldChanges(pLoopPlot->getFeatureType(), YIELD_FOOD)) <= 0) &&
								((GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange(YIELD_PRODUCTION) + GC.getCivilizationInfo(GET_PLAYER((PlayerTypes)iI).getCivilizationType()).getFeatureYieldChanges(pLoopPlot->getFeatureType(), YIELD_PRODUCTION)) <= 0))
/*************************************************************************************************/
/**	Flavour Mod								END													**/
/*************************************************************************************************/
							{
								pLoopPlot->setFeatureType(NO_FEATURE);
							}
						}
					}
				}

				int iX, iY;
				int iCityRange = CITY_PLOTS_RADIUS;
				int iExtraRange = 2;
				int iMaxRange = iCityRange + iExtraRange;

				for (iX = -iMaxRange; iX <= iMaxRange; iX++)
				{
					for (iY = -iMaxRange; iY <= iMaxRange; iY++)
					{
						pLoopPlot = plotXY(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), iX, iY);
						if (pLoopPlot != NULL)
						{
							int iDistance = plotDistance(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());
							if (iDistance <= iMaxRange)
							{
								if (pLoopPlot->getFeatureType() != NO_FEATURE)
								{
/*************************************************************************************************/
/**	CivPlotMods								03/31/09								Jean Elcard	**/
/**																								**/
/**						Consider Civilization-specific Feature Yield Changes.					**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
									if ((GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange(YIELD_FOOD) <= 0) &&
										(GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange(YIELD_PRODUCTION) <= 0))
/**								----  End Original Code  ----									**/
									if (((GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange(YIELD_FOOD) + GC.getCivilizationInfo(GET_PLAYER((PlayerTypes)iI).getCivilizationType()).getFeatureYieldChanges(pLoopPlot->getFeatureType(), YIELD_FOOD)) <= 0) &&
										((GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange(YIELD_PRODUCTION) + GC.getCivilizationInfo(GET_PLAYER((PlayerTypes)iI).getCivilizationType()).getFeatureYieldChanges(pLoopPlot->getFeatureType(), YIELD_PRODUCTION)) <= 0))
/*************************************************************************************************/
/**	Flavour Mod								END													**/
/*************************************************************************************************/
									{
										if (pLoopPlot->isWater())
										{
											if (pLoopPlot->isAdjacentToLand() || (!(iDistance == iMaxRange) && (getSorenRandNum(2, "Remove Bad Feature") == 0)))
											{
												pLoopPlot->setFeatureType(NO_FEATURE);
											}
										}
										else
										{
											if (!(iDistance == iMaxRange) && (getSorenRandNum((2 + (pLoopPlot->getBonusType() == NO_BONUS) ? 0 : 2), "Remove Bad Feature") == 0))
											{
												pLoopPlot->setFeatureType(NO_FEATURE);
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


void CvGame::normalizeRemoveBadTerrain()
{
	CvPlot* pStartingPlot;
	CvPlot* pLoopPlot;
	int iI, iK;
	int iX, iY;

	int iTargetFood;
	int iTargetTotal;
	int iPlotFood;
	int iPlotProduction;


	int iCityRange = CITY_PLOTS_RADIUS;
	int iExtraRange = 1;
	int iMaxRange = iCityRange + iExtraRange;

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			pStartingPlot = GET_PLAYER((PlayerTypes)iI).getStartingPlot();

			if (pStartingPlot != NULL)
			{
				for (iX = -iMaxRange; iX <= iMaxRange; iX++)
				{
					for (iY = -iMaxRange; iY <= iMaxRange; iY++)
					{
						pLoopPlot = plotXY(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), iX, iY);
						if (pLoopPlot != NULL)
						{
							int iDistance = plotDistance(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());
							if (iDistance <= iMaxRange)
							{
								if (!(pLoopPlot->isWater()) && ((iDistance <= iCityRange) || (pLoopPlot->isCoastalLand()) || (0 == getSorenRandNum(1 + iDistance - iCityRange, "Map Upgrade Terrain Food"))))
								{
									iPlotFood = GC.getTerrainInfo(pLoopPlot->getTerrainType()).getYield(YIELD_FOOD);
									iPlotProduction = GC.getTerrainInfo(pLoopPlot->getTerrainType()).getYield(YIELD_PRODUCTION);
/*************************************************************************************************/
/**	CivPlotMods								03/23/09								Jean Elcard	**/
/**																								**/
/**					Consider Civilization-specific Terrain Yield Modifications.					**/
/*************************************************************************************************/
								//	iPlotFood += GC.getCivilizationInfo(GET_PLAYER((PlayerTypes)iI).getCivilizationType()).getTerrainYieldChanges(pLoopPlot->getTerrainType(), YIELD_FOOD);
								//	iPlotProduction += GC.getCivilizationInfo(GET_PLAYER((PlayerTypes)iI).getCivilizationType()).getTerrainYieldChanges(pLoopPlot->getTerrainType(), YIELD_PRODUCTION);
									iPlotFood += GET_PLAYER((PlayerTypes)iI).getTerrainYieldChange(pLoopPlot->getTerrainType(), YIELD_FOOD);
									iPlotProduction += GET_PLAYER((PlayerTypes)iI).getTerrainYieldChange(pLoopPlot->getTerrainType(), YIELD_PRODUCTION);
/*************************************************************************************************/
/**	CivPlotMods								END													**/
/*************************************************************************************************/
									if ((iPlotFood + iPlotProduction) <= 1)
									{
										iTargetFood = 1;
										iTargetTotal = 1;
										if (pLoopPlot->getBonusType(GET_PLAYER((PlayerTypes)iI).getTeam()) != NO_BONUS)
										{
											iTargetFood = 1;
											iTargetTotal = 2;
										}
										else if ((iPlotFood == 1) || (iDistance <= iCityRange))
										{
											iTargetFood = 1 + getSorenRandNum(2, "Map Upgrade Terrain Food");
											iTargetTotal = 2;
										}
										else
										{
											iTargetFood = pLoopPlot->isCoastalLand() ? 2 : 1;
											iTargetTotal = 2;
										}

										for (iK = 0; iK < GC.getNumTerrainInfos(); iK++)
										{
											if (!(GC.getTerrainInfo((TerrainTypes)iK).isWater()))
											{
/*************************************************************************************************/
/**	CivPlotMods								03/23/09								Jean Elcard	**/
/**																								**/
/**					Consider Civilization-specific Terrain Yield Modifications.					**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
												if ((GC.getTerrainInfo((TerrainTypes)iK).getYield(YIELD_FOOD) >= iTargetFood) &&
													(GC.getTerrainInfo((TerrainTypes)iK).getYield(YIELD_FOOD) + GC.getTerrainInfo((TerrainTypes)iK).getYield(YIELD_PRODUCTION)) == iTargetTotal)
/**								----  End Original Code  ----									**/
												if (((GC.getTerrainInfo((TerrainTypes)iK).getYield(YIELD_FOOD) + GET_PLAYER((PlayerTypes)iI).getTerrainYieldChange(pLoopPlot->getTerrainType(), YIELD_FOOD)) >= iTargetFood) &&
													 (GC.getTerrainInfo((TerrainTypes)iK).getYield(YIELD_FOOD) + GET_PLAYER((PlayerTypes)iI).getTerrainYieldChange(pLoopPlot->getTerrainType(), YIELD_FOOD) + GC.getTerrainInfo((TerrainTypes)iK).getYield(YIELD_PRODUCTION) + GET_PLAYER((PlayerTypes)iI).getTerrainYieldChange(pLoopPlot->getTerrainType(), YIELD_PRODUCTION)) == iTargetTotal)
//								if (((GC.getTerrainInfo((TerrainTypes)iK).getYield(YIELD_FOOD) + GC.getCivilizationInfo(GET_PLAYER((PlayerTypes)iI).getCivilizationType()).getTerrainYieldChanges(pLoopPlot->getTerrainType(), YIELD_FOOD)) >= iTargetFood) &&
	//												 (GC.getTerrainInfo((TerrainTypes)iK).getYield(YIELD_FOOD) + GC.getCivilizationInfo(GET_PLAYER((PlayerTypes)iI).getCivilizationType()).getTerrainYieldChanges(pLoopPlot->getTerrainType(), YIELD_FOOD) + GC.getTerrainInfo((TerrainTypes)iK).getYield(YIELD_PRODUCTION) + GC.getCivilizationInfo(GET_PLAYER((PlayerTypes)iI).getCivilizationType()).getTerrainYieldChanges(pLoopPlot->getTerrainType(), YIELD_PRODUCTION)) == iTargetTotal)
/*************************************************************************************************/
/**	CivPlotMods								END													**/
/*************************************************************************************************/
												{
													if ((pLoopPlot->getFeatureType() == NO_FEATURE) || GC.getFeatureInfo(pLoopPlot->getFeatureType()).isTerrain(iK))
													{

//FfH: Modified by Kael 08/02/2007
//                                                        pLoopPlot->setTerrainType((TerrainTypes)iK);
														if (GC.getTerrainInfo((TerrainTypes)iK).isNormalize())
														{
															pLoopPlot->setTerrainType((TerrainTypes)iK);
														}
//FfH: End Modify

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


void CvGame::normalizeAddFoodBonuses()
{
	bool bIgnoreLatitude = pythonIsBonusIgnoreLatitudes();

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			CvPlot* pStartingPlot = GET_PLAYER((PlayerTypes)iI).getStartingPlot();

			if (pStartingPlot != NULL)
			{
				int iFoodBonus = 0;
				int iGoodNatureTileCount = 0;

				for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
				{
					CvPlot* pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), iJ);

					if (pLoopPlot != NULL)
					{
						BonusTypes eBonus = pLoopPlot->getBonusType(GET_PLAYER((PlayerTypes)iI).getTeam());

						if (eBonus != NO_BONUS)
						{
							if (GC.getBonusInfo(eBonus).getYieldChange(YIELD_FOOD) > 0)
							{
								if ((GC.getBonusInfo(eBonus).getTechCityTrade() == NO_TECH) || (GC.getTechInfo((TechTypes)(GC.getBonusInfo(eBonus).getTechCityTrade())).getEra() <= getStartEra()))
								{
									if (pLoopPlot->isWater())
									{
										iFoodBonus += 2;
									}
									else
									{
										iFoodBonus += 3;
									}
								}
							}
/*************************************************************************************************/
/**	CivPlotMods								04/02/09								Jean Elcard	**/
/**																								**/
/**							Calculate Player-specific Nature Yields.							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
							else if (pLoopPlot->calculateBestNatureYield(YIELD_FOOD, GET_PLAYER((PlayerTypes)iI).getTeam()) >= 2)
/**								----  End Original Code  ----									**/
							else if (pLoopPlot->calculateBestNatureYield(YIELD_FOOD, (PlayerTypes)iI) >= 2)
/*************************************************************************************************/
/**	CivPlotMods								END													**/
/*************************************************************************************************/
							{
								iGoodNatureTileCount++;
							}
						}
						else
						{
/*************************************************************************************************/
/**	CivPlotMods								04/02/09								Jean Elcard	**/
/**																								**/
/**							Calculate Player-specific Nature Yields.							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
							if (pLoopPlot->calculateBestNatureYield(YIELD_FOOD, GET_PLAYER((PlayerTypes)iI).getTeam()) >= 3)
/**								----  End Original Code  ----									**/
							if (pLoopPlot->calculateBestNatureYield(YIELD_FOOD, (PlayerTypes)iI) >= 3)
/*************************************************************************************************/
/**	CivPlotMods								END													**/
/*************************************************************************************************/
							{
								iGoodNatureTileCount++;
							}
						}
					}
				}

				int iTargetFoodBonusCount = 3;
				iTargetFoodBonusCount += (iGoodNatureTileCount == 0) ? 2 : 0;

				for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
				{
					if (iFoodBonus >= iTargetFoodBonusCount)
					{
						break;
					}

					CvPlot* pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), iJ);

					if (pLoopPlot != NULL)
					{
						if (pLoopPlot != pStartingPlot)
						{
							if (pLoopPlot->getBonusType() == NO_BONUS)
							{
								for (int iK = 0; iK < GC.getNumBonusInfos(); iK++)
								{
									if (GC.getBonusInfo((BonusTypes)iK).isNormalize())
									{
										if (GC.getBonusInfo((BonusTypes)iK).getYieldChange(YIELD_FOOD) > 0)
										{
											if ((GC.getBonusInfo((BonusTypes)iK).getTechCityTrade() == NO_TECH) || (GC.getTechInfo((TechTypes)(GC.getBonusInfo((BonusTypes)iK).getTechCityTrade())).getEra() <= getStartEra()))
											{
												if (GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).isHasTech((TechTypes)(GC.getBonusInfo((BonusTypes)iK).getTechReveal())))
												{
													if (pLoopPlot->canHaveBonus(((BonusTypes)iK), bIgnoreLatitude))
													{
														pLoopPlot->setBonusType((BonusTypes)iK);
														if (pLoopPlot->isWater())
														{
															iFoodBonus += 2;
														}
														else
														{
															iFoodBonus += 3;
														}
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
}


void CvGame::normalizeAddGoodTerrain()
{
	CvPlot* pStartingPlot;
	CvPlot* pLoopPlot;
	bool bChanged;
	int iGoodPlot;
	int iI, iJ, iK;

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			pStartingPlot = GET_PLAYER((PlayerTypes)iI).getStartingPlot();

			if (pStartingPlot != NULL)
			{
				iGoodPlot = 0;

				for (iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
				{
					pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), iJ);

					if (pLoopPlot != NULL)
					{
						if (pLoopPlot != pStartingPlot)
						{
/*************************************************************************************************/
/**	CivPlotMods								04/02/09								Jean Elcard	**/
/**																								**/
/**							Calculate Player-specific Nature Yields.							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
							if ((pLoopPlot->calculateNatureYield(YIELD_FOOD, GET_PLAYER((PlayerTypes)iI).getTeam()) >= GC.getFOOD_CONSUMPTION_PER_POPULATION()) &&
								  (pLoopPlot->calculateNatureYield(YIELD_PRODUCTION, GET_PLAYER((PlayerTypes)iI).getTeam()) > 0))
/**								----  End Original Code  ----									**/
							if ((pLoopPlot->calculateNatureYield(YIELD_FOOD, (PlayerTypes)iI) >= GC.getFOOD_CONSUMPTION_PER_POPULATION()) &&
								  (pLoopPlot->calculateNatureYield(YIELD_PRODUCTION, (PlayerTypes)iI) > 0))
/*************************************************************************************************/
/**	CivPlotMods								END													**/
/*************************************************************************************************/
							{
								iGoodPlot++;
							}
						}
					}
				}

				for (iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
				{
					if (iGoodPlot >= 4)
					{
						break;
					}

					pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), iJ);

					if (pLoopPlot != NULL)
					{
						if (pLoopPlot != pStartingPlot)
						{
							if (!(pLoopPlot->isWater()))
							{
								if (!(pLoopPlot->isHills()))
								{
									if (pLoopPlot->getBonusType() == NO_BONUS)
									{
										bChanged = false;

/*************************************************************************************************/
/**	CivPlotMods								04/02/09								Jean Elcard	**/
/**																								**/
/**							Calculate Player-specific Nature Yields.							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
										if (pLoopPlot->calculateNatureYield(YIELD_FOOD, GET_PLAYER((PlayerTypes)iI).getTeam()) < GC.getFOOD_CONSUMPTION_PER_POPULATION())
/**								----  End Original Code  ----									**/
										if (pLoopPlot->calculateNatureYield(YIELD_FOOD, (PlayerTypes)iI) < GC.getFOOD_CONSUMPTION_PER_POPULATION())
/*************************************************************************************************/
/**	CivPlotMods								END													**/
/*************************************************************************************************/
										{
											for (iK = 0; iK < GC.getNumTerrainInfos(); iK++)
											{
												if (!(GC.getTerrainInfo((TerrainTypes)iK).isWater()))
												{
/*************************************************************************************************/
/**	CivPlotMods								03/23/09								Jean Elcard	**/
/**																								**/
/**					Consider Civilization-specific Terrain Yield Modifications.					**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
													if (GC.getTerrainInfo((TerrainTypes)iK).getYield(YIELD_FOOD) >= GC.getFOOD_CONSUMPTION_PER_POPULATION())
/**								----  End Original Code  ----									**/
									//				if ((GC.getTerrainInfo((TerrainTypes)iK).getYield(YIELD_FOOD) + GC.getCivilizationInfo(GET_PLAYER((PlayerTypes)iI).getCivilizationType()).getTerrainYieldChanges(pLoopPlot->getTerrainType(), YIELD_FOOD)) >= GC.getFOOD_CONSUMPTION_PER_POPULATION())
													if ((GC.getTerrainInfo((TerrainTypes)iK).getYield(YIELD_FOOD) + GET_PLAYER((PlayerTypes)iI).getTerrainYieldChange(pLoopPlot->getTerrainType(), YIELD_FOOD)) >= GC.getFOOD_CONSUMPTION_PER_POPULATION())
/*************************************************************************************************/
/**	CivPlotMods								END													**/
/*************************************************************************************************/
													{
														pLoopPlot->setTerrainType((TerrainTypes)iK);
														bChanged = true;
														break;
													}
												}
											}
										}

/*************************************************************************************************/
/**	CivPlotMods								04/02/09								Jean Elcard	**/
/**																								**/
/**							Calculate Player-specific Nature Yields.							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
										if (pLoopPlot->calculateNatureYield(YIELD_PRODUCTION, GET_PLAYER((PlayerTypes)iI).getTeam()) == 0)
/**								----  End Original Code  ----									**/
										if (pLoopPlot->calculateNatureYield(YIELD_PRODUCTION, (PlayerTypes)iI) == 0)
/*************************************************************************************************/
/**	CivPlotMods								END													**/
/*************************************************************************************************/
										{
											for (iK = 0; iK < GC.getNumFeatureInfos(); iK++)
											{
/*************************************************************************************************/
/**	CivPlotMods								03/23/09								Jean Elcard	**/
/**																								**/
/**					Consider Civilization-specific Terrain Yield Modifications.					**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
												if ((GC.getFeatureInfo((FeatureTypes)iK).getYieldChange(YIELD_FOOD) >= 0) &&
													  (GC.getFeatureInfo((FeatureTypes)iK).getYieldChange(YIELD_PRODUCTION) > 0))
/**								----  End Original Code  ----									**/
												if (((GC.getFeatureInfo((FeatureTypes)iK).getYieldChange(YIELD_FOOD) + GC.getCivilizationInfo(GET_PLAYER((PlayerTypes)iI).getCivilizationType()).getFeatureYieldChanges((FeatureTypes)iK, YIELD_FOOD)) >= 0) &&
													((GC.getFeatureInfo((FeatureTypes)iK).getYieldChange(YIELD_PRODUCTION) + GC.getCivilizationInfo(GET_PLAYER((PlayerTypes)iI).getCivilizationType()).getFeatureYieldChanges((FeatureTypes)iK, YIELD_PRODUCTION)) > 0))
/*************************************************************************************************/
/**	CivPlotMods								END													**/
/*************************************************************************************************/
												{
													if (GC.getFeatureInfo((FeatureTypes)iK).isTerrain(pLoopPlot->getTerrainType()))
													{
														pLoopPlot->setFeatureType((FeatureTypes)iK);
														bChanged = true;
														break;
													}
												}
											}
										}

										if (bChanged)
										{
											iGoodPlot++;
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


void CvGame::normalizeAddExtras()
{
	bool bIgnoreLatitude = pythonIsBonusIgnoreLatitudes();

	int iTotalValue = 0;
	int iPlayerCount = 0;
	int iBestValue = 0;
	int iWorstValue = MAX_INT;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			CvPlot* pStartingPlot = GET_PLAYER((PlayerTypes)iI).getStartingPlot();

			if (pStartingPlot != NULL)
			{
				int iValue = GET_PLAYER((PlayerTypes)iI).AI_foundValue(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), -1, true);
				iTotalValue += iValue;
				iPlayerCount++;

				iBestValue = std::max(iValue, iBestValue);
				iWorstValue = std::min(iValue, iWorstValue);
			}
		}
	}

	//iTargetValue = (iTotalValue + iBestValue) / (iPlayerCount + 1);
	int iTargetValue = (iBestValue * 4) / 5;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			gDLL->callUpdater();	// allow window to update during launch
			CvPlot* pStartingPlot = GET_PLAYER((PlayerTypes)iI).getStartingPlot();

			if (pStartingPlot != NULL)
			{
				int iCount = 0;
				int iFeatureCount = 0;
				int aiShuffle[NUM_CITY_PLOTS];
				shuffleArray(aiShuffle, NUM_CITY_PLOTS, getMapRand());

				for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
				{
					if (GET_PLAYER((PlayerTypes)iI).AI_foundValue(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), -1, true) >= iTargetValue)
					{
						break;
					}
					if (getSorenRandNum((iCount + 2), "Setting Feature Type") <= 1)
					{
						CvPlot* pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), aiShuffle[iJ]);

						if (pLoopPlot != NULL)
						{
							if (pLoopPlot != pStartingPlot)
							{
								if (pLoopPlot->getBonusType() == NO_BONUS)
								{
									if (pLoopPlot->getFeatureType() == NO_FEATURE)
									{
										for (int iK = 0; iK < GC.getNumFeatureInfos(); iK++)
										{
/*************************************************************************************************/
/**	CivPlotMods								03/23/09								Jean Elcard	**/
/**																								**/
/**					Consider Civilization-specific Terrain Yield Modifications.					**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
											if ((GC.getFeatureInfo((FeatureTypes)iK).getYieldChange(YIELD_FOOD) + GC.getFeatureInfo((FeatureTypes)iK).getYieldChange(YIELD_PRODUCTION)) > 0)
/**								----  End Original Code  ----									**/
											if ((GC.getFeatureInfo((FeatureTypes)iK).getYieldChange(YIELD_FOOD)
											   + GC.getCivilizationInfo(GET_PLAYER((PlayerTypes)iI).getCivilizationType()).getFeatureYieldChanges((FeatureTypes)iK, YIELD_FOOD)
											   + GC.getFeatureInfo((FeatureTypes)iK).getYieldChange(YIELD_PRODUCTION)
											   + GC.getCivilizationInfo(GET_PLAYER((PlayerTypes)iI).getCivilizationType()).getFeatureYieldChanges((FeatureTypes)iK, YIELD_PRODUCTION)) > 0)
/*************************************************************************************************/
/**	CivPlotMods								END													**/
/*************************************************************************************************/
											{
												if (pLoopPlot->canHaveFeature((FeatureTypes)iK))
												{
													pLoopPlot->setFeatureType((FeatureTypes)iK);
													iCount++;
													break;
												}
											}
										}
									}

									iFeatureCount += (pLoopPlot->getFeatureType() != NO_FEATURE) ? 1 : 0;
								}
							}
						}
					}
				}

				int iCoastFoodCount = 0;
				int iOceanFoodCount = 0;
				int iOtherCount = 0;
				int iWaterCount = 0;
				for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
				{
					CvPlot* pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), iJ);
					if (pLoopPlot != NULL)
					{
						if (pLoopPlot != pStartingPlot)
						{
							if (pLoopPlot->isWater())
							{
								iWaterCount++;
								if (pLoopPlot->getBonusType() != NO_BONUS)
								{
									if (pLoopPlot->isAdjacentToLand())
									{
										iCoastFoodCount++;
									}
									else
									{
										iOceanFoodCount++;
									}
								}
							}
							else
							{
								if (pLoopPlot->getBonusType() != NO_BONUS)
								{
									iOtherCount++;
								}
							}
						}
					}
				}

				bool bLandBias = (iWaterCount > NUM_CITY_PLOTS / 2);

				shuffleArray(aiShuffle, NUM_CITY_PLOTS, getMapRand());

				for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
				{
					CvPlot* pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), aiShuffle[iJ]);

					if ((pLoopPlot != NULL) && (pLoopPlot != pStartingPlot))
					{
						if (getSorenRandNum(((bLandBias && pLoopPlot->isWater()) ? 2 : 1), "Placing Bonuses") == 0)
						{
							if ((iOtherCount * 3 + iOceanFoodCount * 2 + iCoastFoodCount * 2) >= 12)
							{
								break;
							}

							if (GET_PLAYER((PlayerTypes)iI).AI_foundValue(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), -1, true) >= iTargetValue)
							{
								break;
							}

							bool bCoast = (pLoopPlot->isWater() && pLoopPlot->isAdjacentToLand());
							bool bOcean = (pLoopPlot->isWater() && !bCoast);
							if ((pLoopPlot != pStartingPlot)
								&& !(bCoast && (iCoastFoodCount > 2))
								&& !(bOcean && (iOceanFoodCount > 2)))
							{
								for (int iPass = 0; iPass < 2; iPass++)
								{
									if (pLoopPlot->getBonusType() == NO_BONUS)
									{
										for (int iK = 0; iK < GC.getNumBonusInfos(); iK++)
										{
											if (GC.getBonusInfo((BonusTypes)iK).isNormalize())
											{
												//???no bonuses with negative yields?
												if ((GC.getBonusInfo((BonusTypes)iK).getYieldChange(YIELD_FOOD) >= 0) &&
													  (GC.getBonusInfo((BonusTypes)iK).getYieldChange(YIELD_PRODUCTION) >= 0))
												{
													if ((GC.getBonusInfo((BonusTypes)iK).getTechCityTrade() == NO_TECH) || (GC.getTechInfo((TechTypes)(GC.getBonusInfo((BonusTypes)iK).getTechCityTrade())).getEra() <= getStartEra()))
													{
														if (GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).isHasTech((TechTypes)(GC.getBonusInfo((BonusTypes)iK).getTechReveal())))
														{
															if ((iPass == 0) ? CvMapGenerator::GetInstance().canPlaceBonusAt(((BonusTypes)iK), pLoopPlot->getX(), pLoopPlot->getY(), bIgnoreLatitude) : pLoopPlot->canHaveBonus(((BonusTypes)iK), bIgnoreLatitude))
															{
																pLoopPlot->setBonusType((BonusTypes)iK);
																iCoastFoodCount += bCoast ? 1 : 0;
																iOceanFoodCount += bOcean ? 1 : 0;
																iOtherCount += !(bCoast || bOcean) ? 1 : 0;
																break;
															}
														}
													}
												}
											}
										}

										if (bLandBias && !pLoopPlot->isWater() && pLoopPlot->getBonusType() == NO_BONUS)
										{
											if (((iFeatureCount > 4) && (pLoopPlot->getFeatureType() != NO_FEATURE))
												&& ((iCoastFoodCount + iOceanFoodCount) > 2))
											{
												if (getSorenRandNum(2, "Clear feature to add bonus") == 0)
												{
												pLoopPlot->setFeatureType(NO_FEATURE);

													for (iK = 0; iK < GC.getNumBonusInfos(); iK++)
													{
														if (GC.getBonusInfo((BonusTypes)iK).isNormalize())
														{
															//???no bonuses with negative yields?
															if ((GC.getBonusInfo((BonusTypes)iK).getYieldChange(YIELD_FOOD) >= 0) &&
																  (GC.getBonusInfo((BonusTypes)iK).getYieldChange(YIELD_PRODUCTION) >= 0))
															{
																if ((GC.getBonusInfo((BonusTypes)iK).getTechCityTrade() == NO_TECH) || (GC.getTechInfo((TechTypes)(GC.getBonusInfo((BonusTypes)iK).getTechCityTrade())).getEra() <= getStartEra()))
																{
																	if ((iPass == 0) ? CvMapGenerator::GetInstance().canPlaceBonusAt(((BonusTypes)iK), pLoopPlot->getX(), pLoopPlot->getY(), bIgnoreLatitude) : pLoopPlot->canHaveBonus(((BonusTypes)iK), bIgnoreLatitude))
																	{
																		pLoopPlot->setBonusType((BonusTypes)iK);
																		iOtherCount++;
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
				}

				shuffleArray(aiShuffle, NUM_CITY_PLOTS, getMapRand());

				for (iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
				{
					if (GET_PLAYER((PlayerTypes)iI).AI_foundValue(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), -1, true) >= iTargetValue)
					{
						break;
					}

					CvPlot* pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), aiShuffle[iJ]);

					if (pLoopPlot != NULL)
					{
						if (pLoopPlot != pStartingPlot)
						{
							if (pLoopPlot->getBonusType() == NO_BONUS)
							{
								if (pLoopPlot->getFeatureType() == NO_FEATURE)
								{
									for (int iK = 0; iK < GC.getNumFeatureInfos(); iK++)
									{
										if ((GC.getFeatureInfo((FeatureTypes)iK).getYieldChange(YIELD_FOOD) + GC.getFeatureInfo((FeatureTypes)iK).getYieldChange(YIELD_PRODUCTION)) > 0)
										{
											if (pLoopPlot->canHaveFeature((FeatureTypes)iK))
											{
												pLoopPlot->setFeatureType((FeatureTypes)iK);
												break;
											}
										}
									}
								}
							}
						}
					}
				}

				int iHillsCount = 0;

				for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
				{
					CvPlot* pLoopPlot =plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), iJ);
					if (pLoopPlot != NULL)
					{
						if (pLoopPlot->isHills())
						{
							iHillsCount++;
						}
					}
				}
				shuffleArray(aiShuffle, NUM_CITY_PLOTS, getMapRand());
				for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
				{
					if (iHillsCount >= 3)
					{
						break;
					}
					CvPlot* pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), aiShuffle[iJ]);
					if (pLoopPlot != NULL)
					{
						if (!pLoopPlot->isWater())
						{
							if (!pLoopPlot->isHills())
							{
								if ((pLoopPlot->getFeatureType() == NO_FEATURE) ||
									!GC.getFeatureInfo(pLoopPlot->getFeatureType()).isRequiresFlatlands())
								{
									if ((pLoopPlot->getBonusType() == NO_BONUS) ||
										GC.getBonusInfo(pLoopPlot->getBonusType()).isHills())
									{
										pLoopPlot->setPlotType(PLOT_HILLS, false, true);
										iHillsCount++;
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


void CvGame::normalizeStartingPlots()
{
	PROFILE_FUNC();

	if (!(GC.getInitCore().getWBMapScript()) || GC.getInitCore().getWBMapNoPlayers())
	{
/*************************************************************************************************/
/**	Flavour Mod								06/23/08								Jean Elcard **/
/**																								**/
/**				Forces game to ignore Normalization Routines in the DLL							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeStartingPlotLocations", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
		{
			normalizeStartingPlotLocations();
		}
/**								----  End Original Code  ----									**/
		if (!isOption(GAMEOPTION_FLAVOUR_START))
		{
			if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeStartingPlotLocations", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
			{
				normalizeStartingPlotLocations();
			}
		}
/*************************************************************************************************/
/**	Flavour Mod								END													**/
/*************************************************************************************************/
	}

	if (GC.getInitCore().getWBMapScript())
	{
		return;
	}

	if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeAddRiver", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
	{
		normalizeAddRiver();
	}

	if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeRemovePeaks", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
	{
		normalizeRemovePeaks();
	}

	if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeAddLakes", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
	{
		normalizeAddLakes();
	}

/*************************************************************************************************/
/**	Flavour Mod								06/23/08								Jean Elcard **/
/**																								**/
/**				Forces game to ignore Normalization Routines in the DLL							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeRemoveBadFeatures", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
	{
		normalizeRemoveBadFeatures();
	}

	if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeRemoveBadTerrain", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
	{
		normalizeRemoveBadTerrain();
	}

	if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeAddFoodBonuses", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
	{
		normalizeAddFoodBonuses();
	}

	if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeAddGoodTerrain", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
	{
		normalizeAddGoodTerrain();
	}
/**								----  End Original Code  ----									**/
	if (!isOption(GAMEOPTION_FLAVOUR_START))
	{
		if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeRemoveBadFeatures", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
		{
			normalizeRemoveBadFeatures();
		}

		if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeRemoveBadTerrain", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
		{
			normalizeRemoveBadTerrain();
		}

		if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeAddFoodBonuses", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
		{
			normalizeAddFoodBonuses();
		}

		if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeAddGoodTerrain", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
		{
			normalizeAddGoodTerrain();
		}
	}
/*************************************************************************************************/
/**	Flavour Mod								END													**/
/*************************************************************************************************/

	if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeAddExtras", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
	{
/*************************************************************************************************/
/**	Flavour Mod								06/23/08								Jean Elcard **/
/**																								**/
/**		Forces game to ignore Normalization Routines in the DLL, but allows Python!				**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		normalizeAddExtras();
/**								----  End Original Code  ----									**/
		if (!isOption(GAMEOPTION_FLAVOUR_START))
		{
			normalizeAddExtras();
		}
/*************************************************************************************************/
/**	Flavour Mod								END													**/
/*************************************************************************************************/
	}
}

// For each of n teams, let the closeness score for that team be the average distance of an edge between two players on that team.
// This function calculates the closeness score for each team and returns the sum of those n scores.
// The lower the result, the better "clumped" the players' starting locations are.
//
// Note: for the purposes of this function, player i will be assumed to start in the location of player aiStartingLocs[i]

int CvGame::getTeamClosenessScore(int** aaiDistances, int* aiStartingLocs)
{
	int iScore = 0;

	for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
	{
		if (GET_TEAM((TeamTypes)iTeam).isAlive())
		{
			int iTeamTotalDist = 0;
			int iNumEdges = 0;
			for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
			{
				if (GET_PLAYER((PlayerTypes)iPlayer).isAlive())
				{
					if (GET_PLAYER((PlayerTypes)iPlayer).getTeam() == (TeamTypes)iTeam)
					{
						for (int iOtherPlayer = 0; iOtherPlayer < iPlayer; iOtherPlayer++)
						{
							if (GET_PLAYER((PlayerTypes)iOtherPlayer).getTeam() == (TeamTypes)iTeam)
							{
								// Add the edge between these two players that are on the same team
								iNumEdges++;
								int iPlayerStart = aiStartingLocs[iPlayer];
								int iOtherPlayerStart = aiStartingLocs[iOtherPlayer];

								if (iPlayerStart < iOtherPlayerStart) // Make sure that iPlayerStart > iOtherPlayerStart
								{
									int iTemp = iPlayerStart;
									iPlayerStart = iOtherPlayerStart;
									iOtherPlayerStart = iTemp;
								}
								else if (iPlayerStart == iOtherPlayerStart)
								{
									FAssertMsg(false, "Two players are (hypothetically) assigned to the same starting location!");
								}
								iTeamTotalDist += aaiDistances[iPlayerStart][iOtherPlayerStart];
							}
						}
					}
				}
			}

			int iTeamScore;
			if (iNumEdges == 0)
			{
				iTeamScore = 0;
			}
			else
			{
				iTeamScore = iTeamTotalDist/iNumEdges; // the avg distance between team edges is the team score
			}

			iScore += iTeamScore;
		}
	}
	return iScore;
}


void CvGame::update()
{
	PROFILE("CvGame::update");

	if (!gDLL->GetWorldBuilderMode() || isInAdvancedStart())
	{
		sendPlayerOptions();

		// FfH Kael 10/15/2008 : Added below check for CvEventReporter
		// if (isNetworkMultiPlayer())
		// Blaze reverted 2025, to get deferCall func working in singleplayer to make updateLandmark function.....
		CyArgsList pyArgs;
		pyArgs.add(getTurnSlice());
		CvEventReporter::getInstance().genericEvent("gameUpdate", pyArgs.makeFunctionArgs());

		if (getTurnSlice() == 0)
		{
			gDLL->getEngineIFace()->AutoSave(true);
		}

		if (getNumGameTurnActive() == 0)
		{
			if (!isPbem() || !getPbemTurnSent())
			{
				doTurn();
			}
		}

		updateScore();

		updateWar();

		updateMoves();

		updateTimers();

		updateTurnTimer();

		AI_updateAssignWork();

		testAlive();

		if ((getAIAutoPlay() == 0) && !(gDLL->GetAutorun()) && GAMESTATE_EXTENDED != getGameState())
		{
			if (countHumanPlayersAlive() == 0)
			{
				setGameState(GAMESTATE_OVER);
			}
		}

		changeTurnSlice(1);
		setTurnSliceSinceBeginning(getTurnSliceSinceBeginning() + 1);


		if (NO_PLAYER != getActivePlayer() && GET_PLAYER(getActivePlayer()).getAdvancedStartPoints() >= 0 && !gDLL->getInterfaceIFace()->isInAdvancedStart())
		{
			gDLL->getInterfaceIFace()->setInAdvancedStart(true);
			gDLL->getInterfaceIFace()->setWorldBuilder(true);
		}
	}
}


void CvGame::updateScore(bool bForce)
{
	bool abPlayerScored[MAX_CIV_PLAYERS];
	bool abTeamScored[MAX_CIV_TEAMS];
	int iScore;
	int iBestScore;
	PlayerTypes eBestPlayer;
	TeamTypes eBestTeam;
	int iI, iJ, iK;

	if (!isScoreDirty() && !bForce)
	{
		return;
	}

	setScoreDirty(false);

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		abPlayerScored[iI] = false;
	}

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		iBestScore = MIN_INT;
		eBestPlayer = NO_PLAYER;

		for (iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
		{
			if (!abPlayerScored[iJ])
			{
				iScore = GET_PLAYER((PlayerTypes)iJ).calculateScore(false);

				if (iScore >= iBestScore)
				{
					iBestScore = iScore;
					eBestPlayer = (PlayerTypes)iJ;
				}
			}
		}

		abPlayerScored[eBestPlayer] = true;

		setRankPlayer(iI, eBestPlayer);
		setPlayerRank(eBestPlayer, iI);
		setPlayerScore(eBestPlayer, iBestScore);
		GET_PLAYER(eBestPlayer).updateScoreHistory(getGameTurn(), iBestScore);
	}

	for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		abTeamScored[iI] = false;
	}

	for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		iBestScore = MIN_INT;
		eBestTeam = NO_TEAM;

		for (iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
		{
			if (!abTeamScored[iJ])
			{
				iScore = 0;

				for (iK = 0; iK < MAX_CIV_PLAYERS; iK++)
				{
					if (GET_PLAYER((PlayerTypes)iK).getTeam() == iJ)
					{
						iScore += getPlayerScore((PlayerTypes)iK);
					}
				}

				if (iScore >= iBestScore)
				{
					iBestScore = iScore;
					eBestTeam = (TeamTypes)iJ;
				}
			}
		}

		abTeamScored[eBestTeam] = true;

		setRankTeam(iI, eBestTeam);
		setTeamRank(eBestTeam, iI);
		setTeamScore(eBestTeam, iBestScore);
	}
}

void CvGame::updatePlotGroups()
{
	PROFILE_FUNC();

	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			GET_PLAYER((PlayerTypes)iI).updatePlotGroups();
		}
	}
}


void CvGame::updateBuildingCommerce()
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			GET_PLAYER((PlayerTypes)iI).updateBuildingCommerce();
		}
	}
}


void CvGame::updateCitySight(bool bIncrement)
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			GET_PLAYER((PlayerTypes)iI).updateCitySight(bIncrement, false);
		}
	}

	updatePlotGroups();
}


void CvGame::updateTradeRoutes()
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			GET_PLAYER((PlayerTypes)iI).updateTradeRoutes();
		}
	}
}


void CvGame::testExtendedGame()
{
	int iI;

	if (getGameState() != GAMESTATE_OVER)
	{
		return;
	}

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).isHuman())
			{
				if (GET_PLAYER((PlayerTypes)iI).isExtendedGame())
				{
					setGameState(GAMESTATE_EXTENDED);
					break;
				}
			}
		}
	}
}


void CvGame::cityPushOrder(CvCity* pCity, OrderTypes eOrder, int iData, bool bAlt, bool bShift, bool bCtrl) const
{
	if (pCity->getProduction() > 0)
	{
		CvMessageControl::getInstance().sendPushOrder(pCity->getID(), eOrder, iData, bAlt, bShift, !bShift);
	}
	else if ((eOrder == ORDER_TRAIN) && (pCity->getProductionUnit() == iData))
	{
		CvMessageControl::getInstance().sendPushOrder(pCity->getID(), eOrder, iData, bAlt, !bCtrl, bCtrl);
	}
	else
	{
		CvMessageControl::getInstance().sendPushOrder(pCity->getID(), eOrder, iData, bAlt, bShift, bCtrl);
	}
}


void CvGame::selectUnit(CvUnit* pUnit, bool bClear, bool bToggle, bool bSound) const
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pEntityNode;
	CvSelectionGroup* pSelectionGroup;
	bool bSelectGroup;
	bool bGroup;

/*************************************************************************************************/
/**	Xienwolf Tweak							01/04/09											**/
/**				Prevents control of units you cannot actually see the location of				**/
/**					Prevents control of AIControl Units by the human COMPLETELY					**/
//Might be able to re-write this slightly so that instead of just returning, it will cycle to the next unit
//in the stack.  If that unit is not selectable (or NULL) return, otherwise, allow that unit to be selected
/*************************************************************************************************/
	if (pUnit->isBlind() && (!pUnit->plot()->isVisible(pUnit->getTeam(), false) || !pUnit->plot()->isRevealed(pUnit->getTeam(), false)))
	{
		return;
	}
	if (pUnit->isAIControl())
	{
		return;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
	if (gDLL->getInterfaceIFace()->getHeadSelectedUnit() == NULL)
	{
		bSelectGroup = true;
	}
	else if (gDLL->getInterfaceIFace()->getHeadSelectedUnit()->getGroup() != pUnit->getGroup())
	{
		bSelectGroup = true;
	}
	else if (pUnit->IsSelected() && !(gDLL->getInterfaceIFace()->mirrorsSelectionGroup()))
	{
		bSelectGroup = !bToggle;
	}
	else
	{
		bSelectGroup = false;
	}

	gDLL->getInterfaceIFace()->clearSelectedCities();

	if (bClear)
	{
		gDLL->getInterfaceIFace()->clearSelectionList();
		bGroup = false;
	}
	else
	{
		bGroup = gDLL->getInterfaceIFace()->mirrorsSelectionGroup();
	}

	if (bSelectGroup)
	{
		pSelectionGroup = pUnit->getGroup();

		gDLL->getInterfaceIFace()->selectionListPreChange();

		pEntityNode = pSelectionGroup->headUnitNode();

		while (pEntityNode != NULL)
		{
			FAssertMsg(::getUnit(pEntityNode->m_data), "null entity in selection group");
			gDLL->getInterfaceIFace()->insertIntoSelectionList(::getUnit(pEntityNode->m_data), false, bToggle, bGroup, bSound, true);

			pEntityNode = pSelectionGroup->nextUnitNode(pEntityNode);
		}

		gDLL->getInterfaceIFace()->selectionListPostChange();
	}
	else
	{
		gDLL->getInterfaceIFace()->insertIntoSelectionList(pUnit, false, bToggle, bGroup, bSound);
	}

	gDLL->getInterfaceIFace()->makeSelectionListDirty();
}


void CvGame::selectGroup(CvUnit* pUnit, bool bShift, bool bCtrl, bool bAlt) const
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvPlot* pUnitPlot;
	bool bGroup;

	FAssertMsg(pUnit != NULL, "pUnit == NULL unexpectedly");

	if (bAlt || bCtrl)
	{
		gDLL->getInterfaceIFace()->clearSelectedCities();

		if (!bShift)
		{
			gDLL->getInterfaceIFace()->clearSelectionList();
			bGroup = true;
		}
		else
		{
			bGroup = gDLL->getInterfaceIFace()->mirrorsSelectionGroup();
		}

		pUnitPlot = pUnit->plot();

		pUnitNode = pUnitPlot->headUnitNode();

		gDLL->getInterfaceIFace()->selectionListPreChange();

		while (pUnitNode != NULL)
		{
			pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = pUnitPlot->nextUnitNode(pUnitNode);

			if (pLoopUnit->getOwnerINLINE() == getActivePlayer())
			{
				if (pLoopUnit->canMove())
				{
					if (!isMPOption(MPOPTION_SIMULTANEOUS_TURNS) || getTurnSlice() - pLoopUnit->getLastMoveTurn() > GC.getDefineINT("MIN_TIMER_UNIT_DOUBLE_MOVES"))
					{
						if (bAlt || (pLoopUnit->getUnitType() == pUnit->getUnitType()))
						{
							gDLL->getInterfaceIFace()->insertIntoSelectionList(pLoopUnit, false, false, bGroup, false, true);
						}
					}
				}
			}
		}

		gDLL->getInterfaceIFace()->selectionListPostChange();
	}
	else
	{
		gDLL->getInterfaceIFace()->selectUnit(pUnit, !bShift, bShift, true);
	}
}


void CvGame::selectAll(CvPlot* pPlot) const
{
	CvUnit* pSelectUnit;
	CvUnit* pCenterUnit;

	pSelectUnit = NULL;

	if (pPlot != NULL)
	{
		pCenterUnit = pPlot->getDebugCenterUnit();

		if ((pCenterUnit != NULL) && (pCenterUnit->getOwnerINLINE() == getActivePlayer()))
		{
			pSelectUnit = pCenterUnit;
		}
	}

	if (pSelectUnit != NULL)
	{
		gDLL->getInterfaceIFace()->selectGroup(pSelectUnit, false, false, true);
	}
}


bool CvGame::selectionListIgnoreBuildingDefense() const
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pSelectedUnitNode;
	CvUnit* pSelectedUnit;
	bool bIgnoreBuilding;
	bool bAttackLandUnit;

	bIgnoreBuilding = false;
	bAttackLandUnit = false;

	pSelectedUnitNode = gDLL->getInterfaceIFace()->headSelectionListNode();

	while (pSelectedUnitNode != NULL)
	{
		pSelectedUnit = ::getUnit(pSelectedUnitNode->m_data);
		pSelectedUnitNode = gDLL->getInterfaceIFace()->nextSelectionListNode(pSelectedUnitNode);

		if (pSelectedUnit != NULL)
		{
			if (pSelectedUnit->ignoreBuildingDefense())
			{
				bIgnoreBuilding = true;
			}

			if ((pSelectedUnit->getDomainType() == DOMAIN_LAND) && pSelectedUnit->canAttack())
			{
				bAttackLandUnit = true;
			}
		}
	}

	if (!bIgnoreBuilding && !bAttackLandUnit)
	{
		if (getBestLandUnit() != NO_UNIT)
		{
			bIgnoreBuilding = GC.getUnitInfo(getBestLandUnit()).isIgnoreBuildingDefense();
		}
	}

	return bIgnoreBuilding;
}


void CvGame::implementDeal(PlayerTypes eWho, PlayerTypes eOtherWho, CLinkList<TradeData>* pOurList, CLinkList<TradeData>* pTheirList, bool bForce)
{
	CvDeal* pDeal;

	FAssertMsg(eWho != NO_PLAYER, "Who is not assigned a valid value");
	FAssertMsg(eOtherWho != NO_PLAYER, "OtherWho is not assigned a valid value");
	FAssertMsg(eWho != eOtherWho, "eWho is not expected to be equal with eOtherWho");

	pDeal = addDeal();
	pDeal->init(pDeal->getID(), eWho, eOtherWho);
	pDeal->addTrades(pOurList, pTheirList, !bForce);
	if ((pDeal->getLengthFirstTrades() == 0) && (pDeal->getLengthSecondTrades() == 0))
	{
		pDeal->kill();
	}
}


void CvGame::verifyDeals()
{
	CvDeal* pLoopDeal;
	int iLoop;

	for(pLoopDeal = firstDeal(&iLoop); pLoopDeal != NULL; pLoopDeal = nextDeal(&iLoop))
	{
		pLoopDeal->verify();
	}
}


/* Globeview configuration control:
If bStarsVisible, then there will be stars visible behind the globe when it is on
If bWorldIsRound, then the world will bend into a globe; otherwise, it will show up as a plane  */
void CvGame::getGlobeviewConfigurationParameters(TeamTypes eTeam, bool& bStarsVisible, bool& bWorldIsRound)
{
	if(GET_TEAM(eTeam).isMapCentering() || isCircumnavigated())
	{
		bStarsVisible = true;
		bWorldIsRound = true;
	}
	else
	{
		bStarsVisible = false;
		bWorldIsRound = false;
	}
}


int CvGame::getSymbolID(int iSymbol)
{
	return gDLL->getInterfaceIFace()->getSymbolID(iSymbol);
}


int CvGame::getAdjustedPopulationPercent(VictoryTypes eVictory) const
{
	int iPopulation;
	int iBestPopulation;
	int iNextBestPopulation;
	int iI;

	if (GC.getVictoryInfo(eVictory).getPopulationPercentLead() == 0)
	{
		return 0;
	}

	if (getTotalPopulation() == 0)
	{
		return 100;
	}

	iBestPopulation = 0;
	iNextBestPopulation = 0;

	for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			iPopulation = GET_TEAM((TeamTypes)iI).getTotalPopulation();

			if (iPopulation > iBestPopulation)
			{
				iNextBestPopulation = iBestPopulation;
				iBestPopulation = iPopulation;
			}
			else if (iPopulation > iNextBestPopulation)
			{
				iNextBestPopulation = iPopulation;
			}
		}
	}

	return std::min(100, (((iNextBestPopulation * 100) / getTotalPopulation()) + GC.getVictoryInfo(eVictory).getPopulationPercentLead()));
}


int CvGame::getProductionPerPopulation(HurryTypes eHurry)
{
	if (NO_HURRY == eHurry)
	{
		return 0;
	}
	return (GC.getHurryInfo(eHurry).getProductionPerPopulation() * 100) / std::max(1, GC.getGameSpeedInfo(getGameSpeedType()).getHurryPercent());
}


int CvGame::getAdjustedLandPercent(VictoryTypes eVictory) const
{
	int iPercent;

	if (GC.getVictoryInfo(eVictory).getLandPercent() == 0)
	{
		return 0;
	}

	iPercent = GC.getVictoryInfo(eVictory).getLandPercent();

	iPercent -= (countCivTeamsEverAlive() * 2);

	return std::max(iPercent, GC.getVictoryInfo(eVictory).getMinLandPercent());
}


bool CvGame::isTeamVote(VoteTypes eVote) const
{
	return (GC.getVoteInfo(eVote).isSecretaryGeneral() || GC.getVoteInfo(eVote).isVictory());
}


bool CvGame::isChooseElection(VoteTypes eVote) const
{
	return !(GC.getVoteInfo(eVote).isSecretaryGeneral());
}


bool CvGame::isTeamVoteEligible(TeamTypes eTeam, VoteSourceTypes eVoteSource) const
{
	CvTeam& kTeam = GET_TEAM(eTeam);

	if (kTeam.isForceTeamVoteEligible(eVoteSource))
	{
		return true;
	}

	if (!kTeam.isFullMember(eVoteSource))
	{
		return false;
	}

	int iCount = 0;
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iI);
		if (kLoopTeam.isAlive())
		{
			if (kLoopTeam.isForceTeamVoteEligible(eVoteSource))
			{
				++iCount;
			}
		}
	}

	int iExtraEligible = GC.getDefineINT("TEAM_VOTE_MIN_CANDIDATES") - iCount;
	if (iExtraEligible <= 0)
	{
		return false;
	}

	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (iI != eTeam)
		{
			CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iI);
			if (kLoopTeam.isAlive())
			{
				if (!kLoopTeam.isForceTeamVoteEligible(eVoteSource))
				{
					if (kLoopTeam.isFullMember(eVoteSource))
					{
						int iLoopVotes = kLoopTeam.getVotes(NO_VOTE, eVoteSource);
						int iVotes = kTeam.getVotes(NO_VOTE, eVoteSource);
						if (iLoopVotes > iVotes || (iLoopVotes == iVotes && iI < eTeam))
						{
							iExtraEligible--;
						}
					}
				}
			}
		}
	}

	return (iExtraEligible > 0);
}


int CvGame::countVote(const VoteTriggeredData& kData, PlayerVoteTypes eChoice) const
{
	int iCount = 0;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; ++iI)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (getPlayerVote(((PlayerTypes)iI), kData.getID()) == eChoice)
			{
				iCount += GET_PLAYER((PlayerTypes)iI).getVotes(kData.kVoteOption.eVote, kData.eVoteSource);
			}
		}
	}

	return iCount;
}


int CvGame::countPossibleVote(VoteTypes eVote, VoteSourceTypes eVoteSource) const
{
	int iCount;
	int iI;

	iCount = 0;

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		iCount += GET_PLAYER((PlayerTypes)iI).getVotes(eVote, eVoteSource);
	}

	return iCount;
}



TeamTypes CvGame::findHighestVoteTeam(const VoteTriggeredData& kData) const
{
	TeamTypes eBestTeam = NO_TEAM;
	int iBestCount = 0;

	if (isTeamVote(kData.kVoteOption.eVote))
	{
		for (int iI = 0; iI < MAX_CIV_TEAMS; ++iI)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
			{
				int iCount = countVote(kData, (PlayerVoteTypes)iI);

				if (iCount > iBestCount)
				{
					iBestCount = iCount;
					eBestTeam = (TeamTypes)iI;
				}
			}
		}
	}

	return eBestTeam;
}


int CvGame::getVoteRequired(VoteTypes eVote, VoteSourceTypes eVoteSource) const
{
	return ((countPossibleVote(eVote, eVoteSource) * GC.getVoteInfo(eVote).getPopulationThreshold()) / 100);
}


TeamTypes CvGame::getSecretaryGeneral(VoteSourceTypes eVoteSource) const
{
	int iI;

	if (!canHaveSecretaryGeneral(eVoteSource))
	{
		for (int iBuilding = 0; iBuilding < GC.getNumBuildingInfos(); ++iBuilding)
		{
			if (GC.getBuildingInfo((BuildingTypes)iBuilding).getVoteSourceType() == eVoteSource)
			{
				for (iI = 0; iI < MAX_CIV_PLAYERS; ++iI)
				{
					CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
					if (kLoopPlayer.isAlive())
					{
						if (kLoopPlayer.getBuildingClassCount((BuildingClassTypes)GC.getBuildingInfo((BuildingTypes)iBuilding).getBuildingClassType()) > 0)
						{
							ReligionTypes eReligion = getVoteSourceReligion(eVoteSource);
							if (NO_RELIGION == eReligion || kLoopPlayer.getStateReligion() == eReligion)
							{
								return kLoopPlayer.getTeam();
							}
						}
					}
				}
			}
		}
	}
	else
	{
		for (iI = 0; iI < GC.getNumVoteInfos(); iI++)
		{
			if (GC.getVoteInfo((VoteTypes)iI).isVoteSourceType(eVoteSource))
			{
				if (GC.getVoteInfo((VoteTypes)iI).isSecretaryGeneral())
				{
					if (isVotePassed((VoteTypes)iI))
					{
						return ((TeamTypes)(getVoteOutcome((VoteTypes)iI)));
					}
				}
			}
		}
	}


	return NO_TEAM;
}

bool CvGame::canHaveSecretaryGeneral(VoteSourceTypes eVoteSource) const
{
	for (int iI = 0; iI < GC.getNumVoteInfos(); iI++)
	{
		if (GC.getVoteInfo((VoteTypes)iI).isVoteSourceType(eVoteSource))
		{
			if (GC.getVoteInfo((VoteTypes)iI).isSecretaryGeneral())
			{
				return true;
			}
		}
	}

	return false;
}

void CvGame::clearSecretaryGeneral(VoteSourceTypes eVoteSource)
{
	for (int j = 0; j < GC.getNumVoteInfos(); ++j)
	{
		CvVoteInfo& kVote = GC.getVoteInfo((VoteTypes)j);

		if (kVote.isVoteSourceType(eVoteSource))
		{
			if (kVote.isSecretaryGeneral())
			{
				VoteTriggeredData kData;
				kData.eVoteSource = eVoteSource;
				kData.kVoteOption.eVote = (VoteTypes)j;
				kData.kVoteOption.iCityId = -1;
				kData.kVoteOption.szText.empty();
				kData.kVoteOption.ePlayer = NO_PLAYER;
				setVoteOutcome(kData, NO_PLAYER_VOTE);
				setSecretaryGeneralTimer(eVoteSource, 0);
			}
		}
	}
}

void CvGame::updateSecretaryGeneral()
{
	for (int i = 0; i < GC.getNumVoteSourceInfos(); ++i)
	{
		TeamTypes eSecretaryGeneral = getSecretaryGeneral((VoteSourceTypes)i);
		if (NO_TEAM != eSecretaryGeneral && !GET_TEAM(eSecretaryGeneral).isFullMember((VoteSourceTypes)i))
		{
			clearSecretaryGeneral((VoteSourceTypes)i);
		}
	}
}

int CvGame::countCivPlayersAlive() const
{
	int iCount = 0;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			iCount++;
		}
	}

	return iCount;
}


int CvGame::countCivPlayersEverAlive() const
{
	int iCount = 0;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes) iI);
		if (kPlayer.isEverAlive())
		{
			if (kPlayer.getParent() == NO_PLAYER)
			{
				iCount++;
			}
		}
	}

	return iCount;
}


int CvGame::countCivTeamsAlive() const
{
	int iCount = 0;

	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			iCount++;
		}
	}

	return iCount;
}


int CvGame::countCivTeamsEverAlive() const
{
	std::set<int> setTeamsEverAlive;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes) iI);
		if (kPlayer.isEverAlive())
		{
			if (kPlayer.getParent() == NO_PLAYER)
			{
				setTeamsEverAlive.insert(kPlayer.getTeam());
			}
		}
	}

	return setTeamsEverAlive.size();
}


int CvGame::countHumanPlayersAlive() const
{
	int iCount = 0;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes) iI);
		if (kPlayer.isAlive() && kPlayer.isHuman())
		{
			iCount++;
		}
	}

	return iCount;
}


int CvGame::countTotalCivPower()
{
	int iCount = 0;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes) iI);
		if (kPlayer.isAlive())
		{
			iCount += kPlayer.getPower();
		}
	}

	return iCount;
}


int CvGame::countTotalNukeUnits()
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes) iI);
		if (kPlayer.isAlive())
		{
			iCount += kPlayer.getNumNukeUnits();
		}
	}

	return iCount;
}


int CvGame::countKnownTechNumTeams(TechTypes eTech)
{
	int iCount = 0;

	for (int iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isEverAlive())
		{
			if (GET_TEAM((TeamTypes)iI).isHasTech(eTech))
			{
				iCount++;
			}
		}
	}

	return iCount;
}


int CvGame::getNumFreeBonuses(BuildingTypes eBuilding)
{
	if (GC.getBuildingInfo(eBuilding).getNumFreeBonuses() == -1)
	{
		return GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getNumFreeBuildingBonuses();
	}
	else
	{
		return GC.getBuildingInfo(eBuilding).getNumFreeBonuses();
	}
}


int CvGame::countReligionLevels(ReligionTypes eReligion)
{
	int iCount;
	int iI;

	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			iCount += GET_PLAYER((PlayerTypes)iI).getHasReligionCount(eReligion);
		}
	}

	return iCount;
}

int CvGame::countCorporationLevels(CorporationTypes eCorporation)
{
	int iCount = 0;

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kLoopPlayer.isAlive())
		{
			iCount += GET_PLAYER((PlayerTypes)iI).getHasCorporationCount(eCorporation);
		}
	}

	return iCount;
}

void CvGame::replaceCorporation(CorporationTypes eCorporation1, CorporationTypes eCorporation2)
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kLoopPlayer.isAlive())
		{
			int iIter;
			for (CvCity* pCity = kLoopPlayer.firstCity(&iIter); NULL != pCity; pCity = kLoopPlayer.nextCity(&iIter))
			{
				if (pCity->isHasCorporation(eCorporation1))
				{
					pCity->setHasCorporation(eCorporation1, false, false, false);
					pCity->setHasCorporation(eCorporation2, true, true);
				}
			}

			for (CvUnit* pUnit = kLoopPlayer.firstUnit(&iIter); NULL != pUnit; pUnit = kLoopPlayer.nextUnit(&iIter))
			{
				if (pUnit->getUnitInfo().getCorporationSpreads(eCorporation1) > 0)
				{
					pUnit->kill(false);
				}
			}
		}
	}
}


int CvGame::calculateReligionPercent(ReligionTypes eReligion) const
{
	CvCity* pLoopCity;
	int iCount;
	int iLoop;
	int iI;

	if (getTotalPopulation() == 0)
	{
		return 0;
	}

	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
			{
				if (pLoopCity->isHasReligion(eReligion))
				{
					iCount += ((pLoopCity->getPopulation() + (pLoopCity->getReligionCount() / 2)) / pLoopCity->getReligionCount());
				}
			}
		}
	}

	return ((iCount * 100) / getTotalPopulation());
}


int CvGame::goldenAgeLength() const
{
	int iLength;

	iLength = GC.getDefineINT("GOLDEN_AGE_LENGTH");

	iLength *= GC.getGameSpeedInfo(getGameSpeedType()).getGoldenAgePercent();
	iLength /= 100;

	return iLength;
}

int CvGame::victoryDelay(VictoryTypes eVictory) const
{
	FAssert(eVictory >= 0 && eVictory < GC.getNumVictoryInfos());

	int iLength = GC.getVictoryInfo(eVictory).getVictoryDelayTurns();

	iLength *= GC.getGameSpeedInfo(getGameSpeedType()).getVictoryDelayPercent();
	iLength /= 100;

	return iLength;
}



int CvGame::getImprovementUpgradeTime(ImprovementTypes eImprovement) const
{
	int iTime;

	iTime = GC.getImprovementInfo(eImprovement).getUpgradeTime();

	iTime *= GC.getGameSpeedInfo(getGameSpeedType()).getImprovementPercent();
	iTime /= 100;

	// They use Era in the WIERDEST places... original code below for posterity : Xienwolf 06/18/09
	// iTime *= GC.getEraInfo(getStartEra()).getImprovementPercent();
	// iTime /= 100;

	return iTime;
}


bool CvGame::canTrainNukes() const
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kPlayer.isAlive())
		{
			for (int iJ = 0; iJ < GC.getNumUnitClassInfos(); iJ++)
			{
				UnitTypes eUnit = (UnitTypes)GC.getCivilizationInfo(kPlayer.getCivilizationType()).getCivilizationUnits((UnitClassTypes)iJ);

				if (NO_UNIT != eUnit)
				{
					if (-1 != GC.getUnitInfo(eUnit).getNukeRange())
					{
						if (kPlayer.canTrain(eUnit))
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


EraTypes CvGame::getCurrentEra() const
{
	int iEra;
	int iCount;
	int iI;

	iEra = 0;
	iCount = 0;
/*************************************************************************************************/
/**	K-mod merger							16/02/12									Snarko	**/
/**																								**/
/**					Merging in features of K-mod, most notably the pathfinder					**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	for (iI = 0; iI < MAX_PLAYERS; iI++)
/**								----  End Original Code  ----									**/
	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++) // K-Mod (dont' count the barbarians)
/*************************************************************************************************/
/**	K-mod merger								END												**/
/*************************************************************************************************/
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			iEra += GET_PLAYER((PlayerTypes)iI).getCurrentEra();
			iCount++;
		}
	}

	if (iCount > 0)
	{
		return ((EraTypes)(iEra / iCount));
	}

	return NO_ERA;
}


TeamTypes CvGame::getActiveTeam() const
{
	if (getActivePlayer() == NO_PLAYER)
	{
		return NO_TEAM;
	}
	else
	{
		return (TeamTypes)GET_PLAYER(getActivePlayer()).getTeam();
	}
}


CivilizationTypes CvGame::getActiveCivilizationType() const
{
	if (getActivePlayer() == NO_PLAYER)
	{
		return NO_CIVILIZATION;
	}
	else
	{
		return (CivilizationTypes)GET_PLAYER(getActivePlayer()).getCivilizationType();
	}
}


bool CvGame::isNetworkMultiPlayer() const
{
	return GC.getInitCore().getMultiplayer();
}


bool CvGame::isGameMultiPlayer() const
{
	return (isNetworkMultiPlayer() || isPbem() || isHotSeat());
}


bool CvGame::isTeamGame() const
{
	FAssert(countCivPlayersAlive() >= countCivTeamsAlive());
	return (countCivPlayersAlive() > countCivTeamsAlive());
}


bool CvGame::isModem()
{
	return gDLL->IsModem();
}
void CvGame::setModem(bool bModem)
{
	if (bModem)
	{
		gDLL->ChangeINIKeyValue("CONFIG", "Bandwidth", "modem");
	}
	else
	{
		gDLL->ChangeINIKeyValue("CONFIG", "Bandwidth", "broadband");
	}

	gDLL->SetModem(bModem);
}


void CvGame::reviveActivePlayer()
{
	if (!(GET_PLAYER(getActivePlayer()).isAlive()))
	{
		setAIAutoPlay(0);

		GC.getInitCore().setSlotStatus(getActivePlayer(), SS_TAKEN);

		// Let Python handle it
		long lResult=0;
		CyArgsList argsList;
		argsList.add(getActivePlayer());

		gDLL->getPythonIFace()->callFunction(PYGameModule, "doReviveActivePlayer", argsList.makeFunctionArgs(), &lResult);
		if (lResult == 1)
		{
			return;
		}

		GET_PLAYER(getActivePlayer()).initUnit(((UnitTypes)0), 0, 0);
	}
}


int CvGame::getNumHumanPlayers()
{
	return GC.getInitCore().getNumHumans();
}


int CvGame::getGameTurn()
{
	return GC.getInitCore().getGameTurn();
}


void CvGame::setGameTurn(int iNewValue)
{
	if (getGameTurn() != iNewValue)
	{
		GC.getInitCore().setGameTurn(iNewValue);
		FAssert(getGameTurn() >= 0);

		updateBuildingCommerce();

		setScoreDirty(true);

		gDLL->getInterfaceIFace()->setDirty(TurnTimer_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
	}
}


void CvGame::incrementGameTurn()
{
	setGameTurn(getGameTurn() + 1);
}


int CvGame::getTurnYear(int iGameTurn)
{
	// moved the body of this method to Game Core Utils so we have access for other games than the current one (replay screen in HOF)
	return getTurnYearForGame(iGameTurn, getStartYear(), getCalendar(), getGameSpeedType());
}


int CvGame::getGameTurnYear()
{
	return getTurnYear(getGameTurn());
}


int CvGame::getElapsedGameTurns() const
{
	return m_iElapsedGameTurns;
}


void CvGame::incrementElapsedGameTurns()
{
	m_iElapsedGameTurns++;
}


int CvGame::getMaxTurns() const
{
	return GC.getInitCore().getMaxTurns();
}


void CvGame::setMaxTurns(int iNewValue)
{
	GC.getInitCore().setMaxTurns(iNewValue);
	FAssert(getMaxTurns() >= 0);
}


void CvGame::changeMaxTurns(int iChange)
{
	setMaxTurns(getMaxTurns() + iChange);
}


int CvGame::getMaxCityElimination() const
{
	return GC.getInitCore().getMaxCityElimination();
}


void CvGame::setMaxCityElimination(int iNewValue)
{
	GC.getInitCore().setMaxCityElimination(iNewValue);
	FAssert(getMaxCityElimination() >= 0);
}

int CvGame::getNumAdvancedStartPoints() const
{
	return GC.getInitCore().getNumAdvancedStartPoints();
}


void CvGame::setNumAdvancedStartPoints(int iNewValue)
{
	GC.getInitCore().setNumAdvancedStartPoints(iNewValue);
	FAssert(getNumAdvancedStartPoints() >= 0);
}

int CvGame::getStartTurn() const
{
	return m_iStartTurn;
}


void CvGame::setStartTurn(int iNewValue)
{
	m_iStartTurn = iNewValue;
}


int CvGame::getStartYear() const
{
	return m_iStartYear;
}


void CvGame::setStartYear(int iNewValue)
{
	m_iStartYear = iNewValue;
}


int CvGame::getEstimateEndTurn() const
{
	return m_iEstimateEndTurn;
}


void CvGame::setEstimateEndTurn(int iNewValue)
{
	m_iEstimateEndTurn = iNewValue;
}


int CvGame::getTurnSlice() const
{
	return m_iTurnSlice;
}

int CvGame::getTurnSliceSinceBeginning() const
{
	return m_iTurnSliceSinceBeginning;
}


int CvGame::getMinutesPlayed() const
{
	return (getTurnSlice() / gDLL->getTurnsPerMinute());
}


void CvGame::setTurnSlice(int iNewValue)
{
	m_iTurnSlice = iNewValue;
}
void CvGame::setTurnSliceSinceBeginning(int iNewValue)
{
	m_iTurnSliceSinceBeginning = iNewValue;
}

void CvGame::changeTurnSlice(int iChange)
{
	setTurnSlice(getTurnSlice() + iChange);
}


int CvGame::getCutoffSlice() const
{
	return m_iCutoffSlice;
}


void CvGame::setCutoffSlice(int iNewValue)
{
	m_iCutoffSlice = iNewValue;
}


void CvGame::changeCutoffSlice(int iChange)
{
	setCutoffSlice(getCutoffSlice() + iChange);
}


int CvGame::getTurnSlicesRemaining()
{
	return (getCutoffSlice() - getTurnSlice());
}


void CvGame::resetTurnTimer()
{
	// We should only use the turn timer if we are in multiplayer
	if (isMPOption(MPOPTION_TURN_TIMER))
	{
		if (getElapsedGameTurns() > 0 || !isOption(GAMEOPTION_ADVANCED_START))
		{
			// Determine how much time we should allow
			int iTurnLen = getMaxTurnLen();
			if (getElapsedGameTurns() == 0 && !isPitboss())
			{
				// Let's allow more time for the initial turn
				TurnTimerTypes eTurnTimer = GC.getInitCore().getTurnTimer();
				FAssertMsg(eTurnTimer >= 0 && eTurnTimer < GC.getNumTurnTimerInfos(), "Invalid TurnTimer selection in InitCore");
				iTurnLen = (iTurnLen * GC.getTurnTimerInfo(eTurnTimer).getFirstTurnMultiplier());
			}
			// Set the current turn slice to start the 'timer'
			setCutoffSlice(getTurnSlice() + iTurnLen);
		}
	}
}

void CvGame::incrementTurnTimer(int iNumTurnSlices)
{
	if (isMPOption(MPOPTION_TURN_TIMER))
	{
		// If the turn timer has expired, we shouldn't increment it as we've sent our turn complete message
		if (getTurnSlice() <= getCutoffSlice())
		{
			changeCutoffSlice(iNumTurnSlices);
		}
	}
}


int CvGame::getMaxTurnLen()
{
	if (isPitboss())
	{
		// Use the user provided input
		// Turn time is in hours
		return ( getPitbossTurnTime() * 3600 * 4);
	}
	else
	{
		int iMaxUnits = 0;
		int iMaxCities = 0;

		// Find out who has the most units and who has the most cities
		// Calculate the max turn time based on the max number of units and cities
		for (int i = 0; i < MAX_CIV_PLAYERS; ++i)
		{
			if (GET_PLAYER((PlayerTypes)i).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)i).getNumUnits() > iMaxUnits)
				{
					iMaxUnits = GET_PLAYER((PlayerTypes)i).getNumUnits();
				}
				if (GET_PLAYER((PlayerTypes)i).getNumCities() > iMaxCities)
				{
					iMaxCities = GET_PLAYER((PlayerTypes)i).getNumCities();
				}
			}
		}

		// Now return turn len based on base len and unit and city bonuses
		TurnTimerTypes eTurnTimer = GC.getInitCore().getTurnTimer();
		FAssertMsg(eTurnTimer >= 0 && eTurnTimer < GC.getNumTurnTimerInfos(), "Invalid TurnTimer Selection in InitCore");
		return ( GC.getTurnTimerInfo(eTurnTimer).getBaseTime() +
				(GC.getTurnTimerInfo(eTurnTimer).getCityBonus()*iMaxCities) +
				(GC.getTurnTimerInfo(eTurnTimer).getUnitBonus()*iMaxUnits) );
	}
}


int CvGame::getTargetScore() const
{
	return GC.getInitCore().getTargetScore();
}


void CvGame::setTargetScore(int iNewValue)
{
	GC.getInitCore().setTargetScore(iNewValue);
	FAssert(getTargetScore() >= 0);
}


int CvGame::getNumGameTurnActive()
{
	return m_iNumGameTurnActive;
}


int CvGame::countNumHumanGameTurnActive() const
{
	int iCount;
	int iI;

	iCount = 0;

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isHuman())
		{
			if (GET_PLAYER((PlayerTypes)iI).isTurnActive())
			{
				iCount++;
			}
		}
	}

	return iCount;
}


void CvGame::changeNumGameTurnActive(int iChange)
{
	m_iNumGameTurnActive = (m_iNumGameTurnActive + iChange);
	FAssert(getNumGameTurnActive() >= 0);
}


int CvGame::getNumCities() const
{
	return m_iNumCities;
}


int CvGame::getNumCivCities() const
{
/*************************************************************************************************/
/**	MultiBarb							12/23/08									Xienwolf	**/
/**																								**/
/**							Adds extra Barbarian Civilizations									**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	return (getNumCities() - GET_PLAYER(BARBARIAN_PLAYER).getNumCities());
/**								----  End Original Code  ----									**/
	return (getNumCities() - GET_PLAYER(ORC_PLAYER).getNumCities() - GET_PLAYER(ANIMAL_PLAYER).getNumCities() - GET_PLAYER(DEMON_PLAYER).getNumCities());
/*************************************************************************************************/
/**	MultiBarb								END													**/
/*************************************************************************************************/
}


void CvGame::changeNumCities(int iChange)
{
	m_iNumCities = (m_iNumCities + iChange);
	FAssert(getNumCities() >= 0);
}


int CvGame::getTotalPopulation() const
{
	return m_iTotalPopulation;
}


void CvGame::changeTotalPopulation(int iChange)
{
	m_iTotalPopulation = (m_iTotalPopulation + iChange);
	FAssert(getTotalPopulation() >= 0);
}


int CvGame::getTradeRoutes() const
{
	return m_iTradeRoutes;
}


void CvGame::changeTradeRoutes(int iChange)
{
	if (iChange != 0)
	{
		m_iTradeRoutes = (m_iTradeRoutes + iChange);
		FAssert(getTradeRoutes() >= 0);

		updateTradeRoutes();
	}
}


int CvGame::getFreeTradeCount() const
{
	return m_iFreeTradeCount;
}


bool CvGame::isFreeTrade() const
{
	return (getFreeTradeCount() > 0);
}


void CvGame::changeFreeTradeCount(int iChange)
{
	bool bOldFreeTrade;

	if (iChange != 0)
	{
		bOldFreeTrade = isFreeTrade();

		m_iFreeTradeCount = (m_iFreeTradeCount + iChange);
		FAssert(getFreeTradeCount() >= 0);

		if (bOldFreeTrade != isFreeTrade())
		{
			updateTradeRoutes();
		}
	}
}


int CvGame::getNoNukesCount() const
{
	return m_iNoNukesCount;
}


bool CvGame::isNoNukes() const
{
	return (getNoNukesCount() > 0);
}


void CvGame::changeNoNukesCount(int iChange)
{
	m_iNoNukesCount = (m_iNoNukesCount + iChange);
	FAssert(getNoNukesCount() >= 0);
}


int CvGame::getSecretaryGeneralTimer(VoteSourceTypes eVoteSource) const
{
	FAssert(eVoteSource >= 0);
	FAssert(eVoteSource < GC.getNumVoteSourceInfos());
	return m_aiSecretaryGeneralTimer[eVoteSource];
}


void CvGame::setSecretaryGeneralTimer(VoteSourceTypes eVoteSource, int iNewValue)
{
	FAssert(eVoteSource >= 0);
	FAssert(eVoteSource < GC.getNumVoteSourceInfos());
	m_aiSecretaryGeneralTimer[eVoteSource] = iNewValue;
	FAssert(getSecretaryGeneralTimer(eVoteSource) >= 0);
}


void CvGame::changeSecretaryGeneralTimer(VoteSourceTypes eVoteSource, int iChange)
{
	setSecretaryGeneralTimer(eVoteSource, getSecretaryGeneralTimer(eVoteSource) + iChange);
}


int CvGame::getVoteTimer(VoteSourceTypes eVoteSource) const
{
	FAssert(eVoteSource >= 0);
	FAssert(eVoteSource < GC.getNumVoteSourceInfos());
	return m_aiVoteTimer[eVoteSource];
}


void CvGame::setVoteTimer(VoteSourceTypes eVoteSource, int iNewValue)
{
	FAssert(eVoteSource >= 0);
	FAssert(eVoteSource < GC.getNumVoteSourceInfos());
	m_aiVoteTimer[eVoteSource] = iNewValue;
	FAssert(getVoteTimer(eVoteSource) >= 0);
}


void CvGame::changeVoteTimer(VoteSourceTypes eVoteSource, int iChange)
{
	setVoteTimer(eVoteSource, getVoteTimer(eVoteSource) + iChange);
}


int CvGame::getNukesExploded() const
{
	return m_iNukesExploded;
}


void CvGame::changeNukesExploded(int iChange)
{
	m_iNukesExploded = (m_iNukesExploded + iChange);
}


int CvGame::getMaxPopulation() const
{
	return m_iMaxPopulation;
}


int CvGame::getMaxLand() const
{
	return m_iMaxLand;
}


int CvGame::getMaxTech() const
{
	return m_iMaxTech;
}


int CvGame::getMaxWonders() const
{
	return m_iMaxWonders;
}


int CvGame::getInitPopulation() const
{
	return m_iInitPopulation;
}


int CvGame::getInitLand() const
{
	return m_iInitLand;
}


int CvGame::getInitTech() const
{
	return m_iInitTech;
}


int CvGame::getInitWonders() const
{
	return m_iInitWonders;
}


void CvGame::initScoreCalculation()
{
	// initialize score calculation
	int iMaxFood = 0;
	for (int i = 0; i < GC.getMapINLINE().numPlotsINLINE(); i++)
	{
		CvPlot* pPlot = GC.getMapINLINE().plotByIndexINLINE(i);
		if (!pPlot->isWater() || pPlot->isAdjacentToLand())
		{
			iMaxFood += pPlot->calculateBestNatureYield(YIELD_FOOD, NO_TEAM);
		}
	}
	m_iMaxPopulation = getPopulationScore(iMaxFood / std::max(1, GC.getFOOD_CONSUMPTION_PER_POPULATION()));
	m_iMaxLand = getLandPlotsScore(GC.getMapINLINE().getLandPlots());
	m_iMaxTech = 0;
	for (int i = 0; i < GC.getNumTechInfos(); i++)
	{
		m_iMaxTech += getTechScore((TechTypes)i);
	}
	m_iMaxWonders = 0;
	for (int i = 0; i < GC.getNumBuildingClassInfos(); i++)
	{
		m_iMaxWonders += getWonderScore((BuildingClassTypes)i);
	}

	if (NO_ERA != getStartEra())
	{
		int iNumSettlers = GC.getEraInfo(getStartEra()).getStartingUnitMultiplier();
		m_iInitPopulation = getPopulationScore(iNumSettlers * (GC.getEraInfo(getStartEra()).getFreePopulation() + 1));

//FfH: Modified by Kael 11/18/2007
//		m_iInitLand = getLandPlotsScore(iNumSettlers *  NUM_CITY_PLOTS);
		m_iInitLand = getLandPlotsScore(iNumSettlers *  21);
//FfH: End Modify

	}
	else
	{
		m_iInitPopulation = 0;
		m_iInitLand = 0;
	}

	m_iInitTech = 0;

//FfH: Modified by Kael 01/31/2009
//	for (int i = 0; i < GC.getNumTechInfos(); i++)
//	{
//		if (GC.getTechInfo((TechTypes)i).getEra() < getStartEra())
//		{
//			m_iInitTech += getTechScore((TechTypes)i);
//		}
//		else
//		{
//			// count all possible free techs as initial to lower the score from immediate retirement
//			for (int iCiv = 0; iCiv < GC.getNumCivilizationInfos(); iCiv++)
//			{
//				if (GC.getCivilizationInfo((CivilizationTypes)iCiv).isPlayable())
//				{
//					if (GC.getCivilizationInfo((CivilizationTypes)iCiv).isCivilizationFreeTechs(i))
//					{
//						m_iInitTech += getTechScore((TechTypes)i);
//						break;
//					}
//				}
//			}
//		}
//	}
//FfH: End Modify

	m_iInitWonders = 0;
}


int CvGame::getAIAutoPlay()
{
	return m_iAIAutoPlay;
}


void CvGame::setAIAutoPlay(int iNewValue)
{
	int iOldValue;

	iOldValue = getAIAutoPlay();

	if (iOldValue != iNewValue)
	{
		m_iAIAutoPlay = std::max(0, iNewValue);

/*************************************************************************************************/
/**	Xienwolf Tweak							04/02/09											**/
/**																								**/
/**						Allows Multiplayer use of the AutoPlay function							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
//FfH: Modified by Kael 05/29/2008 (for jdog5000's AIAutoPlay)
//		if ((iOldValue == 0) && (getAIAutoPlay() > 0))
//		{
//			GET_PLAYER(getActivePlayer()).killUnits();
//			GET_PLAYER(getActivePlayer()).killCities();
//		}
		GET_PLAYER(getActivePlayer()).setDisableHuman((getAIAutoPlay() != 0));
//FfH: End Modify
/**								----  End Original Code  ----									**/
		for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
		{
			GET_PLAYER((PlayerTypes)iPlayer).setDisableHuman((getAIAutoPlay() != 0));
		}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

	}
}


void CvGame::changeAIAutoPlay(int iChange)
{
	setAIAutoPlay(getAIAutoPlay() + iChange);

/*************************************************************************************************/
/**	Xienwolf Tweak							04/02/09											**/
/**																								**/
/**						Allows Multiplayer use of the AutoPlay function							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
//FfH: Added by Kael 05/29/2008 (for jdog5000's AIAutoPlay)
	GET_PLAYER(getActivePlayer()).setDisableHuman((getAIAutoPlay() != 0));
//FfH: End Add
/**								----  End Original Code  ----									**/
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

}

/*************************************************************************************************/
/**	xUPT								02/08/11									Afforess	**/
/**																								**/
/**						xUPT mechanic, ported and modified by Valkrionn							**/
/*************************************************************************************************/
int CvGame::getUPT()
{
	return m_iUPT;
}

void CvGame::setUPT(int iValue)
{
	m_iUPT = std::max(0, iValue);
}

bool CvGame::isUPTLock()
{
	return m_bUPTLock;
}

void CvGame::setUPTLock(bool bNewValue)
{
	m_bUPTLock = bNewValue;
}
/*************************************************************************************************/
/**	xUPT									END													**/
/*************************************************************************************************/

unsigned int CvGame::getInitialTime()
{
	return m_uiInitialTime;
}


void CvGame::setInitialTime(unsigned int uiNewValue)
{
	m_uiInitialTime = uiNewValue;
}


bool CvGame::isScoreDirty() const
{
	return m_bScoreDirty;
}


void CvGame::setScoreDirty(bool bNewValue)
{
	m_bScoreDirty = bNewValue;
}


bool CvGame::isCircumnavigated() const
{
	return m_bCircumnavigated;
}


void CvGame::makeCircumnavigated()
{
	m_bCircumnavigated = true;
}

bool CvGame::circumnavigationAvailable() const
{
	if (isCircumnavigated())
	{
		return false;
	}

	if (GC.getDefineINT("CIRCUMNAVIGATE_FREE_MOVES") == 0)
	{
		return false;
	}

	CvMap& kMap = GC.getMapINLINE();

	if (!(kMap.isWrapXINLINE()) && !(kMap.isWrapYINLINE()))
	{
		return false;
	}

	if (kMap.getLandPlots() > ((kMap.numPlotsINLINE() * 2) / 3))
	{
		return false;
	}

	return true;
}

bool CvGame::isDiploVote(VoteSourceTypes eVoteSource) const
{

//FfH: Added by Kael 03/19/2008
	int iCount = 0;
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
	{
		if (GET_PLAYER((PlayerTypes)iPlayer).isFullMember(eVoteSource))
		{
			iCount += 1;
		}
	}
	if (iCount >= 2)
	{
		return true;
	}
//FfH: End Add

	return (getDiploVoteCount(eVoteSource) > 0);
}


int CvGame::getDiploVoteCount(VoteSourceTypes eVoteSource) const
{
	FAssert(eVoteSource >= 0 && eVoteSource < GC.getNumVoteSourceInfos());
	return m_aiDiploVote[eVoteSource];
}


void CvGame::changeDiploVote(VoteSourceTypes eVoteSource, int iChange)
{
	FAssert(eVoteSource >= 0 && eVoteSource < GC.getNumVoteSourceInfos());

	if (0 != iChange)
	{
		for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
		{
			GET_PLAYER((PlayerTypes)iPlayer).processVoteSourceBonus(eVoteSource, false);
		}

		m_aiDiploVote[eVoteSource] += iChange;
		FAssert(getDiploVoteCount(eVoteSource) >= 0);

		for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
		{
			GET_PLAYER((PlayerTypes)iPlayer).processVoteSourceBonus(eVoteSource, true);
		}
	}
}

bool CvGame::canDoResolution(VoteSourceTypes eVoteSource, const VoteSelectionSubData& kData) const
{
	if (GC.getVoteInfo(kData.eVote).isVictory())
	{
/*************************************************************************************************/
/**	K-mod merger								16/02/12								Snarko	**/
/**																								**/
/**		Merging in features of K-mod, most notably the pathfinder				**/
/*************************************************************************************************/
		int iVotesRequired = getVoteRequired(kData.eVote, eVoteSource); // K-Mod - for efficiency
/*************************************************************************************************/
/**	K-mod merger								END												**/
/*************************************************************************************************/
		for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
		{
			CvTeam& kTeam = GET_TEAM((TeamTypes)iTeam);

/*************************************************************************************************/
/**	K-mod merger								16/02/12								Snarko	**/
/**																								**/
/**		Merging in features of K-mod, most notably the pathfinder				**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
			if (kTeam.isVotingMember(eVoteSource))
			{
				if (kTeam.getVotes(kData.eVote, eVoteSource) >= getVoteRequired(kData.eVote, eVoteSource))
				{
					// Can't vote on a winner if one team already has all the votes necessary to win
					return false;
				}
			}
/**								----  End Original Code  ----									**/
			if (kTeam.getVotes(kData.eVote, eVoteSource) >= iVotesRequired) // K-Mod. same, but faster.
				return false;
/*************************************************************************************************/
/**	K-mod merger								END												**/
/*************************************************************************************************/
		}
	}

	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);

		if (kPlayer.isVotingMember(eVoteSource))
		{
			if (!kPlayer.canDoResolution(eVoteSource, kData))
			{
				return false;
			}
		}
		else if (kPlayer.isAlive() && !kPlayer.isBarbarian() && !kPlayer.isMinorCiv())
		{
			// all players need to be able to vote for a diplo victory
			if (GC.getVoteInfo(kData.eVote).isVictory())
			{
				return false;
			}
		}
	}

	return true;
}

bool CvGame::isValidVoteSelection(VoteSourceTypes eVoteSource, const VoteSelectionSubData& kData) const
{
	if (NO_PLAYER != kData.ePlayer)
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.ePlayer);
		if (!kPlayer.isAlive() || kPlayer.isBarbarian() || kPlayer.isMinorCiv())
		{
			return false;
		}
	}

	if (NO_PLAYER != kData.eOtherPlayer)
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.eOtherPlayer);
		if (!kPlayer.isAlive() || kPlayer.isBarbarian() || kPlayer.isMinorCiv())
		{
			return false;
		}
	}

	int iNumVoters = 0;
	for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
	{
		if (GET_TEAM((TeamTypes)iTeam).isVotingMember(eVoteSource))
		{
			++iNumVoters;
		}
	}
	if (iNumVoters  < GC.getVoteInfo(kData.eVote).getMinVoters())
	{
		return false;
	}

	if (GC.getVoteInfo(kData.eVote).isOpenBorders())
	{
		bool bOpenWithEveryone = true;
		for (int iTeam1 = 0; iTeam1 < MAX_CIV_TEAMS; ++iTeam1)
		{
			if (GET_TEAM((TeamTypes)iTeam1).isFullMember(eVoteSource))
			{
				for (int iTeam2 = iTeam1 + 1; iTeam2 < MAX_CIV_TEAMS; ++iTeam2)
				{
					CvTeam& kTeam2 = GET_TEAM((TeamTypes)iTeam2);

					if (kTeam2.isFullMember(eVoteSource))
					{
						if (!kTeam2.isOpenBorders((TeamTypes)iTeam1))
						{
							bOpenWithEveryone = false;
							break;
						}
					}
				}
			}
		}
		if (bOpenWithEveryone)
		{
			return false;
		}
	}
	else if (GC.getVoteInfo(kData.eVote).isDefensivePact())
	{
		bool bPactWithEveryone = true;
		for (int iTeam1 = 0; iTeam1 < MAX_CIV_TEAMS; ++iTeam1)
		{
			if (GET_TEAM((TeamTypes)iTeam1).isFullMember(eVoteSource))
			{
				for (int iTeam2 = iTeam1 + 1; iTeam2 < MAX_CIV_TEAMS; ++iTeam2)
				{
					CvTeam& kTeam2 = GET_TEAM((TeamTypes)iTeam2);

					if (kTeam2.isFullMember(eVoteSource))
					{
						if (!kTeam2.isDefensivePact((TeamTypes)iTeam1))
						{
							bPactWithEveryone = false;
							break;
						}
					}
				}
			}
		}
		if (bPactWithEveryone)
		{
			return false;
		}
	}
	else if (GC.getVoteInfo(kData.eVote).isForcePeace())
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.ePlayer);

		if (GET_TEAM(kPlayer.getTeam()).isAVassal())
		{
			return false;
		}

		if (!kPlayer.isFullMember(eVoteSource))
		{
			return false;
		}

		bool bValid = false;

		for (int iTeam2 = 0; iTeam2 < MAX_CIV_TEAMS; ++iTeam2)
		{
			if (atWar(kPlayer.getTeam(), (TeamTypes)iTeam2))
			{
				CvTeam& kTeam2 = GET_TEAM((TeamTypes)iTeam2);

				if (kTeam2.isVotingMember(eVoteSource))
				{
					bValid = true;
					break;
				}
			}
		}

		if (!bValid)
		{
			return false;
		}
	}
	else if (GC.getVoteInfo(kData.eVote).isForceNoTrade())
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.ePlayer);

		if (kPlayer.isFullMember(eVoteSource))
		{
			return false;
		}

		bool bNoTradeWithEveryone = true;
		for (int iPlayer2 = 0; iPlayer2 < MAX_CIV_PLAYERS; ++iPlayer2)
		{
			CvPlayer& kPlayer2 = GET_PLAYER((PlayerTypes)iPlayer2);
			if (kPlayer2.getTeam() != kPlayer.getTeam())
			{
				if (kPlayer2.isFullMember(eVoteSource))
				{
					if (kPlayer2.canStopTradingWithTeam(kPlayer.getTeam()))
					{
						bNoTradeWithEveryone = false;
						break;
					}
				}
			}
		}
		// Not an option if already at war with everyone
		if (bNoTradeWithEveryone)
		{
			return false;
		}
	}
	else if (GC.getVoteInfo(kData.eVote).isForceWar())
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.ePlayer);
		CvTeam& kTeam = GET_TEAM(kPlayer.getTeam());

		if (kTeam.isAVassal())
		{
			return false;
		}

		if (kPlayer.isFullMember(eVoteSource))
		{
			return false;
		}

		bool bAtWarWithEveryone = true;
		for (int iTeam2 = 0; iTeam2 < MAX_CIV_TEAMS; ++iTeam2)
		{
			if (iTeam2 != kPlayer.getTeam())
			{
				CvTeam& kTeam2 = GET_TEAM((TeamTypes)iTeam2);
				if (kTeam2.isFullMember(eVoteSource))
				{
					if (!kTeam2.isAtWar(kPlayer.getTeam()) && kTeam2.canChangeWarPeace(kPlayer.getTeam()))
					{
						bAtWarWithEveryone = false;
						break;
					}
				}
			}
		}
		// Not an option if already at war with everyone
		if (bAtWarWithEveryone)
		{
			return false;
		}

		// Can be passed against a non-member only if he is already at war with a member
		if (!kPlayer.isVotingMember(eVoteSource))
		{
			bool bValid = false;
			for (int iTeam2 = 0; iTeam2 < MAX_CIV_TEAMS; ++iTeam2)
			{
				if (atWar(kPlayer.getTeam(), (TeamTypes)iTeam2))
				{
					CvTeam& kTeam2 = GET_TEAM((TeamTypes)iTeam2);

					if (kTeam2.isFullMember(eVoteSource))
					{
						bValid = true;
						break;
					}
				}
			}

			if (!bValid)
			{
				return false;
			}
		}
	}
	else if (GC.getVoteInfo(kData.eVote).isAssignCity())
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.ePlayer);
		if (kPlayer.isFullMember(eVoteSource) || !kPlayer.isVotingMember(eVoteSource))
		{
			return false;
		}

		CvCity* pCity = kPlayer.getCity(kData.iCityId);
		FAssert(NULL != pCity);
		if (NULL == pCity)
		{
			return false;
		}

		if (NO_PLAYER == kData.eOtherPlayer)
		{
			return false;
		}

		CvPlayer& kOtherPlayer = GET_PLAYER(kData.eOtherPlayer);
		if (kOtherPlayer.getTeam() == kPlayer.getTeam())
		{
			return false;
		}

		if (atWar(kPlayer.getTeam(), GET_PLAYER(kData.eOtherPlayer).getTeam()))
		{
			return false;
		}

		if (!kOtherPlayer.isFullMember(eVoteSource))
		{
			return false;
		}

		if (kOtherPlayer.isHuman() && isOption(GAMEOPTION_ONE_CITY_CHALLENGE))
		{
			return false;
		}
	}

	if (!canDoResolution(eVoteSource, kData))
	{
		return false;
	}

	return true;
}


bool CvGame::isDebugMode() const
{
	return m_bDebugModeCache;
}


void CvGame::toggleDebugMode()
{
	m_bDebugMode = ((m_bDebugMode) ? false : true);
	updateDebugModeCache();

	GC.getMapINLINE().updateVisibility();
	GC.getMapINLINE().updateSymbols();
	GC.getMapINLINE().updateMinimapColor();

	gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
	gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
	gDLL->getInterfaceIFace()->setDirty(MinimapSection_DIRTY_BIT, true);
	gDLL->getInterfaceIFace()->setDirty(UnitInfo_DIRTY_BIT, true);
	gDLL->getInterfaceIFace()->setDirty(CityInfo_DIRTY_BIT, true);
	gDLL->getInterfaceIFace()->setDirty(GlobeLayer_DIRTY_BIT, true);

	//gDLL->getEngineIFace()->SetDirty(GlobeTexture_DIRTY_BIT, true);
	gDLL->getEngineIFace()->SetDirty(MinimapTexture_DIRTY_BIT, true);
	gDLL->getEngineIFace()->SetDirty(CultureBorders_DIRTY_BIT, true);

	if (m_bDebugMode)
	{
		gDLL->getEngineIFace()->PushFogOfWar(FOGOFWARMODE_OFF);
	}
	else
	{
		gDLL->getEngineIFace()->PopFogOfWar();
	}
	gDLL->getEngineIFace()->setFogOfWarFromStack();
}

void CvGame::updateDebugModeCache()
{
	if ((gDLL->getChtLvl() > 0) || (gDLL->GetWorldBuilderMode()))
	{
		m_bDebugModeCache = m_bDebugMode;
	}
	else
	{
		m_bDebugModeCache = false;
	}
}

int CvGame::getPitbossTurnTime() const
{
	return GC.getInitCore().getPitbossTurnTime();
}

void CvGame::setPitbossTurnTime(int iHours)
{
	GC.getInitCore().setPitbossTurnTime(iHours);
}


bool CvGame::isHotSeat() const
{
	return (GC.getInitCore().getHotseat());
}

bool CvGame::isPbem() const
{
	return (GC.getInitCore().getPbem());
}



bool CvGame::isPitboss() const
{
	return (GC.getInitCore().getPitboss());
}

bool CvGame::isSimultaneousTeamTurns() const
{
	if (!isNetworkMultiPlayer())
	{
		return false;
	}

	if (isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
	{
		return false;
	}

	return true;
}

bool CvGame::isFinalInitialized() const
{
	return m_bFinalInitialized;
}


void CvGame::setFinalInitialized(bool bNewValue)
{
	PROFILE_FUNC();

	int iI;

	if (isFinalInitialized() != bNewValue)
	{
		m_bFinalInitialized = bNewValue;

		if (isFinalInitialized())
		{
			updatePlotGroups();

			GC.getMapINLINE().updateIrrigated();

			for (iI = 0; iI < MAX_TEAMS; iI++)
			{
				if (GET_TEAM((TeamTypes)iI).isAlive())
				{
					GET_TEAM((TeamTypes)iI).AI_updateAreaStragies();
				}
			}
		}
	}
}


bool CvGame::getPbemTurnSent() const
{
	return m_bPbemTurnSent;
}


void CvGame::setPbemTurnSent(bool bNewValue)
{
	m_bPbemTurnSent = bNewValue;
}


bool CvGame::getHotPbemBetweenTurns() const
{
	return m_bHotPbemBetweenTurns;
}


void CvGame::setHotPbemBetweenTurns(bool bNewValue)
{
	m_bHotPbemBetweenTurns = bNewValue;
}


bool CvGame::isPlayerOptionsSent() const
{
	return m_bPlayerOptionsSent;
}


void CvGame::sendPlayerOptions(bool bForce)
{
	int iI;

	if (getActivePlayer() == NO_PLAYER)
	{
		return;
	}

	if (!isPlayerOptionsSent() || bForce)
	{
		m_bPlayerOptionsSent = true;

		for (iI = 0; iI < NUM_PLAYEROPTION_TYPES; iI++)
		{
			gDLL->sendPlayerOption(((PlayerOptionTypes)iI), gDLL->getPlayerOption((PlayerOptionTypes)iI));
		}
	}
}

bool CvGame::isNoCrash() const
{
	return true;
}

PlayerTypes CvGame::getActivePlayer() const
{
	return GC.getInitCore().getActivePlayer();
}


void CvGame::setActivePlayer(PlayerTypes eNewValue, bool bForceHotSeat)
{
	PlayerTypes eOldActivePlayer = getActivePlayer();
	if (eOldActivePlayer != eNewValue)
	{
		int iActiveNetId = ((NO_PLAYER != eOldActivePlayer) ? GET_PLAYER(eOldActivePlayer).getNetID() : -1);
		GC.getInitCore().setActivePlayer(eNewValue);

		if (GET_PLAYER(eNewValue).isHuman() && (isHotSeat() || isPbem() || bForceHotSeat))
		{
			gDLL->getPassword(eNewValue);
			setHotPbemBetweenTurns(false);
			gDLL->getInterfaceIFace()->dirtyTurnLog(eNewValue);

			if (NO_PLAYER != eOldActivePlayer)
			{
				int iInactiveNetId = GET_PLAYER(eNewValue).getNetID();
				GET_PLAYER(eNewValue).setNetID(iActiveNetId);
				GET_PLAYER(eOldActivePlayer).setNetID(iInactiveNetId);
			}

			GET_PLAYER(eNewValue).showMissedMessages();

			if (countHumanPlayersAlive() == 1 && isPbem())
			{
				// Nobody else left alive
				GC.getInitCore().setType(GAME_HOTSEAT_NEW);
			}

//FfH: Modified by Kael 10/07/2007
//			if (isHotSeat() || bForceHotSeat)
//			{
//				sendPlayerOptions(true);
//			}
			sendPlayerOptions(true);
//FfH: End Modify

		}

		if (GC.IsGraphicsInitialized())
		{
			GC.getMapINLINE().updateFog();
			GC.getMapINLINE().updateVisibility();
			GC.getMapINLINE().updateSymbols();
			GC.getMapINLINE().updateMinimapColor();

			updateUnitEnemyGlow();

			gDLL->getInterfaceIFace()->setEndTurnMessage(false);

			gDLL->getInterfaceIFace()->clearSelectedCities();
			gDLL->getInterfaceIFace()->clearSelectionList();

			gDLL->getInterfaceIFace()->setDirty(PercentButtons_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(ResearchButtons_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(MinimapSection_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(CityInfo_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(UnitInfo_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(Flag_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(GlobeLayer_DIRTY_BIT, true);

			gDLL->getEngineIFace()->SetDirty(CultureBorders_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(BlockadedPlots_DIRTY_BIT, true);
		}
	}
}

void CvGame::updateUnitEnemyGlow()
{
	//update unit enemy glow
	for(int i=0;i<MAX_PLAYERS;i++)
	{
		PlayerTypes playerType = (PlayerTypes) i;
		int iLoop;
		for(CvUnit *pLoopUnit = GET_PLAYER(playerType).firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = GET_PLAYER(playerType).nextUnit(&iLoop))
		{
			//update glow
			gDLL->getEntityIFace()->updateEnemyGlow(pLoopUnit->getUnitEntity());
		}
	}
}

HandicapTypes CvGame::getHandicapType() const
{
	return m_eHandicap;
}

void CvGame::setHandicapType(HandicapTypes eHandicap)
{
	m_eHandicap = eHandicap;
}

PlayerTypes CvGame::getPausePlayer() const
{
	return m_ePausePlayer;
}


bool CvGame::isPaused() const
{
	return (getPausePlayer() != NO_PLAYER);
}


void CvGame::setPausePlayer(PlayerTypes eNewValue)
{
	m_ePausePlayer = eNewValue;
}


UnitTypes CvGame::getBestLandUnit() const
{
	return m_eBestLandUnit;
}


int CvGame::getBestLandUnitCombat() const
{
	if (getBestLandUnit() == NO_UNIT)
	{
		return 1;
	}

	return std::max(1, GC.getUnitInfo(getBestLandUnit()).getCombat());
}


void CvGame::setBestLandUnit(UnitTypes eNewValue)
{
	if (getBestLandUnit() != eNewValue)
	{
		m_eBestLandUnit = eNewValue;

		gDLL->getInterfaceIFace()->setDirty(UnitInfo_DIRTY_BIT, true);
	}
}


TeamTypes CvGame::getWinner() const
{
	return m_eWinner;
}


VictoryTypes CvGame::getVictory() const
{
	return m_eVictory;
}


void CvGame::setWinner(TeamTypes eNewWinner, VictoryTypes eNewVictory)
{
	CvWString szBuffer;

	if ((getWinner() != eNewWinner) || (getVictory() != eNewVictory))
	{
		m_eWinner = eNewWinner;
		m_eVictory = eNewVictory;

		if (getVictory() != NO_VICTORY)
		{
			if (getWinner() != NO_TEAM)
			{
				szBuffer = gDLL->getText("TXT_KEY_GAME_WON", GET_TEAM(getWinner()).getName().GetCString(), GC.getVictoryInfo(getVictory()).getTextKeyWide());
				addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, GET_TEAM(getWinner()).getLeaderID(), szBuffer, -1, -1, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
			}

			if ((getAIAutoPlay() > 0) || gDLL->GetAutorun())
			{
				setGameState(GAMESTATE_EXTENDED);
			}
			else
			{
				setGameState(GAMESTATE_OVER);
			}
		}

		gDLL->getInterfaceIFace()->setDirty(Center_DIRTY_BIT, true);
		CvEventReporter::getInstance().victory(eNewWinner, eNewVictory);
		gDLL->getInterfaceIFace()->setDirty(Soundtrack_DIRTY_BIT, true);
	}
}


GameStateTypes CvGame::getGameState() const
{
	return m_eGameState;
}


void CvGame::setGameState(GameStateTypes eNewValue)
{
	CvPopupInfo* pInfo;
	int iI;

	if (getGameState() != eNewValue)
	{
		m_eGameState = eNewValue;

		if (eNewValue == GAMESTATE_OVER)
		{
			CvEventReporter::getInstance().gameEnd();

			showEndGameSequence();

			for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
			{
				if (GET_PLAYER((PlayerTypes)iI).isHuman())
				{
					// One more turn?
					pInfo = new CvPopupInfo(BUTTONPOPUP_EXTENDED_GAME);
					if (NULL != pInfo)
					{
						GET_PLAYER((PlayerTypes)iI).addPopup(pInfo);
					}
				}
			}
		}

		gDLL->getInterfaceIFace()->setDirty(Cursor_DIRTY_BIT, true);
	}
}


GameSpeedTypes CvGame::getGameSpeedType() const
{
	return GC.getInitCore().getGameSpeed();
}


EraTypes CvGame::getStartEra() const
{
	return GC.getInitCore().getEra();
}


CalendarTypes CvGame::getCalendar() const
{
	return GC.getInitCore().getCalendar();
}


PlayerTypes CvGame::getRankPlayer(int iRank) const
{
	FAssertMsg(iRank >= 0, "iRank is expected to be non-negative (invalid Rank)");
	FAssertMsg(iRank < MAX_PLAYERS, "iRank is expected to be within maximum bounds (invalid Rank)");
	return (PlayerTypes)m_aiRankPlayer[iRank];
}


void CvGame::setRankPlayer(int iRank, PlayerTypes ePlayer)
{
	FAssertMsg(iRank >= 0, "iRank is expected to be non-negative (invalid Rank)");
	FAssertMsg(iRank < MAX_PLAYERS, "iRank is expected to be within maximum bounds (invalid Rank)");

	if (getRankPlayer(iRank) != ePlayer)
	{
		m_aiRankPlayer[iRank] = ePlayer;

		gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
	}
}


int CvGame::getPlayerRank(PlayerTypes ePlayer) const
{
	FAssertMsg(ePlayer >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(ePlayer < MAX_PLAYERS, "ePlayer is expected to be within maximum bounds (invalid Index)");
	return m_aiPlayerRank[ePlayer];
}


void CvGame::setPlayerRank(PlayerTypes ePlayer, int iRank)
{
	FAssertMsg(ePlayer >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(ePlayer < MAX_PLAYERS, "ePlayer is expected to be within maximum bounds (invalid Index)");
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/03/09                       poyuzhe & jdog5000     */
/*                                                                                              */
/* Efficiency                                                                                   */
/************************************************************************************************/
	// From Sanguo Mod Performance, ie the CAR Mod
	// Attitude cache
	if (iRank != m_aiPlayerRank[ePlayer])
	{
		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				GET_PLAYER(ePlayer).AI_invalidateAttitudeCache((PlayerTypes)iI);
				GET_PLAYER((PlayerTypes)iI).AI_invalidateAttitudeCache(ePlayer);
			}
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	m_aiPlayerRank[ePlayer] = iRank;
	FAssert(getPlayerRank(ePlayer) >= 0);
}


int CvGame::getPlayerScore(PlayerTypes ePlayer)	const
{
	FAssertMsg(ePlayer >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(ePlayer < MAX_PLAYERS, "ePlayer is expected to be within maximum bounds (invalid Index)");
	return m_aiPlayerScore[ePlayer];
}


void CvGame::setPlayerScore(PlayerTypes ePlayer, int iScore)
{
	FAssertMsg(ePlayer >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(ePlayer < MAX_PLAYERS, "ePlayer is expected to be within maximum bounds (invalid Index)");

	if (getPlayerScore(ePlayer) != iScore)
	{
		m_aiPlayerScore[ePlayer] = iScore;
		FAssert(getPlayerScore(ePlayer) >= 0);

		gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
	}
}


TeamTypes CvGame::getRankTeam(int iRank) const
{
	FAssertMsg(iRank >= 0, "iRank is expected to be non-negative (invalid Rank)");
	FAssertMsg(iRank < MAX_TEAMS, "iRank is expected to be within maximum bounds (invalid Index)");
	return (TeamTypes)m_aiRankTeam[iRank];
}


void CvGame::setRankTeam(int iRank, TeamTypes eTeam)
{
	FAssertMsg(iRank >= 0, "iRank is expected to be non-negative (invalid Rank)");
	FAssertMsg(iRank < MAX_TEAMS, "iRank is expected to be within maximum bounds (invalid Index)");

	if (getRankTeam(iRank) != eTeam)
	{
		m_aiRankTeam[iRank] = eTeam;

		gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
	}
}


int CvGame::getTeamRank(TeamTypes eTeam) const
{
	FAssertMsg(eTeam >= 0, "eTeam is expected to be non-negative (invalid Index)");
	FAssertMsg(eTeam < MAX_TEAMS, "eTeam is expected to be within maximum bounds (invalid Index)");
	return m_aiTeamRank[eTeam];
}


void CvGame::setTeamRank(TeamTypes eTeam, int iRank)
{
	FAssertMsg(eTeam >= 0, "eTeam is expected to be non-negative (invalid Index)");
	FAssertMsg(eTeam < MAX_TEAMS, "eTeam is expected to be within maximum bounds (invalid Index)");
	m_aiTeamRank[eTeam] = iRank;
	FAssert(getTeamRank(eTeam) >= 0);
}


int CvGame::getTeamScore(TeamTypes eTeam) const
{
	FAssertMsg(eTeam >= 0, "eTeam is expected to be non-negative (invalid Index)");
	FAssertMsg(eTeam < MAX_TEAMS, "eTeam is expected to be within maximum bounds (invalid Index)");
	return m_aiTeamScore[eTeam];
}


void CvGame::setTeamScore(TeamTypes eTeam, int iScore)
{
	FAssertMsg(eTeam >= 0, "eTeam is expected to be non-negative (invalid Index)");
	FAssertMsg(eTeam < MAX_TEAMS, "eTeam is expected to be within maximum bounds (invalid Index)");
	m_aiTeamScore[eTeam] = iScore;
	FAssert(getTeamScore(eTeam) >= 0);
}


bool CvGame::isOption(GameOptionTypes eIndex) const
{

//FfH: Added by Kael 07/12/2008 (locked options)
	if (eIndex == GAMEOPTION_NO_ESPIONAGE)
	{
		return true;
	}
	if (eIndex == GAMEOPTION_RANDOM_PERSONALITIES)
	{
		return false;
	}
	if (eIndex == GAMEOPTION_PICK_RELIGION)
	{
		return false;
	}
/*************************************************************************************************/
/**	Xienwolf Tweak							02/06/09											**/
/**																								**/
/**						Rigid assurance that we never have bad options							**/
/*************************************************************************************************/
	if (eIndex >= GAMEOPTION_LIMITED_CIVILIZATION_SELECTION)
	{
		return false;
	}
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
//FfH: End Add

	return GC.getInitCore().getOption(eIndex);
}


void CvGame::setOption(GameOptionTypes eIndex, bool bEnabled)
{
	GC.getInitCore().setOption(eIndex, bEnabled);
}


bool CvGame::isMPOption(MultiplayerOptionTypes eIndex) const
{
	return GC.getInitCore().getMPOption(eIndex);
}


void CvGame::setMPOption(MultiplayerOptionTypes eIndex, bool bEnabled)
{
	GC.getInitCore().setMPOption(eIndex, bEnabled);
}


bool CvGame::isForcedControl(ForceControlTypes eIndex) const
{
	return GC.getInitCore().getForceControl(eIndex);
}


void CvGame::setForceControl(ForceControlTypes eIndex, bool bEnabled)
{
	GC.getInitCore().setForceControl(eIndex, bEnabled);
}


int CvGame::getUnitCreatedCount(UnitTypes eIndex)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumUnitInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiUnitCreatedCount[eIndex];
}


void CvGame::incrementUnitCreatedCount(UnitTypes eIndex)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumUnitInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiUnitCreatedCount[eIndex]++;
}


int CvGame::getUnitClassCreatedCount(UnitClassTypes eIndex)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumUnitClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiUnitClassCreatedCount[eIndex];
}

bool CvGame::isUnitClassMaxedOut(UnitClassTypes eIndex, int iExtra)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumUnitClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (!isWorldUnitClass(eIndex))
	{
		return false;
	}

	//FAssertMsg(getUnitClassCreatedCount(eIndex) <= GC.getUnitClassInfo(eIndex).getMaxGlobalInstances(), "Index is expected to be within maximum bounds (invalid Index)");

	return ((getUnitClassCreatedCount(eIndex) + iExtra) >= GC.getUnitClassInfo(eIndex).getMaxGlobalInstances());
}


void CvGame::incrementUnitClassCreatedCount(UnitClassTypes eIndex)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumUnitClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiUnitClassCreatedCount[eIndex]++;
}


/*************************************************************************************************/
/**	Spawn Groups						08/05/10									Valkrionn	**/
/**																								**/
/**					New spawn mechanic, allowing us to customize stacks							**/
/*************************************************************************************************/
int CvGame::getSpawnGroupCreatedCount(SpawnGroupTypes eIndex)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumSpawnGroupInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiSpawnGroupCreatedCount[eIndex];
}
void CvGame::incrementSpawnGroupCreatedCount(SpawnGroupTypes eIndex)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumSpawnGroupInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiSpawnGroupCreatedCount[eIndex]++;
}
/*************************************************************************************************/
/**	Spawn Groups							END													**/
/*************************************************************************************************/

int CvGame::getBuildingClassCreatedCount(BuildingClassTypes eIndex)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiBuildingClassCreatedCount[eIndex];
}


bool CvGame::isBuildingClassMaxedOut(BuildingClassTypes eIndex, int iExtra)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (!isWorldWonderClass(eIndex))
	{
		return false;
	}

	FAssertMsg(getBuildingClassCreatedCount(eIndex) <= GC.getBuildingClassInfo(eIndex).getMaxGlobalInstances(), "Index is expected to be within maximum bounds (invalid Index)");

	return ((getBuildingClassCreatedCount(eIndex) + iExtra) >= GC.getBuildingClassInfo(eIndex).getMaxGlobalInstances());
}


void CvGame::incrementBuildingClassCreatedCount(BuildingClassTypes eIndex)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiBuildingClassCreatedCount[eIndex]++;
}


int CvGame::getProjectCreatedCount(ProjectTypes eIndex)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiProjectCreatedCount[eIndex];
}


bool CvGame::isProjectMaxedOut(ProjectTypes eIndex, int iExtra)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (!isWorldProject(eIndex))
	{
		return false;
	}

	FAssertMsg(getProjectCreatedCount(eIndex) <= GC.getProjectInfo(eIndex).getMaxGlobalInstances(), "Index is expected to be within maximum bounds (invalid Index)");

	return ((getProjectCreatedCount(eIndex) + iExtra) >= GC.getProjectInfo(eIndex).getMaxGlobalInstances());
}


void CvGame::incrementProjectCreatedCount(ProjectTypes eIndex, int iExtra)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiProjectCreatedCount[eIndex] += iExtra;
}


int CvGame::getForceCivicCount(CivicTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumCivicInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiForceCivicCount[eIndex];
}


bool CvGame::isForceCivic(CivicTypes eIndex) const
{
	return (getForceCivicCount(eIndex) > 0);
}


bool CvGame::isForceCivicOption(CivicOptionTypes eCivicOption) const
{
	int iI;

	for (iI = 0; iI < GC.getNumCivicInfos(); iI++)
	{
		if (GC.getCivicInfo((CivicTypes)iI).getCivicOptionType() == eCivicOption)
		{
			if (isForceCivic((CivicTypes)iI))
			{
				return true;
			}
		}
	}

	return false;
}


void CvGame::changeForceCivicCount(CivicTypes eIndex, int iChange)
{
	bool bOldForceCivic;

	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumCivicInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		bOldForceCivic = isForceCivic(eIndex);

		m_paiForceCivicCount[eIndex] += iChange;
		FAssert(getForceCivicCount(eIndex) >= 0);

		if (bOldForceCivic != isForceCivic(eIndex))
		{
			verifyCivics();
		}
	}
}


PlayerVoteTypes CvGame::getVoteOutcome(VoteTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumVoteInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiVoteOutcome[eIndex];
}


bool CvGame::isVotePassed(VoteTypes eIndex) const
{
	PlayerVoteTypes ePlayerVote = getVoteOutcome(eIndex);

	if (isTeamVote(eIndex))
	{
		return (ePlayerVote >= 0 && ePlayerVote < MAX_CIV_TEAMS);
	}
	else
	{
		return (ePlayerVote == PLAYER_VOTE_YES);
	}
}


void CvGame::setVoteOutcome(const VoteTriggeredData& kData, PlayerVoteTypes eNewValue)
{
	bool bOldPassed;

	VoteTypes eIndex = kData.kVoteOption.eVote;
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumVoteInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (getVoteOutcome(eIndex) != eNewValue)
	{
		bOldPassed = isVotePassed(eIndex);

		m_paiVoteOutcome[eIndex] = eNewValue;

		if (bOldPassed != isVotePassed(eIndex))
		{
			processVote(kData, ((isVotePassed(eIndex)) ? 1 : -1));
		}
	}

	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kPlayer.isAlive())
		{
			kPlayer.setVote(kData.getID(), NO_PLAYER_VOTE);
		}
	}
}


int CvGame::getReligionGameTurnFounded(ReligionTypes eIndex)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumReligionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiReligionGameTurnFounded[eIndex];
}


bool CvGame::isReligionFounded(ReligionTypes eIndex)
{
	return (getReligionGameTurnFounded(eIndex) != -1);
}


void CvGame::makeReligionFounded(ReligionTypes eIndex, PlayerTypes ePlayer)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumReligionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (!isReligionFounded(eIndex))
	{
		FAssertMsg(getGameTurn() != -1, "getGameTurn() is not expected to be equal with -1");
		m_paiReligionGameTurnFounded[eIndex] = getGameTurn();

		CvEventReporter::getInstance().religionFounded(eIndex, ePlayer);
	}
}

bool CvGame::isReligionSlotTaken(ReligionTypes eReligion) const
{
	FAssertMsg(eReligion >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eReligion < GC.getNumReligionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_abReligionSlotTaken[eReligion];
}

void CvGame::setReligionSlotTaken(ReligionTypes eReligion, bool bTaken)
{
	FAssertMsg(eReligion >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eReligion < GC.getNumReligionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_abReligionSlotTaken[eReligion] = bTaken;
}


int CvGame::getCorporationGameTurnFounded(CorporationTypes eIndex)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumCorporationInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiCorporationGameTurnFounded[eIndex];
}


bool CvGame::isCorporationFounded(CorporationTypes eIndex)
{
	return (getCorporationGameTurnFounded(eIndex) != -1);
}


void CvGame::makeCorporationFounded(CorporationTypes eIndex, PlayerTypes ePlayer)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumCorporationInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (!isCorporationFounded(eIndex))
	{
		FAssertMsg(getGameTurn() != -1, "getGameTurn() is not expected to be equal with -1");
		m_paiCorporationGameTurnFounded[eIndex] = getGameTurn();

		CvEventReporter::getInstance().corporationFounded(eIndex, ePlayer);
	}
}

bool CvGame::isVictoryValid(VictoryTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumVictoryInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return GC.getInitCore().getVictory(eIndex);
}

void CvGame::setVictoryValid(VictoryTypes eIndex, bool bValid)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumVictoryInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	GC.getInitCore().setVictory(eIndex, bValid);
}


bool CvGame::isSpecialUnitValid(SpecialUnitTypes eIndex)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumSpecialUnitInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_pabSpecialUnitValid[eIndex];
}


void CvGame::makeSpecialUnitValid(SpecialUnitTypes eIndex)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumSpecialUnitInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_pabSpecialUnitValid[eIndex] = true;
}


bool CvGame::isSpecialBuildingValid(SpecialBuildingTypes eIndex)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumSpecialBuildingInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_pabSpecialBuildingValid[eIndex];
}


void CvGame::makeSpecialBuildingValid(SpecialBuildingTypes eIndex, bool bAnnounce)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumSpecialBuildingInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (!m_pabSpecialBuildingValid[eIndex])
	{
		m_pabSpecialBuildingValid[eIndex] = true;


		if (bAnnounce)
		{
			CvWString szBuffer = gDLL->getText("TXT_KEY_SPECIAL_BUILDING_VALID", GC.getSpecialBuildingInfo(eIndex).getTextKeyWide());

			for (int iI = 0; iI < MAX_PLAYERS; iI++)
			{
				if (GET_PLAYER((PlayerTypes)iI).isAlive())
				{
					gDLL->getInterfaceIFace()->addMessage(((PlayerTypes)iI), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_PROJECT_COMPLETED", MESSAGE_TYPE_MAJOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
				}
			}
		}
	}
}


bool CvGame::isNukesValid() const
{
	return m_bNukesValid;
}


void CvGame::makeNukesValid(bool bValid)
{
	m_bNukesValid = bValid;
}

bool CvGame::isInAdvancedStart() const
{
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
	{
		if ((GET_PLAYER((PlayerTypes)iPlayer).getAdvancedStartPoints() >= 0) && GET_PLAYER((PlayerTypes)iPlayer).isHuman())
		{
			return true;
		}
	}

	return false;
}

void CvGame::setVoteChosen(int iSelection, int iVoteId)
{
	VoteSelectionData* pVoteSelectionData = getVoteSelection(iVoteId);
	if (NULL != pVoteSelectionData)
	{
		addVoteTriggered(*pVoteSelectionData, iSelection);
	}

	deleteVoteSelection(iVoteId);
}


CvCity* CvGame::getHolyCity(ReligionTypes eIndex)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumReligionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return getCity(m_paHolyCity[eIndex]);
}


void CvGame::setHolyCity(ReligionTypes eIndex, CvCity* pNewValue, bool bAnnounce)
{
	CvWString szBuffer;
	CvCity* pOldValue;
	CvCity* pHolyCity;
	int iI;

	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumReligionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	pOldValue = getHolyCity(eIndex);

	if (pOldValue != pNewValue)
	{
		  // religion visibility now part of espionage
		//updateCitySight(false, true);

		if (pNewValue != NULL)
		{
			m_paHolyCity[eIndex] = pNewValue->getIDInfo();

/*************************************************************************************************/
/**	WorldBreakers						01/05/09									Xienwolf	**/
/**																								**/
/**							Tracks AC Contributions on a player basis							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
//FfH: Added by Kael 08/13/2007
			changeGlobalCounter(GC.getReligionInfo(eIndex).getGlobalCounterFound());
//FfH: End Add
/**								----  End Original Code  ----									**/
			GET_PLAYER(pNewValue->getOwner()).changeGlobalCounterContrib(GC.getReligionInfo(eIndex).getGlobalCounterFound());
/*************************************************************************************************/
/**	WorldBreakers							END													**/
/*************************************************************************************************/

		}
		else
		{
			m_paHolyCity[eIndex].reset();

/*************************************************************************************************/
/**	WorldBreakers						01/05/09									Xienwolf	**/
/**																								**/
/**							Tracks AC Contributions on a player basis							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
//FfH: Added by Kael 08/13/2007
			changeGlobalCounter(-1 * GC.getReligionInfo(eIndex).getGlobalCounterFound());
//FfH: End Add
/**								----  End Original Code  ----									**/
			GET_PLAYER(pOldValue->getOwner()).changeGlobalCounterContrib(-1 * GC.getReligionInfo(eIndex).getGlobalCounterFound());
/*************************************************************************************************/
/**	WorldBreakers							END													**/
/*************************************************************************************************/

		}

		// religion visibility now part of espionage
		//updateCitySight(true, true);

		if (pOldValue != NULL)
		{
			pOldValue->changeReligionInfluence(eIndex, -(GC.getDefineINT("HOLY_CITY_INFLUENCE")));

/*************************************************************************************************/
/**	Stasis									11/17/09								Valkrionn	**/
/**																								**/
/**			Adds new commerces to Religions			**/
/*************************************************************************************************/
			pOldValue->updateReligion();
/*************************************************************************************************/
/**	Stasis									END													**/
/*************************************************************************************************/

			pOldValue->setInfoDirty(true);
		}

		if (getHolyCity(eIndex) != NULL)
		{
			pHolyCity = getHolyCity(eIndex);

			pHolyCity->setHasReligion(eIndex, true, bAnnounce, true);
			pHolyCity->changeReligionInfluence(eIndex, GC.getDefineINT("HOLY_CITY_INFLUENCE"));

/*************************************************************************************************/
/**	Stasis									11/17/09								Valkrionn	**/
/**																								**/
/**			Adds new commerces to Religions			**/
/*************************************************************************************************/
			pHolyCity->updateReligion();
/*************************************************************************************************/
/**	Stasis									END													**/
/*************************************************************************************************/

			pHolyCity->setInfoDirty(true);

			if (bAnnounce)
			{
				if (isFinalInitialized() && !(gDLL->GetWorldBuilderMode()))
				{
					szBuffer = gDLL->getText("TXT_KEY_MISC_REL_FOUNDED", GC.getReligionInfo(eIndex).getTextKeyWide(), pHolyCity->getNameKey());
					addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, pHolyCity->getOwnerINLINE(), szBuffer, pHolyCity->getX_INLINE(), pHolyCity->getY_INLINE(), (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));

					for (iI = 0; iI < MAX_PLAYERS; iI++)
					{
						if (GET_PLAYER((PlayerTypes)iI).isAlive())
						{
							if (pHolyCity->isRevealed(GET_PLAYER((PlayerTypes)iI).getTeam(), false)

//FfH: Added by Kael 12/11/2007
							  && GET_PLAYER((PlayerTypes)iI).canSeeReligion(eIndex, pHolyCity)
//FfH: End Add

							)
							{
								szBuffer = gDLL->getText("TXT_KEY_MISC_REL_FOUNDED", GC.getReligionInfo(eIndex).getTextKeyWide(), pHolyCity->getNameKey());
								gDLL->getInterfaceIFace()->addMessage(((PlayerTypes)iI), false, GC.getDefineINT("EVENT_MESSAGE_TIME_LONG"), szBuffer, GC.getReligionInfo(eIndex).getSound(), MESSAGE_TYPE_MAJOR_EVENT, GC.getReligionInfo(eIndex).getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"), pHolyCity->getX_INLINE(), pHolyCity->getY_INLINE());
							}
							else
							{
								szBuffer = gDLL->getText("TXT_KEY_MISC_REL_FOUNDED_UNKNOWN", GC.getReligionInfo(eIndex).getTextKeyWide());
								gDLL->getInterfaceIFace()->addMessage(((PlayerTypes)iI), false, GC.getDefineINT("EVENT_MESSAGE_TIME_LONG"), szBuffer, GC.getReligionInfo(eIndex).getSound(), MESSAGE_TYPE_MAJOR_EVENT, GC.getReligionInfo(eIndex).getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
							}
						}
					}
				}
			}
		}

		AI_makeAssignWorkDirty();

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/26/09                       poyuzhe & jdog5000     */
/*                                                                                              */
/* Efficiency                                                                                   */
/************************************************************************************************/
		// From Sanguo Mod Performance, ie the CAR Mod
		// Attitude cache
		if (isFinalInitialized())
		{
			for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
			{
				if (GET_PLAYER((PlayerTypes)iJ).isAlive() && GET_PLAYER((PlayerTypes)iJ).getStateReligion() == eIndex)
				{
					if( pNewValue != NULL )
					{
						GET_PLAYER(pNewValue->getOwnerINLINE()).AI_invalidateAttitudeCache((PlayerTypes)iJ);
						GET_PLAYER((PlayerTypes)iJ).AI_invalidateAttitudeCache(pNewValue->getOwnerINLINE());
					}

					if( pOldValue != NULL )
					{
						GET_PLAYER(pOldValue->getOwnerINLINE()).AI_invalidateAttitudeCache((PlayerTypes)iJ);
						GET_PLAYER((PlayerTypes)iJ).AI_invalidateAttitudeCache(pOldValue->getOwnerINLINE());
					}
				}
			}
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	}
}


CvCity* CvGame::getHeadquarters(CorporationTypes eIndex)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumCorporationInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return getCity(m_paHeadquarters[eIndex]);
}


void CvGame::setHeadquarters(CorporationTypes eIndex, CvCity* pNewValue, bool bAnnounce)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumCorporationInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	CvCity* pOldValue = getHeadquarters(eIndex);

	if (pOldValue != pNewValue)
	{
		if (pNewValue != NULL)
		{
			m_paHeadquarters[eIndex] = pNewValue->getIDInfo();
		}
		else
		{
			m_paHeadquarters[eIndex].reset();
		}

		if (pOldValue != NULL)
		{
			pOldValue->updateCorporation();

			pOldValue->setInfoDirty(true);
		}

		CvCity* pHeadquarters = getHeadquarters(eIndex);

		if (NULL != pHeadquarters)
		{
			pHeadquarters->setHasCorporation(eIndex, true, bAnnounce);
			pHeadquarters->updateCorporation();
			pHeadquarters->setInfoDirty(true);

			if (bAnnounce)
			{
				if (isFinalInitialized() && !(gDLL->GetWorldBuilderMode()))
				{
					CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_CORPORATION_FOUNDED", GC.getCorporationInfo(eIndex).getTextKeyWide(), pHeadquarters->getNameKey());
					addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, pHeadquarters->getOwnerINLINE(), szBuffer, pHeadquarters->getX_INLINE(), pHeadquarters->getY_INLINE(), (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));

					for (int iI = 0; iI < MAX_PLAYERS; iI++)
					{
						if (GET_PLAYER((PlayerTypes)iI).isAlive())
						{
							if (pHeadquarters->isRevealed(GET_PLAYER((PlayerTypes)iI).getTeam(), false))
							{
								gDLL->getInterfaceIFace()->addMessage(((PlayerTypes)iI), false, GC.getDefineINT("EVENT_MESSAGE_TIME_LONG"), szBuffer, GC.getCorporationInfo(eIndex).getSound(), MESSAGE_TYPE_MAJOR_EVENT, GC.getCorporationInfo(eIndex).getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"), pHeadquarters->getX_INLINE(), pHeadquarters->getY_INLINE());
							}
							else
							{
								CvWString szBuffer2 = gDLL->getText("TXT_KEY_MISC_CORPORATION_FOUNDED_UNKNOWN", GC.getCorporationInfo(eIndex).getTextKeyWide());
								gDLL->getInterfaceIFace()->addMessage(((PlayerTypes)iI), false, GC.getDefineINT("EVENT_MESSAGE_TIME_LONG"), szBuffer2, GC.getCorporationInfo(eIndex).getSound(), MESSAGE_TYPE_MAJOR_EVENT, GC.getCorporationInfo(eIndex).getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
							}
						}
					}
				}
			}
		}

		AI_makeAssignWorkDirty();
	}
}


PlayerVoteTypes CvGame::getPlayerVote(PlayerTypes eOwnerIndex, int iVoteId) const
{
	FAssert(eOwnerIndex >= 0);
	FAssert(eOwnerIndex < MAX_CIV_PLAYERS);
	FAssert(NULL != getVoteTriggered(iVoteId));

	return GET_PLAYER(eOwnerIndex).getVote(iVoteId);
}


void CvGame::setPlayerVote(PlayerTypes eOwnerIndex, int iVoteId, PlayerVoteTypes eNewValue)
{
	FAssert(eOwnerIndex >= 0);
	FAssert(eOwnerIndex < MAX_CIV_PLAYERS);
	FAssert(NULL != getVoteTriggered(iVoteId));

	GET_PLAYER(eOwnerIndex).setVote(iVoteId, eNewValue);
}


void CvGame::castVote(PlayerTypes eOwnerIndex, int iVoteId, PlayerVoteTypes ePlayerVote)
{
	VoteTriggeredData* pTriggeredData = getVoteTriggered(iVoteId);
	if (NULL != pTriggeredData)
	{
		CvVoteInfo& kVote = GC.getVoteInfo(pTriggeredData->kVoteOption.eVote);
		if (kVote.isAssignCity())
		{
			FAssert(pTriggeredData->kVoteOption.ePlayer != NO_PLAYER);
			CvPlayer& kCityPlayer = GET_PLAYER(pTriggeredData->kVoteOption.ePlayer);

			if (GET_PLAYER(eOwnerIndex).getTeam() != kCityPlayer.getTeam())
			{
				switch (ePlayerVote)
				{
				case PLAYER_VOTE_YES:
					kCityPlayer.AI_changeMemoryCount(eOwnerIndex, MEMORY_VOTED_AGAINST_US, 1);
					break;
				case PLAYER_VOTE_NO:
					kCityPlayer.AI_changeMemoryCount(eOwnerIndex, MEMORY_VOTED_FOR_US, 1);
					break;
				default:
					break;
				}
			}
		}
		else if (isTeamVote(pTriggeredData->kVoteOption.eVote))
		{
			if ((PlayerVoteTypes)GET_PLAYER(eOwnerIndex).getTeam() != ePlayerVote)
			{
				for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
				{
					CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
					if (kLoopPlayer.isAlive())
					{
						if (kLoopPlayer.getTeam() != GET_PLAYER(eOwnerIndex).getTeam() && kLoopPlayer.getTeam() == (TeamTypes)ePlayerVote)
						{
							kLoopPlayer.AI_changeMemoryCount(eOwnerIndex, MEMORY_VOTED_FOR_US, 1);
						}
					}
				}
			}
		}

		setPlayerVote(eOwnerIndex, iVoteId, ePlayerVote);
	}
}


std::string CvGame::getScriptData() const
{
	return m_szScriptData;
}


void CvGame::setScriptData(std::string szNewValue)
{
	m_szScriptData = szNewValue;
}

const CvWString & CvGame::getName()
{
	return GC.getInitCore().getGameName();
}


void CvGame::setName(const TCHAR* szName)
{
	GC.getInitCore().setGameName(szName);
}


bool CvGame::isDestroyedCityName(CvWString& szName) const
{
	std::vector<CvWString>::const_iterator it;

	for (it = m_aszDestroyedCities.begin(); it != m_aszDestroyedCities.end(); it++)
	{
		if (*it == szName)
		{
			return true;
		}
	}

	return false;
}

void CvGame::addDestroyedCityName(const CvWString& szName)
{
	m_aszDestroyedCities.push_back(szName);
}

bool CvGame::isGreatPersonBorn(CvWString& szName) const
{
	std::vector<CvWString>::const_iterator it;

	for (it = m_aszGreatPeopleBorn.begin(); it != m_aszGreatPeopleBorn.end(); it++)
	{
		if (*it == szName)
		{
			return true;
		}
	}

	return false;
}

void CvGame::addGreatPersonBornName(const CvWString& szName)
{
	m_aszGreatPeopleBorn.push_back(szName);
}


// Protected Functions...


void CvGame::doTurn()
{
	//Snarko temp
	//PROFILE_BEGIN("CvGame::doTurn()");
	//startProfilingDLL();

	int aiShuffle[MAX_PLAYERS];
	int iLoopPlayer;
	int iI;

	// END OF TURN
	CvEventReporter::getInstance().beginGameTurn( getGameTurn() );

	doUpdateCacheOnTurn();
	setTurnSliceSinceBeginning(0);

	// Merging in features of K-mod, most notably the pathfinder : K-mod merger Snarko 16/02/12
	CvSelectionGroup::path_finder.Reset(); // K-Mod. (this is the only manual reset we need. - I hope.)

	updateScore();

	doDeals();

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			GET_TEAM((TeamTypes)iI).doTurn();
		}
	}

	// BARB SPAWNING:
	// Spawning from python heroes and events ignores any limits (done elsewhere). Then:
	// Lairs only spawn if under barb limit for that area and type.
	// 1 animal lair can always spawn in valid plot per area, even if target limit is 0
	createLairs();

	// Lairs will roll to spawn barbs up to their limit or area limit (among all other plot::doTurn things...)
	GC.getMapINLINE().doTurn();

	// May spawn some barbs as defenders, ignoring limit. Barb cities build regardless of area limits
	createBarbarianCities();
	// Random barbs of all types will attempt to spawn, rate-limited, up to their area limit.
	createBarbarianUnits();
	createAnimals();
	createDemons();

	doGlobalWarming();

	doHolyCity();

	doHeadquarters();

	doDiploVote();

	gDLL->getInterfaceIFace()->setEndTurnMessage(false);
	gDLL->getInterfaceIFace()->setHasMovedUnit(false);

	if (getAIAutoPlay() > 0)
	{
		changeAIAutoPlay(-1);

		if (getAIAutoPlay() == 0)
		{
			reviveActivePlayer();
		}
	}

	CvEventReporter::getInstance().endGameTurn(getGameTurn());

	incrementGameTurn();
	incrementElapsedGameTurns();

	// Per Turn Decay Function : New Tag Defs (ProjectInfos, GameInfos) Xienwolf 10/01/08
	for (iI=0;iI<GC.getNumProjectInfos();iI++)
	{
		if (getProjectTimer((ProjectTypes)iI, 0) != -1 && (getProjectTimer((ProjectTypes)iI, 0) + (GC.getProjectInfo((ProjectTypes)iI).getCooldown() * GC.getGameSpeedInfo(getGameSpeedType()).getGrowthPercent() / 100)) < getGameTurn())
		{
			clearProjectTimer((ProjectTypes)iI);
		}
	}

	if (isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
	{
		shuffleArray(aiShuffle, MAX_PLAYERS, getSorenRand());

		for (iI = 0; iI < MAX_PLAYERS; iI++)
		{
			iLoopPlayer = aiShuffle[iI];

			if (GET_PLAYER((PlayerTypes)iLoopPlayer).isAlive())
			{
				GET_PLAYER((PlayerTypes)iLoopPlayer).setTurnActive(true);
			}
		}
	}
	else if (isSimultaneousTeamTurns())
	{
		for (iI = 0; iI < MAX_TEAMS; iI++)
		{
			CvTeam& kTeam = GET_TEAM((TeamTypes)iI);
			if (kTeam.isAlive())
			{
				kTeam.setTurnActive(true);
				FAssert(getNumGameTurnActive() == kTeam.getAliveCount());

				// Fixing BtS bug. Caused WoC if first team was dead : Snarko 08/06/10
				// Vanilla:
				// }
				// break;

				break;
			}
		}
	}
	else
	{
		for (iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (isPbem() && GET_PLAYER((PlayerTypes)iI).isHuman())
				{
					if (iI == getActivePlayer())
					{
						// Nobody else left alive
						GC.getInitCore().setType(GAME_HOTSEAT_NEW);
						GET_PLAYER((PlayerTypes)iI).setTurnActive(true);
					}
					else if (!getPbemTurnSent())
					{
						gDLL->sendPbemTurn((PlayerTypes)iI);
					}
				}
				else
				{
					GET_PLAYER((PlayerTypes)iI).setTurnActive(true);
					FAssert(getNumGameTurnActive() == 1);
				}

				break;
			}
		}
	}

	//FfH: Added by Kael 09/26/2008
	if (isOption(GAMEOPTION_CHALLENGE_CUT_LOSERS))
	{
		if (countCivPlayersAlive() > 5)
		{
			changeCutLosersCounter(-1);
			if (getCutLosersCounter() == 0)
			{
				GET_PLAYER(getRankPlayer(countCivPlayersAlive() -1)).setAlive(false);
				changeCutLosersCounter(50 * GC.getGameSpeedInfo(getGameSpeedType()).getGrowthPercent() / 100);
			}
		}
	}
	if (isOption(GAMEOPTION_CHALLENGE_HIGH_TO_LOW))
	{
		if (!isGameMultiPlayer())
		{
			// Modifies Challenge escalation based on Gamespeed : Xienwolf 12/13/08
			if (getGameTurn() >= 50 * GC.getGameSpeedInfo(getGameSpeedType()).getGrowthPercent() / 100)
			{
				if (getHighToLowCounter() < 2)
				{
					for (iI = 0; iI < MAX_PLAYERS; iI++)
					{
						if (GET_PLAYER((PlayerTypes)iI).isAlive())
						{
							if (GET_PLAYER((PlayerTypes)iI).isHuman())
							{
								if (getPlayerRank((PlayerTypes)iI) == 0)
								{
									GC.getInitCore().reassignPlayerAdvanced((PlayerTypes)iI, getRankPlayer(countCivPlayersAlive() -1), -1);
									changeHighToLowCounter(1);
								}
							}
						}
					}
				}
			}
		}
	}
	if (isOption(GAMEOPTION_CHALLENGE_INCREASING_DIFFICULTY))
	{
		changeIncreasingDifficultyCounter(1);
		// Modifies Challenge escalation based on Gamespeed : Xienwolf 12/13/08
		if (getIncreasingDifficultyCounter() >= 50 * GC.getGameSpeedInfo(getGameSpeedType()).getGrowthPercent() / 100)
		{
			if (getHandicapType() < (GC.getNumHandicapInfos() - 1))
			{
				for (iI = 0; iI < MAX_PLAYERS; iI++)
				{
					if (GET_PLAYER((PlayerTypes)iI).isAlive())
					{
						if (GET_PLAYER((PlayerTypes)iI).isHuman())
						{
							GC.getInitCore().setHandicap((PlayerTypes)iI, (HandicapTypes)(getHandicapType() + 1));
						}
					}
				}
				setHandicapType((HandicapTypes)(getHandicapType() + 1));
				changeIncreasingDifficultyCounter(getIncreasingDifficultyCounter() * -1);
			}
		}
	}
	if (isOption(GAMEOPTION_FLEXIBLE_DIFFICULTY))
	{
		if (!isGameMultiPlayer())
		{
			changeFlexibleDifficultyCounter(1);
			// Modifies Challenge escalation based on Gamespeed : Xienwolf 12/13/08
			if (getFlexibleDifficultyCounter() >= 20 * GC.getGameSpeedInfo(getGameSpeedType()).getGrowthPercent() / 100)
			{
				for (iI = 0; iI < MAX_PLAYERS; iI++)
				{
					if (GET_PLAYER((PlayerTypes)iI).isAlive())
					{
						if (GET_PLAYER((PlayerTypes)iI).isHuman())
						{
							if (getPlayerRank((PlayerTypes)iI) <= (countCivPlayersAlive() / 3) && getHandicapType() < (GC.getNumHandicapInfos() - 1))
							{
								GC.getInitCore().setHandicap((PlayerTypes)iI, (HandicapTypes)(getHandicapType() + 1));
								setHandicapType((HandicapTypes)(getHandicapType() + 1));
								changeFlexibleDifficultyCounter(getFlexibleDifficultyCounter() * -1);
							}
							if (getPlayerRank((PlayerTypes)iI) > (countCivPlayersAlive() * 2 / 3) && getHandicapType() > 0)
							{
								GC.getInitCore().setHandicap((PlayerTypes)iI, (HandicapTypes)(getHandicapType() - 1));
								setHandicapType((HandicapTypes)(getHandicapType() - 1));
								changeFlexibleDifficultyCounter(getFlexibleDifficultyCounter() * -1);
							}
						}
					}
				}
			}
		}
	}

	testVictory();

	gDLL->getEngineIFace()->SetDirty(GlobePartialTexture_DIRTY_BIT, true);
	gDLL->getEngineIFace()->DoTurn();

	//Snarko temp
	//	PROFILE_END();
	//	stopProfilingDLL();

	gDLL->getEngineIFace()->AutoSave();
}


void CvGame::doDeals()
{
	CvDeal* pLoopDeal;
	int iLoop;

	verifyDeals();

	for(pLoopDeal = firstDeal(&iLoop); pLoopDeal != NULL; pLoopDeal = nextDeal(&iLoop))
	{
		pLoopDeal->doTurn();
	}
}


void CvGame::doGlobalWarming()
{
	int iGlobalWarmingDefense = 0;
	for (int i = 0; i < GC.getMapINLINE().numPlotsINLINE(); ++i)
	{
		CvPlot* pPlot = GC.getMapINLINE().plotByIndexINLINE(i);

		if (!pPlot->isWater())
		{
			if (pPlot->getFeatureType() != NO_FEATURE)
			{
				if (GC.getFeatureInfo(pPlot->getFeatureType()).getGrowthProbability() > 0) // hack, but we don't want to add new XML field in the patch just for this
				{
					++iGlobalWarmingDefense;
				}
			}
		}
	}
	iGlobalWarmingDefense = iGlobalWarmingDefense * GC.getDefineINT("GLOBAL_WARMING_FOREST") / std::max(1, GC.getMapINLINE().getLandPlots());

	int iUnhealthWeight = GC.getDefineINT("GLOBAL_WARMING_UNHEALTH_WEIGHT");
	int iGlobalWarmingValue = 0;
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes) iPlayer);
		if (kPlayer.isAlive())
		{
			int iLoop;
			for (CvCity* pCity = kPlayer.firstCity(&iLoop); pCity != NULL; pCity = kPlayer.nextCity(&iLoop))
			{
				iGlobalWarmingValue -= pCity->getBuildingBadHealth() * iUnhealthWeight;
			}
		}
	}
	iGlobalWarmingValue /= GC.getMapINLINE().numPlotsINLINE();

	iGlobalWarmingValue += getNukesExploded() * GC.getDefineINT("GLOBAL_WARMING_NUKE_WEIGHT") / 100;

	TerrainTypes eWarmingTerrain = ((TerrainTypes)(GC.getDefineINT("GLOBAL_WARMING_TERRAIN")));

	for (int iI = 0; iI < iGlobalWarmingValue; iI++)
	{
		if (getSorenRandNum(100, "Global Warming") + iGlobalWarmingDefense < GC.getDefineINT("GLOBAL_WARMING_PROB"))
		{
			CvPlot* pPlot = GC.getMapINLINE().syncRandPlot(RANDPLOT_LAND | RANDPLOT_NOT_CITY);

			if (pPlot != NULL)
			{
				bool bChanged = false;

				if (pPlot->getFeatureType() != NO_FEATURE)
				{
					if (pPlot->getFeatureType() != GC.getDefineINT("NUKE_FEATURE"))
					{
						pPlot->setFeatureType(NO_FEATURE);
						bChanged = true;
					}
				}
				else if (pPlot->getTerrainType() != eWarmingTerrain)
				{
					if (pPlot->calculateTotalBestNatureYield(NO_TEAM) > 1)
					{
						pPlot->setTerrainType(eWarmingTerrain);
						bChanged = true;
					}
				}

/*************************************************************************************************/
/**	Xienwolf Tweak							12/13/08											**/
/**																								**/
/**					Attempt to keep Unique Features from being removed on Mapgen				**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
				if (bChanged)
/**								----  End Original Code  ----									**/
				if (bChanged && (pPlot->getImprovementType() == NO_IMPROVEMENT || !GC.getImprovementInfo(pPlot->getImprovementType()).isPermanent()))
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
				{
					pPlot->setImprovementType(NO_IMPROVEMENT);

					CvCity* pCity = GC.getMapINLINE().findCity(pPlot->getX_INLINE(), pPlot->getY_INLINE());
					if (pCity != NULL)
					{
						if (pPlot->isVisible(pCity->getTeam(), false))
						{
							CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_GLOBAL_WARMING_NEAR_CITY", pCity->getNameKey());
							gDLL->getInterfaceIFace()->addMessage(pCity->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_GLOBALWARMING", MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_NEGATIVE_TEXT"), pPlot->getX_INLINE(), pPlot->getY_INLINE(), true, true);
						}
					}
				}
			}
		}
	}
}


void CvGame::doHolyCity()
{
	PlayerTypes eBestPlayer;
	TeamTypes eBestTeam;
	long lResult;
	int iValue;
	int iBestValue;
	int iI, iJ, iK;

	lResult = 0;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "doHolyCity", NULL, &lResult);
	if (lResult == 1)
	{
		return;
	}

	if (getElapsedGameTurns() < 5 && !isOption(GAMEOPTION_ADVANCED_START))
	{
		return;
	}

	int iRandOffset = getSorenRandNum(GC.getNumReligionInfos(), "Holy City religion offset");
	for (int iLoop = 0; iLoop < GC.getNumReligionInfos(); ++iLoop)
	{
		iI = ((iLoop + iRandOffset) % GC.getNumReligionInfos());

		if (!isReligionSlotTaken((ReligionTypes)iI))
		{
			iBestValue = MAX_INT;
			eBestTeam = NO_TEAM;

			for (iJ = 0; iJ < MAX_TEAMS; iJ++)
			{
				if (GET_TEAM((TeamTypes)iJ).isAlive())
				{
					if (GET_TEAM((TeamTypes)iJ).isHasTech((TechTypes)(GC.getReligionInfo((ReligionTypes)iI).getTechPrereq())))
					{
						if (GET_TEAM((TeamTypes)iJ).getNumCities() > 0)
						{
							iValue = getSorenRandNum(10, "Found Religion (Team)");

							for (iK = 0; iK < GC.getNumReligionInfos(); iK++)
							{
								int iReligionCount = GET_TEAM((TeamTypes)iJ).getHasReligionCount((ReligionTypes)iK);

								if (iReligionCount > 0)
								{
									iValue += iReligionCount * 20;
								}
							}

							if (iValue < iBestValue)
							{
								iBestValue = iValue;
								eBestTeam = ((TeamTypes)iJ);
							}
						}
					}
				}
			}

			if (eBestTeam != NO_TEAM)
			{
				iBestValue = MAX_INT;
				eBestPlayer = NO_PLAYER;

				for (iJ = 0; iJ < MAX_PLAYERS; iJ++)
				{
/*************************************************************************************************/
/**	Heathens								05/26/08								Xienwolf	**/
/**																								**/
/**						Prevents Founding of Holy City for Heathen Player						**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
					if (GET_PLAYER((PlayerTypes)iJ).isAlive())
/**								----  End Original Code  ----									**/
					if (GET_PLAYER((PlayerTypes)iJ).isAlive() && !(GET_PLAYER((PlayerTypes)iJ).getModReligionSpreadChance() < -100))
/*************************************************************************************************/
/**	Heathens								END													**/
/*************************************************************************************************/
					{
						if (GET_PLAYER((PlayerTypes)iJ).getTeam() == eBestTeam)
						{
							if (GET_PLAYER((PlayerTypes)iJ).getNumCities() > 0)
							{
								iValue = getSorenRandNum(10, "Found Religion (Player)");

								if (!(GET_PLAYER((PlayerTypes)iJ).isHuman()))
								{
									iValue += 10;
								}

								for (iK = 0; iK < GC.getNumReligionInfos(); iK++)
								{
									int iReligionCount = GET_PLAYER((PlayerTypes)iJ).getHasReligionCount((ReligionTypes)iK);

									if (iReligionCount > 0)
									{
										iValue += iReligionCount * 20;
									}
								}

								if (iValue < iBestValue)
								{
									iBestValue = iValue;
									eBestPlayer = ((PlayerTypes)iJ);
								}
							}
						}
					}
				}

				if (eBestPlayer != NO_PLAYER)
				{
					ReligionTypes eReligion = (ReligionTypes)iI;

					if (isOption(GAMEOPTION_PICK_RELIGION))
					{
						eReligion = GET_PLAYER(eBestPlayer).AI_chooseReligion();
					}

					if (NO_RELIGION != eReligion)
					{
						GET_PLAYER(eBestPlayer).foundReligion(eReligion, (ReligionTypes)iI, false);
					}
				}
			}
		}
	}
}


void CvGame::doHeadquarters()
{
	long lResult = 0;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "doHeadquarters", NULL, &lResult);
	if (lResult == 1)
	{
		return;
	}

	if (getElapsedGameTurns() < 5)
	{
		return;
	}

	for (int iI = 0; iI < GC.getNumCorporationInfos(); iI++)
	{
		CvCorporationInfo& kCorporation = GC.getCorporationInfo((CorporationTypes)iI);
		if (!isCorporationFounded((CorporationTypes)iI))
		{
			int iBestValue = MAX_INT;
			TeamTypes eBestTeam = NO_TEAM;

			for (int iJ = 0; iJ < MAX_TEAMS; iJ++)
			{
				CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iJ);
				if (kLoopTeam.isAlive())
				{
					if (NO_TECH != kCorporation.getTechPrereq() && kLoopTeam.isHasTech((TechTypes)(kCorporation.getTechPrereq())))
					{
						if (kLoopTeam.getNumCities() > 0)
						{
							bool bHasBonus = false;
							for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
							{
								if (NO_BONUS != kCorporation.getPrereqBonus(i) && kLoopTeam.hasBonus((BonusTypes)kCorporation.getPrereqBonus(i)))
								{
									bHasBonus = true;
									break;
								}
							}

							if (bHasBonus)
							{
								int iValue = getSorenRandNum(10, "Found Corporation (Team)");

								for (int iK = 0; iK < GC.getNumCorporationInfos(); iK++)
								{
									int iCorporationCount = GET_PLAYER((PlayerTypes)iJ).getHasCorporationCount((CorporationTypes)iK);

									if (iCorporationCount > 0)
									{
										iValue += iCorporationCount * 20;
									}
								}

								if (iValue < iBestValue)
								{
									iBestValue = iValue;
									eBestTeam = ((TeamTypes)iJ);
								}
							}
						}
					}
				}
			}

			if (eBestTeam != NO_TEAM)
			{
				iBestValue = MAX_INT;
				PlayerTypes eBestPlayer = NO_PLAYER;

				for (int iJ = 0; iJ < MAX_PLAYERS; iJ++)
				{
					CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iJ);
					if (kLoopPlayer.isAlive())
					{
						if (kLoopPlayer.getTeam() == eBestTeam)
						{
							if (kLoopPlayer.getNumCities() > 0)
							{
								bool bHasBonus = false;
								for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
								{
									if (NO_BONUS != kCorporation.getPrereqBonus(i) && kLoopPlayer.hasBonus((BonusTypes)kCorporation.getPrereqBonus(i)))
									{
										bHasBonus = true;
										break;
									}
								}

								if (bHasBonus)
								{
									int iValue = getSorenRandNum(10, "Found Religion (Player)");

									if (!kLoopPlayer.isHuman())
									{
										iValue += 10;
									}

									for (int iK = 0; iK < GC.getNumCorporationInfos(); iK++)
									{
										int iCorporationCount = GET_PLAYER((PlayerTypes)iJ).getHasCorporationCount((CorporationTypes)iK);

										if (iCorporationCount > 0)
										{
											iValue += iCorporationCount * 20;
										}
									}

									if (iValue < iBestValue)
									{
										iBestValue = iValue;
										eBestPlayer = ((PlayerTypes)iJ);
									}
								}
							}
						}
					}
				}

				if (eBestPlayer != NO_PLAYER)
				{
					GET_PLAYER(eBestPlayer).foundCorporation((CorporationTypes)iI);
				}
			}
		}
	}
}


void CvGame::doDiploVote()
{
	doVoteResults();

	doVoteSelection();
}


void CvGame::createBarbarianCities()
{
	// MultiBarb edits; BARBARIAN_PLAYER -> ORC_PLAYER : Xienwolf 12/23/08

	PROFILE("CvGame::createBarbarianCities");
	CvPlot* pLoopPlot;
	CvPlot* pBestPlot;
	long lResult;
	int iTargetCities;
	int iValue;
	int iBestValue;
	int iI;

	if (getMaxCityElimination() > 0)
	{
		return;
	}

	if (isOption(GAMEOPTION_NO_BARBARIANS))
	{
		return;
	}

	lResult = 0;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "createBarbarianCities", NULL, &lResult);
	if (lResult == 1)
	{
		return;
	}

	if (GC.getEraInfo(getCurrentEra()).isNoBarbCities())
	{
		return;
	}

	if (GC.getHandicapInfo(getHandicapType()).getUnownedTilesPerBarbarianCity() <= 0)
	{
		return;
	}

	if (getNumCivCities() < (countCivPlayersAlive() * 2))
	{
		return;
	}

	if (getElapsedGameTurns() < (((GC.getHandicapInfo(getHandicapType()).getBarbarianCityCreationTurnsElapsed() * GC.getGameSpeedInfo(getGameSpeedType()).getBarbPercent()) / 100) / std::max(getStartEra() + 1, 1)))
	{
		return;
	}

	if (getSorenRandNum(100, "Barb City Creation") >= GC.getHandicapInfo(getHandicapType()).getBarbarianCityCreationProb())
	{
		return;
	}

	iBestValue = 0;
	pBestPlot = NULL;

	int iTargetCitiesMultiplier = 100;
	{
		int iTargetBarbCities = (getNumCivCities() * 5 * GC.getHandicapInfo(getHandicapType()).getBarbarianCityCreationProb()) / 100;
		int iBarbCities = GET_PLAYER(ORC_PLAYER).getNumCities();
		if (iBarbCities < iTargetBarbCities)
		{
			iTargetCitiesMultiplier += (300 * (iTargetBarbCities - iBarbCities)) / iTargetBarbCities;
		}

		if (isOption(GAMEOPTION_RAGING_BARBARIANS))
		{
			iTargetCitiesMultiplier *= 3;
			iTargetCitiesMultiplier /= 2;
		}
	}


	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (!(pLoopPlot->isWater()))
		{
			if (!(pLoopPlot->isVisibleToCivTeam()))
			{
				iTargetCities = pLoopPlot->area()->getNumUnownedTiles();

				if (pLoopPlot->area()->getNumCities() == pLoopPlot->area()->getCitiesPerPlayer(ORC_PLAYER))
				{
					iTargetCities *= 3;
				}

				int iUnownedTilesThreshold = GC.getHandicapInfo(getHandicapType()).getUnownedTilesPerBarbarianCity();

				if (pLoopPlot->area()->getNumTiles() < (iUnownedTilesThreshold / 3))
				{
					iTargetCities *= iTargetCitiesMultiplier;
					iTargetCities /= 100;
				}

				iTargetCities /= std::max(1, iUnownedTilesThreshold);

				if (pLoopPlot->area()->getCitiesPerPlayer(ORC_PLAYER) < iTargetCities)
				{
					iValue = GET_PLAYER(ORC_PLAYER).AI_foundValue(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), GC.getDefineINT("MIN_BARBARIAN_CITY_STARTING_DISTANCE"));

					if (iTargetCitiesMultiplier > 100)
					{
						iValue *= pLoopPlot->area()->getNumOwnedTiles();
					}

					iValue += (getSorenRandNum(5, "Barb City Found"));

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
		GET_PLAYER(ORC_PLAYER).found(pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
	}
}


void CvGame::updateWar()
{
	int iI, iJ;

	if (isOption(GAMEOPTION_ALWAYS_WAR))
	{
/*************************************************************************************************/
/**	Xienwolf Tweak							02/14/09											**/
/**																								**/
/**					Should prevent forced declaration of war against barbarians					**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		for (iI = 0; iI < MAX_TEAMS; iI++)
/**								----  End Original Code  ----									**/
		for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
		{
			CvTeam& kTeam1 = GET_TEAM((TeamTypes)iI);
			if (kTeam1.isAlive() && kTeam1.isHuman())
			{
/*************************************************************************************************/
/**	Xienwolf Tweak							02/14/09											**/
/**																								**/
/**					Should prevent forced declaration of war against barbarians					**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
				for (iJ = 0; iJ < MAX_TEAMS; iJ++)
/**								----  End Original Code  ----									**/
				for (iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
				{
					CvTeam& kTeam2 = GET_TEAM((TeamTypes)iJ);
					if (kTeam2.isAlive() && !kTeam2.isHuman())
					{
						FAssert(iI != iJ);

						if (kTeam1.isHasMet((TeamTypes)iJ))
						{
							if (!kTeam1.isAtWar((TeamTypes)iJ))
							{
								kTeam1.declareWar(((TeamTypes)iJ), false, NO_WARPLAN);
							}
						}
					}
				}
			}
		}
	}
}


void CvGame::updateMoves()
{
	CvSelectionGroup* pLoopSelectionGroup;
	int aiShuffle[MAX_PLAYERS];
	int iLoop;
	int iI;

	if (isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
	{
		shuffleArray(aiShuffle, MAX_PLAYERS, getSorenRand());
	}
	else
	{
		for (iI = 0; iI < MAX_PLAYERS; iI++)
		{
			aiShuffle[iI] = iI;
		}
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& player = GET_PLAYER((PlayerTypes)(aiShuffle[iI]));

		if (player.isAlive())
		{
			if (player.isTurnActive())
			{
				if (player.isHuman() || gDLL->getInterfaceIFace()->isFocused())
				{
					setTurnSliceSinceBeginning(0);

				}
				else
				{
					if (getTurnSliceSinceBeginning() > 20 && !(gDLL->getInterfaceIFace()->isFocused()))
					{
						player.setTurnActive(false);
						setTurnSliceSinceBeginning(0);
					}
				}
				if (!(player.isAutoMoves()))
				{
					player.AI_unitUpdate();

					if (!(player.isHuman()))
					{
						if (!(player.hasBusyUnit()) && !(player.hasReadyUnit(true)))
						{
							player.setAutoMoves(true);
						}
					}
				}

				if (player.isAutoMoves())
				{
					for(pLoopSelectionGroup = player.firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = player.nextSelectionGroup(&iLoop))
					{
						pLoopSelectionGroup->autoMission();
					}

					if (!(player.hasBusyUnit()))
					{
						player.setAutoMoves(false);
					}
				}
			}
		}
	}
}


void CvGame::verifyCivics()
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			GET_PLAYER((PlayerTypes)iI).verifyCivics();
		}
	}
}


void CvGame::updateTimers()
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			GET_PLAYER((PlayerTypes)iI).updateTimers();
		}
	}
}


void CvGame::updateTurnTimer()
{
	int iI;

	// Are we using a turn timer?
	if (isMPOption(MPOPTION_TURN_TIMER))
	{
		if (getElapsedGameTurns() > 0 || !isOption(GAMEOPTION_ADVANCED_START))
		{
			// Has the turn expired?
			if (getTurnSlice() > getCutoffSlice())
			{
				for (iI = 0; iI < MAX_PLAYERS; iI++)
				{
					if (GET_PLAYER((PlayerTypes)iI).isAlive() && GET_PLAYER((PlayerTypes)iI).isTurnActive())
					{
						GET_PLAYER((PlayerTypes)iI).setEndTurn(true);

						if (!isMPOption(MPOPTION_SIMULTANEOUS_TURNS) && !isSimultaneousTeamTurns())
						{
							break;
						}
					}
				}
			}
		}
	}
}


void CvGame::testAlive()
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		GET_PLAYER((PlayerTypes)iI).verifyAlive();
	}
}
/*************************************************************************************************/
/**	AI Altar Victory				10/04/10								Snarko				**/
/**																								**/
/**							Convince the AI to go for altar victory								**/
/**		Sorry for messing with this function but no point recreating an almost exact copy		**/
/*************************************************************************************************/
/**								----  Start Original Code  ----
bool CvGame::testVictory(VictoryTypes eVictory, TeamTypes eTeam, bool* pbEndScore) const
/**								----  End Original Code  ----									**/
bool CvGame::testVictory(VictoryTypes eVictory, TeamTypes eTeam, bool* pbEndScore, BuildingClassTypes eIgnoreBuildingClass) const
/*************************************************************************************************/
/**	AI Altar Victory					END														**/
/*************************************************************************************************/
{
	FAssert(eVictory >= 0 && eVictory < GC.getNumVictoryInfos());
	FAssert(eTeam >=0 && eTeam < MAX_CIV_TEAMS);
	FAssert(GET_TEAM(eTeam).isAlive());

	bool bValid = isVictoryValid(eVictory);
	if (pbEndScore)
	{
		*pbEndScore = false;
	}

	if (bValid)
	{
		if (GC.getVictoryInfo(eVictory).isEndScore())
		{
			if (pbEndScore)
			{
				*pbEndScore = true;
			}

			if (getMaxTurns() == 0)
			{
				bValid = false;
			}
			else if (getElapsedGameTurns() < getMaxTurns())
			{
				bValid = false;
			}
			else
			{
				bool bFound = false;

				for (int iK = 0; iK < MAX_CIV_TEAMS; iK++)
				{
					if (GET_TEAM((TeamTypes)iK).isAlive())
					{
						if (iK != eTeam)
						{
							if (getTeamScore((TeamTypes)iK) >= getTeamScore(eTeam))
							{
								bFound = true;
								break;
							}
						}
					}
				}

				if (bFound)
				{
					bValid = false;
				}
			}
		}
	}

	if (bValid)
	{
		if (GC.getVictoryInfo(eVictory).isTargetScore())
		{
			if (getTargetScore() == 0)
			{
				bValid = false;
			}
			else if (getTeamScore(eTeam) < getTargetScore())
			{
				bValid = false;
			}
			else
			{
				bool bFound = false;

				for (int iK = 0; iK < MAX_CIV_TEAMS; iK++)
				{
					if (GET_TEAM((TeamTypes)iK).isAlive())
					{
						if (iK != eTeam)
						{
							if (getTeamScore((TeamTypes)iK) >= getTeamScore(eTeam))
							{
								bFound = true;
								break;
							}
						}
					}
				}

				if (bFound)
				{
					bValid = false;
				}
			}
		}
	}

	if (bValid)
	{
		if (GC.getVictoryInfo(eVictory).isConquest())
		{
			if (GET_TEAM(eTeam).getNumCities() == 0)
			{
				bValid = false;
			}
			else
			{
				bool bFound = false;

				for (int iK = 0; iK < MAX_CIV_TEAMS; iK++)
				{
					if (GET_TEAM((TeamTypes)iK).isAlive())
					{
						if (iK != eTeam && !GET_TEAM((TeamTypes)iK).isVassal(eTeam))
						{
							if (GET_TEAM((TeamTypes)iK).getNumCities() > 0)
							{
								bFound = true;
								break;
							}
						}
					}
				}

				if (bFound)
				{
					bValid = false;
				}
			}
		}
	}

	if (bValid)
	{
		if (GC.getVictoryInfo(eVictory).isDiploVote())
		{
			bool bFound = false;

			for (int iK = 0; iK < GC.getNumVoteInfos(); iK++)
			{
				if (GC.getVoteInfo((VoteTypes)iK).isVictory())
				{
					if (getVoteOutcome((VoteTypes)iK) == eTeam)
					{
						bFound = true;
						break;
					}
				}
			}

			if (!bFound)
			{
				bValid = false;
			}
		}
	}

	if (bValid)
	{
		if (getAdjustedPopulationPercent(eVictory) > 0)
		{
			if (100 * GET_TEAM(eTeam).getTotalPopulation() < getTotalPopulation() * getAdjustedPopulationPercent(eVictory))
			{
				bValid = false;
			}
		}
	}

	if (bValid)
	{
		if (getAdjustedLandPercent(eVictory) > 0)
		{
			if (100 * GET_TEAM(eTeam).getTotalLand() < GC.getMapINLINE().getLandPlots() * getAdjustedLandPercent(eVictory))
			{
				bValid = false;
			}
		}
	}

	if (bValid)
	{
		if (GC.getVictoryInfo(eVictory).getReligionPercent() > 0)
		{
			bool bFound = false;

			if (getNumCivCities() > (countCivPlayersAlive() * 2))
			{
				for (int iK = 0; iK < GC.getNumReligionInfos(); iK++)
				{
					if (GET_TEAM(eTeam).hasHolyCity((ReligionTypes)iK))
					{
						if (calculateReligionPercent((ReligionTypes)iK) >= GC.getVictoryInfo(eVictory).getReligionPercent())
						{
							bFound = true;
							break;
						}
					}

					if (bFound)
					{
						break;
					}
				}
			}

			if (!bFound)
			{
				bValid = false;
			}
		}
	}

	if (bValid)
	{
		if ((GC.getVictoryInfo(eVictory).getCityCulture() != NO_CULTURELEVEL) && (GC.getVictoryInfo(eVictory).getNumCultureCities() > 0))
		{
			int iCount = 0;

			for (int iK = 0; iK < MAX_CIV_PLAYERS; iK++)
			{
				if (GET_PLAYER((PlayerTypes)iK).isAlive())
				{
					if (GET_PLAYER((PlayerTypes)iK).getTeam() == eTeam)
					{
						int iLoop;
						for (CvCity* pLoopCity = GET_PLAYER((PlayerTypes)iK).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iK).nextCity(&iLoop))
						{
							if (pLoopCity->getCultureLevel() >= GC.getVictoryInfo(eVictory).getCityCulture())
							{
								iCount++;
							}
						}
					}
				}
			}

			if (iCount < GC.getVictoryInfo(eVictory).getNumCultureCities())
			{
				bValid = false;
			}
		}
	}

	if (bValid)
	{
		if (GC.getVictoryInfo(eVictory).getTotalCultureRatio() > 0)
		{
			int iThreshold = ((GET_TEAM(eTeam).countTotalCulture() * 100) / GC.getVictoryInfo(eVictory).getTotalCultureRatio());

			bool bFound = false;

			for (int iK = 0; iK < MAX_CIV_TEAMS; iK++)
			{
				if (GET_TEAM((TeamTypes)iK).isAlive())
				{
					if (iK != eTeam)
					{
						if (GET_TEAM((TeamTypes)iK).countTotalCulture() > iThreshold)
						{
							bFound = true;
							break;
						}
					}
				}
			}

			if (bFound)
			{
				bValid = false;
			}
		}
	}

	if (bValid)
	{
		for (int iK = 0; iK < GC.getNumBuildingClassInfos(); iK++)
		{
/*************************************************************************************************/
/**	AI Altar Victory				10/04/10								Snarko				**/
/**																								**/
/**							Convince the AI to go for altar victory								**/
/**		Sorry for messing with this function but no point recreating an almost exact copy		**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
			if (GC.getBuildingClassInfo((BuildingClassTypes) iK).getVictoryThreshold(eVictory) > GET_TEAM(eTeam).getBuildingClassCount((BuildingClassTypes)iK))
			{
				bValid = false;
				break;
			}
/**								----  End Original Code  ----									**/
			if ((eIgnoreBuildingClass == NO_BUILDINGCLASS) || ((BuildingClassTypes)iK != eIgnoreBuildingClass))
			{
				if (GC.getBuildingClassInfo((BuildingClassTypes) iK).getVictoryThreshold(eVictory) > GET_TEAM(eTeam).getBuildingClassCount((BuildingClassTypes)iK))
				{
					bValid = false;
					break;
				}
			}
/*************************************************************************************************/
/**	AI Altar Victory					END														**/
/*************************************************************************************************/
		}
	}

	if (bValid)
	{
		for (int iK = 0; iK < GC.getNumProjectInfos(); iK++)
		{
			if (GC.getProjectInfo((ProjectTypes) iK).getVictoryMinThreshold(eVictory) > GET_TEAM(eTeam).getProjectCount((ProjectTypes)iK))
			{
				bValid = false;
				break;
			}
		}
	}

	if (bValid)
	{
		long lResult = 1;
		CyArgsList argsList;
		argsList.add(eVictory);
/*************************************************************************************************/
/**	Xienwolf Tweak							03/27/09											**/
/**																								**/
/**							Not sure why Team wasn't passed originally							**/
/*************************************************************************************************/
		argsList.add(eTeam);
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
		gDLL->getPythonIFace()->callFunction(PYGameModule, "isVictory", argsList.makeFunctionArgs(), &lResult);
		if (0 == lResult)
		{
			bValid = false;
		}
	}

	return bValid;
}

void CvGame::testVictory()
{
	bool bEndScore = false;

	if (getVictory() != NO_VICTORY)
	{
		return;
	}

	if (getGameState() == GAMESTATE_EXTENDED)
	{
		return;
	}

	updateScore();

	long lResult = 1;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "isVictoryTest", NULL, &lResult);
	if (lResult == 0)
	{
		return;
	}

	std::vector<std::vector<int> > aaiWinners;

	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iI);
		if (kLoopTeam.isAlive())
		{
			if (!(kLoopTeam.isMinorCiv()))
			{
				for (int iJ = 0; iJ < GC.getNumVictoryInfos(); iJ++)
				{
					if (testVictory((VictoryTypes)iJ, (TeamTypes)iI, &bEndScore))
					{
						if (kLoopTeam.getVictoryCountdown((VictoryTypes)iJ) < 0)
						{
							if (kLoopTeam.getVictoryDelay((VictoryTypes)iJ) == 0)
							{
								kLoopTeam.setVictoryCountdown((VictoryTypes)iJ, 0);
							}
						}

						//update victory countdown
						if (kLoopTeam.getVictoryCountdown((VictoryTypes)iJ) > 0)
						{
							kLoopTeam.changeVictoryCountdown((VictoryTypes)iJ, -1);
						}

						if (kLoopTeam.getVictoryCountdown((VictoryTypes)iJ) == 0)
						{
							if (getSorenRandNum(100, "Victory Success") < kLoopTeam.getLaunchSuccessRate((VictoryTypes)iJ))
							{
								std::vector<int> aWinner;
								aWinner.push_back(iI);
								aWinner.push_back(iJ);
								aaiWinners.push_back(aWinner);
							}
							else
							{
								kLoopTeam.resetVictoryProgress();
							}
						}
					}
				}
			}
		}

	}

	if (aaiWinners.size() > 0)
	{
		int iWinner = getSorenRandNum(aaiWinners.size(), "Victory tie breaker");
		setWinner(((TeamTypes)aaiWinners[iWinner][0]), ((VictoryTypes)aaiWinners[iWinner][1]));
	}

	if (getVictory() == NO_VICTORY)
	{
		if (getMaxTurns() > 0)
		{
			if (getElapsedGameTurns() >= getMaxTurns())
			{
				if (!bEndScore)
				{
					if ((getAIAutoPlay() > 0) || gDLL->GetAutorun())
					{
						setGameState(GAMESTATE_EXTENDED);
					}
					else
					{
						setGameState(GAMESTATE_OVER);
					}
				}
			}
		}
	}
}


void CvGame::processVote(const VoteTriggeredData& kData, int iChange)
{
	CvVoteInfo& kVote = GC.getVoteInfo(kData.kVoteOption.eVote);

	changeTradeRoutes(kVote.getTradeRoutes() * iChange);
	changeFreeTradeCount(kVote.isFreeTrade() ? iChange : 0);
	changeNoNukesCount(kVote.isNoNukes() ? iChange : 0);

	for (int iI = 0; iI < GC.getNumCivicInfos(); iI++)
	{
		changeForceCivicCount((CivicTypes)iI, kVote.isForceCivic(iI) ? iChange : 0);
	}

//FfH: Added by Kael 11/14/2007
	bool bChange = false;
	if (iChange == 1)
	{
		bChange = true;
	}
	if (kVote.getNoBonus() != NO_BONUS)
	{
	//	setNoBonus((BonusTypes)kVote.getNoBonus(), bChange);

		for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
		{
			CvPlayer& pPlayer = GET_PLAYER((PlayerTypes)iPlayer);
			if (pPlayer.isAlive() && pPlayer.isFullMember(kData.eVoteSource))
			{
				pPlayer.changeNoBonus(iChange, kVote.getNoBonus());
			}
		}
	}
	if (kVote.isGamblingRing())
	{
		setGamblingRing(kData.eVoteSource, bChange);
	}
	if (kVote.isNoOutsideTechTrades())
	{
		setNoOutsideTechTrades(kData.eVoteSource, bChange);
	}
	if (kVote.isSlaveTrade())
	{
		setSlaveTrade(kData.eVoteSource, bChange);
	}
	if (kVote.isSmugglingRing())
	{
		setSmugglingRing(kData.eVoteSource, bChange);
	}
	if (kVote.getFreeUnits() > 0 && bChange)
	{
		for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
		{
			CvPlayer& pPlayer = GET_PLAYER((PlayerTypes)iPlayer);
			if (pPlayer.isAlive() && pPlayer.isFullMember(kData.eVoteSource) && kVote.getCost() < pPlayer.getGold())
			{
				UnitTypes eFreeUnit = ((UnitTypes)(GC.getCivilizationInfo(pPlayer.getCivilizationType()).getCivilizationUnits(kVote.getFreeUnitClass())));
				CvCity* pCity = pPlayer.getCapitalCity();
				if (pCity != NULL && eFreeUnit != NO_UNIT)
				{
					pPlayer.changeGold(-kVote.getCost());
					for (int i = 0; i < kVote.getFreeUnits(); ++i)
					{
						pPlayer.initUnit(eFreeUnit, pCity->getX(), pCity->getY());
					}
				}
			}
		}
	}
	if (kVote.getCrime() != 0)
	{
		changeCrime(kVote.getCrime());
	}
	if (!CvString(kVote.getPyResult()).empty())
	{
		CyArgsList argsList;
		argsList.add(kData.kVoteOption.eVote);
		argsList.add(1);
		gDLL->getPythonIFace()->callFunction(PYSpellModule, "vote", argsList.makeFunctionArgs());
	}
//FfH: End Add

	if (iChange > 0)
	{
		if (kVote.isOpenBorders())
		{
			for (int iTeam1 = 0; iTeam1 < MAX_CIV_PLAYERS; ++iTeam1)
			{
				CvTeam& kLoopTeam1 = GET_TEAM((TeamTypes)iTeam1);
				if (kLoopTeam1.isVotingMember(kData.eVoteSource))
				{
					for (int iTeam2 = iTeam1 + 1; iTeam2 < MAX_CIV_PLAYERS; ++iTeam2)
					{
						CvTeam& kLoopTeam2 = GET_TEAM((TeamTypes)iTeam2);
						if (kLoopTeam2.isVotingMember(kData.eVoteSource))
						{
							kLoopTeam1.signOpenBorders((TeamTypes)iTeam2);
						}
					}
				}
			}

			setVoteOutcome(kData, NO_PLAYER_VOTE);
		}
		else if (kVote.isDefensivePact())
		{
			for (int iTeam1 = 0; iTeam1 < MAX_CIV_PLAYERS; ++iTeam1)
			{
				CvTeam& kLoopTeam1 = GET_TEAM((TeamTypes)iTeam1);
				if (kLoopTeam1.isVotingMember(kData.eVoteSource))
				{
					for (int iTeam2 = iTeam1 + 1; iTeam2 < MAX_CIV_PLAYERS; ++iTeam2)
					{
						CvTeam& kLoopTeam2 = GET_TEAM((TeamTypes)iTeam2);
						if (kLoopTeam2.isVotingMember(kData.eVoteSource))
						{
							kLoopTeam1.signDefensivePact((TeamTypes)iTeam2);
						}
					}
				}
			}

			setVoteOutcome(kData, NO_PLAYER_VOTE);
		}
		else if (kVote.isForcePeace())
		{
			FAssert(NO_PLAYER != kData.kVoteOption.ePlayer);
			CvPlayer& kPlayer = GET_PLAYER(kData.kVoteOption.ePlayer);

			for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
			{
				CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
				if (kLoopPlayer.getTeam() != kPlayer.getTeam())
				{
					if (kLoopPlayer.isVotingMember(kData.eVoteSource))
					{
						kLoopPlayer.forcePeace(kData.kVoteOption.ePlayer);
					}
				}
			}

			setVoteOutcome(kData, NO_PLAYER_VOTE);
		}
		else if (kVote.isForceNoTrade())
		{
			FAssert(NO_PLAYER != kData.kVoteOption.ePlayer);
			CvPlayer& kPlayer = GET_PLAYER(kData.kVoteOption.ePlayer);

			for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
			{
				CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
				if (kLoopPlayer.isVotingMember(kData.eVoteSource))
				{
					if (kLoopPlayer.canStopTradingWithTeam(kPlayer.getTeam()))
					{
						kLoopPlayer.stopTradingWithTeam(kPlayer.getTeam());
					}
				}
			}

			setVoteOutcome(kData, NO_PLAYER_VOTE);
		}
		else if (kVote.isForceWar())
		{
			FAssert(NO_PLAYER != kData.kVoteOption.ePlayer);
			CvPlayer& kPlayer = GET_PLAYER(kData.kVoteOption.ePlayer);

			for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
			{
				CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
				if (kLoopPlayer.isVotingMember(kData.eVoteSource))
				{
					if (GET_TEAM(kLoopPlayer.getTeam()).canChangeWarPeace(kPlayer.getTeam()))
					{
						GET_TEAM(kLoopPlayer.getTeam()).declareWar(kPlayer.getTeam(), false, WARPLAN_DOGPILE);
					}
				}
			}

			setVoteOutcome(kData, NO_PLAYER_VOTE);
		}
		else if (kVote.isAssignCity())
		{
			FAssert(NO_PLAYER != kData.kVoteOption.ePlayer);
			CvPlayer& kPlayer = GET_PLAYER(kData.kVoteOption.ePlayer);
			CvCity* pCity = kPlayer.getCity(kData.kVoteOption.iCityId);
			FAssert(NULL != pCity);

			if (NULL != pCity)
			{
				if (NO_PLAYER != kData.kVoteOption.eOtherPlayer && kData.kVoteOption.eOtherPlayer != pCity->getOwnerINLINE())
				{
					GET_PLAYER(kData.kVoteOption.eOtherPlayer).acquireCity(pCity, false, true, true);
				}
			}

			setVoteOutcome(kData, NO_PLAYER_VOTE);
		}
	}
}


int CvGame::getIndexAfterLastDeal()
{
	return m_deals.getIndexAfterLast();
}


int CvGame::getNumDeals()
{
	return m_deals.getCount();
}


 CvDeal* CvGame::getDeal(int iID)
{
	return ((CvDeal *)(m_deals.getAt(iID)));
}


CvDeal* CvGame::addDeal()
{
	return ((CvDeal *)(m_deals.add()));
}


 void CvGame::deleteDeal(int iID)
{
	m_deals.removeAt(iID);
	gDLL->getInterfaceIFace()->setDirty(Foreign_Screen_DIRTY_BIT, true);
}

CvDeal* CvGame::firstDeal(int *pIterIdx, bool bRev)
{
	return !bRev ? m_deals.beginIter(pIterIdx) : m_deals.endIter(pIterIdx);
}


CvDeal* CvGame::nextDeal(int *pIterIdx, bool bRev)
{
	return !bRev ? m_deals.nextIter(pIterIdx) : m_deals.prevIter(pIterIdx);
}


 CvRandom& CvGame::getMapRand()
{
	return m_mapRand;
}


int CvGame::getMapRandNum(int iNum, const char* pszLog)
{
	return m_mapRand.get(iNum, pszLog);
}


CvRandom& CvGame::getSorenRand()
{
	return m_sorenRand;
}


int CvGame::getSorenRandNum(int iNum, const char* pszLog)
{
/*************************************************************************************************/
/**	Xienwolf Tweak							12/10/08											**/
/**																								**/
/**					Reveals all Random checks if Debugging is enabled							**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	return m_sorenRand.get(iNum, pszLog);
/**								----  End Original Code  ----									**/
	int Result = m_sorenRand.get(iNum, pszLog);
	if (isMPOption(MPOPTION_RANDOM_LOGGER) || gDLL->getChtLvl() > 0)
	{
		if (isFinalInitialized())
		{
			TCHAR szFile[1024];
			sprintf(szFile, "RandomLogger - Player %d - Set %d.log", getActivePlayer(), getGameTurn()/50);
			TCHAR szOut[1024];
			sprintf(szOut, "[%04d] %5d of %-5d -- %s\n", getGameTurn()+1, Result, iNum, pszLog);
			gDLL->logMsg(szFile,szOut, false, false);
		}
	}
	return Result;
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
}


int CvGame::calculateSyncChecksum()
{
	PROFILE_FUNC();

	CvUnit* pLoopUnit;
	int iMultiplier;
	int iValue;
	int iLoop;
	int iI, iJ;

	iValue = 0;

	iValue += getMapRand().getSeed();
	iValue += getSorenRand().getSeed();

	iValue += getNumCities();
	iValue += getTotalPopulation();
	iValue += getNumDeals();

	iValue += GC.getMapINLINE().getOwnedPlots();
	iValue += GC.getMapINLINE().getNumAreas();

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isEverAlive())
		{
			iMultiplier = getPlayerScore((PlayerTypes)iI);

			switch (getTurnSlice() % 4)
			{
			case 0:
				iMultiplier += (GET_PLAYER((PlayerTypes)iI).getTotalPopulation() * 543271);
				iMultiplier += (GET_PLAYER((PlayerTypes)iI).getTotalLand() * 327382);
				iMultiplier += (GET_PLAYER((PlayerTypes)iI).getGold() * 107564);
				iMultiplier += (GET_PLAYER((PlayerTypes)iI).getAssets() * 327455);
				iMultiplier += (GET_PLAYER((PlayerTypes)iI).getPower() * 135647);
				iMultiplier += (GET_PLAYER((PlayerTypes)iI).getNumCities() * 436432);
				iMultiplier += (GET_PLAYER((PlayerTypes)iI).getNumUnits() * 324111);
				iMultiplier += (GET_PLAYER((PlayerTypes)iI).getNumSelectionGroups() * 215356);
				break;

			case 1:
				for (iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
				{
					iMultiplier += (GET_PLAYER((PlayerTypes)iI).calculateTotalYield((YieldTypes)iJ) * 432754);
				}

				for (iJ = 0; iJ < NUM_COMMERCE_TYPES; iJ++)
				{
					iMultiplier += (GET_PLAYER((PlayerTypes)iI).getCommerceRate((CommerceTypes)iJ) * 432789);
				}
				break;

			case 2:
				for (iJ = 0; iJ < GC.getNumBonusInfos(); iJ++)
				{
					iMultiplier += (GET_PLAYER((PlayerTypes)iI).getNumAvailableBonuses((BonusTypes)iJ) * 945732);
					iMultiplier += (GET_PLAYER((PlayerTypes)iI).getBonusImport((BonusTypes)iJ) * 326443);
					iMultiplier += (GET_PLAYER((PlayerTypes)iI).getBonusExport((BonusTypes)iJ) * 932211);
				}

				for (iJ = 0; iJ < GC.getNumImprovementInfos(); iJ++)
				{
					iMultiplier += (GET_PLAYER((PlayerTypes)iI).getImprovementCount((ImprovementTypes)iJ) * 883422);
				}

				for (iJ = 0; iJ < GC.getNumBuildingClassInfos(); iJ++)
				{
					iMultiplier += (GET_PLAYER((PlayerTypes)iI).getBuildingClassCountPlusMaking((BuildingClassTypes)iJ) * 954531);
				}

				for (iJ = 0; iJ < GC.getNumUnitClassInfos(); iJ++)
				{
					iMultiplier += (GET_PLAYER((PlayerTypes)iI).getUnitClassCountPlusMaking((UnitClassTypes)iJ) * 754843);
				}

				for (iJ = 0; iJ < NUM_UNITAI_TYPES; iJ++)
				{
					iMultiplier += (GET_PLAYER((PlayerTypes)iI).AI_totalUnitAIs((UnitAITypes)iJ) * 643383);
				}
				break;

			case 3:
				for (pLoopUnit = GET_PLAYER((PlayerTypes)iI).firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = GET_PLAYER((PlayerTypes)iI).nextUnit(&iLoop))
				{
					iMultiplier += (pLoopUnit->getX_INLINE() * 876543);
					iMultiplier += (pLoopUnit->getY_INLINE() * 985310);
					iMultiplier += (pLoopUnit->getDamage() * 736373);
					iMultiplier += (pLoopUnit->getExperience() * 820622);
					iMultiplier += (pLoopUnit->getLevel() * 367291);
				}
				break;
			}

			if (iMultiplier != 0)
			{
				iValue *= iMultiplier;
			}
		}
	}

	return iValue;
}


int CvGame::calculateOptionsChecksum()
{
	PROFILE_FUNC();

	int iValue;
	int iI, iJ;

	iValue = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		for (iJ = 0; iJ < NUM_PLAYEROPTION_TYPES; iJ++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isOption((PlayerOptionTypes)iJ))
			{
				iValue += (iI * 943097);
				iValue += (iJ * 281541);
			}
		}
	}

	return iValue;
}


void CvGame::addReplayMessage(ReplayMessageTypes eType, PlayerTypes ePlayer, CvWString pszText, int iPlotX, int iPlotY, ColorTypes eColor)
{
	int iGameTurn = getGameTurn();
	CvReplayMessage* pMessage = new CvReplayMessage(iGameTurn, eType, ePlayer);
	if (NULL != pMessage)
	{
		pMessage->setPlot(iPlotX, iPlotY);
		pMessage->setText(pszText);
		if (NO_COLOR == eColor)
		{
			eColor = (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE");
		}
		pMessage->setColor(eColor);
		m_listReplayMessages.push_back(pMessage);
	}
}

void CvGame::clearReplayMessageMap()
{
	for (ReplayMessageList::const_iterator itList = m_listReplayMessages.begin(); itList != m_listReplayMessages.end(); itList++)
	{
		const CvReplayMessage* pMessage = *itList;
		if (NULL != pMessage)
		{
			delete pMessage;
		}
	}
	m_listReplayMessages.clear();
}

int CvGame::getReplayMessageTurn(uint i) const
{
	if (i >= m_listReplayMessages.size())
	{
		return (-1);
	}
	const CvReplayMessage* pMessage =  m_listReplayMessages[i];
	if (NULL == pMessage)
	{
		return (-1);
	}
	return pMessage->getTurn();
}

ReplayMessageTypes CvGame::getReplayMessageType(uint i) const
{
	if (i >= m_listReplayMessages.size())
	{
		return (NO_REPLAY_MESSAGE);
	}
	const CvReplayMessage* pMessage =  m_listReplayMessages[i];
	if (NULL == pMessage)
	{
		return (NO_REPLAY_MESSAGE);
	}
	return pMessage->getType();
}

int CvGame::getReplayMessagePlotX(uint i) const
{
	if (i >= m_listReplayMessages.size())
	{
		return (-1);
	}
	const CvReplayMessage* pMessage =  m_listReplayMessages[i];
	if (NULL == pMessage)
	{
		return (-1);
	}
	return pMessage->getPlotX();
}

int CvGame::getReplayMessagePlotY(uint i) const
{
	if (i >= m_listReplayMessages.size())
	{
		return (-1);
	}
	const CvReplayMessage* pMessage =  m_listReplayMessages[i];
	if (NULL == pMessage)
	{
		return (-1);
	}
	return pMessage->getPlotY();
}

PlayerTypes CvGame::getReplayMessagePlayer(uint i) const
{
	if (i >= m_listReplayMessages.size())
	{
		return (NO_PLAYER);
	}
	const CvReplayMessage* pMessage =  m_listReplayMessages[i];
	if (NULL == pMessage)
	{
		return (NO_PLAYER);
	}
	return pMessage->getPlayer();
}

LPCWSTR CvGame::getReplayMessageText(uint i) const
{
	if (i >= m_listReplayMessages.size())
	{
		return (NULL);
	}
	const CvReplayMessage* pMessage =  m_listReplayMessages[i];
	if (NULL == pMessage)
	{
		return (NULL);
	}
	return pMessage->getText().GetCString();
}

ColorTypes CvGame::getReplayMessageColor(uint i) const
{
	if (i >= m_listReplayMessages.size())
	{
		return (NO_COLOR);
	}
	const CvReplayMessage* pMessage =  m_listReplayMessages[i];
	if (NULL == pMessage)
	{
		return (NO_COLOR);
	}
	return pMessage->getColor();
}


uint CvGame::getNumReplayMessages() const
{
	return m_listReplayMessages.size();
}

// Private Functions...

void CvGame::read(FDataStreamBase* pStream)
{
	int iI;

	reset(NO_HANDICAP);

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	if (uiFlag < 1)
	{
		int iEndTurnMessagesSent;
		pStream->Read(&iEndTurnMessagesSent);
	}
	pStream->Read(&m_iElapsedGameTurns);
	pStream->Read(&m_iStartTurn);
	pStream->Read(&m_iStartYear);
	pStream->Read(&m_iEstimateEndTurn);
	pStream->Read(&m_iTurnSlice);
	pStream->Read(&m_iCutoffSlice);
	pStream->Read(&m_iNumGameTurnActive);
	pStream->Read(&m_iNumCities);
	pStream->Read(&m_iTotalPopulation);
	pStream->Read(&m_iTradeRoutes);
	pStream->Read(&m_iFreeTradeCount);
	pStream->Read(&m_iNoNukesCount);
	pStream->Read(&m_iNukesExploded);
	pStream->Read(&m_iMaxPopulation);
	pStream->Read(&m_iMaxLand);
	pStream->Read(&m_iMaxTech);
	pStream->Read(&m_iMaxWonders);
	pStream->Read(&m_iInitPopulation);
	pStream->Read(&m_iInitLand);
	pStream->Read(&m_iInitTech);
	pStream->Read(&m_iInitWonders);
	pStream->Read(&m_iAIAutoPlay);
/*************************************************************************************************/
/**	xUPT								02/08/11									Afforess	**/
/**																								**/
/**						xUPT mechanic, ported and modified by Valkrionn							**/
/*************************************************************************************************/
	pStream->Read(&m_iUPT);
	pStream->Read(&m_bUPTLock);
/*************************************************************************************************/
/**	xUPT									END													**/
/*************************************************************************************************/

	// m_uiInitialTime not saved

	pStream->Read(&m_bScoreDirty);
	pStream->Read(&m_bCircumnavigated);
	// m_bDebugMode not saved
	pStream->Read(&m_bFinalInitialized);
	// m_bPbemTurnSent not saved
	pStream->Read(&m_bHotPbemBetweenTurns);
	// m_bPlayerOptionsSent not saved
	pStream->Read(&m_bNukesValid);

	pStream->Read((int*)&m_eHandicap);
	pStream->Read((int*)&m_ePausePlayer);
	pStream->Read((int*)&m_eBestLandUnit);
	pStream->Read((int*)&m_eWinner);
	pStream->Read((int*)&m_eVictory);
	pStream->Read((int*)&m_eGameState);

	pStream->ReadString(m_szScriptData);

	if (uiFlag < 1)
	{
		std::vector<int> aiEndTurnMessagesReceived(MAX_PLAYERS);
		pStream->Read(MAX_PLAYERS, &aiEndTurnMessagesReceived[0]);
	}
	pStream->Read(MAX_PLAYERS, m_aiRankPlayer);
	pStream->Read(MAX_PLAYERS, m_aiPlayerRank);
	pStream->Read(MAX_PLAYERS, m_aiPlayerScore);
	pStream->Read(MAX_TEAMS, m_aiRankTeam);
	pStream->Read(MAX_TEAMS, m_aiTeamRank);
	pStream->Read(MAX_TEAMS, m_aiTeamScore);

	pStream->Read(GC.getNumUnitInfos(), m_paiUnitCreatedCount);
	pStream->Read(GC.getNumUnitClassInfos(), m_paiUnitClassCreatedCount);
/*************************************************************************************************/
/**	Spawn Groups						08/05/10									Valkrionn	**/
/**																								**/
/**					New spawn mechanic, allowing us to customize stacks							**/
/*************************************************************************************************/
	pStream->Read(GC.getNumSpawnGroupInfos(), m_paiSpawnGroupCreatedCount);
/*************************************************************************************************/
/**	Spawn Groups							END													**/
/*************************************************************************************************/
	pStream->Read(GC.getNumBuildingClassInfos(), m_paiBuildingClassCreatedCount);
	pStream->Read(GC.getNumProjectInfos(), m_paiProjectCreatedCount);
	pStream->Read(GC.getNumCivicInfos(), m_paiForceCivicCount);
	pStream->Read(GC.getNumVoteInfos(), (int*)m_paiVoteOutcome);
	pStream->Read(GC.getNumReligionInfos(), m_paiReligionGameTurnFounded);
	pStream->Read(GC.getNumCorporationInfos(), m_paiCorporationGameTurnFounded);
	pStream->Read(GC.getNumVoteSourceInfos(), m_aiSecretaryGeneralTimer);
	pStream->Read(GC.getNumVoteSourceInfos(), m_aiVoteTimer);
	pStream->Read(GC.getNumVoteSourceInfos(), m_aiDiploVote);

	pStream->Read(GC.getNumSpecialUnitInfos(), m_pabSpecialUnitValid);
	pStream->Read(GC.getNumSpecialBuildingInfos(), m_pabSpecialBuildingValid);
	pStream->Read(GC.getNumReligionInfos(), m_abReligionSlotTaken);

	for (iI=0;iI<GC.getNumReligionInfos();iI++)
	{
		pStream->Read((int*)&m_paHolyCity[iI].eOwner);
		pStream->Read(&m_paHolyCity[iI].iID);
	}

	for (iI=0;iI<GC.getNumCorporationInfos();iI++)
	{
		pStream->Read((int*)&m_paHeadquarters[iI].eOwner);
		pStream->Read(&m_paHeadquarters[iI].iID);
	}

	{
		CvWString szBuffer;
		uint iSize;

		m_aszDestroyedCities.clear();
		pStream->Read(&iSize);
		for (uint i = 0; i < iSize; i++)
		{
			pStream->ReadString(szBuffer);
			m_aszDestroyedCities.push_back(szBuffer);
		}

		m_aszGreatPeopleBorn.clear();
		pStream->Read(&iSize);
		for (uint i = 0; i < iSize; i++)
		{
			pStream->ReadString(szBuffer);
			m_aszGreatPeopleBorn.push_back(szBuffer);
		}
	}

	ReadStreamableFFreeListTrashArray(m_deals, pStream);
	ReadStreamableFFreeListTrashArray(m_voteSelections, pStream);
	ReadStreamableFFreeListTrashArray(m_votesTriggered, pStream);

	m_mapRand.read(pStream);
	m_sorenRand.read(pStream);

	{
		clearReplayMessageMap();
		ReplayMessageList::_Alloc::size_type iSize;
		pStream->Read(&iSize);
		for (ReplayMessageList::_Alloc::size_type i = 0; i < iSize; i++)
		{
			CvReplayMessage* pMessage = new CvReplayMessage(0);
			if (NULL != pMessage)
			{
				pMessage->read(*pStream);
			}
			m_listReplayMessages.push_back(pMessage);
		}
	}
	// m_pReplayInfo not saved

	pStream->Read(&m_iNumSessions);
	if (!isNetworkMultiPlayer())
	{
		++m_iNumSessions;
	}

	{
		int iSize;
		m_aPlotExtraYields.clear();
		pStream->Read(&iSize);
		for (int i = 0; i < iSize; ++i)
		{
			PlotExtraYield kPlotYield;
			kPlotYield.read(pStream);
			m_aPlotExtraYields.push_back(kPlotYield);
		}
	}

	{
		int iSize;
		m_aPlotExtraCosts.clear();
		pStream->Read(&iSize);
		for (int i = 0; i < iSize; ++i)
		{
			PlotExtraCost kPlotCost;
			kPlotCost.read(pStream);
			m_aPlotExtraCosts.push_back(kPlotCost);
		}
	}

	{
		int iSize;
		m_mapVoteSourceReligions.clear();
		pStream->Read(&iSize);
		for (int i = 0; i < iSize; ++i)
		{
			VoteSourceTypes eVoteSource;
			ReligionTypes eReligion;
			pStream->Read((int*)&eVoteSource);
			pStream->Read((int*)&eReligion);
			m_mapVoteSourceReligions[eVoteSource] = eReligion;
		}
	}

	{
		int iSize;
		m_aeInactiveTriggers.clear();
		pStream->Read(&iSize);
		for (int i = 0; i < iSize; ++i)
		{
			int iTrigger;
			pStream->Read(&iTrigger);
			m_aeInactiveTriggers.push_back((EventTriggerTypes)iTrigger);
		}
	}

	// Get the active player information from the initialization structure
	if (!isGameMultiPlayer())
	{
		for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isHuman())
			{
				setActivePlayer((PlayerTypes)iI);
				break;
			}
		}
		addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getActivePlayer(), gDLL->getText("TXT_KEY_MISC_RELOAD", m_iNumSessions));
	}

	if (isOption(GAMEOPTION_NEW_RANDOM_SEED))
	{
		if (!isNetworkMultiPlayer())
		{
			m_sorenRand.reseed(timeGetTime());
		}
	}

	pStream->Read(&m_iShrineBuildingCount);
	pStream->Read(GC.getNumBuildingInfos(), m_aiShrineBuilding);
	pStream->Read(GC.getNumBuildingInfos(), m_aiShrineReligion);
	pStream->Read(&m_iNumCultureVictoryCities);
	pStream->Read(&m_eCultureVictoryCultureLevel);

//FfH: Added by Kael 08/07/2007
	pStream->Read(&m_iCrime);
	pStream->Read(&m_iCutLosersCounter);
	pStream->Read(&m_iFlexibleDifficultyCounter);
	pStream->Read(&m_iGlobalCounter);
	pStream->Read(&m_iHighToLowCounter);
	pStream->Read(&m_iIncreasingDifficultyCounter);
	pStream->Read(&m_iMaxGlobalCounter);
	pStream->Read(&m_iGlobalCounterLimit);
	pStream->Read(&m_iScenarioCounter);
	pStream->Read(GC.getNumEventTriggerInfos(), m_pabEventTriggered);
	pStream->Read(GC.getNumVoteSourceInfos(), m_pabGamblingRing);
	pStream->Read(GC.getNumBonusInfos(), m_pabNoBonus);
	pStream->Read(GC.getNumVoteSourceInfos(), m_pabNoOutsideTechTrades);
	pStream->Read(GC.getNumVoteSourceInfos(), m_pabSlaveTrade);
	pStream->Read(GC.getNumVoteSourceInfos(), m_pabSmugglingRing);
//FfH: End Add
/*************************************************************************************************/
/**	New Tag Defs	(ProjectInfos)			10/01/08								Xienwolf	**/
/**	New Tag Defs	(GameInfos)				10/01/08								Xienwolf	**/
/**									Read Data from Save Files									**/
/*************************************************************************************************/
	for (int i=0;i<GC.getNumProjectInfos();i++)
	{
		pStream->Read(GC.getMaxNumProjectsAllowed(), m_ppaaiProjectTimers[i]);
	}
/*************************************************************************************************/
/**	New Tag Defs							END													**/
/*************************************************************************************************/

}


void CvGame::write(FDataStreamBase* pStream)
{
	int iI;

	uint uiFlag=1;
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(m_iElapsedGameTurns);
	pStream->Write(m_iStartTurn);
	pStream->Write(m_iStartYear);
	pStream->Write(m_iEstimateEndTurn);
	pStream->Write(m_iTurnSlice);
	pStream->Write(m_iCutoffSlice);
	pStream->Write(m_iNumGameTurnActive);
	pStream->Write(m_iNumCities);
	pStream->Write(m_iTotalPopulation);
	pStream->Write(m_iTradeRoutes);
	pStream->Write(m_iFreeTradeCount);
	pStream->Write(m_iNoNukesCount);
	pStream->Write(m_iNukesExploded);
	pStream->Write(m_iMaxPopulation);
	pStream->Write(m_iMaxLand);
	pStream->Write(m_iMaxTech);
	pStream->Write(m_iMaxWonders);
	pStream->Write(m_iInitPopulation);
	pStream->Write(m_iInitLand);
	pStream->Write(m_iInitTech);
	pStream->Write(m_iInitWonders);
/*************************************************************************************************/
/**	Xienwolf Tweak							02/01/09											**/
/**																								**/
/**							Stops AutoPlay after reloading a game								**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	pStream->Write(m_iAIAutoPlay);
/**								----  End Original Code  ----									**/
	pStream->Write(m_iAIAutoPlay);
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	xUPT								02/08/11									Afforess	**/
/**																								**/
/**						xUPT mechanic, ported and modified by Valkrionn							**/
/*************************************************************************************************/
	pStream->Write(m_iUPT);
	pStream->Write(m_bUPTLock);
/*************************************************************************************************/
/**	xUPT									END													**/
/*************************************************************************************************/

	// m_uiInitialTime not saved

	pStream->Write(m_bScoreDirty);
	pStream->Write(m_bCircumnavigated);
	// m_bDebugMode not saved
	pStream->Write(m_bFinalInitialized);
	// m_bPbemTurnSent not saved
	pStream->Write(m_bHotPbemBetweenTurns);
	// m_bPlayerOptionsSent not saved
	pStream->Write(m_bNukesValid);

	pStream->Write(m_eHandicap);
	pStream->Write(m_ePausePlayer);
	pStream->Write(m_eBestLandUnit);
	pStream->Write(m_eWinner);
	pStream->Write(m_eVictory);
	pStream->Write(m_eGameState);

	pStream->WriteString(m_szScriptData);

	pStream->Write(MAX_PLAYERS, m_aiRankPlayer);
	pStream->Write(MAX_PLAYERS, m_aiPlayerRank);
	pStream->Write(MAX_PLAYERS, m_aiPlayerScore);
	pStream->Write(MAX_TEAMS, m_aiRankTeam);
	pStream->Write(MAX_TEAMS, m_aiTeamRank);
	pStream->Write(MAX_TEAMS, m_aiTeamScore);

	pStream->Write(GC.getNumUnitInfos(), m_paiUnitCreatedCount);
	pStream->Write(GC.getNumUnitClassInfos(), m_paiUnitClassCreatedCount);
/*************************************************************************************************/
/**	Spawn Groups						08/05/10									Valkrionn	**/
/**																								**/
/**					New spawn mechanic, allowing us to customize stacks							**/
/*************************************************************************************************/
	pStream->Write(GC.getNumSpawnGroupInfos(), m_paiSpawnGroupCreatedCount);
/*************************************************************************************************/
/**	Spawn Groups							END													**/
/*************************************************************************************************/
	pStream->Write(GC.getNumBuildingClassInfos(), m_paiBuildingClassCreatedCount);
	pStream->Write(GC.getNumProjectInfos(), m_paiProjectCreatedCount);
	pStream->Write(GC.getNumCivicInfos(), m_paiForceCivicCount);
	pStream->Write(GC.getNumVoteInfos(), (int*)m_paiVoteOutcome);
	pStream->Write(GC.getNumReligionInfos(), m_paiReligionGameTurnFounded);
	pStream->Write(GC.getNumCorporationInfos(), m_paiCorporationGameTurnFounded);
	pStream->Write(GC.getNumVoteSourceInfos(), m_aiSecretaryGeneralTimer);
	pStream->Write(GC.getNumVoteSourceInfos(), m_aiVoteTimer);
	pStream->Write(GC.getNumVoteSourceInfos(), m_aiDiploVote);

	pStream->Write(GC.getNumSpecialUnitInfos(), m_pabSpecialUnitValid);
	pStream->Write(GC.getNumSpecialBuildingInfos(), m_pabSpecialBuildingValid);
	pStream->Write(GC.getNumReligionInfos(), m_abReligionSlotTaken);

	for (iI=0;iI<GC.getNumReligionInfos();iI++)
	{
		pStream->Write(m_paHolyCity[iI].eOwner);
		pStream->Write(m_paHolyCity[iI].iID);
	}

	for (iI=0;iI<GC.getNumCorporationInfos();iI++)
	{
		pStream->Write(m_paHeadquarters[iI].eOwner);
		pStream->Write(m_paHeadquarters[iI].iID);
	}

	{
		std::vector<CvWString>::iterator it;

		pStream->Write(m_aszDestroyedCities.size());
		for (it = m_aszDestroyedCities.begin(); it != m_aszDestroyedCities.end(); it++)
		{
			pStream->WriteString(*it);
		}

		pStream->Write(m_aszGreatPeopleBorn.size());
		for (it = m_aszGreatPeopleBorn.begin(); it != m_aszGreatPeopleBorn.end(); it++)
		{
			pStream->WriteString(*it);
		}
	}

	WriteStreamableFFreeListTrashArray(m_deals, pStream);
	WriteStreamableFFreeListTrashArray(m_voteSelections, pStream);
	WriteStreamableFFreeListTrashArray(m_votesTriggered, pStream);

	m_mapRand.write(pStream);
	m_sorenRand.write(pStream);

	ReplayMessageList::_Alloc::size_type iSize = m_listReplayMessages.size();
	pStream->Write(iSize);
	ReplayMessageList::const_iterator it;
	for (it = m_listReplayMessages.begin(); it != m_listReplayMessages.end(); it++)
	{
		const CvReplayMessage* pMessage = *it;
		if (NULL != pMessage)
		{
			pMessage->write(*pStream);
		}
	}
	// m_pReplayInfo not saved

	pStream->Write(m_iNumSessions);

	pStream->Write(m_aPlotExtraYields.size());
	for (std::vector<PlotExtraYield>::iterator it = m_aPlotExtraYields.begin(); it != m_aPlotExtraYields.end(); ++it)
	{
		(*it).write(pStream);
	}

	pStream->Write(m_aPlotExtraCosts.size());
	for (std::vector<PlotExtraCost>::iterator it = m_aPlotExtraCosts.begin(); it != m_aPlotExtraCosts.end(); ++it)
	{
		(*it).write(pStream);
	}

	pStream->Write(m_mapVoteSourceReligions.size());
	for (stdext::hash_map<VoteSourceTypes, ReligionTypes>::iterator it = m_mapVoteSourceReligions.begin(); it != m_mapVoteSourceReligions.end(); ++it)
	{
		pStream->Write(it->first);
		pStream->Write(it->second);
	}

	pStream->Write(m_aeInactiveTriggers.size());
	for (std::vector<EventTriggerTypes>::iterator it = m_aeInactiveTriggers.begin(); it != m_aeInactiveTriggers.end(); ++it)
	{
		pStream->Write(*it);
	}

	pStream->Write(m_iShrineBuildingCount);
	pStream->Write(GC.getNumBuildingInfos(), m_aiShrineBuilding);
	pStream->Write(GC.getNumBuildingInfos(), m_aiShrineReligion);
	pStream->Write(m_iNumCultureVictoryCities);
	pStream->Write(m_eCultureVictoryCultureLevel);

//FfH: Added by Kael 08/07/2007
	pStream->Write(m_iCrime);
	pStream->Write(m_iCutLosersCounter);
	pStream->Write(m_iFlexibleDifficultyCounter);
	pStream->Write(m_iGlobalCounter);
	pStream->Write(m_iHighToLowCounter);
	pStream->Write(m_iIncreasingDifficultyCounter);
	pStream->Write(m_iMaxGlobalCounter);
	pStream->Write(m_iGlobalCounterLimit);
	pStream->Write(m_iScenarioCounter);
	pStream->Write(GC.getNumEventTriggerInfos(), m_pabEventTriggered);
	pStream->Write(GC.getNumVoteSourceInfos(), m_pabGamblingRing);
	pStream->Write(GC.getNumBonusInfos(), m_pabNoBonus);
	pStream->Write(GC.getNumVoteSourceInfos(), m_pabNoOutsideTechTrades);
	pStream->Write(GC.getNumVoteSourceInfos(), m_pabSlaveTrade);
	pStream->Write(GC.getNumVoteSourceInfos(), m_pabSmugglingRing);
//FfH: End Add
/*************************************************************************************************/
/**	New Tag Defs	(ProjectInfos)			10/01/08								Xienwolf	**/
/**	New Tag Defs	(GameInfos)				10/01/08								Xienwolf	**/
/**									Write Data to Save Files									**/
/*************************************************************************************************/
	for (iI=0;iI<GC.getNumProjectInfos();iI++)
	{
		pStream->Write(GC.getMaxNumProjectsAllowed(), m_ppaaiProjectTimers[iI]);
	}
/*************************************************************************************************/
/**	New Tag Defs							END													**/
/*************************************************************************************************/

}

void CvGame::writeReplay(FDataStreamBase& stream, PlayerTypes ePlayer)
{
	SAFE_DELETE(m_pReplayInfo);
	m_pReplayInfo = new CvReplayInfo();
	if (m_pReplayInfo)
	{
		m_pReplayInfo->createInfo(ePlayer);

		m_pReplayInfo->write(stream);
	}
}

void CvGame::saveReplay(PlayerTypes ePlayer)
{
	gDLL->getEngineIFace()->SaveReplay(ePlayer);
}


void CvGame::showEndGameSequence()
{
	CvPopupInfo* pInfo;
	CvWString szBuffer;
	int iI;

	long iHours = getMinutesPlayed() / 60;
	long iMinutes = getMinutesPlayed() % 60;

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer& player = GET_PLAYER((PlayerTypes)iI);
		if (player.isHuman())
		{
			addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, (PlayerTypes)iI, gDLL->getText("TXT_KEY_MISC_TIME_SPENT", iHours, iMinutes));

			pInfo = new CvPopupInfo(BUTTONPOPUP_TEXT);
			if (NULL != pInfo)
			{
				if ((getWinner() != NO_TEAM) && (getVictory() != NO_VICTORY))
				{
					pInfo->setText(gDLL->getText("TXT_KEY_GAME_WON", GET_TEAM(getWinner()).getName().GetCString(), GC.getVictoryInfo(getVictory()).getTextKeyWide()));
				}
				else
				{
					pInfo->setText(gDLL->getText("TXT_KEY_MISC_DEFEAT"));
				}
				player.addPopup(pInfo);
			}

			if (getWinner() == player.getTeam())
			{
				if (!CvString(GC.getVictoryInfo(getVictory()).getMovie()).empty())
				{
					// show movie
					pInfo = new CvPopupInfo(BUTTONPOPUP_PYTHON_SCREEN);
					if (NULL != pInfo)
					{
						pInfo->setText(L"showVictoryMovie");
						pInfo->setData1((int)getVictory());
						player.addPopup(pInfo);
					}
				}
				else if (GC.getVictoryInfo(getVictory()).isDiploVote())
				{
					pInfo = new CvPopupInfo(BUTTONPOPUP_PYTHON_SCREEN);
					if (NULL != pInfo)
					{
						pInfo->setText(L"showUnVictoryScreen");
						player.addPopup(pInfo);
					}
				}
			}

			// show replay
			pInfo = new CvPopupInfo(BUTTONPOPUP_PYTHON_SCREEN);
			if (NULL != pInfo)
			{
				pInfo->setText(L"showReplay");
				pInfo->setData1(iI);
				pInfo->setOption1(false); // don't go to HOF on exit
				player.addPopup(pInfo);
			}

			// show top cities / stats
			pInfo = new CvPopupInfo(BUTTONPOPUP_PYTHON_SCREEN);
			if (NULL != pInfo)
			{
				pInfo->setText(L"showInfoScreen");
				pInfo->setData1(0);
				pInfo->setData2(1);
				player.addPopup(pInfo);
			}

			// show Dan
			pInfo = new CvPopupInfo(BUTTONPOPUP_PYTHON_SCREEN);
			if (NULL != pInfo)
			{
				pInfo->setText(L"showDanQuayleScreen");
				player.addPopup(pInfo);
			}

			// show Hall of Fame
			pInfo = new CvPopupInfo(BUTTONPOPUP_PYTHON_SCREEN);
			if (NULL != pInfo)
			{
				pInfo->setText(L"showHallOfFame");
				player.addPopup(pInfo);
			}
		}
	}
}

CvReplayInfo* CvGame::getReplayInfo() const
{
	return m_pReplayInfo;
}

void CvGame::setReplayInfo(CvReplayInfo* pReplay)
{
	SAFE_DELETE(m_pReplayInfo);
	m_pReplayInfo = pReplay;
}

bool CvGame::hasSkippedSaveChecksum() const
{
	return gDLL->hasSkippedSaveChecksum();
}

void CvGame::addPlayer(PlayerTypes eNewPlayer, LeaderHeadTypes eLeader, CivilizationTypes eCiv)
{
	// UNOFFICIAL_PATCH Start
	// * Fixed bug with colonies who occupy recycled player slots showing the old leader or civ names.
	CvWString szEmptyString = L"";
	LeaderHeadTypes eOldLeader = GET_PLAYER(eNewPlayer).getLeaderType();
	if ( (eOldLeader != NO_LEADER) && (eOldLeader != eLeader) )
	{
		GC.getInitCore().setLeaderName(eNewPlayer, szEmptyString);
	}
	CivilizationTypes eOldCiv = GET_PLAYER(eNewPlayer).getCivilizationType();
	if ( (eOldCiv != NO_CIVILIZATION) && (eOldCiv != eCiv) )
	{
		GC.getInitCore().setCivAdjective(eNewPlayer, szEmptyString);
		GC.getInitCore().setCivDescription(eNewPlayer, szEmptyString);
		GC.getInitCore().setCivShortDesc(eNewPlayer, szEmptyString);
	}
	// UNOFFICIAL_PATCH End
	PlayerColorTypes eColor = (PlayerColorTypes)GC.getCivilizationInfo(eCiv).getDefaultPlayerColor();

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{

/************************************************************************************************/
/* UNOFFICIAL_PATCH                       12/30/08                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original bts code
		if (eColor == NO_PLAYERCOLOR || GET_PLAYER((PlayerTypes)iI).getPlayerColor() == eColor)
*/
		// Don't invalidate color choice if it's taken by this player
		if (eColor == NO_PLAYERCOLOR || (GET_PLAYER((PlayerTypes)iI).getPlayerColor() == eColor && (PlayerTypes)iI != eNewPlayer) )
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
		{
			for (int iK = 0; iK < GC.getNumPlayerColorInfos(); iK++)
			{
/*************************************************************************************************/
/**	MultiBarb								12/23/08								Xienwolf	**/
/**																								**/
/**							Adds extra Barbarian Civilizations									**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
				if (iK != GC.getCivilizationInfo((CivilizationTypes)GC.getDefineINT("BARBARIAN_CIVILIZATION")).getDefaultPlayerColor())
/**								----  End Original Code  ----									**/
				if (iK != GC.getCivilizationInfo((CivilizationTypes)GC.getDefineINT("ORC_CIVILIZATION")).getDefaultPlayerColor() && iK != GC.getCivilizationInfo((CivilizationTypes)GC.getDefineINT("ANIMAL_CIVILIZATION")).getDefaultPlayerColor() && iK != GC.getCivilizationInfo((CivilizationTypes)GC.getDefineINT("DEMON_CIVILIZATION")).getDefaultPlayerColor())
/*************************************************************************************************/
/**	MultiBarb								END													**/
/*************************************************************************************************/
				{
					bool bValid = true;

					for (int iL = 0; iL < MAX_CIV_PLAYERS; iL++)
					{
						if (GET_PLAYER((PlayerTypes)iL).getPlayerColor() == iK)
						{
							bValid = false;
							break;
						}
					}

					if (bValid)
					{
						eColor = (PlayerColorTypes)iK;
						iI = MAX_CIV_PLAYERS;
						break;
					}
				}
			}
		}
	}

	TeamTypes eTeam = GET_PLAYER(eNewPlayer).getTeam();
	GC.getInitCore().setLeader(eNewPlayer, eLeader);
	GC.getInitCore().setCiv(eNewPlayer, eCiv);
	GC.getInitCore().setSlotStatus(eNewPlayer, SS_COMPUTER);
	GC.getInitCore().setColor(eNewPlayer, eColor);
	GET_TEAM(eTeam).init(eTeam);
	GET_PLAYER(eNewPlayer).init(eNewPlayer);
}

//FfH: Added by Kael 08/24/2007
void CvGame::addPlayerAdvanced(PlayerTypes eNewPlayer, int iNewTeam, LeaderHeadTypes eLeader, CivilizationTypes eCiv,PlayerTypes eSpawnPlayer)
{
	PlayerColorTypes eColor = (PlayerColorTypes)GC.getCivilizationInfo(eCiv).getDefaultPlayerColor();
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (eColor == NO_PLAYERCOLOR || GET_PLAYER((PlayerTypes)iI).getPlayerColor() == eColor)
		{
			for (int iK = 0; iK < GC.getNumPlayerColorInfos(); iK++)
			{
/*************************************************************************************************/
/**	MultiBarb								12/23/08								Xienwolf	**/
/**																								**/
/**							Adds extra Barbarian Civilizations									**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
				if (iK != GC.getCivilizationInfo((CivilizationTypes)GC.getDefineINT("BARBARIAN_CIVILIZATION")).getDefaultPlayerColor())
/**								----  End Original Code  ----									**/
				if (iK != GC.getCivilizationInfo((CivilizationTypes)GC.getDefineINT("ORC_CIVILIZATION")).getDefaultPlayerColor() && iK != GC.getCivilizationInfo((CivilizationTypes)GC.getDefineINT("ANIMAL_CIVILIZATION")).getDefaultPlayerColor() && iK != GC.getCivilizationInfo((CivilizationTypes)GC.getDefineINT("DEMON_CIVILIZATION")).getDefaultPlayerColor())
/*************************************************************************************************/
/**	MultiBarb								END													**/
/*************************************************************************************************/
				{
					bool bValid = true;
					for (int iL = 0; iL < MAX_CIV_PLAYERS; iL++)
					{
						if (GET_PLAYER((PlayerTypes)iL).getPlayerColor() == iK)
						{
							bValid = false;
							break;
						}
					}
					if (bValid)
					{
						eColor = (PlayerColorTypes)iK;
						iI = MAX_CIV_PLAYERS;
						break;
					}
				}
			}
		}
	}
	if (iNewTeam != NO_TEAM)
	{
		GET_PLAYER(eNewPlayer).setTeam((TeamTypes)iNewTeam);
		gDLL->getInterfaceIFace()->setDirty(Fog_DIRTY_BIT, true);
	}
	else
	{
		bool bValid = false;
		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			if (!GET_TEAM((TeamTypes)iI).isEverAlive() && bValid == false)
			{
				iNewTeam = iI;
				bValid = true;
			}
		}
		if (bValid)
		{
			GET_TEAM((TeamTypes)iNewTeam).init((TeamTypes)iNewTeam);
			for (int iJ = 0; iJ < MAX_TEAMS; iJ++)
			{
				if (GET_TEAM((TeamTypes)iJ).isBarbarian() || GET_TEAM((TeamTypes)iJ).isMinorCiv())
				{
					if ((TeamTypes)iNewTeam != iJ)
					{
						GET_TEAM((TeamTypes)iNewTeam).setAtWar((TeamTypes)iJ, true);
/*************************************************************************************************/
/**	Tweak								28/06/10										Snarko	**/
/**																								**/
/**						We want both teams to be at war with eachother.							**/
/*************************************************************************************************/
						GET_TEAM((TeamTypes)iJ).setAtWar((TeamTypes)iNewTeam, true);
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
					}
				}
			}
		}
	}
	GC.getInitCore().setTeam(eNewPlayer, (TeamTypes)iNewTeam);
	GC.getInitCore().setLeader(eNewPlayer, eLeader);
	GC.getInitCore().setCiv(eNewPlayer, eCiv);
	GC.getInitCore().setSlotStatus(eNewPlayer, SS_COMPUTER);
	GC.getInitCore().setColor(eNewPlayer, eColor);
	GET_PLAYER(eNewPlayer).init(eNewPlayer);

	for (int iI = 0; iI < GC.getNumTechInfos(); ++iI)
	{
		if (GET_TEAM((TeamTypes)iNewTeam).isHasTech((TechTypes)iI))
		{
			GET_PLAYER(eNewPlayer).changeAssets(GC.getTechInfo((TechTypes)iI).getAssetValue());
			GET_PLAYER(eNewPlayer).changePower(GC.getTechInfo((TechTypes)iI).getPowerValue());
			GET_PLAYER(eNewPlayer).changeTechScore(getTechScore((TechTypes)iI));
		}
	}
	if (eSpawnPlayer != NO_PLAYER)
	{
		for (int i = 0; i < GC.getNumFlagInfos(); i++)
		{
			if (GET_PLAYER(eSpawnPlayer).isHasFlag((FlagTypes)i))
			{
				GET_PLAYER(eNewPlayer).setHasFlag((FlagTypes)i, true);
			}
		}
	}
}
//FfH: End Add

bool CvGame::isCompetingCorporation(CorporationTypes eCorporation1, CorporationTypes eCorporation2) const
{
	bool bShareResources = false;

	for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES() && !bShareResources; ++i)
	{
		if (GC.getCorporationInfo(eCorporation1).getPrereqBonus(i) != NO_BONUS)
		{
			for (int j = 0; j < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++j)
			{
				if (GC.getCorporationInfo(eCorporation2).getPrereqBonus(j) != NO_BONUS)
				{
					if (GC.getCorporationInfo(eCorporation1).getPrereqBonus(i) == GC.getCorporationInfo(eCorporation2).getPrereqBonus(j))
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

int CvGame::getPlotExtraYield(int iX, int iY, YieldTypes eYield) const
{
	for (std::vector<PlotExtraYield>::const_iterator it = m_aPlotExtraYields.begin(); it != m_aPlotExtraYields.end(); ++it)
	{
		if ((*it).m_iX == iX && (*it).m_iY == iY)
		{
			return (*it).m_aeExtraYield[eYield];
		}
	}

	return 0;
}

void CvGame::setPlotExtraYield(int iX, int iY, YieldTypes eYield, int iExtraYield)
{
	bool bFound = false;

	for (std::vector<PlotExtraYield>::iterator it = m_aPlotExtraYields.begin(); it != m_aPlotExtraYields.end(); ++it)
	{
		if ((*it).m_iX == iX && (*it).m_iY == iY)
		{
			(*it).m_aeExtraYield[eYield] += iExtraYield;
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		PlotExtraYield kExtraYield;
		kExtraYield.m_iX = iX;
		kExtraYield.m_iY = iY;
		for (int i = 0; i < NUM_YIELD_TYPES; ++i)
		{
			if (eYield == i)
			{
				kExtraYield.m_aeExtraYield.push_back(iExtraYield);
			}
			else
			{
				kExtraYield.m_aeExtraYield.push_back(0);
			}
		}
		m_aPlotExtraYields.push_back(kExtraYield);
	}

	CvPlot* pPlot = GC.getMapINLINE().plot(iX, iY);
	if (NULL != pPlot)
	{
		pPlot->updateYield();
	}
}

void CvGame::removePlotExtraYield(int iX, int iY)
{
	for (std::vector<PlotExtraYield>::iterator it = m_aPlotExtraYields.begin(); it != m_aPlotExtraYields.end(); ++it)
	{
		if ((*it).m_iX == iX && (*it).m_iY == iY)
		{
			m_aPlotExtraYields.erase(it);
			break;
		}
	}

	CvPlot* pPlot = GC.getMapINLINE().plot(iX, iY);
	if (NULL != pPlot)
	{
		pPlot->updateYield();
	}
}

int CvGame::getPlotExtraCost(int iX, int iY) const
{
	for (std::vector<PlotExtraCost>::const_iterator it = m_aPlotExtraCosts.begin(); it != m_aPlotExtraCosts.end(); ++it)
	{
		if ((*it).m_iX == iX && (*it).m_iY == iY)
		{
			return (*it).m_iCost;
		}
	}

	return 0;
}

void CvGame::changePlotExtraCost(int iX, int iY, int iCost)
{
	bool bFound = false;

	for (std::vector<PlotExtraCost>::iterator it = m_aPlotExtraCosts.begin(); it != m_aPlotExtraCosts.end(); ++it)
	{
		if ((*it).m_iX == iX && (*it).m_iY == iY)
		{
			(*it).m_iCost += iCost;
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		PlotExtraCost kExtraCost;
		kExtraCost.m_iX = iX;
		kExtraCost.m_iY = iY;
		kExtraCost.m_iCost = iCost;
		m_aPlotExtraCosts.push_back(kExtraCost);
	}
}

void CvGame::removePlotExtraCost(int iX, int iY)
{
	for (std::vector<PlotExtraCost>::iterator it = m_aPlotExtraCosts.begin(); it != m_aPlotExtraCosts.end(); ++it)
	{
		if ((*it).m_iX == iX && (*it).m_iY == iY)
		{
			m_aPlotExtraCosts.erase(it);
			break;
		}
	}
}

ReligionTypes CvGame::getVoteSourceReligion(VoteSourceTypes eVoteSource) const
{
	stdext::hash_map<VoteSourceTypes, ReligionTypes>::const_iterator it;

	it = m_mapVoteSourceReligions.find(eVoteSource);
	if (it == m_mapVoteSourceReligions.end())
	{
		return NO_RELIGION;
	}

	return it->second;
}

void CvGame::setVoteSourceReligion(VoteSourceTypes eVoteSource, ReligionTypes eReligion, bool bAnnounce)
{
	m_mapVoteSourceReligions[eVoteSource] = eReligion;

	if (bAnnounce)
	{
		if (NO_RELIGION != eReligion)
		{
			CvWString szBuffer = gDLL->getText("TXT_KEY_VOTE_SOURCE_RELIGION", GC.getReligionInfo(eReligion).getTextKeyWide(), GC.getReligionInfo(eReligion).getAdjectiveKey(), GC.getVoteSourceInfo(eVoteSource).getTextKeyWide());

			for (int iI = 0; iI < MAX_PLAYERS; iI++)
			{
				if (GET_PLAYER((PlayerTypes)iI).isAlive())
				{
					gDLL->getInterfaceIFace()->addMessage(((PlayerTypes)iI), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, GC.getReligionInfo(eReligion).getSound(), MESSAGE_TYPE_MAJOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
				}
			}
		}
	}
}


// CACHE: cache frequently used values
///////////////////////////////////////


int CvGame::getShrineBuildingCount(ReligionTypes eReligion)
{
	int	iShrineBuildingCount = 0;

	if (eReligion == NO_RELIGION)
		iShrineBuildingCount = m_iShrineBuildingCount;
	else for (int iI = 0; iI < m_iShrineBuildingCount; iI++)
		if (m_aiShrineReligion[iI] == eReligion)
			iShrineBuildingCount++;

	return iShrineBuildingCount;
}

BuildingTypes CvGame::getShrineBuilding(int eIndex, ReligionTypes eReligion)
{
	FAssertMsg(eIndex >= 0 && eIndex < m_iShrineBuildingCount, "invalid index to CvGame::getShrineBuilding");

	BuildingTypes eBuilding = NO_BUILDING;

	if (eIndex >= 0 && eIndex < m_iShrineBuildingCount)
	{
		if (eReligion == NO_RELIGION)
			eBuilding = (BuildingTypes) m_aiShrineBuilding[eIndex];
		else for (int iI = 0, iReligiousBuilding = 0; iI < m_iShrineBuildingCount; iI++)
			if (m_aiShrineReligion[iI] == (int) eReligion)
			{
				if (iReligiousBuilding == eIndex)
				{
					// found it
					eBuilding = (BuildingTypes) m_aiShrineBuilding[iI];
					break;
				}

				iReligiousBuilding++;
			}
	}

	return eBuilding;
}

void CvGame::changeShrineBuilding(BuildingTypes eBuilding, ReligionTypes eReligion, bool bRemove)
{
	FAssertMsg(eBuilding >= 0 && eBuilding < GC.getNumBuildingInfos(), "invalid index to CvGame::changeShrineBuilding");
	FAssertMsg(bRemove || m_iShrineBuildingCount < GC.getNumBuildingInfos(), "trying to add too many buildings to CvGame::changeShrineBuilding");

	if (bRemove)
	{
		bool bFound = false;

		for (int iI = 0; iI < m_iShrineBuildingCount; iI++)
		{
			if (!bFound)
			{
				// note, eReligion is not important if we removing, since each building is always one religion
				if (m_aiShrineBuilding[iI] == (int) eBuilding)
					bFound = true;
			}

			if (bFound)
			{
				int iToMove = iI + 1;
				if (iToMove < m_iShrineBuildingCount)
				{
					m_aiShrineBuilding[iI] = m_aiShrineBuilding[iToMove];
					m_aiShrineReligion[iI] = m_aiShrineReligion[iToMove];
				}
				else
				{
					m_aiShrineBuilding[iI] = (int) NO_BUILDING;
					m_aiShrineReligion[iI] = (int) NO_RELIGION;
				}
			}

		if (bFound)
			m_iShrineBuildingCount--;

		}
	}
	else if (m_iShrineBuildingCount < GC.getNumBuildingInfos())
	{
		// add this item to the end
		m_aiShrineBuilding[m_iShrineBuildingCount] = eBuilding;
		m_aiShrineReligion[m_iShrineBuildingCount] = eReligion;
		m_iShrineBuildingCount++;
	}

}

bool CvGame::culturalVictoryValid()
{
	if (m_iNumCultureVictoryCities > 0)
	{
		return true;
	}

	return false;
}

int CvGame::culturalVictoryNumCultureCities()
{
	return m_iNumCultureVictoryCities;
}

CultureLevelTypes CvGame::culturalVictoryCultureLevel()
{
	if (m_iNumCultureVictoryCities > 0)
	{
		return (CultureLevelTypes) m_eCultureVictoryCultureLevel;
	}

	return NO_CULTURELEVEL;
}

int CvGame::getCultureThreshold(CultureLevelTypes eLevel) const
{
	int iThreshold = GC.getCultureLevelInfo(eLevel).getSpeedThreshold(getGameSpeedType());
//	if (isOption(GAMEOPTION_NO_ESPIONAGE))
//	{
//		iThreshold *= 100 + GC.getDefineINT("NO_ESPIONAGE_CULTURE_LEVEL_MODIFIER");
//		iThreshold /= 100;
//	}
	return iThreshold;
}

void CvGame::doUpdateCacheOnTurn()
{
	int	iI;

	// reset shrine count
	m_iShrineBuildingCount = 0;

	for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		CvBuildingInfo&	kBuildingInfo = GC.getBuildingInfo((BuildingTypes) iI);

		// if it is for holy city, then its a shrine-thing, add it
		if (kBuildingInfo.getHolyCity() != NO_RELIGION)
		{
			changeShrineBuilding((BuildingTypes) iI, (ReligionTypes) kBuildingInfo.getReligionType());
		}
	}

	// reset cultural victories
	m_iNumCultureVictoryCities = 0;
	for (iI = 0; iI < GC.getNumVictoryInfos(); iI++)
	{
		if (isVictoryValid((VictoryTypes) iI))
		{
			CvVictoryInfo& kVictoryInfo = GC.getVictoryInfo((VictoryTypes) iI);
			if (kVictoryInfo.getCityCulture() > 0)
			{
				int iNumCultureCities = kVictoryInfo.getNumCultureCities();
				if (iNumCultureCities > m_iNumCultureVictoryCities)
				{
					m_iNumCultureVictoryCities = iNumCultureCities;
					m_eCultureVictoryCultureLevel = kVictoryInfo.getCityCulture();
				}
			}
		}
	}
}

VoteSelectionData* CvGame::getVoteSelection(int iID) const
{
	return ((VoteSelectionData*)(m_voteSelections.getAt(iID)));
}

VoteSelectionData* CvGame::addVoteSelection(VoteSourceTypes eVoteSource)
{
	VoteSelectionData* pData = ((VoteSelectionData*)(m_voteSelections.add()));

	if  (NULL != pData)
	{
		pData->eVoteSource = eVoteSource;

		for (int iI = 0; iI < GC.getNumVoteInfos(); iI++)
		{
			if (GC.getVoteInfo((VoteTypes)iI).isVoteSourceType(eVoteSource))
			{
				if (isChooseElection((VoteTypes)iI))
				{
					VoteSelectionSubData kData;
					kData.eVote = (VoteTypes)iI;
					kData.iCityId = -1;
					kData.ePlayer = NO_PLAYER;
					kData.eOtherPlayer = NO_PLAYER;

					if (GC.getVoteInfo(kData.eVote).isOpenBorders())
					{
						if (isValidVoteSelection(eVoteSource, kData))
						{
							kData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_OPEN_BORDERS", getVoteRequired(kData.eVote, eVoteSource), countPossibleVote(kData.eVote, eVoteSource));
							pData->aVoteOptions.push_back(kData);
						}
					}
					else if (GC.getVoteInfo(kData.eVote).isDefensivePact())
					{
						if (isValidVoteSelection(eVoteSource, kData))
						{
							kData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_DEFENSIVE_PACT", getVoteRequired(kData.eVote, eVoteSource), countPossibleVote(kData.eVote, eVoteSource));
							pData->aVoteOptions.push_back(kData);
						}
					}
					else if (GC.getVoteInfo(kData.eVote).isForcePeace())
					{
						for (int iTeam1 = 0; iTeam1 < MAX_CIV_TEAMS; ++iTeam1)
						{
							CvTeam& kTeam1 = GET_TEAM((TeamTypes)iTeam1);

							if (kTeam1.isAlive())
							{
								kData.ePlayer = kTeam1.getLeaderID();

								if (isValidVoteSelection(eVoteSource, kData))
								{
									kData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_FORCE_PEACE", kTeam1.getName().GetCString(), getVoteRequired(kData.eVote, eVoteSource), countPossibleVote(kData.eVote, eVoteSource));
									pData->aVoteOptions.push_back(kData);
								}
							}
						}
					}
					else if (GC.getVoteInfo(kData.eVote).isForceNoTrade())
					{
						for (int iTeam1 = 0; iTeam1 < MAX_CIV_TEAMS; ++iTeam1)
						{
							CvTeam& kTeam1 = GET_TEAM((TeamTypes)iTeam1);

							if (kTeam1.isAlive())
							{
								kData.ePlayer = kTeam1.getLeaderID();

								if (isValidVoteSelection(eVoteSource, kData))
								{
									kData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_FORCE_NO_TRADE", kTeam1.getName().GetCString(), getVoteRequired(kData.eVote, eVoteSource), countPossibleVote(kData.eVote, eVoteSource));
									pData->aVoteOptions.push_back(kData);
								}
							}
						}
					}
					else if (GC.getVoteInfo(kData.eVote).isForceWar())
					{
						for (int iTeam1 = 0; iTeam1 < MAX_CIV_TEAMS; ++iTeam1)
						{
							CvTeam& kTeam1 = GET_TEAM((TeamTypes)iTeam1);

							if (kTeam1.isAlive())
							{
								kData.ePlayer = kTeam1.getLeaderID();

								if (isValidVoteSelection(eVoteSource, kData))
								{
									kData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_FORCE_WAR", kTeam1.getName().GetCString(), getVoteRequired(kData.eVote, eVoteSource), countPossibleVote(kData.eVote, eVoteSource));
									pData->aVoteOptions.push_back(kData);
								}
							}
						}
					}
					else if (GC.getVoteInfo(kData.eVote).isAssignCity())
					{
						for (int iPlayer1 = 0; iPlayer1 < MAX_CIV_PLAYERS; ++iPlayer1)
						{
							CvPlayer& kPlayer1 = GET_PLAYER((PlayerTypes)iPlayer1);

							int iLoop;
							for (CvCity* pLoopCity = kPlayer1.firstCity(&iLoop); NULL != pLoopCity; pLoopCity = kPlayer1.nextCity(&iLoop))
							{
								PlayerTypes eNewOwner = pLoopCity->plot()->findHighestCulturePlayer();
								if (NO_PLAYER != eNewOwner)
								{
									kData.ePlayer = (PlayerTypes)iPlayer1;
									kData.iCityId =	pLoopCity->getID();
									kData.eOtherPlayer = eNewOwner;

									if (isValidVoteSelection(eVoteSource, kData))
									{
										kData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_ASSIGN_CITY", kPlayer1.getCivilizationAdjectiveKey(), pLoopCity->getNameKey(), GET_PLAYER(eNewOwner).getNameKey(), getVoteRequired(kData.eVote, eVoteSource), countPossibleVote(kData.eVote, eVoteSource));
										pData->aVoteOptions.push_back(kData);
									}
								}
							}
						}
					}
					else
					{
						kData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_OPTION", GC.getVoteInfo(kData.eVote).getTextKeyWide(), getVoteRequired(kData.eVote, eVoteSource), countPossibleVote(kData.eVote, eVoteSource));
						if (isVotePassed(kData.eVote))
						{
							kData.szText += gDLL->getText("TXT_KEY_POPUP_PASSED");
						}

						if (canDoResolution(eVoteSource, kData))
						{
							pData->aVoteOptions.push_back(kData);
						}
					}
				}
			}
		}

		if (0 == pData->aVoteOptions.size())
		{
			deleteVoteSelection(pData->getID());
			pData = NULL;
		}
	}

	return pData;
}

void CvGame::deleteVoteSelection(int iID)
{
	m_voteSelections.removeAt(iID);
}

VoteTriggeredData* CvGame::getVoteTriggered(int iID) const
{
	return ((VoteTriggeredData*)(m_votesTriggered.getAt(iID)));
}

VoteTriggeredData* CvGame::addVoteTriggered(const VoteSelectionData& kData, int iChoice)
{
	if (-1 == iChoice || iChoice >= (int)kData.aVoteOptions.size())
	{
		return NULL;
	}

	return addVoteTriggered(kData.eVoteSource, kData.aVoteOptions[iChoice]);
}

VoteTriggeredData* CvGame::addVoteTriggered(VoteSourceTypes eVoteSource, const VoteSelectionSubData& kOptionData)
{
	VoteTriggeredData* pData = ((VoteTriggeredData*)(m_votesTriggered.add()));

	if (NULL != pData)
	{
		pData->eVoteSource = eVoteSource;
		pData->kVoteOption = kOptionData;

		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iI);
			if (kPlayer.isVotingMember(eVoteSource))
			{
				if (kPlayer.isHuman())
				{
					CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_DIPLOVOTE);
					if (NULL != pInfo)
					{
						pInfo->setData1(pData->getID());
						gDLL->getInterfaceIFace()->addPopup(pInfo, (PlayerTypes)iI);
					}
				}
				else
				{
					castVote(((PlayerTypes)iI), pData->getID(), GET_PLAYER((PlayerTypes)iI).AI_diploVote(kOptionData, eVoteSource, false));
				}
			}
		}
	}

	return pData;
}

void CvGame::deleteVoteTriggered(int iID)
{
	m_votesTriggered.removeAt(iID);
}

void CvGame::doVoteResults()
{
	int iLoop;
	for (VoteTriggeredData* pVoteTriggered = m_votesTriggered.beginIter(&iLoop); NULL != pVoteTriggered; pVoteTriggered = m_votesTriggered.nextIter(&iLoop))
	{
		CvWString szBuffer;
		CvWString szMessage;
		VoteTypes eVote = pVoteTriggered->kVoteOption.eVote;
		VoteSourceTypes eVoteSource = pVoteTriggered->eVoteSource;
		bool bPassed = false;

		if (!canDoResolution(eVoteSource, pVoteTriggered->kVoteOption))
		{
			for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
			{
				CvPlayer& kPlayer = GET_PLAYER((PlayerTypes) iPlayer);
				if (kPlayer.isVotingMember(eVoteSource))
				{
					CvWString szMessage;
					szMessage.Format(L"%s: %s", gDLL->getText("TXT_KEY_ELECTION_CANCELLED").GetCString(), GC.getVoteInfo(eVote).getDescription());
					gDLL->getInterfaceIFace()->addMessage((PlayerTypes)iPlayer, false, GC.getEVENT_MESSAGE_TIME(), szMessage, "AS2D_NEW_ERA", MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
				}
			}
		}
		else
		{
			bool bAllVoted = true;
			for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
			{
				PlayerTypes ePlayer = (PlayerTypes) iJ;
				if (GET_PLAYER(ePlayer).isVotingMember(eVoteSource))
				{
					if (getPlayerVote(ePlayer, pVoteTriggered->getID()) == NO_PLAYER_VOTE)
					{
						//give player one more turn to submit vote
						setPlayerVote(ePlayer, pVoteTriggered->getID(), NO_PLAYER_VOTE_CHECKED);
						bAllVoted = false;
						break;
					}
					else if (getPlayerVote(ePlayer, pVoteTriggered->getID()) == NO_PLAYER_VOTE_CHECKED)
					{
						//default player vote to abstain
						setPlayerVote(ePlayer, pVoteTriggered->getID(), PLAYER_VOTE_ABSTAIN);
					}
				}
			}

			if (!bAllVoted)
			{
				continue;
			}

			if (isTeamVote(eVote))
			{
				TeamTypes eTeam = findHighestVoteTeam(*pVoteTriggered);

				if (NO_TEAM != eTeam)
				{
					bPassed = countVote(*pVoteTriggered, (PlayerVoteTypes)eTeam) >= getVoteRequired(eVote, eVoteSource);
				}

				szBuffer = GC.getVoteInfo(eVote).getDescription();

				if (eTeam != NO_TEAM)
				{
					szBuffer += NEWLINE + gDLL->getText("TXT_KEY_POPUP_DIPLOMATIC_VOTING_VICTORY", GET_TEAM(eTeam).getName().GetCString(), countVote(*pVoteTriggered, (PlayerVoteTypes)eTeam), getVoteRequired(eVote, eVoteSource), countPossibleVote(eVote, eVoteSource));
				}

				for (int iI = MAX_CIV_TEAMS; iI >= 0; --iI)
				{
					for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
					{
						if (GET_PLAYER((PlayerTypes)iJ).isVotingMember(eVoteSource))
						{
							if (getPlayerVote(((PlayerTypes)iJ), pVoteTriggered->getID()) == (PlayerVoteTypes)iI)
							{
								szBuffer += NEWLINE + gDLL->getText("TXT_KEY_POPUP_VOTES_FOR", GET_PLAYER((PlayerTypes)iJ).getNameKey(), GET_TEAM((TeamTypes)iI).getName().GetCString(), GET_PLAYER((PlayerTypes)iJ).getVotes(eVote, eVoteSource));
							}
						}
					}
				}

				for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
				{
					if (GET_PLAYER((PlayerTypes)iJ).isVotingMember(eVoteSource))
					{
						if (getPlayerVote(((PlayerTypes)iJ), pVoteTriggered->getID()) == PLAYER_VOTE_ABSTAIN)
						{
							szBuffer += NEWLINE + gDLL->getText("TXT_KEY_POPUP_ABSTAINS", GET_PLAYER((PlayerTypes)iJ).getNameKey(), GET_PLAYER((PlayerTypes)iJ).getVotes(eVote, eVoteSource));
						}
					}
				}

				if (NO_TEAM != eTeam && bPassed)
				{
					setVoteOutcome(*pVoteTriggered, (PlayerVoteTypes)eTeam);
				}
				else
				{
					setVoteOutcome(*pVoteTriggered, PLAYER_VOTE_ABSTAIN);
				}
			}
			else
			{
				bPassed = countVote(*pVoteTriggered, PLAYER_VOTE_YES) >= getVoteRequired(eVote, eVoteSource);

				// Defying resolution
				if (bPassed)
				{
					for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
					{
						if (getPlayerVote((PlayerTypes)iJ, pVoteTriggered->getID()) == PLAYER_VOTE_NEVER)
						{
							bPassed = false;

							GET_PLAYER((PlayerTypes)iJ).setDefiedResolution(eVoteSource, pVoteTriggered->kVoteOption);
						}
					}
				}

				if (bPassed)
				{
					for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
					{
						if (GET_PLAYER((PlayerTypes)iJ).isVotingMember(eVoteSource))
						{
							if (getPlayerVote(((PlayerTypes)iJ), pVoteTriggered->getID()) == PLAYER_VOTE_YES)
							{
								GET_PLAYER((PlayerTypes)iJ).setEndorsedResolution(eVoteSource, pVoteTriggered->kVoteOption);
							}
						}
					}
				}

				szBuffer += NEWLINE + gDLL->getText((bPassed ? "TXT_KEY_POPUP_DIPLOMATIC_VOTING_SUCCEEDS" : "TXT_KEY_POPUP_DIPLOMATIC_VOTING_FAILURE"), GC.getVoteInfo(eVote).getTextKeyWide(), countVote(*pVoteTriggered, PLAYER_VOTE_YES), getVoteRequired(eVote, eVoteSource), countPossibleVote(eVote, eVoteSource));

				for (int iI = PLAYER_VOTE_NEVER; iI <= PLAYER_VOTE_YES; ++iI)
				{
					for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
					{
						if (GET_PLAYER((PlayerTypes)iJ).isVotingMember(eVoteSource))
						{
							if (getPlayerVote(((PlayerTypes)iJ), pVoteTriggered->getID()) == (PlayerVoteTypes)iI)
							{
								switch ((PlayerVoteTypes)iI)
								{
								case PLAYER_VOTE_ABSTAIN:
									szBuffer += NEWLINE + gDLL->getText("TXT_KEY_POPUP_ABSTAINS", GET_PLAYER((PlayerTypes)iJ).getNameKey(), GET_PLAYER((PlayerTypes)iJ).getVotes(eVote, eVoteSource));
									break;
								case PLAYER_VOTE_NEVER:
									szBuffer += NEWLINE + gDLL->getText("TXT_KEY_POPUP_VOTES_YES_NO", GET_PLAYER((PlayerTypes)iJ).getNameKey(), L"TXT_KEY_POPUP_VOTE_NEVER", GET_PLAYER((PlayerTypes)iJ).getVotes(eVote, eVoteSource));
									break;
								case PLAYER_VOTE_NO:
									szBuffer += NEWLINE + gDLL->getText("TXT_KEY_POPUP_VOTES_YES_NO", GET_PLAYER((PlayerTypes)iJ).getNameKey(), L"TXT_KEY_POPUP_NO", GET_PLAYER((PlayerTypes)iJ).getVotes(eVote, eVoteSource));
									break;
								case PLAYER_VOTE_YES:
									szBuffer += NEWLINE + gDLL->getText("TXT_KEY_POPUP_VOTES_YES_NO", GET_PLAYER((PlayerTypes)iJ).getNameKey(), L"TXT_KEY_POPUP_YES", GET_PLAYER((PlayerTypes)iJ).getVotes(eVote, eVoteSource));
									break;
								default:
									FAssert(false);
									break;
								}
							}
						}
					}
				}

				setVoteOutcome(*pVoteTriggered, bPassed ? PLAYER_VOTE_YES : PLAYER_VOTE_NO);
			}

			for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
			{
				CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iI);
				if (kPlayer.isHuman())
				{
					bool bShow = kPlayer.isVotingMember(pVoteTriggered->eVoteSource);

					if (bShow)
					{
						CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_TEXT);
						if (NULL != pInfo)
						{
							pInfo->setText(szBuffer);
							gDLL->getInterfaceIFace()->addPopup(pInfo, (PlayerTypes)iI);
						}
					}

					if (!bShow)
					{
						if (iI == pVoteTriggered->kVoteOption.ePlayer && GET_PLAYER(pVoteTriggered->kVoteOption.ePlayer).isVotingMember(pVoteTriggered->eVoteSource))
						{
							bShow = true;
						}
					}

					if (!bShow)
					{
						if (iI == pVoteTriggered->kVoteOption.eOtherPlayer && GET_PLAYER(pVoteTriggered->kVoteOption.eOtherPlayer).isVotingMember(pVoteTriggered->eVoteSource))
						{
							bShow = true;
						}
					}

					if (bShow && bPassed)
					{
						CvWString szMessage = gDLL->getText("TXT_KEY_VOTE_RESULTS", GC.getVoteSourceInfo(eVoteSource).getTextKeyWide(), pVoteTriggered->kVoteOption.szText.GetCString());
						gDLL->getInterfaceIFace()->addMessage(((PlayerTypes)iI), false, GC.getEVENT_MESSAGE_TIME(), szMessage, "AS2D_NEW_ERA", MESSAGE_TYPE_MINOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
					}

				}
			}
		}

		if (!bPassed && GC.getVoteInfo(eVote).isSecretaryGeneral())
		{
			setSecretaryGeneralTimer(eVoteSource, 0);
		}

		deleteVoteTriggered(pVoteTriggered->getID());
	}
}

void CvGame::doVoteSelection()
{
	for (int iI = 0; iI < GC.getNumVoteSourceInfos(); ++iI)
	{
		VoteSourceTypes eVoteSource = (VoteSourceTypes)iI;

		if (isDiploVote(eVoteSource))
		{
			if (getVoteTimer(eVoteSource) > 0)
			{
				changeVoteTimer(eVoteSource, -1);
			}
			else
			{
				setVoteTimer(eVoteSource, (GC.getVoteSourceInfo(eVoteSource).getVoteInterval() * GC.getGameSpeedInfo(getGameSpeedType()).getVictoryDelayPercent()) / 100);

				for (int iTeam1 = 0; iTeam1 < MAX_CIV_TEAMS; ++iTeam1)
				{
					CvTeam& kTeam1 = GET_TEAM((TeamTypes)iTeam1);

					if (kTeam1.isAlive() && kTeam1.isVotingMember(eVoteSource))
					{
						for (int iTeam2 = iTeam1 + 1; iTeam2 < MAX_CIV_TEAMS; ++iTeam2)
						{
							CvTeam& kTeam2 = GET_TEAM((TeamTypes)iTeam2);

							if (kTeam2.isAlive() && kTeam2.isVotingMember(eVoteSource))
							{
								kTeam1.meet((TeamTypes)iTeam2, true);
							}
						}
					}
				}

				TeamTypes eSecretaryGeneral = getSecretaryGeneral(eVoteSource);
				PlayerTypes eSecretaryPlayer;

				if (eSecretaryGeneral != NO_TEAM)
				{
					eSecretaryPlayer = GET_TEAM(eSecretaryGeneral).getSecretaryID();
				}
				else
				{
					eSecretaryPlayer = NO_PLAYER;
				}

				bool bSecretaryGeneralVote = false;
				if (canHaveSecretaryGeneral(eVoteSource))
				{
					if  (getSecretaryGeneralTimer(eVoteSource) > 0)
					{
						changeSecretaryGeneralTimer(eVoteSource, -1);
					}
					else
					{
						setSecretaryGeneralTimer(eVoteSource, GC.getDefineINT("DIPLO_VOTE_SECRETARY_GENERAL_INTERVAL"));

						for (int iJ = 0; iJ < GC.getNumVoteInfos(); iJ++)
						{
							if (GC.getVoteInfo((VoteTypes)iJ).isSecretaryGeneral() && GC.getVoteInfo((VoteTypes)iJ).isVoteSourceType(iI))
							{
								VoteSelectionSubData kOptionData;
								kOptionData.iCityId = -1;
								kOptionData.ePlayer = NO_PLAYER;
								kOptionData.eVote = (VoteTypes)iJ;
								kOptionData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_OPTION", GC.getVoteInfo((VoteTypes)iJ).getTextKeyWide(), getVoteRequired((VoteTypes)iJ, eVoteSource), countPossibleVote((VoteTypes)iJ, eVoteSource));
								addVoteTriggered(eVoteSource, kOptionData);
								bSecretaryGeneralVote = true;
								break;
							}
						}
					}
				}

				if (!bSecretaryGeneralVote && eSecretaryGeneral != NO_TEAM && eSecretaryPlayer != NO_PLAYER)
				{
					VoteSelectionData* pData = addVoteSelection(eVoteSource);
					if (NULL != pData)
					{
						if (GET_PLAYER(eSecretaryPlayer).isHuman())
						{
							CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_CHOOSEELECTION);
							if (NULL != pInfo)
							{
								pInfo->setData1(pData->getID());
								gDLL->getInterfaceIFace()->addPopup(pInfo, eSecretaryPlayer);
							}
						}
						else
						{
							setVoteChosen(GET_TEAM(eSecretaryGeneral).AI_chooseElection(*pData), pData->getID());
						}
					}
					else
					{
						setVoteTimer(eVoteSource, 0);
					}
				}
			}
		}
	}
}

bool CvGame::isEventActive(EventTriggerTypes eTrigger) const
{
	for (std::vector<EventTriggerTypes>::const_iterator it = m_aeInactiveTriggers.begin(); it != m_aeInactiveTriggers.end(); ++it)
	{
		if (*it == eTrigger)
		{
			return false;
		}
	}

	return true;
}

void CvGame::initEvents()
{
	for (int iTrigger = 0; iTrigger < GC.getNumEventTriggerInfos(); ++iTrigger)
	{
		if (isOption(GAMEOPTION_NO_EVENTS) || getSorenRandNum(100, "Event Active?") >= GC.getEventTriggerInfo((EventTriggerTypes)iTrigger).getPercentGamesActive())
		{
			m_aeInactiveTriggers.push_back((EventTriggerTypes)iTrigger);
		}
	}
}

bool CvGame::isCivEverActive(CivilizationTypes eCivilization) const
{
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kLoopPlayer.isEverAlive())
		{
			if (kLoopPlayer.getCivilizationType() == eCivilization)
			{
				return true;
			}
		}
	}

	return false;
}

bool CvGame::isLeaderEverActive(LeaderHeadTypes eLeader) const
{
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kLoopPlayer.isEverAlive())
		{
			if (kLoopPlayer.getLeaderType() == eLeader)
			{
				return true;
			}
		}
	}

	return false;
}

bool CvGame::isUnitEverActive(UnitTypes eUnit) const
{
	for (int iCiv = 0; iCiv < GC.getNumCivilizationInfos(); ++iCiv)
	{
		if (isCivEverActive((CivilizationTypes)iCiv))
		{
			if (eUnit == GC.getCivilizationInfo((CivilizationTypes)iCiv).getCivilizationUnits(GC.getUnitInfo(eUnit).getUnitClassType()))
			{
				return true;
			}
		}
	}

	return false;
}

bool CvGame::isBuildingEverActive(BuildingTypes eBuilding) const
{
	for (int iCiv = 0; iCiv < GC.getNumCivilizationInfos(); ++iCiv)
	{
		if (isCivEverActive((CivilizationTypes)iCiv))
		{
			if (eBuilding == GC.getCivilizationInfo((CivilizationTypes)iCiv).getCivilizationBuildings(GC.getBuildingInfo(eBuilding).getBuildingClassType()))
			{
				return true;
			}
		}
	}

	return false;
}

void CvGame::processBuilding(BuildingTypes eBuilding, int iChange)
{
	for (int iI = 0; iI < GC.getNumVoteSourceInfos(); ++iI)
	{
		if (GC.getBuildingInfo(eBuilding).getVoteSourceType() == (VoteSourceTypes)iI)
		{
			changeDiploVote((VoteSourceTypes)iI, iChange);
		}
	}
}

bool CvGame::pythonIsBonusIgnoreLatitudes() const
{
	long lResult = -1;
	if (gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "isBonusIgnoreLatitude", NULL, &lResult))
	{
		if (!gDLL->getPythonIFace()->pythonUsingDefaultImpl() && lResult != -1)
		{
			return (lResult != 0);
		}
	}

	return false;
}

//FfH: Added by Kael 08/07/2007
void CvGame::decrementUnitCreatedCount(UnitTypes eIndex)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumUnitInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiUnitCreatedCount[eIndex]--;
/*************************************************************************************************/
/**	Xienwolf Tweak							02/01/09											**/
/**																								**/
/**							"Safety Net" for Python incase it goes too far						**/
/*************************************************************************************************/
	if (m_paiUnitCreatedCount[eIndex] < 0) m_paiUnitCreatedCount[eIndex] = 0;
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
}

void CvGame::decrementUnitClassCreatedCount(UnitClassTypes eIndex)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumUnitClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiUnitClassCreatedCount[eIndex]--;
/*************************************************************************************************/
/**	Xienwolf Tweak							02/01/09											**/
/**																								**/
/**							"Safety Net" for Python incase it goes too far						**/
/*************************************************************************************************/
	if (m_paiUnitClassCreatedCount[eIndex] < 0) m_paiUnitClassCreatedCount[eIndex] = 0;
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
}

int CvGame::getCrime() const
{
	return m_iCrime;
}

void CvGame::changeCrime(int iChange)
{
	m_iCrime += iChange;
}

int CvGame::getCutLosersCounter() const
{
	return m_iCutLosersCounter;
}

void CvGame::changeCutLosersCounter(int iChange)
{
	m_iCutLosersCounter += iChange;
}

int CvGame::getFlexibleDifficultyCounter() const
{
	return m_iFlexibleDifficultyCounter;
}

void CvGame::changeFlexibleDifficultyCounter(int iChange)
{
	m_iFlexibleDifficultyCounter += iChange;
}

int CvGame::getHighToLowCounter() const
{
	return m_iHighToLowCounter;
}

void CvGame::changeHighToLowCounter(int iChange)
{
	m_iHighToLowCounter += iChange;
}

int CvGame::getIncreasingDifficultyCounter() const
{
	return m_iIncreasingDifficultyCounter;
}

void CvGame::changeIncreasingDifficultyCounter(int iChange)
{
	m_iIncreasingDifficultyCounter += iChange;
}

int CvGame::getGlobalCounter() const
{
	return (m_iGlobalCounter * 100 / m_iGlobalCounterLimit);
}

int CvGame::getTrueGlobalCounter() const
{
	return m_iGlobalCounter;
}

int CvGame::getMaxGlobalCounter() const
{
	return (m_iMaxGlobalCounter * 100 / m_iGlobalCounterLimit);
}

int CvGame::getTrueMaxGlobalCounter() const
{
	return m_iMaxGlobalCounter;
}

void CvGame::changeGlobalCounter(int iChange)
{
	if (iChange != 0)
	{
		if (!isOption(GAMEOPTION_NO_GLOBAL_COUNTER))
		{
			if (isOption(GAMEOPTION_DOUBLE_GLOBAL_COUNTER))
			{
				iChange *= 2;
			}
			int iNewValue = m_iGlobalCounter + iChange;
			if (iNewValue < 0)
			{
				iNewValue = 0;
			}
			if (iNewValue > m_iGlobalCounterLimit)
			{
				iNewValue = m_iGlobalCounterLimit;
			}
			m_iGlobalCounter = iNewValue;
			if (m_iMaxGlobalCounter < iNewValue)
			{
				m_iMaxGlobalCounter = iNewValue;
			}
			if (iChange < 0)
			{
				gDLL->getInterfaceIFace()->playGeneralSound("AS2D_ARMAGEDDON_DECREASE");
			}
			if (iChange > 0)
			{
				gDLL->getInterfaceIFace()->playGeneralSound("AS2D_ARMAGEDDON_INCREASE");
			}
		}
	}
}

int CvGame::getGlobalCounterLimit() const
{
	return m_iGlobalCounterLimit;
}

void CvGame::changeGlobalCounterLimit(int iChange)
{
	m_iGlobalCounterLimit += iChange;
}

int CvGame::getScenarioCounter() const
{
	return m_iScenarioCounter;
}

void CvGame::changeScenarioCounter(int iChange)
{
	m_iScenarioCounter += iChange;
}

void CvGame::foundBarbarianCity()
{
	bool bValid = true;
	int iBestValue = 0;
	int iDist = 0;
	int iValue = 0;
	CvPlot* pPlotI = NULL;
	CvPlot* pBestPlot = NULL;
	CvPlot* pLoopPlot = NULL;
	for (int iPlot = 0; iPlot < GC.getMapINLINE().numPlotsINLINE(); iPlot++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iPlot);
		bValid = true;
		iValue = 0;
		if (pLoopPlot->isWater())
		{
			bValid = false;
		}
		else if (pLoopPlot->isImpassable())
		{
			bValid = false;
		}
/*************************************************************************************************/
/**	Mountain Mod 		 		expanded by Ahwaric	22.09.09		**/
/*************************************************************************************************/
		else if (pLoopPlot->isPeak())
		{
			bValid = false;
		}
/*************************************************************************************************/
/**	Mountain Mod END									**/
/*************************************************************************************************/
		else if (pLoopPlot->isCity())
		{
			bValid = false;
		}
		else if (pLoopPlot->getImprovementType() != NO_IMPROVEMENT)
		{
			bValid = false;
		}
		else if (pLoopPlot->isFoundDisabled())
		{
			bValid = false;
		}
		else if (GC.getDefineINT("BONUS_MANA") != -1)
		{
			if (pLoopPlot->getBonusType() == GC.getDefineINT("BONUS_MANA"))
			{
				bValid = false;
			}
		}
		if (bValid)
		{
			for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
			{
				if (GET_PLAYER((PlayerTypes)iI).isAlive())
				{
					pPlotI = GET_PLAYER((PlayerTypes)iI).getStartingPlot();
					if (pPlotI != NULL)
					{
						iDist = GC.getMapINLINE().calculatePathDistance(pPlotI, pLoopPlot);
						if (iDist == -1)
						{
							iValue += 100;
						}
						else if (iDist < 5)
						{
							bValid = false;
						}
						else
						{
							iValue += iDist * 10;
						}
					}
				}
			}
			if (bValid)
			{
/*************************************************************************************************/
/**	MultiBarb								12/23/08								Xienwolf	**/
/**																								**/
/**							Adds extra Barbarian Civilizations									**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
				iValue += GET_PLAYER(BARBARIAN_PLAYER).AI_foundValue(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), GC.getDefineINT("MIN_BARBARIAN_CITY_STARTING_DISTANCE"));
/**								----  End Original Code  ----									**/
				iValue += GET_PLAYER(ORC_PLAYER).AI_foundValue(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), GC.getDefineINT("MIN_BARBARIAN_CITY_STARTING_DISTANCE"));
/*************************************************************************************************/
/**	MultiBarb								END													**/
/*************************************************************************************************/
				iValue += pLoopPlot->area()->getNumOwnedTiles() + 10;
				iValue += getSorenRandNum(250, "Barb City Found");
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
/*************************************************************************************************/
/**	MultiBarb								12/23/08								Xienwolf	**/
/**																								**/
/**							Adds extra Barbarian Civilizations									**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		GET_PLAYER(BARBARIAN_PLAYER).found(pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
/**								----  End Original Code  ----									**/
		GET_PLAYER(ORC_PLAYER).found(pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
/*************************************************************************************************/
/**	MultiBarb								END													**/
/*************************************************************************************************/
	}
}

bool CvGame::isEventTriggered(EventTriggerTypes eTrigger) const
{
	return m_pabEventTriggered[eTrigger];
}

void CvGame::setEventTriggered(EventTriggerTypes eTrigger, bool bNewValue)
{
	m_pabEventTriggered[eTrigger] = bNewValue;
}

bool CvGame::isGamblingRing(VoteSourceTypes eIndex) const
{
	return m_pabGamblingRing[eIndex];
}

void CvGame::setGamblingRing(VoteSourceTypes eIndex, bool bNewValue)
{
	m_pabGamblingRing[eIndex] = bNewValue;
}

bool CvGame::isNoBonus(BonusTypes eIndex) const
{
	return m_pabNoBonus[eIndex];
}

void CvGame::setNoBonus(BonusTypes eIndex, bool bNewValue)
{
	m_pabNoBonus[eIndex] = bNewValue;
}

bool CvGame::isNoOutsideTechTrades(VoteSourceTypes eIndex) const
{
	return m_pabNoOutsideTechTrades[eIndex];
}

void CvGame::setNoOutsideTechTrades(VoteSourceTypes eIndex, bool bNewValue)
{
	m_pabNoOutsideTechTrades[eIndex] = bNewValue;
}

bool CvGame::isSlaveTrade(VoteSourceTypes eIndex) const
{
	return m_pabSlaveTrade[eIndex];
}

void CvGame::setSlaveTrade(VoteSourceTypes eIndex, bool bNewValue)
{
	m_pabSlaveTrade[eIndex] = bNewValue;
}

bool CvGame::isSmugglingRing(VoteSourceTypes eIndex) const
{
	return m_pabSmugglingRing[eIndex];
}

void CvGame::setSmugglingRing(VoteSourceTypes eIndex, bool bNewValue)
{
	m_pabSmugglingRing[eIndex] = bNewValue;
}

int CvGame::getTrophyValue(const TCHAR* szName) const
{
	if (isGameMultiPlayer())
	{
		return 0;
	}
	int iValue;
	CyArgsList argsList;
	argsList.add(szName);
	long lResult = 0;
	gDLL->getPythonIFace()->callFunction(PYDataStorageModule, "getTrophyValue", argsList.makeFunctionArgs(), &lResult);
	iValue = range(lResult, 0, MAX_INT);
	return iValue;
}

bool CvGame::isHasTrophy(const TCHAR* szName) const
{
	return (getTrophyValue(szName) > 0);
}

void CvGame::setTrophyValue(const TCHAR* szName, int iNewValue)
{
	if (!isGameMultiPlayer())
	{
		int iValue = range(iNewValue, 0, MAX_INT);
		CyArgsList argsList;
		argsList.add(szName);
		argsList.add(iValue);
		gDLL->getPythonIFace()->callFunction(PYDataStorageModule, "setTrophyValue", argsList.makeFunctionArgs());
	}
}

void CvGame::changeTrophyValue(const TCHAR* szName, int iChange)
{
	setTrophyValue(szName, (getTrophyValue(szName) + iChange));
}

bool CvGame::isReligionDisabled(int iReligion) const
{
	if (iReligion == 0)
	{
		if (isOption(GAMEOPTION_NO_RELIGION_0))
		{
			return true;
		}
	}
	if (iReligion == 1)
	{
		if (isOption(GAMEOPTION_NO_RELIGION_1))
		{
			return true;
		}
	}
	if (iReligion == 2)
	{
		if (isOption(GAMEOPTION_NO_RELIGION_2))
		{
			return true;
		}
	}
	if (iReligion == 3)
	{
		if (isOption(GAMEOPTION_NO_RELIGION_3))
		{
			return true;
		}
	}
	if (iReligion == 4)
	{
		if (isOption(GAMEOPTION_NO_RELIGION_4))
		{
			return true;
		}
	}
	if (iReligion == 5)
	{
		if (isOption(GAMEOPTION_NO_RELIGION_5))
		{
			return true;
		}
	}
	if (iReligion == 6)
	{
		if (isOption(GAMEOPTION_NO_RELIGION_6))
		{
			return true;
		}
	}
	if (iReligion == 7)
	{
		if (isOption(GAMEOPTION_NO_RELIGION_7))
		{
			return true;
		}
	}
	return false;
}
//FfH: End Add
/*************************************************************************************************/
/**	New Tag Defs	(ProjectInfos)			10/01/08								Xienwolf	**/
/**	New Tag Defs	(GameInfos)				10/01/08								Xienwolf	**/
/**									Called for Logic Checks										**/
/*************************************************************************************************/
int CvGame::getProjectTimer(ProjectTypes eProject, int iCount)
{
	return m_ppaaiProjectTimers[eProject][iCount];
}

void CvGame::clearProjectTimer(ProjectTypes eProject)
{
	for (int i=0;i<(GC.getMaxNumProjectsAllowed() - 1);i++)
	{
		m_ppaaiProjectTimers[eProject][i] = m_ppaaiProjectTimers[eProject][i+1];
	}
	m_ppaaiProjectTimers[eProject][GC.getMaxNumProjectsAllowed()] = -1;
	if (getProjectCreatedCount(eProject) != 0)
	{
		incrementProjectCreatedCount(eProject, -1);
	}
}

void CvGame::setProjectTimer(ProjectTypes eProject)
{
	for (int i=0;i<GC.getMaxNumProjectsAllowed();i++)
	{
		if (m_ppaaiProjectTimers[eProject][i] == -1)
		{
			m_ppaaiProjectTimers[eProject][i] = getGameTurn();
			break;
		}
	}
}

void CvGame::createLairs()
{
	PROFILE("CvGame::createLairs");
	int iNetWeight, iWeight, iTargetWeight, iI, iJ, iK, iCiv, iFlags, iLairFlags, iGoal;
	bool bEvilValid, bNormalValid, bNoSpawns;
	PlayerTypes ePlayer;
	CvPlot* pPlot = NULL;
	CvArea* pArea = NULL;
	ImprovementTypes eLair = NO_IMPROVEMENT;

	if (isOption(GAMEOPTION_NO_LAIRS))
		return;

	if (getGameTurn() < getNextLairCycle())
		return;

	int iLairCycleLength = GC.getGameSpeedInfo(getGameSpeedType()).getTurnsPerLairCycle();
	if (getGameTurn() == 0)
	{
		setNextLairCycle(iLairCycleLength);
		return;
	}
	else
	{
		// Some randomization in followup lair cycle length
		setNextLairCycle(getGameTurn() + 11 * iLairCycleLength / 10 - getSorenRandNum(iLairCycleLength/5, "~ +-10% Randomization in cycle length"));
	}

	iGoal = GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getDefaultPlayers();
	if (iGoal <= 0) return;

	// Calculate the net of all possible positive weights on all lairs
	iNetWeight = 0;
	for (iJ = 0; iJ < GC.getNumImprovementInfos(); iJ++)
	{
		iCiv = GC.getImprovementInfo((ImprovementTypes)iJ).getSpawnUnitCiv();
		if (!(iCiv == GC.getDefineINT("DEMON_CIVILIZATION") && isOption(GAMEOPTION_NO_DEMONS))
		 && !(iCiv == GC.getDefineINT("ANIMAL_CIVILIZATION") && isOption(GAMEOPTION_NO_ANIMALS))
		 && !(iCiv == GC.getDefineINT("ORC_CIVILIZATION") && isOption(GAMEOPTION_NO_BARBARIANS)))
		{
			// LairCreationWeights adjusted(?) : Hinterlands Valkrionn 07/11/09
			iWeight = GC.getImprovementInfo((ImprovementTypes)iJ).getLairCreationWeight();
			for (iK = 0; iK < GC.getNumTechInfos(); iK++)
			{
				iWeight += (GC.getImprovementInfo((ImprovementTypes)iJ).getLairCreationWeightTechs(iK)) * countKnownTechNumTeams((TechTypes)iK);
			}
			// Total weight for this lair might be negative; don't include if so
			if (iWeight > 0)
			{
				iNetWeight += iWeight;
			}
		}
	}

	// Setting up the static flags
	iFlags = RANDPLOT_PASSIBLE | RANDPLOT_NOT_CITY | RANDPLOT_NOT_IMPROVED | RANDPLOT_UNOCCUPIED | RANDPLOT_ADJACENT_UNOWNED;
	if (isOption(GAMEOPTION_NO_VISIBLE_BARBARIANS))
	{
		iFlags |= RANDPLOT_NOT_VISIBLE_TO_CIV;
	}

	if (iCiv == GC.getDefineINT("DEMON_CIVILIZATION"))
	{
		iFlags |= RANDPLOT_DEMON_ALLY;
	}
	else if (iCiv == GC.getDefineINT("ORC_CIVILIZATION"))
	{
		iFlags |= RANDPLOT_ORC_ALLY;
	}
	else if (iCiv == GC.getDefineINT("ANIMAL_CIVILIZATION"))
	{
		iFlags |= RANDPLOT_ANIMAL_ALLY;
	}
	else
	{
		iFlags |= RANDPLOT_UNOWNED;
	}

	// 50 attempts to place the lairs...... stops early if places 2 though
	for (iI = 0; iI < 50; iI++)
	{
		// Pick a random value from the static total sum. If we recalculate weights until this value, then our stopping point will be random & weight-modulated.
		iTargetWeight = getMapRandNum(iNetWeight, "Select Lair Spawn");
		for (iJ = 0; iJ < GC.getNumImprovementInfos(); iJ++)
		{
			iCiv = GC.getImprovementInfo((ImprovementTypes)iJ).getSpawnUnitCiv();
			if (!(iCiv == GC.getDefineINT("DEMON_CIVILIZATION") && isOption(GAMEOPTION_NO_DEMONS))
			 && !(iCiv == GC.getDefineINT("ANIMAL_CIVILIZATION") && isOption(GAMEOPTION_NO_ANIMALS))
			 && !(iCiv == GC.getDefineINT("ORC_CIVILIZATION") && isOption(GAMEOPTION_NO_BARBARIANS)))
			{
				// LairCreationWeights adjusted(?) : Hinterlands Valkrionn 07/11/09
				iWeight = GC.getImprovementInfo((ImprovementTypes)iJ).getLairCreationWeight();
				for (iK = 0; iK < GC.getNumTechInfos(); iK++)
				{
					iWeight += (GC.getImprovementInfo((ImprovementTypes)iJ).getLairCreationWeightTechs(iK)) * countKnownTechNumTeams((TechTypes)iK);
				}
				// Total weight for this lair might be negative; don't include if so
				if (iWeight > 0)
				{
					iTargetWeight -= iWeight;
				}
				if (iTargetWeight <= 0)
				{
					eLair = (ImprovementTypes)iJ;
					break;
				}
			}
		}

		iLairFlags = iFlags;
		if (GC.getImprovementInfo(eLair).isWater())
		{
			iLairFlags |= RANDPLOT_WATER;
		}
		else
		{
			iLairFlags |= RANDPLOT_LAND;
		}

		if (GC.getImprovementInfo(eLair).isPeakMakesValid())
		{
			iLairFlags |= RANDPLOT_PEAK;
		}
		else
		{
			iLairFlags |= RANDPLOT_NOT_PEAK;
		}

		// Checking to see if *only* valid on hell terrain
		bEvilValid = false;
		bNormalValid = false;
		for (iJ = 0; iJ < GC.getNumTerrainInfos(); iJ++)
		{
			if (GC.getImprovementInfo(eLair).getTerrainMakesValid(iJ))
			{
				if (GC.getTerrainInfo((TerrainTypes)iJ).isHell())
				{
					bEvilValid = true;
				}
				else
				{
					bNormalValid = true;
				}
			}
		}
		if (bEvilValid && !bNormalValid)
		{
			iLairFlags |= RANDPLOT_EVIL;
		}

		pPlot = GC.getMapINLINE().syncRandPlot(iLairFlags);
		if (pPlot == NULL || !GC.getImprovementInfo(eLair).getTerrainMakesValid(pPlot->getTerrainType()))
			continue;

		// Check spawning criteria vs density requirements. Lairs that don't spawn might fill up tiles...
		bNoSpawns = false;
		if (iCiv == GC.getDefineINT("DEMON_CIVILIZATION")) ePlayer = DEMON_PLAYER;
		else if (iCiv == GC.getDefineINT("ORC_CIVILIZATION")) ePlayer = ORC_PLAYER;
		else if (iCiv == GC.getDefineINT("ANIMAL_CIVILIZATION")) ePlayer = ANIMAL_PLAYER;
		else bNoSpawns = true;

		pArea = pPlot->area();

		if (bNoSpawns)
		{
			pPlot->setImprovementType(eLair);
			iGoal--;
		}
		// Lairs can only spawn if there's space for barbs to spawn; prevents wildly dense barb areas
		// 1 animal lair can always spawn even if there's no space for a normal animal to spawn
		else if (pArea != NULL && ((calcTargetBarbs(pArea, true, ePlayer) + (ePlayer == ANIMAL_PLAYER)) > pArea->getUnitsPerPlayer(ePlayer)))
		{
			pPlot->setImprovementType(eLair);
		}

		if (iGoal <= 0) return;
	}
}

void CvGame::createDemons()
{
	PROFILE("CvGame::createDemons");
	CvArea* pLoopArea;
	CvPlot* pPlot;
	UnitTypes eBestUnit;
	UnitTypes eLoopUnit;
	int iNeededDemons;
	int iValue;
	int iBestValue;
	int iLoop;
	int iI, iJ, iFlags;

	if (isOption(GAMEOPTION_NO_DEMONS))
	{
		return;
	}

	iFlags = RANDPLOT_EVIL | RANDPLOT_PASSIBLE | RANDPLOT_UNOCCUPIED;
	// This makes demons much, much less dangerous if set
	// if (isOption(GAMEOPTION_NO_VISIBLE_BARBARIANS))
	// {
	// 	iFlags |= RANDPLOT_NOT_VISIBLE_TO_CIV;
	// }

	for(pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
	{
		// Demons have no spawn rate limit, instead depending on hell tiles
		iNeededDemons = calcTargetBarbs(pLoopArea, true, DEMON_PLAYER) - (pLoopArea->getUnitsPerPlayer(DEMON_PLAYER));

		for (iI = 0; iI < iNeededDemons; iI++)
		{
			pPlot = GC.getMapINLINE().syncRandPlot(iFlags, pLoopArea->getID());

			if (pPlot == NULL) continue;

			eBestUnit = NO_UNIT;
			iBestValue = 0;

			for (iJ = 0; iJ < GC.getNumUnitClassInfos(); iJ++)
			{
				eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(GET_PLAYER(DEMON_PLAYER).getCivilizationType()).getCivilizationUnits(iJ)));
				if (eLoopUnit == NO_UNIT) continue;

				CvUnitInfo& kUnit = GC.getUnitInfo(eLoopUnit);

				// Hero demon units must be built/spawned in a dedicated manner
				if (GC.getUnitClassInfo((UnitClassTypes)iJ).getMaxGlobalInstances() == 1) continue;

				if (pLoopArea->isWater() && kUnit.getDomainType() != DOMAIN_SEA) continue;
				else if (!pLoopArea->isWater() && !(kUnit.getDomainType() == DOMAIN_LAND || kUnit.getDomainType() == DOMAIN_IMMOBILE)) continue;
				else if (pPlot->isPeak() && !(kUnit.isCanClimbPeaks() || kUnit.isCanMoveAllTerrain())) continue;

				if (!GET_PLAYER(DEMON_PLAYER).canTrain(eLoopUnit, false, false, true)) continue;

				if (NO_BONUS != kUnit.getPrereqAndBonus())
				{
					if (!GET_TEAM(DEMON_TEAM).isHasTech((TechTypes)GC.getBonusInfo((BonusTypes)kUnit.getPrereqAndBonus()).getTechCityTrade())) continue;
				}

				bool bFound = false;
				bool bRequires = false;
				for (int i = 0; i < GC.getNUM_UNIT_PREREQ_OR_BONUSES(); ++i)
				{
					if (kUnit.getPrereqOrBonuses(i) == NO_BONUS) continue;

					TechTypes eTech = (TechTypes)GC.getBonusInfo((BonusTypes)kUnit.getPrereqOrBonuses(i)).getTechCityTrade();
					if (eTech == NO_TECH) continue;
					bRequires = true;

					if (GET_TEAM(DEMON_TEAM).isHasTech(eTech))
					{
						bFound = true;
						break;
					}
				}
				if (bRequires && !bFound) continue;

				iValue = (1 + getSorenRandNum(1500, "Demon Unit Selection")) + GET_PLAYER(DEMON_PLAYER).AI_unitValue(eLoopUnit, UNITAI_ATTACK, pLoopArea);

				if (kUnit.getUnitAIType(UNITAI_ATTACK))
				{
					iValue += 200;
				}

				if (iValue > iBestValue)
				{
					eBestUnit = eLoopUnit;
					iBestValue = iValue;
				}
			}

			if (eBestUnit != NO_UNIT)
			{
				CvUnit* pUnit = GET_PLAYER(DEMON_PLAYER).initUnit(eBestUnit, pPlot->getX_INLINE(), pPlot->getY_INLINE(), UNITAI_ATTACK);
				if (isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
				{
					pUnit->setImmobileTimer(2);
				}
			}
		}
	}
}

void CvGame::createAnimals()
{
	PROFILE("CvGame::createAnimals");
	CvArea* pLoopArea;
	CvPlot* pPlot;
	UnitTypes eBestUnit;
	UnitTypes eLoopUnit;
	int iNeededAnimals, iTargetAnimals, iValue, iNetValue, iTargetValue, iLoop, iI, iJ, iFlags;

	if (isOption(GAMEOPTION_NO_ANIMALS) || isOption(GAMEOPTION_NO_RANDOM_BARBARIANS))
		return;

	if (getNumCivCities() < countCivPlayersAlive())
		return;

	// Random animals spawn 5x sooner than barbs, or once all civs settled
	if ((5 * getElapsedGameTurns() < (GC.getGameSpeedInfo(getGameSpeedType()).getTurnsPerLairCycle()))
	 || (getNumCivCities() < countCivPlayersAlive()))
		return;

	// Animals weighting gets weird if a mountain is picked; best to simply not choose mountains
	iFlags = RANDPLOT_ANIMAL_ALLY | RANDPLOT_PASSIBLE | RANDPLOT_NOT_PEAK | RANDPLOT_UNOCCUPIED;
	if (isOption(GAMEOPTION_NO_VISIBLE_BARBARIANS))
	{
		iFlags |= RANDPLOT_NOT_VISIBLE_TO_CIV;
	}

	for(pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
	{
		// Spawn at most 20% of limit (round up)
		iTargetAnimals = calcTargetBarbs(pLoopArea, false, ANIMAL_PLAYER);
		iNeededAnimals =  std::min(std::max(1, 2 * iTargetAnimals/10), iTargetAnimals - pLoopArea->getUnitsPerPlayer(ANIMAL_PLAYER));

		for (iI = 0; iI < iNeededAnimals; iI++)
		{
			pPlot = GC.getMapINLINE().syncRandPlot(iFlags, pLoopArea->getID());
			if (pPlot == NULL) continue;

			eBestUnit = NO_UNIT;
			iNetValue = 0;

			// Special check ;)
			for (int iL = 0; iL < MAX_CIV_PLAYERS; iL++)
			{
				CvWString wDefault = CvWString::format(L"TiberiusW").GetCString();
				CvWString wDefault2 = CvWString::format(L"Tiberius_W").GetCString();
				if (GET_PLAYER((PlayerTypes)iL).isAlive() &&(GET_PLAYER((PlayerTypes)iL).getNameKey()==wDefault || GET_PLAYER((PlayerTypes)iL).getNameKey() == wDefault2) && ((ModuleIds)GC.getInfoTypeForString("MODULE_NOGGORMOTHA") != NO_MODULE) && !isUnitClassMaxedOut((UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_MURDERHOOF")))
				{
					eBestUnit = (UnitTypes)GC.getInfoTypeForString("UNIT_MURDERHOOF");
					iNetValue = 1000;
				}
			}

			// Get the plot-modulated weight summation of all possible animals to spawn
			for (iJ = 0; iJ < GC.getNumUnitClassInfos(); iJ++)
			{
				eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(GET_PLAYER(ANIMAL_PLAYER).getCivilizationType()).getCivilizationUnits(iJ)));
				if (eLoopUnit == NO_UNIT) continue;

				// Disallowing already spawned world animals : Feral Animals Valkrionn 10/19/09
				if ((GET_PLAYER(ANIMAL_PLAYER).isUnitClassMaxedOut((UnitClassTypes)iJ))
				 || (GET_TEAM(GET_PLAYER(ANIMAL_PLAYER).getTeam()).isUnitClassMaxedOut((UnitClassTypes)iJ))
				 || (isUnitClassMaxedOut((UnitClassTypes)iJ)))
					continue;

				// To be valid: If the plot has a feature, it must match. Otherwise, it must match the terrain.
				if ((pPlot->getFeatureType() == NO_FEATURE) ? !GC.getUnitInfo(eLoopUnit).getTerrainNative(pPlot->getTerrainType()) : !GC.getUnitInfo(eLoopUnit).getFeatureNative(pPlot->getFeatureType()))
					continue;

				iValue = (GC.getUnitInfo(eLoopUnit).getAppearanceProb());
				for (int iI = 0; iI < GC.getNumTechInfos(); iI++)
				{
					iValue += (GC.getUnitInfo(eLoopUnit).getAppearanceTechs(iI)) * (countKnownTechNumTeams((TechTypes)iI) > 0 ? 1 : 0);
				}
				if (iValue > 0)
				{
					iNetValue += iValue;
				}
			}

			// From the total weight, pick a number at random. We then need to do the same calculation, to find which animal it's associated with.
			iTargetValue = getMapRandNum(iNetValue, "Select Animal Spawn");
			for (iJ = 0; iJ < GC.getNumUnitClassInfos(); iJ++)
			{
				eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(GET_PLAYER(ANIMAL_PLAYER).getCivilizationType()).getCivilizationUnits(iJ)));
				if (eLoopUnit == NO_UNIT) continue;

				// Disallowing already spawned world animals : Feral Animals Valkrionn 10/19/09
				if ((GET_PLAYER(ANIMAL_PLAYER).isUnitClassMaxedOut((UnitClassTypes)iJ))
				 || (GET_TEAM(GET_PLAYER(ANIMAL_PLAYER).getTeam()).isUnitClassMaxedOut((UnitClassTypes)iJ))
				 || (isUnitClassMaxedOut((UnitClassTypes)iJ)))
					continue;

				// To be valid: If the plot has a feature, it must match. Otherwise, it must match the terrain.
				if ((pPlot->getFeatureType() == NO_FEATURE) ? !GC.getUnitInfo(eLoopUnit).getTerrainNative(pPlot->getTerrainType()) : !GC.getUnitInfo(eLoopUnit).getFeatureNative(pPlot->getFeatureType()))
					continue;

				iValue = (GC.getUnitInfo(eLoopUnit).getAppearanceProb());
				for (int iI = 0; iI < GC.getNumTechInfos(); iI++)
				{
					iValue += (GC.getUnitInfo(eLoopUnit).getAppearanceTechs(iI)) * (countKnownTechNumTeams((TechTypes)iI) > 0 ? 1 : 0);
				}
				if (iValue > 0)
				{
					iTargetValue -= iValue;
				}
				if (iTargetValue <= 0)
				{
					eBestUnit = eLoopUnit;
					break;
				}
			}

			if (eBestUnit != NO_UNIT)
			{
				CvUnit* pUnit = GET_PLAYER(ANIMAL_PLAYER).initUnit(eBestUnit, pPlot->getX_INLINE(), pPlot->getY_INLINE(), UNITAI_ANIMAL);
				if (isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
				{
					pUnit->setImmobileTimer(2);
				}
			}
		}
	}
}

/*************************************************************************************************/
/**	Spawn Groups						08/05/10									Valkrionn	**/
/**																								**/
/**					New spawn mechanic, allowing us to customize stacks							**/
/**					Entire function rewritten, just commenting it all.							**/
/*************************************************************************************************/
void CvGame::createBarbarianUnits()
{
	PROFILE("CvGame::createBarbarianUnits");
	CvArea* pLoopArea;
	CvPlot* pPlot;
	UnitAITypes eBarbUnitAI;
	SpawnGroupTypes eBestGroup;
	SpawnGroupTypes eLoopGroup;
	long lResult;
	int iNeededBarbs, iTargetBarbs, iValue, iBestValue, iLoop, iI, iJ, iPreference, iFlags;
	bool bAlwaysSpawn;

	if (isOption(GAMEOPTION_NO_BARBARIANS))
	{
		return;
	}

	lResult = 0;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "createBarbarianUnits", NULL, &lResult);
	if (lResult == 1)
	{
		return;
	}

	// Random barbs only spawn past lair start, and if there are enough civ cities in the world
	if ((getElapsedGameTurns() < GC.getGameSpeedInfo(getGameSpeedType()).getTurnsPerLairCycle())
		|| (getNumCivCities() < ((countCivPlayersAlive() * 3) / 2) || isOption(GAMEOPTION_NO_SETTLERS)))
	{
		return;
	}

	// Random spawn can't be on 1-tile island I guess, nor in deep ocean. Weighting is also weird for peaks, so don't spawn on those
	iFlags = RANDPLOT_ORC_ALLY | RANDPLOT_ADJACENT_LAND | RANDPLOT_NOT_PEAK | RANDPLOT_PASSIBLE | RANDPLOT_UNOCCUPIED;
	if (isOption(GAMEOPTION_NO_VISIBLE_BARBARIANS))
	{
		iFlags |= RANDPLOT_NOT_VISIBLE_TO_CIV;
	}

	for (pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
	{
		// Don't count owned tiles, even if allied to orc player, to spawn random orcs
		iTargetBarbs = calcTargetBarbs(pLoopArea, false, ORC_PLAYER);
		// Spawn at most 20% of limit (round up)
		iNeededBarbs = std::min(std::max(1, 2 * iTargetBarbs/10), iTargetBarbs - pLoopArea->getUnitsPerPlayer(ORC_PLAYER));

		pLoopArea->isWater() ? eBarbUnitAI = UNITAI_ATTACK_SEA : eBarbUnitAI = UNITAI_ATTACK;

		for (iI = 0; iI < iNeededBarbs; iI++)
		{
			pPlot = GC.getMapINLINE().syncRandPlot(iFlags, pLoopArea->getID());

			if (pPlot == NULL) continue;

			eBestGroup = NO_SPAWNGROUP;
			iBestValue = 0;
			bAlwaysSpawn = false;

			for (iJ = 0; iJ < GC.getNumSpawnGroupInfos(); iJ++)
			{
				eLoopGroup = ((SpawnGroupTypes)iJ);

				if (eLoopGroup == NO_SPAWNGROUP) continue;
				if (!isSpawnGroupValid(eLoopGroup, pPlot, ORC_TEAM)) continue;

				// We assume prereqs are met due to above. If group has prereqs (that are met), may adjust weight
				iPreference = 0;

				// Possible: Increase weight if has tech reqs? Or decrease, if lategame barbs are too strong? Simply diluting the pool may suffice.
				// More prereqs from isSpawnGroupValid can be added to weights as well if needed.
				// // 200 per TechAND?
				// iPreference += (GC.getSpawnGroupInfo(eLoopGroup).getNumPrereqTechANDs() * 200);
				// // 200 per TechOR?
				// iPreference += (GC.getSpawnGroupInfo(eLoopGroup).getNumPrereqTechORs() * 200);

				// Any met terrain limit increases weight. The more possible limits, reduce the (still positive) weight
				if (GC.getSpawnGroupInfo(eLoopGroup).getNumSpawnTerrains() > 0)
				{
					iPreference += 300;
					iPreference -= std::min(GC.getSpawnGroupInfo(eLoopGroup).getNumSpawnTerrains() * 50, 200);
				}
				// Any met feature limit increases weight. The more possible limits, reduce the (still positive) weight
				if (GC.getSpawnGroupInfo(eLoopGroup).getNumSpawnFeatures() > 0)
				{
					iPreference += 300;
					iPreference -= std::min(GC.getSpawnGroupInfo(eLoopGroup).getNumSpawnFeatures() * 50, 200);
				}

				// Weights by default are ~1000
				iValue = iPreference + (1 + getSorenRandNum(GC.getSpawnGroupInfo(eLoopGroup).getWeight(), "Barb Unit Selection"));

				// AlwaysSpawns should still look for best among possible AlwaysSpawns
				if (GC.getSpawnGroupInfo(eLoopGroup).isAlwaysSpawn())
				{
					if (bAlwaysSpawn && iBestValue > iValue) continue;

					eBestGroup = eLoopGroup;
					iBestValue = iValue;
					bAlwaysSpawn = true;
				}
				else if (iValue > iBestValue && !bAlwaysSpawn)
				{
					eBestGroup = eLoopGroup;
					iBestValue = iValue;
				}
			}

			if (eBestGroup != NO_SPAWNGROUP)
			{
				createSpawnGroup(eBestGroup, pPlot, ORC_PLAYER, eBarbUnitAI);
			}
		}
		// Python exposure for modular barb bosses. They can spawn regardless of density limit (if exist...)
		if (pLoopArea->getNumUnownedTiles() > 0)
		{
			if (isOption(GAMEOPTION_NO_VISIBLE_BARBARIANS))
				pPlot = GC.getMapINLINE().syncRandPlot((RANDPLOT_ORC_ALLY | RANDPLOT_ADJACENT_LAND | RANDPLOT_PASSIBLE | RANDPLOT_UNOCCUPIED| RANDPLOT_NOT_VISIBLE_TO_CIV), pLoopArea->getID(), GC.getDefineINT("MIN_BARBARIAN_STARTING_DISTANCE"));
			else
				pPlot = GC.getMapINLINE().syncRandPlot((RANDPLOT_ORC_ALLY | RANDPLOT_ADJACENT_LAND | RANDPLOT_PASSIBLE | RANDPLOT_UNOCCUPIED), pLoopArea->getID(), GC.getDefineINT("MIN_BARBARIAN_STARTING_DISTANCE"));
			CyPlot* pyPlot = new CyPlot(pPlot);
			CyArgsList argsList;
			argsList.add(gDLL->getPythonIFace()->makePythonObject(pyPlot));	// pass in plot class
			gDLL->getPythonIFace()->callFunction(PYGameModule, "createBarbarianBosses", argsList.makeFunctionArgs());
			delete pyPlot;	// python fxn must not hold on to this pointer
		}
	}
}

// Each barb type has slightly different density limit calculation. All here for easy comparison.
// Note that barbs can spawn as groups and so go over the limit on the turn spawned, but should not
// be substantially over aside from happenstance of small limit and multiple large groups spawning.
int CvGame::calcTargetBarbs(CvArea* pArea, bool bCountOwnedPlots, PlayerTypes ePlayer) const
{
	FAssertMsg(pArea != NULL, "Need passing valid area to calculated barb density");
	FAssertMsg(ePlayer == ANIMAL_PLAYER || ePlayer == ORC_PLAYER || ePlayer == DEMON_PLAYER, "Need to calculate spawn for a barb team")

	int iTargetBarbs = 0;
	int iAreaSize = 0;

	if (ePlayer == ANIMAL_PLAYER)
	{
		iAreaSize = (bCountOwnedPlots ? pArea->getNumTiles() : pArea->getNumUnownedTiles());
		int iDivisor = (pArea->isWater() ?
			GC.getHandicapInfo(getHandicapType()).getWaterTilesPerAnimal() :
			GC.getHandicapInfo(getHandicapType()).getTilesPerAnimal());

		iTargetBarbs = iAreaSize / std::max(1, iDivisor);

		if (isOption(GAMEOPTION_DOUBLE_ANIMALS)) iTargetBarbs *= 2;
	}
	else if (ePlayer == ORC_PLAYER)
	{
		iAreaSize = (bCountOwnedPlots ? pArea->getNumTiles() : pArea->getNumUnownedTiles());
		int iDivisor = (pArea->isWater() ?
			GC.getHandicapInfo(getHandicapType()).getWaterTilesPerOrc() :
			GC.getHandicapInfo(getHandicapType()).getTilesPerOrc());

		// Originally had a max of 5 per body of water; will see if need to reduce
		iTargetBarbs = iAreaSize / std::max(1, iDivisor);
	}
	else
	{
		// Demons don't have an "unowned evil" tile calculation; need to add one in CvArea if desired
		iAreaSize = pArea->getNumEvilTiles();
		// Reduce density on water, otherwise big oceans can get loooots of demons
		if (pArea->isWater()) iAreaSize /= 20;

		// AC effect is additive to demons/evil plot calcuation. Ex: 10AC*0.4/AC adds +4% to the X%demon/evil tile target density
		int iCounterAdjust = getGlobalCounter() * GC.getHandicapInfo(getHandicapType()).getPercentDemonsPerEvilPlotPerGlobalCounter() / 100;
		iTargetBarbs = (GC.getHandicapInfo(getHandicapType()).getPercentDemonsPerEvilPlot() + iCounterAdjust) * iAreaSize / 100;
	}

	if (isOption(GAMEOPTION_RAGING_BARBARIANS)) iTargetBarbs *= 2;

	// Min limit for spawning is handled per spawning type (random/group, lair)
	return std::max(0, iTargetBarbs);
}

/*************************************************************************************************/
/**	Spawn Groups						08/12/10									Valkrionn	**/
/**																								**/
/**					New spawn mechanic, allowing us to customize stacks							**/
/*************************************************************************************************/
bool CvGame::isSpawnGroupValid(SpawnGroupTypes eSpawnGroup, CvPlot* pPlot, TeamTypes eTeam)
{
	if (GC.getSpawnGroupInfo(eSpawnGroup).isNeverSpawn())
		return false;

	if (isOption(GAMEOPTION_NO_RANDOM_BARBARIANS) && !GC.getSpawnGroupInfo(eSpawnGroup).isAlwaysSpawn())
		return false;

	if (pPlot->isWater() && !GC.getSpawnGroupInfo(eSpawnGroup).isNaval())
	{
		return false;
	}
	else if (!pPlot->isWater() && GC.getSpawnGroupInfo(eSpawnGroup).isNaval())
	{
		return false;
	}

	if (GC.getSpawnGroupInfo(eSpawnGroup).isUnique())
	{
		if (getSpawnGroupCreatedCount(eSpawnGroup) != 0)
		{
			return false;
		}
	}

	if (GC.getSpawnGroupInfo(eSpawnGroup).getPrereqMinTurn() != 0)
	{
		if (GC.getSpawnGroupInfo(eSpawnGroup).getPrereqMinTurn() >= getGameTurn())
		{
			return false;
		}
	}

	if (GC.getSpawnGroupInfo(eSpawnGroup).getPrereqMaxTurn() != 0)
	{
		if (GC.getSpawnGroupInfo(eSpawnGroup).getPrereqMaxTurn() <= getGameTurn())
		{
			return false;
		}
	}

	if (GC.getSpawnGroupInfo(eSpawnGroup).getPrereqMinAC() != 0)
	{
		if (GC.getSpawnGroupInfo(eSpawnGroup).getPrereqMinAC() >= getGlobalCounter())
		{
			return false;
		}
	}

	if (GC.getSpawnGroupInfo(eSpawnGroup).getPrereqMaxAC() != 0)
	{
		if (GC.getSpawnGroupInfo(eSpawnGroup).getPrereqMaxAC() <= getGlobalCounter())
		{
			return false;
		}
	}

	if (GC.getSpawnGroupInfo(eSpawnGroup).getPrereqProject() != NO_PROJECT)
	{
		if (getProjectCreatedCount((ProjectTypes)(GC.getSpawnGroupInfo(eSpawnGroup).getPrereqProject())) == 0)
		{
			return false;
		}
	}

	if (GC.getSpawnGroupInfo(eSpawnGroup).getNumPrereqTechANDs() > 0)
	{
		// Barb and demon teams gain tech in CvTeam::doTurn at a delay from civs that know it
		int iNumPrereqTechANDs = GC.getSpawnGroupInfo(eSpawnGroup).getNumPrereqTechANDs();
		for (int iK = 0; iK < iNumPrereqTechANDs; iK++)
		{
			if (!GET_TEAM(eTeam).isHasTech((TechTypes)GC.getSpawnGroupInfo(eSpawnGroup).getPrereqTechANDs(iK)))
			{
				return false;
			}
		}
	}

	if (GC.getSpawnGroupInfo(eSpawnGroup).getNumPrereqTechORs() > 0)
	{
		bool bPass = false;
		int iNumPrereqTechORs = GC.getSpawnGroupInfo(eSpawnGroup).getNumPrereqTechORs();
		for (int iK = 0; iK < iNumPrereqTechORs; iK++)
		{
			if (GET_TEAM(eTeam).isHasTech((TechTypes)GC.getSpawnGroupInfo(eSpawnGroup).getPrereqTechORs(iK)))
			{
				bPass = true;
				break;
			}
		}
		if (!bPass)
		{
			return false;
		}
	}

	if (GC.getSpawnGroupInfo(eSpawnGroup).getNumBlockTechANDs() > 0)
	{
		int iNumBlockTechANDs = GC.getSpawnGroupInfo(eSpawnGroup).getNumBlockTechANDs();
		for (int iK = 0; iK < iNumBlockTechANDs; iK++)
		{
			if (GET_TEAM(eTeam).isHasTech((TechTypes)GC.getSpawnGroupInfo(eSpawnGroup).getBlockTechANDs(iK)))
			{
				return false;
			}
		}
	}

	if (GC.getSpawnGroupInfo(eSpawnGroup).getNumBlockTechORs() > 0)
	{
		bool bPass = true;
		int iNumBlockTechORs = GC.getSpawnGroupInfo(eSpawnGroup).getNumBlockTechORs();
		for (int iK = 0; iK < iNumBlockTechORs; iK++)
		{
			if (GET_TEAM(eTeam).isHasTech((TechTypes)GC.getSpawnGroupInfo(eSpawnGroup).getBlockTechORs(iK)))
			{
				bPass = false;
				break;
			}
		}
		if (!bPass)
		{
			return false;
		}
	}

	if (GC.getSpawnGroupInfo(eSpawnGroup).getNumSpawnTerrains() > 0)
	{
		bool bPass = false;
		int iNumSpawnTerrains = GC.getSpawnGroupInfo(eSpawnGroup).getNumSpawnTerrains();
		for (int iK = 0; iK < iNumSpawnTerrains; iK++)
		{
			TerrainTypes eSpawnTerrain = (TerrainTypes)GC.getSpawnGroupInfo(eSpawnGroup).getSpawnTerrains(iK);
			if (pPlot->getTerrainType() == eSpawnTerrain)
			{
				bPass = true;
				break;
			}
		}
		if (!bPass)
		{
			return false;
		}
	}

	if (GC.getSpawnGroupInfo(eSpawnGroup).getNumSpawnFeatures() > 0)
	{
		bool bPass = false;
		int iNumSpawnFeatures = GC.getSpawnGroupInfo(eSpawnGroup).getNumSpawnFeatures();
		for (int iK = 0; iK < iNumSpawnFeatures; iK++)
		{
			FeatureTypes eSpawnFeature = (FeatureTypes)GC.getSpawnGroupInfo(eSpawnGroup).getSpawnFeatures(iK);
			if (pPlot->getFeatureType() == eSpawnFeature)
			{
				bPass = true;
				break;
			}
		}
		if (!bPass)
		{
			return false;
		}
	}

	if (!CvString(GC.getSpawnGroupInfo(eSpawnGroup).getPyRequirement()).empty()) /* testing lfgr proposition - 2013-11-21 */
	{
		CyPlot* pyPlot = new CyPlot(pPlot);
		CyArgsList argsList;
		argsList.add(gDLL->getPythonIFace()->makePythonObject(pyPlot));
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYSpellModule, GC.getSpawnGroupInfo(eSpawnGroup).getPyRequirement(), argsList.makeFunctionArgs(), &lResult);
		delete pyPlot;
		if (lResult == 0)
		{
			return false;
		}
	}

	return true;
}

void CvGame::createSpawnGroup(SpawnGroupTypes eSpawnGroup, CvPlot* pPlot, PlayerTypes ePlayer, UnitAITypes eUnitAI)
{
	CvUnit* pCommanderUnit=NULL;
	bool bCommanded = false;
	CvWString szBuffer;

	if (GC.getSpawnGroupInfo(eSpawnGroup).getNumCommanderUnits() > 0)
	{
		int iNumCommanders = GC.getSpawnGroupInfo(eSpawnGroup).getNumCommanderUnits();
		for (int iK = 0; iK < iNumCommanders; iK++)
		{
			pCommanderUnit = GET_PLAYER(ePlayer).initUnit((UnitTypes)GC.getSpawnGroupInfo(eSpawnGroup).getCommanderUnits(iK), pPlot->getX_INLINE(), pPlot->getY_INLINE(), eUnitAI);
			bCommanded = true;

			// Tell players something spawned for them
			if (ePlayer != DEMON_PLAYER && ePlayer != ANIMAL_PLAYER && ePlayer != ORC_PLAYER)
			{
				szBuffer = gDLL->getText("TXT_KEY_IMPROVEMENT_SPAWN_GROUP", GC.getImprovementInfo(pPlot->getImprovementType()).getTextKeyWide(), pCommanderUnit->getName().GetCString());
				gDLL->getInterfaceIFace()->addMessage(ePlayer, false, GC.getEVENT_MESSAGE_TIME(), szBuffer,  "AS2D_UNITGIFTED", MESSAGE_TYPE_INFO, pCommanderUnit->getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_YELLOW"), pPlot->getX_INLINE(), pPlot->getY_INLINE(), true, true);
			}

			if (GC.getSpawnGroupInfo(eSpawnGroup).getNumCommanderPromotions() > 0)
			{
				int iNumCommanderPromotions = GC.getSpawnGroupInfo(eSpawnGroup).getNumCommanderPromotions();
				for (int iL = 0; iL < iNumCommanderPromotions; iL++)
				{
					pCommanderUnit->setHasPromotion((PromotionTypes)GC.getSpawnGroupInfo(eSpawnGroup).getCommanderPromotions(iL), true);
				}
			}
			if (GC.getSpawnGroupInfo(eSpawnGroup).getNumGroupPromotions() > 0)
			{
				int iNumGroupPromotions = GC.getSpawnGroupInfo(eSpawnGroup).getNumGroupPromotions();
				for (int iL = 0; iL < iNumGroupPromotions; iL++)
				{
					pCommanderUnit->setHasPromotion((PromotionTypes)GC.getSpawnGroupInfo(eSpawnGroup).getGroupPromotions(iL), true);
				}
			}

			if (isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
			{
				pCommanderUnit->setImmobileTimer(2);
			}

		}
	}

	for (int iK = 0; iK < GC.getNumUnitInfos(); iK++)
	{
		if (GC.getSpawnGroupInfo(eSpawnGroup).getGroupUnits(iK) > 0)
		{
			for (int iM=0; iM < GC.getSpawnGroupInfo(eSpawnGroup).getGroupUnits(iK); iM++)
			{
				CvUnit* pUnit = GET_PLAYER(ePlayer).initUnit((UnitTypes)iK, pPlot->getX_INLINE(), pPlot->getY_INLINE(), eUnitAI);

				if (GC.getSpawnGroupInfo(eSpawnGroup).getNumGroupPromotions() > 0)
				{
					int iNumGroupPromotions = GC.getSpawnGroupInfo(eSpawnGroup).getNumGroupPromotions();
					for (int iL = 0; iL < iNumGroupPromotions; iL++)
					{
						pUnit->setHasPromotion((PromotionTypes)GC.getSpawnGroupInfo(eSpawnGroup).getGroupPromotions(iL), true);
					}
				}

				if (isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
				{
					pUnit->setImmobileTimer(2);  /* testing lfgr proposition - 2013-11-21 */
				}
				if (bCommanded)
				{
					pUnit->joinGroup(pCommanderUnit->getGroup());
					if (pCommanderUnit->getCommandLimit() > pCommanderUnit->getNumMinions())
					{
						pCommanderUnit->addMinionUnit(pUnit->getID());
					}
				}
				else
				{
					szBuffer = gDLL->getText("TXT_KEY_IMPROVEMENT_SPAWN_UNIT", GC.getImprovementInfo(pPlot->getImprovementType()).getTextKeyWide(), pUnit->getName().GetCString());
					gDLL->getInterfaceIFace()->addMessage(ePlayer, false, GC.getEVENT_MESSAGE_TIME(), szBuffer,  "AS2D_UNITGIFTED", MESSAGE_TYPE_INFO, pUnit->getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_YELLOW"), pPlot->getX_INLINE(), pPlot->getY_INLINE(), iK==0, iK==0);
				}
			}
		}
	}

	for (int iK = 0; iK < GC.getNumUnitClassInfos(); iK++)
	{
		if (GC.getSpawnGroupInfo(eSpawnGroup).getGroupUnitClasses(iK) > 0)
		{
			UnitTypes eLoopUnit=NO_UNIT;
			UnitTypes eBestUnit=NO_UNIT;
			int iCivValue = 0;
			int iBestCivValue = 0;

			for (int iL = 0; iL < GC.getNumCivilizationInfos(); iL++)
			{
				eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo((CivilizationTypes)iL).getCivilizationUnits(iK)));

				if (eLoopUnit != NO_UNIT)
				{
					int iRand = 100;
					if (iL == GET_PLAYER(ePlayer).getCivilizationType())
					{
						iRand += 50;
					}
					iCivValue = getSorenRandNum(iRand, "Unique Unit Selection");
					if (iCivValue > iBestCivValue)
					{
						eBestUnit = eLoopUnit;
						iBestCivValue = iCivValue;
					}
				}
			}

			if (eBestUnit != NO_UNIT)
			{
				for (int iM = 0; iM < GC.getSpawnGroupInfo(eSpawnGroup).getGroupUnitClasses(iK); iM++)
				{
					CvUnit* pUnit = GET_PLAYER(ePlayer).initUnit(eBestUnit, pPlot->getX_INLINE(), pPlot->getY_INLINE(), eUnitAI);

					if (GC.getSpawnGroupInfo(eSpawnGroup).getNumGroupPromotions() > 0)
					{
						int iNumGroupPromotions = GC.getSpawnGroupInfo(eSpawnGroup).getNumGroupPromotions();
						for (int iL = 0; iL < iNumGroupPromotions; iL++)
						{
							pUnit->setHasPromotion((PromotionTypes)GC.getSpawnGroupInfo(eSpawnGroup).getGroupPromotions(iL), true);
						}
					}

					if (isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
					{
						pUnit->setImmobileTimer(2);   /* testing lfgr proposition - 2013-11-21 */
					}
					if (bCommanded)
					{
						pUnit->joinGroup(pCommanderUnit->getGroup());
						if (pCommanderUnit->getCommandLimit() > pCommanderUnit->getNumMinions())
						{
							pCommanderUnit->addMinionUnit(pUnit->getID());
						}
					}
				}
			}
		}
	}

	incrementSpawnGroupCreatedCount(eSpawnGroup);

	if (!CvString(GC.getSpawnGroupInfo(eSpawnGroup).getPyOnSpawn()).empty())
	{
		CyPlot* pyPlot = new CyPlot(pPlot);
		CyArgsList argsList;
		argsList.add(gDLL->getPythonIFace()->makePythonObject(pyPlot));
		gDLL->getPythonIFace()->callFunction(PYSpellModule, GC.getSpawnGroupInfo(eSpawnGroup).getPyOnSpawn(), argsList.makeFunctionArgs());
		delete pyPlot;
	}
}
/*************************************************************************************************/
/**	Spawn Groups							END													**/
/*************************************************************************************************/

int CvGame::getNextLairCycle() const
{
	return m_iNextLairCycle;
}

void CvGame::setNextLairCycle(int iNewValue)
{
	m_iNextLairCycle = iNewValue;
}

int CvGame::getNumCivActive(CivilizationTypes eCivilization) const
{
	int iCount = 0;
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kLoopPlayer.isAlive())
		{
			if (kLoopPlayer.getCivilizationType() == eCivilization)
			{
				iCount++;
			}
		}
	}

	return iCount;
}

int CvGame::getCivActivePlayer(CivilizationTypes eCivilization, int iCount) const
{
	if (iCount < 0)
	{
		return -1;
	}
	iCount++;		//Required for standard loops, which run from 0 to (iCount - 1), to work properly
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kLoopPlayer.isAlive())
		{
			if (kLoopPlayer.getCivilizationType() == eCivilization)
			{
				iCount--;
				if (iCount == 0)
				{
					return iPlayer;
				}
			}
		}
	}
	return -1;
}
/*************************************************************************************************/
/**	New Tag Defs							END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	Speedup								11/02/12										Snarko	**/
/**																								**/
/**			Was sometimes called thousands of times per turn, which is slow						**/
/*************************************************************************************************/
bool CvGame::isRouteAlwaysAvailable(RouteTypes eRoute) const
{
	CvRouteInfo kRouteInfo = GC.getRouteInfo(eRoute);
	int iI, iJ;

	if (kRouteInfo.getPrereqBonus() != NO_BONUS)
		return false;

	for (iI = 0; iI < GC.getNUM_ROUTE_PREREQ_OR_BONUSES(); iI++)
		if (kRouteInfo.getPrereqOrBonus(iI) != NO_BONUS)
			return false;

	RouteTypes eBuildRoute;
	CvBuildInfo kBuildInfo;
	for (iI = 0; iI < GC.getNumBuildInfos(); iI++)
	{
		eBuildRoute = (RouteTypes)GC.getBuildInfo((BuildTypes)iI).getRoute();

		if (eBuildRoute == eRoute)
		{
			kBuildInfo = GC.getBuildInfo((BuildTypes)iI);

			if (kBuildInfo.getCost() > 0)
				return false;

			for (iJ = 0; iJ < GC.getNumFeatureInfos(); iJ++)
			{
				if (kBuildInfo.getFeatureTech(iJ) != NO_TECH)
					return false;
			}
		}
	}

	return true;
}
/*************************************************************************************************/
/**	Speedup									END													**/
/*************************************************************************************************/
//FlagSystem
void CvGame::setGlobalFlag(FlagTypes eFlag, bool eChange)
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
		GET_PLAYER((PlayerTypes)iI).setHasFlag(eFlag, eChange);
}
void CvGame::setGlobalFlagValue(FlagTypes eFlag, int eChange)
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
		GET_PLAYER((PlayerTypes)iI).setFlagValue(eFlag, eChange);
}
void CvGame::changeGlobalFlagValue(FlagTypes eFlag, int eChange)
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
		GET_PLAYER((PlayerTypes)iI).changeFlagValue(eFlag, eChange);
}
bool CvGame::isUniDay() {
	time_t rawtime;
	tm* ptm;

	time(&rawtime);

	ptm = localtime(&rawtime);

	if (ptm->tm_mon == 3)
	{
		if (ptm->tm_mday == 1)
		{
			return true;
		}
	}

	return false;
}
bool CvGame::isTriggeredGoody(GoodyTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumGoodyInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_pabTriggeredGoodies[eIndex];
}

void CvGame::setTriggeredGoody(GoodyTypes eIndex, bool bValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumGoodyInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_pabTriggeredGoodies[eIndex] = bValue;
}

