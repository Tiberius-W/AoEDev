import CvUtil
from CvPythonExtensions import *
import PyHelpers
PyPlayer = PyHelpers.PyPlayer

gc = CyGlobalContext()

import CustomFunctions
cf = CustomFunctions.CustomFunctions()

def spellHolyMarch(pCaster):
	py = PyPlayer(pCaster.getOwner())
	iPhoenixBlood = gc.getInfoTypeForString('PROMOTION_IMMORTAL')

	for pUnit in py.getUnitList():
		pUnit.setHasPromotion(iPhoenixBlood, True)
