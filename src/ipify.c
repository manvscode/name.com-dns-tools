#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

typedef struct response_body {
	size_t len;
	char* text;
} response_body_t;

static size_t ipify_writefunc( void *ptr, size_t size, size_t nmemb, void* userdata )
{
	response_body_t* res_body = userdata;
	size_t new_len = res_body->len + size * nmemb;

	res_body->text = realloc( res_body->text, new_len + 1 );

	if( res_body->text == NULL )
	{
		fprintf(stderr, "[ERROR] Out of memory!\n");
		exit(EXIT_FAILURE);
	}

	memcpy( res_body->text + res_body->len, ptr, size * nmemb );
	res_body->text[ new_len ] = '\0';
	res_body->len = new_len;

	return size * nmemb;
}

char* ipify_public_ip( void )
{
	char* result = NULL;
	CURL* curl = curl_easy_init();

	if( curl )
	{
		curl_easy_setopt( curl, CURLOPT_URL, "http://api.ipify.org?format=text" );

		//char header_user_agent[ 256 ];
		//snprintf( header_user_agent, sizeof(header_user_agent), "User-Agent: %s v%s", NAMECOM_API_USERAGENT, NAMECOM_API_VERSION );


		struct curl_slist* headers_list = NULL;
		//headers_list = curl_slist_append( headers_list, header_user_agent );

		curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers_list );

		response_body_t response_body = { .text = NULL, .len = 0 };

		curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, ipify_writefunc );
		curl_easy_setopt( curl, CURLOPT_WRITEDATA, &response_body );

		/*
		 * If you want to connect to a site who isn't using a certificate that is
		 * signed by one of the certs in the CA bundle you have, you can skip the
		 * verification of the server's certificate. This makes the connection
		 * A LOT LESS SECURE.
		 *
		 * If you have a CA cert for the server stored someplace else than in the
		 * default bundle, then the CURLOPT_CAPATH option might come handy for
		 * you.
		 */
		curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 0L );

		/*
		 * If the site you're connecting to uses a different host name that what
		 * they have mentioned in their server certificate's commonName (or
		 * subjectAltName) fields, libcurl will refuse to connect. You can skip
		 * this check, but this will make the connection less secure.
		 */
		curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L );

		CURLcode res = curl_easy_perform( curl );

		if( res == CURLE_OK )
		{
			result = response_body.text;
		}
		else
		{
			fprintf(stderr, "[ERROR] %s\n", curl_easy_strerror(res));
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}

	return result;
}
