#ifndef MEMORY_H
#define MEMORY_H 

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
    extern char *__brkval;
#endif  // __arm__

/*
  What getFreeMemory() is actually reporting is the space between the heap and the stack. 
  it does not report any de-allocated memory that is buried in the heap. 
  Buried heap space is not usable by the stack, and may be fragmented enough 
  that it is not usable for many heap allocations either. 
  
  The space between the heap and the stack is what you 
  really need to monitor if you are trying to avoid stack crashes.
*/
inline int getFreeMemory()
{
	char top;
#ifdef __arm__
	return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
      return &top - __brkval;
#else  // __arm__
      return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

// In Arduino, malloc can't be trusted. 
// It does not always return a NULL pointer when there is, in fact, not enough memory available!
inline void* mallocWrapper(size_t bytes)
{
	if (bytes > getFreeMemory())
		return nullptr;
	return malloc(bytes);
}


#endif
