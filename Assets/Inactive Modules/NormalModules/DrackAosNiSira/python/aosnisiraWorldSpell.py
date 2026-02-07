import CvUtil
from CvPythonExtensions import *
import PyHelpers
PyPlayer = PyHelpers.PyPlayer

gc = CyGlobalContext()

import CustomFunctions
cf = CustomFunctions.CustomFunctions()

def spellNaturesFury(pCaster):
	py = PyPlayer(pCaster.getOwner())
	iEmpowerI = gc.getInfoTypeForString('PROMOTION_EMPOWER1')
	iEmpowerII = gc.getInfoTypeForString('PROMOTION_EMPOWER2')
	iEmpowerIII = gc.getInfoTypeForString('PROMOTION_EMPOWER3')
	iEmpowerIV = gc.getInfoTypeForString('PROMOTION_EMPOWER4')
	iEmpowerV = gc.getInfoTypeForString('PROMOTION_EMPOWER5')

	for pUnit in py.getUnitList():
		pUnit.setHasPromotion(iEmpowerI, True)
		pUnit.setHasPromotion(iEmpowerII, True)
		pUnit.setHasPromotion(iEmpowerIII, True)
		pUnit.setHasPromotion(iEmpowerIV, True)
		pUnit.setHasPromotion(iEmpowerV, True)
