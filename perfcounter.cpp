#include "perfCounter.h"

#include <Arduino.h>

namespace perfCounter
{
  std::vector<ScopedArea::ScopeData> ScopedArea::scopes;
  ScopedArea::TimeStamp ScopedArea::frameStartTime = 0;

  ScopedArea::ScopedArea(const char* scopeName)
  {
    scopes.push_back({millis(), 0, scopeName});
  }
      
  ScopedArea::~ScopedArea()
  {
    assert(!scopes.empty());
    scopes.back().endTime = millis();
  }

  void ScopedArea::nextFrame()
  {
    scopes.clear();
    frameStartTime = millis();
  }

  void ScopedArea::printScope(char cChar, TimeStamp start, TimeStamp end)
  {
    const auto scopeLength = end - start;
    const auto charLength = scopeLength / millisPerChar;
    for(int i=0; i < charLength; i++) Serial.print(cChar);
  }

  void ScopedArea::print()
  { 
    TimeStamp startTime = frameStartTime;
    
    for (const auto& scope : scopes)
    {      
      printScope('#', startTime, scope.startTime);      
      printScope(scope.scopeName[0], scope.startTime, scope.endTime);
      startTime = scope.endTime;
    }

    const auto currentTime = millis();
    printScope('#', startTime, currentTime);
    Serial.printf(" %d ms\n", currentTime - frameStartTime);

    nextFrame();
  }
}
