## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
## 2013-08-15 Modified by Ronkhar, using "More Detailed Civilopedia" by denev
from CvPythonExtensions import *
import CvUtil
import ScreenInput
import CvScreenEnums

# globals
gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class CvPediaUnit:
	"Civilopedia Screen for Units"

	def __init__(self, main):
		self.iUnit = -1
		self.top = main

		#self.BUTTON_SIZE = 64 # did not find it elsewhere in this file. Useless?
		self.W_WORLD_UNIT = 384 # banner width for world units (heroes, ...)
		self.H_WORLD_UNIT = 128 # banner height for world units (heroes, ...)


		self.FILTERS =	[
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNFILTERED", ())',		# Name of the filter as it appears in dropdown menu.  Needs to be unique for this screen
				"Purpose" : "Clears all currently active filters",		# Description for the sanity of those who modify this file, appears and is used nowhere else
				"TypeHardcoded" : False,		# If this is True, then only those items in the HardcodeList will pass the check
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,		# If this is True, then only those items in the HardcodeList will pass the check
				"ClassHardcodeList" : [],
				"Value to Check" : 'None',	# Note that eUnit is the UnitInfo object being tested
				"Desired Result" : 'None',
				"Primary" : True,
				"Civ Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_CONCEPT_HEROES", ())',
				"Purpose" : "World Units only",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'isWorldUnitClass(eUnit.getUnitClassType())',
				"Desired Result" : 'True',
				"Primary" : True,
				"Civ Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_NATIONAL", ())',
				"Purpose" : "National Limited Units only (T4)",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'isNationalUnitClass(eUnit.getUnitClassType()) or isTeamUnitClass(eUnit.getUnitClassType())',
				"Desired Result" : 'True',
				"Primary" : True,
				"Civ Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_BUILDING_UNBUILDABLE", ())',
				"Purpose" : "Simple approach, but should cover most all cases",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'eUnit.getProductionCost() < 0 or eUnit.getMinLevel() > 0',
				"Desired Result" : 'True',
				"Primary" : True,
				"Civ Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_BUILDING_BUILDABLE", ())',
				"Purpose" : "Simple approach, but should cover most all cases",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'eUnit.getProductionCost() < 0 or eUnit.getMinLevel() > 0',
				"Desired Result" : 'False',
				"Primary" : True,
				"Civ Specific" : False,
			},
			# {
			# 	"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_STARTING", ())',
			# 	"Purpose" : "Test for the Hardcoded Unit List primarily",
			# 	"TypeHardcoded" : False,
			# 	"TypeHardcodeList" : [],
			# 	"ClassHardcoded" : True,
			# 	"ClassHardcodeList" : [
			# 		'UNITCLASS_SETTLER',
			# 		'UNITCLASS_SCOUT',
			# 		'UNITCLASS_WARRIOR'],
			# 	"Value to Check" : 'None',
			# 	"Desired Result" : 'None',
			# 	"Primary" : True,
			# 	"Civ Specific" : False,
			# },
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_DEFAULT", ())',
				"Purpose" : "All Units which don't require a certain Civ",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getUnitClassInfo(eUnit.getUnitClassType()).getDefaultUnitIndex() != iUnit or gc.getUnitClassInfo(eUnit.getUnitClassType()).isUnique()',
				"Desired Result" : 'False',
				"Primary" : True,
				"Civ Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_CIV_SPECIFIC", ())',
				"Purpose" : "All Units which require a certain Civ",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getUnitClassInfo(eUnit.getUnitClassType()).getDefaultUnitIndex() != iUnit or gc.getUnitClassInfo(eUnit.getUnitClassType()).isUnique()',
				"Desired Result" : 'True',
				"Primary" : True,
				"Civ Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_TIER_1", ())',
				"Purpose" : "Units from Tier 1",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'eUnit.getTier() == 1',
				"Desired Result" : 'True',
				"Primary" : True,
				"Civ Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_TIER_2", ())',
				"Purpose" : "Units from Tier 2",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'eUnit.getTier() == 2',
				"Desired Result" : 'True',
				"Primary" : True,
				"Civ Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_TIER_3", ())',
				"Purpose" : "Units from Tier 3",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'eUnit.getTier() == 3',
				"Desired Result" : 'True',
				"Primary" : True,
				"Civ Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_TIER_4", ())',
				"Purpose" : "Units from Tier 4",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'eUnit.getTier() == 4',
				"Desired Result" : 'True',
				"Primary" : True,
				"Civ Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_AMURITE", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_AMURITES")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_ARCHOS", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_ARCHOS")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_AUSTRIN", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_AUSTRIN")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_BALSERAPHS", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_BALSERAPHS")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_BANNOR", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_BANNOR")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_CALABIM", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_CALABIM")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_CHIMAR", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_GOBLIN")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_CHISLEV", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_CHISLEV")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_CLAN", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_CLAN_OF_EMBERS")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_CUALLI", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_CUALLI")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_DOVIELLO", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_DOVIELLO")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_DURAL", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_DURAL")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_ELOHIM", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_ELOHIM")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_FROZEN", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_FROZEN")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_GRIGORI", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_GRIGORI")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_HAMSTALFAR", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_HAMSTALFAR")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_HIPPUS", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_HIPPUS")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_ILLIAN", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_ILLIANS")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_INFERNAL", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_INFERNAL")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_JOTNAR", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_JOTNAR")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_KHAZAD", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_KHAZAD")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_KURIOTATE", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_KURIOTATES")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_LANUN", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_LANUN")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_LJOSALFAR", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_LJOSALFAR")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_LUCHUIRP", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_LUCHUIRP")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_MALAKIM", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_MALAKIM")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_MAZATL", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_MAZATL")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_MECHANOS", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_MECHANOS")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_MEKARA", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_MEKARA")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_MERCURIAN", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_MERCURIANS")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_SCIONS", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_SCIONS")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_SHEAIM", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_SHEAIM")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_SIDAR", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_SIDAR")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_SVARTALFAR", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_SVARTALFAR")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_DTESH", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_DTESH")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_BARBARIAN", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_ORC")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_ANIMAL", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_ANIMAL")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_DEMONIC", ())',
				"Purpose" : "Civ Specific Cases, massive PITA to just write all these in...",
				"TypeHardcoded" : False,
				"TypeHardcodeList" : [],
				"ClassHardcoded" : False,
				"ClassHardcodeList" : [],
				"Value to Check" : 'gc.getCivilizationInfo(gc.getInfoTypeForString("CIVILIZATION_DEMON")).getCivilizationUnits(eUnit.getUnitClassType()) == iUnit',
				"Desired Result" : 'True',
				"Primary" : False,
				"Civ Specific" : True,
			},
			]

		# List the filters which you want to be available initially, or self.FILTERS to have all of them available from the start
		self.ALLOWED_FILTERS = []
		for i, filter in enumerate(self.FILTERS):
			if self.FILTERS[i]["Primary"]:
				self.ALLOWED_FILTERS.append(self.FILTERS[i])
		self.CURRENT_FILTER = self.FILTERS[0]

		self.SORTS =	[
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_ALPHABETICAL", ())',
				"Purpose" : "Default Sorting Method",
				"Value to Sort" : 'eUnit.getDescription()',
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_COST_PRODUCTION", ())',
				"Purpose" : "Be nice if all sorts were so easy to decide on",
				"Value to Sort" : '-eUnit.getProductionCost()',
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_ATTACK", ())',
				"Purpose" : "Though this doesn't account for affinity and other such bonuses",
				"Value to Sort" : '-eUnit.getCombat()',
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_DEFENSE", ())',
				"Purpose" : "Again, doesn't count for starting promotion bonuses and other such goodies",
				"Value to Sort" : '-eUnit.getCombatDefense()',
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_RANGED", ())',
				"Purpose" : "But these are still nice, even without the extra checks IMO",
				"Value to Sort" : '-eUnit.getAirCombat()',
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_COLLATERAL", ())',
				"Purpose" : "Maybe sorting by limit would be better here, but could do that ALSO if someone wanted...",
				"Value to Sort" : '-eUnit.getCollateralDamage()',
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_UNITCOMBAT", ())',
				"Purpose" : "To pair with filtering by UnitCombat, though useless when together ;)",
				"Value to Sort" : 'eUnit.getUnitCombatType()',
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_TIER", ())',
				"Purpose" : "Sort by unit tier",
				"Value to Sort" : 'eUnit.getTier()',
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_XML_ORDER", ())',
				"Purpose" : "Default, unsorted method",
				"Value to Sort" : None,
			},
			]

		# List the sorts which you want to be available initially, or self.SORTS to have all of them available from the start
		self.ALLOWED_SORTS = self.SORTS
		self.CURRENT_SORT = self.SORTS[0]
		self.SUB_SORT = self.SORTS[0]

	# Screen construction function
	def interfaceScreen(self, iUnit):
		self.iUnit = iUnit

		self.top.deleteAllWidgets()

		screen = self.top.getScreen()

		bNotActive = (not screen.isActive())
		if bNotActive:
			self.top.setPediaCommonWidgets()

		#Filter/Sort dropdowns
		self.top.FILTER_DROPDOWN_ID = self.top.getNextWidgetName()
		screen.addDropDownBoxGFC(self.top.FILTER_DROPDOWN_ID, self.top.X_FILTER_DROPDOWN, self.top.Y_DROPDOWN, self.top.W_DROPDOWN, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
		for i, filter in enumerate(self.ALLOWED_FILTERS):
			screen.addPullDownString(self.top.FILTER_DROPDOWN_ID, eval(filter["name"]), i, i, filter == self.CURRENT_FILTER )

		self.top.SORT_DROPDOWN_ID = self.top.getNextWidgetName()
		screen.addDropDownBoxGFC(self.top.SORT_DROPDOWN_ID, self.top.X_SORT_DROPDOWN, self.top.Y_DROPDOWN, self.top.W_DROPDOWN, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
		for i, sort in enumerate(self.ALLOWED_SORTS):
			screen.addPullDownString(self.top.SORT_DROPDOWN_ID, eval(sort["name"]), 1, 1, sort == self.CURRENT_SORT )

		# Header...
		szHeader = u"<font=4b>" + gc.getUnitInfo(self.iUnit).getDescription().upper() + u"</font>"
		TopPage = CivilopediaPageTypes.CIVILOPEDIA_PAGE_UNIT
		screen.setLabel(self.top.getNextWidgetName(), "Background", szHeader, CvUtil.FONT_CENTER_JUSTIFY, self.top.X_SCREEN, self.top.Y_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, TopPage, iUnit)

		# Top
		screen.setText(self.top.getNextWidgetName(), "Background", self.top.MENU_TEXT, CvUtil.FONT_LEFT_JUSTIFY, self.top.X_MENU, self.top.Y_MENU, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_PEDIA_MAIN, TopPage, -1)

		if self.top.iLastScreen	!= CvScreenEnums.PEDIA_UNIT or bNotActive:
			self.placeLinks(true)
			self.top.iLastScreen = CvScreenEnums.PEDIA_UNIT
		else:
			self.placeLinks(false)

		self.placeIcon()
		self.placeStats()
		self.placeRequires()
		self.placeUpgradesTo()
		self.placeSpecial()
		self.placeAnimation()
		self.placePromotions()
		self.placeHistory()
		self.placeStrategy()

	def placeIcon(self):
		screen = self.top.getScreen()
		self.X_ICON = self.top.EXT_SPACING
		self.Y_ICON = self.top.H_TOP_BAR + self.top.INT_SPACING
		self.W_ICON = 150
		self.H_ICON = 150
		self.ICON_SIZE = 64
		screen.addPanel(self.top.getNextWidgetName(), "", "", false, false,
			self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_BLUE50)
		szButton = gc.getUnitInfo(self.iUnit).getButton()
		if self.top.iActivePlayer != -1:
			szButton = gc.getPlayer(self.top.iActivePlayer).getUnitButton(self.iUnit)
		screen.addDDSGFC(self.top.getNextWidgetName(), szButton,
			self.X_ICON + self.W_ICON/2 - self.ICON_SIZE/2, self.Y_ICON + self.H_ICON/2 - self.ICON_SIZE/2, self.ICON_SIZE, self.ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1 )

	# Place strength/movement
	def placeStats(self):
		screen = self.top.getScreen()
		self.X_UNIT_PANE = self.X_ICON + self.W_ICON + self.top.INT_SPACING
		self.Y_UNIT_PANE = self.top.H_TOP_BAR + self.top.INT_SPACING
		self.W_UNIT_PANE = self.W_WORLD_UNIT - self.W_ICON - self.top.INT_SPACING
		self.H_UNIT_PANE = 190

		screen.addPanel( self.top.getNextWidgetName(), "", "", False, False,
			self.X_UNIT_PANE, self.Y_UNIT_PANE, self.W_UNIT_PANE, self.H_UNIT_PANE, PanelStyles.PANEL_STYLE_BLUE50)

		panelName = self.top.getNextWidgetName()
		self.X_STATS_PANE = self.X_UNIT_PANE + self.top.HM_TEXT
		self.Y_STATS_PANE = self.Y_UNIT_PANE + 16
		self.W_STATS_PANE = self.W_UNIT_PANE - 2*self.top.HM_TEXT
		self.H_STATS_PANE = self.H_UNIT_PANE - self.top.VM_TEXT
		# Unit combat group
		iCombatType = gc.getUnitInfo(self.iUnit).getUnitCombatType()
		if (iCombatType != -1):
			screen.setImageButton(self.top.getNextWidgetName(), gc.getUnitCombatInfo(iCombatType).getButton(), self.X_STATS_PANE, self.Y_STATS_PANE, 32, 32, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT_COMBAT, iCombatType, 0)
			screen.setText(self.top.getNextWidgetName(), "", u"<font=3>" + gc.getUnitCombatInfo(iCombatType).getDescription() + u"</font>", CvUtil.FONT_LEFT_JUSTIFY, self.X_STATS_PANE + 37, self.Y_STATS_PANE + 3, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT_COMBAT, iCombatType, 0)

		screen.addListBoxGFC(panelName, "", self.X_STATS_PANE, self.Y_STATS_PANE + 32, self.W_STATS_PANE, self.H_STATS_PANE, TableStyles.TABLE_STYLE_EMPTY)
		screen.enableSelect(panelName, False)

		if (gc.getUnitInfo(self.iUnit).getAirRange() > 0):
			szRanged = localText.getText("TXT_KEY_PEDIA_RANGED", ( gc.getUnitInfo(self.iUnit).getAirCombat(), ) )
			if (gc.getUnitInfo(self.iUnit).getAirCombatLimit() == 100):
				screen.appendListBoxStringNoUpdate(panelName, u"<font=3>" + szRanged.upper() + u"%c" % CyGame().getSymbolID(FontSymbols.RANGED_CHAR) + u"</font>", WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)
			else:
				screen.appendListBoxStringNoUpdate(panelName, u"<font=3>" + szRanged.upper() + u"%c<color=255,255,0>%d%%</color>" % (CyGame().getSymbolID(FontSymbols.RANGED_CHAR),gc.getUnitInfo(self.iUnit).getAirCombatLimit()) + u"</font>", WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)
		
		if (gc.getUnitInfo(self.iUnit).getMagicalPower() > 0):
			szRanged = localText.getText("TXT_KEY_PEDIA_MAGICAL_POWER", ( gc.getUnitInfo(self.iUnit).getMagicalPower(),) )
			symbol = gc.getBonusInfo(gc.getDefineINT("BONUS_MANA")).getChar()
			screen.appendListBoxStringNoUpdate(panelName, (u"<font=3>" + szRanged.upper() + u"%c" %symbol  + u"</font>"), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)
		
		if (gc.getUnitInfo(self.iUnit).getWorkRate() > 0):
			szRanged = localText.getText("TXT_KEY_PEDIA_WORK_RATE", ( gc.getUnitInfo(self.iUnit).getWorkRate(),) )
		#	symbol = gc.getBonusInfo(gc.getDefineINT("BONUS_MANA")).getChar()
			screen.appendListBoxStringNoUpdate(panelName, (u"<font=3>" + szRanged.upper() + u"</font>"), WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)
			
		iStrength = gc.getUnitInfo(self.iUnit).getCombat()

		szName = self.top.getNextWidgetName()

		if iStrength == gc.getUnitInfo(self.iUnit).getCombatDefense():
			szStrength = localText.getText("TXT_KEY_PEDIA_STRENGTH", ( iStrength, ) )
		else:
			szStrength = localText.getText("TXT_KEY_PEDIA_STRENGTH_DEFENSE", ( iStrength, gc.getUnitInfo(self.iUnit).getCombatDefense()) )

		if(gc.getUnitInfo(self.iUnit).getCombatLimit() == gc.getMAX_HIT_POINTS()):
			screen.appendListBoxStringNoUpdate(panelName, u"<font=3>" + szStrength.upper() + u"%c" % CyGame().getSymbolID(FontSymbols.STRENGTH_CHAR) + u"</font>", WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)
		else:
			screen.appendListBoxStringNoUpdate(panelName, u"<font=3>" + szStrength.upper() + u"%c<color=255,255,0>%d%%</color>" % (CyGame().getSymbolID(FontSymbols.STRENGTH_CHAR),gc.getUnitInfo(self.iUnit).getCombatLimit() / gc.getDefineINT("HIT_POINT_FACTOR")) + u"</font>", WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

		szName = self.top.getNextWidgetName()
		szMovement = localText.getText("TXT_KEY_PEDIA_MOVEMENT", ( gc.getUnitInfo(self.iUnit).getMoves(), ) )
		screen.appendListBoxStringNoUpdate(panelName, u"<font=3>" + szMovement.upper() + u"%c" % CyGame().getSymbolID(FontSymbols.MOVES_CHAR) + u"</font>", WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

		if (gc.getUnitInfo(self.iUnit).getProductionCost() >= 0 and not gc.getUnitInfo(self.iUnit).isFound()):
			szName = self.top.getNextWidgetName()
			if self.top.iActivePlayer == -1:
				szCost = localText.getText("TXT_KEY_PEDIA_COST", ((gc.getUnitInfo(self.iUnit).getProductionCost() * gc.getDefineINT("UNIT_PRODUCTION_PERCENT"))/100,))
			else:
				szCost = localText.getText("TXT_KEY_PEDIA_COST", ( gc.getActivePlayer().getUnitProductionNeeded(self.iUnit), ) )
			screen.appendListBoxStringNoUpdate(panelName, u"<font=3>" + szCost.upper() + u"%c" % gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getChar() + u"</font>", WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

		if (gc.getUnitInfo(self.iUnit).getAirRange() > 0):
			szName = self.top.getNextWidgetName()
			szRange = localText.getText("TXT_KEY_PEDIA_RANGE", ( gc.getUnitInfo(self.iUnit).getAirRange(), ) )
			screen.appendListBoxStringNoUpdate(panelName, u"<font=3>" + szRange.upper() + u"</font>", WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

		if (gc.getUnitInfo(self.iUnit).getExtraPerception() > 0):
			szName = self.top.getNextWidgetName()
			szRange = localText.getText("TXT_KEY_PEDIA_PERCEPTION", ( gc.getUnitInfo(self.iUnit).getExtraPerception(), ) )
			screen.appendListBoxStringNoUpdate(panelName, u"<font=3>" + szRange.upper() + u"</font>", WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

		screen.updateListBox(panelName)

	# Place prereqs (techs, resources)
	def placeRequires(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		self.X_PREREQ_PANE = self.top.EXT_SPACING
		self.Y_PREREQ_PANE = self.Y_UNIT_PANE + self.H_UNIT_PANE
		self.W_PREREQ_PANE = self.W_WORLD_UNIT
		self.H_PREREQ_PANE = self.top.H_BLUE50_PANEL

		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_REQUIRES", ()), "", false, true, self.X_PREREQ_PANE, self.Y_PREREQ_PANE, self.W_PREREQ_PANE, self.H_PREREQ_PANE, PanelStyles.PANEL_STYLE_BLUE50 )

		screen.attachLabel(panelName, "", "  ")

		# add tech buttons
		iPrereq = gc.getUnitInfo(self.iUnit).getPrereqAndTech()
		if (iPrereq >= 0):
			screen.attachImageButton( panelName, "", gc.getTechInfo(iPrereq).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iPrereq, 1, False )

		for j in range(gc.getDefineINT("NUM_UNIT_AND_TECH_PREREQS")):
			iPrereq = gc.getUnitInfo(self.iUnit).getPrereqAndTechs(j)
			if (iPrereq >= 0):
				screen.attachImageButton( panelName, "", gc.getTechInfo(iPrereq).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iPrereq, -1, False )

		# add resource buttons
		bFirst = True
		iPrereq = gc.getUnitInfo(self.iUnit).getPrereqAndBonus()
		if (iPrereq >= 0):
			bFirst = False
			screen.attachImageButton( panelName, "", gc.getBonusInfo(iPrereq).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iPrereq, -1, False )

		# count the number of OR resources
		nOr = 0
		for j in range(gc.getNUM_UNIT_PREREQ_OR_BONUSES()):
			if (gc.getUnitInfo(self.iUnit).getPrereqOrBonuses(j) > -1):
				nOr += 1

		szLeftDelimeter = ""
		szRightDelimeter = ""
		#  Display a bracket if we have more than one OR resource and an AND resource
		if (not bFirst):
			if (nOr > 1):
				szLeftDelimeter = localText.getText("TXT_KEY_AND", ()) + "( "
				szRightDelimeter = " ) "
			elif (nOr > 0):
				szLeftDelimeter = localText.getText("TXT_KEY_AND", ())

		if len(szLeftDelimeter) > 0:
			screen.attachLabel(panelName, "", szLeftDelimeter)

		bFirst = True
		for j in range(gc.getNUM_UNIT_PREREQ_OR_BONUSES()):
			eBonus = gc.getUnitInfo(self.iUnit).getPrereqOrBonuses(j)
			if (eBonus > -1):
				if (not bFirst):
					screen.attachLabel(panelName, "", localText.getText("TXT_KEY_OR", ()))
				else:
					bFirst = False
				screen.attachImageButton( panelName, "", gc.getBonusInfo(eBonus).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, eBonus, -1, False )

		if len(szRightDelimeter) > 0:
			screen.attachLabel(panelName, "", szRightDelimeter)

		# add religion buttons
		iPrereq = gc.getUnitInfo(self.iUnit).getPrereqReligion()
		if (iPrereq >= 0):
			screen.attachImageButton( panelName, "", gc.getReligionInfo(iPrereq).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_RELIGION, iPrereq, -1, False )

		# add building buttons
		iPrereq = gc.getUnitInfo(self.iUnit).getPrereqBuilding()
		if (iPrereq >= 0):
			screen.attachImageButton( panelName, "", gc.getBuildingInfo(iPrereq).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iPrereq, -1, False )

		# requires buildingclass
		iPrereq = gc.getUnitInfo(self.iUnit).getPrereqBuildingClass()
		if (iPrereq >= 0):
			eClass = gc.getUnitInfo(self.iUnit).getUnitClassType()
			bCanBuild = False
			# if ingame, get active civ variant for the unitclass
			if self.top.iActivePlayer != -1:
				ePlayerUnit = gc.getCivilizationInfo(gc.getGame().getActiveCivilizationType()).getCivilizationUnits(eClass)
				if (ePlayerUnit == self.iUnit):
					bCanBuild = True
			if bCanBuild:# if ingame and activeciv can build unit, get active civ unitupgrades
				eRequiredBuilding = gc.getCivilizationInfo(gc.getGame().getActiveCivilizationType()).getCivilizationBuildings(iPrereq)
				if (eRequiredBuilding == -1):
					eRequiredBuilding=gc.getBuildingClassInfo(iPrereq).getDefaultBuildingIndex()
				screen.attachImageButton( panelName, "", gc.getBuildingInfo(eRequiredBuilding).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, eRequiredBuilding, -1, False )
			else:
				lPrereqBuildings = []
				for iLoopCiv in range(gc.getNumCivilizationInfos()):
					eLoopUnit = gc.getCivilizationInfo(iLoopCiv).getCivilizationUnits(eClass)
					if eLoopUnit == self.iUnit: # If the current civ has the same UU as the one we're studying
						eLoopBuilding = gc.getCivilizationInfo(iLoopCiv).getCivilizationBuildings(iPrereq) # get the required building
						if eLoopBuilding != -1:
							lPrereqBuildings.append(eLoopBuilding)
				# sort and remove duplicates
				lPrereqBuildings.sort()
				lPrereqBuildings = list(set(lPrereqBuildings))
				bFirst = True
				for eLoopBuilding in lPrereqBuildings:
					if not bFirst:
						screen.attachLabel(panelName, "", localText.getText("TXT_KEY_OR", ()))
					else:
						bFirst = False
					screen.attachImageButton( panelName, "", gc.getBuildingInfo(eLoopBuilding).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, eLoopBuilding, -1, False )

		# requires civic
		iPrereq = gc.getUnitInfo(self.iUnit).getPrereqCivic()
		if (iPrereq >= 0):
			screen.attachImageButton( panelName, "", gc.getCivicInfo(iPrereq).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIVIC, iPrereq, -1, False )

	# Place upgrades
	def placeUpgradesTo(self):
		screen = self.top.getScreen()
		self.X_UPGRADES_TO_PANE = self.top.EXT_SPACING
		self.Y_UPGRADES_TO_PANE = self.Y_PREREQ_PANE + self.H_PREREQ_PANE
		eClass = gc.getUnitInfo(self.iUnit).getUnitClassType()
		if isWorldUnitClass(eClass):
			self.Y_UPGRADES_TO_PANE += self.top.INT_SPACING
			self.W_UPGRADES_TO_PANE = self.W_WORLD_UNIT
			self.H_UPGRADES_TO_PANE = self.H_WORLD_UNIT
			szImage = str(gc.getUnitInfo(self.iUnit).getImage())
			screen.addDDSGFC(self.top.getNextWidgetName(), szImage,
				self.X_UPGRADES_TO_PANE, self.Y_UPGRADES_TO_PANE, self.W_WORLD_UNIT, self.H_WORLD_UNIT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		else:
			panelName = self.top.getNextWidgetName()
			self.W_UPGRADES_TO_PANE = self.W_PREREQ_PANE
			self.H_UPGRADES_TO_PANE = self.top.H_BLUE50_PANEL
			screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_UPGRADES_TO", ()), "", false, true, self.X_UPGRADES_TO_PANE, self.Y_UPGRADES_TO_PANE, self.W_WORLD_UNIT, self.top.H_BLUE50_PANEL, PanelStyles.PANEL_STYLE_BLUE50)
			screen.attachLabel(panelName, "", "  ")
			bCanBuild = False
			# if ingame, get active civ variant for the unitclass
			if self.top.iActivePlayer != -1:
				pPlayer = gc.getPlayer(gc.getGame().getActivePlayer())
				ePlayerUnit = pPlayer.getPlayerUnit(eClass)
				if (ePlayerUnit == self.iUnit):
					bCanBuild = True
			# get all civs that can build our unit
			Civlist = []
			for iLoopCiv in range(gc.getNumCivilizationInfos()):
				eLoopUnit = gc.getCivilizationInfo(iLoopCiv).getCivilizationUnits(eClass)
				if (eLoopUnit == self.iUnit):
					Civlist.append(iLoopCiv)
			# check all upgradeclasses for our unit
			for k in range(gc.getUnitInfo(self.iUnit).getNumUpgradeUnitClass()):
				eUpgradeClass = gc.getUnitInfo(self.iUnit).getUpgradeUnitClass(k)
				if bCanBuild:# if ingame and activeciv can build unit, get active civ unitupgrades
					eLoopUpgradeUnit = pPlayer.getPlayerUnit(eUpgradeClass)
					if (eLoopUpgradeUnit >= 0 and gc.getUnitInfo(eLoopUpgradeUnit).isDisableUpgradeTo() == False):
						# szButton = gc.getUnitInfo(eLoopUpgradeUnit).getButton()
						szButton = gc.getPlayer(self.top.iActivePlayer).getUnitButton(eLoopUpgradeUnit)
						screen.attachImageButton( panelName, "", szButton, GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, eLoopUpgradeUnit, 1, False )
				else:
					lUpgradeList = []
					bFirst = True
					if Civlist:
						for iLoopCiv in Civlist:
							eLoopUpgradeUnit = gc.getCivilizationInfo(iLoopCiv).getCivilizationUnits(eUpgradeClass)
							if (eLoopUpgradeUnit >= 0 and gc.getUnitInfo(eLoopUpgradeUnit).isDisableUpgradeTo() == False):
								lUpgradeList.append(eLoopUpgradeUnit)
					else:
						eLoopUpgradeUnit = gc.getUnitClassInfo(eUpgradeClass).getDefaultUnitIndex()
						if eLoopUpgradeUnit == -1: continue
						lUpgradeList.append(eLoopUpgradeUnit)
					# sort and remove duplicates
					lUpgradeList.sort()
					lUpgradeList = list(set(lUpgradeList))
					for eLoopUpgradeUnit in lUpgradeList:
						szButton = gc.getUnitInfo(eLoopUpgradeUnit).getButton()
						if not bFirst:
							screen.attachLabel(panelName, "", localText.getText("TXT_KEY_OR", ()))
						else:
							bFirst = False
						screen.attachImageButton( panelName, "", szButton, GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, eLoopUpgradeUnit, 1, False )

	# Place Special abilities
	def placeSpecial(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()

		self.X_SPECIAL_PANE = self.top.EXT_SPACING
		self.Y_SPECIAL_PANE = self.Y_UPGRADES_TO_PANE + self.H_UPGRADES_TO_PANE
		self.W_SPECIAL_PANE = self.W_WORLD_UNIT
		self.H_SPECIAL_PANE = self.top.H_SCREEN - self.top.H_BOT_BAR - self.top.EXT_SPACING - self.Y_SPECIAL_PANE
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_SPECIAL_ABILITIES", ()), "", true, false,
								self.X_SPECIAL_PANE, self.Y_SPECIAL_PANE, self.W_SPECIAL_PANE, self.H_SPECIAL_PANE, PanelStyles.PANEL_STYLE_BLUE50 )

		listName = self.top.getNextWidgetName()

		szSpecialText = CyGameTextMgr().getUnitHelp( self.iUnit, True, False, False, None )[1:]
		screen.addMultilineText(listName, szSpecialText, self.X_SPECIAL_PANE+5, self.Y_SPECIAL_PANE+30, self.W_SPECIAL_PANE-10, self.H_SPECIAL_PANE-35, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeAnimation(self):
		screen = self.top.getScreen()
		self.X_UNIT_ANIMATION = self.top.EXT_SPACING + self.W_WORLD_UNIT + self.top.INT_SPACING
		self.Y_UNIT_ANIMATION = self.Y_UNIT_PANE
		self.W_UNIT_ANIMATION = 350
		self.H_UNIT_ANIMATION = self.Y_SPECIAL_PANE - self.Y_UNIT_ANIMATION
		self.X_ROTATION_UNIT_ANIMATION = -20
		self.Z_ROTATION_UNIT_ANIMATION = 30
		self.SCALE_ANIMATION = 1.0

		# Unit animation
		screen.addUnitGraphicGFC(self.top.getNextWidgetName(), self.iUnit, self.X_UNIT_ANIMATION, self.Y_UNIT_ANIMATION, self.W_UNIT_ANIMATION, self.H_UNIT_ANIMATION, WidgetTypes.WIDGET_GENERAL, -1, -1, self.X_ROTATION_UNIT_ANIMATION, self.Z_ROTATION_UNIT_ANIMATION, self.SCALE_ANIMATION, True)

	def placeHistory(self):
		screen = self.top.getScreen()
		self.X_HISTORY = self.X_UNIT_ANIMATION + self.W_UNIT_ANIMATION + self.top.INT_SPACING
		self.Y_HISTORY = self.top.H_TOP_BAR + self.top.INT_SPACING
		self.W_HISTORY = self.top.X_LINKS - self.X_HISTORY - self.top.EXT_SPACING
		# the remaining vertical space under the logo is split in 2. upper-half --> History ; bottom-half --> Strategy
		self.H_HISTORY = int(0.5 * (self.top.H_SCREEN-self.top.H_BOT_BAR-self.top.EXT_SPACING-self.Y_HISTORY))
		HistoryPanel = self.top.getNextWidgetName()
		screen.addPanel( HistoryPanel, localText.getText("TXT_KEY_CIVILOPEDIA_HISTORY", ()), "", True, True,
						self.X_HISTORY, self.Y_HISTORY,self.W_HISTORY, self.H_HISTORY,
						PanelStyles.PANEL_STYLE_BLUE50 )
		HistoryTextPanel = self.top.getNextWidgetName()
		screen.addPanel( HistoryTextPanel, "", "", true, true,self.X_HISTORY+self.top.HM_TEXT, self.Y_HISTORY+self.top.VM_TEXT, self.W_HISTORY - 2 * self.top.HM_TEXT, self.H_HISTORY - self.top.VM_TEXT, PanelStyles.PANEL_STYLE_EMPTY)
		HistoryText = gc.getUnitInfo(self.iUnit).getCivilopedia()
		screen.attachMultilineText( HistoryTextPanel, "", HistoryText, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeStrategy(self):
		screen = self.top.getScreen()
		self.X_STRATEGY = self.X_HISTORY
		self.Y_STRATEGY = self.Y_HISTORY + self.H_HISTORY
		self.W_STRATEGY = self.W_HISTORY
		self.H_STRATEGY = self.H_HISTORY
		StrategyPanel = self.top.getNextWidgetName()
		screen.addPanel( StrategyPanel, localText.getText("TXT_KEY_STRATEGY", ()), "", true, true,self.X_STRATEGY, self.Y_STRATEGY, self.W_STRATEGY, self.H_STRATEGY, PanelStyles.PANEL_STYLE_BLUE50 )
		StrategyTextPanel = self.top.getNextWidgetName()
		screen.addPanel( StrategyTextPanel, "", "", true, true,self.X_STRATEGY+self.top.HM_TEXT, self.Y_STRATEGY+self.top.VM_TEXT, self.W_STRATEGY - 2 * self.top.HM_TEXT, self.H_STRATEGY - self.top.VM_TEXT, PanelStyles.PANEL_STYLE_EMPTY)
		StrategyText = gc.getUnitInfo(self.iUnit).getStrategy()
		screen.attachMultilineText( StrategyTextPanel, "", StrategyText,WidgetTypes.WIDGET_GENERAL,-1,-1, CvUtil.FONT_LEFT_JUSTIFY )

	def placePromotions(self):
		screen = self.top.getScreen()

		# add pane and text
		panelName = self.top.getNextWidgetName()
		self.X_PROMO_PANE = self.X_UNIT_ANIMATION# + self.W_UNIT_ANIMATION + self.top.INT_SPACING
		self.Y_PROMO_PANE = self.Y_UNIT_ANIMATION + self.H_UNIT_ANIMATION
		self.W_PROMO_PANE = self.W_UNIT_ANIMATION
		self.H_PROMO_PANE = self.top.H_SCREEN - self.top.H_BOT_BAR - self.top.EXT_SPACING - self.Y_PROMO_PANE
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_CATEGORY_PROMOTION", ()), "", true, true, self.X_PROMO_PANE, self.Y_PROMO_PANE, self.W_PROMO_PANE, self.H_PROMO_PANE, PanelStyles.PANEL_STYLE_BLUE50 )

		# add promotion buttons
		rowListName = self.top.getNextWidgetName()
		self.PROMOTION_ICON_SIZE = 32
		screen.addMultiListControlGFC(rowListName, "", self.X_PROMO_PANE+15, self.Y_PROMO_PANE+40, self.W_PROMO_PANE-20, self.H_PROMO_PANE-40, 1, self.PROMOTION_ICON_SIZE, self.PROMOTION_ICON_SIZE, TableStyles.TABLE_STYLE_STANDARD)

		for k in range(gc.getNumPromotionInfos()):
			if (isPromotionValid(k, self.iUnit, false) and not gc.getPromotionInfo(k).isGraphicalOnly()):
				screen.appendMultiListButton( rowListName, gc.getPromotionInfo(k).getButton(), 0, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, k, -1, false )

	def placeLinks(self, bRedraw):
		screen = self.top.getScreen()

		if bRedraw:
			screen.clearListBoxGFC(self.top.LIST_ID)

		listSorted = self.getSortedList()

		i = 0
		iSelected = 0
		for iI in range(len(listSorted)):
			iUnit = listSorted[iI][2]
			szName = listSorted[iI][3]
			if bRedraw:
				screen.appendListBoxString(self.top.LIST_ID, szName, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, -1, CvUtil.FONT_LEFT_JUSTIFY)
			if iUnit == self.iUnit:
				iSelected = i
			i += 1

		screen.setSelectedListBoxStringGFC(self.top.LIST_ID, iSelected)

	def getSortedList(self):
		listUnits = []
		iCount = 0
		for iUnit in range(gc.getNumUnitInfos()):
			eUnit = gc.getUnitInfo(iUnit)
			if not eUnit.isGraphicalOnly():
				if self.CURRENT_FILTER["TypeHardcoded"]:
					for unit in self.CURRENT_FILTER["TypeHardcodeList"]:
						if iUnit == gc.getInfoTypeForString(unit):
							listUnits.append(iUnit)
							iCount += 1
				elif self.CURRENT_FILTER["ClassHardcoded"]:
					for unitclass in self.CURRENT_FILTER["ClassHardcodeList"]:
						if gc.getUnitInfo(iUnit).getUnitClassType() == gc.getInfoTypeForString(unitclass):
							listUnits.append(iUnit)
							iCount += 1
				elif eval(self.CURRENT_FILTER["Value to Check"]) == eval(self.CURRENT_FILTER["Desired Result"]):
					listUnits.append(iUnit)
					iCount += 1

		listSorted = [(0,0,0,0,0,0)] * iCount
		iI = 0
		for iUnit in listUnits:
			eUnit = gc.getUnitInfo(iUnit)
			szButton = eUnit.getButton()
			if self.top.iActivePlayer != -1:
				szButton = gc.getPlayer(self.top.iActivePlayer).getUnitButton(iUnit)
			sort1 = 0
			sort2 = 0
			if not self.CURRENT_SORT["Value to Sort"] == None:
				sort1 = eval(self.CURRENT_SORT["Value to Sort"])
				if self.CURRENT_SORT["Value to Sort"] == 'eUnit.getDescription()':
					sort1 = CvUtil.removeAccent(self, sort1) # alphabetical sorting in the Pedia for languages using accents/diacritics
				if not self.SUB_SORT["Value to Sort"] == None:
					sort2 = eval(self.SUB_SORT["Value to Sort"])
			listSorted[iI] = (sort1, sort2, iUnit, eUnit.getDescription(), szButton, 1)
			iI += 1
		listSorted.sort()

		return listSorted

	def applyFilterSort(self, filter, sort):
		if not self.CURRENT_SORT == sort:
			self.SUB_SORT = self.CURRENT_SORT
			self.CURRENT_SORT = sort
			self.top.pediaJump(CvScreenEnums.PEDIA_MAIN, CivilopediaPageTypes.CIVILOPEDIA_PAGE_UNIT, True)

		if not self.CURRENT_FILTER == filter:
			if eval(filter["name"]) == eval('localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_CIV_SPECIFIC", ())') or filter["Civ Specific"]:
				self.ALLOWED_FILTERS = []
				for i, filterlist in enumerate(self.FILTERS):
					if filterlist["Civ Specific"] or eval(filterlist["name"]) == eval('localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_CIV_SPECIFIC", ())') or eval(filterlist["name"]) == eval('localText.getText("TXT_KEY_PEDIA_FILTER_UNFILTERED", ())'):	#Things break if the item you use to get into a sublist is not also a part of that sublist
						self.ALLOWED_FILTERS.append(filterlist)
			elif (self.CURRENT_FILTER["Civ Specific"] and filter["Primary"]) or eval(self.CURRENT_FILTER["name"]) == eval('localText.getText("TXT_KEY_PEDIA_FILTER_UNIT_CIV_SPECIFIC", ())'):
				self.ALLOWED_FILTERS = []
				for i, filterlist in enumerate(self.FILTERS):
					if filterlist["Primary"]:
						self.ALLOWED_FILTERS.append(filterlist)
			self.CURRENT_FILTER = filter
			self.top.pediaJump(CvScreenEnums.PEDIA_MAIN, CivilopediaPageTypes.CIVILOPEDIA_PAGE_UNIT, True)

	# Will handle the input for this screen...
	def handleInput (self, inputClass):
		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_LISTBOX_ITEM_SELECTED):
			screen = self.top.getScreen()
			iFilterIndex = screen.getSelectedPullDownID(self.top.FILTER_DROPDOWN_ID)
			iSortIndex = screen.getSelectedPullDownID(self.top.SORT_DROPDOWN_ID)
			self.applyFilterSort(self.ALLOWED_FILTERS[iFilterIndex], self.ALLOWED_SORTS[iSortIndex])
			return 1

		return 0
