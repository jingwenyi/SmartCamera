#include <iconv.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

static int code_convert(char *from_charset, char *to_charset,
		char *inbuf, size_t inlen, char *outbuf, size_t outlen) 
{
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;

	cd = iconv_open(to_charset, from_charset);
	if (cd == (iconv_t)-1) {
		perror("iconv_open");
		return -1;
	}
	memset(outbuf, 0, outlen);
	if (iconv(cd, pin, &inlen, pout, &outlen) == (size_t)-1) {
		perror("iconv");
		return -1;
	}
	iconv_close(cd);
	*pout = '\0';

	return 0;
}

int u2g(char *inbuf, size_t inlen, char *outbuf, size_t outlen) 
{
	return code_convert("utf-8", "gbk", inbuf, inlen, outbuf, outlen);
}

int g2u(char *inbuf, size_t inlen, char *outbuf, size_t outlen)
{
	return code_convert("gbk", "utf-8", inbuf, inlen, outbuf, outlen);
}


