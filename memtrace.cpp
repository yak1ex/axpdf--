#include <iostream>
#include <cstdlib>

void* operator new(std::size_t sz)
{
	if(!sz) sz = 1;
	void *p = malloc(sz);
	if(!p) throw std::bad_alloc();
	std::cerr << "alloced " << sz << " bytes at " << p << std::endl;
	return p;
}

void operator delete(void *p)
{
	std::cerr << "freed at " << p << std::endl;
	free(p);
}
