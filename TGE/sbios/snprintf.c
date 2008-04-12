/* $Id$ */

/* Copyright 1997, Brian J. Swetland <swetland@neog.com>                 
** Free for non-commercial use.  Share and enjoy                         
**
** Minimal snprintf() function.
** %s - string     %d - signed int    %x - 32bit hex number (0 padded)
** %c - character  %u - unsigned int  %X -  8bit hex number (0 padded) 
*/

#include <stdarg.h>
#include "stdio.h"

static char hexmap[] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' 
};

int vsnprintf(char *b, int len, const char *fmt, va_list pvar) 
{
    int n,i;
    unsigned u;    
    char *t,d[10];
    int l = len;

    if(!fmt || !b || (l < 1)) return 0; 
    
    while(l && *fmt) {
        if(*fmt == '%'){
            fmt++;
            if(!(--l)) break;
            
            switch(*fmt){
            case 's': /* string */
                t = va_arg(pvar,char *);
                while(l && *t) *b++ = *t++, l--;                
                break;
                
            case 'c': /* single character */
                fmt++;                
                *b++ = va_arg(pvar,int); // XXX: Type "char" is correct but will cause an bounds exception.
                l--;                
                break;
                
            case 'x': /* 8 digit, unsigned 32bit hex integer */
                if(l < 8) { l = 0; break; }
                u = va_arg(pvar,unsigned int);
                for(i=7;i>=0;i--){
                    b[i] = hexmap[u & 0x0F];
                    u >>= 4;
                }
                b+=8;
                l-=8;                
                break;

            case 'd': /* signed integer */
                n = va_arg(pvar,int);
                if(n < 0) {
                    u = -n;
                    *b++ = '-';
                    if(!(--l)) break;                    
                } else {
                    u = n;
                }
                goto u2;                

            case 'u': /* unsigned integer */
                u = va_arg(pvar,unsigned int);               
              u2:
                i = 9;
                do {
                    d[i] = (u % 10) + '0';
                    u /= 10;
                    i--;
                } while(u && i >= 0);
                while(++i < 10){
                    *b++ = d[i];
                    if(!(--l)) break;                    
                }
                break;                                    
                
            case 'X': /* 2 digit, unsigned 8bit hex int */
                if(l < 2) { l = 0; break; }
                n = va_arg(pvar,int);
                *b++ = hexmap[(n & 0xF0) >> 4];
                *b++ = hexmap[n & 0x0F];
                l-=2;                
                break;
            default:
                *b++ = *fmt;
				break;
            }
        } else {
            *b++ = *fmt;
            l--;            
        }
        fmt++;            
    }
    *b = 0;
    return len - l;
}

int snprintf(char *str, int len, const char *fmt, ...)
{
    int ret;
    va_list pvar;    
    va_start(pvar,fmt);
    ret = vsnprintf(str,len,fmt,pvar);
    va_end(pvar);    
    return ret;
}
