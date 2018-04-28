
#ifndef BASE64_H  
#define BASE64_H 

/* Base64 ±àÂë */   
char* base64_encode(const unsigned char* data, int data_len);   

/* Base64 ½âÂë */   
unsigned char *base64_decode(const char* data, int data_len , int *return_len); 


#endif