#pragma once
#include "StackWalker/StackWalker.h"

#include <functional>
#include <mutex>
#include <unordered_set>
#include <unordered_map>
#include <string_view>
#include <array>

namespace Simple
{
	constexpr std::size_t STACK_TRACE_MAX_LINES = 32;
	constexpr std::size_t STACK_TRACE_MAX_LINE_LENGTH = 1024;

	struct StackTraceImpl final
	{
		std::array<const char*, STACK_TRACE_MAX_LINES> myLines = {};

		bool operator==(const StackTraceImpl& aStackTraceImpl) const noexcept
		{
			return memcmp(myLines.data(), aStackTraceImpl.myLines.data(), sizeof(const char*) * STACK_TRACE_MAX_LINES) == 0;
		}
	};

	class StackTrace final
	{
	public:
		StackTrace();

		bool operator==(const StackTrace& aStackTrace) const noexcept
		{
			return myImpl == aStackTrace.myImpl;
		}

		void Print() const;
		std::size_t ComputeHash() const;
		static StackTrace CaptureStackTrace(int aSkipDepth);

		const std::vector<const char*> GetLines() const;
	private:
		StackTrace(const StackTraceImpl&);
		const StackTraceImpl* myImpl;
	};
}

namespace std
{
	template<> struct hash<Simple::StackTrace>
	{
		std::size_t operator()(const Simple::StackTrace& aStackTrace) const noexcept
		{
			return aStackTrace.ComputeHash();
		}
	};

	template<> struct hash<Simple::StackTraceImpl>
	{
		std::size_t operator()(const Simple::StackTraceImpl& aStackTrace) const noexcept
		{
			size_t result = 0;
			for (auto e : aStackTrace.myLines)
			{
				// Magic function to combine hashes. Should be switched if we add a dependency on a library with hash utility functions
				result ^= std::hash<const char*>{}(e)+0x9e3779b9 + (result << 6) + (result >> 2);
			}
			return result;
		}
	};
}

namespace Simple
{
	struct StackTraceLineHash
	{
		size_t operator() (const std::array<char, STACK_TRACE_MAX_LINE_LENGTH>& aLine) const
		{
			return std::hash<std::string_view>()(std::string_view(aLine.data(), std::strlen(aLine.data())));
		}
	};

	class ToStringStackWalker : public StackWalker
	{
	public:
		ToStringStackWalker() : StackWalker()
		{
			LoadModules();
		}
		void SetBuffer(std::array<const char*, STACK_TRACE_MAX_LINES>* aBuffer, const int aSkipCount = 0);
	protected:
		virtual void OnOutput(LPCSTR szText);
		virtual void OnDbgHelpErr(LPCSTR, DWORD, DWORD64) {}; // supress error output
	private:
		std::array<const char*, STACK_TRACE_MAX_LINES>* myBuffer = 0;
		int myCurrentIndex = 0;
	};

	class ToStrongStackWalkerWrapper final
	{
	public:
		inline static Simple::ToStringStackWalker myLocalStackWalker = {};
		inline static std::unordered_set<std::array<char, STACK_TRACE_MAX_LINE_LENGTH>, Simple::StackTraceLineHash> myLocalStackTraceLineCache;
		inline static std::unordered_set<Simple::StackTraceImpl> myLocalStackTraceCache;
		inline static std::mutex myLocalStackTraceMutex;
	};
}