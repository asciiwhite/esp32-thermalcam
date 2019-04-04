#ifndef H_PERFCOUNTER
#define H_PERFCOUNTER

#include <vector>

namespace perfCounter
{
  class ScopedArea
  {
    public:
      ScopedArea(const char* scopeName);
      ~ScopedArea();

      static void print();
      
      private:
        using TimeStamp = unsigned long;
        struct ScopeData {
          TimeStamp startTime;
          TimeStamp endTime;
          const char* scopeName;
        };
        static std::vector<ScopeData> scopes;
        static TimeStamp frameStartTime;
        static const int millisPerChar = 10;

        static void nextFrame();
        static void printScope(char cChar, TimeStamp start, TimeStamp end);
      };
}

#define LOG_PERF(x) static const char* name = x; perfCounter::ScopedArea counter(name);
#define LOG_PRINT   perfCounter::ScopedArea::print()

#endif
