// Copyright (c) Naiky Company. All rights reserved.
// 高级任务加工
//
#define UNICODE
#include "stdafx.h"
#include "Ncui.h"
#include "AdvProgramTaskPage.h"
#include "SetCoorDialog.h"
#include <windows.h>
#include <lm.h>
#include <lmshare.h>
#pragma comment(lib, "netapi32.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CS_T s_csNOSELECT = _DEFCS(_CSV(4680),
	_CHINESE("|W|请先从文件列表中选择一个加工文件，再执行该操作。")
	_ENGLISH("|W|Please select a file first for further operation."));
CS_T s_szMODIFYWCA = _DEFCS(_CSV(6686),
	_CHINESE("请输入A轴工件坐标位置")
	_ENGLISH("Enter A-axis Workpiece Coordinate"));
CS_T s_szMODIFYTECHNAME = _DEFCS(_CSV(6687),
	_CHINESE("请输入加工工艺名\n匹配加工文件名: %s")
	_ENGLISH("Enter Technic Name\nFile name to Match: %s"));
CS_T s_szMODIFYFILEMATCHNAME = _DEFCS(_CSV(1614),
	_CHINESE("请输入加工文件匹配名\n加工工艺: %s")
	_ENGLISH("Enter Name for Files\nTechnic: %s"));
CS_T s_csISSIMULATING = _DEFCS(_CSV(876), 
	_CHINESE("|W|仿真状态不能执行该操作")
	_ENGLISH("|W|Unable to perform the action under simulation mode"));
CS_T s_csINVALIDMS = _DEFCS(_CSV(1899),
	_CHINESE("|W|当前加工状态不能执行该操作")
	_ENGLISH("|W|Unable to perform the action under the current state"));
CS_T s_csCANNOTLOADINVALIDTASK = _DEFCS(_CSV(6690),
	_CHINESE("不能装载无效任务！")
	_ENGLISH("Unable to load Invalid task!"));
CS_T s_csMODIFYTECHNAMENULL = _DEFCS(_CSV(6703),
	_CHINESE("|W|工艺名为空，修改工艺名失败！")
	_ENGLISH("|W|Technic name is empty. Failed to modify the technic name!"));
CS_T s_csMODIFYTECHNAMESAME = _DEFCS(_CSV(6704),
	_CHINESE("|W|工艺名重名，修改工艺名失败！")
	_ENGLISH("|W|Duplicated technic name. Failed to modify the technic name!"));
CS_T s_csMODIFYFILENAMENULL = _DEFCS(_CSV(6705),
	_CHINESE("|W|文件匹配名为空，修改文件名失败！")
	_ENGLISH("|W|Matched tool path name is empty. Failed to modify the file name!"));
CS_T s_csMODIFYFILENAMESAME = _DEFCS(_CSV(6706),
	_CHINESE("|W|文件匹配名重名，修改文件名失败！")
	_ENGLISH("|W|Duplicated name of the matched tool path. Failed to modify the file name!"));

const int c_IDT_FILECHANGE = 112;

// 支持的加工任务的文件类型
// 存在多种类型时，需考虑优先匹配
// 因为匹配名中不带扩展名
const struct IncludeExt
{
	LPCSTR pszExt;
	int nSize;
} c_IncludeExt[] =
{
	{".NC",		3},	// NC
	{".PIM",	4},	// PIM
};

IMPLEMENT_SERIAL(CAdvProgramTaskPage, CProgramPage, 1)

BEGIN_MESSAGE_MAP(CAdvProgramTaskPage, CProgramPage)
	ON_WM_TIMER()
	ON_WM_SHOWWINDOW()
	ON_COMMAND(IDCMD_ADVPROGTASK_LOADTASK, OnLoadTask)
	ON_UPDATE_COMMAND_UI(IDCMD_ADVPROGTASK_LOADTASK, OnUpdateLoadTask)
	ON_COMMAND(IDCMD_ADVPROGTASK_DELETE, OnDeleteFile)
	ON_UPDATE_COMMAND_UI(IDCMD_ADVPROGTASK_DELETE, OnUpdateDeleteFile)
	ON_COMMAND(IDCMD_ADVPROGTASK_MOVEUP, OnMoveUp)
	ON_UPDATE_COMMAND_UI(IDCMD_ADVPROGTASK_MOVEUP, OnUpdateMoveUp)
	ON_COMMAND(IDCMD_ADVPROGTASK_MOVEDOWN, OnMoveDown)
	ON_UPDATE_COMMAND_UI(IDCMD_ADVPROGTASK_MOVEDOWN, OnUpdateMoveDown)
	ON_UPDATE_COMMAND_UI(IDCMD_ADVPROGTASK_MODIFY, OnUpdateModify)
	ON_COMMAND(IDCMD_ADVPROGTASK_MODIFYTECHNAME, OnModifyTechName)
	ON_UPDATE_COMMAND_UI(IDCMD_ADVPROGTASK_MODIFYTECHNAME, OnUpdateModifyTechName)
	ON_COMMAND(IDCMD_ADVPROGTASK_MODIFYFILEMATCHNAME, OnModifyFileMatchName)
	ON_UPDATE_COMMAND_UI(IDCMD_ADVPROGTASK_MODIFYFILEMATCHNAME, OnUpdateModifyFileMatchName)
	ON_COMMAND(IDCMD_ADVPROGTASK_MODIFYCOORNO, OnModifyCoorNo)
	ON_UPDATE_COMMAND_UI(IDCMD_ADVPROGTASK_MODIFYCOORNO, OnUpdateModifyCoorNo)
	ON_COMMAND(IDCMD_ADVPROGTASK_MODIFYWORKCOORA, OnModifyWorkCoorA)
	ON_UPDATE_COMMAND_UI(IDCMD_ADVPROGTASK_MODIFYWORKCOORA, OnUpdateModifyWorkCoorA)
	ON_COMMAND(IDCMD_ADVPROGTASK_SAVETASKINFO, OnSaveTaskInfo)
	ON_UPDATE_COMMAND_UI(IDCMD_ADVPROGTASK_SAVETASKINFO, OnUpdateSaveTaskInfo)
	ON_COMMAND(IDCMD_ADVPROGTASK_BACK, OnBackToFolder)
	ON_UPDATE_COMMAND_UI(IDCMD_ADVPROGTASK_BACK, OnUpdateBackToFolder)
END_MESSAGE_MAP()

CAdvProgramTaskPage::CAdvProgramTaskPage(void)
{
	lstrcpyn(m_szCurFolder, "D:\\NcFiles\\", _MAX_PATH);
	m_pszCurPath = "D:\\NcFiles\\";
	m_FocusItem = -1;

	// 若没有NcFiles文件夹则创建
	if (!PathFileExists(m_szCurFolder))
	{
		CreateDirectory(m_szCurFolder, NULL);
	}

	for (int _i = 0; _i < NUMOF_FCOL; _i++)
	{
		CString _strTmp;
		_strTmp.Format("FLHead%d", _i);
		Register(m_pwndFLHead[_i], _strTmp);
	}

	for (int _i = 0; _i < NUMOF_TFCOL; _i++)
	{
		CString _strTmp;
		_strTmp.Format("TFLHead%d", _i);
		Register(m_pwndTFLHead[_i], _strTmp);
	}

	Register(m_pwndPath, "path");
	Register(m_pwndDiskSpace, "space");

	m_bAlreadyInTimer = false;
	LOAD_PARAMETER(this, CProgramPage);

	m_nListShow = LIST_FOLDER;
	m_nDiskType = harddisk;

	m_nExtendedStyle = 0;
	m_ImageList.Create(IDB_FILELISTIMAGES, 16, 0, RGB(0,0,255));
	m_ImageList.SetBkColor(GetSysColor(COLOR_WINDOW));

	CNcAdvTaskMngTarget* _pTarget = GetNcAdvTaskMngTarget();
	if (_pTarget != NULL)
	{
		m_pAdvTask = _pTarget->GetAdvTaskNode();
	}

	m_bReadFileInfoSucc = false;
	m_bModifyDefFileInfo = false;
}

CAdvProgramTaskPage::~CAdvProgramTaskPage(void)
{
	if (m_bModifyDefFileInfo)
	{
		CNcAdvTaskMngTarget* _pTarget = GetNcAdvTaskMngTarget();
		if (_pTarget != NULL)
		{
			_pTarget->SaveDefFileInfo();
		}

		m_bModifyDefFileInfo = false;
	}
}

void CAdvProgramTaskPage::OnInitPage()
{
	if (m_pwndList != NULL)
	{
		m_nExtendedStyle = m_pwndList->GetExtendedStyle();
	}

	m_FocusItem = 0;
	ShowListContent(LIST_FOLDER);
}

void CAdvProgramTaskPage::InitListBox()
{
	if (m_pwndList == NULL)
	{
		ASSERT(FALSE);
		return;
	}

	// 删除CFileMngListBox中所有列
	int nColCount = m_pwndList->GetHeaderCtrl()->GetItemCount();
	for (int _i = 0; _i < nColCount; _i++)
	{
		m_pwndList->DeleteColumn(0);
	}

	// 重新插入新列
	m_pwndList->ModifyStyle(NULL, LVS_NOCOLUMNHEADER);
	if (m_nListShow == LIST_FOLDER)
	{
		m_pwndList->SetExtendedStyle(m_nExtendedStyle);
		m_pwndList->SetImageList(&m_ImageList, LVSIL_SMALL);
		m_pwndList->InsertColumn(FCOL_NAME, _T(""), LVCFMT_RIGHT, 401);
		m_pwndList->InsertColumn(FCOL_PROGRAMNUMS, _T(""), LVCFMT_RIGHT, 171);
		m_pwndList->InsertColumn(FCOL_MODEFYTIME, _T(""), LVCFMT_RIGHT, 205);
	}
	else
	{
		m_pwndList->SetExtendedStyle(m_nExtendedStyle | LVS_EX_CHECKBOXES);
		CImageList _ImageList;
		_ImageList.Create(1, 20, ILC_MASK | ILC_COLOR, 0, 0);
		m_pwndList->SetImageList(&_ImageList, LVSIL_SMALL);
		m_pwndList->InsertColumn(TFCOL_TECHNOLOGY, _T(""), LVCFMT_LEFT, 81);
		m_pwndList->InsertColumn(TFCOL_PROGRAMNAME, _T(""), LVCFMT_LEFT, 201);
		m_pwndList->InsertColumn(TFCOL_COORNO, _T(""), LVCFMT_LEFT, 101);
		m_pwndList->InsertColumn(TFCOL_MACHCOORA, _T(""), LVCFMT_LEFT, 101);
		m_pwndList->InsertColumn(TFCOL_PROGRAMSIZE, _T(""), LVCFMT_RIGHT, 121);
		m_pwndList->InsertColumn(TFCOL_MODEFYTIME, _T(""), LVCFMT_RIGHT, 172);
	}
}

void CAdvProgramTaskPage::ShowListContent(list_t list_)
{
	m_nListShow = list_;

	int _nFSW = (list_ == LIST_FOLDER) ? SW_SHOW : SW_HIDE;
	for (int _i = 0; _i < NUMOF_FCOL; _i++)
	{
		if (m_pwndFLHead[_i] != NULL)
		{
			m_pwndFLHead[_i]->ShowWindow(_nFSW);
		}
	}

	int _nTFSW = (list_ == LIST_TASKFILE) ? SW_SHOW : SW_HIDE;
	for (int _i = 0; _i < NUMOF_TFCOL; _i++)
	{
		if (m_pwndTFLHead[_i] != NULL)
		{
			m_pwndTFLHead[_i]->ShowWindow(_nTFSW);
		}
	}

	InitListBox();
	UpdateListContent(true);
}

////////////////////////////////////////////////////////////
void CAdvProgramTaskPage::OnShowWindow(BOOL bShow_, UINT nStatus_)
{
	if (bShow_ == TRUE)
	{
		RefreshDiskPath(m_szCurFolder);
		SetTimer(c_IDT_FILECHANGE, 500, NULL);
	}
	else
	{
		KillTimer(c_IDT_FILECHANGE);
	}
	__super::OnShowWindow(bShow_, nStatus_);
}

void CAdvProgramTaskPage::GetSelFiles(CStringList& Files_)
{
	if (Files_.GetCount())
	{
		Files_.RemoveAll();
	}

	int _nCol = (m_nListShow == LIST_FOLDER) ? FCOL_NAME : TFCOL_PROGRAMNAME;
	for (int i = 0; i < m_pwndList->GetItemCount(); i++)
	{
		if (m_pwndList->GetItemSel(i))
		{
			Files_.AddTail(m_pwndList->GetItemText(i, _nCol));
		}
	}

	if (!Files_.GetHeadPosition())
	{
		int _nItemSel = m_pwndList->GetNextItem(-1, LVNI_SELECTED);
		while (_nItemSel != -1)
		{
			//选择“返回上层目录”时，不将此选项加到选择的文件链表中
			CString _str = m_pwndList->GetItemText(_nItemSel, _nCol);
			if (_str.Compare(".."))
			{
				Files_.AddHead(_str);
			}
			_nItemSel = m_pwndList->GetNextItem(_nItemSel, LVNI_SELECTED);
		}
	}
}

// 列表更新
void CAdvProgramTaskPage::UpdateListContent(bool bSelRst_/* = false*/)
{
	if (m_nListShow == LIST_FOLDER)
	{
		ReadFolderList();
		UpdateFolderList(bSelRst_);
	}
	else
	{
		ReadTaskFileList();
		UpdateTaskFileList(bSelRst_);
	}
}

// 读取文件目录
void CAdvProgramTaskPage::ReadFolderList()
{
	if (!CheckDiskExist(m_szCurFolder))
	{
		return;
	}

	ClearFileInfoArray();
	// 当前为D:\NcFiles\下一级目录时，显示上一级图标
	NCFILE_INFO* _pfi = NULL;
	if (!PathIsEqual(m_szCurFolder, m_pszCurPath))
	{
		_pfi = new NCFILE_INFO;
		_pfi->bIsFolder = true;
		_pfi->nImageNum = 8;
		_pfi->strFileName = "..";
		_pfi->nFileLength = 0;
		m_arrayFileInfo.Add(_pfi);
	}

	PathAddBackslash(m_szCurFolder);

	CString _strFilter = m_szCurFolder;
	_strFilter += "*.*";
	CFileFind _finder;
	BOOL _bWorking = _finder.FindFile(_strFilter);
	while (_bWorking)
	{
		_bWorking = _finder.FindNextFile();
		if (_finder.IsDots() 
			|| _finder.IsTemporary() 
			|| (_finder.IsDirectory() == FALSE))
		{
			continue;
		}

		_pfi = new NCFILE_INFO;
		_pfi->bIsFolder = true;
		_pfi->nImageNum = 1;
		_pfi->strFileName = _finder.GetFileName();
		// 文件目录时，nFileLength记录该文件目录中加工文件个数
		CString _strPath = m_szCurFolder;
		_strPath += _pfi->strFileName;
		_pfi->nFileLength = GetTaskFileCount(_strPath);
		_finder.GetLastWriteTime(_pfi->nLastTime);

		_pfi->strFileName.MakeUpper();
		if (_pfi->strFileName.Compare(_T("WIZARDS")))
		{
			m_arrayFileInfo.Add(_pfi);
		}
		else
		{
			delete _pfi;
			_pfi = NULL;
		}
	}
}

// 文件目录列表更新
void CAdvProgramTaskPage::UpdateFolderList(bool bSelRst_/* = false*/)
{
	if (m_pwndList == NULL)
	{
		ASSERT(FALSE);
		return;
	}

	m_pwndList->DeleteAllItems();

	NCFILE_INFO* _pfi = NULL;
	int _nSize = m_arrayFileInfo.GetSize();
	for (int _i = 0; _i < _nSize; _i++)
	{
		_pfi = (NCFILE_INFO*)m_arrayFileInfo.GetAt(_i);
		LV_ITEM lvi;
		lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
		lvi.iItem = _i;
		lvi.iSubItem = FCOL_NAME;
		lvi.iImage = _pfi->nImageNum;
		lvi.pszText = (char*)LPCTSTR(_pfi->strFileName);
		m_pwndList->InsertItem(&lvi);

		if (_pfi->nImageNum != 8)
		{
			CString _strTmp;
			_strTmp.Format(_T("%d"), _pfi->nFileLength);
			m_pwndList->SetItemText(_i, FCOL_PROGRAMNUMS, _strTmp);

			_strTmp = _pfi->nLastTime.Format("%Y-%m-%d  %H:%M");
			m_pwndList->SetItemText(_i, FCOL_MODEFYTIME, _strTmp);
		}

		m_pwndList->SetItemData(_i, (LPARAM)_i);
	}

	m_pwndList->SortItems(SortFunc, (LPARAM)this);

	// 设置焦点
	SetListFocus();
}

// 读取加工任务文件列表
void CAdvProgramTaskPage::ReadTaskFileList()
{
	if (!CheckDiskExist(m_szCurFolder))
	{
		return;
	}

	CNcAdvTaskMngTarget* _pTarget = GetNcAdvTaskMngTarget();
	if (_pTarget == NULL)
	{
		return;
	}

	PathAddBackslash(m_szCurFolder);

	// 读取文件列表
	m_bReadFileInfoSucc = true;
	if (_pTarget->LoadAdvTaskList(m_szCurFolder))
	{
		TaskFileListCheck();
	}
	else
	{
		SetDefaultTechInfo();
		m_bReadFileInfoSucc = false;
	}

	// 读取当前目录下加工文件
	AdvFileNode _FileNode;
	CString _strFilter = m_szCurFolder;
	_strFilter += "*.*";
	CFileFind _finder;
	int _nTech = 0;
	BOOL _bWorking = _finder.FindFile(_strFilter);
	while (_bWorking)
	{
		_bWorking = _finder.FindNextFile();
		if (_finder.IsDots() 
			|| _finder.IsTemporary() 
			|| (_finder.IsDirectory() == TRUE))
		{
			continue;
		}

		CString _strFileName = _finder.GetFileName();
		if (!FileExtensionCheck(_strFileName))
		{
			continue;
		}

		AdvFileNode* _pFileNode = MatchFileName(_strFileName);
		if (_pFileNode != NULL)
		{
			_pFileNode->bIsFolder = false;
			_pFileNode->nFileLength = (DWORD)_finder.GetLength();
			_finder.GetLastWriteTime(_pFileNode->nLastTime);
			continue;
		}

		_FileNode.ReSet();
		_FileNode.bIsFolder = false;
		_FileNode.bEnable = false;
		CString _strTech;
		_strTech.Format(_T("%d"), ++_nTech);
		int _nCount = _countof(_FileNode.szTechName);
		_tcsncpy_s(_FileNode.szTechName, _strTech, _nCount - 1);
		_nCount = _countof(_FileNode.szFileName);
		_tcsncpy_s(_FileNode.szFileName, _finder.GetFileName(), _nCount - 1);
		_FileNode.nCoorNo = WORKCOOR_EXTRA_FIRST;
		_FileNode.nWC = 0.0;
		_FileNode.nFileLength = (DWORD)_finder.GetLength();
		_finder.GetLastWriteTime(_FileNode.nLastTime);
		m_pAdvTask->m_vecFiles.push_back(_FileNode);
	}
}

// 读取的加工任务文件列表数据检查
void CAdvProgramTaskPage::TaskFileListCheck()
{
	if (m_pAdvTask->m_vecFiles.empty())
	{
		return;
	}

	// 检查文件是否存在
	std::vector<AdvFileNode> _vecFilesBk;
	std::vector<AdvFileNode>::iterator _iter = m_pAdvTask->m_vecFiles.begin();
	while (_iter != m_pAdvTask->m_vecFiles.end())
	{
		int _nDefFileID = _iter->nDefFileID;
		// 检查工艺名
		if (_nDefFileID != c_nNOTDEFFILE)
		{
			CString _strTech = _iter->szTechName;
			DefFileInfo* _pDefFileInfo = GetDefFileInfo() + _nDefFileID;
			if (_strTech.CompareNoCase(_pDefFileInfo->szTech) != 0)
			{
				int _nCount = _countof(_iter->szTechName);
				_tcsncpy_s(_iter->szTechName, _pDefFileInfo->szTech, _nCount - 1);
			}
		}

		// 检查文件是否存在
		PathAddBackslash(m_szCurFolder);
		CString _strFilePath = m_szCurFolder;
		_strFilePath += _iter->szFileName;
		if ((_iter->szFileName[0] == 0) || !PathFileExists(_strFilePath))
		{
			// 非默认工艺则删除
			if (_iter->nDefFileID == c_nNOTDEFFILE)
			{
				_iter = m_pAdvTask->m_vecFiles.erase(_iter);
				continue;
			}
			else
			{
				_iter->ReSet();
				_iter->nDefFileID = _nDefFileID;
				int _nCount = _countof(_iter->szTechName);
				DefFileInfo* _pDefFileInfo = GetDefFileInfo() + _nDefFileID;
				_tcsncpy_s(_iter->szTechName, _pDefFileInfo->szTech, _nCount - 1);
			}
			_iter++;
			continue;
		}
		else
		{
			// 匹配文件名
			CString _strFileName = _iter->szFileName;
			int _nIndex = _strFileName.ReverseFind('.');
			if (_nIndex != -1)
			{
				_strFileName = _strFileName.Left(_nIndex);
			}

			if (_iter->nDefFileID != c_nNOTDEFFILE)
			{
				// 文件存在，检查文件名是否与默认工艺匹配文件名相同
				DefFileInfo* _pDefFileInfo = GetDefFileInfo() + _nDefFileID;
				if (_strFileName.CompareNoCase(_pDefFileInfo->szFile) == 0)
				{
					// 匹配则继续
					_iter++;
					continue;
				}

				// 文件名不匹配，则删除对应信息
				_iter->ReSet();
				_iter->nDefFileID = _nDefFileID;
				int _nCount = _countof(_iter->szTechName);
				_tcsncpy_s(_iter->szTechName, _pDefFileInfo->szTech, _nCount - 1);
				_iter++;
				continue;
			}

			// 非默认工艺文件，则检查是否为默认工艺匹配文件
			bool _bMatchFileSucc = false;
			for (int _i = 0; _i < c_nDEFFILES_NUM; _i++)
			{
				DefFileInfo* _pDefFileInfo = GetDefFileInfo() + _i;
				if (_strFileName.CompareNoCase(_pDefFileInfo->szFile) != 0)
				{
					continue;
				}

				// 相同则先存储
				AdvFileNode _FileNode;
				AdvFileNode* _pFileNode = &(*_iter);
				memcpy(&_FileNode, _pFileNode, sizeof(AdvFileNode));
				_vecFilesBk.push_back(_FileNode);

				_bMatchFileSucc = true;
				_iter = m_pAdvTask->m_vecFiles.erase(_iter);
				break;
			}

			if (!_bMatchFileSucc)
			{
				_iter++;
			}
		}
	}

	// 读取的文件中有加工文件与默认工艺匹配，
	// 但没有添加到对应工艺里
	if (!_vecFilesBk.empty())
	{
		std::vector<AdvFileNode>::iterator _iterBk = _vecFilesBk.begin();
		while (_iterBk != _vecFilesBk.end())
		{
			CString _strFileName = _iterBk->szFileName;
			int _nIndex = _strFileName.ReverseFind('.');
			if (_nIndex != -1)
			{
				_strFileName = _strFileName.Left(_nIndex);
			}

			_iter = m_pAdvTask->m_vecFiles.begin();
			while (_iter != m_pAdvTask->m_vecFiles.end())
			{
				int _nDefFileID = _iter->nDefFileID;
				DefFileInfo* _pDefFileInfo = GetDefFileInfo() + _nDefFileID;
				if (_strFileName.CompareNoCase(_pDefFileInfo->szFile) != 0)
				{
					_iter++;
					continue;
				}

				_iter->ReSet();
				AdvFileNode* _pFileNodeBk = &(*_iterBk);
				AdvFileNode* _pFileNode = &(*_iter);
				memcpy(_pFileNode, _pFileNodeBk, sizeof(AdvFileNode));
				_iter->nDefFileID = _nDefFileID;
				int _nCount = _countof(_iter->szTechName);
				_tcsncpy_s(_iter->szTechName, _pDefFileInfo->szTech, _nCount - 1);
				break;
			}

			_iterBk++;
		}

		_vecFilesBk.clear();
	}
}

// 加工任务文件列表更新
void CAdvProgramTaskPage::UpdateTaskFileList(bool bSelRst_/* = false*/)
{
	if (m_pwndList == NULL)
	{
		ASSERT(FALSE);
		return;
	}

	m_pwndList->DeleteAllItems();
	int _nVecFileSize = m_pAdvTask->m_vecFiles.size();

	// _nCountUser 用户要显示的列表个数
	
	int _nIndex = 0
	for (; _nIndex < _nVecFileSize; _nIndex++)
	{
		if (_nIndex == _nCountUser) 
		{
			_nIndex = c_nDEFFILES_NUM;
			if (_nIndex >= _nVecFileSize)
			{
				break;
			}
		}

		AdvFileNode* _pFileNode = &m_pAdvTask->m_vecFiles.at(_nIndex);
		LV_ITEM lvi;
		lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
		lvi.iItem = _nIndex;
		lvi.iSubItem = TFCOL_TECHNOLOGY;
		lvi.pszText = _pFileNode->szTechName;
		// 工艺
		m_pwndList->InsertItem(&lvi);

		if (_pFileNode->szFileName[0] != 0)
		{
			BOOL _bCheck = _pFileNode->bEnable ? TRUE : FALSE;
			m_pwndList->SetCheck(_nIndex, _bCheck);

			// 加工文件名
			m_pwndList->SetItemText(_nIndex, TFCOL_PROGRAMNAME, _pFileNode->szFileName);

			CString _strTmp;
			// 坐标系
			int _nCoorNo = _pFileNode->nCoorNo;
			if (_nCoorNo >= WORKCOOR_G54 && _nCoorNo <= WORKCOOR_G59)
			{
				_nCoorNo = _nCoorNo + 54;
				_strTmp.Format(_T("G%d"), _nCoorNo);
			}
			else if (_nCoorNo >= WORKCOOR_EXTRA_FIRST && _nCoorNo <= WORKCOOR_EXTRA_LAST)
			{
				_nCoorNo = _nCoorNo - WORKCOOR_EXTRA_FIRST;
				_strTmp.Format(_T("G54 P%d"), _nCoorNo);
			}
			else
			{
				ASSERT(FALSE);
			}
			m_pwndList->SetItemText(_nIndex, TFCOL_COORNO, _strTmp);

			// A轴工件坐标
			CString _strWC;
			_strWC.Format(_T("%.3f"), _pFileNode->nWC);
			NormalizeFloatString(_strWC, NFS_TRIMTAIL);
			_strTmp.Format(_T("A%s"), _strWC);
			m_pwndList->SetItemText(_nIndex, TFCOL_MACHCOORA, _strTmp);

			// 文件大小
			// 得到按字节为单位的长度
			DWORD _nFileLength = _pFileNode->nFileLength;
			// 单位为KB
			int _nMulti = 1024;
			double _nFileLengthK = (double)_nFileLength / (double)_nMulti;
			if (_nFileLengthK >= 1 || DOUBLE_EQU_ZERO(_nFileLengthK))
			{
				_strTmp.Format(_T("%d"), int(_nFileLengthK));
				int _nLength = _strTmp.GetLength();
				// 文件大小长度的字符串，从后往前，每3位插入一个逗号
				for (int _j = _nLength - 1; _j > 0; _j--)
				{
					if (!((_nLength - _j) % 3))
					{
						_strTmp.Insert(_j, ',');
					}
				}
			}
			else
			{
				_strTmp.Format("%.3f", _nFileLengthK);
				NormalizeFloatString(_strTmp, NFS_TRIMTAIL);
			}
			m_pwndList->SetItemText(_nIndex, TFCOL_PROGRAMSIZE, _strTmp);

			// 修改时间
			_strTmp = _pFileNode->nLastTime.Format("%Y-%m-%d  %H:%M");
			m_pwndList->SetItemText(_nIndex, TFCOL_MODEFYTIME, _strTmp);
		}

		m_pwndList->SetItemData(_nIndex, (LPARAM)_nIndex);
	}

	for (_nIndex = _nCountUser; _nIndex < c_nDEFFILES_NUM; _nIndex++)
	{
		AdvFileNode* _pFileNode = &m_pAdvTask->m_vecFiles.at(_nIndex);
		if (_pFileNode->szFileName[0] != 0)
		{
			if (_pFileNode->bEnable) 
			{
				m_pwndList->SetCheck(_nIndex, false);
			}
		}
	}

	// 设置焦点
	SetListFocus();
}

// 获取文件目录下加工程序文件个数
UINT CAdvProgramTaskPage::GetTaskFileCount(CString& strPath_)
{
	TCHAR _szPath[MAX_PATH] = {0};
	_tcsncpy_s(_szPath, strPath_, _countof(_szPath) - 1);
	PathAddBackslash(_szPath);
	CString _strFilter = _szPath;
	_strFilter += "*.*";
	CFileFind _finder;
	BOOL _bWorking = _finder.FindFile(_strFilter);
	UINT _nFilesCount = 0;
	while (_bWorking)
	{
		_bWorking = _finder.FindNextFile();
		if (_finder.IsDots() 
			|| _finder.IsTemporary() 
			|| (_finder.IsDirectory() == TRUE))
		{
			continue;
		}

		CString _strFileName = _finder.GetFileName();
		if (FileExtensionCheck(_strFileName))
		{
			_nFilesCount++;
		}
	}

	return _nFilesCount;
}

bool CAdvProgramTaskPage::FileExtensionCheck(CString& strFileName_)
{
	CString _strFileName = strFileName_;
	LPTSTR _pszExt = PathFindExtension(_strFileName);
	for (int _i = 0; _i < _countof(c_IncludeExt); _i++)
	{
		if (!strnicmp(_pszExt, c_IncludeExt[_i].pszExt, c_IncludeExt[_i].nSize))
		{
			return true;
		}
	}

	return false;
}

// 设置默认工艺信息，可修改
// 加工文件不存在时，其他内容为空
void CAdvProgramTaskPage::SetDefaultTechInfo()
{
	m_pAdvTask->ReSet();

	// 记录当前根目录
	int _nCount = _countof(m_pAdvTask->szFoldPath);
	_tcsncpy_s(m_pAdvTask->szFoldPath, m_szCurFolder, _nCount - 1);

	// 设置默认文件信息
	for (int _i = 0; _i < c_nDEFFILES_NUM; _i++)
	{
		AdvFileNode _FileNode;
		_FileNode.nDefFileID = _i;
		DefFileInfo* _pDefFileInfo = GetDefFileInfo() + _i;
		int _nCount = _countof(_FileNode.szTechName);
		// 默认工艺名
		_tcsncpy_s(_FileNode.szTechName, _pDefFileInfo->szTech, _nCount - 1);
		m_pAdvTask->m_vecFiles.push_back(_FileNode);
	}
}

AdvFileNode* CAdvProgramTaskPage::MatchFileName(CString& strFileName_)
{
	AdvFileNode* _pFileNode = NULL;
	CString _strFileName = strFileName_;
	int _nIndex = _strFileName.ReverseFind('.');
	CString _strTmp = _strFileName;
	if (_nIndex != -1)
	{
		_strTmp = _strFileName.Left(_nIndex);
	}

	if (m_bReadFileInfoSucc)
	{
		// 读取AdvTask.tsklst文件成功时，
		// 匹配m_vecFiles中文件名
		int _nCount = m_pAdvTask->m_vecFiles.size();
		for (int _i = 0; _i < _nCount; _i++)
		{
			// 匹配当前文件是否已存在于文件表里
			_pFileNode = &m_pAdvTask->m_vecFiles.at(_i);
			if (_strFileName.CompareNoCase(_pFileNode->szFileName) == 0)
			{
				return _pFileNode;
			}

			// 检查当前文件是否与文件表里文件名为空的默认工艺文件匹配名匹配
			if (_pFileNode->nDefFileID != c_nNOTDEFFILE 
				&& _pFileNode->szFileName[0] == 0)
			{
				DefFileInfo* _pDefFileInfo = GetDefFileInfo() + _pFileNode->nDefFileID;
				if (_strTmp.CompareNoCase(_pDefFileInfo->szFile) == 0)
				{
					int _nCount = _countof(_pFileNode->szFileName) - 1;
					// 文件名
					_tcsncpy_s(_pFileNode->szFileName, _strFileName, _nCount);
					// 默认工件坐标系
					_pFileNode->nCoorNo = _pDefFileInfo->nCoorNo;
					// 默认A轴工件坐标
					_pFileNode->nWC = _pDefFileInfo->nWC;
					return _pFileNode;
				}
			}
		}

		return NULL;
	}

	// 读取AdvTask.tsklst文件失败或没读取时，
	// 匹配默认文件，并设置默认文件信息
	for (int _i = 0; _i < c_nDEFFILES_NUM; _i++)
	{
		DefFileInfo* _pDefFileInfo = GetDefFileInfo() + _i;
		int _nRet = _strTmp.CompareNoCase(_pDefFileInfo->szFile);
		if (_nRet == 0)
		{
			_pFileNode = &m_pAdvTask->m_vecFiles.at(_i);
			// 默认文件名存在时，默认勾选
			_pFileNode->bEnable = true;
			int _nCount = _countof(_pFileNode->szFileName) - 1;
			// 文件名
			_tcsncpy_s(_pFileNode->szFileName, _strFileName, _nCount);
			// 默认工件坐标系
			_pFileNode->nCoorNo = _pDefFileInfo->nCoorNo;
			// 默认A轴工件坐标
			_pFileNode->nWC = _pDefFileInfo->nWC;
			return _pFileNode;
		}
	}

	return _pFileNode;
}

// 显示当前目录及磁盘空间状况
void CAdvProgramTaskPage::ShowPathFileInfo()
{
	CString _strFolder = m_szCurFolder;
	if (m_pwndPath != NULL)
	{
		m_pwndPath->SetText(_strFolder);
	}

	CString _strRootPath = m_szCurFolder;
	int _pos = _strFolder.Find(":");
	_strRootPath = _strRootPath.Left(_pos + 1);
	int _nLength = _strRootPath.GetLength();
	LPSTR _pszRootPath = _strRootPath.GetBuffer(_nLength + 3);
	_pszRootPath[_nLength] = '\\';
	_pszRootPath[_nLength + 1] = '\\';
	_pszRootPath[_nLength + 2] = '\0';

	ULARGE_INTEGER FreeAv;
	ULARGE_INTEGER TotalBytes;
	ULARGE_INTEGER FreeBytes;
	if(::GetDiskFreeSpaceEx(_pszRootPath,&FreeAv,&TotalBytes,&FreeBytes))
	{
		CString _strTotalBytes;
		CString _strFreeBytes;
		_strTotalBytes.Format(_T("%uM"), TotalBytes.QuadPart / 1024 / 1024);
		_strFreeBytes.Format(_T("%uM"), FreeBytes.QuadPart / 1024 / 1024);
		_strFreeBytes += " / ";
		_strFreeBytes += _strTotalBytes;

		if (m_pwndDiskSpace != NULL)
		{
			m_pwndDiskSpace->SetText(_strFreeBytes);
		}
	}
}

// 更新当前路径
void CAdvProgramTaskPage::UpdateCurFold(CString& strFoldName_)
{
	bool _bFolderChange = false;
	CString _strFolder = m_szCurFolder;
	if (strFoldName_ == "..")
	{
		// 返回上一级目录
		int _nIndex = _strFolder.ReverseFind('\\');
		if (_nIndex != -1)
		{
			if (_nIndex == _strFolder.GetLength() - 1)
			{
				_strFolder = _strFolder.Left(_nIndex);
				_nIndex = _strFolder.ReverseFind('\\');
				if (_nIndex != -1)
				{
					_strFolder = _strFolder.Left(_nIndex);
					_bFolderChange = true;
				}
			}
			else
			{
				_strFolder = _strFolder.Left(_nIndex);
				_bFolderChange = true;
			}
		}
	}
	else
	{
		// 进入下一级目录
		if (_strFolder.Right(1) != "\\")
		{
			_strFolder += "\\";
		}
		_strFolder += strFoldName_;
		_bFolderChange = true;
	}

	if (_strFolder.Right(1) != "\\")
	{
		_strFolder += "\\";
	}

	sprintf(m_szCurFolder, "%s", _strFolder);

	if (_bFolderChange)
	{
		CreateFolderChangeNotification();
	}
}

// 设置焦点
void CAdvProgramTaskPage::SetListFocus()
{
	if (m_pwndList == NULL)
	{
		ASSERT(FALSE);
		return;
	}

	// 进入加工任务列表时，焦点永远在第一行
	int _nFocusItem = -1;
	if (m_nListShow == LIST_FOLDER)
	{
		_nFocusItem = m_FocusItem;
	}

	// 设置焦点
	int _nLastItem = m_pwndList->GetItemCount() - 1;
	if ((_nFocusItem < 0) || (_nFocusItem > _nLastItem))
	{
		_nFocusItem = 0;
	}
	m_pwndList->SetItemState(_nFocusItem, LVIS_FOCUSED | LVIS_SELECTED, 
		LVIS_FOCUSED | LVIS_SELECTED);
}

bool CAdvProgramTaskPage::TechNameCheck(CString& strTechName_)
{
	// 检查工艺名是否为空
	if (strTechName_.IsEmpty())
	{
		ShowMessage(_GETCS(s_csMODIFYTECHNAMENULL));
		return false;
	}

	// 检查工艺名是否重名
	bool _bHasSameName = false;
	for (int _i = 0; _i < c_nDEFFILES_NUM; _i++)
	{
		DefFileInfo* _pDefFileInfo = GetDefFileInfo() + _i;
		if (strTechName_.CompareNoCase(_pDefFileInfo->szTech) == 0)
		{
			_bHasSameName = true;
			break;
		}
	}

	if (_bHasSameName)
	{
		ShowMessage(_GETCS(s_csMODIFYTECHNAMESAME));
		return false;
	}

	return true;
}

bool CAdvProgramTaskPage::MatchFileNameCheck(CString& strFileName_)
{
	// 检查文件匹配名是否为空
	if (strFileName_.IsEmpty())
	{
		ShowMessage(_GETCS(s_csMODIFYFILENAMENULL));
		return false;
	}

	// 检查文件匹配名是否重名
	bool _bHasSameName = false;
	for (int _i = 0; _i < c_nDEFFILES_NUM; _i++)
	{
		DefFileInfo* _pDefFileInfo = GetDefFileInfo() + _i;
		if (strFileName_.CompareNoCase(_pDefFileInfo->szFile) == 0)
		{
			_bHasSameName = true;
			break;
		}
	}

	if (_bHasSameName)
	{
		ShowMessage(_GETCS(s_csMODIFYFILENAMESAME));
		return false;
	}

	return true;
}

void CAdvProgramTaskPage::OnSelItem(NMHDR* pNMHDR_, LRESULT* pResult_)
{
	*pResult_ = FALSE;
	int _nItem = m_pwndList->GetNextItem(-1, LVNI_SELECTED);
	if (_nItem == -1)
	{
		return;
	}

	// 文件目录操作
	if (m_nListShow == LIST_FOLDER)
	{
		CString _strName = m_pwndList->GetItemText(_nItem, FCOL_NAME);
		if (_strName.IsEmpty())
		{
			return;
		}

		// 记录当前文件夹目录选中项
		m_FocusItem = _nItem;

		// 更新当前路径
		UpdateCurFold(_strName);

		if (_strName == "..")
		{
			ShowListContent(LIST_FOLDER);
		}
		else
		{
			ShowListContent(LIST_TASKFILE);
		}

		return;
	}

	// 加工任务文件操作
	BOOL _bCheck = m_pwndList->GetCheck(_nItem);
	CString _strFileName = m_pwndList->GetItemText(_nItem, TFCOL_PROGRAMNAME);
	if (_strFileName.IsEmpty() && (_bCheck == FALSE))
	{
		// 无文件时，不允许勾选
		return;
	}
	
	m_pwndList->SetCheck(_nItem, !_bCheck);
	int _nData = m_pwndList->GetItemData(_nItem);
	AdvFileNode*_pFileNode = &m_pAdvTask->m_vecFiles.at(_nData);
	_pFileNode->bEnable = (_bCheck == TRUE) ? false : true;
}

// 键盘Backspace
void CAdvProgramTaskPage::OnBackToPreviousDir(NMHDR* pNMHDR_, LRESULT* pResult_)
{
	OnBackToFolder();
}

void CAdvProgramTaskPage::OnUpdateDeleteFile(CCmdUI* pCmdUI_)
{
	CStringList m_listNcfiles;
	GetSelFiles(m_listNcfiles);

	if (!m_listNcfiles.GetCount() 
		|| !CheckDiskExist(m_szCurFolder))
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	POSITION _pos = m_listNcfiles.GetHeadPosition();
	while (_pos != NULL)
	{
		CString _strName = m_listNcfiles.GetNext(_pos);
		if (_strName.IsEmpty())
		{
			pCmdUI_->Enable(FALSE);
			return;
		}
	}

	pCmdUI_->Enable(TRUE);
}

// 装载
void CAdvProgramTaskPage::OnLoadTask()
{
	CNcKernel* _pNcKernel = GetNcKernel();
	if (_pNcKernel->IsSimulating())
	{
		ShowMessage(_GETCS(s_csISSIMULATING));
		return;
	}
	else if(!_pNcKernel->IsNcKernelIdle())
	{
		ShowMessage(_GETCS(s_csINVALIDMS));
		return;
	}

	CNcAdvTaskMngTarget* _pTarget = GetNcAdvTaskMngTarget();
	if (_pTarget == NULL)
	{
		return;
	}

	if (!_pTarget->IsAdvTaskValid())
	{
		AgileMessageBox(_GETCS(s_csCANNOTLOADINVALIDTASK));
		return;
	}

	// 转载时自动保存
	OnSaveTaskInfo();

	// 设置高级加工任务
	_pTarget->SetCurAdvTask();
}

void CAdvProgramTaskPage::OnUpdateLoadTask(CCmdUI* pCmdUI_)
{
	CNcAdvTaskMngTarget* _pTarget = GetNcAdvTaskMngTarget();
	if (_pTarget == NULL)
	{
		return;
	}

	// 没卸载不能装载
	CNcKernel* _pNcKernel = GetNcKernel();
	if (_pNcKernel->m_FileCodeBuffer.IsValidFile())
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	if (m_nListShow == LIST_FOLDER)
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	BOOL _bShow = _pTarget->IsAdvTaskValid() ? TRUE : FALSE;
	pCmdUI_->Enable(_bShow);
}

void CAdvProgramTaskPage::OnMoveUp()
{
	if (m_nListShow == LIST_FOLDER)
	{
		return;
	}

	if (m_pwndList == NULL)
	{
		return;
	}

	int _nSel = m_pwndList->GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (_nSel < 0)
	{
		ShowMessage(_GETCS(s_csNOSELECT));
		return;
	}

	if (_nSel > 0)
	{
		int _nItem = m_pwndList->GetItemData(_nSel);
		AdvFileNode*_pNodeU = &m_pAdvTask->m_vecFiles.at(_nItem - 1);
		AdvFileNode*_pNodeD = &m_pAdvTask->m_vecFiles.at(_nItem);
		AdvFileNode _NodeM;
		memcpy(&_NodeM, _pNodeU, sizeof(AdvFileNode));
		memcpy(_pNodeU, _pNodeD, sizeof(AdvFileNode));
		memcpy(_pNodeD, &_NodeM, sizeof(AdvFileNode));

		UpdateTaskFileList();

		// 除去选中状态
		POSITION _pos = m_pwndList->GetFirstSelectedItemPosition();
		while (_pos != NULL)
		{
			int _nSelItem = m_pwndList->GetNextSelectedItem(_pos);
			m_pwndList->SetItemState(_nSelItem, 0, LVIS_SELECTED | LVIS_FOCUSED);
		}

		// 焦点保持在之前的文件上
		m_pwndList->SetItemState(_nSel - 1, LVIS_FOCUSED | LVIS_SELECTED,	LVIS_FOCUSED | LVIS_SELECTED);
	}
}

void CAdvProgramTaskPage::OnUpdateMoveUp(CCmdUI* pCmdUI_)
{
	if (m_nListShow == LIST_FOLDER)
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	int _nSel = m_pwndList->GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (_nSel < 0)
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	pCmdUI_->Enable(TRUE);
}

void CAdvProgramTaskPage::OnMoveDown()
{
	if (m_nListShow == LIST_FOLDER)
	{
		return;
	}

	if (m_pwndList == NULL)
	{
		return;
	}

	int _nSel = m_pwndList->GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (_nSel < 0)
	{
		ShowMessage(_GETCS(s_csNOSELECT));
		return;
	}

	int _nLastItem = m_pwndList->GetItemCount() - 1;
	if (_nSel < _nLastItem)
	{
		int _nItem = m_pwndList->GetItemData(_nSel);
		AdvFileNode*_pNodeU = &m_pAdvTask->m_vecFiles.at(_nItem);
		AdvFileNode*_pNodeD = &m_pAdvTask->m_vecFiles.at(_nItem + 1);
		AdvFileNode _NodeM;
		memcpy(&_NodeM, _pNodeD, sizeof(AdvFileNode));
		memcpy(_pNodeD, _pNodeU, sizeof(AdvFileNode));
		memcpy(_pNodeU, &_NodeM, sizeof(AdvFileNode));

		UpdateTaskFileList();

		// 除去选中状态
		POSITION _pos = m_pwndList->GetFirstSelectedItemPosition();
		while (_pos != NULL)
		{
			int _nSelItem = m_pwndList->GetNextSelectedItem(_pos);
			m_pwndList->SetItemState(_nSelItem, 0, LVIS_SELECTED | LVIS_FOCUSED);
		}

		// 焦点保持在之前的文件上
		m_pwndList->SetItemState(_nSel + 1, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
}

void CAdvProgramTaskPage::OnUpdateMoveDown(CCmdUI* pCmdUI_)
{
	if (m_nListShow == LIST_FOLDER)
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	int _nSel = m_pwndList->GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (_nSel < 0)
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	pCmdUI_->Enable(TRUE);
}

void CAdvProgramTaskPage::OnModifyTechName()
{
	if (m_nListShow == LIST_FOLDER)
	{
		return;
	}

	if (m_pwndList == NULL)
	{
		return;
	}

	int _nSel = m_pwndList->GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (_nSel < 0)
	{
		ShowMessage(_GETCS(s_csNOSELECT));
		return;
	}

	int _nItem = m_pwndList->GetItemData(_nSel);
	AdvFileNode*_pFileNode = &m_pAdvTask->m_vecFiles.at(_nItem);
	DefFileInfo* _pDefFileInfo = GetDefFileInfo() + _pFileNode->nDefFileID;
	CString _strValue = _pDefFileInfo->szTech;
	CString _strPrompt;
	_strPrompt.Format(_GETCS(s_szMODIFYTECHNAME), _pDefFileInfo->szFile);
	int _nCount = _countof(_pDefFileInfo->szTech);
	if (GetInput()->InputValue(_strPrompt, _strValue, _nCount - 1) != IDOK)
	{
		return;
	}

	// 工艺名检查
	if (!TechNameCheck(_strValue))
	{
		return;
	}

	// 修改默认文件信息
	_tcsncpy_s(_pDefFileInfo->szTech, _strValue, _nCount - 1);
	_nCount = _countof(_pFileNode->szTechName);
	_tcsncpy_s(_pFileNode->szTechName, _strValue, _nCount - 1);

	// 刷新List表
	UpdateTaskFileList();

	m_bModifyDefFileInfo = true;
}

void CAdvProgramTaskPage::OnUpdateModifyTechName(CCmdUI* pCmdUI_)
{
	if (m_nListShow == LIST_FOLDER)
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	int _nSel = m_pwndList->GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (_nSel < 0)
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	int _nItem = m_pwndList->GetItemData(_nSel);
	AdvFileNode*_pFileNode = &m_pAdvTask->m_vecFiles.at(_nItem);
	if (_pFileNode->nDefFileID == c_nNOTDEFFILE)
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	pCmdUI_->Enable(TRUE);
}

void CAdvProgramTaskPage::OnModifyFileMatchName()
{
	if (m_nListShow == LIST_FOLDER)
	{
		return;
	}

	if (m_pwndList == NULL)
	{
		return;
	}

	int _nSel = m_pwndList->GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (_nSel < 0)
	{
		ShowMessage(_GETCS(s_csNOSELECT));
		return;
	}

	int _nItem = m_pwndList->GetItemData(_nSel);
	AdvFileNode*_pFileNode = &m_pAdvTask->m_vecFiles.at(_nItem);
	DefFileInfo* _pDefFileInfo = GetDefFileInfo() + _pFileNode->nDefFileID;
	CString _strValue = _pDefFileInfo->szFile;
	CString _strPrompt;
	_strPrompt.Format(_GETCS(s_szMODIFYFILEMATCHNAME), _pDefFileInfo->szTech);
	int _nCount = _countof(_pDefFileInfo->szFile);
	if (GetInput()->InputValue(_strPrompt, _strValue, _nCount - 1) != IDOK)
	{
		return;
	}

	// 文件匹配名检查
	if (!MatchFileNameCheck(_strValue))
	{
		return;
	}

	// 修改默认文件信息
	_tcsncpy_s(_pDefFileInfo->szFile, _strValue, _nCount - 1);

	// 修改文件匹配名后需重新匹配
	UpdateListContent();

	m_bModifyDefFileInfo = true;
}

void CAdvProgramTaskPage::OnUpdateModifyFileMatchName(CCmdUI* pCmdUI_)
{
	if (m_nListShow == LIST_FOLDER)
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	int _nSel = m_pwndList->GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (_nSel < 0)
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	int _nItem = m_pwndList->GetItemData(_nSel);
	AdvFileNode*_pFileNode = &m_pAdvTask->m_vecFiles.at(_nItem);
	if (_pFileNode->nDefFileID == c_nNOTDEFFILE)
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	pCmdUI_->Enable(TRUE);
}

// 修改
void CAdvProgramTaskPage::OnUpdateModify(CCmdUI* pCmdUI_)
{
	if (m_nListShow == LIST_FOLDER)
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	CNcKernel* _pNcKernel = GetNcKernel();
	if (_pNcKernel->m_FileCodeBuffer.IsValidFile())
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	pCmdUI_->Enable(TRUE);
}

void CAdvProgramTaskPage::OnModifyCoorNo()
{
	if (m_nListShow == LIST_FOLDER)
	{
		return;
	}

	if (m_pwndList == NULL)
	{
		return;
	}

	int _nSel = m_pwndList->GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (_nSel < 0)
	{
		ShowMessage(_GETCS(s_csNOSELECT));
		return;
	}

	int _nItem = m_pwndList->GetItemData(_nSel);
	AdvFileNode*_pFileNode = &m_pAdvTask->m_vecFiles.at(_nItem);
	CSetCoorDialog _dlg;
	_dlg.m_nCoor = (int)_pFileNode->nCoorNo;
	if (_dlg.DoModal() != IDOK)
	{
		return;
	}

	_pFileNode->nCoorNo = (workcoor_t)_dlg.m_nCoor;
	UpdateTaskFileList();
}

void CAdvProgramTaskPage::OnUpdateModifyCoorNo(CCmdUI* pCmdUI_)
{
	if (m_nListShow == LIST_FOLDER)
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	int _nSel = m_pwndList->GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (_nSel < 0)
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	pCmdUI_->Enable(TRUE);
}

void CAdvProgramTaskPage::OnModifyWorkCoorA()
{
	if (m_nListShow == LIST_FOLDER)
	{
		return;
	}

	if (m_pwndList == NULL)
	{
		return;
	}

	int _nSel = m_pwndList->GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (_nSel < 0)
	{
		ShowMessage(_GETCS(s_csNOSELECT));
		return;
	}

	int _nItem = m_pwndList->GetItemData(_nSel);
	AdvFileNode*_pFileNode = &m_pAdvTask->m_vecFiles.at(_nItem);
	double _nValue = _pFileNode->nWC;
	if (GetInput()->InputValue(_GETCS(s_szMODIFYWCA), _nValue, -100000, 100000) != IDOK)
	{
		return;
	}

	_pFileNode->nWC = _nValue;
	UpdateTaskFileList();
}

void CAdvProgramTaskPage::OnUpdateModifyWorkCoorA(CCmdUI* pCmdUI_)
{
	if (m_nListShow == LIST_FOLDER)
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	int _nSel = m_pwndList->GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (_nSel < 0)
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	pCmdUI_->Enable(TRUE);
}

// 保存
void CAdvProgramTaskPage::OnSaveTaskInfo()
{
	CNcAdvTaskMngTarget* _pTarget = GetNcAdvTaskMngTarget();
	if (_pTarget == NULL)
	{
		return;
	}

	if (m_pwndList == NULL)
	{
		return;
	}

	int _nSize = m_pwndList->GetItemCount();
	for (int _i = 0; _i < _nSize; _i++)
	{
		int _nItem = m_pwndList->GetItemData(_i);
		AdvFileNode* _pFileNode = &m_pAdvTask->m_vecFiles.at(_nItem);
		_pFileNode->bEnable = m_pwndList->GetCheck(_i);
	}

	if (m_bModifyDefFileInfo)
	{
		// 保存高级加工任务默认工艺信息
		_pTarget->SaveDefFileInfo();
		m_bModifyDefFileInfo = false;
	}

	_pTarget->SaveAdvTaskList(m_szCurFolder);
}

void CAdvProgramTaskPage::OnUpdateSaveTaskInfo(CCmdUI* pCmdUI_)
{
	if (m_nListShow == LIST_FOLDER)
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	if (m_pAdvTask->m_vecFiles.empty())
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	pCmdUI_->Enable(TRUE);
}

// 返回
void CAdvProgramTaskPage::OnBackToFolder()
{
	if (m_nListShow == LIST_FOLDER)
	{
		return;
	}

	if (m_szCurFolder[0] == 0
		|| m_pszCurPath == NULL
		|| *m_pszCurPath == 0
		|| PathIsEqual(m_szCurFolder, m_pszCurPath))
	{
		return;
	}

	if (m_nListShow == LIST_TASKFILE)
	{
		// 更新当前路径到上一级
		CString _strTmp = _T("..");
		UpdateCurFold(_strTmp);
		ShowListContent(LIST_FOLDER);
	}

	// 退回到上一级后焦点复位
	m_FocusItem = 0;
}

void CAdvProgramTaskPage::OnUpdateBackToFolder(CCmdUI* pCmdUI_)
{
	if (m_nListShow == LIST_FOLDER)
	{
		pCmdUI_->Enable(FALSE);
		return;
	}

	pCmdUI_->Enable(TRUE);
}

void CAdvProgramTaskPage::OnTimer(UINT_PTR nIDEvent_)
{
	if (nIDEvent_ == c_IDT_FILECHANGE)
	{
		if (m_bAlreadyInTimer)
		{
			ASSERT(false);
			return;
		}

		CReentryGuard _guard(m_bAlreadyInTimer);
		CheckFileChanges();
	}
}
