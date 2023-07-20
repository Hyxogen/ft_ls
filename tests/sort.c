#include "../main.c"
#include <assert.h>

#ifdef LS_TEST

int compare_ints(const void *a, const void *b)
{
	int ia = *(const int *)a;
	int ib = *(const int *)b;

	if (ia < ib)
		return -1;
	if (ia > ib)
		return 1;
	return 0;
}

int main()
{
        int ints[] = { -2, 5, -34, 8, 1 };
        size_t size = sizeof ints / sizeof *ints;

        sort(ints, size, sizeof(int), compare_ints);

        for (size_t i = 0; i < size; ++i) {
                if (i == 0)
                        continue;
                assert(ints[i - 1] < ints[i] && "not sorted");
        }
}

#endif
