// ---
//
// $Id: textoutput.cpp,v 1.3 2005/06/08 08:08:06 nilu Exp $
//
// CppTest - A C++ Unit Testing Framework
// Copyright (c) 2003 Niklas Lundell
//
// ---
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.
//
// ---

#include <iomanip>
#include <iostream>
#include <ios>

#include "cpptest-source.h"
#include "cpptest-textoutput.h"
#include "cpptest-time.h"

using namespace std;

namespace Test
{
	TextOutput::TextOutput(Mode mode, ostream& stream)
	:	_mode(mode),
		_stream(stream),
		_suite_errors(0),
		_suite_tests(0),
		_suite_total_tests(0),
		_total_errors(0)
	{}
	
	void
	TextOutput::finished(int tests, const Time& time)
	{
		_stream << endl;
		
		if (_total_errors == 0)
		{
			_stream << "All " << tests << " tests passed";
		}
		else
		{
			_stream << _total_errors << " of " << tests << " tests failed";
		}
		
		// 计算毫秒精度的时间
		double total_ms = (time.seconds() * 1000.0) + (time.microseconds() / 1000.0);
		_stream << " (" << std::fixed << std::setprecision(3) << total_ms << " ms)" << endl;
	}
	
	void
	TextOutput::suite_start(int tests, const string& name)
	{
		_suite_errors = 0;
		_suite_tests = 0;
		_suite_total_tests = tests;
		_suite_name = name;
		_suite_error_list.clear();
	}
	
	void
	TextOutput::suite_end(int tests, const string& name, const Time& time)
	{
		if (_mode == Verbose)
		{
			_stream << endl;
			
			if (_suite_errors == 0)
			{
				_stream << "All " << tests << " tests in " << name << " passed";
			}
			else
			{
				_stream << _suite_errors << " of " << tests << " tests in " << name << " failed";
			}
			
			// 计算毫秒精度的时间
			double total_ms = (time.seconds() * 1000.0) + (time.microseconds() / 1000.0);
			_stream << " (" << std::fixed << std::setprecision(3) << total_ms << " ms)" << endl;
		}
	}
	
	void
	TextOutput::test_start(const string& name)
	{
		// 在Verbose模式下输出测试开始信息
		if (_mode == Verbose)
		{
			_stream << "  Starting test: " << name << endl;
		}
	}
	
	void
	TextOutput::test_end(const string& name, bool ok, const Time& time)
	{
		_suite_tests++;
		
		// 计算毫秒精度的时间
		double total_ms = (time.seconds() * 1000.0) + (time.microseconds() / 1000.0);
		
		if (ok)
		{
			if (_mode == Verbose)
			{
				_stream << "  " << name << " passed (" << std::fixed << std::setprecision(3) << total_ms << " ms)" << endl;
			}
		}
		else
		{
			_total_errors++;
			_suite_errors++;
			
			if (_mode == Verbose)
			{
				_stream << "  " << name << " failed (" << std::fixed << std::setprecision(3) << total_ms << " ms)" << endl;
			}
		}
	}
	
	void
	TextOutput::assertment(const Source& s)
	{
		_suite_error_list.push_back(s);
		
		if (_mode == Verbose)
		{
			_stream << "    " << s.file() << ":" << s.line() << ": " << s.suite() << "::" << s.test() << " failed" << endl;
		}
	}
	
} // namespace Test 