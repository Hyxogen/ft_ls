#include <sys/stat.h>
#include <dirent.h>
#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

enum filetype {
	blockdev,
	chardev,
	dir,
	pipe,
	link,
	file,
	sock,
	unknown,
};

struct entry {
	char filename[256];
	enum filetype type;
	struct stat stat;
};

enum result {
	LS_OK,
	LS_EOF,
	LS_ERROR,
};

static char *program_name;

static int ls_strcmp(const char *s1, const char *s2)
{
        unsigned char ch1, ch2;

        while (1) {
                ch1 = *s1++;
                ch2 = *s2++;
                if (ch1 != ch2)
                        return ch1 < ch2 ? 1 : -1;
                if (!ch1)
                        break;
        }
        return 0;
}

static void *ls_memcpy(void *restrict dst, const void *restrict src,
		       size_t count)
{
	for (size_t idx = 0; idx < count; ++idx) {
		((unsigned char *)dst)[idx] = ((const unsigned char *)src)[idx];
	}
	return dst;
}

static void *ls_realloc(void *ptr, size_t old_size, size_t new_size)
{
	void *new_ptr = malloc(new_size);
	if (!new_ptr)
		return NULL;

	ls_memcpy(new_ptr, ptr, old_size < new_size ? old_size : new_size);
	free(ptr);
	return new_ptr;
}

static void *ls_reallocarray(void *ptr, size_t nmemb, size_t old_size,
			     size_t new_size)
{
	if ((nmemb * new_size) / new_size != nmemb) {
		errno = ENOMEM;
		return NULL;
	}
	return ls_realloc(ptr, nmemb * old_size, nmemb * new_size);
}


static void swap(void *restrict a, void *restrict b, size_t size)
{
        char *cha = a, *chb = b;
        while (size--) {
                char tmp = *cha;
                *cha++ = *chb;
                *chb++ = tmp;
        }
}

static size_t partition(void *arr, size_t size, size_t begin, size_t end,
			int (*cmp)(const void *, const void *))
{
        char *carr = arr;
        char *pivot = carr + (((end - begin) / 2) + begin) * size;
        size_t i = begin;
        size_t j = end;

        while (1) {
                while (cmp(carr + i * size, pivot) < 0)
                        ++i;
                while (cmp(carr + j * size, pivot) > 0)
                        --j;

                if (i >= j)
                        break;

                char *a = carr + i * size, *b = carr + j * size;
                if (pivot == a)
                        pivot = b;
                else if (pivot == b)
                        pivot = a;

                swap(a, b, size);

                ++i;
                --j;
        }
        return j;
}

static void quicksort(void *arr, size_t size, size_t begin, size_t end,
		      int (*cmp)(const void *, const void *))
{
        if (begin < end) {
                size_t p = partition(arr, size, begin, end, cmp);
                quicksort(arr, size, begin, p, cmp);
                quicksort(arr, size, p + 1, end, cmp);
        }
}

static void sort(void *arr, size_t nmemb, size_t size,
		 int (*cmp)(const void *, const void *))
{
        if (size > 0)
                quicksort(arr, size, 0, nmemb - 1, cmp);
}

static void print_error()
{
	perror(program_name);
}

static struct entry convert_dirent(const struct dirent *ent)
{
        struct entry entry;

        ls_memcpy(entry.filename, ent->d_name, sizeof(entry.filename));
        entry.type = unknown;
        return entry;
}

static enum result read_entry(struct entry *dst, DIR *dir)
{
	errno = 0;

	const struct dirent *ent = readdir(dir);

	if (!ent) {
		if (errno)
			return LS_ERROR;
		return LS_EOF;
	}

	*dst = convert_dirent(ent);
	return LS_OK;
}

static struct entry *read_entries_dir(DIR *dir, size_t *count)
{
	struct entry *entries = NULL;
	size_t size = 0;
	size_t cap = 0;

	struct entry current;
	enum result res;

	while ((res = read_entry(&current, dir)) == LS_OK) {
		if (size == cap) {
			size_t new_cap = cap == 0 ? 1 : cap * 2;
			struct entry *new_entries = ls_reallocarray(
				entries, sizeof(struct entry), size, new_cap);

			if (!new_entries) {
				res = LS_ERROR;
				break;
			}

                        cap = new_cap;
			entries = new_entries;
		}
		entries[size++] = current;
	}

	if (res == LS_ERROR) {
		print_error();
		free(entries);
		entries = NULL;
		size = 0;
	}

	*count = size;
	return entries;
}

static struct entry *read_entries(const char *name, size_t *count)
{
	struct entry *entries = NULL;

	DIR *dir = opendir(name);
	if (dir)
		entries = read_entries_dir(dir, count);

	if (!dir || closedir(dir) < 0) {
		print_error();
		free(entries);
		entries = NULL;
		*count = 0;
	}

	return entries;
}

static void print_entry(const struct entry *entry)
{
        printf("%s ", entry->filename);
}

static void print_entries(const struct entry *entries, size_t count)
{
        for (size_t i = 0; i < count; ++i) {
                print_entry(&entries[i]);
        }
}

static int cmp_entry(const void *a, const void *b)
{
        const struct entry *entry_a = a;
        const struct entry *entry_b = b;

        return ls_strcmp(entry_a->filename, entry_b->filename);
}

static void print_dir(const char *name)
{
        size_t count;
        struct entry *entries = read_entries(name, &count);
        if (entries)
                print_entries(entries, count);

        sort(entries, count, sizeof(*entries), cmp_entry);
        free(entries);
}

#ifndef LS_TEST

int main(int argc, char **argv)
{
        program_name = argv[1];

        if (argc < 2)
                return 1;
        print_dir(argv[1]);
        return 0;
}

#endif
