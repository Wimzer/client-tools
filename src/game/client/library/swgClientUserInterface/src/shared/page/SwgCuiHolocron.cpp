//======================================================================
//
// SwgCuiHolocron.cpp
// Copyright Sony Online Entertainment
//
//======================================================================

#include "swgClientUserInterface/FirstSwgClientUserInterface.h"
#include "swgClientUserInterface/SwgCuiHolocron.h"

#include "clientUserInterface/CuiActionManager.h"
#include "clientUserInterface/CuiActions.h"
#include "clientUserInterface/CuiKnowledgeBaseManager.h"
#include "clientUserInterface/CuiManager.h"
#include "clientUserInterface/CuiSystemMessageManager.h"
#include "UIButton.h"
#include "UIDataSourceContainer.h"
#include "UIImage.h"
#include "UIPage.h"
#include "UIText.h"
#include "UITreeView.h"

//======================================================================

namespace SwgCuiHolocronNamespace
{
	static const UILowerString KBNodeNameProperty = UILowerString("KBNodeNameProperty");
	static const UILowerString KBLinkTargetProperty = UILowerString("KBLinkTargetProperty");

	std::string const cms_linkButton("linkButton");

	Unicode::String const cms_expandAllStyle(Unicode::narrowToWide("/Styles.tree.image.expanded"));
	Unicode::String const cms_collapseAllStyle(Unicode::narrowToWide("/Styles.tree.image.collapsed"));
}

using namespace SwgCuiHolocronNamespace;

#define HOLOCRON_DEBUG(msg) CuiSystemMessageManager::sendFakeSystemMessage(Unicode::narrowToWide(msg))

//----------------------------------------------------------------------


//======================================================================

SwgCuiHolocron::SwgCuiHolocron(UIPage & page) :
CuiMediator                ("SwgCuiHolocron", page)
, UIEventCallback          ()
, m_treeTopics             (0)
, m_entryText              (0)
, m_entryTitle             (0)
, m_textInfo               (0)
, m_entryComp              (0)
, m_imageSample            (0)
, m_buttonBack             (0)
, m_buttonNext             (0)
, m_buttonClose            (0)
, m_buttonExit             (0)
, m_buttonServices         (0)
, m_buttonBug              (0)
, m_buttonReportPlayer     (0)
, m_buttonReload           (0)
, m_buttonSample           (0)
, m_textSample             (0)
, m_navigationHistory      ()
, m_navigationIndex        (-1)
, m_currentPageName        ()
{
	getCodeDataObject(TUITreeView,  m_treeTopics,        "treeTopics");
	getCodeDataObject(TUIText,      m_entryText,         "entrytext");
	getCodeDataObject(TUIText,      m_entryTitle,        "entrytitle");
	getCodeDataObject(TUIText,      m_textInfo,          "textInfor");
	getCodeDataObject(TUIPage,      m_entryComp,         "entrycomp");
	getCodeDataObject(TUIImage,     m_imageSample,       "imagesample");
	getCodeDataObject(TUIButton,    m_buttonBack,        "buttonback");
	getCodeDataObject(TUIButton,    m_buttonNext,        "buttonnext");
	getCodeDataObject(TUIButton,    m_buttonClose,       "buttonclose");
	getCodeDataObject(TUIButton,    m_buttonExit,        "buttonExit");
	getCodeDataObject(TUIButton,    m_buttonServices,    "buttonservices");
	getCodeDataObject(TUIButton,    m_buttonBug,         "buttonbug");
	getCodeDataObject(TUIButton,    m_buttonReportPlayer,"buttonReportPlayer");
	getCodeDataObject(TUIButton,    m_buttonSample,      "buttonSample");
	getCodeDataObject(TUIText,      m_textSample,        "textSample");

	// Optional debug reload button
	UIButton * debugButton = 0;
	getCodeDataObject(TUIButton, debugButton, "buttonreload", true);
	if (debugButton)
	{
		m_buttonReload = debugButton;
		registerMediatorObject(*m_buttonReload, true);
	}

	registerMediatorObject(*m_treeTopics,        true);
	registerMediatorObject(*m_buttonBack,        true);
	registerMediatorObject(*m_buttonNext,        true);
	registerMediatorObject(*m_buttonClose,       true);
	registerMediatorObject(*m_buttonExit,        true);
	registerMediatorObject(*m_buttonServices,    true);
	registerMediatorObject(*m_buttonBug,         true);
	registerMediatorObject(*m_buttonReportPlayer,true);

	m_entryTitle->SetPreLocalized(true);
	m_entryTitle->Clear();
	m_entryText->SetPreLocalized(true);
	m_entryText->Clear();
	m_textInfo->SetPreLocalized(true);

	m_buttonSample->SetVisible(false);
	m_textSample->SetVisible(false);
	m_imageSample->SetVisible(false);

	m_buttonBack->SetEnabled(false);
	m_buttonNext->SetEnabled(false);

	setState(MS_closeable);
	setState(MS_closeDeactivates);

	HOLOCRON_DEBUG("[Holocron] constructor complete");
}

//----------------------------------------------------------------------

SwgCuiHolocron::~SwgCuiHolocron()
{
	m_treeTopics         = 0;
	m_entryText          = 0;
	m_entryTitle         = 0;
	m_textInfo           = 0;
	m_entryComp          = 0;
	m_imageSample        = 0;
	m_buttonBack         = 0;
	m_buttonNext         = 0;
	m_buttonClose        = 0;
	m_buttonExit         = 0;
	m_buttonServices     = 0;
	m_buttonBug          = 0;
	m_buttonReportPlayer = 0;
	m_buttonReload       = 0;
	m_buttonSample       = 0;
	m_textSample         = 0;
}

//----------------------------------------------------------------------

void SwgCuiHolocron::performActivate()
{
	HOLOCRON_DEBUG("[Holocron] performActivate BEGIN");

	CuiManager::requestPointer(true);

	// Strings are pre-warmed at startup in CuiManagerManager::install().
	// This is a safety net — near-instant since it's all cache hits.
	CuiKnowledgeBaseManager::warmAllBodyStrings();

	populateTopicTree();

	if (m_currentPageName.empty())
	{
		HOLOCRON_DEBUG("[Holocron] performActivate: loading Root page");
		loadPage("Root");
	}

	// Pre-warm the button template so the first LinkButton page doesn't hitch.
	// Create an invisible duplicate, add it (forces template resource load),
	// then clearContent() will clean it up on the next page navigation.
	if (m_buttonSample && m_entryComp)
	{
		UIButton * const warmup = safe_cast<UIButton *>(m_buttonSample->DuplicateObject());
		if (warmup)
		{
			warmup->SetVisible(false);
			warmup->SetName(cms_linkButton);
			m_entryComp->AddChild(warmup);
		}
	}

	char buf[256];
	snprintf(buf, sizeof(buf), "[Holocron] performActivate END - page visible=%d size=%ldx%ld",
		getPage().IsVisible() ? 1 : 0,
		getPage().GetWidth(),
		getPage().GetHeight());
	HOLOCRON_DEBUG(buf);
}

//----------------------------------------------------------------------

void SwgCuiHolocron::performDeactivate()
{
	HOLOCRON_DEBUG("[Holocron] performDeactivate");
	CuiManager::requestPointer(false);
}

//----------------------------------------------------------------------

void SwgCuiHolocron::OnButtonPressed(UIWidget * context)
{
	if (context == m_buttonClose || context == m_buttonExit)
	{
		closeThroughWorkspace();
	}
	else if (context == m_buttonBack)
	{
		navigateBack();
	}
	else if (context == m_buttonNext)
	{
		navigateNext();
	}
	else if (context == m_buttonServices)
	{
		IGNORE_RETURN(CuiActionManager::performAction(CuiActions::service, Unicode::emptyString));
	}
	else if (context == m_buttonBug)
	{
		IGNORE_RETURN(CuiActionManager::performAction(CuiActions::bugReport, Unicode::emptyString));
	}
	else if (context == m_buttonReportPlayer)
	{
		IGNORE_RETURN(CuiActionManager::performAction(CuiActions::harassmentMessage, Unicode::emptyString));
	}
	else if (context == m_buttonReload && m_buttonReload != 0)
	{
		CuiKnowledgeBaseManager::reloadData();
		populateTopicTree();
		if (!m_currentPageName.empty())
		{
			loadPage(m_currentPageName);
		}
	}
	else
	{
		// Check if a dynamically created link button was pressed
		std::string linkTarget;
		context->GetPropertyNarrow(KBLinkTargetProperty, linkTarget);
		if (!linkTarget.empty())
		{
			loadPage(linkTarget);
		}
	}
}

//----------------------------------------------------------------------

void SwgCuiHolocron::OnGenericSelectionChanged(UIWidget * context)
{
	if (context == m_treeTopics)
	{
		int const selectedRow = m_treeTopics->GetLastSelectedRow();
		UIDataSourceContainer const * const dsc = m_treeTopics->GetDataSourceContainerAtRow(selectedRow);

		if (dsc)
		{
			std::string nodeName;
			dsc->GetPropertyNarrow(KBNodeNameProperty, nodeName);

			if (!nodeName.empty())
			{
				loadPage(nodeName);
			}
		}
	}
}

//----------------------------------------------------------------------

void SwgCuiHolocron::loadPage(const std::string & pageName)
{
	char buf[256];
	snprintf(buf, sizeof(buf), "[Holocron] loadPage: '%s'", pageName.c_str());
	HOLOCRON_DEBUG(buf);

	CuiKnowledgeBaseManager::BaseKBNode * const node = CuiKnowledgeBaseManager::findNode(pageName);

	if (node)
	{
		HOLOCRON_DEBUG("[Holocron] loadPage: node found");
		displayPage(node);
		m_currentPageName = pageName;

		// Truncate forward history if navigating to a new page
		if (m_navigationIndex < static_cast<int>(m_navigationHistory.size()) - 1)
		{
			m_navigationHistory.erase(
				m_navigationHistory.begin() + m_navigationIndex + 1,
				m_navigationHistory.end()
			);
		}

		m_navigationHistory.push_back(pageName);
		m_navigationIndex = static_cast<int>(m_navigationHistory.size()) - 1;

		updateNavigationButtons();
	}
	else
	{
		snprintf(buf, sizeof(buf), "[Holocron] loadPage: NODE NOT FOUND '%s'", pageName.c_str());
		HOLOCRON_DEBUG(buf);
		DEBUG_WARNING(true, ("SwgCuiHolocron: Page not found: %s", pageName.c_str()));
	}
}

//----------------------------------------------------------------------

void SwgCuiHolocron::setActivePage(const std::string & pageName)
{
	loadPage(pageName);
}

//----------------------------------------------------------------------

void SwgCuiHolocron::populateTopicTree()
{
	HOLOCRON_DEBUG("[Holocron] populateTopicTree BEGIN");

	if (!m_treeTopics)
	{
		HOLOCRON_DEBUG("[Holocron] populateTopicTree: m_treeTopics is NULL!");
		return;
	}

	UIDataSourceContainer * const mainDsc = NON_NULL(m_treeTopics->GetDataSourceContainer());
	mainDsc->Attach(0);
	m_treeTopics->SetDataSourceContainer(0);
	mainDsc->Clear();

	CuiKnowledgeBaseManager::BaseKBNode * const root = CuiKnowledgeBaseManager::getRoot();
	if (root)
	{
		char buf[256];
		snprintf(buf, sizeof(buf), "[Holocron] populateTopicTree: root has %d children",
			static_cast<int>(root->m_children.size()));
		HOLOCRON_DEBUG(buf);

		for (std::vector<CuiKnowledgeBaseManager::BaseKBNode *>::const_iterator it = root->m_children.begin();
		     it != root->m_children.end(); ++it)
		{
			populateDataSourceRecursive(mainDsc, *it);
		}
	}
	else
	{
		HOLOCRON_DEBUG("[Holocron] populateTopicTree: KB root is NULL - no data loaded!");
	}

	m_treeTopics->SetDataSourceContainer(mainDsc);
	mainDsc->Detach(0);

	HOLOCRON_DEBUG("[Holocron] populateTopicTree END");
}

//----------------------------------------------------------------------

void SwgCuiHolocron::populateDataSourceRecursive(UIDataSourceContainer * parentDsc, CuiKnowledgeBaseManager::BaseKBNode * node)
{
	if (!node || !parentDsc)
		return;

	if (node->m_type != CuiKnowledgeBaseManager::s_pageType)
		return;

	CuiKnowledgeBaseManager::PageKBNode * const pageNode = static_cast<CuiKnowledgeBaseManager::PageKBNode *>(node);

	Unicode::String title;
	if (pageNode->m_string.isValid())
	{
		title = pageNode->m_string.localize();
	}
	else
	{
		title = Unicode::narrowToWide(node->m_name);
	}

	UIDataSourceContainer * const nodeDsc = new UIDataSourceContainer;
	nodeDsc->SetProperty(UITreeView::DataProperties::Text, title);
	nodeDsc->SetPropertyNarrow(KBNodeNameProperty, node->m_name);
	nodeDsc->SetPropertyBoolean(UITreeView::DataProperties::Selectable, true);
	nodeDsc->SetPropertyBoolean(UITreeView::DataProperties::Expanded, false);

	parentDsc->AddChild(nodeDsc);

	for (std::vector<CuiKnowledgeBaseManager::BaseKBNode *>::const_iterator it = node->m_children.begin();
	     it != node->m_children.end(); ++it)
	{
		populateDataSourceRecursive(nodeDsc, *it);
	}
}

//----------------------------------------------------------------------

void SwgCuiHolocron::displayPage(CuiKnowledgeBaseManager::BaseKBNode * node)
{
	if (!node)
		return;

	clearContent();

	if (node->m_type == CuiKnowledgeBaseManager::s_pageType)
	{
		CuiKnowledgeBaseManager::PageKBNode * const pageNode = static_cast<CuiKnowledgeBaseManager::PageKBNode *>(node);
		if (pageNode->m_string.isValid())
		{
			m_entryTitle->SetLocalText(pageNode->m_string.localize());
		}
		else
		{
			m_entryTitle->SetLocalText(Unicode::narrowToWide(node->m_name));
		}
	}

	Unicode::String contentText;

	{
		char dbg[256];
		snprintf(dbg, sizeof(dbg), "[Holocron] displayPage: childCount=%d", static_cast<int>(node->m_children.size()));
		HOLOCRON_DEBUG(dbg);
	}

	for (std::vector<CuiKnowledgeBaseManager::BaseKBNode *>::const_iterator it = node->m_children.begin();
	     it != node->m_children.end(); ++it)
	{
		CuiKnowledgeBaseManager::BaseKBNode * const child = *it;

		if (child->m_type == CuiKnowledgeBaseManager::s_stringType)
		{
			CuiKnowledgeBaseManager::StringKBNode * const stringNode = static_cast<CuiKnowledgeBaseManager::StringKBNode *>(child);
			{
				std::string const sid = stringNode->m_string.getDebugString();
				char dbg[256];
				snprintf(dbg, sizeof(dbg), "[Holocron] displayPage: String node '%s'", sid.c_str());
				HOLOCRON_DEBUG(dbg);
			}
			if (stringNode->m_string.isValid())
			{
				contentText += stringNode->m_string.localize();
				contentText += Unicode::narrowToWide("\n\n");
			}
		}
		else if (child->m_type == CuiKnowledgeBaseManager::s_imageType)
		{
			CuiKnowledgeBaseManager::ImageKBNode * const imageNode = static_cast<CuiKnowledgeBaseManager::ImageKBNode *>(child);
			{
				char dbg[256];
				snprintf(dbg, sizeof(dbg), "[Holocron] displayPage: Image node path='%s'", imageNode->m_path.c_str());
				HOLOCRON_DEBUG(dbg);
			}
			if (!imageNode->m_path.empty())
			{
				m_imageSample->SetVisible(true);
				IGNORE_RETURN(m_imageSample->SetSourceResource(Unicode::narrowToWide(imageNode->m_path)));
			}
		}
		else if (child->m_type == CuiKnowledgeBaseManager::s_linkButtonType)
		{
			CuiKnowledgeBaseManager::LinkButtonKBNode * const linkNode = static_cast<CuiKnowledgeBaseManager::LinkButtonKBNode *>(child);
			{
				char dbg[256];
				snprintf(dbg, sizeof(dbg), "[Holocron] displayPage: LinkButton text='%s' link='%s'", linkNode->m_string.c_str(), linkNode->m_link.c_str());
				HOLOCRON_DEBUG(dbg);
			}

			UIButton * const linkButton = safe_cast<UIButton *>(m_buttonSample->DuplicateObject());
			NOT_NULL(linkButton);
			linkButton->SetVisible(true);
			linkButton->SetLocalText(Unicode::narrowToWide(linkNode->m_string));
			linkButton->SetName(cms_linkButton);
			linkButton->SetPropertyNarrow(KBLinkTargetProperty, linkNode->m_link);
			registerMediatorObject(*linkButton, true);
			m_entryComp->AddChild(linkButton);
		}
	}

	if (m_entryText)
	{
		m_entryText->SetLocalText(contentText);
	}
}

//----------------------------------------------------------------------

void SwgCuiHolocron::clearContent()
{
	if (m_entryText)
		m_entryText->Clear();
	if (m_entryTitle)
		m_entryTitle->Clear();
	if (m_imageSample)
		m_imageSample->SetVisible(false);

	// Remove dynamically created link buttons from entrycomp
	if (m_entryComp)
	{
		UIBaseObject * linkBtn = m_entryComp->GetChild(cms_linkButton.c_str());
		while (linkBtn)
		{
			m_entryComp->RemoveChild(linkBtn);
			linkBtn = m_entryComp->GetChild(cms_linkButton.c_str());
		}
	}
}

//----------------------------------------------------------------------

void SwgCuiHolocron::navigateBack()
{
	if (m_navigationIndex > 0)
	{
		--m_navigationIndex;
		std::string const & pageName = m_navigationHistory[static_cast<size_t>(m_navigationIndex)];

		CuiKnowledgeBaseManager::BaseKBNode * const node = CuiKnowledgeBaseManager::findNode(pageName);
		if (node)
		{
			displayPage(node);
			m_currentPageName = pageName;
		}

		updateNavigationButtons();
	}
}

//----------------------------------------------------------------------

void SwgCuiHolocron::navigateNext()
{
	if (m_navigationIndex < static_cast<int>(m_navigationHistory.size()) - 1)
	{
		++m_navigationIndex;
		std::string const & pageName = m_navigationHistory[static_cast<size_t>(m_navigationIndex)];

		CuiKnowledgeBaseManager::BaseKBNode * const node = CuiKnowledgeBaseManager::findNode(pageName);
		if (node)
		{
			displayPage(node);
			m_currentPageName = pageName;
		}

		updateNavigationButtons();
	}
}

//----------------------------------------------------------------------

void SwgCuiHolocron::updateNavigationButtons()
{
	if (m_buttonBack)
		m_buttonBack->SetEnabled(m_navigationIndex > 0);

	if (m_buttonNext)
		m_buttonNext->SetEnabled(m_navigationIndex < static_cast<int>(m_navigationHistory.size()) - 1);
}

//----------------------------------------------------------------------

SwgCuiHolocron * SwgCuiHolocron::createInto(UIPage * parent)
{
	HOLOCRON_DEBUG("[Holocron] createInto BEGIN");

	UIPage * const dupe = UIPage::DuplicateInto(*parent, "/PDA.help");

	char buf[256];
	snprintf(buf, sizeof(buf), "[Holocron] createInto: dupe=%p visible=%d size=%ldx%ld",
		static_cast<void*>(dupe),
		dupe ? dupe->IsVisible() ? 1 : 0 : -1,
		dupe ? dupe->GetWidth() : 0,
		dupe ? dupe->GetHeight() : 0);
	HOLOCRON_DEBUG(buf);

	if (!dupe)
	{
		DEBUG_WARNING(true, ("[Holocron] createInto: DuplicateInto failed for /PDA.help"));
		return 0;
	}

	return new SwgCuiHolocron(*dupe);
}

//======================================================================
