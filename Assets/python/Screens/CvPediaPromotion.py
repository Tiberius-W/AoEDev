## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
from CvPythonExtensions import *
import CvUtil
import ScreenInput
import CvScreenEnums

# globals
gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class CvPediaPromotion:
	"Civilopedia Screen for Promotions"

	def __init__(self, main):
		self.iPromotion = -1
		self.top = main

		self.BUTTON_SIZE = 46

		self.FILTERS =	[
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNFILTERED", ())',		# Name of the filter as it appears in dropdown menu.  Needs to be unique for this screen
				"Purpose" : "Clears all currently active filters",		# Description for the sanity of those who modify this file, appears and is used nowhere else
				"Hardcoded" : False,		# If this is True, then only those items in the HardcodeList will pass the check
				"HardcodeList" : [],
				"Value to Check" : 'None',	# Note that ePromotion is the PromotionInfo object being tested
				"Desired Result" : 'None',
				"Unit Type Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_PROMOTION_DEFAULT", ())',
				"Purpose" : "EXP-purchased promotions",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.getMinLevel() < 0 or ePromotion.isNoXP()',
				"Desired Result" : 'False',
				"Unit Type Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_PROMOTION_EFFECTS", ())',
				"Purpose" : "Non-EXP-purchased promotions",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.getMinLevel() < 0 or ePromotion.isNoXP()',
				"Desired Result" : 'True',
				"Unit Type Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_PROMOTION_RACES", ())',
				"Purpose" : "Racial promotions",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.isRace()',
				"Desired Result" : 'True',
				"Unit Type Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_PROMOTION_EQUIPMENT", ())',
				"Purpose" : "Gear promotions",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.isEquipment()',
				"Desired Result" : 'True',
				"Unit Type Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_PROMOTION_MOUNTS", ())',
				"Purpose" : "Mount promotions",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.getPromotionClass()==gc.getInfoTypeForString("PROMOTIONCLASS_MOUNT")',
				"Desired Result" : 'True',
				"Unit Type Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_PROMOTION_NAVAL_CREW", ())',
				"Purpose" : "Naval Crew promotions",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.getPromotionClass()==gc.getInfoTypeForString("PROMOTIONCLASS_NAVAL_CREW")',
				"Desired Result" : 'True',
				"Unit Type Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_PROMOTION_COMMAND_SYSTEM", ())',
				"Purpose" : "Benevolence & Feedback Promotions as I originally dubbed them",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.getNumMinionPromotions() > 0 or ePromotion.getNumCommanderPromotions() or ePromotion.getNumSlavePromotions() > 0 or ePromotion.getNumMasterPromotions()',
				"Desired Result" : 'True',
				"Unit Type Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_PROMOTION_CITY_AUGMENTATION", ())',
				"Purpose" : "Promotions using the so far rarely exploited CityBonus system",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.getNumCityBonuses() > 0',
				"Desired Result" : 'True',
				"Unit Type Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_PROMOTION_AUTOMATIC", ())',
				"Purpose" : "I've wanted a section for these for so long...",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.isAutoAcquire()',
				"Desired Result" : 'True',
				"Unit Type Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_PROMOTION_MAGIC_SPHERES", ())',
				"Purpose" : "I've wanted a section for these for so long...",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'self.isMagicSpherePromotion(ePromotion)',
				"Desired Result" : 'True',
				"Unit Type Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_PROMOTION_NON_UNITCOMBAT_SPECIFIC", ())',
				"Purpose" : "I've wanted a section for these for so long...",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'self.isUnitcombatSpecific(ePromotion)',
				"Desired Result" : 'False',
				"Unit Type Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_PROMOTION_UNITCOMBAT_SPECIFIC", ())',
				"Purpose" : "I've wanted a section for these for so long...",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'self.isUnitcombatSpecific(ePromotion)',
				"Desired Result" : 'True',
				"Unit Type Specific" : False,
			},
			{
				"name" : 'localText.getText("TXT_KEY_UNITCOMBAT_ANIMAL", ())',
				"Purpose" : "Promotions available per unitcombat",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_ANIMAL"))',
				"Desired Result" : 'True',
				"Unit Type Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_UNITCOMBAT_ADEPT", ())',
				"Purpose" : "Promotions available per unitcombat",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_ADEPT"))',
				"Desired Result" : 'True',
				"Unit Type Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_UNITCOMBAT_ARCHER", ())',
				"Purpose" : "Promotions available per unitcombat",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_ARCHER"))',
				"Desired Result" : 'True',
				"Unit Type Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_UNITCOMBAT_BEAST", ())',
				"Purpose" : "Promotions available per unitcombat",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_BEAST"))',
				"Desired Result" : 'True',
				"Unit Type Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_UNITCOMBAT_COMMANDER", ())',
				"Purpose" : "Promotions available per unitcombat",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_COMMANDER"))',
				"Desired Result" : 'True',
				"Unit Type Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_UNITCOMBAT_DISCIPLE", ())',
				"Purpose" : "Promotions available per unitcombat",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_DISCIPLE"))',
				"Desired Result" : 'True',
				"Unit Type Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_UNITCOMBAT_MELEE", ())',
				"Purpose" : "Promotions available per unitcombat",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_MELEE"))',
				"Desired Result" : 'True',
				"Unit Type Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_UNITCOMBAT_DEFENSIVE_MELEE", ())',
				"Purpose" : "Promotions available per unitcombat",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_DEFENSIVE_MELEE"))',
				"Desired Result" : 'True',
				"Unit Type Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_UNITCOMBAT_MOUNTED", ())',
				"Purpose" : "Promotions available per unitcombat",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_MOUNTED"))',
				"Desired Result" : 'True',
				"Unit Type Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_UNITCOMBAT_NAVAL", ())',
				"Purpose" : "Promotions available per unitcombat",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_NAVAL"))',
				"Desired Result" : 'True',
				"Unit Type Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_UNITCOMBAT_RECON", ())',
				"Purpose" : "Promotions available per unitcombat",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_RECON"))',
				"Desired Result" : 'True',
				"Unit Type Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_UNITCOMBAT_ROGUE", ())',
				"Purpose" : "Promotions available per unitcombat",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_ROGUE"))',
				"Desired Result" : 'True',
				"Unit Type Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_UNITCOMBAT_SIEGE", ())',
				"Purpose" : "Promotions available per unitcombat",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_SIEGE"))',
				"Desired Result" : 'True',
				"Unit Type Specific" : True,
			},
			{
				"name" : 'localText.getText("TXT_KEY_UNITCOMBAT_WORKER", ())',
				"Purpose" : "Promotions available per unitcombat",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_WORKER"))',
				"Desired Result" : 'True',
				"Unit Type Specific" : True,
			},
			]

		# List the filters which you want to be available initially, or self.FILTERS to have all of them available from the start
		self.ALLOWED_FILTERS = []
		for i, filter in enumerate(self.FILTERS):
			if not self.FILTERS[i]["Unit Type Specific"]:
				self.ALLOWED_FILTERS.append(self.FILTERS[i])
		self.CURRENT_FILTER = self.FILTERS[0]

		self.SORTS =	[
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_ALPHABETICAL", ())',
				"Purpose" : "Default Sorting Method",
				"Value to Sort" : 'ePromotion.getDescription()',
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
	def interfaceScreen(self, iPromotion):
		self.iPromotion = iPromotion

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
		szHeader = u"<font=4b>" + gc.getPromotionInfo(self.iPromotion).getDescription().upper() + u"</font>"
		szHeaderId = self.top.getNextWidgetName()
		screen.setLabel(szHeaderId, "Background", szHeader, CvUtil.FONT_CENTER_JUSTIFY, self.top.X_SCREEN, self.top.Y_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		# Title (upper-right corner, above the list)
		szHeader = u"<font=3b>" + localText.getText("TXT_KEY_PEDIA_CATEGORY_PROMOTION", ()).upper() + u"</font>"
		szHeaderId = self.top.getNextWidgetName()
		screen.setLabel(szHeaderId, "", szHeader, CvUtil.FONT_CENTER_JUSTIFY, self.top.W_SCREEN -self.top.W_LINKS/2, self.top.Y_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, CivilopediaPageTypes.CIVILOPEDIA_PAGE_PROMOTION, -1)

		# Top
		JumpTo = CivilopediaPageTypes.CIVILOPEDIA_PAGE_PROMOTION
		screen.setText(self.top.getNextWidgetName(), "Background", self.top.MENU_TEXT, CvUtil.FONT_LEFT_JUSTIFY, self.top.X_MENU, self.top.Y_MENU, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_PEDIA_MAIN, JumpTo, -1)

		CurrScreen = CvScreenEnums.PEDIA_PROMOTION
		if self.top.iLastScreen	!= CurrScreen or bNotActive:
			self.placeLinks(true)
			self.top.iLastScreen = CurrScreen
		else:
			self.placeLinks(false)

		# Icon
		self.X_UNIT_PANE = self.top.EXT_SPACING
		self.Y_UNIT_PANE = self.top.H_TOP_BAR + self.top.INT_SPACING
		self.W_UNIT_PANE = 250
		self.H_UNIT_PANE = 210
		self.X_ICON = 98
		self.Y_ICON = 100
		self.W_ICON = 150
		self.H_ICON = 150
		self.ICON_SIZE = 64
		
		screen.addPanel( self.top.getNextWidgetName(), "", "", False, False,
			self.X_UNIT_PANE, self.Y_UNIT_PANE, self.W_UNIT_PANE, self.H_UNIT_PANE, PanelStyles.PANEL_STYLE_BLUE50)
		screen.addDDSGFC(self.top.getNextWidgetName(), gc.getPromotionInfo(self.iPromotion).getButton(),
			self.X_ICON + self.W_ICON/2 - self.ICON_SIZE/2, self.Y_ICON + self.H_ICON/2 - self.ICON_SIZE/2, self.ICON_SIZE, self.ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		self.placePrereqs() # Place Required promotions
		self.placeLeadsTo() # Place Allowing promotions
		self.placeUnitGroups()
		self.placeSpecial() # Place the Special abilities block
		self.placeHistory()
		self.placeUnits()
		self.placeSpells()

	# Place prereqs...
	def placePrereqs(self):
		screen = self.top.getScreen()
		self.X_PREREQ_PANE = self.X_UNIT_PANE + self.W_UNIT_PANE + self.top.INT_SPACING
		self.Y_PREREQ_PANE = self.top.H_TOP_BAR + self.top.INT_SPACING
		self.W_PREREQ_PANE = 420
		self.H_PREREQ_PANE = self.top.H_BLUE50_PANEL
		panelName = self.top.getNextWidgetName()
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_REQUIRES", ()), "", false, true,
				 self.X_PREREQ_PANE, self.Y_PREREQ_PANE, self.W_PREREQ_PANE, self.H_PREREQ_PANE, PanelStyles.PANEL_STYLE_BLUE50 )

		screen.attachLabel(panelName, "", "  ")

		iPromotionORs = 0
		iPromotionANDs = 0
		bFirst = true
		for i in xrange(gc.getPromotionInfo(self.iPromotion).getNumPrereqPromotionANDs()):
			if (gc.getPromotionInfo(self.iPromotion).getPrereqPromotionANDs(i, True) > 0):
				iPromotion = gc.getPromotionInfo(self.iPromotion).getPrereqPromotionANDs(i, False)
				if not gc.getPromotionInfo(iPromotion).isEffectProm():
					iPromotionANDs += 1
					if iPromotionANDs > 1:
						screen.attachLabel(panelName, "", localText.getText("TXT_KEY_AND", ()))
					screen.attachImageButton( panelName, "", gc.getPromotionInfo(iPromotion).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, iPromotion, 1, False )
					if (gc.getPromotionInfo(self.iPromotion).getPrereqPromotionANDs(i, True) > 1):
						screen.attachLabel(panelName, "", localText.getText("TXT_KEY_NUM_PROMOTIONS_REQUIRED", (gc.getPromotionInfo(self.iPromotion).getPrereqPromotionANDs(i, True), )))
		for iPromotion in xrange(gc.getPromotionInfo(self.iPromotion).getNumPrereqPromotionORs()):
			if (gc.getPromotionInfo(self.iPromotion).getPrereqPromotionORs(iPromotion, True) > 0):
				if not gc.getPromotionInfo(iPromotion).isEffectProm():
					iPromotionORs += 1
		if iPromotionORs > 0:
			if iPromotionANDs > 0:
				screen.attachLabel(panelName, "", localText.getText("TXT_KEY_AND", ()))
			if iPromotionORs > 1:
				screen.attachLabel(panelName, "", "(")
			for i in xrange(gc.getPromotionInfo(self.iPromotion).getNumPrereqPromotionORs()):
				if (gc.getPromotionInfo(self.iPromotion).getPrereqPromotionORs(i, True) > 0):
					iPromotion = gc.getPromotionInfo(self.iPromotion).getPrereqPromotionORs(i, False)
					if not gc.getPromotionInfo(iPromotion).isEffectProm():
						if not bFirst:
							screen.attachLabel(panelName, "", localText.getText("TXT_KEY_OR", ()))
						else:
							bFirst = false
						screen.attachImageButton( panelName, "", gc.getPromotionInfo(iPromotion).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, iPromotion, 1, False )
						if (gc.getPromotionInfo(self.iPromotion).getPrereqPromotionORs(i, True) > 1):
							screen.attachLabel(panelName, "", localText.getText("TXT_KEY_NUM_PROMOTIONS_REQUIRED", (gc.getPromotionInfo(self.iPromotion).getPrereqPromotionORs(i, True), )))
			if iPromotionORs > 1:
				screen.attachLabel(panelName, "", ")")

		eTech = gc.getPromotionInfo(self.iPromotion).getTechPrereq()
		if (eTech > -1):
			screen.attachImageButton( panelName, "", gc.getTechInfo(eTech).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, eTech, 1, False )

		eReligion = gc.getPromotionInfo(self.iPromotion).getStateReligionPrereq()
		if (eReligion > -1):
			screen.attachImageButton( panelName, "", gc.getReligionInfo(eReligion).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_RELIGION, eReligion, 1, False )

	# Place Leads To...
	def placeLeadsTo(self):
		screen = self.top.getScreen()
		self.X_LEADS_TO_PANE = self.X_PREREQ_PANE
		self.Y_LEADS_TO_PANE = self.Y_PREREQ_PANE + self.H_PREREQ_PANE
		self.W_LEADS_TO_PANE = self.W_PREREQ_PANE
		self.H_LEADS_TO_PANE = self.top.H_BLUE50_PANEL
		panelName = self.top.getNextWidgetName()
		self.W_LEADS_TO_PANE = self.top.X_LINKS - self.top.EXT_SPACING - self.X_LEADS_TO_PANE
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_LEADS_TO", ()), "", false, true,
				 self.X_LEADS_TO_PANE, self.Y_LEADS_TO_PANE, self.W_LEADS_TO_PANE, self.H_LEADS_TO_PANE, PanelStyles.PANEL_STYLE_BLUE50 )

		screen.attachLabel(panelName, "", "  ")

		for j in range(gc.getNumPromotionInfos()):
			if not (gc.getPromotionInfo(j).isEffectProm()):
				for k in xrange(gc.getPromotionInfo(j).getNumPrereqPromotionORs()):
					if (gc.getPromotionInfo(j).getPrereqPromotionORs(k, False) == self.iPromotion):
						screen.attachImageButton( panelName, "", gc.getPromotionInfo(j).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, j, 1, False )
				for k in xrange(gc.getPromotionInfo(j).getNumPrereqPromotionANDs()):
					if (gc.getPromotionInfo(j).getPrereqPromotionANDs(k, False) == self.iPromotion):
						screen.attachImageButton( panelName, "", gc.getPromotionInfo(j).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, j, 1, False )

	def placeUnitGroups(self):
		screen = self.top.getScreen()
		self.X_UNIT_GROUP_PANE = self.top.EXT_SPACING
		self.Y_UNIT_GROUP_PANE = self.Y_LEADS_TO_PANE + self.H_LEADS_TO_PANE
		self.W_UNIT_GROUP_PANE = 250
		self.H_UNIT_GROUP_PANE = self.top.H_SCREEN - self.top.H_BOT_BAR - self.top.EXT_SPACING - 2*self.top.H_BLUE50_PANEL - self.Y_UNIT_GROUP_PANE
		self.DY_UNIT_GROUP_PANE = 25

		panelName = self.top.getNextWidgetName()
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_PROMOTION_UNITS", ()), "", true, true,
			self.X_UNIT_GROUP_PANE, self.Y_UNIT_GROUP_PANE, self.W_UNIT_GROUP_PANE, self.H_UNIT_GROUP_PANE, PanelStyles.PANEL_STYLE_BLUE50 )

		szTable = self.top.getNextWidgetName()
		screen.addTableControlGFC(szTable, 1,
			self.X_UNIT_GROUP_PANE + 10, self.Y_UNIT_GROUP_PANE + 40, self.W_UNIT_GROUP_PANE - 20, self.H_UNIT_GROUP_PANE - 50, False, False, 24, 24, TableStyles.TABLE_STYLE_EMPTY)

		i = 0
		for iI in range(gc.getNumUnitCombatInfos()):
			if (0 != gc.getPromotionInfo(self.iPromotion).getUnitCombat(iI)):
				iRow = screen.appendTableRow(szTable)
				screen.setTableText(szTable, 0, i, u"<font=2>" + gc.getUnitCombatInfo(iI).getDescription() + u"</font>", gc.getUnitCombatInfo(iI).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT_COMBAT, iI, -1, CvUtil.FONT_LEFT_JUSTIFY)
				i += 1

	def placeSpecial(self):
		screen = self.top.getScreen()
		self.X_SPECIAL_PANE = self.X_PREREQ_PANE
		self.Y_SPECIAL_PANE = self.Y_LEADS_TO_PANE + self.H_LEADS_TO_PANE
		self.W_SPECIAL_PANE = self.top.X_LINKS - self.top.EXT_SPACING - self.X_SPECIAL_PANE
		self.H_SPECIAL_PANE = ( self.top.H_SCREEN - self.top.H_BOT_BAR - self.top.EXT_SPACING - 2*self.top.H_BLUE50_PANEL - self.Y_SPECIAL_PANE ) / 2
		panelName = self.top.getNextWidgetName()
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_SPECIAL_ABILITIES", ()), "", true, false,
				 self.X_SPECIAL_PANE, self.Y_SPECIAL_PANE, self.W_SPECIAL_PANE, self.H_SPECIAL_PANE, PanelStyles.PANEL_STYLE_BLUE50 )

		listName = self.top.getNextWidgetName()

		szSpecialText = CyGameTextMgr().getPromotionHelp(self.iPromotion, True)[1:]
		screen.addMultilineText(listName, szSpecialText, self.X_SPECIAL_PANE+5, self.Y_SPECIAL_PANE+30, self.W_SPECIAL_PANE-10, self.H_SPECIAL_PANE-35, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeHistory(self):
		screen = self.top.getScreen()
		self.X_HISTORY = self.X_SPECIAL_PANE
		self.Y_HISTORY = self.Y_SPECIAL_PANE + self.H_SPECIAL_PANE
		self.W_HISTORY = self.W_SPECIAL_PANE
		self.H_HISTORY = self.H_SPECIAL_PANE
		HistoryPanel = self.top.getNextWidgetName()
		screen.addPanel( HistoryPanel, localText.getText("TXT_KEY_CIVILOPEDIA_HISTORY", ()), "", True, True,self.X_HISTORY, self.Y_HISTORY,self.W_HISTORY, self.H_HISTORY,PanelStyles.PANEL_STYLE_BLUE50 )
		HistoryTextPanel = self.top.getNextWidgetName()
		screen.addPanel( HistoryTextPanel, "", "", true, true,self.X_HISTORY+self.top.HM_TEXT, self.Y_HISTORY+self.top.VM_TEXT, self.W_HISTORY - 2 * self.top.HM_TEXT, self.H_HISTORY - self.top.VM_TEXT, PanelStyles.PANEL_STYLE_EMPTY)
		HistoryText = gc.getPromotionInfo(self.iPromotion).getCivilopedia()
		screen.attachMultilineText( HistoryTextPanel, "", HistoryText, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	# List units that begin with promotion
	def placeUnits(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		self.X_UNIT_PANE = self.top.EXT_SPACING
		self.H_UNIT_PANE = self.top.H_BLUE50_PANEL
		self.Y_UNIT_PANE = self.top.H_SCREEN-self.top.H_BOT_BAR-self.top.EXT_SPACING-2*self.H_UNIT_PANE
		self.W_UNIT_PANE = self.top.X_LINKS - self.top.EXT_SPACING - self.X_UNIT_PANE
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_PROMO_UNITS", ()), "", false, true,
				 self.X_UNIT_PANE, self.Y_UNIT_PANE, self.W_UNIT_PANE, self.H_UNIT_PANE, PanelStyles.PANEL_STYLE_BLUE50 )

		screen.attachLabel(panelName, "", "  ")

		for iUnit in range(gc.getNumUnitInfos()):
			if gc.getUnitInfo(iUnit).getFreePromotions(self.iPromotion):
				szButton = gc.getUnitInfo(iUnit).getButton()
				screen.attachImageButton( panelName, "", szButton, GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUnit, 1, False )
		
	# Place Leads To...
	def placeSpells(self):

		screen = self.top.getScreen()

		panelName = self.top.getNextWidgetName()
		self.X_SPELL_PANE = self.top.EXT_SPACING
		self.H_SPELL_PANE = self.top.H_BLUE50_PANEL
		self.Y_SPELL_PANE = self.top.H_SCREEN-self.top.H_BOT_BAR-self.top.EXT_SPACING-self.H_SPELL_PANE
		self.W_SPELL_PANE = self.top.X_LINKS - self.top.EXT_SPACING - self.X_SPELL_PANE
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_SPELLS (or LEADS_TO)", ()), "", false, true,
				 self.X_SPELL_PANE, self.Y_SPELL_PANE, self.W_SPELL_PANE, self.H_SPELL_PANE, PanelStyles.PANEL_STYLE_BLUE50 )

		screen.attachLabel(panelName, "", "  ")

		for j in range(gc.getNumSpellInfos()):
			iPrereq = gc.getSpellInfo(j).getPromotionPrereq1()
			if (iPrereq == self.iPromotion):
				screen.attachImageButton( panelName, "", gc.getSpellInfo(j).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_SPELL, j, 1, False )
			iPrereq = gc.getSpellInfo(j).getPromotionPrereq2()
			if (iPrereq == self.iPromotion):
				screen.attachImageButton( panelName, "", gc.getSpellInfo(j).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_SPELL, j, 1, False )

	def placeLinks(self, bRedraw):
		screen = self.top.getScreen()

		if bRedraw:
			screen.clearListBoxGFC(self.top.LIST_ID)

		listSorted = self.getSortedList()

		i = 0
		iSelected = 0
		for iI in range(len(listSorted)):
			iPromotion = listSorted[iI][2]
			szName = listSorted[iI][3]
			if bRedraw:
				screen.appendListBoxString(self.top.LIST_ID, szName, WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION, iPromotion, -1, CvUtil.FONT_LEFT_JUSTIFY)
			if iPromotion == self.iPromotion:
				iSelected = i
			i += 1

		screen.setSelectedListBoxStringGFC(self.top.LIST_ID, iSelected)

	def getSortedList(self):
		listPromotions = []
		iCount = 0
		for iPromotion in range(gc.getNumPromotionInfos()):
			ePromotion = gc.getPromotionInfo(iPromotion)
			if not ePromotion.isGraphicalOnly():
				if self.CURRENT_FILTER["Hardcoded"]:
					for promotion in self.CURRENT_FILTER["HardcodeList"]:
						if iPromotion == gc.getInfoTypeForString(promotion):
							listPromotions.append(iPromotion)
							iCount += 1
				elif eval(self.CURRENT_FILTER["Value to Check"]) == eval(self.CURRENT_FILTER["Desired Result"]):
					listPromotions.append(iPromotion)
					iCount += 1

		listSorted = [(0,0,0,0,0,0)] * iCount
		iI = 0
		for iPromotion in listPromotions:
			ePromotion = gc.getPromotionInfo(iPromotion)
			sort1 = 0
			sort2 = 0
			if not self.CURRENT_SORT["Value to Sort"] == None:
				sort1 = eval(self.CURRENT_SORT["Value to Sort"])
				if self.CURRENT_SORT["Value to Sort"] == 'ePromotion.getDescription()':
					sort1 = CvUtil.removeAccent(self,sort1) # alphabetical sorting in the Pedia for languages using accents/diacritics
				if not self.SUB_SORT["Value to Sort"] == None:
					sort2 = eval(self.SUB_SORT["Value to Sort"])
			listSorted[iI] = (sort1, sort2, iPromotion, ePromotion.getDescription(), ePromotion.getButton(), 1)
			iI += 1
		listSorted.sort()

		return listSorted

	def applyFilterSort(self, filter, sort):
		if not self.CURRENT_SORT == sort:
			self.SUB_SORT = self.CURRENT_SORT
			self.CURRENT_SORT = sort
			self.top.pediaJump(CvScreenEnums.PEDIA_MAIN, CivilopediaPageTypes.CIVILOPEDIA_PAGE_PROMOTION, True)

		if not self.CURRENT_FILTER == filter:
			if eval(filter["name"]) == eval('localText.getText("TXT_KEY_PEDIA_FILTER_PROMOTION_UNITCOMBAT_SPECIFIC", ())') or filter["Unit Type Specific"]:
				self.ALLOWED_FILTERS = []
				for i, filterlist in enumerate(self.FILTERS):
					if filterlist["Unit Type Specific"] or eval(filterlist["name"]) == eval('localText.getText("TXT_KEY_PEDIA_FILTER_PROMOTION_UNITCOMBAT_SPECIFIC", ())') or eval(filterlist["name"]) == eval('localText.getText("TXT_KEY_PEDIA_FILTER_UNFILTERED", ())'):	#Things break if the item you use to get into a sublist is not also a part of that sublist
						self.ALLOWED_FILTERS.append(filterlist)
			elif (self.CURRENT_FILTER["Unit Type Specific"] and (not filter["Unit Type Specific"])) or eval(self.CURRENT_FILTER["name"]) == eval('localText.getText("TXT_KEY_PEDIA_FILTER_PROMOTION_UNITCOMBAT_SPECIFIC", ())'):
				self.ALLOWED_FILTERS = []
				for i, filterlist in enumerate(self.FILTERS):
					if not filterlist["Unit Type Specific"]:
						self.ALLOWED_FILTERS.append(filterlist)
			self.CURRENT_FILTER = filter
			self.top.pediaJump(CvScreenEnums.PEDIA_MAIN, CivilopediaPageTypes.CIVILOPEDIA_PAGE_PROMOTION, True)

	# Will handle the input for this screen...
	def handleInput (self, inputClass):
		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_LISTBOX_ITEM_SELECTED):
			screen = self.top.getScreen()
			iFilterIndex = screen.getSelectedPullDownID(self.top.FILTER_DROPDOWN_ID)
			iSortIndex = screen.getSelectedPullDownID(self.top.SORT_DROPDOWN_ID)
			self.applyFilterSort(self.ALLOWED_FILTERS[iFilterIndex], self.ALLOWED_SORTS[iSortIndex])
			return 1

		return 0
	
	def isMagicSpherePromotion(self, ePromotion):
		# Include channeling because it is relevant
		lChannelingPromotions = [gc.getInfoTypeForString('PROMOTION_CHANNELING1'), gc.getInfoTypeForString('PROMOTION_CHANNELING2'), gc.getInfoTypeForString('PROMOTION_CHANNELING3')]
		if gc.getInfoTypeForString(ePromotion.getType()) in lChannelingPromotions:
			return True
		
		# Firstly the promotion has to require channeling
		lPrereqPromotions = [ePromotion.getPrereqPromotionANDs(i, False) for i in range(ePromotion.getNumPrereqPromotionANDs())]

		if bool(set(lPrereqPromotions) & set(lChannelingPromotions)):

			# If the promotion requires mana, it is a level 1 magic sphere promotion.
			for i in range(ePromotion.getNumPrereqBonusANDs()):
				if gc.getBonusInfo(ePromotion.getPrereqBonusAND(i)).getBonusClassType() == gc.getInfoTypeForString('BONUSCLASS_MANA'):
					return True

			# if the promotion requires a magic sphere promotion, it is a 2 or 3 magic sphere promotion.
			for pp in lPrereqPromotions:
				isMagic = self.isMagicSpherePromotion(gc.getPromotionInfo(pp)) and gc.getInfoTypeForString(gc.getPromotionInfo(pp).getType()) not in lChannelingPromotions
				if isMagic:
					return isMagic

		return False
	
	
	def isUnitcombatSpecific(self, ePromotion):
		return ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_ANIMAL")) or \
			ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_ADEPT")) or \
			ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_ARCHER")) or \
			ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_BEAST")) or \
			ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_COMMANDER")) or \
			ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_DISCIPLE")) or \
			ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_MELEE")) or \
			ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_MOUNTED")) or \
			ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_NAVAL")) or \
			ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_RECON")) or \
			ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_SIEGE")) or \
			ePromotion.getUnitCombat(gc.getInfoTypeForString("UNITCOMBAT_WORKER"))