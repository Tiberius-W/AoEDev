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

class CvPediaLore:
	"Civilopedia Screen for Lore Info"

	def __init__(self, main):
		self.iEntryId = -1
		self.iCivilopediaPageType = -1
		self.iEntry = -1
		self.top = main

		self.BUTTON_SIZE = 48

	# Screen construction function
	def interfaceScreen(self, iEntryId):

		self.iEntryId = iEntryId
		self.getEntryInfoFromId(iEntryId)

		self.top.deleteAllWidgets()

		screen = self.top.getScreen()

		bNotActive = (not screen.isActive())
		if bNotActive:
			self.top.setPediaCommonWidgets()

		# Header...
		szHeader = u"<font=4b>" + self.getDescription(self.iEntry).upper() + u"</font>"
		szHeaderId = self.top.getNextWidgetName()
		if self.getLink() == WidgetTypes.WIDGET_GENERAL:
			screen.setLabel(szHeaderId, "Background", szHeader, CvUtil.FONT_CENTER_JUSTIFY, self.top.X_SCREEN, self.top.Y_TITLE, 0, FontTypes.TITLE_FONT, self.getLink(),  self.iEntry, -1)
		else:
			screen.setText(szHeaderId, "Background", szHeader, CvUtil.FONT_CENTER_JUSTIFY, self.top.X_SCREEN, self.top.Y_TITLE, 0, FontTypes.TITLE_FONT, self.getLink(),  self.iEntry, -1)
			screen.setImageButton(self.top.getNextWidgetName(), ArtFileMgr.getInterfaceArtInfo("INTERFACE_GENERAL_CIVILOPEDIA_ICON").getPath(), self.top.X_EXIT, self.top.Y_TITLE, 32, 32, self.getLink(),  self.iEntry, -1)

		# Top
		screen.setText(self.top.getNextWidgetName(), "Background", self.top.MENU_TEXT, CvUtil.FONT_LEFT_JUSTIFY, self.top.X_MENU, self.top.Y_MENU, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_PEDIA_MAIN, self.iCivilopediaPageType, -1)

		if self.top.iLastScreen	!= CvScreenEnums.PEDIA_LORE or bNotActive:
			self.placeLinks(true)
			self.top.iLastScreen = CvScreenEnums.PEDIA_LORE
		else:
			self.placeLinks(false)

		self.placeText()

	def placeText(self):
		screen = self.top.getScreen()
		self.X_TEXT = self.top.EXT_SPACING
		self.Y_TEXT = self.top.H_TOP_BAR + self.top.INT_SPACING
		self.H_TEXT = self.top.H_SCREEN - self.top.H_BOT_BAR - self.top.EXT_SPACING - self.Y_TEXT
		self.W_TEXT = self.top.X_LINKS - self.X_TEXT - self.top.EXT_SPACING
		panelName = self.top.getNextWidgetName()
		screen.addPanel( panelName, "", "", true, true,
								 self.X_TEXT, self.Y_TEXT, self.W_TEXT, self.H_TEXT, PanelStyles.PANEL_STYLE_BLUE50 )

		szText = self.getCivilopedia()
		screen.attachMultilineText( panelName, "Text", szText, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeLinks(self, bRedraw):
		screen = self.top.getScreen()

		if bRedraw:
			screen.clearListBoxGFC(self.top.LIST_ID)

		iNum = self.getNumInfos()
		listSorted=[(0,0)] * iNum
		for j in range(iNum):
			listSorted[j] = (self.getDescription(j), j)
		listSorted.sort()

		iSelected = 0
		i = 0
		for iI in range(iNum):
			if (not self.getInfo(listSorted[iI][1]).isGraphicalOnly()):
				if bRedraw:
					screen.appendListBoxStringNoUpdate(self.top.LIST_ID, listSorted[iI][0], WidgetTypes.WIDGET_PEDIA_DESCRIPTION_NO_HELP, self.iCivilopediaPageType, listSorted[iI][1], CvUtil.FONT_LEFT_JUSTIFY)
				if listSorted[iI][1] == self.iEntry:
					iSelected = i
				i += 1

		if bRedraw:
			screen.updateListBox(self.top.LIST_ID)

		screen.setSelectedListBoxStringGFC(self.top.LIST_ID, iSelected)

	def getNumInfos(self):
		if (CivilopediaPageTypes.CIVILOPEDIA_PAGE_TECH == self.iCivilopediaPageType):
			iNum = gc.getNumTechInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_UNIT == self.iCivilopediaPageType):
			iNum = gc.getNumUnitInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_BUILDING == self.iCivilopediaPageType):
			iNum = gc.getNumBuildingInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_BONUS == self.iCivilopediaPageType):
			iNum = gc.getNumBonusInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_IMPROVEMENT == self.iCivilopediaPageType):
			iNum = gc.getNumImprovementInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_PROMOTION == self.iCivilopediaPageType):
			iNum = gc.getNumPromotionInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_UNIT_GROUP == self.iCivilopediaPageType):
			iNum = gc.getNumUnitCombatInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_CIV == self.iCivilopediaPageType):
			iNum = gc.getNumCivilizationInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_CITYCLASS == self.iCivilopediaPageType):
			iNum = gc.getNumCityClassInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_ROUTE == self.iCivilopediaPageType):
			iNum = gc.getNumRouteInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_LEADER == self.iCivilopediaPageType):
			iNum = gc.getNumLeaderHeadInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_RELIGION == self.iCivilopediaPageType):
			iNum = gc.getNumReligionInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_CORPORATION == self.iCivilopediaPageType):
			iNum = gc.getNumCorporationInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_CIVIC == self.iCivilopediaPageType):
			iNum = gc.getNumCivicInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_PROJECT == self.iCivilopediaPageType):
			iNum = gc.getNumProjectInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_LORE == self.iCivilopediaPageType):
			iNum = gc.getNumLoreInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_CONCEPT == self.iCivilopediaPageType):
			iNum = gc.getNumConceptInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_TRAIT == self.iCivilopediaPageType):
			iNum = gc.getNumTraitInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_SPAWNGROUP == self.iCivilopediaPageType):
			iNum = gc.getNumSpawnGroupInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_AFFINITY == self.iCivilopediaPageType):
			iNum = gc.getNumAffinityInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_SPECIALIST == self.iCivilopediaPageType):
			iNum = gc.getNumSpecialistInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_TERRAIN == self.iCivilopediaPageType):
			iNum = gc.getNumTerrainInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_FEATURE == self.iCivilopediaPageType):
			iNum = gc.getNumFeatureInfos()
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_PLOT_EFFECT == self.iCivilopediaPageType):
			iNum = gc.getNumPlotEffectInfos()
		else:
			iNum = ""
		return iNum

	def getInfo(self, iEntry):
		if (CivilopediaPageTypes.CIVILOPEDIA_PAGE_TECH == self.iCivilopediaPageType):
			info = gc.getTechInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_UNIT == self.iCivilopediaPageType):
			info = gc.getUnitInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_BUILDING == self.iCivilopediaPageType):
			info = gc.getBuildingInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_BONUS == self.iCivilopediaPageType):
			info = gc.getBonusInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_IMPROVEMENT == self.iCivilopediaPageType):
			info = gc.getImprovementInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_PROMOTION == self.iCivilopediaPageType):
			info = gc.getPromotionInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_UNIT_GROUP == self.iCivilopediaPageType):
			info = gc.getUnitCombatInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_CIV == self.iCivilopediaPageType):
			info = gc.getCivilizationInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_CITYCLASS == self.iCivilopediaPageType):
			iNum = gc.getCityClassInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_ROUTE == self.iCivilopediaPageType):
			iNum = gc.getRouteInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_LEADER == self.iCivilopediaPageType):
			info = gc.getLeaderHeadInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_RELIGION == self.iCivilopediaPageType):
			info = gc.getReligionInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_CORPORATION == self.iCivilopediaPageType):
			info = gc.getCorporationInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_CIVIC == self.iCivilopediaPageType):
			info = gc.getCivicInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_PROJECT == self.iCivilopediaPageType):
			info = gc.getProjectInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_LORE == self.iCivilopediaPageType):
			info = gc.getLoreInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_CONCEPT == self.iCivilopediaPageType):
			info = gc.getConceptInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_TRAIT == self.iCivilopediaPageType):
			info = gc.getTraittInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_SPAWNGROUP == self.iCivilopediaPageType):
			info = gc.getSpawnGroupInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_AFFINITY == self.iCivilopediaPageType):
			info = gc.getAffinityInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_SPECIALIST == self.iCivilopediaPageType):
			info = gc.getSpecialistInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_TERRAIN == self.iCivilopediaPageType):
			info = gc.getTerrainInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_FEATURE == self.iCivilopediaPageType):
			info = gc.getFeatureInfo(iEntry)
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_PLOT_EFFECT == self.iCivilopediaPageType):
			info = gc.getPlotEffectInfo(iEntry)
		else:
			info = None
		return info

	def getDescription(self, iEntry):
		info = self.getInfo(iEntry)
		if info != None:
			return info.getDescription()
		else:
			return u""

	def getLink(self):
		if (CivilopediaPageTypes.CIVILOPEDIA_PAGE_TECH == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_UNIT == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_BUILDING == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_TRAIT == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_TRAIT
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_SPAWNGROUP == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_SPAWNGROUP
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_AFFINITY == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_AFFINITY
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_BONUS == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_IMPROVEMENT == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_IMPROVEMENT
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_PROMOTION == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROMOTION
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_UNIT_GROUP == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT_COMBAT
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_CIV == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIV
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_CITYCLASS == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_CITYCLASS
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_ROUTE == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_ROUTE
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_LEADER == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_LEADER
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_RELIGION == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_RELIGION
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_CORPORATION == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_CORPORATION
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_CIVIC == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIVIC
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_PROJECT == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_PROJECT
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_SPECIALIST == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_SPECIALIST
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_TERRAIN == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_TERRAIN
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_FEATURE == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_FEATURE
		elif (CivilopediaPageTypes.CIVILOPEDIA_PAGE_PLOT_EFFECT == self.iCivilopediaPageType):
			iLink = WidgetTypes.WIDGET_PEDIA_JUMP_TO_PLOT_EFFECT
		else:
			iLink = WidgetTypes.WIDGET_GENERAL
		return iLink

	def getCivilopedia(self):
		info = self.getInfo(self.iEntry)
		if info != None:
			return info.getCivilopedia()
		else:
			return u""

	def getEntryInfoFromId(self, iEntryId):
		self.iCivilopediaPageType = iEntryId % CivilopediaPageTypes.NUM_CIVILOPEDIA_PAGE_TYPES
		self.iEntry = iEntryId // CivilopediaPageTypes.NUM_CIVILOPEDIA_PAGE_TYPES

	def getIdFromEntryInfo(self, iCivilopediaPageType, iEntry):
		return (iEntry * CivilopediaPageTypes.NUM_CIVILOPEDIA_PAGE_TYPES + iCivilopediaPageType)

	# Will handle the input for this screen...
	def handleInput (self, inputClass):
		return 0

