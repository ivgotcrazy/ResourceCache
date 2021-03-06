/*

Copyright (c) 2007, Arvid Norberg
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the distribution.
    * Neither the name of the author nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#include <execinfo.h>
#include <cxxabi.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string>
#include <cstring>

namespace BroadInter
{

namespace PUB
{

std::string Demangle(const char* name)
{
// in case this string comes
	// this is needed on linux
	char const* start = strchr(name, '(');
	if (start != 0)
	{
		++start;
	}
	else
	{
		// this is needed on macos x
		start = strstr(name, "0x");
		if (start != 0)
		{
			start = strchr(start, ' ');
			if (start != 0) ++start;
			else start = name;
		}
		else start = name;
	}

	char const* end = strchr(start, '+');
	if (end) while (*(end-1) == ' ') --end;

	std::string in;
	if (end == 0) in.assign(start);
	else in.assign(start, end);

	size_t len;
	int status;
	char* unmangled = ::abi::__cxa_demangle(in.c_str(), 0, &len, &status);
	if (unmangled == 0) return in;
	std::string ret(unmangled);
	free(unmangled);
	return ret;
}

void PrintBacktrace(const char* label)
{
	void* stack[50];

	int size = backtrace(stack, 50);
	char** symbols = backtrace_symbols(stack, size);

	fprintf(stderr, "%s\n", label);
	fprintf(stderr, "%s\n", "-------------------------------------------------------------------------------");
	for (int i = 1; i < size; ++i)
	{
		fprintf(stderr, "%d: %s\n", i, Demangle(symbols[i]).c_str());
	}

	free(symbols);
}

void AssertFail(const char* expr, int line, const char* file, const char* function)
{
	// 打印记录信息
	fprintf(stderr,
			"INFO\n"
			"-------------------------------------------------------------------------------\n"
		    "function   : %s\n"
			"file       : %s\n"
			"line       : %d\n"
			"expression : %s\n",
			function, file, line, expr);


	// 打印调用栈
	PrintBacktrace("\nCALL-STACK");

	// 强制进程退出
 	raise(SIGINT);
 	abort();
}

} // namespace PUB

} // namespace Broadinter
