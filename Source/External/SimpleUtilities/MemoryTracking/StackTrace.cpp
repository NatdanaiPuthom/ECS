#include "StackTrace.hpp"

namespace Simple
{
	StackTrace::StackTrace(const StackTraceImpl& aStackTraceImpl)
		: myImpl(&aStackTraceImpl)
	{
	}

	StackTrace::StackTrace()
		: myImpl(nullptr)
	{
	}

	StackTrace StackTrace::CaptureStackTrace(int aSkipDepth)
	{
		std::lock_guard<std::mutex> guard(ToStrongStackWalkerWrapper::myLocalStackTraceMutex);

		StackTraceImpl impl;
		ToStrongStackWalkerWrapper::myLocalStackWalker.SetBuffer(&impl.myLines, 2 + aSkipDepth);
		ToStrongStackWalkerWrapper::myLocalStackWalker.ShowCallstack();
		ToStrongStackWalkerWrapper::myLocalStackWalker.SetBuffer(nullptr);

		const auto& it = ToStrongStackWalkerWrapper::myLocalStackTraceCache.insert(impl).first;
		return StackTrace(*it);
	}

	const std::vector<const char*> StackTrace::GetLines() const
	{
		std::vector<const char*> lines;

		if (myImpl == nullptr)
		{
			return lines;
		}

		for (const auto& line : myImpl->myLines)
		{
			if (line == nullptr)
			{
				break;
			}

			lines.push_back(line);
		}

		return lines;
	}

	void StackTrace::Print() const
	{
		if (myImpl == nullptr)
		{
			OutputDebugStringA("Empty Stack Trace\n");
			return;
		}

		for (const auto& line : myImpl->myLines)
		{
			if (line == nullptr)
				break;

			OutputDebugStringA(line);
		}
	}

	std::size_t StackTrace::ComputeHash() const
	{
		if (myImpl == nullptr)
			return 0;

		return std::hash<StackTraceImpl>()(*myImpl);
	}

	void ToStringStackWalker::SetBuffer(std::array<const char*, STACK_TRACE_MAX_LINES>* aBuffer, const int aSkipCount)
	{
		myBuffer = aBuffer;
		myCurrentIndex = -aSkipCount;
		m_MaxRecursionCount = STACK_TRACE_MAX_LINES + aSkipCount;
	}

	void ToStringStackWalker::OnOutput(LPCSTR szText)
	{
		if (!myBuffer)
			return;

		if (myCurrentIndex < 0)
		{
			myCurrentIndex++;
			return;
		}

		if (myCurrentIndex >= STACK_TRACE_MAX_LINES)
			return;

		std::array<char, STACK_TRACE_MAX_LINE_LENGTH> myLine{};
		strncpy_s(myLine.data(), STACK_TRACE_MAX_LINE_LENGTH, szText, STACK_TRACE_MAX_LINE_LENGTH);
		auto pair = ToStrongStackWalkerWrapper::myLocalStackTraceLineCache.insert(myLine);
		const char* cachedLine = pair.first->data();

		(*myBuffer)[myCurrentIndex] = cachedLine;
		myCurrentIndex++;
	}
}