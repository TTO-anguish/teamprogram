/**
 * @FilePath     : /home/ZF/bottom_program/server_item/cJSON.c
 * @Description  :  
 * @Author       : CMH,ZF,ZY,SSS
 * @Version      : 0.0.1
 * @LastEditors  : zongfei
 * @LastEditTime : 2024-08-23 11:19:13
**/
/* cJSON */
/* 在C语言中的JSON解析器。 */

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include "cJSON.h"

static const char *ep;

const char *cJSON_GetErrorPtr(void) {return ep;} /* 获取错误指针 */

static int cJSON_strcasecmp(const char *s1,const char *s2)
{
	if (!s1) return (s1==s2)?0:1;if (!s2) return 1; /* 比较字符串，忽略大小写 */
	for(; tolower(*s1) == tolower(*s2); ++s1, ++s2)	if(*s1 == 0)	return 0;
	return tolower(*(const unsigned char *)s1) - tolower(*(const unsigned char *)s2);
}

static void *(*cJSON_malloc)(size_t sz) = malloc; /* 分配内存 */
static void (*cJSON_free)(void *ptr) = free; /* 释放内存 */

static char* cJSON_strdup(const char* str)
{
      size_t len;
      char* copy;

      len = strlen(str) + 1; /* 复制字符串 */
      if (!(copy = (char*)cJSON_malloc(len))) return 0;
      memcpy(copy,str,len);
      return copy;
}

void cJSON_InitHooks(cJSON_Hooks* hooks)
{
    if (!hooks) { /* 重置钩子 */
        cJSON_malloc = malloc;
        cJSON_free = free;
        return;
    }

	cJSON_malloc = (hooks->malloc_fn)?hooks->malloc_fn:malloc; /* 初始化钩子 */
	cJSON_free	 = (hooks->free_fn)?hooks->free_fn:free;
}

/* 内部构造函数。 */
static cJSON *cJSON_New_Item(void)
{
	cJSON* node = (cJSON*)cJSON_malloc(sizeof(cJSON)); /* 创建新项 */
	if (node) memset(node,0,sizeof(cJSON));
	return node;
}

/* 删除一个cJSON结构。 */
void cJSON_Delete(cJSON *c)
{
	cJSON *next;
	while (c)
	{
		next=c->next; /* 遍历并删除结构 */
		if (!(c->type&cJSON_IsReference) && c->child) cJSON_Delete(c->child);
		if (!(c->type&cJSON_IsReference) && c->valuestring) cJSON_free(c->valuestring);
		if (!(c->type&cJSON_StringIsConst) && c->string) cJSON_free(c->string);
		cJSON_free(c);
		c=next;
	}
}

/* 解析输入文本以生成一个数字，并将结果填充到item中。 */
static const char *parse_number(cJSON *item,const char *num)
{
	double n=0,sign=1,scale=0;int subscale=0,signsubscale=1; /* 解析数字 */

	if (*num=='-') sign=-1,num++;	/* 有符号？ */
	if (*num=='0') num++;			/* 是零 */
	if (*num>='1' && *num<='9')	do	n=(n*10.0)+(*num++ -'0');	while (*num>='0' && *num<='9');	/* 数字？ */
	if (*num=='.' && num[1]>='0' && num[1]<='9') {num++;		do	n=(n*10.0)+(*num++ -'0'),scale--; while (*num>='0' && *num<='9');}	/* 小数部分？ */
	if (*num=='e' || *num=='E')		/* 指数？ */
	{	num++;if (*num=='+') num++;	else if (*num=='-') signsubscale=-1,num++;		/* 带符号？ */
		while (*num>='0' && *num<='9') subscale=(subscale*10)+(*num++ - '0');	/* 数字？ */
	}

	n=sign*n*pow(10.0,(scale+subscale*signsubscale));	/* 数字 = +/- 数字.小数 * 10^+/- 指数 */
	
	item->valuedouble=n;
	item->valueint=(int)n;
	item->type=cJSON_Number;
	return num;
}

static int pow2gt (int x)	{	--x;	x|=x>>1;	x|=x>>2;	x|=x>>4;	x|=x>>8;	x|=x>>16;	return x+1;	} /* 计算大于等于x的最小2的幂 */

typedef struct {char *buffer; int length; int offset; } printbuffer; /* 打印缓冲区结构 */

static char* ensure(printbuffer *p,int needed)
{
	char *newbuffer;int newsize;
	if (!p || !p->buffer) return 0; /* 确保缓冲区足够大 */
	needed+=p->offset;
	if (needed<=p->length) return p->buffer+p->offset;

	newsize=pow2gt(needed); /* 扩展缓冲区 */
	newbuffer=(char*)cJSON_malloc(newsize);
	if (!newbuffer) {cJSON_free(p->buffer);p->length=0,p->buffer=0;return 0;}
	if (newbuffer) memcpy(newbuffer,p->buffer,p->length);
	cJSON_free(p->buffer);
	p->length=newsize;
	p->buffer=newbuffer;
	return newbuffer+p->offset;
}

static int update(printbuffer *p)
{
	char *str;
	if (!p || !p->buffer) return 0; /* 更新打印缓冲区的偏移量 */
	str=p->buffer+p->offset;
	return p->offset+strlen(str);
}

/* 将给定项的数字渲染成字符串。 */
static char *print_number(cJSON *item,printbuffer *p)
{
	char *str=0;
	double d=item->valuedouble; /* 打印数字 */
	if (d==0)
	{
		if (p)	str=ensure(p,2); /* 特殊情况：0 */
		else	str=(char*)cJSON_malloc(2);	
		if (str) strcpy(str,"0");
	}
	else if (fabs(((double)item->valueint)-d)<=DBL_EPSILON && d<=INT_MAX && d>=INT_MIN)
	{
		if (p)	str=ensure(p,21); /* 整数 */
		else	str=(char*)cJSON_malloc(21);	
		if (str)	sprintf(str,"%d",item->valueint);
	}
	else
	{
		if (p)	str=ensure(p,64); /* 浮点数 */
		else	str=(char*)cJSON_malloc(64);	
		if (str)
		{
			if (fpclassify(d) != FP_ZERO && !isnormal(d))				sprintf(str,"null"); /* 非正常值 */
			else if (fabs(floor(d)-d)<=DBL_EPSILON && fabs(d)<1.0e60)	sprintf(str,"%.0f",d); /* 无小数部分 */
			else if (fabs(d)<1.0e-6 || fabs(d)>1.0e9)					sprintf(str,"%e",d); /* 指数形式 */
			else														sprintf(str,"%f",d); /* 默认 */
		}
	}
	return str;
}

static unsigned parse_hex4(const char *str)
{
	unsigned h=0;
	if (*str>='0' && *str<='9') h+=(*str)-'0'; else if (*str>='A' && *str<='F') h+=10+(*str)-'A'; else if (*str>='a' && *str<='f') h+=10+(*str)-'a'; else return 0; /* 解析4位十六进制数 */
	h=h<<4;str++;
	if (*str>='0' && *str<='9') h+=(*str)-'0'; else if (*str>='A' && *str<='F') h+=10+(*str)-'A'; else if (*str>='a' && *str<='f') h+=10+(*str)-'a'; else return 0;
	h=h<<4;str++;
	if (*str>='0' && *str<='9') h+=(*str)-'0'; else if (*str>='A' && *str<='F') h+=10+(*str)-'A'; else if (*str>='a' && *str<='f') h+=10+(*str)-'a'; else return 0;
	h=h<<4;str++;
	if (*str>='0' && *str<='9') h+=(*str)-'0'; else if (*str>='A' && *str<='F') h+=10+(*str)-'A'; else if (*str>='a' && *str<='f') h+=10+(*str)-'a'; else return 0;
	return h;
}

/* 解析输入文本到一个未转义的cstring，并填充到item中。 */
static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
static const char *parse_string(cJSON *item,const char *str)
{
	const char *ptr=str+1;char *ptr2;char *out;int len=0;unsigned uc,uc2;
	if (*str!='\"') {ep=str;return 0;}	/* 不是字符串！ */
	
	while (*ptr!='\"' && *ptr && ++len) if (*ptr++ == '\\') ptr++;	/* 跳过转义引号。 */
	
	out=(char*)cJSON_malloc(len+1);	/* 大致需要的字符串长度。 */
	if (!out) return 0;
	
	ptr=str+1;ptr2=out;
	while (*ptr!='\"' && *ptr)
	{
		if (*ptr!='\\') *ptr2++=*ptr++;
		else
		{
			ptr++;
			switch (*ptr)
			{
				case 'b': *ptr2++='\b';	break; /* 转义字符 */
				case 'f': *ptr2++='\f';	break;
				case 'n': *ptr2++='\n';	break;
				case 'r': *ptr2++='\r';	break;
				case 't': *ptr2++='\t';	break;
				case 'u':	 /* utf16转utf8。 */
					uc=parse_hex4(ptr+1);ptr+=4;	/* 获取unicode字符。 */

					if ((uc>=0xDC00 && uc<=0xDFFF) || uc==0)	break;	/* 检查无效。	*/

					if (uc>=0xD800 && uc<=0xDBFF)	/* UTF16代理对。	*/
					{
						if (ptr[1]!='\\' || ptr[2]!='u')	break;	/* 缺少代理的第二部分。	*/
						uc2=parse_hex4(ptr+3);ptr+=6;
						if (uc2<0xDC00 || uc2>0xDFFF)		break;	/* 无效的代理第二部分。	*/
						uc=0x10000 + (((uc&0x3FF)<<10) | (uc2&0x3FF));
					}

					len=4;if (uc<0x80) len=1;else if (uc<0x800) len=2;else if (uc<0x10000) len=3; ptr2+=len;
					
					switch (len) {
						case 4: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
						case 3: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
						case 2: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
						case 1: *--ptr2 =(uc | firstByteMark[len]);
					}
					ptr2+=len;
					break;
				default:  *ptr2++=*ptr; break;
			}
			ptr++;
		}
	}
	*ptr2=0;
	if (*ptr=='\"') ptr++;
	item->valuestring=out;
	item->type=cJSON_String;
	return ptr;
}

/* 将提供的cstring渲染为可以打印的转义版本。 */
static char *print_string_ptr(const char *str,printbuffer *p)
{
	const char *ptr;char *ptr2,*out;int len=0,flag=0;unsigned char token;
	
	for (ptr=str;*ptr;ptr++) flag|=((*ptr>0 && *ptr<32)||(*ptr=='\"')||(*ptr=='\\'))?1:0; /* 检查是否需要转义 */
	if (!flag)
	{
		len=ptr-str;
		if (p) out=ensure(p,len+3); /* 不需要转义 */
		else		out=(char*)cJSON_malloc(len+3);
		if (!out) return 0;
		ptr2=out;*ptr2++='\"';
		strcpy(ptr2,str);
		ptr2[len]='\"';
		ptr2[len+1]=0;
		return out;
	}
	
	if (!str)
	{
		if (p)	out=ensure(p,3); /* 空字符串 */
		else	out=(char*)cJSON_malloc(3);
		if (!out) return 0;
		strcpy(out,"\"\"");
		return out;
	}
	ptr=str;while ((token=*ptr) && ++len) {if (strchr("\"\\\b\f\n\r\t",token)) len++; else if (token<32) len+=5;ptr++;} /* 计算转义后的长度 */
	
	if (p)	out=ensure(p,len+3); /* 分配足够的内存 */
	else	out=(char*)cJSON_malloc(len+3);
	if (!out) return 0;

	ptr2=out;ptr=str;
	*ptr2++='\"';
	while (*ptr)
	{
		if ((unsigned char)*ptr>31 && *ptr!='\"' && *ptr!='\\') *ptr2++=*ptr++; /* 直接复制 */
		else
		{
			*ptr2++='\\';
			switch (token=*ptr++)
			{
				case '\\':	*ptr2++='\\';	break; /* 转义字符 */
				case '\"':	*ptr2++='\"';	break;
				case '\b':	*ptr2++='b';	break;
				case '\f':	*ptr2++='f';	break;
				case '\n':	*ptr2++='n';	break;
				case '\r':	*ptr2++='r';	break;
				case '\t':	*ptr2++='t';	break;
				default: sprintf(ptr2,"u%04x",token);ptr2+=5;	break;	/* 转义并打印 */
			}
		}
	}
	*ptr2++='\"';*ptr2++=0;
	return out;
}
/* 在项上调用print_string_ptr（这是有用的）。 */
static char *print_string(cJSON *item,printbuffer *p)	{return print_string_ptr(item->valuestring,p);}

/* 预先声明这些原型。 */
static const char *parse_value(cJSON *item,const char *value);
static char *print_value(cJSON *item,int depth,int fmt,printbuffer *p);
static const char *parse_array(cJSON *item,const char *value);
static char *print_array(cJSON *item,int depth,int fmt,printbuffer *p);
static const char *parse_object(cJSON *item,const char *value);
static char *print_object(cJSON *item,int depth,int fmt,printbuffer *p);

/* 跳过空白和换行符的实用程序 */
static const char *skip(const char *in) {while (in && *in && (unsigned char)*in<=32) in++; return in;}

/* 解析一个对象 - 创建一个新的根，并填充。 */
cJSON *cJSON_ParseWithOpts(const char *value,const char **return_parse_end,int require_null_terminated)
{
	const char *end=0;
	cJSON *c=cJSON_New_Item(); /* 创建新的JSON项 */
	ep=0;
	if (!c) return 0;       /* 内存分配失败 */

	end=parse_value(c,skip(value)); /* 解析值 */
	if (!end)	{cJSON_Delete(c);return 0;}	/* 解析失败。ep被设置。 */

	/* 如果我们需要没有附加垃圾的空终止的JSON，跳过然后检查空终止符 */
	if (require_null_terminated) {end=skip(end);if (*end) {cJSON_Delete(c);ep=end;return 0;}}
	if (return_parse_end) *return_parse_end=end;
	return c;
}
/* cJSON_Parse的默认选项 */
cJSON *cJSON_Parse(const char *value) {return cJSON_ParseWithOpts(value,0,0);} /* 解析JSON文本 */

/* 将cJSON项/实体/结构渲染为文本。 */
char *cJSON_Print(cJSON *item)				{return print_value(item,0,1,0);} /* 格式化打印 */
char *cJSON_PrintUnformatted(cJSON *item)	{return print_value(item,0,0,0);} /* 无格式打印 */

char *cJSON_PrintBuffered(cJSON *item,int prebuffer,int fmt)
{
	printbuffer p; /* 使用缓冲区打印 */
	p.buffer=(char*)cJSON_malloc(prebuffer);
	p.length=prebuffer;
	p.offset=0;
	return print_value(item,0,fmt,&p);
	return p.buffer;
}


/* 解析器核心 - 遇到文本时适当处理。 */
static const char *parse_value(cJSON *item,const char *value)
{
	if (!value)						return 0;	/* 失败在空值上。 */
	if (!strncmp(value,"null",4))	{ item->type=cJSON_NULL;  return value+4; } /* null值 */
	if (!strncmp(value,"false",5))	{ item->type=cJSON_False; return value+5; } /* false值 */
	if (!strncmp(value,"true",4))	{ item->type=cJSON_True; item->valueint=1;	return value+4; } /* true值 */
	if (*value=='\"')				{ return parse_string(item,value); } /* 字符串 */
	if (*value=='-' || (*value>='0' && *value<='9'))	{ return parse_number(item,value); } /* 数字 */
	if (*value=='[')				{ return parse_array(item,value); } /* 数组 */
	if (*value=='{')				{ return parse_object(item,value); } /* 对象 */

	ep=value;return 0;	/* 失败。 */
}

/* 将值渲染为文本。 */
static char *print_value(cJSON *item,int depth,int fmt,printbuffer *p)
{
	char *out=0;
	if (!item) return 0; /* 项为空 */
	if (p)
	{
		switch ((item->type)&255)
		{
			case cJSON_NULL:	{out=ensure(p,5);	if (out) strcpy(out,"null");	break;} /* null */
			case cJSON_False:	{out=ensure(p,6);	if (out) strcpy(out,"false");	break;} /* false */
			case cJSON_True:	{out=ensure(p,5);	if (out) strcpy(out,"true");	break;} /* true */
			case cJSON_Number:	out=print_number(item,p);break; /* 数字 */
			case cJSON_String:	out=print_string(item,p);break; /* 字符串 */
			case cJSON_Array:	out=print_array(item,depth,fmt,p);break; /* 数组 */
			case cJSON_Object:	out=print_object(item,depth,fmt,p);break; /* 对象 */
		}
	}
	else
	{
		switch ((item->type)&255)
		{
			case cJSON_NULL:	out=cJSON_strdup("null");	break; /* null */
			case cJSON_False:	out=cJSON_strdup("false");break; /* false */
			case cJSON_True:	out=cJSON_strdup("true"); break; /* true */
			case cJSON_Number:	out=print_number(item,0);break; /* 数字 */
			case cJSON_String:	out=print_string(item,0);break; /* 字符串 */
			case cJSON_Array:	out=print_array(item,depth,fmt,0);break; /* 数组 */
			case cJSON_Object:	out=print_object(item,depth,fmt,0);break; /* 对象 */
		}
	}
	return out;
}

/* 从输入文本构建数组。 */
static const char *parse_array(cJSON *item,const char *value)
{
	cJSON *child;
	if (*value!='[')	{ep=value;return 0;}	/* 不是数组！ */

	item->type=cJSON_Array;
	value=skip(value+1);
	if (*value==']') return value+1;	/* 空数组。 */

	item->child=child=cJSON_New_Item();
	if (!item->child) return 0;		 /* 内存分配失败 */
	value=skip(parse_value(child,skip(value)));	/* 跳过任何空格，获取值。 */
	if (!value) return 0;

	while (*value==',')
	{
		cJSON *new_item;
		if (!(new_item=cJSON_New_Item())) return 0; 	/* 内存分配失败 */
		child->next=new_item;new_item->prev=child;child=new_item;
		value=skip(parse_value(child,skip(value+1)));
		if (!value) return 0;	/* 内存分配失败 */
	}

	if (*value==']') return value+1;	/* 数组结束 */
	ep=value;return 0;	/* 格式错误。 */
}

/* 将数组渲染为文本 */
static char *print_array(cJSON *item,int depth,int fmt,printbuffer *p)
{
	char **entries;
	char *out=0,*ptr,*ret;int len=5;
	cJSON *child=item->child;
	int numentries=0,i=0,fail=0;
	size_t tmplen=0;
	
	/* 数组中有多少条目？ */
	while (child) numentries++,child=child->next; /* 计算条目数量 */
	/* 显式处理 numentries==0 的情况 */
	if (!numentries)
	{
		if (p)	out=ensure(p,3); /* 使用 printbuffer */
		else	out=(char*)cJSON_malloc(3); /* 直接分配内存 */
		if (out) strcpy(out,"[]"); /* 复制空数组文本 */
		return out;
	}

	if (p)
	{
		/* 组成输出数组。 */
		i=p->offset;
		ptr=ensure(p,1);if (!ptr) return 0;	*ptr='[';	p->offset++; /* 添加数组开始符 */
		child=item->child;
		while (child && !fail)
		{
			print_value(child,depth+1,fmt,p); /* 打印子元素 */
			p->offset=update(p); /* 更新偏移量 */
			if (child->next) {len=fmt?2:1;ptr=ensure(p,len+1);if (!ptr) return 0;*ptr++=',';if(fmt)*ptr++=' ';*ptr=0;p->offset+=len;} /* 添加逗号和空格（如果格式化） */
			child=child->next;
		}
		ptr=ensure(p,2);if (!ptr) return 0;	*ptr++=']';*ptr=0; /* 添加数组结束符 */
		out=(p->buffer)+i;
	}
	else
	{
		/* 分配一个数组来保存每个值 */
		entries=(char**)cJSON_malloc(numentries*sizeof(char*));
		if (!entries) return 0; /* 内存分配失败 */
		memset(entries,0,numentries*sizeof(char*)); /* 初始化内存 */
		/* 检索所有结果： */
		child=item->child;
		while (child && !fail)
		{
			ret=print_value(child,depth+1,fmt,0); /* 打印值 */
			entries[i++]=ret; /* 存储结果 */
			if (ret) len+=strlen(ret)+2+(fmt?1:0); else fail=1; /* 计算长度 */
			child=child->next;
		}
		
		/* 如果没有失败，尝试分配输出字符串的内存 */
		if (!fail)	out=(char*)cJSON_malloc(len); /* 分配内存 */
		/* 如果失败，我们失败。 */
		if (!out) fail=1; /* 内存分配失败 */

		/* 处理失败。 */
		if (fail)
		{
			for (i=0;i<numentries;i++) if (entries[i]) cJSON_free(entries[i]); /* 释放条目内存 */
			cJSON_free(entries); /* 释放条目数组内存 */
			return 0;
		}
		
		/* 组成输出数组。 */
		*out='['; /* 开始数组 */
		ptr=out+1;*ptr=0; /* 初始化指针 */
		for (i=0;i<numentries;i++)
		{
			tmplen=strlen(entries[i]);memcpy(ptr,entries[i],tmplen);ptr+=tmplen; /* 复制条目 */
			if (i!=numentries-1) {*ptr++=',';if(fmt)*ptr++=' ';*ptr=0;} /* 添加逗号和空格（如果格式化） */
			cJSON_free(entries[i]); /* 释放条目内存 */
		}
		cJSON_free(entries); /* 释放条目数组内存 */
		*ptr++=']';*ptr++=0; /* 结束数组 */
	}
	return out;	
}

/* 从文本构建对象。 */
static const char *parse_object(cJSON *item,const char *value)
{
	cJSON *child;
	if (*value!='{')	{ep=value;return 0;}	/* 不是对象！ */

	item->type=cJSON_Object;
	value=skip(value+1);
	if (*value=='}') return value+1;	/* 空对象。 */
	
	item->child=child=cJSON_New_Item();
	if (!item->child) return 0; /* 内存分配失败 */
	value=skip(parse_string(child,skip(value))); /* 跳过任何空格，获取字符串。 */
	if (!value) return 0;
	child->string=child->valuestring;child->valuestring=0;
	if (*value!=':') {ep=value;return 0;}	/* 失败！ */
	value=skip(parse_value(child,skip(value+1)));	/* 跳过任何空格，获取值。 */
	if (!value) return 0;
	
	while (*value==',')
	{
		cJSON *new_item;
		if (!(new_item=cJSON_New_Item()))	return 0; /* 内存分配失败 */
		child->next=new_item;new_item->prev=child;child=new_item;
		value=skip(parse_string(child,skip(value+1))); /* 跳过空格，获取字符串。 */
		if (!value) return 0;
		child->string=child->valuestring;child->valuestring=0;
		if (*value!=':') {ep=value;return 0;}	/* 失败！ */
		value=skip(parse_value(child,skip(value+1)));	/* 跳过空格，获取值。 */
		if (!value) return 0;
	}
	
	if (*value=='}') return value+1;	/* 对象结束 */
	ep=value;return 0;	/* 格式错误。 */
}

/* 将对象渲染为文本。 */
static char *print_object(cJSON *item,int depth,int fmt,printbuffer *p)
{
	char **entries=0,**names=0;
	char *out=0,*ptr,*ret,*str;int len=7,i=0,j;
	cJSON *child=item->child;
	int numentries=0,fail=0;
	size_t tmplen=0;
	/* 计算条目数量。 */
	while (child) numentries++,child=child->next; /* 计数条目 */
	/* 显式处理空对象情况 */
	if (!numentries)
	{
		if (p) out=ensure(p,fmt?depth+4:3); /* 使用 printbuffer */
		else	out=(char*)cJSON_malloc(fmt?depth+4:3); /* 直接分配内存 */
		if (!out)	return 0; /* 内存分配失败 */
		ptr=out;*ptr++='{';
		if (fmt) {*ptr++='\n';for (i=0;i<depth-1;i++) *ptr++='\t';} /* 添加格式化的缩进 */
		*ptr++='}';*ptr++=0; /* 结束对象 */
		return out;
	}
	if (p)
	{
		/* 组成输出： */
		i=p->offset;
		len=fmt?2:1;	ptr=ensure(p,len+1);	if (!ptr) return 0;
		*ptr++='{';	if (fmt) *ptr++='\n';	*ptr=0;	p->offset+=len; /* 添加对象开始符和换行符（如果格式化） */
		child=item->child;depth++;
		while (child)
		{
			if (fmt)
			{
				ptr=ensure(p,depth);	if (!ptr) return 0;
				for (j=0;j<depth;j++) *ptr++='\t'; /* 添加缩进 */
				p->offset+=depth;
			}
			print_string_ptr(child->string,p); /* 打印键名 */
			p->offset=update(p); /* 更新偏移量 */
			
			len=fmt?2:1;
			ptr=ensure(p,len);	if (!ptr) return 0;
			*ptr++=':';if (fmt) *ptr++='\t'; /* 添加冒号和可能的制表符 */
			p->offset+=len;
			
			print_value(child,depth,fmt,p); /* 打印值 */
			p->offset=update(p); /* 更新偏移量 */

			len=(fmt?1:0)+(child->next?1:0);
			ptr=ensure(p,len+1); if (!ptr) return 0;
			if (child->next) *ptr++=','; /* 添加逗号 */
			if (fmt) *ptr++='\n';*ptr=0; /* 添加换行符（如果格式化） */
			p->offset+=len;
			child=child->next;
		}
		ptr=ensure(p,fmt?(depth+1):2);	 if (!ptr) return 0;
		if (fmt)	for (i=0;i<depth-1;i++) *ptr++='\t'; /* 添加缩进 */
		*ptr++='}';*ptr=0; /* 结束对象 */
		out=(p->buffer)+i;
	}
	else
	{
		/* 为名称和对象分配空间 */
		entries=(char**)cJSON_malloc(numentries*sizeof(char*));
		if (!entries) return 0; /* 内存分配失败 */
		names=(char**)cJSON_malloc(numentries*sizeof(char*));
		if (!names) {cJSON_free(entries);return 0;} /* 内存分配失败 */
		memset(entries,0,sizeof(char*)*numentries); /* 初始化条目数组 */
		memset(names,0,sizeof(char*)*numentries); /* 初始化名称数组 */

		/* 收集所有结果到我们的数组： */
		child=item->child;depth++;if (fmt) len+=depth; /* 增加深度 */
		while (child && !fail)
		{
			names[i]=str=print_string_ptr(child->string,0); /* 打印键名 */
			entries[i++]=ret=print_value(child,depth,fmt,0); /* 打印值 */
			if (str && ret) len+=strlen(ret)+strlen(str)+2+(fmt?2+depth:0); else fail=1; /* 计算长度 */
			child=child->next;
		}
		
		/* 尝试分配输出字符串 */
		if (!fail)	out=(char*)cJSON_malloc(len); /* 分配内存 */
		if (!out) fail=1; /* 内存分配失败 */

		/* 处理失败 */
		if (fail)
		{
			for (i=0;i<numentries;i++) {if (names[i]) cJSON_free(names[i]);if (entries[i]) cJSON_free(entries[i]);} /* 释放内存 */
			cJSON_free(names);cJSON_free(entries); /* 释放数组内存 */
			return 0;
		}
		
		/* 组成输出： */
		*out='{';ptr=out+1;if (fmt)*ptr++='\n';*ptr=0; /* 开始对象 */
		for (i=0;i<numentries;i++)
		{
			if (fmt) for (j=0;j<depth;j++) *ptr++='\t'; /* 添加缩进 */
			tmplen=strlen(names[i]);memcpy(ptr,names[i],tmplen);ptr+=tmplen; /* 复制键名 */
			*ptr++=':';if (fmt) *ptr++='\t'; /* 添加冒号和可能的制表符 */
			strcpy(ptr,entries[i]);ptr+=strlen(entries[i]); /* 复制值 */
			if (i!=numentries-1) *ptr++=','; /* 添加逗号 */
			if (fmt) *ptr++='\n';*ptr=0; /* 添加换行符（如果格式化） */
			cJSON_free(names[i]);cJSON_free(entries[i]); /* 释放内存 */
		}
		
		cJSON_free(names);cJSON_free(entries); /* 释放数组内存 */
		if (fmt) for (i=0;i<depth-1;i++) *ptr++='\t'; /* 添加缩进 */
		*ptr++='}';*ptr++=0; /* 结束对象 */
	}
	return out;	
}

/* 获取数组大小/项或对象项。 */
int    cJSON_GetArraySize(cJSON *array)							{cJSON *c=array->child;int i=0;while(c)i++,c=c->next;return i;} // 获取数组大小
cJSON *cJSON_GetArrayItem(cJSON *array,int item)				{cJSON *c=array->child;  while (c && item>0) item--,c=c->next; return c;} // 获取数组中的项
cJSON *cJSON_GetObjectItem(cJSON *object,const char *string)	{cJSON *c=object->child; while (c && cJSON_strcasecmp(c->string,string)) c=c->next; return c;} // 获取对象中的项
int cJSON_HasObjectItem(cJSON *object,const char *string)	{
	cJSON *c=object->child;
	while (c )
	{
		if(cJSON_strcasecmp(c->string,string)==0){
			return 1; // 如果找到指定的项，则返回1
		}
	c=c->next;
	}
	return 0; // 如果没有找到指定的项，则返回0
}

/* 用于处理数组列表的实用程序。 */
static void suffix_object(cJSON *prev,cJSON *item) {prev->next=item;item->prev=prev;} // 将项添加到列表中
/* 用于处理引用的实用程序。 */
static cJSON *create_reference(cJSON *item) {cJSON *ref=cJSON_New_Item();if (!ref) return 0;memcpy(ref,item,sizeof(cJSON));ref->string=0;ref->type|=cJSON_IsReference;ref->next=ref->prev=0;return ref;} // 创建一个项的引用

/* 将项添加到数组/对象中。 */
void   cJSON_AddItemToArray(cJSON *array, cJSON *item)						{cJSON *c=array->child;if (!item) return; if (!c) {array->child=item;} else {while (c && c->next) c=c->next; suffix_object(c,item);}} // 将项添加到数组中
void   cJSON_AddItemToObject(cJSON *object,const char *string,cJSON *item)	{if (!item) return; if (item->string) cJSON_free(item->string);item->string=cJSON_strdup(string);cJSON_AddItemToArray(object,item);} // 将项添加到对象中
void   cJSON_AddItemToObjectCS(cJSON *object,const char *string,cJSON *item)	{if (!item) return; if (!(item->type&cJSON_StringIsConst) && item->string) cJSON_free(item->string);item->string=(char*)string;item->type|=cJSON_StringIsConst;cJSON_AddItemToArray(object,item);} // 将项以常量字符串形式添加到对象中
void	cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item)						{cJSON_AddItemToArray(array,create_reference(item));} // 将项的引用添加到数组中
void	cJSON_AddItemReferenceToObject(cJSON *object,const char *string,cJSON *item)	{cJSON_AddItemToObject(object,string,create_reference(item));} // 将项的引用添加到对象中

cJSON *cJSON_DetachItemFromArray(cJSON *array,int which)			{cJSON *c=array->child;while (c && which>0) c=c->next,which--;if (!c) return 0;
	if (c->prev) c->prev->next=c->next;if (c->next) c->next->prev=c->prev;if (c==array->child) array->child=c->next;c->prev=c->next=0;return c;} // 从数组中分离项
void   cJSON_DeleteItemFromArray(cJSON *array,int which)			{cJSON_Delete(cJSON_DetachItemFromArray(array,which));} // 从数组中删除项
cJSON *cJSON_DetachItemFromObject(cJSON *object,const char *string) {int i=0;cJSON *c=object->child;while (c && cJSON_strcasecmp(c->string,string)) i++,c=c->next;if (c) return cJSON_DetachItemFromArray(object,i);return 0;} // 从对象中分离项
void   cJSON_DeleteItemFromObject(cJSON *object,const char *string) {cJSON_Delete(cJSON_DetachItemFromObject(object,string));} // 从对象中删除项

/* 用新项替换数组/对象项。 */
void   cJSON_InsertItemInArray(cJSON *array,int which,cJSON *newitem)		{cJSON *c=array->child;while (c && which>0) c=c->next,which--;if (!c) {cJSON_AddItemToArray(array,newitem);return;}
	newitem->next=c;newitem->prev=c->prev;c->prev=newitem;if (c==array->child) array->child=newitem; else newitem->prev->next=newitem;} // 在数组中插入新项
void   cJSON_ReplaceItemInArray(cJSON *array,int which,cJSON *newitem)		{cJSON *c=array->child;while (c && which>0) c=c->next,which--;if (!c) return;
	newitem->next=c->next;newitem->prev=c->prev;if (newitem->next) newitem->next->prev=newitem;
	if (c==array->child) array->child=newitem; else newitem->prev->next=newitem;c->next=c->prev=0;cJSON_Delete(c);} // 替换数组中的项
void   cJSON_ReplaceItemInObject(cJSON *object,const char *string,cJSON *newitem){int i=0;cJSON *c=object->child;while(c && cJSON_strcasecmp(c->string,string))i++,c=c->next;if(c){newitem->string=cJSON_strdup(string);cJSON_ReplaceItemInArray(object,i,newitem);}} // 替换对象中的项

/* 创建基本类型： */
cJSON *cJSON_CreateNull(void)					{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_NULL;return item;} // 创建Null类型
cJSON *cJSON_CreateTrue(void)					{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_True;return item;} // 创建True类型
cJSON *cJSON_CreateFalse(void)					{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_False;return item;} // 创建False类型
cJSON *cJSON_CreateBool(int b)					{cJSON *item=cJSON_New_Item();if(item)item->type=b?cJSON_True:cJSON_False;return item;} // 创建Bool类型
cJSON *cJSON_CreateNumber(double num)			{cJSON *item=cJSON_New_Item();if(item){item->type=cJSON_Number;item->valuedouble=num;item->valueint=(int)num;}return item;} // 创建Number类型
cJSON *cJSON_CreateString(const char *string)	{cJSON *item=cJSON_New_Item();if(item){item->type=cJSON_String;item->valuestring=cJSON_strdup(string);}return item;} // 创建String类型
cJSON *cJSON_CreateArray(void)					{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_Array;return item;} // 创建Array类型
cJSON *cJSON_CreateObject(void)					{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_Object;return item;} // 创建Object类型

/* 创建数组： */
cJSON *cJSON_CreateIntArray(const int *numbers,int count)		{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;a && i<count;i++){n=cJSON_CreateNumber(numbers[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;} // 创建整数数组
cJSON *cJSON_CreateFloatArray(const float *numbers,int count)	{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;a && i<count;i++){n=cJSON_CreateNumber(numbers[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;} // 创建浮点数数组
cJSON *cJSON_CreateDoubleArray(const double *numbers,int count)	{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;a && i<count;i++){n=cJSON_CreateNumber(numbers[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;} // 创建双精度浮点数数组
cJSON *cJSON_CreateStringArray(const char **strings,int count)	{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;a && i<count;i++){n=cJSON_CreateString(strings[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;} // 创建字符串数组

/* 复制 */
cJSON *cJSON_Duplicate(cJSON *item,int recurse)
{
	cJSON *newitem,*cptr,*nptr=0,*newchild;
	/* 如果指针无效，则退出 */
	if (!item) return 0;
	/* 创建新项 */
	newitem=cJSON_New_Item();
	if (!newitem) return 0;
	/* 复制所有变量 */
	newitem->type=item->type&(~cJSON_IsReference),newitem->valueint=item->valueint,newitem->valuedouble=item->valuedouble;
	if (item->valuestring)	{newitem->valuestring=cJSON_strdup(item->valuestring);	if (!newitem->valuestring)	{cJSON_Delete(newitem);return 0;}}
	if (item->string)		{newitem->string=cJSON_strdup(item->string);			if (!newitem->string)		{cJSON_Delete(newitem);return 0;}}
	/* 如果不递归，则完成！ */
	if (!recurse) return newitem;
	/* 遍历子项的 ->next 链。 */
	cptr=item->child;
	while (cptr)
	{
		newchild=cJSON_Duplicate(cptr,1);		/* 复制 ->next 链中的每个项（带递归） */
		if (!newchild) {cJSON_Delete(newitem);return 0;}
		if (nptr)	{nptr->next=newchild,newchild->prev=nptr;nptr=newchild;}	/* 如果 newitem->child 已设置，则交叉连接 ->prev 和 ->next 并继续 */
		else		{newitem->child=newchild;nptr=newchild;}					/* 设置 newitem->child 并移动到它 */
		cptr=cptr->next;
	}
	return newitem;
}

void cJSON_Minify(char *json)
{
	char *into=json;
	while (*json)
	{
		if (*json==' ') json++;
		else if (*json=='\t') json++;	/* 空格字符。 */
		else if (*json=='\r') json++;
		else if (*json=='\n') json++;
		else if (*json=='/' && json[1]=='/')  while (*json && *json!='\n') json++;	/* 双斜杠注释，直到行尾。 */
		else if (*json=='/' && json[1]=='*') {while (*json && !(*json=='*' && json[1]=='/')) json++;json+=2;}	/* 多行注释。 */
		else if (*json=='\"'){*into++=*json++;while (*json && *json!='\"'){if (*json=='\\') *into++=*json++;*into++=*json++;}*into++=*json++;} /* 字符串文字，对 \" 敏感。 */
		else *into++=*json++;			/* 所有其他字符。 */
	}
	*into=0;	/* 并以空字符结尾。 */
}
