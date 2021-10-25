// demo.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <windows.h>
#include <iostream>
#include <regex>
#include <string>
#include <codecvt>
#include <fstream>
#include <locale>
#include <vector>
#include <memory>
#include <set>
#include <algorithm>
#include <pathcch.h>
#include <filesystem>
#include <sstream>
#include "demo.h"
using namespace std;
using namespace std::filesystem;

struct GitLog
{
	wstring commit_id;
	wstring commit_msg;
	vector<wstring> included_files;
};

std::string WcsToUtf8(const wchar_t* src, size_t length)
{
	auto u8length = WideCharToMultiByte(CP_UTF8, 0, src, length, NULL, 0, 0,0);
	if (u8length > 0)
	{
		string buffer;
		buffer.resize(u8length);
		if (0 == WideCharToMultiByte(CP_UTF8, 0, src, length, &buffer[0], u8length,0,0))
		{
			return {};
		}
		return buffer;
	}
	return {};
}

std::wstring Utf8ToWcs(const char* src, size_t src_length)
{
	auto widechar_length = MultiByteToWideChar(CP_UTF8, 0, src, src_length, NULL, 0);
	if (widechar_length > 0)
	{
		wstring buffer;
		buffer.resize(widechar_length);
		if (0 == MultiByteToWideChar(CP_UTF8, 0, src, src_length, &buffer[0], widechar_length))
		{
			return {};
		}
		return buffer;
	}
	return {};
}
std::wstring Utf8ToWcs(const std::string& src)
{
	return Utf8ToWcs(src.c_str(), src.length());
}

std::wstring ReadFile(const std::wstring& file_path)
{
	fstream fs;
	fs.open(file_path, ios::in);
	stringstream stream;
	stream << fs.rdbuf();
	return Utf8ToWcs(stream.str());
}

bool CheckPathParams(const std::wstring& root_path, const std::wstring& output_path)
{
	if (!exists(root_path))
	{
		return false;
	}

	if (!exists(output_path))
	{
		return create_directories(output_path);
	}

	return true;
}
vector<shared_ptr<GitLog>> ParseLogDetail(const std::wstring& content)
{
	wstringstream wstream(content);
	wstring line;
	wregex fmt{ LR"((\w+)\s-?\s?(.+))" };
	vector<shared_ptr<GitLog>> logs;
	shared_ptr<GitLog> git_log;

	while (getline(wstream, line))
	{
		if (line.empty())
		{
			continue;
		}
		wsmatch match;
		if (regex_match(line, match, fmt) && match.size() == 3)
		{
			git_log = make_shared<GitLog>();
			git_log->commit_id = match[1];
			git_log->commit_msg = match[2];
			logs.emplace_back(git_log);
		}
		else if (git_log)
		{
			git_log->included_files.emplace_back(line);
		}
	}

	wcout << L"Total git commits: " << logs.size() << endl;

	return logs;
}

void CopyFiles(const std::wstring& root_path, const std::wstring& output_path, const vector<wstring>& files)
{
	size_t sum = 0;
	size_t failed_count = 0;
	stringstream mem_stream;
	
	for (auto& file : files)
	{
		path old_file(root_path + L"\\" + file);
		path new_file(output_path + L"\\" + file);
		old_file.make_preferred();
		new_file.make_preferred();
		if (!exists(new_file.parent_path()))
		{
			create_directories(new_file.parent_path());
		}

		try
		{
			sum++;
			if (!exists(old_file))
			{
				wcerr << L"File not exists: " << old_file.wstring() << endl;
				failed_count++;
				mem_stream << WcsToUtf8(file.c_str(), file.length()) << endl;
				continue;
			}
			copy_file(old_file, new_file, copy_options::overwrite_existing);
		}
		catch (const filesystem_error& err)
		{
			wcerr << "filesystem_error: " << err.what() << endl;
			failed_count++;
		}
	}

	ofstream outfile(output_path+L"\\deleted.txt");
	outfile << mem_stream.rdbuf();
	outfile.close();

	std::wcout << L"total files " << sum << L" and failed(to be deleted) " << failed_count << endl;
}

void DeleteFiles(const std::wstring& target_dir)
{	
	size_t no_del = 0;
	std::wstring deleted_file = target_dir;
	deleted_file.append(L"\\deleted.txt");
	if (exists(deleted_file))
	{
		wstringstream wstream;
		wstream << ReadFile(deleted_file);
		wstring line;
		wstring dir = target_dir+L"\\";
		while (getline(wstream, line))
		{
			if (exists(dir + line))
			{
				remove(dir + line);
			}
			else
			{
				no_del++;
				wcout << L"File not exists: " << dir << line << endl;
			}
		}
	}

	wcout << L"not found file count " << no_del << endl;
}

void Help()
{
	wcout << L"demo gitlog.txt <cherry-pick-target-dir> <source_dir> <target_dir>" << endl;
}

std::wstring format(const wchar_t* fmt, ...)
{
	std::wstring strResult = L"";
	if (NULL != fmt)
	{
		va_list marker = NULL;
		va_start(marker, fmt);                            //初始化变量参数
		size_t nLength = _vscwprintf(fmt, marker) + 1;    //获取格式化字符串长度
		std::vector<wchar_t> vBuffer(nLength, L'\0');    //创建用于存储格式化字符串的字符数组
		int nWritten = _vsnwprintf_s(&vBuffer[0], vBuffer.size(), nLength, fmt, marker);
		if (nWritten > 0)
		{
			strResult = &vBuffer[0];
		}
		va_end(marker);                                    //重置变量参数
	}
	return strResult;
}

void Log(const wstring& log)
{
	auto wstring_to_gbk = [](const std::wstring& str)->std::string {
		int len = WideCharToMultiByte(CP_ACP, 0, str.c_str(), -1, NULL, 0, NULL, NULL);
		std::string ret;
		ret.resize(len);
		len = WideCharToMultiByte(CP_ACP, 0, str.c_str(), -1, const_cast<char*>(ret.data()), len, NULL, NULL);
		return ret;
	};

	cout << wstring_to_gbk(log);
}

int wmain(int argv, wchar_t** args)
{
	if (argv < 2)
	{
		Help();
		return 1;
	}
	
	setlocale(LC_ALL, "");

	wstring content = ReadFile(args[1]);

	if (content.empty())
	{
		return 2;
	}

	auto gitlogs = ParseLogDetail(content);

	if (argv==2)
	{
		std::set<std::wstring> affected_files;
		for (auto&& logIt = gitlogs.rbegin(); logIt != gitlogs.rend(); logIt++)
		{
			affected_files.insert((*logIt)->included_files.begin(), (*logIt)->included_files.end());
		}

		wstringstream stream;
		for (auto& f : affected_files)
		{	
			stream << f << endl;
		}

		Log(stream.str());		
	}

	std::string validCmd = "ynq";

	if (argv == 3)
	{
		int curIndex = 0;
		const int total = gitlogs.size();
		for(auto&& logIt=gitlogs.rbegin(); logIt!=gitlogs.rend(); logIt++)
		{
			curIndex++;
			wstringstream stream;
			stream << (*logIt)->commit_id << L" - " << (*logIt)->commit_msg << endl;			
			for (auto& f : (*logIt)->included_files)
			{
				stream << f << endl;
			}	
			Log(L"\r\n"+stream.str());

			char ch;
			do
			{
				Log(format(L"\r\nNeed merge [%d/%d]?(y/n/q)", curIndex, total));
				cin >> ch;
			} while (validCmd.find(ch)==std::string::npos);

			if (ch=='n')
			{
				continue;
			}
			else if (ch == 'q')
			{
				wcout << L"exit.";
				break;
			}

			STARTUPINFOW si;
			ZeroMemory(&si, sizeof(STARTUPINFOW));

			PROCESS_INFORMATION pi;
			std::wstring cmdLine = L"git cherry-pick " + (*logIt)->commit_id;
			Log(cmdLine+L"\r\n");

			BOOL created=CreateProcess(NULL, cmdLine.data(), NULL, NULL, TRUE, 0, 0, args[2], &si, &pi);
			if (created)
			{
				WaitForSingleObject(pi.hProcess, INFINITE);
			}
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);			
		}
		return 0;
	}

	if (argv == 4)
	{
		if (!CheckPathParams(args[2], args[3]))
		{
			return 2;
		}

		set<wstring> files;
		for (auto& log : gitlogs)
		{
			files.insert(log->included_files.begin(), log->included_files.end());			
		}

		CopyFiles(args[2], args[3], {files.begin(), files.end()});

		wcout << L"Want delete no exsit files?(y/n)" << endl;
		char ch;
		cin >> ch;

		if (ch == 'y')
		{
			wcout << L"Delete..." << endl;
			DeleteFiles(args[3]);
		}
	}

	return 0;
}
