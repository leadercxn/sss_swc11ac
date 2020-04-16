#include "sha1.h"


/*?????*/
#define K0 0x5a827999
#define K1 0x6ed9eba1
#define K2 0x8f1bbcdc
#define K3 0xca62c1d6

/*datas????bits?*/
#define ROL( datas , bits ) (( datas ) << ( bits ) | ( datas ) >> ( 32 - ( bits )))

/*????????*/
#define F0( x , y , z ) ((( x ) & ( y )) | ((~( x )) & ( z )))
#define F1( x , y , z ) (( x ) ^ ( y ) ^ ( z ))
#define F2( x , y , z ) ((( x ) & ( y )) | (( x ) & ( z )) | (( y ) & ( z )))
#define F3( x , y , z ) (( x ) ^ ( y ) ^ ( z ))

/*????????????????????*/
#define LOAD32B( x , y )                                             \
	{  x =  ((unsigned long) (( y )[0] & 255 ) << 24 ) | \
			((unsigned long) (( y )[1] & 255 ) << 16 ) |                    \
			((unsigned long) (( y )[2] & 255 ) << 8 ) |                      \
			((unsigned long) (( y )[3]) & 255 ) ;                          \
	}

/*???????????*/
#define STORE32B( x , y )                                                 \
	{     ( y )[0] = ( unsigned char )((( x ) >> 24 ) & 255 ) ;\
		 ( y )[1] = ( unsigned char )((( x ) >> 16 ) & 255 ) ;\
		 ( y )[2] = ( unsigned char )((( x ) >> 8 ) & 255 ) ;\
		 ( y )[3] = ( unsigned char )(( x ) & 255 ) ;\
	}

/*sha1??*/
typedef struct  
{
	unsigned char buf[64];
	unsigned long buflen , msglen;
	unsigned long s[5];
}sha1_state;

/*???sha1???*/
void sha1_init( sha1_state *md )
{
	md->s[0] = 0x67452301;
	md->s[1] = 0xefcdab89;
	md->s[2] = 0x98badcfe;
	md->s[3] = 0x10325476;
	md->s[4] = 0xc3d2e1f0;
	md->buflen = md->msglen = 0;
}

/*????????????????*/
static void sha1_compress( sha1_state *md )
{
	unsigned long w[80] , a , b , c , d , e , temp ;
	unsigned x ;

	/*??80???*/
	for( x = 0 ; x < 16 ; x++ )
	{
		 LOAD32B( w[x] , md->buf + 4 * x ) ;
	}
	for( x = 16 ; x < 80 ; x++ )
	{
		w[x] = ROL( w[x-3] ^ w[x-8] ^ w[x-14] ^ w[x-16] , 1 ) ;
	}

	/*????*/
	a = md->s[0] ;
	b = md->s[1] ;
	c = md->s[2] ;
	d = md->s[3] ;
	e = md->s[4] ;

	/*???1?20???*/
	for( x = 0 ; x < 20 ; x++ )
	{
		temp = ROL( a , 5 ) + F0( b , c , d ) + e + w[x] + K0;
		e = d ;
		d = c;
		c = ROL( b , 30 );
		b = a;
		a = temp;
	}

	/*???2?20???*/
	for( ; x < 40 ; x++ )
	{
		temp = ROL( a , 5 ) + F1( b , c , d ) + e + w[x] + K1;
		e = d ;
		d = c;
		c = ROL( b , 30 );
		b = a;
		a = temp;
	}

    /*???3?20???*/
	for( ; x < 60 ; x++ )
	{
		temp = ROL( a , 5 ) + F2( b , c , d ) + e + w[x] + K2;
		e = d ;
		d = c;
		c = ROL( b , 30 );
		b = a;
		a = temp;
	}

	/*???4?20???*/
	for( ; x < 80 ; x++ )
	{
		temp = ROL( a , 5 ) + F3( b , c , d ) + e + w[x] + K3;
		e = d ;
		d = c;
		c = ROL( b , 30 );
		b = a;
		a = temp;
	}

	/*????*/
	md->s[0] += a ;
	md->s[1] += b ;
	md->s[2] += c ;
	md->s[3] += d ;
	md->s[4] += e ;
}

/*??????????????,??64????????*/
void sha1_copy( sha1_state *md , const unsigned char *buf ,unsigned long len )
{
	unsigned long x , y ;
	while( len )
	{
		x = ( 64 - md->buflen ) < len ? 64 - md->buflen : len ;
		len -= x ;

		for( y = 0 ; y < x ; y++ )
		{
			md->buf[md->buflen++] = *buf++ ;
		}

		if( md->buflen == 64 )
		{
			sha1_compress( md ) ;
			md->buflen = 0 ;
			md->msglen += 64 ;
		}
	}
}

void sha1_finish( sha1_state *md , unsigned char *dst )
{
	unsigned long l1 , l2 ,i ;

	/*??????,??:bits,????????????????,??????8???*/
	md->msglen += md->buflen ;
	l2 = md->msglen >> 29 ;
	l1 = ( md->msglen << 3 ) & 0xFFFFFFFF ;

	/*???????????1000 0000*/
	md->buf[ md->buflen++ ] = 0x80 ;

	/*???????????????*/
	if( md->buflen > 56 )
	{
		while( md->buflen <64 )
		{
			md->buf[md->buflen++] = 0x00 ;
		}
		sha1_compress( md ) ;
		md->buflen = 0 ;
	}

	/*??????????????????????*/
	/*???????0,???????*/
	while( md->buflen < 56 )
	{
		md->buf[md->buflen++] = 0x00 ;
	}

	/*??????*/
	STORE32B( l2 , md->buf + 56 ) ;
	STORE32B( l1 , md->buf + 60 ) ;

	/*???????????????*/
	sha1_compress( md ) ;

	/*??????,???dst???*/
	for( i = 0 ; i < 5 ; i++ )
	{
		STORE32B( md->s[i] , dst + i * 4 ) ;
	}
}

/*????????????????,????*/
void sha1_memory( const unsigned char *in , unsigned long len , unsigned char *dst )
{
	sha1_state md ;
	sha1_init( &md ) ;
	sha1_copy( &md , in , len ) ;
	sha1_finish( &md , dst ) ;
}


