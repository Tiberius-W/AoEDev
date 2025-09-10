## MODULAR PYTHON EXAMPLE
## ArcticNightWolf 29-may-2010
## ArcticNightWolf@gmail.com

from CvPythonExtensions import *

import PyHelpers

import FoxDebug
import FoxTools
import time
import CustomFunctions
from BasicFunctions import *

gc = CyGlobalContext()
getInfoType = gc.getInfoTypeForString
PyPlayer = PyHelpers.PyPlayer
cf = CustomFunctions.CustomFunctions()


def onBeginGameTurn(self, argsList):
		'Called at the beginning of the end of each turn'
		iGameTurn = argsList[0]
		bPlayer = gc.getPlayer(gc.getDEMON_PLAYER())
		if (iGameTurn + 1) % (40- 5*CyGame().getGameSpeedType()) == 0 and not bPlayer.isHasFlag(gc.getInfoTypeForString('FLAG_DEAD_BADB')):
			iRnd = 4 - CyGame().getGameSpeedType()
			iBB = gc.getInfoTypeForString('IMPROVEMENT_BADBS_BLIZZARD')
			lBB = cf.findImprovements(iBB)
			if len(lBB) > 0:
				pPlotBB = lBB[0]
				iIce = gc.getInfoTypeForString('BONUS_MANA_ICE')
				iBl = gc.getInfoTypeForString('FEATURE_BLIZZARD')
				lCold = [gc.getInfoTypeForString('TERRAIN_TUNDRA'),gc.getInfoTypeForString('TERRAIN_SNOW'),gc.getInfoTypeForString('TERRAIN_GLACIER')]
				iBestValue = 0
				pBestPlot = -1
				for i in xrange (CyMap().numPlots()):
					pTargetPlot = CyMap().plotByIndex(i)
					if pTargetPlot == pPlotBB:
						continue
					if pTargetPlot.isPeak():
						continue
					if pTargetPlot.isWater():
						continue
					if pTargetPlot.getBonusType(-1) != -1:
						continue
					if pTargetPlot.isCity():
						continue
					iValue = 0
					iImp = pTargetPlot.getImprovementType()
					if iImp == -1:
						iValue += 1000
					elif gc.getImprovementInfo(iImp).isPermanent():
						continue
					if pTargetPlot.getTerrainType() in lCold:
						iValue += 400
					iValue += CyGame().getSorenRandNum(1000, "Badb move ")
					if not pTargetPlot.isOwned():
						iValue += 400
					if iValue > iBestValue:
						iBestValue = iValue
						pBestPlot = pTargetPlot
				if pBestPlot != -1:
					iBadb = gc.getInfoTypeForString('UNIT_BADB')
					for i in xrange(pPlotBB.getNumUnits()):
						pUnit = pPlotBB.getUnit(i)
						if iBadb in [pUnit.getUnitType()]:
							pUnit.setXY(pBestPlot.getX(), pBestPlot.getY(), False, True, True)
							break

					iBonusReal = pPlotBB.getRealBonusType()
					if iBonusReal == iIce:
						pPlotBB.setBonusType(-1)
					else:
						pPlotBB.setBonusType(iBonusReal)
				#	if (pPlotBB.getTempTerrainTimer()>0):
				#		pPlotBB.changeTempTerrainTimer(1-pPlotBB.getTempTerrainTimer())
				#	iReal = pPlotBB.getRealImprovementType()
				#	if iReal == iBB:
					pPlotBB.setImprovementType(-1)
					pPlotBB.setFeatureType(-1, 0)
				#	else:
				#		pPlotBB.setImprovementType(iReal)

					pBestPlot.setTempBonusType(iIce, iRnd)
					pBestPlot.setExploreNextTurn(pPlotBB.getExploreNextTurn())
					pPlotBB.setExploreNextTurn(0)

					pBestPlot.setImprovementType(iBB)

					pBestPlot.setBonusType(iIce)
					pBestPlot.setFeatureType(iBl, 0)
