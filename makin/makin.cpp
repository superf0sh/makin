//
// author: Lasha Khasaia
// contact: @_qaz_qaz
// license: MIT License
//

#include "stdafx.h"

#ifndef _WIN64
#define DWORD64 DWORD32 // maybe there is a better way...
#endif

enum DrReg
{
   Dr0,
   Dr1,
   Dr2,
   Dr3
};

typedef NTSTATUS (WINAPI *pNtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

std::vector<std::wstring> loadDll{};

std::vector<std::string> hookFunctions{};

inline void SetBits(DWORD_PTR& dw, int lowBit, int bits, int newValue)
{
   const int mask = (1 << bits) - 1;

   dw = (dw & ~(mask << lowBit)) | (newValue << lowBit);
}

VOID process_output_string(const PROCESS_INFORMATION pi, const OUTPUT_DEBUG_STRING_INFO out_info)
{
   std::unique_ptr<TCHAR[]> pMsg{new TCHAR[out_info.nDebugStringLength * sizeof(TCHAR)]};

   ReadProcessMemory(pi.hProcess, out_info.lpDebugStringData, pMsg.get(), out_info.nDebugStringLength, nullptr);

   const auto isUnicode = IsTextUnicode(pMsg.get(), out_info.nDebugStringLength, nullptr);

   if (!isUnicode)
      printf("[OutputDebugString] msg: %s\n\n", reinterpret_cast<char*>(pMsg.get())); // as ASCII


   auto cmdSubStr = wcsstr(pMsg.get(), L"DBG_NEW_PROC:");
   if (cmdSubStr)
   {
	   cmdSubStr += 13;
	   wprintf_s(L"Monitor new process in a new console...\n\n");

	   TCHAR curExe[0x1000]{};
	   GetModuleFileName(nullptr, curExe, 0x1000);

	   swprintf_s(curExe, L"%s %s", curExe, cmdSubStr);

	   STARTUPINFO nsi{ sizeof(STARTUPINFO) };
	   PROCESS_INFORMATION npi{};

	   CreateProcess(nullptr, curExe, nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr, &nsi, &npi);

	   return;
   }

   if (isUnicode && pMsg.get()[0] != L'[')
   {
      wprintf_s(L"[OutputDebugString] msg: %s\n\n", pMsg.get()); // raw message from the sample
   }
   else if (_tcslen(pMsg.get()) > 3 && (pMsg.get()[0] == L'[' && pMsg.get()[1] == L'_' && pMsg.get()[2] == L']')) // [_]
   {
      for (auto i = 0; i < loadDll.size(); ++i)
      {
         TCHAR tmp[MAX_PATH + 2]{};
         _tcscpy_s(tmp, MAX_PATH + 2, pMsg.get() + 3);
         const std::wstring wtmp(tmp);
         if (!wtmp.compare(loadDll[i])) // #SOURCE - The "Ultimate" Anti-Debugging Reference: 7.B.iv
         {
            hookFunctions.push_back("LdrLoadDll");
            wprintf(
               L"[LdrLoadDll] The debuggee attempts to use LdrLoadDll/NtCreateFile trick: %s\n\tref: The \"Ultimate\" Anti-Debugging Reference: 7.B.iv\n\n",
               wtmp.data());
         }
      }
   }
   else if (isUnicode)
   {
      wprintf(L"%s\n", pMsg.get()); // from us, starts with [ symbol

      // save functions for IDA script
      std::wstring tmpStr(pMsg.get());

      // ntdll
      if (tmpStr.find(L"NtClose") != std::string::npos)
         hookFunctions.push_back("NtClose");
      else if (tmpStr.find(L"NtOpenProcess") != std::string::npos)
         hookFunctions.push_back("NtOpenProcess");
      else if (tmpStr.find(L"NtCreateFile") != std::string::npos)
         hookFunctions.push_back("NtCreateFile");
      else if (tmpStr.find(L"NtSetDebugFilterState") != std::string::npos)
         hookFunctions.push_back("NtSetDebugFilterState");
      else if (tmpStr.find(L"NtQueryInformationProcess") != std::string::npos)
         hookFunctions.push_back("NtQueryInformationProcess");
      else if (tmpStr.find(L"NtQuerySystemInformation") != std::string::npos)
         hookFunctions.push_back("NtQuerySystemInformation");
      else if (tmpStr.find(L"NtSetInformationThread") != std::string::npos)
         hookFunctions.push_back("NtSetInformationThread");
      else if (tmpStr.find(L"NtCreateUserProcess") != std::string::npos)
         hookFunctions.push_back("NtCreateUserProcess");
      else if (tmpStr.find(L"NtCreateThreadEx") != std::string::npos)
         hookFunctions.push_back("NtCreateThreadEx");
      else if (tmpStr.find(L"NtSystemDebugControl") != std::string::npos)
         hookFunctions.push_back("NtSystemDebugControl");
      else if (tmpStr.find(L"NtYieldExecution") != std::string::npos)
         hookFunctions.push_back("NtYieldExecution");
      else if (tmpStr.find(L"NtSetLdtEntries") != std::string::npos)
         hookFunctions.push_back("NtSetLdtEntries");
      else if (tmpStr.find(L"NtQueryInformationThread") != std::string::npos)
         hookFunctions.push_back("NtQueryInformationThread");
      else if (tmpStr.find(L"NtCreateDebugObject") != std::string::npos)
         hookFunctions.push_back("NtCreateDebugObject");
      else if (tmpStr.find(L"NtQueryObject") != std::string::npos)
         hookFunctions.push_back("NtQueryObject");
      else if (tmpStr.find(L"RtlAdjustPrivilege") != std::string::npos)
         hookFunctions.push_back("RtlAdjustPrivilege");
      else if (tmpStr.find(L"NtShutdownSystem") != std::string::npos)
         hookFunctions.push_back("NtShutdownSystem");
      else if (tmpStr.find(L"ZwAllocateVirtualMemory") != std::string::npos)
         hookFunctions.push_back("ZwAllocateVirtualMemory");
      else if (tmpStr.find(L"ZwGetWriteWatch") != std::string::npos)
         hookFunctions.push_back("ZwGetWriteWatch");

         // kernelbase
      else if (tmpStr.find(L"IsDebuggerPresent") != std::string::npos)
         hookFunctions.push_back("IsDebuggerPresent");
      else if (tmpStr.find(L"CheckRemoteDebuggerPresent") != std::string::npos)
         hookFunctions.push_back("CheckRemoteDebuggerPresent");
      else if (tmpStr.find(L"SetUnhandledExceptionFilter") != std::string::npos)
         hookFunctions.push_back("SetUnhandledExceptionFilter");
      else if (tmpStr.find(L"RegOpenKeyExInternalW") != std::string::npos)
         hookFunctions.push_back("RegOpenKeyExInternalW");
      else if (tmpStr.find(L"RegQueryValueExW") != std::string::npos)
         hookFunctions.push_back("RegQueryValueExW");
   }
}

VOID genRandStr(TCHAR* str, const size_t size) // just enough randomness
{
   srand(static_cast<unsigned int>(time(nullptr)));
   static const TCHAR syms[] =
      L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      L"abcdefghijklmnopqrstuvwxyz";
   for (size_t i = 0; i < size - 1; ++i)
   {
      str[i] = syms[rand() / (RAND_MAX / (_tcslen(syms) - 1) + 1)];
   }
   str[size - 1] = 0;
}

void SetHardwareBreakpoint(HANDLE tHandle, CONTEXT& cxt, const DWORD64 addr, size_t size, DrReg dbgReg)
{
   cxt.ContextFlags = CONTEXT_DEBUG_REGISTERS;

   GetThreadContext(tHandle, &cxt);

   const int m_index = dbgReg; // Dr_
   switch (dbgReg)
   {
   case Dr0:
      cxt.Dr0 = addr;
      break;
   case Dr1:
      cxt.Dr1 = addr;
      break;
   case Dr2:
      cxt.Dr2 = addr;
      break;
   case Dr3:
      cxt.Dr3 = addr;
      break;
   }

   SetBits(cxt.Dr7, 16 + (m_index * 4), 2, 3); // read/write
   SetBits(cxt.Dr7, 18 + (m_index * 4), 2, size); // size
   SetBits(cxt.Dr7, m_index * 2, 1, 1);

   SetThreadContext(tHandle, &cxt);
}

int _tmain()
{
   // welcome 
   const TCHAR welcome[] = L"makin --- Copyright (c) 2018 Lasha Khasaia\n"
      L"https://www.secrary.com - @_qaz_qaz\n"
      L"----------------------------------------------------\n\n";
   wprintf(L"%s", welcome);

   STARTUPINFO si{};
   si.cb = sizeof(si);
   PROCESS_INFORMATION pi{};
   DWORD err;
   DEBUG_EVENT d_event{};
   auto done = FALSE;
   TCHAR dll_path[MAX_PATH + 2]{};
   TCHAR proc_path[MAX_PATH + 2]{};
   auto first_its_me = FALSE;
   TCHAR filePath[MAX_PATH + 2]{};
   CONTEXT cxt{};
   PVOID ex_addr = nullptr;
   HANDLE tHandle{};
   DWORD64 expAddr{};

   int nArgs{};
   const auto pArgv = CommandLineToArgvW(GetCommandLine(), &nArgs);

   if (nArgs < 2)
   {
      printf("Usage: \n./makin.exe \"/path/to/sample\"\n");
      return 1;
   }

   TCHAR cmdLine[0x1000]{};
	for (int i = 2; i < nArgs; ++i)
	{
		_tcscat_s(cmdLine, pArgv[i]);
		_tcscat_s(cmdLine, L" ");
	}

   _tcsncpy_s(proc_path, pArgv[1], MAX_PATH + 2);

   if (!PathFileExists(proc_path))
   {
      err = GetLastError();
      wprintf(L"[!] %s is not a valid file\n", proc_path);
	  getchar();
      return err;
   }


   const auto hFile = CreateFile(proc_path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);

   if (hFile == INVALID_HANDLE_VALUE)
   {
      err = GetLastError();
      printf("CreateFile error: %ul\n", err);
      return err;
   }

   LARGE_INTEGER size{};
   GetFileSizeEx(hFile, &size);

   SYSTEM_INFO sysInfo{};
   GetSystemInfo(&sysInfo);

   const auto hMapFile = CreateFileMapping(hFile,
                                           nullptr,
                                           PAGE_READONLY,
                                           size.HighPart,
                                           size.LowPart,
                                           nullptr);

   if (!hMapFile)
   {
      err = GetLastError();
      printf("CreateFileMapping is NULL: %ul", err);
      return err;
   }

   // Map just one page
   auto lpMapAddr = MapViewOfFile(hMapFile,
                                  FILE_MAP_READ,
                                  0,
                                  0,
                                  sysInfo.dwPageSize); // one page size is more than we need for now

   if (!lpMapAddr)
   {
      err = GetLastError();
      printf("MapViewOfFIle is NULL: %ul\n", err);
      return err;
   }
   // IMAGE_DOS_HEADER->e_lfanew
   const auto e_lfanew = *reinterpret_cast<DWORD*>(static_cast<byte*>(lpMapAddr) + sizeof(IMAGE_DOS_HEADER) - sizeof(
      DWORD));
   UnmapViewOfFile(lpMapAddr);


   const DWORD NtMapAddrLow = (e_lfanew / sysInfo.dwAllocationGranularity) * sysInfo.dwAllocationGranularity;
   lpMapAddr = MapViewOfFile(hMapFile,
                             FILE_MAP_READ,
                             0,
                             NtMapAddrLow,
                             sysInfo.dwPageSize);

   if (!lpMapAddr)
   {
      err = GetLastError();
      printf("MapViewOfFIle is NULL: %ul\n", err);
      return err;
   }

   auto ntHeaderAddr = lpMapAddr;
   if (NtMapAddrLow != e_lfanew)
   {
      ntHeaderAddr = static_cast<byte*>(ntHeaderAddr) + e_lfanew;
   }

   if (PIMAGE_NT_HEADERS(ntHeaderAddr)->OptionalHeader.DataDirectory[9].VirtualAddress)
   {
      printf(
         "[TLS] The executable contains TLS callback(s)\nI can not hook code executed by TLS callbacks\nPlease, abort execution and check it manually\n[c]ontinue / [A]bort: \n\n");
      const char ic = getchar();
      if (ic != 'c')
         ExitProcess(0);
   }

   const DWORD64 sizeOfImage = PIMAGE_NT_HEADERS(ntHeaderAddr)->OptionalHeader.SizeOfImage;

   UnmapViewOfFile(lpMapAddr);
   CloseHandle(hFile);

   wprintf(L"PROCESS NAME: %s\nCOMMAND LINE: %s\n\n", proc_path, cmdLine);

   // !!! copy capstone.dll to curr dir
   if (!CreateProcess(proc_path, cmdLine, nullptr, nullptr, FALSE,
                      DEBUG_ONLY_THIS_PROCESS | CREATE_SUSPENDED | DETACHED_PROCESS, nullptr, nullptr, &si, &pi))
   {
      err = GetLastError();
      printf_s("[!] CreateProces failed: %lu\n", err);
      return err;
   }

   // Detect memory accesses

   const pNtQueryInformationProcess NtQueryInformationProcess = pNtQueryInformationProcess(
      GetProcAddress(GetModuleHandle(L"ntdll"), "NtQueryInformationProcess"));

   PROCESS_BASIC_INFORMATION pbi{};
   NtQueryInformationProcess(pi.hProcess, ProcessBasicInformation, &pbi, sizeof(PROCESS_BASIC_INFORMATION), nullptr);

   DWORD64 peb = DWORD64(pbi.PebBaseAddress);

   const DWORD64 pBeingDebugged = DWORD64(reinterpret_cast<byte*>(peb) + 0x2); // PEB->BeingDebugged

#ifndef _WIN64
   peb -= 0x1000; // 32-bit PEB
#endif

   const DWORD64 pImageBaseAddr = DWORD64(reinterpret_cast<byte*>(peb) + 0x10); // 0x010 ImageBaseAddress : Ptr64 Void

   DWORD64 imageBaseAddr{};
   SIZE_T ret{};
   ReadProcessMemory(pi.hProcess, PVOID(pImageBaseAddr), &imageBaseAddr, sizeof(DWORD64), &ret);

   SetHardwareBreakpoint(pi.hThread, cxt, pBeingDebugged, 1, Dr0);

   // GlobalFlags
   DWORD64 pNtGlobalFlag{};
#ifdef _WIN64
   pNtGlobalFlag = DWORD64(reinterpret_cast<byte*>(peb) + 0xBC);
#else
   pNtGlobalFlag = DWORD64(reinterpret_cast<byte*>(peb) + 0x68);
   pNtGlobalFlag += 0x1000; // 32-bit PEB
#endif

   SetHardwareBreakpoint(pi.hThread, cxt, pNtGlobalFlag, 2, Dr1);

   // ref: https://github.com/LordNoteworthy/al-khaser/blob/master/al-khaser/Anti%20Debug/SharedUserData_KernelDebugger.cpp
   // KUSER_SHARED_DATA: https://www.geoffchappell.com/studies/windows/km/ntoskrnl/structs/kuser_shared_data.htm
   const ULONG_PTR UserSharedData = 0x7FFE0000;
   const DWORD64 KdDebuggerEnabledByte = UserSharedData + 0x2D4; // UserSharedData->KdDebuggerEnabled

   SetHardwareBreakpoint(pi.hThread, cxt, KdDebuggerEnabledByte, sizeof(BOOLEAN), Dr2);


   // Create Job object
   // UPDATE: dbg child proc.
   //JOBOBJECT_EXTENDED_LIMIT_INFORMATION jbli{0};
   //JOBOBJECT_BASIC_UI_RESTRICTIONS jbur;
   //const auto hJob = CreateJobObject(nullptr, L"makinAKAasho");
   //if (hJob)
   //{
   //   jbli.BasicLimitInformation.ActiveProcessLimit = 1; // Blocked new process creation
   //   jbli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_ACTIVE_PROCESS | JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

   //   jbur.UIRestrictionsClass = JOB_OBJECT_UILIMIT_DESKTOP | JOB_OBJECT_UILIMIT_EXITWINDOWS;
   //   /*| JOB_OBJECT_UILIMIT_HANDLES*/ // uncomment if you want

   //   SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jbli, sizeof(jbli));

   //   SetInformationJobObject(hJob, JobObjectBasicUIRestrictions, &jbur, sizeof(jbur));

   //   if (!AssignProcessToJobObject(hJob, pi.hProcess))
   //   {
   //      printf("[!] AssignProcessToJobObject failed: %ul\n", GetLastError());
   //   }
   //}

#ifdef _DEBUG
   SetCurrentDirectory(L"../Debug");
#ifdef _WIN64
   SetCurrentDirectory(L"../x64/Debug");
#endif
#endif

   GetFullPathName(L"./asho.dll", MAX_PATH + 2, dll_path, nullptr);
   if (!PathFileExists(dll_path))
   {
      err = GetLastError();
      wprintf(L"[!] %s is not a valid file\n", dll_path);

      return err;
   }

   // generate random name for asho.dll ;)
   TCHAR randAsho[0x5]{};
   genRandStr(randAsho, 0x5);
   TCHAR ashoTmpDir[MAX_PATH + 2]{};
   GetTempPath(MAX_PATH + 2, ashoTmpDir);
   _tcscat_s(ashoTmpDir, randAsho);
   _tcscat_s(ashoTmpDir, L".dll");
   const auto cStatus = CopyFile(dll_path, ashoTmpDir, FALSE);
   if (!cStatus)
   {
      err = GetLastError();
      wprintf(L"[!] CopyFile failed: %ul\n", err);

      return err;
   }
   if (!PathFileExists(ashoTmpDir))
   {
      err = GetLastError();
      wprintf(L"[!] %s is not a valid file\n", ashoTmpDir);

      return err;
   }

   const LPVOID p_alloc = VirtualAllocEx(pi.hProcess, nullptr, MAX_PATH + 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
   if (!p_alloc)
   {
      err = GetLastError();
      printf("[!] Allocation failed: %lu\n", err);
      return err;
   }
   if (!WriteProcessMemory(pi.hProcess, p_alloc, ashoTmpDir, MAX_PATH + 2, nullptr))
   {
      err = GetLastError();
      printf("WriteProcessMemory failed: %lu\n", err);
      return err;
   }
   const HMODULE h_module = GetModuleHandle(L"kernel32");
   if (!h_module)
   {
      err = GetLastError();
      printf("GetmModuleHandle failed: %lu\n", err);
      return err;
   }
   const FARPROC load_library_addr = GetProcAddress(h_module, "LoadLibraryW");

   if (!load_library_addr)
   {
      err = GetLastError();
      printf("GetProcAddress failed: %lu\n", err);
      return err;
   }

   const auto qStatus = QueueUserAPC(PAPCFUNC(load_library_addr), pi.hThread, ULONG_PTR(p_alloc));
   if (!qStatus)
   {
      err = GetLastError();
      printf("QueueUserAPC failed: %lu\n", err);
      return err;
   }

   ResumeThread(pi.hThread);

   while (!done)
   {
      auto cont_status = DBG_CONTINUE;
      if (WaitForDebugEventEx(&d_event, INFINITE))
      {
         switch (d_event.dwDebugEventCode)
         {
         case OUTPUT_DEBUG_STRING_EVENT:
            process_output_string(pi, d_event.u.DebugString);

            cont_status = DBG_CONTINUE;
            break;
         case LOAD_DLL_DEBUG_EVENT:
            // we get load dll as file handle 
            GetFinalPathNameByHandle(d_event.u.LoadDll.hFile, filePath, MAX_PATH + 2, 0);
            if (filePath)
            {
               const std::wstring tmpStr(filePath + 4);
               loadDll.push_back(tmpStr);
            }
            // to avoid LdrloadDll / NtCreateFile trick ;)
            CloseHandle(d_event.u.LoadDll.hFile);
            break;

         case EXCEPTION_DEBUG_EVENT:
            cont_status = DBG_EXCEPTION_NOT_HANDLED;
            if (!d_event.u.Exception.dwFirstChance)
               break;
            switch (d_event.u.Exception.ExceptionRecord.ExceptionCode)
            {
            case EXCEPTION_ACCESS_VIOLATION:
               printf("[EXCEPTION] EXCEPTION_ACCESS_VIOLATION\n\n");
               // cont_status = DBG_EXCEPTION_HANDLED;
               break;

            case EXCEPTION_BREAKPOINT:

               if (!first_its_me)
               {
                  first_its_me = TRUE;
                  break;
               }
               printf("[EXCEPTION] EXCEPTION_BREAKPOINT\n\n");
               // cont_status = DBG_EXCEPTION_HANDLED;

               break;

               /*case EXCEPTION_DATATYPE_MISALIGNMENT:
               printf("[EXCEPTION] EXCEPTION_DATATYPE_MISALIGNMENT\n");

               break;*/

            case EXCEPTION_SINGLE_STEP:

               expAddr = DWORD64(d_event.u.Exception.ExceptionRecord.ExceptionAddress);

               // HANDLE hardware accesses

               tHandle = OpenThread(GENERIC_ALL, FALSE, d_event.dwThreadId);
               cxt.ContextFlags = CONTEXT_DEBUG_REGISTERS;
               GetThreadContext(tHandle, &cxt);
               CloseHandle(tHandle);

               if (cxt.Dr6 & 0b1111) //  There are HBs
                  cont_status = DBG_EXCEPTION_HANDLED;
               else
                  printf("[EXCEPTION] EXCEPTION_SINGLE_STEP\n");

               if (expAddr > imageBaseAddr && expAddr < imageBaseAddr + sizeOfImage)
               {
                  if (cxt.Dr6 & 0x1)
                     printf(
                        "[PEB->BeingDebugged] The debuggee attempts to detect a debugger.\nBase address of the image: 0x%p\nException address: 0x%p\nRVA: 0x%p\n\n",
                        PVOID(imageBaseAddr), PVOID(expAddr), PVOID(expAddr - imageBaseAddr));
                  else if (cxt.Dr6 & 0b10)
                     printf("[PEB->NtGlobalFlag] The debuggee attempts to detect a debugger.\nBase address of the image: 0x%p\nException address: 0x%p\nRVA: 0x%p\n\n",
                        PVOID(imageBaseAddr), PVOID(expAddr), PVOID(expAddr - imageBaseAddr));
                  else if (cxt.Dr6 & 0b100)
                     printf("[UserSharedData->KdDebuggerEnabled] The debuggee attempts to detect a debugger.\nBase address of the image: 0x%p\nException address: 0x%p\nRVA: 0x%p\n\n",
                        PVOID(imageBaseAddr), PVOID(expAddr), PVOID(expAddr - imageBaseAddr));
                  else if (cxt.Dr6 & 0b1000)
                     printf("DR3\n"); // Not implemented yet

                  break;
               }

               break;

            case DBG_CONTROL_C:
               printf("[EXCEPTION] DBG_CONTROL_C\n\n");

               break;

            case EXCEPTION_GUARD_PAGE:
               printf("[EXCEPTION] EXCEPTION_GUARD_PAGE\n\n");
               // cont_status = DBG_EXCEPTION_HANDLED;
               break;

            default:
               // Handle other exceptions. 
               break;
            }
            break;
         case EXIT_PROCESS_DEBUG_EVENT:
            done = TRUE;
            printf("[EOF] ========================================================================\n");
			getchar();
            break;

         default:
            break;
         }

         ContinueDebugEvent(d_event.dwProcessId, d_event.dwThreadId, cont_status);
      }
   }

   CloseHandle(pi.hThread);
   CloseHandle(pi.hProcess);
   //if (hJob)
   //   CloseHandle(hJob);

   // IDA script TODO: FIX fn calls
   char header[] =
      "import idc\n"
      "import idaapi\n"
      "import idautils\n"
      "\n"
      "hookFunctions = [\n";
   char tail[] =
      "\n]\n"
      "\n"
      "def makinbp():\n"
      "	if not idaapi.is_debugger_on():\n"
      "		print \"Please run the process... and call makinbp() again\"\n"
      "		return\n"
      "	print \"\\n\\n---------- makin ----------- \\n\\n\"\n"
      "	for mods in idautils.Modules():\n"
      "		if \"ntdll.dll\" in mods.name.lower() or \"kernelbase.dll\" in mods.name.lower():\n"
      "			# idaapi.analyze_area(mods.base, mods.base + mods.size)\n"
      "			name_addr = idaapi.get_debug_names(mods.base, mods.base + mods.size)\n"
      "			for addr in name_addr:\n"
      "				func_name = Name(addr)\n"
      "				func_name = func_name.split(\"_\")[1]\n"
      "				for funcs in hookFunctions:\n"
      "					if funcs.lower() == func_name.lower():\n"
      "						print \"Adding bp on \", hex(addr), func_name\n"
      "						add_bpt(addr)\n"
      "						hookFunctions.remove(funcs)\n"
      "	print \"\\n\\n----------EOF makin EOF----------- \\n\""
      "\n\n"
      "def main():\n"
      "	if idaapi.is_debugger_on():\n"
      "		makinbp()\n"
      "	else:\n"
      "		print \"Please run the process... and call makinbp()\"\n"
      "\n"
      "main()\n";


   const auto hFileIda = CreateFile(L"makin_ida_bp.py", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
   if (hFileIda == INVALID_HANDLE_VALUE)
   {
      err = GetLastError();
      wprintf(L"CreateFile failed: %ul", err);
   }

   WriteFile(hFileIda, header, strlen(header), nullptr, nullptr);

   // http://en.cppreference.com/w/cpp/algorithm/unique
   std::sort(hookFunctions.begin(), hookFunctions.end());
   const auto last = std::unique(hookFunctions.begin(), hookFunctions.end());
   hookFunctions.erase(last, hookFunctions.end());

   for (auto func : hookFunctions)
   {
      WriteFile(hFileIda, "\"", strlen("\""), nullptr, nullptr);
      WriteFile(hFileIda, func.data(), strlen(func.data()), nullptr, nullptr);
      WriteFile(hFileIda, "\",\n", strlen("\",\n"), nullptr, nullptr);
   }

   WriteFile(hFileIda, tail, strlen(tail), nullptr, nullptr);

   CloseHandle(hFileIda);

   return 0;
}
