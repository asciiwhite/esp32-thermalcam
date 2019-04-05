#include "perfCounter.h"

#include <Arduino.h>

namespace perfCounter
{
  std::vector<ScopedArea::ScopeData> ScopedArea::scopes[];
  ScopedArea::TimeStamp ScopedArea::frameStartTime = 0;

  ScopedArea::ScopedArea(const char* scopeName)
  {
    const int coreId = xPortGetCoreID();
    scopes[coreId].push_back({millis(), 0, scopeName});
  }
      
  ScopedArea::~ScopedArea()
  {
    const int coreId = xPortGetCoreID();
    assert(!scopes[coreId].empty());
    scopes[coreId].back().endTime = millis();
  }

  void ScopedArea::printScope(char cChar, TimeStamp start, TimeStamp end)
  {
    const auto scopeLength = end - start;
    const auto charLength = scopeLength / millisPerChar;
    for(int i=0; i < charLength; i++) Serial.print(cChar);
  }

  void ScopedArea::print()
  { 
    const auto currentTime = millis();

    for (int coreId=0; coreId < 2; coreId++)
    {
      Serial.printf("Core %d: ", coreId);
      TimeStamp startTime = frameStartTime;
      for (const auto& scope : scopes[coreId])
      {      
        printScope('#', startTime, scope.startTime);      
        printScope(scope.scopeName[0], scope.startTime, scope.endTime);
        startTime = scope.endTime;
      }      
      printScope('#', startTime, currentTime);
      Serial.printf(" %d ms\n", currentTime - frameStartTime);
      scopes[coreId].clear();

      
    }
    frameStartTime = currentTime;
  }
}
