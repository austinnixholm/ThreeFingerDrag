#pragma once

#include <string>
#include <iostream>
#include <comdef.h>
#pragma comment(lib, "taskschd.lib")
#include <taskschd.h>
#include <Lmcons.h>

class TaskScheduler {
public:
    static bool TaskExists(const std::string& taskName);
    static bool CreateLoginTask(const std::string& taskName, const std::string& exePath);
    static void DeleteTask(const std::string& taskName);
};

bool TaskScheduler::CreateLoginTask(const std::string& taskName, const std::string& exePath) {
    // Get current user name
    wchar_t username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    GetUserName(username, &username_len);

    // Initialize COM
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // Create Task Service
    ITaskService* pService = NULL;
    CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);

    // Connect to Task Service
    pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());

    // Get Task Folder
    ITaskFolder* pRootFolder = NULL;
    pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);

    // Create Task Definition
    ITaskDefinition* pTask = NULL;
    pService->NewTask(0, &pTask);

    // Set Task Registration Information
    IRegistrationInfo* pRegInfo = NULL;
    pTask->get_RegistrationInfo(&pRegInfo);
    pRegInfo->put_Author(SysAllocString(username));

    // Create Task Trigger
    ITriggerCollection* pTriggerCollection = NULL;
    pTask->get_Triggers(&pTriggerCollection);
    ITrigger* pTrigger = NULL;
    pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);
    ILogonTrigger* pLogonTrigger = NULL;
    pTrigger->QueryInterface(IID_ILogonTrigger, (void**)&pLogonTrigger);
    pLogonTrigger->put_UserId(_bstr_t(username));

    // Create Task Action
    IActionCollection* pActionCollection = NULL;
    pTask->get_Actions(&pActionCollection);
    IAction* pAction = NULL;
    pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
    IExecAction* pExecAction = NULL;
    pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
    pExecAction->put_Path(_bstr_t(exePath.c_str()));

    // Register Task
    IRegisteredTask* pRegisteredTask = NULL;
    HRESULT hr = pRootFolder->RegisterTaskDefinition(_bstr_t(taskName.c_str()), pTask, TASK_CREATE_OR_UPDATE, _variant_t(), _variant_t(), TASK_LOGON_INTERACTIVE_TOKEN, _variant_t(), &pRegisteredTask);

    if (FAILED(hr)) {
        _com_error err(hr);
        LPCTSTR errMsg = err.ErrorMessage();
        std::wcout << L"Failed to create task. Error: " << errMsg << std::endl;
        return false;
    }
    pRegisteredTask->Release();
    pExecAction->Release();
    pAction->Release();
    pActionCollection->Release();
    pLogonTrigger->Release();
    pTrigger->Release();
    pTriggerCollection->Release();
    pRegInfo->Release();
    pTask->Release();
    pRootFolder->Release();
    pService->Release();
    CoUninitialize();
    return true;
}

void TaskScheduler::DeleteTask(const std::string& taskName) {
    // Initialize COM
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // Create Task Service
    ITaskService* pService = NULL;
    CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);

    // Connect to Task Service
    pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());

    // Get Task Folder
    ITaskFolder* pRootFolder = NULL;
    pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);

    // Delete the task
    pRootFolder->DeleteTask(_bstr_t(taskName.c_str()), 0);

    // Release COM objects
    pRootFolder->Release();
    pService->Release();
    CoUninitialize();
}

bool TaskScheduler::TaskExists(const std::string& taskName) {
    // Initialize COM
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // Create Task Service
    ITaskService* pService = NULL;
    CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);

    // Connect to Task Service
    pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());

    // Get Task Folder
    ITaskFolder* pRootFolder = NULL;
    pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);

    // Check if the task exists
    IRegisteredTask* pRegisteredTask = NULL;
    HRESULT hr = pRootFolder->GetTask(_bstr_t(taskName.c_str()), &pRegisteredTask);

    if (!SUCCEEDED(hr)) {
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    // Release COM objects
    pRegisteredTask->Release();
    pRootFolder->Release();
    pService->Release();
    CoUninitialize();
    return SUCCEEDED(hr);
}
