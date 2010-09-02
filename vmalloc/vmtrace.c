#include	"vmhdr.h"

/*	Turn on tracing for regions
**
**	Written by Kiem-Phong Vo, kpv@research.att.com, 01/16/94.
*/

static int	Trfile = -1;
static char	Trbuf[128];

#if __STD_C
static char* trstrcpy(char* to, char* from, int endc)
#else
static char* trstrcpy(to, from, endc)
char*	to;
char*	from;
int	endc;
#endif
{	reg int	n;

	n = strlen(from);
	memcpy(to,from,n);
	to += n;
	if((*to = endc) )
		to += 1;
	return to;
}

/* convert a long value to an ascii representation */
#if __STD_C
static char* tritoa(Vmulong_t v, int type)
#else
static char* tritoa(v, type)
Vmulong_t	v;	/* value to convert					*/
int		type;	/* =0 base-16, >0: unsigned base-10, <0: signed base-10	*/
#endif
{
	char*	s;

	s = &Trbuf[sizeof(Trbuf) - 1];
	*s-- = '\0';

	if(type == 0)		/* base-16 */
	{	reg char*	digit = "0123456789abcdef";
		do
		{	*s-- = digit[v&0xf];
			v >>= 4;
		} while(v);
	}
	else if(type > 0)	/* unsigned base-10 */
	{	do
		{	*s-- = (char)('0' + (v%10));
			v /= 10;
		} while(v);
	}
	else			/* signed base-10 */
	{	int	sign = ((long)v < 0);
		if(sign)
			v = (Vmulong_t)(-((long)v));
		do
		{	*s-- = (char)('0' + (v%10));
			v /= 10;
		} while(v);
		if(sign)
			*s-- = '-';
	}

	return s+1;
}

/* generate a trace of some call */
#if __STD_C
static void trtrace(Vmalloc_t* vm,
		    Vmuchar_t* oldaddr, Vmuchar_t* newaddr, size_t size, size_t align )
#else
static void trtrace(vm, oldaddr, newaddr, size, align)
Vmalloc_t*	vm;		/* region call was made from	*/
Vmuchar_t*	oldaddr;	/* old data address		*/
Vmuchar_t*	newaddr;	/* new data address		*/
size_t		size;		/* size of piece		*/
size_t		align;		/* alignment			*/
#endif
{
	char		buf[1024], *bufp, *endbuf;
	Vmdata_t*	vd = vm->data;
	char*		file = NIL(char*);
	int		line = 0;
	Void_t*		func = NIL(Void_t*);
	int		comma;
	
	int		type;
#define SLOP	64

	if(oldaddr == (Vmuchar_t*)(-1)) /* printing busy blocks */
	{	type = 0;
		oldaddr = NIL(Vmuchar_t*);
	}
	else
	{	type = vd->mode&VM_METHODS;
		VMFLF(vm,file,line,func);
	}

	if(Trfile < 0)
		return;

	bufp = buf; endbuf = buf+sizeof(buf);
	bufp = trstrcpy(bufp, tritoa(oldaddr ? VLONG(oldaddr) : 0L, 0), ':');
	bufp = trstrcpy(bufp, tritoa(newaddr ? VLONG(newaddr) : 0L, 0), ':');
	bufp = trstrcpy(bufp, tritoa((Vmulong_t)size, 1), ':');
	bufp = trstrcpy(bufp, tritoa((Vmulong_t)align, 1), ':');
	bufp = trstrcpy(bufp, tritoa(VLONG(vm), 0), ':');
	if(type&VM_MTBEST)
		bufp = trstrcpy(bufp, "b", ':');
	else if(type&VM_MTLAST)
		bufp = trstrcpy(bufp, "l", ':');
	else if(type&VM_MTPOOL)
		bufp = trstrcpy(bufp, "p", ':');
	else if(type&VM_MTPROFILE)
		bufp = trstrcpy(bufp, "s", ':');
	else if(type&VM_MTDEBUG)
		bufp = trstrcpy(bufp, "d", ':');
	else	bufp = trstrcpy(bufp, "u", ':');

	comma = 0;
	if(file && file[0] && line > 0)
	{	if((bufp + strlen(file) + SLOP) >= endbuf)
		{	char*	f;
			for(f = bufp + strlen(file); f > file; --f)
				if(f[-1] == '/' || f[-1] == '\\')
					break; 
			file = f;
		}

		bufp = trstrcpy(bufp, "file", '=');
		if((bufp + strlen(file) + SLOP) >= endbuf)
			bufp = trstrcpy(bufp, file, ',');
		else	bufp = trstrcpy(bufp, "NameTooLong", ',');
		bufp = trstrcpy(bufp, "line", '=');
		bufp = trstrcpy(bufp, tritoa((Vmulong_t)line,1), 0);
		comma = 1;
	}
	if(func)
	{	if(comma)
			*bufp++ = ',';
		bufp = trstrcpy(bufp, "func", '=');
		bufp = trstrcpy(bufp, tritoa((Vmulong_t)func,0), 0);
		comma = 1;
	}
	if(comma)
		*bufp++ = ':';

	*bufp++ = '\n';
	*bufp = '\0';

	write(Trfile,buf,(bufp-buf));
}

#if __STD_C
int vmtrace(int file)
#else
int vmtrace(file)
int	file;
#endif
{
	int	fd;

	_Vmstrcpy = trstrcpy;
	_Vmitoa = tritoa;
	_Vmtrace = trtrace;

	fd = Trfile;
	Trfile = file;
	return fd;
}

#if __STD_C
int vmtrbusy(Vmalloc_t* vm)
#else
int vmtrbusy(vm)
Vmalloc_t*	vm;
#endif
{
	Seg_t*		seg;
	Vmdata_t*	vd = vm->data;

	if(Trfile < 0 || !(vd->mode&(VM_MTBEST|VM_MTDEBUG|VM_MTPROFILE)))
		return -1;

	for(seg = vd->seg; seg; seg = seg->next)
	{	Block_t		*b, *endb;
		Vmuchar_t*	data;
		size_t		s;

		for(b = SEGBLOCK(seg), endb = BLOCK(seg->baddr); b < endb; )
		{	if(ISJUNK(SIZE(b)) || !ISBUSY(SIZE(b)))
				continue;

			data = DATA(b);
			if(vd->mode&VM_MTDEBUG)
			{	data = DB2DEBUG(data);
				s = DBSIZE(data);
			}
			else if(vd->mode&VM_MTPROFILE)
				s = PFSIZE(data);
			else	s = SIZE(b)&~BITS;

			trtrace(vm, (Vmuchar_t*)(-1), data, s, 0);

			b = (Block_t*)((Vmuchar_t*)DATA(b) + (SIZE(b)&~BITS) );
		}
	}

	return 0;
}