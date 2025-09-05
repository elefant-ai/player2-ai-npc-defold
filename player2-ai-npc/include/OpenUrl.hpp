#include <cstdlib>
#include <string>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

int invoke_browser( const std::string& URL )
{
  #ifdef _WIN32
  return std::system( ("start \"\" " + URL).c_str() );

  #elif defined(__APPLE__) && TARGET_OS_MAC
  return std::system( ("open " + URL + " &").c_str() );

  #else
  return std::system( ("xdg-open " + URL + " &").c_str() );
  #endif
}