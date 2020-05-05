// Copyright (c) Naiky Company. All rights reserved.
//
// 高级加工任务管理，用于鞋模机
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NcuiBase.h"
#include "NcuiBasePriv.h"
#include "LangManagerTarget.h"
#include "NcAdvTaskMngTarget.h"
#include "NcAutoTarget.h"
#include "NcShell.h"
#include "..\NcCore\NcMachTask.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TASKINFOFILE		_T("AdvTask.tsklst")
#define TASKDEFFILEINFO		_T("AdvTask.definfo")

CS_T s_csSUCCEEDLOADTASKLIST = _DEFCS(_CSV(2060),
	_CHINESE("|M|成功加载任务列表！")
	_ENGLISH("|M|Successfully load task list!"));
CS_T s_csSUCCEEDLOADDEFFILEINFO = _DEFCS(_CSV(6697),
	_CHINESE("|M|成功加载任务默认工艺信息！")
	_ENGLISH("|M|Succeeded in loading default technic information!"));
CS_T s_csCURTASKTOOBIG = _DEFCS(_CSV(6689),
	_CHINESE("|M|当前加工任务文件数超限！")
	_ENGLISH("|M|Number of current machining files exceeds the limit!"));
CS_T s_csFAILEDTOLOADTASK = _DEFCS(_CSV(2061),
	_CHINESE("装载任务失败！")
	_ENGLISH("failed to load task!"));

CNcAdvTaskMngTarget* CNcAdvTaskMngTarget::m_pTarget = NULL;

// 默认文件信息
DefFileInfo CNcAdvTaskMngTarget::m_DefFileInfo[c_nDEFFILES_NUM] =
{
	{_T("内仁"),	_T("TOP"),		WORKCOOR_G54,	0},
	{_T("底面"),	_T("BOT"),		WORKCOOR_G54,	0},
	{_T("内腰"),	_T("IN"),		WORKCOOR_G55,	0},
	{_T("鞋头"),	_T("FN"),		WORKCOOR_G55,	90},
	{_T("外腰"),	_T("OUT"),		WORKCOOR_G55,	180},
	{_T("鞋跟"),	_T("BK"),		WORKCOOR_G55,	270},
	{_T("工序1"),	_T("ZDY1"),		WORKCOOR_G54,	0},
	{_T("工序2"),	_T("ZDY2"),		WORKCOOR_G54,	0},
	{_T("工序3"),	_T("ZDY3"),		WORKCOOR_G54,	0},
	{_T("工序4"),	_T("ZDY4"),		WORKCOOR_G54,	0},
	{_T("工序5"),	_T("ZDY5"),		WORKCOOR_G54,	0},
	{_T("工序6"),	_T("ZDY6"),		WORKCOOR_G54,	0},
	{_T("工序7"),	_T("ZDY7"),		WORKCOOR_G54,	0},
	{_T("工序8"),	_T("ZDY8"),		WORKCOOR_G54,	0},
	{_T("工序9"),	_T("ZDY9"),		WORKCOOR_G54,	0},
	{_T("工序10"),	_T("ZDY10"),	WORKCOOR_G54,	0},
	{_T("工序11"),	_T("ZDY11"),	WORKCOOR_G54,	0},
	{_T("工序12"),	_T("ZDY12"),	WORKCOOR_G54,	0},
	{_T("工序13"),	_T("ZDY13"),	WORKCOOR_G54,	0},
	{_T("工序14"),	_T("ZDY14"),	WORKCOOR_G54,	0},
};

//英文版本下工艺名信息
const static LPCTSTR s_cszEngTech[] = {_T("Inner"), _T("Bottom"), _T("MedialView"), 
									  _T("ToeCap"), _T("LateralView"), _T("Heel")};

//////////////////////////////////////////////////////////////////////
CNcAdvTaskMngTarget* GetNcAdvTaskMngTarget()
{
	return CNcAdvTaskMngTarget::m_pTarget;
}

DefFileInfo* GetDefFileInfo()
{
	return CNcAdvTaskMngTarget::m_DefFileInfo;
}

//////////////////////////////////////////////////////////////////////
// class CNcAdvTaskMngTarget
IMPLEMENT_DYNAMIC(CNcAdvTaskMngTarget, CNcTarget)
BEGIN_MESSAGE_MAP(CNcAdvTaskMngTarget, CNcTarget)
	//{{AFX_MSG_MAP(CNcAdvTaskMngTarget)

	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CNcAdvTaskMngTarget::CNcAdvTaskMngTarget(void)
{
	int _nIndex = 0;
	m_pTarget = this;
	m_nInterval = 1.0;
	CLangManagerTarget* _pTarget = GetLangManagerTarget();
	_pTarget->GetActiveLangIndex(_nIndex);
	if (_nIndex == 1)
	{
		for (int _i = 0; _i < c_nDEFFILES_NUM; _i++)
		{
			int _nCount = _countof(m_DefFileInfo[_i].szTech);
			_tcsncpy_s(m_DefFileInfo[_i].szTech, s_cszEngTech[_i], _nCount - 1);
		}
	}
	
	// 软件启动时，加载高级加工任务默认工艺信息
	LoadDefFileInfo();
}

CNcAdvTaskMngTarget::~CNcAdvTaskMngTarget(void)
{

}

LPCTSTR CNcAdvTaskMngTarget::GetTaskInfoFileName(LPCTSTR strPath_) const
{
	static TCHAR _s_szFileName[_MAX_PATH];
	ZeroMemory(_s_szFileName, sizeof(_s_szFileName));
	_tcsncpy_s(_s_szFileName, strPath_, _MAX_PATH - 1);
	PathAddBackslash(_s_szFileName);
	PathAppend(_s_szFileName, TASKINFOFILE);
	return _s_szFileName;
}

// 保存加工任务文件信息
bool CNcAdvTaskMngTarget::SaveAdvTaskList(LPCTSTR strPath_)
{
	if (m_AdvTask.m_vecFiles.empty())
	{
		return false;
	}

	LPCTSTR _pszTaskInfo = GetTaskInfoFileName(strPath_);
	try
	{
		CWriteThroughFile _file(_pszTaskInfo, 
			CFile::modeCreate | CFile::modeReadWrite | CFile::shareDenyWrite);
		// 首先保存路径名
		TCHAR _szFoldPath[_MAX_PATH] = {0};
		_tcsncpy_s(_szFoldPath, m_AdvTask.szFoldPath, _MAX_PATH);
		_file.Write(_szFoldPath, _MAX_PATH);

		// 保存路径名下加工文件信息
		int _nCount = m_AdvTask.m_vecFiles.size();
		for (int _i = 0; _i < _nCount; _i++)
		{
			_file.Write(&m_AdvTask.m_vecFiles[_i], sizeof(AdvFileNode));
		}
		_file.Close();
	}
	catch (CFileException* e)
	{
		TCHAR _szCause[_MAX_PATH] = {0};
		e->GetErrorMessage(_szCause, _countof(_szCause));
		CString _strLog;
		_strLog.Format(_T("SaveAdvTaskInfo Error: %s >>> %s\n"), e->m_strFileName, _szCause);
		e->Delete();
		LogMessage(_strLog);
		return false;
	}

	return true;
}

bool CNcAdvTaskMngTarget::LoadAdvTaskList(LPCTSTR strPath_)
{
	LPCTSTR _pszTaskInfo = GetTaskInfoFileName(strPath_);
	if (!PathFileExists(_pszTaskInfo))
	{
		return false;
	}

	bool _bRet = true;
	try
	{
		m_AdvTask.ReSet();
		CFile _file(_pszTaskInfo, CFile::modeRead);
		// 首先读取路径名
		TCHAR _szFoldPath[_MAX_PATH] = {0};
		if (_file.Read(_szFoldPath, _MAX_PATH) == _MAX_PATH)
		{
			CString _strPath = strPath_;
			if (_strPath.CompareNoCase(_szFoldPath) != 0)
			{
				// 保存文件中文件路径与当前目录路径不一致
				_file.Close();
				return false;
			}

			_tcsncpy_s(m_AdvTask.szFoldPath, _szFoldPath, _MAX_PATH);
		}

		// 读取路径名下加工文件信息
		AdvFileNode _FileNode;
		UINT _nCount = sizeof(AdvFileNode);
		while (_file.Read(&_FileNode, _nCount) == _nCount)
		{
			m_AdvTask.m_vecFiles.push_back(_FileNode);
		}

		// 默认情况下，m_vecFiles至少有c_nDEFFILES_NUM项
		if (m_AdvTask.m_vecFiles.size() < c_nDEFFILES_NUM)
		{
			_bRet = false;
		}
		
		_file.Close();
	}
	catch (CFileException* e)
	{
		TCHAR _szCause[_MAX_PATH] = {0};
		e->GetErrorMessage(_szCause, _countof(_szCause));
		CString _strLog;
		_strLog.Format(_T("LoadAdvTaskInfo Error: %s >>> %s\n"), e->m_strFileName, _szCause);
		e->Delete();
		LogMessage(_strLog);
		_bRet = false;
	}

	if (_bRet)
	{
		LogMessage(_GETCS(s_csSUCCEEDLOADTASKLIST));
	}

	return _bRet;
}

bool CNcAdvTaskMngTarget::IsAdvTaskValid()
{
	return (GetAdvTaskCount() != 0);
}

// 获取加工任务中有效加工文件计数
int CNcAdvTaskMngTarget::GetAdvTaskCount()
{
	int _nValiTaskCount = 0;
	int _nTaskCount = m_AdvTask.m_vecFiles.size();
	for (int _i = 0; _i < _nTaskCount; _i++)
	{
		AdvFileNode* _pFileNode = &m_AdvTask.m_vecFiles.at(_i);
		if (_pFileNode->bEnable)
		{
			_nValiTaskCount++;
			break;
		}
	}

	return _nValiTaskCount;
}

void CNcAdvTaskMngTarget::SetCurAdvTask()
{
	if (GetAdvTaskCount() > c_nMaxFilesPerTask)
	{
		ShowMessage(_GETCS(s_csCURTASKTOOBIG));
		return;
	}

	TaskNode _tnCurTask;
	const CString _strFoldPath = m_AdvTask.szFoldPath;
	// 加工任务名使用当前目录文件夹名
	TCHAR _szFoldName[_MAX_PATH] = {0};
	_tcsncpy_s(_szFoldName, _strFoldPath, _MAX_PATH - 1);
	PathRemoveBackslash(_szFoldName);
	PathStripPath(_szFoldName);
	int _nCount = _countof(_tnCurTask.szTaskName);
	_tcsncpy_s(_tnCurTask.szTaskName, _szFoldName, _nCount - 1);

	// 加载加工任务文件信息
	int _nTaskCount = m_AdvTask.m_vecFiles.size();
	for (int _i = 0; _i < _nTaskCount; _i++)
	{
		AdvFileNode* _pFileNode = &m_AdvTask.m_vecFiles.at(_i);
		if (!_pFileNode->bEnable)
		{
			continue;
		}

		CString _strFullPath = _strFoldPath + _pFileNode->szFileName;
		if (PathIsDirectory(_strFullPath))
		{
			continue;
		}

		// 加工任务文件
		_nCount = _countof(_tnCurTask.fns[_i].szFileName);
		_tcsncpy_s(_tnCurTask.fns[_i].szFileName, _strFullPath, _nCount - 1);

		// 坐标系
		_tnCurTask.fns[_i].nCoor = (int)_pFileNode->nCoorNo;

		// A轴工件坐标
		_tnCurTask.fns[_i].nWC = _pFileNode->nWC;

		// 任务文件加工时间间隔，默认1s
		_tnCurTask.fns[_i].nInterval = m_nInterval;
	}

	CString _strRet;
	// 检查文件是否存在
	if (!_tnCurTask.CheckValidity(_strRet))
	{
		AgileMessageBox(_strRet + _GETCS(s_csFAILEDTOLOADTASK));
		return;
	}

	// 检查文件是否为空
	if (!_tnCurTask.CheckFileEmpty(_strRet))
	{
		AgileMessageBox(_strRet + _GETCS(s_csFAILEDTOLOADTASK));
		return;
	}

	CNcKernel* _pNcKernel = GetNcKernel();
	_pNcKernel->m_MachTask.SetCurTask(&_tnCurTask);
}

// 获取高级加工任务默认文件信息存储路径
LPCTSTR CNcAdvTaskMngTarget::GetTaskDefFileInfo() const
{
	static TCHAR _s_szDefInfoPath[_MAX_PATH] = {0};
	if (_s_szDefInfoPath[0] == 0)
	{
		_tcsncpy_s(_s_szDefInfoPath, 
			GetUserConfigFileName(TASKDEFFILEINFO), _MAX_PATH - 1);
	}
	return _s_szDefInfoPath;
}

// 保存高级加工任务默认文件信息
bool CNcAdvTaskMngTarget::SaveDefFileInfo()
{
	try
	{
		LPCTSTR _pszDefFilePath = GetTaskDefFileInfo();
		CWriteThroughFile _file(_pszDefFilePath, 
			CFile::modeCreate | CFile::modeReadWrite | CFile::shareDenyWrite);
		_file.Write(m_DefFileInfo, sizeof(DefFileInfo) * _countof(m_DefFileInfo));
		_file.Close();
	}
	catch (CFileException* e)
	{
		TCHAR _szCause[_MAX_PATH] = {0};
		e->GetErrorMessage(_szCause, _countof(_szCause));
		CString _strLog;
		_strLog.Format(_T("SaveDefFileInfo Error: %s >>> %s\n"), e->m_strFileName, _szCause);
		e->Delete();
		LogMessage(_strLog);
		return false;
	}

	return true;
}

bool CNcAdvTaskMngTarget::LoadDefFileInfo()
{
	LPCTSTR _pszDefFilePath = GetTaskDefFileInfo();
	if (!PathFileExists(_pszDefFilePath))
	{
		return false;
	}

	bool _bRet = true;
	try
	{
		CFile _file(_pszDefFilePath, CFile::modeRead);
		DefFileInfo _DefFileInfo[c_nDEFFILES_NUM];
		ZeroMemory(_DefFileInfo, sizeof(DefFileInfo) * _countof(_DefFileInfo));

		UINT _nReadSize = _file.Read(_DefFileInfo, sizeof(DefFileInfo) * _countof(_DefFileInfo));
		if (_nReadSize == sizeof(DefFileInfo) * _countof(_DefFileInfo))
		{
			memcpy(m_DefFileInfo, _DefFileInfo, sizeof(DefFileInfo) * _countof(m_DefFileInfo));
		}
		else
		{
			_bRet = false;
			CString _strLog;
			_strLog.Format(_T("LoadDefFileInfo error: %d : %d\n"), _nReadSize, sizeof(DefFileInfo) * _countof(m_DefFileInfo));
			LogMessage(_strLog);
		}

		_file.Close();
	}
	catch (CFileException* e)
	{
		TCHAR _szCause[_MAX_PATH] = {0};
		e->GetErrorMessage(_szCause, _countof(_szCause));
		CString _strLog;
		_strLog.Format(_T("LoadDefFileInfo Error: %s >>> %s\n"), e->m_strFileName, _szCause);
		e->Delete();
		LogMessage(_strLog);
		_bRet = false;
	}

	if (_bRet)
	{
		LogMessage(_GETCS(s_csSUCCEEDLOADDEFFILEINFO));
	}

	return _bRet;
}