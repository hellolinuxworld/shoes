// Copyright (c) Naiky Company. All rights reserved.
// 高级加工任务
// 进入文件目录直接编辑加工任务
//
#pragma once
#include <vector>
#include "ProgramPage.h"
#include "..\Agile\AgileLabel.h"
#include "..\NcuiBase\NcAdvTaskMngTarget.h"

enum //range:[43460, 43500)
{
	IDCMD_ADVPROGTASK_LOADTASK = 43460,
	IDCMD_ADVPROGTASK_DELETE = 43462,
	IDCMD_ADVPROGTASK_MOVEUP = 43463,
	IDCMD_ADVPROGTASK_MOVEDOWN = 43464,
	IDCMD_ADVPROGTASK_MODIFY = 43465,
	IDCMD_ADVPROGTASK_SAVETASKINFO = 43466,
	IDCMD_ADVPROGTASK_BACK = 43467,

	IDCMD_ADVPROGTASK_MODIFYTECHNAME = 43470,
	IDCMD_ADVPROGTASK_MODIFYFILEMATCHNAME = 43471,
	IDCMD_ADVPROGTASK_MODIFYCOORNO = 43472,
	IDCMD_ADVPROGTASK_MODIFYWORKCOORA = 43473,
};

enum list_t
{
	LIST_FOLDER,
	LIST_TASKFILE,
};

class CNcAdvTaskMngTarget;
class CAdvProgramTaskPage : public CProgramPage
{
	DECLARE_SERIAL(CAdvProgramTaskPage)

public:
	CAdvProgramTaskPage(void);
	virtual ~CAdvProgramTaskPage(void);

public:
	void OnInitPage();
	// GetSelFiles函数重写
	void GetSelFiles(CStringList& Files_);
	// UpdateListContent函数重写，显示文件目录
	void UpdateListContent(bool bSelRst_ = false);

protected:
	afx_msg void OnTimer(UINT_PTR nIDEvent_);
	afx_msg void OnShowWindow(BOOL bShow_, UINT nStatus_);
	// OnSelItem函数重写
	afx_msg void OnSelItem(NMHDR* pNMHDR_, LRESULT* pResult_);
	// OnBackToPreviousDir函数重写
	afx_msg void OnBackToPreviousDir(NMHDR* pNMHDR_, LRESULT* pResult_);
	// OnUpdateDeleteFile函数重写
	afx_msg void OnUpdateDeleteFile(CCmdUI* pCmdUI_);
	afx_msg void OnLoadTask();
	afx_msg void OnUpdateLoadTask(CCmdUI* pCmdUI_);
	afx_msg void OnMoveUp();
	afx_msg void OnUpdateMoveUp(CCmdUI* pCmdUI_);
	afx_msg void OnMoveDown();
	afx_msg void OnUpdateMoveDown(CCmdUI* pCmdUI_);
	afx_msg void OnUpdateModify(CCmdUI* pCmdUI_);
	afx_msg void OnModifyTechName();
	afx_msg void OnUpdateModifyTechName(CCmdUI* pCmdUI_);
	afx_msg void OnModifyFileMatchName();
	afx_msg void OnUpdateModifyFileMatchName(CCmdUI* pCmdUI_);
	afx_msg void OnModifyCoorNo();
	afx_msg void OnUpdateModifyCoorNo(CCmdUI* pCmdUI_);
	afx_msg void OnModifyWorkCoorA();
	afx_msg void OnUpdateModifyWorkCoorA(CCmdUI* pCmdUI_);
	afx_msg void OnSaveTaskInfo();
	afx_msg void OnUpdateSaveTaskInfo(CCmdUI* pCmdUI_);
	afx_msg void OnBackToFolder();
	afx_msg void OnUpdateBackToFolder(CCmdUI* pCmdUI_);

	DECLARE_MESSAGE_MAP()

private:
	// 初始化列表信息：文件目录显示与加工文件显示切换
	void InitListBox();
	void ShowListContent(list_t list_);
	// 读取文件目录
	void ReadFolderList();
	// 更新文件目录列表
	void UpdateFolderList(bool bSelRst_ = false);
	// 读取加工任务文件列表
	void ReadTaskFileList();
	// 读取的加工任务文件列表数据检查
	void TaskFileListCheck();
	// 更新加工任务文件列表
	void UpdateTaskFileList(bool bSelRst_ = false);
	// 获取文件目录下加工程序文件数
	UINT GetTaskFileCount(CString& strPath_);
	// 文件扩展名检查
	bool FileExtensionCheck(CString& strFileName_);
	// 加工任务设置默认工艺信息
	void SetDefaultTechInfo();
	AdvFileNode* MatchFileName(CString& strFileName_);
	// 显示当前目录及磁盘空间状况
	void ShowPathFileInfo();
	// 更新当前路径
	void UpdateCurFold(CString& strFoldName_);
	// 设置焦点
	void SetListFocus();
	// 检查工艺名
	bool TechNameCheck(CString& strTechName_);
	// 检查文件匹配名
	bool MatchFileNameCheck(CString& strFileName_);

private:
	enum
	{
		// 文件目录
		FCOL_NAME = 0,				// 文件目录名
		FCOL_PROGRAMNUMS,			// 加工程序文件数
		FCOL_MODEFYTIME,			// 修改时间
		NUMOF_FCOL,

		// 加工任务文件列表
		TFCOL_TECHNOLOGY = 0,		// 工艺
		TFCOL_PROGRAMNAME,			// 程序文件名
		TFCOL_COORNO,				// 坐标系
		TFCOL_MACHCOORA,			// A轴机械坐标位置
		TFCOL_PROGRAMSIZE,			// 程序文件大小
		TFCOL_MODEFYTIME,			// 修改时间
		NUMOF_TFCOL,
	};

private:
	// 标记当前是否进入加工任务编辑状态
	list_t m_nListShow;
	AdvTaskNode* m_pAdvTask;

	// 文件目录列表
	// 使用基类m_pwndList，只显示文件目录
	CAgileLabel* m_pwndFLHead[NUMOF_FCOL];
	// 加工任务文件列表
	CAgileLabel* m_pwndTFLHead[NUMOF_TFCOL];
	//CAgileDrawListBox* m_pwndTaskFileList;

	// 本机程序路径
	CAgileLabel* m_pwndPath;
	// 驱动器空间：剩余空间/总空间
	CAgileLabel* m_pwndDiskSpace;

	bool m_bAlreadyInTimer;

	// List表属性
	DWORD m_nExtendedStyle;
	CImageList m_ImageList;

	// 读取文件成功
	bool m_bReadFileInfoSucc;
	// 修改默认工艺信息：
	// 工艺名和加工文件匹配名
	bool m_bModifyDefFileInfo;
};

