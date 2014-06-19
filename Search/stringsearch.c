#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define DEFAULT_LEN 	32
#define REALLOC_INC	8
#define MAX_ALPHA	256
#define rdtscl(val) asm volatile ("rdtsc" : "=A" (val))

int read_file (const char *path, char **buffer);
/* kmp */
void kmp_table (int **T, const char *W, int len);
int kmp_search (const char *text, const char *term, size_t text_len, size_t term_len, size_t **pos_array);
/* rabin karp */
int rk_hash (const char *src, int len);
int rk_search (const char *text, const char *term, int text_len, int term_len, size_t **pos_array);
/* automata */
int am_nextstate (const char *term, int term_len, int state, int x);
void am_table (const char *term, int term_len, int **table);
int am_search (const char *text, const char *term, size_t text_len, size_t term_len, size_t **pos_array);

int read_file (const char *path, char **buffer)
{
	struct stat buf;
	char *map;
	int fd = open(path, O_RDONLY);

	if (fd < 0 || fstat(fd, &buf) || buf.st_size <= 0)
		return -1;
		
	if ((map = mmap(NULL, buf.st_size, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED)
		return -1;
		
	if ((*buffer = malloc(buf.st_size)) == NULL)
		return -1;
		
	memcpy(*buffer, map, buf.st_size);
	
	munmap(map, buf.st_size);
	close(fd);
	
	return buf.st_size;
}

void kmp_table (int **T, const char *W, int len)
{
	int pos = 2;
	int cnd = 0;
	
	*T = calloc(sizeof(int), len + 2);

	(*T)[0] = -1;
	(*T)[1] = 0;
	
	while (pos < len) {
		if (W[pos - 1] == W[cnd])
			(*T)[pos++] = ++cnd;
		else if (cnd > 0)
			cnd = (*T)[cnd];
		else
			(*T)[pos++] = 0;
	}
}

int kmp_search (const char *text, const char *term, size_t text_len, size_t term_len, size_t **pos_array)
{
	int *table;
	int c = 0, m = 0, i = 0;
	int l = DEFAULT_LEN;
	
	if ((*pos_array = calloc(sizeof(size_t), l)) == NULL)
		return -1;
		
	kmp_table(&table, term, term_len);	

	while ((m + i) < text_len) {
		if (term[i] == text[m + i]) {
			if (i++ == (term_len - 1)) {
				if ((c + 1) >= l) {
					l += REALLOC_INC;
					*pos_array = realloc(*pos_array, l * sizeof(size_t));
				}
				(*pos_array)[c++] = m;
			}
		} else {
			if (table[i] > -1) {
				i = table[i];
				m += (i - table[i]);
			} else { 
				i = 0;
				m++;
			}
		}
	}
	
	free(table);

	return c;
}

int rk_hash (const char *src, int len)
{
    	unsigned h = 0;
    	
    	while (len--)
       		h = 33 * h + *src++;

    	return h;
}

int rk_search (const char *text, const char *term, int text_len, int term_len, size_t **pos_array)
{
	int i, c = 0;
	int hsub = rk_hash(term, term_len);
	int l = DEFAULT_LEN;
	
	if ((*pos_array = calloc(sizeof(size_t), l)) == NULL)
		return -1;
		
	for (i = 0; i < (text_len - term_len) + 1; i++) {
		if (rk_hash(text + i, term_len) == hsub) {
			if (memcmp(text + i, term, term_len - 1) == 0) {
				if ((c + 1) >= l) {
					l += REALLOC_INC;
					*pos_array = realloc(*pos_array, l * sizeof(size_t));
				}
				(*pos_array)[c++] = i;
			}
		}
	}
	
	return c;
}

int am_nextstate (const char *term, int term_len, int state, int x)
{
	int ns, i;
	
	if (state < term_len && x == term[state])
		return state + 1;

	for (ns = state; ns > 0; ns--) {
		if (term[ns - 1] == x) {
			for (i = 0; i < ns - 1; i++)
				if (term[i] != term[state - ns + 1 + i])
					break;
			if (i == ns - 1)
				return ns;
		}
	}
 
	return 0;
}
 
void am_table (const char *term, int term_len, int **table)
{
	int state, x;
	
	for (state = 0; state <= term_len; ++state)
		for (x = 0; x < MAX_ALPHA; ++x)
			table[state][x] = am_nextstate(term, term_len, state, x);
}

int am_search (const char *text, const char *term, size_t text_len, size_t term_len, size_t **pos_array)
{
	int **table;
	int c = 0, i, state = 0;
	int l = DEFAULT_LEN;
	
	if ((*pos_array = calloc(sizeof(size_t), l)) == NULL)
		return -1;
		
	if ((table = malloc(sizeof(void *) * (term_len + 1))) == NULL)
		return -1;
		
	for (i = 0; i < (term_len + 1); i++)
		if ((table[i] = calloc(sizeof(int), MAX_ALPHA)) == NULL)
			return -1;
 
	am_table(term, term_len, table);

	for (i = 0; i < text_len; i++) {
		state = table[state][text[i]];
		if (state == term_len) {
			if ((c + 1) >= l) {
				l += REALLOC_INC;
				*pos_array = realloc(*pos_array, l * sizeof(size_t));
			}
			(*pos_array)[c++] = (i - term_len + 1);
		}
	}
	
	for (i = 0; i < (term_len + 1); i++)
		free(table[i]);
	free(table);
    
	return c;
}

int search_file (const char *path, const char *term)
{
	int i;
	char *text;
	int text_len;
	size_t term_len = strlen(term);
	
	int rk_found, kp_found, am_found;
	size_t *rk_pos_array, *kp_pos_array, *am_pos_array;
	unsigned long t1, t2;
	
	if ((text_len = read_file(path, &text)) <= 0)
		perror("file read");
		
	printf("read %d bytes, searching for \"%s\" in \"%s\"\n", text_len, term, path);
	
	rdtscl(t1);
	if ((rk_found = rk_search(text, term, text_len, term_len, &rk_pos_array)) <= 0)
		printf("error searching\n");
	rdtscl(t2);
	printf("rk: found %d in %ld cycles\n", rk_found, t2 - t1);
	
	rdtscl(t1);	
	if ((kp_found = kmp_search(text, term, text_len, term_len, &kp_pos_array)) <= 0)
		printf("error searching\n");	
	rdtscl(t2);
	printf("kmp: found %d in %ld cycles\n", kp_found, t2 - t1);
		
	rdtscl(t1);		
	if ((am_found = am_search(text, term, text_len, term_len, &am_pos_array)) <= 0)
		printf("error searching\n");				
	rdtscl(t2);
	printf("am: found %d in %ld cycles\n", am_found, t2 - t1);
		
	if (kp_found != rk_found || kp_found != am_found) {
		printf("error!\n");
	}
	
	for (i = 0; i < kp_found; i++) {
		if (kp_pos_array[i] != rk_pos_array[i] || kp_pos_array[i] != am_pos_array[i])
			printf("search term %d error\n", i);
	//	printf("%ld/%ld/%ld: %.10s\n", kp_pos_array[i], rk_pos_array[i], am_pos_array[i], text + rk_pos_array[i]);
	}

	free(kp_pos_array);
	free(rk_pos_array);
	free(am_pos_array);
	free(text);
	
	return 0;
}

int main (void)
{
	return search_file("mobydick.txt", "the");
}

