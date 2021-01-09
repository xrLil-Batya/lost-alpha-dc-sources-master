#include "xrCore.h"
#include "_random.h"
#include <stdio.h>

int __cdecl main(int argc, char* argv[])
{
	Core._initialize("xrRandomTest", 0, FALSE);
	printf("xrRandomTest\n");

	CRandom R;
	u32 min = 0;
	u32 max = 2;
	printf("Min value: %d, max value: %d\n", min, max);

	for (int i = 0; i < 100; ++i)
	{
		u32 testRandom = R.randI(min, max);
		printf("%d, ", testRandom);
	}
	printf("\n\n");


	Core._destroy();
	return 0;
}
