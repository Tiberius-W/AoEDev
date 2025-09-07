#pragma once

// selectionGroup.h

#ifndef CIV4_GROUP_H
#define CIV4_GROUP_H

//#include "CvStructs.h"
#include "LinkedList.h"
/*************************************************************************************************/
/**	K-mod merger								16/02/12								Snarko	**/
/**																								**/
/**					Merging in features of K-mod, most notably the pathfinder					**/
/*************************************************************************************************/
#include "KmodPathFinder.h"
/*************************************************************************************************/
/**	K-mod merger								END												**/
/*************************************************************************************************/

class CvPlot;
class CvArea;
class FAStarNode;

class CvSelectionGroup
{

public:

	CvSelectionGroup();
	virtual ~CvSelectionGroup();

	DllExport void init(int iID, PlayerTypes eOwner);
	DllExport void uninit();
	DllExport void reset(int iID = 0, PlayerTypes eOwner = NO_PLAYER, bool bConstructorCall = false);

	void kill();

	void doTurn();

	bool showMoves() const;

	void updateTimers();
	bool doDelayedDeath();

	void playActionSound();

	DllExport void pushMission(MissionTypes eMission, int iData1 = -1, int iData2 = -1, int iFlags = 0, bool bAppend = false, bool bManual = false, MissionAITypes eMissionAI = NO_MISSIONAI, CvPlot* pMissionAIPlot = NULL, CvUnit* pMissionAIUnit = NULL);		// Exposed to Python
	void popMission();																																										// Exposed to Python
	DllExport void autoMission();
	void updateMission();
	DllExport CvPlot* lastMissionPlot();																																					// Exposed to Python

	DllExport bool canStartMission(int iMission, int iData1, int iData2, CvPlot* pPlot = NULL, bool bTestVisible = false, bool bUseCache = false);		// Exposed to Python
	void startMission();
	void continueMission(int iSteps = 0);

	DllExport bool canDoInterfaceMode(InterfaceModeTypes eInterfaceMode);													// Exposed to Python
	DllExport bool canDoInterfaceModeAt(InterfaceModeTypes eInterfaceMode, CvPlot* pPlot);				// Exposed to Python

	DllExport bool canDoCommand(CommandTypes eCommand, int iData1, int iData2, bool bTestVisible = false, bool bUseCache = false);		// Exposed to Python
	bool canEverDoCommand(CommandTypes eCommand, int iData1, int iData2, bool bTestVisible, bool bUseCache);
	void setupActionCache();

	bool isHuman();																																											// Exposed to Python
	DllExport bool isBusy();
	bool isCargoBusy();
/*************************************************************************************************/
/**	K-mod merger								16/02/12								Snarko	**/
/**																								**/
/**					Merging in features of K-mod, most notably the pathfinder					**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	int baseMoves();
/**								----  End Original Code  ----									**/
	int baseMoves() const;																																										// Exposed to Python
	int maxMoves() const; // K-Mod
	int movesLeft() const; // K-Mod
/*************************************************************************************************/
/**	K-mod merger								END												**/
/*************************************************************************************************/
																																										// Exposed to Python
	bool isWaiting() const;																																							// Exposed to Python
	bool isFull();																																											// Exposed to Python
	bool hasCargo();																																										// Exposed to Python
	int getCargo() const;
	DllExport bool canAllMove();																																				// Exposed to Python
	bool canAnyMove();																																									// Exposed to Python
	bool hasMoved();																																										// Exposed to Python
	bool canEnterTerritory(TeamTypes eTeam, bool bIgnoreRightOfPassage = false) const;									// Exposed to Python
	bool canEnterArea(TeamTypes eTeam, const CvArea* pArea, bool bIgnoreRightOfPassage = false) const;									// Exposed to Python
	DllExport bool canMoveInto(CvPlot* pPlot, bool bAttack = false);																		// Exposed to Python
	DllExport bool canMoveOrAttackInto(CvPlot* pPlot, bool bDeclareWar = false);												// Exposed to Python
/*************************************************************************************************/
/**	AITweak							14/07/10								Snarko				**/
/**																								**/
/**			Teaching AI to sometimes ignore if the other unit has attacked this turn			**/
/**				canMoveOrAttackInto is called by exe so we can't directly change it.			**/
/*************************************************************************************************/
	bool canMoveOrAttackInto2(CvPlot* pPlot, bool bDeclareWar = false, bool bIgnoreHasAttacked = false);
/*************************************************************************************************/
/**	AITweak									END													**/
/*************************************************************************************************/
	bool canMoveThrough(CvPlot* pPlot);
																																// Exposed to Python
/*************************************************************************************************/
/**	Tweak								15/07/10										Snarko	**/
/**																								**/
/**								Making workers run away											**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	bool canFight();																																										// Exposed to Python
/**								----  End Original Code  ----									**/
	int canFight(bool bIgnoreWorkers = false, bool bIgnoreSettlers = false);																																										// Exposed to Python
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
	bool canDefend();																																										// Exposed to Python
	bool canBombard(const CvPlot* pPlot);
	bool visibilityRange();
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
	int getBombardTurns( CvCity* pCity );
	bool isHasPathToAreaPlayerCity( PlayerTypes ePlayer, int iFlags = 0, int iMaxPathTurns = -1 );
	bool isHasPathToAreaEnemyCity( bool bIgnoreMinors = true, int iFlags = 0, int iMaxPathTurns = -1 );
	bool isStranded();
	void invalidateIsStrandedCache();
	bool calculateIsStranded();
	bool canMoveAllTerrain() const;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	void unloadAll();
	bool alwaysInvisible() const;																																							// Exposed to Python
	bool isInvisible(TeamTypes eTeam) const;																								// Exposed to Python
	int countNumUnitAIType(UnitAITypes eUnitAI);																												// Exposed to Python
	bool hasWorker();																																										// Exposed to Python
	bool IsSelected();
	DllExport void NotifyEntity(MissionTypes eMission);
	void airCircle(bool bStart);
	void setBlockading(bool bStart);

	int getX() const;
	int getY() const;
	bool at(int iX, int iY) const;																																								// Exposed to Python
	bool atPlot(const CvPlot* pPlot) const;																																				// Exposed to Python
	DllExport CvPlot* plot() const;																																								// Exposed to Python
	int getArea() const;
	CvArea* area() const;																																													// Exposed to Python
	DomainTypes getDomainType() const;

	RouteTypes getBestBuildRoute(CvPlot* pPlot, BuildTypes* peBestBuild = NULL) const;	// Exposed to Python

	bool groupDeclareWar(CvPlot* pPlot, bool bForce = false);
	bool groupAttack(int iX, int iY, int iFlags, bool& bFailedAlreadyFighting);
	void groupMove(CvPlot* pPlot, bool bCombat, CvUnit* pCombatUnit = NULL, bool bEndMove = false);
	bool groupPathTo(int iX, int iY, int iFlags);
	bool groupRoadTo(int iX, int iY, int iFlags);
	bool groupBuild(BuildTypes eBuild);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/18/10                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
	void setTransportUnit(CvUnit* pTransportUnit, CvSelectionGroup** pOtherGroup = NULL);
	void setRemoteTransportUnit(CvUnit* pTransportUnit);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	bool isAmphibPlot(const CvPlot* pPlot) const;																																		// Exposed to Python
	bool groupAmphibMove(CvPlot* pPlot, int iFlags);

	DllExport bool readyToSelect(bool bAny = false);																										// Exposed to Python
	bool readyToMove(bool bAny = false);																																// Exposed to Python
	bool readyToAuto();																																									// Exposed to Python

	int getID() const;																																												// Exposed to Python
	void setID(int iID);

	int getMissionTimer() const;
	void setMissionTimer(int iNewValue);
	void changeMissionTimer(int iChange);
	void updateMissionTimer(int iSteps = 0);

	bool isForceUpdate();
	void setForceUpdate(bool bNewValue);

//FfH: Added by Kael 12/28/2008
	bool isEnraged() const;
//FfH: End Add

	DllExport PlayerTypes getOwner() const;																															// Exposed to Python
#ifdef _USRDLL
	inline PlayerTypes getOwnerINLINE() const
	{
		return m_eOwner;
	}
#endif
	TeamTypes getTeam() const;																																					// Exposed to Python

	ActivityTypes getActivityType() const;																															// Exposed to Python
	void setActivityType(ActivityTypes eNewValue);																											// Exposed to Python

	AutomateTypes getAutomateType() const;																																		// Exposed to Python
	bool isAutomated();																																									// Exposed to Python
	void setAutomateType(AutomateTypes eNewValue);																											// Exposed to Python

	FAStarNode* getPathLastNode() const;
	CvPlot* getPathFirstPlot() const;																																		// Exposed to Python
	CvPlot* getPathEndTurnPlot() const;																																	// Exposed to Python
/*************************************************************************************************/
/**	K-mod merger								16/02/12								Snarko	**/
/**																								**/
/**					Merging in features of K-mod, most notably the pathfinder					**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	bool generatePath( const CvPlot* pFromPlot, const CvPlot* pToPlot, int iFlags = 0, bool bReuse = false, int* piPathTurns = NULL) const;	// Exposed to Python
	void resetPath();																																										// Exposed to Python
/**								----  End Original Code  ----									**/
	bool generatePath(const CvPlot* pFromPlot, const CvPlot* pToPlot, int iFlags = 0, bool bReuse = false, int* piPathTurns = NULL, int iMaxPath = -1) const; // Exposed to Python (K-mod added iMaxPath)
	void resetPath() const;					// Exposed to Python
/*************************************************************************************************/
/**	K-mod merger								END												**/
/*************************************************************************************************/

	DllExport void clearUnits();
	DllExport bool addUnit(CvUnit* pUnit, bool bMinimalChange);
	void removeUnit(CvUnit* pUnit);
	void mergeIntoGroup(CvSelectionGroup* pSelectionGroup);
	CvSelectionGroup* splitGroup(int iSplitSize, CvUnit* pNewHeadUnit = NULL, CvSelectionGroup** ppOtherGroup = NULL);

	DllExport CLLNode<IDInfo>* deleteUnitNode(CLLNode<IDInfo>* pNode);
	DllExport CLLNode<IDInfo>* nextUnitNode(CLLNode<IDInfo>* pNode) const;
	DllExport int getNumUnits() const;																												// Exposed to Python
	DllExport int getUnitIndex(CvUnit* pUnit, int maxIndex = -1) const;
	DllExport CLLNode<IDInfo>* headUnitNode() const;
	DllExport CvUnit* getHeadUnit() const;
	DllExport CvUnit* getUnitAt(int index) const;
	UnitAITypes getHeadUnitAI() const;
	PlayerTypes getHeadOwner() const;
	TeamTypes getHeadTeam() const;

	void clearMissionQueue();																																	// Exposed to Python
	DllExport int getLengthMissionQueue() const;																											// Exposed to Python
	MissionData* getMissionFromQueue(int iIndex) const;																							// Exposed to Python
	void insertAtEndMissionQueue(MissionData mission, bool bStart = true);
	CLLNode<MissionData>* deleteMissionQueueNode(CLLNode<MissionData>* pNode);
	DllExport CLLNode<MissionData>* nextMissionQueueNode(CLLNode<MissionData>* pNode) const;
	CLLNode<MissionData>* prevMissionQueueNode(CLLNode<MissionData>* pNode) const;
	DllExport CLLNode<MissionData>* headMissionQueueNode() const;
	CLLNode<MissionData>* tailMissionQueueNode() const;
	int getMissionType(int iNode) const;																														// Exposed to Python
	int getMissionData1(int iNode) const;																														// Exposed to Python
	int getMissionData2(int iNode) const;																														// Exposed to Python

	// for serialization
	virtual void read(FDataStreamBase* pStream);
	virtual void write(FDataStreamBase* pStream);

	virtual void AI_init() = 0;
	virtual void AI_reset() = 0;
	virtual void AI_separate() = 0;
/*************************************************************************************************/
/**	Improved spell usage				11/05/10				Moved/modified by Snarko		**/
/**								Potentially improves AI Spell Useage							**/
/**							Potentially improves AI Range Damage Useage							**/
/**					Moved from CvPlayerAI::AI_movementPriority to prevent CtD					**/
/*************************************************************************************************/
	virtual bool AI_PreUpdate() = 0;
/*************************************************************************************************/
/**	Improved spell usage					END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	Tweak							05/05/11								Snarko				**/
/**			Making ranged attacks cost a movement point and adjusting the AI.					**/
/*************************************************************************************************/
	virtual bool AI_doRangedAttackPost() = 0;
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/
	virtual bool AI_update() = 0;
	virtual int AI_attackOdds(const CvPlot* pPlot, bool bPotentialEnemy) const = 0;
	virtual CvUnit* AI_getBestGroupAttacker(const CvPlot* pPlot, bool bPotentialEnemy, int& iUnitOdds, bool bForce = false, bool bNoBlitz = false) const = 0;
	virtual CvUnit* AI_getBestGroupSacrifice(const CvPlot* pPlot, bool bPotentialEnemy, bool bForce = false, bool bNoBlitz = false) const = 0;

	virtual int AI_compareStacks(const CvPlot* pPlot, bool bPotentialEnemy, bool bCheckCanAttack = false, bool bCheckCanMove = false) const = 0;
	virtual int AI_sumStrength(const CvPlot* pAttackedPlot = NULL, DomainTypes eDomainType = NO_DOMAIN, bool bCheckCanAttack = false, bool bCheckCanMove = false) const = 0;

	virtual void AI_queueGroupAttack(int iX, int iY) = 0;
	virtual void AI_cancelGroupAttack() = 0;
	virtual bool AI_isGroupAttack() = 0;

	virtual bool AI_isControlled() = 0;
	virtual bool AI_isDeclareWar(const CvPlot* pPlot = NULL) = 0;
	virtual CvPlot* AI_getMissionAIPlot() = 0;
	virtual bool AI_isForceSeparate() = 0;
	virtual void AI_makeForceSeparate() = 0;
/*************************************************************************************************/
/**	K-mod merger								16/02/12								Snarko	**/
/**																								**/
/**					Merging in features of K-mod, most notably the pathfinder					**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	virtual MissionAITypes AI_getMissionAIType() = 0;
/**								----  End Original Code  ----									**/
	//virtual MissionAITypes AI_getMissionAIType() = 0;
	virtual MissionAITypes AI_getMissionAIType() const = 0; // K-Mod
/*************************************************************************************************/
/**	K-mod merger								END												**/
/*************************************************************************************************/
	virtual void AI_setMissionAI(MissionAITypes eNewMissionAI, CvPlot* pNewPlot, CvUnit* pNewUnit) = 0;
	virtual CvUnit* AI_getMissionAIUnit() = 0;
	virtual CvUnit* AI_ejectBestDefender(CvPlot* pTargetPlot) = 0;
	virtual void AI_separateNonAI(UnitAITypes eUnitAI) = 0;
	virtual void AI_separateAI(UnitAITypes eUnitAI) = 0;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/02/09                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
	virtual void AI_separateImpassable() = 0;
	virtual void AI_separateEmptyTransports() = 0;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	virtual bool AI_isFull() = 0;

/*************************************************************************************************/
/**	Alertness								11/30/08	Written: Pep		Imported: Xienwolf	**/
/**																								**/
/**			Prevents annoying accidental attacks when moving into non-visible tiles				**/
/*************************************************************************************************/
	bool m_lastPlotVisible;
	void setLastPathPlotVisibility(bool eVisible);
	bool isLastPathPlotVisible();
	bool m_lastPlotRevealed;
	void setLastPathPlotRevealed(bool eReveal);
	bool isLastPathPlotRevealed();
/*************************************************************************************************/
/**	Alertness								END													**/
/*************************************************************************************************/
protected:
	// WARNING: adding to this class will cause the civ4 exe to crash

	int m_iID;
	int m_iMissionTimer;

	bool m_bForceUpdate;

	PlayerTypes m_eOwner;
	ActivityTypes m_eActivityType;
	AutomateTypes m_eAutomateType;

	CLinkList<IDInfo> m_units;

	CLinkList<MissionData> m_missionQueue;
	std::vector<CvUnit *> m_aDifferentUnitCache;
	bool m_bIsBusyCache;

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
	bool m_bIsStrandedCache;
	bool m_bIsStrandedCacheValid;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	void activateHeadMission();
	void deactivateHeadMission();

	bool sentryAlert() const;
/*************************************************************************************************/
/**	K-mod merger								16/02/12								Snarko	**/
/**																								**/
/**					Merging in features of K-mod, most notably the pathfinder					**/
/*************************************************************************************************/
public:
	static KmodPathFinder path_finder; // K-Mod! I'd rather this not be static, but I can't do that here.
/*************************************************************************************************/
/**	K-mod merger								END												**/
/*************************************************************************************************/
};

#endif
