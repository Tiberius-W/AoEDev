
from CvPythonExtensions import *
import CvUtil
import Popup as PyPopup
import PyHelpers
import CvScreenEnums

PLUS1       = ((-1,0),(0,-1),(0,0),(0,1),(1,0)) # center tile plus above, under, left, right
ADJPLUS1    = ((-1,0),(0,-1),(0,1),(1,0)) # above, under, left, right
RANGE1      = ((-1,-1),(-1,0),(-1,1),(0,-1),(0,0),(0,1),(1,-1),(1,0),(1,1)) # 3x3 square = 9 tiles
SURROUND1   = [(-1,-1),(-1,0),(-1,1),(0,-1),(0,1),(1,-1),(1,0),(1,1)] # as RANGE1 without the center tile = the 8 tiles around AND it's a list of tuples, not a tuple of tuples (this allows shuffle)
RANGE2      = ((-1,-2),(-1,-1),(-1,0),(-1,1),(-1,2),(0,-2),(0,-1),(0,0),(0,1),(0,2),(1,-2),(1,-1),(1,0),(1,1),(1,2),(-2,-2),(-2,-1),(-2,0),(-2,1),(-2,2),(2,-2),(2,-1),(2,0),(2,1),(2,2)) # 5x5 square = 25 tiles
BFC         = ((-2,-1),(-2,0),(-2,1),(-1,-2),(-1,-1),(-1,0),(-1,1),(-1,2),(0,-2),(0,-1),(0,0),(0,1),(0,2),(1,-2),(1,-1),(1,0),(1,1),(1,2),(2,-1),(2,0),(2,1)) # Big Fat Cross = area of a standard city = 21 tiles
BFC2        = ((-2,-1),(-2,0),(-2,1),(-1,-2),(-1,2),(0,-2),(0,2),(1,-2),(1,2),(2,-1),(2,0),(2,1)) # the exterior part of the Big Fat Cross = 12 tiles, used for sidar mist
#  XXX
# X   X
# X   X
# X   X
#  XXX

def addBonus(iBonus, iNum, sIcon):
	getInfoType		= CyGlobalContext().getInfoTypeForString #Cause local variables are faster
	map 			= CyMap()
	game			= CyGame()
	randNum			= game.getSorenRandNum
	eBonus			= getInfoType(iBonus)

	listPlots = []
	plotByIndex = map.plotByIndex
	for i in xrange(map.numPlots()):
		pPlot = plotByIndex(i)
		if (pPlot.canHaveBonus(eBonus,True) and pPlot.getBonusType(-1) == -1 and not pPlot.isCity()):
			listPlots.append(i)
	if len(listPlots) > 0:
		addMsg 		  = CyInterface().addMessage
		iActivePlayer = game.getActivePlayer()
		for i in xrange(iNum):
			iRnd = randNum(len(listPlots), "Add Bonus")
			pPlot = plotByIndex(listPlots[iRnd])
			pPlot.setBonusType(eBonus)
			if sIcon != -1:
				addMsg(iActivePlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_RESOURCE_DISCOVERED",()),'AS2D_DISCOVERBONUS',1,sIcon,ColorTypes(8),pPlot.getX(),pPlot.getY(),True,True)


def addPopup(szText, sDDS):
	szTitle = CyGameTextMgr().getTimeStr(CyGame().getGameTurn(), False)
	popup = PyPopup.PyPopup(-1)
	popup.addDDS(sDDS, 0, 0, 128, 384)
	popup.addSeparator()
	popup.setHeaderString(szTitle)
	popup.setBodyString(szText)
	popup.launch(true, PopupStates.POPUPSTATE_IMMEDIATE)

def addPopupWB(szText, sDDS):
	szTitle = CyGameTextMgr().getTimeStr(CyGame().getGameTurn(), False)
	screen = CyGInterfaceScreen( "MainInterface", CvScreenEnums.MAIN_INTERFACE )
	xRes = screen.getXResolution()
	yRes = screen.getYResolution()
	popup = PyPopup.PyPopup(-1)
	popup.addDDS(sDDS, 0, 0, 500, 800)
	popup.addSeparator()
	popup.setHeaderString(szTitle)
	popup.setBodyString(szText)
	popup.setPosition((xRes - 840) / 2,(yRes - 640) / 2)
	popup.setSize(840, 640)
	popup.launch(true, PopupStates.POPUPSTATE_IMMEDIATE)

def addPlayerPopup(szText, iPlayer):
	popupInfo = CyPopupInfo()
	popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON)
	popupInfo.setText(szText)
	popupInfo.addPythonButton(CyTranslator().getText("TXT_KEY_POPUP_CLOSE", ()), "")
	popupInfo.addPopup(iPlayer)


# Ronkhar: "what happens if several players use the same civ? it looks like only the last one is selected"
def getCivilization(iCiv):
	gc = CyGlobalContext() #Cause local variables are faster
	i = -1
	getPlayer = gc.getPlayer
	for iPlayer in xrange(gc.getMAX_PLAYERS()):
		pPlayer = getPlayer(iPlayer)
		if pPlayer.getCivilizationType() == iCiv:
			i = iPlayer
	return i

def addUnit(iUnit, iPlayer):
	gc 				= CyGlobalContext() #Cause local variables are faster
	map 			= CyMap()
	randNum			= CyGame().getSorenRandNum

	if iPlayer < 0 or iPlayer > gc.getDEMON_PLAYER():
		iPlayer = gc.getORC_PLAYER()
	pBestPlot = -1
	iBestPlot = -1

	plotByIndex = map.plotByIndex
	for i in xrange(map.numPlots()):
		pPlot = plotByIndex(i)
		iPlot = -1
		if pPlot.isWater() == False:
			if pPlot.getNumUnits() == 0:
				if pPlot.isCity() == False:
					if pPlot.isImpassable() == False:
						iPlot = randNum(500, "Add Unit")
						iPlot = iPlot + (pPlot.area().getNumTiles() * 10)
						if pPlot.isBarbarian():
							iPlot = iPlot + 200
						if pPlot.isOwned():
							iPlot = iPlot / 2
						if iPlot > iBestPlot:
							iBestPlot = iPlot
							pBestPlot = pPlot
	if iBestPlot != -1:
		bPlayer = gc.getPlayer(iPlayer)
		newUnit = bPlayer.initUnit(iUnit, pBestPlot.getX(), pBestPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
		newUnit.finishMoves()
		return newUnit
		
def addUnitFixed(caster, iUnit, iPlayer):
	gc = CyGlobalContext() #Cause local variables are faster
	if iPlayer < 0 or iPlayer > gc.getDEMON_PLAYER():
		iPlayer = gc.getORC_PLAYER()
	pPlot = caster.plot()
	pNewPlot = findClearPlot(-1, pPlot)
	if pNewPlot != -1:
		pPlayer = gc.getPlayer(iPlayer)
		newUnit = pPlayer.initUnit(iUnit, pNewPlot.getX(), pNewPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
		return newUnit
	return -1
def plotsInRange( centerX, centerY, maxRange, minRange=None ):
	if minRange is None:
		minRange = 0
	for offsetX in xrange( -maxRange, maxRange + 1 ):
		plotX = centerX + offsetX
		for offsetY in xrange( -maxRange, maxRange + 1 ):
			plotY = centerY + offsetY
			distance = stepDistance( centerX, centerY, plotX, plotY )
			if minRange <= distance <= maxRange:
				yield ( plotX, plotY )

def plotsInCircularRange( centerX, centerY, maxRange, minRange=None ):
	if minRange is None:
		minRange = 0
	for offsetX in xrange( -maxRange, maxRange + 1 ):
		plotX = centerX + offsetX
		for offsetY in xrange( -maxRange, maxRange + 1 ):
			plotY = centerY + offsetY
			distance = plotDistance( centerX, centerY, plotX, plotY )
			if minRange <= distance <= maxRange:
				yield ( plotX, plotY )

def findClearPlot(pUnit, plot,iPlayer=-1):
	BestPlot = -1
	iBestPlot = 0
	if plot==None:
		return -1
	getPlot	= CyMap().plot
	iRange = 1
	if pUnit == -1:
		while(BestPlot==-1):
			for x, y in plotsInRange( plot.getX(), plot.getY(), iRange ):
				pPlot = getPlot(x, y)
				iCurrentPlot = 0
				if not pPlot.isNone():
					if pPlot.getNumUnits() == 0 or pPlot.getUnit(0).getOwner==iPlayer:
						if pPlot.isWater() == plot.isWater():
							if not pPlot.isPeak():
								if not pPlot.isCity():
									iCurrentPlot = CyGame().getSorenRandNum(5, "findClearPlot")
									if iCurrentPlot >= iBestPlot:
										BestPlot = pPlot
										iBestPlot = iCurrentPlot						
			if BestPlot == -1:
				iRange = iRange+1
			else:
				return BestPlot
	while(BestPlot==-1):
		for x, y in plotsInRange( pUnit.getX(), pUnit.getY(), iRange ):
			pPlot = getPlot(x, y)
			iCurrentPlot = 0
			if pPlot.getNumUnits() == 0:
				if pUnit.canMoveOrAttackInto(pPlot, False):
					iCurrentPlot = iCurrentPlot + 5
		
			if(pUnit.getOwner()==iPlayer or iPlayer==-1):		
				for i in range(pPlot.getNumUnits()):
					if pPlot.getUnit(i).getOwner() == pUnit.getOwner():
						if pUnit.canMoveOrAttackInto(pPlot, False):
							iCurrentPlot = iCurrentPlot + 15
				if pPlot.isCity():
					if pPlot.getPlotCity().getOwner() == pUnit.getOwner():
						iCurrentPlot = iCurrentPlot + 50
				if iCurrentPlot >= 1:
					iCurrentPlot = iCurrentPlot + CyGame().getSorenRandNum(5, "findClearPlot")
					if iCurrentPlot >= iBestPlot:
						BestPlot = pPlot
						iBestPlot = iCurrentPlot
			else:
				for i in range(pPlot.getNumUnits()):	
					if pPlot.getUnit(i).getOwner() == iPlayer:
						iCurrentPlot = iCurrentPlot + 15
				if pPlot.isCity():
					if pPlot.getPlotCity().getOwner() == iPlayer:
						iCurrentPlot = iCurrentPlot + 50
					else:
						iCurrentPlot = iCurrentPlot - 50
				if iCurrentPlot >= 1:
					iCurrentPlot = iCurrentPlot + CyGame().getSorenRandNum(5, "findClearPlot")
					if iCurrentPlot >= iBestPlot:
						BestPlot = pPlot
						iBestPlot = iCurrentPlot
		if BestPlot == -1:
			iRange = iRange+1
		else:
			return BestPlot
	return BestPlot

def getUnitPlayerID(pUnit):
	gc = CyGlobalContext() #Cause local variables are faster
	pPlayer = gc.getPlayer(pUnit.getOwner())
	iID = pUnit.getID()
	iUnitID = -1
	for iUnit in xrange(pPlayer.getNumUnits()):
		pLoopUnit = pPlayer.getUnit(iUnit)
		if pLoopUnit.getID() == iID:
			iUnitID = iUnit
	return iUnitID

def grace(): # TODO Ronkhar write help
	gc 		= CyGlobalContext() #Cause local variables are faster
	game 	= CyGame()
	iGrace 	= 50 * (int(game.getGameSpeedType()) + 1)
	iDiff 	= gc.getNumHandicapInfos() + 1 - int(game.getHandicapType())
	iGrace 	= iGrace * iDiff
	iGrace 	= game.getSorenRandNum(iGrace, "grace") + iGrace
	if iGrace > game.getGameTurn(): return True
	return False

def scale(iGameTurns):
	#Scales things by gamespeed; Longer game speeds yield larger amounts. Use for timed effects (Temp Terrain, for instance)
	gc 			 	= CyGlobalContext() #Cause local variables are faster
	getInfoType	 	= gc.getInfoTypeForString
	gameSpeedInfo 	= gc.getGameSpeedInfo
	'scales iGameTurns (given in normal game speed) by the chosen game speed'
	iNumTurnsChosenSpeed = gameSpeedInfo(CyGame().getGameSpeedType()).getGameTurnInfo(0).iNumGameTurnsPerIncrement
	iNumTurnsNormalSpeed = gameSpeedInfo(getInfoType("GAMESPEED_NORMAL")).getGameTurnInfo(0).iNumGameTurnsPerIncrement
	fScalingFactor = float(iNumTurnsChosenSpeed) / float(iNumTurnsNormalSpeed)
	iGameTurnsScaled = int(fScalingFactor * iGameTurns)
	return max(1, iGameTurnsScaled)

def scaleInverse(iGameTurns):
	#Scales things by gamespeed; Longer game speeds yield smaller amounts. Use for incremental effects (Spawn Functions, for instance)
	gc 			 	= CyGlobalContext() #Cause local variables are faster
	getInfoType	 	= gc.getInfoTypeForString
	gameSpeedInfo 	= gc.getGameSpeedInfo
	iNumTurnsChosenSpeed = gameSpeedInfo(CyGame().getGameSpeedType()).getGameTurnInfo(0).iNumGameTurnsPerIncrement
	iNumTurnsNormalSpeed = gameSpeedInfo(getInfoType("GAMESPEED_NORMAL")).getGameTurnInfo(0).iNumGameTurnsPerIncrement
	fScalingFactor = float(iNumTurnsNormalSpeed) / float(iNumTurnsChosenSpeed)
	iGameTurnsScaled = int(fScalingFactor * iGameTurns)
	return max(1, iGameTurnsScaled)

def getLeader(iLeader):
	gc = CyGlobalContext() #Cause local variables are faster
	i = -1
	for iPlayer in xrange(gc.getMAX_PLAYERS()):
		pPlayer = gc.getPlayer(iPlayer)
		if pPlayer.getLeaderType() == iLeader:
			i = iPlayer
	return i

def getOpenPlayer():
	gc = CyGlobalContext() #Cause local variables are faster
	i = -1
	for iPlayer in xrange(gc.getMAX_PLAYERS()):
		pPlayer = gc.getPlayer(iPlayer)
		if (pPlayer.isEverAlive() == False and i == -1): # TODO Ronkhar : probably bad programming -> use break instead
			i = iPlayer
	return i

def doFear(pVictim, pPlot, pCaster, bResistable):
	gc = CyGlobalContext() #Cause local variables are faster
	getInfoType	= gc.getInfoTypeForString #Cause local variables are faster
	randNum 	= CyGame().getSorenRandNum
	if pVictim.isImmuneToFear():
		return False
	if bResistable:
		if randNum(100, "Roar Fear Check") < pVictim.getResistChance(pCaster, getInfoType('SPELL_ROAR')):
			return False
	iX = pVictim.getX()
	iY = pVictim.getY()
	pBestPlot = -1
	iBestPlot = 0
	plot 	= CyMap().plot
	pVictimPlot=plot(iX,iY)
	if pVictimPlot.isCity(): 
		if randNum(100,"Roar City Check") <80:
			return False
	canMove = pVictim.canMoveOrAttackInto
	iOwner 	= pVictim.getOwner()
	iPlotX 	= pPlot.getX()
	iPlotY 	= pPlot.getY()
	for iiX in xrange(iX-1, iX+2, 1):
		for iiY in xrange(iY-1, iY+2, 1):
			pLoopPlot = plot(iiX,iiY)
			if not pLoopPlot.isNone():
				if not pLoopPlot.isVisibleEnemyUnit(iOwner):
					if canMove(pLoopPlot, False):
						if (abs(pLoopPlot.getX() - iPlotX)>1) or (abs(pLoopPlot.getY() - iPlotY)>1):
							iRnd = randNum(500, "Fear Scatter choose Plot")
							if iRnd > iBestPlot:
								iBestPlot = iRnd
								pBestPlot = pLoopPlot
	if pBestPlot != -1:
		pVictim.setXY(pBestPlot.getX(), pBestPlot.getY(), False, true, true)
		return True
	return False

# FF: Added by Jean Elcard 14/01/2009 (speed tweak)
def rebuildGraphics():
	COMPLETE_MAP_REBUILD_THRESHOLD = 0.06
	map = CyMap()
	if map.isNeedsRebuilding():
		numPlots = map.numPlots()
		plotIndex = map.plotByIndex
		pPlotsToRebuild = []
		append = pPlotsToRebuild.append
		for i in xrange(numPlots):
			pPlot = plotIndex(i)
			if pPlot.isNeedsRebuilding():
				append(pPlot)
		if len(pPlotsToRebuild) > int(COMPLETE_MAP_REBUILD_THRESHOLD * numPlots):
			try:
				map.rebuildGraphics()
			except MemoryError:
				rebuildPlots(pPlotsToRebuild)
				raise MemoryError("Your system is running out of memory! Fall Further had to use a slower plot update method.")
		else:
			rebuildPlots(pPlotsToRebuild)

def rebuildPlots(pPlotsToRebuild):
	'Rebuilds each plot individually.'
	for pPlot in pPlotsToRebuild:
		pPlot.rebuildGraphics()
	CyMap().setNeedsRebuilding(False)

def startWar(iPlayer, iOtherPlayer, iWarPlan):
	gc 		= CyGlobalContext() #Cause local variables are faster
	iTeam 	= gc.getPlayer(iPlayer).getTeam()
	iOtherTeam 	= gc.getPlayer(iOtherPlayer).getTeam()
	pTeam 	= gc.getTeam(iTeam)
	pOtherTeam 	= gc.getTeam(iOtherTeam)
	if pTeam.isAlive():
		if pOtherTeam.isAlive():
			if not pTeam.isAtWar(iOtherTeam):
				if iTeam != iOtherTeam:
					if pTeam.isHasMet(iOtherTeam):
						if not pTeam.isPermanentWarPeace(iOtherTeam):
							pTeam.declareWar(iOtherTeam, False, iWarPlan)


def showUniqueImprovements(iPlayer):
	gc 			= CyGlobalContext() #Cause local variables are faster
	getImprov	= gc.getImprovementInfo
	map			= CyMap()
	plotByIndex = map.plotByIndex
	pPlayer	 	= gc.getPlayer(iPlayer)
	iTeam 		= pPlayer.getTeam()
	for i in xrange(map.numPlots()):
		pPlot = plotByIndex(i)
		iImprovement = pPlot.getImprovementType()
		if iImprovement != -1:
			if getImprov(iImprovement).isUnique():
				pPlot.setRevealed(iTeam, True, False, TeamTypes.NO_TEAM)


def placeTreasure(iPlayer, iUnit):
	gc 			= CyGlobalContext() #Cause local variables are faster
	map			= CyMap()
	plotByIndex = map.plotByIndex
	game		= CyGame()
	getRandNum	= game.getSorenRandNum
	pPlayer 	= gc.getPlayer(iPlayer)
	pBestPlot 	= -1
	iBestPlot 	= -1
	for i in xrange(map.numPlots()):
		pPlot = plotByIndex(i)
		iPlot = -1
		if not pPlot.isWater():
			if pPlot.getNumUnits() == 0:
				if not pPlot.isCity():
					if not pPlot.isImpassable():
						iPlot = getRandNum(1000, "Add Unit")
						if pPlot.area().getNumTiles() < 8:
							iPlot += 1000
						if not pPlot.isOwned():
							iPlot += 1000
						if iPlot > iBestPlot:
							iBestPlot = iPlot
							pBestPlot = pPlot
	if iBestPlot != -1:
		newUnit = pPlayer.initUnit(iUnit, pBestPlot.getX(), pBestPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.DIRECTION_SOUTH)
		CyInterface().addMessage(iPlayer,True,25,CyTranslator().getText("TXT_KEY_MESSAGE_EXPLORE_LAIR_TREASURE",()),'',1,'Art/Interface/Buttons/Equipment/Treasure.dds',ColorTypes(8),newUnit.getX(),newUnit.getY(),True,True)
		if (iPlayer == game.getActivePlayer()):
			iTeam = pPlayer.getTeam()
			signText = CvUtil.convertToStr(CyTranslator().getText("TXT_KEY_EQUIPMENT_TREASURE", ()))
			pBestPlot.setRevealed(iTeam, True, False, TeamTypes.NO_TEAM)
			CyCamera().JustLookAtPlot(pBestPlot)
			CyEngine().addSign(pBestPlot, iPlayer, signText)

def wchoice(weighted_choices, log_message='Log message' ):
	objects, frequences = zip( *weighted_choices )
	addedFreq = []
	lastSum = 0
	for freq in frequences:
		lastSum += freq
		addedFreq.append(lastSum)
	def choosing_function():
		ballNumber = CyGame().getSorenRandNum(lastSum, log_message)
		for index, subTotal in enumerate( addedFreq ):
			if subTotal > ballNumber:
				return objects[ index ]
	return choosing_function