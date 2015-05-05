/*
	The MIT License (MIT)

	Copyright (c) 2015 Cengiz Terzibas

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
*/

#include "CodeLiteSourceCodeAccessPrivatePCH.h"
#include "CodeLiteSourceCodeAccessor.h"
#include "CodeLiteSourceCodeAccessModule.h"
#include "ISourceCodeAccessModule.h"
#include "ModuleManager.h"
#include "DesktopPlatformModule.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>

#if WITH_EDITOR
#include "Developer/HotReload/Public/IHotReload.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogCodeLiteAccessor, Log, All);

#define LOCTEXT_NAMESPACE "CodeLiteSourceCodeAccessor"

void FCodeLiteSourceCodeAccessor::Startup()
{
	// Cache this so we don't have to do it on a background thread
	GetSolutionPath();

	// FIXME: look for CodeLite and cache the path
}

void FCodeLiteSourceCodeAccessor::Shutdown()
{
}

bool FCodeLiteSourceCodeAccessor::CanAccessSourceCode() const
{
	FString Path;
	return CanRunCodeLite(Path);
}

FName FCodeLiteSourceCodeAccessor::GetFName() const
{
	return FName("CodeLiteSourceCodeAccessor");
}

FText FCodeLiteSourceCodeAccessor::GetNameText() const
{
	return LOCTEXT("CodeLiteDisplayName", "CodeLite 7.0");
}

FText FCodeLiteSourceCodeAccessor::GetDescriptionText() const
{
	return LOCTEXT("CodeLiteDisplayDesc", "Open source code files in CodeLite 7.0");
}

bool FCodeLiteSourceCodeAccessor::OpenSolution()
{
	if(IsIDERunning())
	{
		UE_LOG(LogCodeLiteAccessor, Warning, TEXT("FCodeLiteSourceCodeAccessor::OpenSolution: OpenSolution: CodeLite is running, use qdbus to open the project within session?"));
		return false;
	}

	FString Solution = GetSolutionPath();
	FString IDEPath;
	if(!CanRunCodeLite(IDEPath))
	{
		UE_LOG(LogCodeLiteAccessor, Warning, TEXT("FCodeLiteSourceCodeAccessor::OpenSolution: Cannot find CodeLite binary"));
		return false;
	}
	printf("******************** FCodeLiteSourceCodeAccessor::OpenSolution: %s\n", TCHAR_TO_UTF8(*Solution));
	UE_LOG(LogCodeLiteAccessor, Warning, TEXT("FCodeLiteSourceCodeAccessor::OpenSolution: %s"), *Solution);
	
	FProcHandle Proc = FPlatformProcess::CreateProc(*IDEPath, *Solution, true, false, false, nullptr, 0, nullptr, nullptr);
	if(Proc.IsValid())
	{
		FPlatformProcess::CloseProc(Proc);
		return true;
	}
	return false;
}

bool FCodeLiteSourceCodeAccessor::OpenSourceFiles(const TArray<FString>& AbsoluteSourcePaths)
{
	FString IDEPath;
	if(!CanRunCodeLite(IDEPath))
	{
		UE_LOG(LogCodeLiteAccessor,Warning, TEXT("FCodeLiteSourceCodeAccessor::OpenSourceFiles: Cannot find CodeLite binary"));
		return false;
	}

	for(const auto& SourcePath : AbsoluteSourcePaths)
	{
		const FString NewSourcePath = FString::Printf(TEXT("\"%s\""), *SourcePath);
		printf("******************** FCodeLiteSourceCodeAccessor::OpenSourceFiles: %s\n", TCHAR_TO_UTF8(*NewSourcePath));
		if(!(FLinuxPlatformProcess::CreateProc(*IDEPath, *NewSourcePath, true, true, false, nullptr, 0, nullptr, nullptr).IsValid()))
		{
			return false;
		}
	}
	return true;
}

bool FCodeLiteSourceCodeAccessor::OpenFileAtLine(const FString& FullPath, int32 LineNumber, int32 ColumnNumber)
{
	FString IDEPath;
	if(!CanRunCodeLite(IDEPath))
	{
		UE_LOG(LogCodeLiteAccessor,Warning, TEXT("FCodeLiteSourceCodeAccessor::OpenSourceFiles: Cannot find CodeLite binary"));
		return false;
	}
	
	const FString Path = FString::Printf(TEXT("\"%s --line=%d\""), *FullPath, LineNumber);
	printf("******************** FCodeLiteSourceCodeAccessor::OpenFileAtLine: %s %d\n", TCHAR_TO_UTF8(*Path), LineNumber);
	
	UE_LOG(LogCodeLiteAccessor, Warning, TEXT("FCodeLiteSourceCodeAccessor::OpenSourceFiles: Open: %s"), *Path);
	if(FLinuxPlatformProcess::CreateProc(*IDEPath, *Path, true, true, false, nullptr, 0, nullptr, nullptr).IsValid())
	{
		return true;
	}

	return false;
}

bool FCodeLiteSourceCodeAccessor::AddSourceFiles(const TArray<FString>& AbsoluteSourcePaths, const TArray<FString>& AvailableModules)
{
	for(const auto& SourcePath : AbsoluteSourcePaths)
	{
		const FString NewSourcePath = FString::Printf(TEXT("\"%s\""), *SourcePath);
		printf("******************** FCodeLiteSourceCodeAccessor::AddSourceFiles: %s\n", TCHAR_TO_UTF8(*NewSourcePath));
	}
	return false;
}

bool FCodeLiteSourceCodeAccessor::SaveAllOpenDocuments() const
{
	printf("******************** FCodeLiteSourceCodeAccessor::SaveAllOpenDocuments\n");
	return false;
}

void FCodeLiteSourceCodeAccessor::Tick(const float DeltaTime)
{
}


bool FCodeLiteSourceCodeAccessor::CanRunCodeLite(FString& OutPath) const
{
	OutPath = TEXT("/usr/bin/codelite");
	return FPaths::FileExists(OutPath);
}


bool FCodeLiteSourceCodeAccessor::IsIDERunning()
{
	pid_t pid = FindProcess("codelite");
	if(pid == -1)
	{
		return false;
	}
	return true;
}

FString FCodeLiteSourceCodeAccessor::GetSolutionPath() const
{
	if(IsInGameThread())
	{
		FString SolutionPath;
		if(FDesktopPlatformModule::Get()->GetSolutionPath(SolutionPath))
		{
			CachedSolutionPath = FPaths::ConvertRelativePathToFull(SolutionPath);
		}
	}
	return CachedSolutionPath;
}

pid_t FCodeLiteSourceCodeAccessor::FindProcess(const char* name)
{
	DIR* dir;
	struct dirent* ent;
	char* endptr;
	char buf[512];

	if(!(dir = opendir("/proc")))
	{
		perror("can't open /proc");
		return -1;
	}

	while((ent = readdir(dir)) != NULL)
	{
		long lpid = strtol(ent->d_name, &endptr, 10);
		if(*endptr != '\0')
		{
			continue;
		}

		// Try to open the cmdline file
		snprintf(buf, sizeof(buf), "/proc/%ld/cmdline", lpid);
		FILE* fp = fopen(buf, "r");
		if(fp)
		{
			if(fgets(buf, sizeof(buf), fp) != NULL)
			{
				// check the first token in the file, the program name.
				char* first = strtok(buf, " ");
				if(!strcmp(first, name))
				{
					fclose(fp);
					closedir(dir);
					return (pid_t)lpid;
				}
			}
			fclose(fp);
		}
	}

	closedir(dir);
	return -1;
}

#undef LOCTEXT_NAMESPACE