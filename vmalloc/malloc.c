#include	"vmhdr.h"

#if _std_malloc || _BLD_INSTRUMENT_ || cray
int	_STUB_malloc;
#else

/*	malloc compatibility functions.
**	These are aware of debugging/profiling and driven by the environment variables:
**	VMETHOD: select an allocation method by name.
**
**	VMPROFILE: if is a file name, write profile data to it.
**	VMTRACE: if is a file name, write trace data to it.
**	The pattern %p in a file name will be replaced by the process ID.
**
**	VMDEBUG:
**		a:			abort on any warning
**		[decimal]:		period to check arena.
**		0x[hexadecimal]:	address to watch.
**
**	Written by Kiem-Phong Vo, kpv@research.att.com, 01/16/94.
*/

#if 0
#include	<stat.h>
#endif
#if 0
#include	<sys/stat.h>
#endif
//#include	<fcntl.h>

#ifdef S_IRUSR
#define CREAT_MODE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
#else
#define CREAT_MODE	0644
#endif

#undef malloc
#undef free
#undef realloc
#undef calloc
#undef cfree
#undef memalign
#undef valloc

#if 0
#if __STD_C
static Vmulong_t atou(char** sp)
#else
static Vmulong_t atou(sp)
char**	sp;
#endif
{
	char*		s = *sp;
	Vmulong_t	v = 0;

	if(s[0] == '0' && (s[1] == 'x' || s[1] == 'X') )
	{	for(s += 2; *s; ++s)
		{	if(*s >= '0' && *s <= '9')
				v = (v << 4) + (*s - '0');
			else if(*s >= 'a' && *s <= 'f')
				v = (v << 4) + (*s - 'a') + 10;
			else if(*s >= 'A' && *s <= 'F')
				v = (v << 4) + (*s - 'A') + 10;
			else break;
		}
	}
	else
	{	for(; *s; ++s)
		{	if(*s >= '0' && *s <= '9')
				v = v*10 + (*s - '0');
			else break;
		}
	}

	*sp = s;
	return v;
}
#endif 
/*static int		_Vmflinit = 0;
static Vmulong_t	_Vmdbcheck = 0;
static Vmulong_t	_Vmdbtime = 0; 
static int		_Vmpffd = -1; */

#if 0
#if __STD_C
static void pfprint(void)
#else
static void pfprint()
#endif
{
	if(Vmregion->meth.meth == VM_MTPROFILE)
		vmprofile(Vmregion,_Vmpffd);
}
#endif

#if __STD_C
Void_t* malloc(reg size_t size)
#else
Void_t* malloc(size)
reg size_t	size;
#endif
{
	return (*Vmregion->meth.allocf)(Vmregion,size);
}

#if __STD_C
Void_t* realloc(reg Void_t* data, reg size_t size)
#else
Void_t* realloc(data,size)
reg Void_t*	data;	/* block to be reallocated	*/
reg size_t	size;	/* new size			*/
#endif
{
	return (*Vmregion->meth.resizef)(Vmregion,data,size,VM_RSCOPY|VM_RSMOVE);
}

#if __STD_C
void free(reg Void_t* data)
#else
void free(data)
reg Void_t*	data;
#endif
{

	(void)(*Vmregion->meth.freef)(Vmregion,data);
}

#if __STD_C
Void_t* calloc(reg size_t n_obj, reg size_t s_obj)
#else
Void_t* calloc(n_obj, s_obj)
reg size_t	n_obj;
reg size_t	s_obj;
#endif
{
	return (*Vmregion->meth.resizef)(Vmregion,NIL(Void_t*),n_obj*s_obj,VM_RSZERO);
}

#if __STD_C
void cfree(reg Void_t* data)
#else
void cfree(data)
reg Void_t*	data;
#endif
{
	(void)(*Vmregion->meth.freef)(Vmregion,data);
}

#if __STD_C
Void_t* memalign(reg size_t align, reg size_t size)
#else
Void_t* memalign(align, size)
reg size_t	align;
reg size_t	size;
#endif
{
	return (*Vmregion->meth.alignf)(Vmregion,size,align);
}

#if __STD_C
Void_t* valloc(reg size_t size)
#else
Void_t* valloc(size)
reg size_t	size;
#endif
{
	GETPAGESIZE(_Vmpagesize);
	return (*Vmregion->meth.alignf)(Vmregion,size,_Vmpagesize);
}

#if _hdr_malloc

#define calloc	______calloc
#define free	______free
#define malloc	______malloc
#define realloc	______realloc

#include	<malloc.h>

#if _lib_mallopt
#if __STD_C
int mallopt(int cmd, int value)
#else
int mallopt(cmd, value)
int	cmd;
int	value;
#endif
{
	return 0;
}
#endif

#if _lib_mallinfo
#if __STD_C
struct mallinfo mallinfo(void)
#else
struct mallinfo mallinfo()
#endif
{
	Vmstat_t	sb;
	struct mallinfo	mi;

	memset(&mi,0,sizeof(mi));
	if(vmstat(Vmregion,&sb) >= 0)
	{	mi.arena = sb.extent;
		mi.ordblks = sb.n_busy+sb.n_free;
		mi.uordblks = sb.s_busy;
		mi.fordblks = sb.s_free;
	}
	return mi;
}
#endif

#if _lib_mstats
#if __STD_C
struct mstats mstats(void)
#else
struct mstats mstats()
#endif
{
	Vmstat_t	sb;
	struct mstats	ms;

	memset(&ms,0,sizeof(ms));
	if(vmstat(Vmregion,&sb) >= 0)
	{	ms.bytes_total = sb.extent;
		ms.chunks_used = sb.n_busy;
		ms.bytes_used = sb.s_busy;
		ms.chunks_free = sb.n_free;
		ms.bytes_free = sb.s_free;
	}
	return ms;
}
#endif

#endif/*_hdr_malloc*/

#if !_lib_alloca || _mal_alloca
#ifndef _stk_down
#define _stk_down	0
#endif
typedef struct _alloca_s	Alloca_t;
union _alloca_u
{	struct
	{	char*		addr;
		Alloca_t*	next;
	} head;
	char	array[ALIGN];
};
struct _alloca_s
{	union _alloca_u	head;
	Vmuchar_t	data[1];
};

#if __STD_C
Void_t* alloca(size_t size)
#else
Void_t* alloca(size)
size_t	size;
#endif
{	char		array[ALIGN];
	char*		file;
	int		line;
	Void_t*		func;
	reg Alloca_t*	f;
	static Alloca_t* Frame;

	VMFLF(Vmregion,file,line,func);
	while(Frame)
	{	if(( _stk_down && &array[0] > Frame->head.head.addr) ||
		   (!_stk_down && &array[0] < Frame->head.head.addr) )
		{	f = Frame;
			Frame = f->head.head.next;
			(void)(*Vmregion->meth.freef)(Vmregion,f);
		}
		else	break;
	}

	Vmregion->file = file;
	Vmregion->line = line;
	Vmregion->func = func;
	f = (Alloca_t*)(*Vmregion->meth.allocf)(Vmregion,size+sizeof(Alloca_t)-1);

	f->head.head.addr = &array[0];
	f->head.head.next = Frame;
	Frame = f;

	return (Void_t*)f->data;
}
#endif /*!_lib_alloca || _mal_alloca*/

#endif /*_std_malloc || _BLD_INSTRUMENT_ || cray*/