/*
 * Copyright (C) 2016 by Joseph A. Marrero. http://www.joemarrero.com/
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <curl/curl.h>
#include "namecom_api.h"
#include <jansson.h>
#include <utility.h>
#include <collections/vector.h>

#define NAMECOM_API_SERVER_DEV    "api.dev.name.com"
#define NAMECOM_API_SERVER_REL    "api.name.com"
#define NAMECOM_API_USERAGENT     "Name.com Dynamic DNS Client"

#define NAMECOM_API_RESPONSE_CODE_COMMAND_SUCCESSFUL         100
#define NAMECOM_API_RESPONSE_CODE_REQUIRED_PARAM_MISSING     203
#define NAMECOM_API_RESPONSE_CODE_PARAM_VALUE_ERROR          204
#define NAMECOM_API_RESPONSE_CODE_INVALID_COMMAND_URL        211
#define NAMECOM_API_RESPONSE_CODE_AUTHORIZATION_ERROR        221
#define NAMECOM_API_RESPONSE_CODE_COMMAND_FAILED             240
#define NAMECOM_API_RESPONSE_CODE_UNEXPECTED_ERROR           250
#define NAMECOM_API_RESPONSE_CODE_AUTHENTICATION_ERROR       251 // Invalid Username Or Api Token
#define NAMECOM_API_RESPONSE_CODE_INSUFFICIENT_FUNDS         260
#define NAMECOM_API_RESPONSE_CODE_UNABLE_TO_AUTHORIZE_FUNDS  261

struct namecom_api {
	char* username;
	char* api_token;
	char* api_server;
	char* session_token;
	bool verbose;
};


static const char* namecom_api_code_string( int code );

namecom_api_t* namecom_api_create( const char* username, const char* api_token, bool is_dev, bool verbose )
{
	namecom_api_t* api = malloc( sizeof(namecom_api_t) );

	if( api )
	{
		api->username      = string_dup( username );
		api->api_token     = string_dup( api_token );
		api->api_server    = is_dev ? NAMECOM_API_SERVER_DEV : NAMECOM_API_SERVER_REL;
		api->session_token = NULL;
		api->verbose       = verbose;
	}

	return api;
}


void namecom_api_destroy( namecom_api_t* api )
{
	if( api )
	{
		free( api->username );
		free( api->api_token );
		if( api->session_token ) free( api->session_token );

		free( api );
	}
}

const char* namecom_api_username( const namecom_api_t* api )
{
	return api->username;
}

const char* namecom_api_token( const namecom_api_t* api )
{
	return api->api_token;
}

const char* namecom_api_server( const namecom_api_t* api )
{
	return api->api_server;
}

const char* namecom_api_session_token( const namecom_api_t* api )
{
	return api->session_token;
}

static void namecom_api_set_authentication_headers( namecom_api_t* api, struct curl_slist* headers_list )
{
	if( api->session_token )
	{
		char header_api_session_token[ 512 ];
		snprintf( header_api_session_token, sizeof(header_api_session_token), "Api-Session-Token: %s", api->session_token );
		header_api_session_token[ sizeof(header_api_session_token) - 1 ] = '\0';

		headers_list = curl_slist_append( headers_list, header_api_session_token );
	}
	else
	{
		char header_api_username[ 64 ];
		snprintf( header_api_username, sizeof(header_api_username), "Api-Username: %s", api->username );
		header_api_username[ sizeof(header_api_username) - 1 ] = '\0';

		headers_list = curl_slist_append( headers_list, header_api_username );

		char header_api_token[ 512 ];
		snprintf( header_api_token, sizeof(header_api_token), "Api-Token: %s", api->api_token );
		header_api_token[ sizeof(header_api_token) - 1 ] = '\0';

		headers_list = curl_slist_append( headers_list, header_api_token );
	}
}

typedef struct response_body {
	size_t len;
	char* text;
} response_body_t;

static size_t namecom_api_writefunc( void *ptr, size_t size, size_t nmemb, void* userdata )
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

bool namecom_api_login( namecom_api_t* api )
{
	bool result = true;
	CURL* curl = curl_easy_init();

	if( curl )
	{
		char url[ 256 ];
		snprintf( url, sizeof(url), "https://%s/api/login", api->api_server );

		curl_easy_setopt( curl, CURLOPT_URL, url );

		char header_content_type[ 256 ];
		snprintf( header_content_type, sizeof(header_content_type), "Content-Type: application/json" );

		char header_user_agent[ 256 ];
		snprintf( header_user_agent, sizeof(header_user_agent), "User-Agent: %s v%s", NAMECOM_API_USERAGENT, NAMECOM_API_VERSION );


		struct curl_slist* headers_list = NULL;
		headers_list = curl_slist_append( headers_list, header_content_type );
		headers_list = curl_slist_append( headers_list, header_user_agent );
		namecom_api_set_authentication_headers( api, headers_list );

		curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers_list );

		char post_body[ 512 ];
		snprintf( post_body, sizeof(post_body), "{\"username\": \"%s\", \"api_token\": \"%s\"}", api->username, api->api_token );

		//printf( "DEBUG: %s\n", post_body );

		curl_easy_setopt( curl, CURLOPT_POSTFIELDS, post_body );


		response_body_t response_body = { .text = NULL, .len = 0 };

		curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, namecom_api_writefunc );
		curl_easy_setopt( curl, CURLOPT_WRITEDATA, &response_body );

		curl_easy_setopt( curl, CURLOPT_VERBOSE, NAMECOM_API_VERBOSE );


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

		/* Perform the request, res will get the return code */
		CURLcode res = curl_easy_perform( curl );

		int status_code = 0;
		curl_easy_getinfo( curl, CURLINFO_HTTP_CODE, &status_code );
		//printf( "DEBUG: %d\n", status_code );

		if( res == CURLE_OK )
		{
		    //printf( "DEBUG: %s\n", response_body.text );

			json_error_t error;
			json_t* root = json_loads( response_body.text, 0, &error );

			if( root )
			{
				json_t* session_token_obj = json_object_get( root, "session_token" );

				if( json_is_string(session_token_obj) )
				{
					api->session_token = string_dup( json_string_value(session_token_obj) );
				}
				else
				{
					if( api->verbose )
					{
						json_t* result_obj = json_object_get( root, "result" );

						if( json_is_object(result_obj) )
						{
							json_t* code_obj = json_object_get( result_obj, "code" );

							if( json_is_integer(code_obj) )
							{
								json_int_t code = json_integer_value( code_obj );
								fprintf( stderr, "[ERROR] %s\n", namecom_api_code_string( code ) );
							}
						}
					}
					result = false;
				}
			}
			else
			{
				result = false;
			}

			json_decref( root );
		}
		else
		{
			fprintf(stderr, "[ERROR] %s\n", curl_easy_strerror(res));

			result = false;
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}

	return result;
}

bool namecom_api_logout( namecom_api_t* api )
{
	bool result = true;
	CURL* curl = curl_easy_init();

	if( curl )
	{
		char url[ 256 ];
		snprintf( url, sizeof(url), "https://%s/api/logout", api->api_server );

		curl_easy_setopt( curl, CURLOPT_URL, url );

		char header_content_type[ 256 ];
		snprintf( header_content_type, sizeof(header_content_type), "Content-Type: application/json" );

		char header_user_agent[ 256 ];
		snprintf( header_user_agent, sizeof(header_user_agent), "User-Agent: %s v%s", NAMECOM_API_USERAGENT, NAMECOM_API_VERSION );


		struct curl_slist* headers_list = NULL;
		headers_list = curl_slist_append( headers_list, header_content_type );
		headers_list = curl_slist_append( headers_list, header_user_agent );
		namecom_api_set_authentication_headers( api, headers_list );

		curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers_list );

		response_body_t response_body = { .text = NULL, .len = 0 };

		curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, namecom_api_writefunc );
		curl_easy_setopt( curl, CURLOPT_WRITEDATA, &response_body );

		curl_easy_setopt( curl, CURLOPT_VERBOSE, NAMECOM_API_VERBOSE );

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
			//printf( "DEBUG: %s\n", response_body.text );

			json_error_t error;
			json_t* root = json_loads( response_body.text, 0, &error );

			if( root )
			{
				json_t* result_obj = json_object_get( root, "result" );

				if( json_is_object(result_obj) )
				{
					json_t* code_obj = json_object_get( result_obj, "code" );

					if( json_is_integer(code_obj) )
					{
						json_int_t code = json_integer_value( code_obj );
						result = code == NAMECOM_API_RESPONSE_CODE_COMMAND_SUCCESSFUL;

						if( !result && api->verbose )
						{
							fprintf( stderr, "[ERROR] %s\n", namecom_api_code_string(code) );
						}
					}
					else
					{
						result = false;
					}
				}
				else
				{
					result = false;
				}
			}
			else
			{
				result = false;
			}

			json_decref( root );
		}
		else
		{
			fprintf(stderr, "[ERROR] %s\n", curl_easy_strerror(res));

			result = false;
		}

		/* always cleanup */
		curl_easy_cleanup( curl );
	}

	return result;
}

bool namecom_api_hello( namecom_api_t* api )
{
	bool result = true;
	CURL* curl = curl_easy_init();

	if( curl )
	{
		char url[ 256 ];
		snprintf( url, sizeof(url), "https://%s/api/hello", api->api_server );

		curl_easy_setopt( curl, CURLOPT_URL, url );

		char header_content_type[ 256 ];
		snprintf( header_content_type, sizeof(header_content_type), "Content-Type: application/json" );

		char header_user_agent[ 256 ];
		snprintf( header_user_agent, sizeof(header_user_agent), "User-Agent: %s v%s", NAMECOM_API_USERAGENT, NAMECOM_API_VERSION );

		struct curl_slist* headers_list = NULL;
		headers_list = curl_slist_append( headers_list, header_content_type );
		headers_list = curl_slist_append( headers_list, header_user_agent );
		namecom_api_set_authentication_headers( api, headers_list );

		curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers_list );

		response_body_t response_body = { .text = NULL, .len = 0 };

		curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, namecom_api_writefunc );
		curl_easy_setopt( curl, CURLOPT_WRITEDATA, &response_body );

		curl_easy_setopt( curl, CURLOPT_VERBOSE, NAMECOM_API_VERBOSE );


		#ifdef SKIP_PEER_VERIFICATION
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
		#endif

		#ifdef SKIP_HOSTNAME_VERIFICATION
		/*
		 * If the site you're connecting to uses a different host name that what
		 * they have mentioned in their server certificate's commonName (or
		 * subjectAltName) fields, libcurl will refuse to connect. You can skip
		 * this check, but this will make the connection less secure.
		 */
		curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L );
		#endif

		CURLcode res = curl_easy_perform( curl );

		if( res == CURLE_OK )
		{
			//printf( "DEBUG: %s\n", response_body.text );

			json_error_t error;
			json_t* root = json_loads( response_body.text, 0, &error );

			if( root )
			{
				json_t* result_obj = json_object_get( root, "result" );

				if( json_is_object(result_obj) )
				{
					json_t* code_obj = json_object_get( result_obj, "code" );

					if( json_is_integer(code_obj) )
					{
						json_int_t code = json_integer_value( code_obj );
						result = code == NAMECOM_API_RESPONSE_CODE_COMMAND_SUCCESSFUL;

						if( !result && api->verbose )
						{
							fprintf( stderr, "[ERROR] %s\n", namecom_api_code_string(code) );
						}
					}
					else
					{
						result = false;
					}
				}
				else
				{
					result = false;
				}
			}
			else
			{
				result = false;
			}

			json_decref( root );
		}
		else
		{
			fprintf(stderr, "[ERROR] %s\n", curl_easy_strerror(res));

			result = false;
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}

	return result;
}

bool namecom_api_domains_list( namecom_api_t* api )
{
	bool result = true;
	CURL* curl = curl_easy_init();

	if( curl )
	{
		char url[ 256 ];
		snprintf( url, sizeof(url), "https://%s/api/domain/list", api->api_server );

		curl_easy_setopt( curl, CURLOPT_URL, url );


		char header_content_type[ 256 ];
		snprintf( header_content_type, sizeof(header_content_type), "Content-Type: application/json" );

		char header_user_agent[ 256 ];
		snprintf( header_user_agent, sizeof(header_user_agent), "User-Agent: %s v%s", NAMECOM_API_USERAGENT, NAMECOM_API_VERSION );


		struct curl_slist* headers_list = NULL;
		headers_list = curl_slist_append( headers_list, header_content_type );
		headers_list = curl_slist_append( headers_list, header_user_agent );
		namecom_api_set_authentication_headers( api, headers_list );

		curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers_list );

		response_body_t response_body = { .text = NULL, .len = 0 };

		curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, namecom_api_writefunc );
		curl_easy_setopt( curl, CURLOPT_WRITEDATA, &response_body );

		curl_easy_setopt( curl, CURLOPT_VERBOSE, NAMECOM_API_VERBOSE );


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
			//printf( "DEBUG: %s\n", response_body.text );

			json_error_t error;
			json_t* root = json_loads( response_body.text, 0, &error );

			if( root )
			{
				json_t* result_obj = json_object_get( root, "result" );

				if( json_is_object(result_obj) )
				{
					json_t* code_obj = json_object_get( result_obj, "code" );

					if( json_is_integer(code_obj) )
					{
						json_int_t code = json_integer_value( code_obj );
						result = code == NAMECOM_API_RESPONSE_CODE_COMMAND_SUCCESSFUL;

						if( !result && api->verbose )
						{
							fprintf( stderr, "[ERROR] %s\n", namecom_api_code_string(code) );
						}
					}
					else
					{
						result = false;
					}
				}
				else
				{
					result = false;
				}
			}
			else
			{
				result = false;
			}

			json_decref( root );
		}
		else
		{
			fprintf(stderr, "[ERROR] %s\n", curl_easy_strerror(res));

			result = false;
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}

	return result;
}

namecom_api_dns_record_t* namecom_api_dns_record_create( long id, const char* fqdn, const char* type, const char* content, int ttl, const char* create_date )
{
	namecom_api_dns_record_t* r = malloc( sizeof(namecom_api_dns_record_t) );

	if( r )
	{
		r->id          = id;
		r->fqdn        = string_dup(fqdn);
		r->type        = string_dup(type);
		r->content     = string_dup(content);
		r->ttl         = ttl;
		r->create_date = string_dup(create_date);
	}

	return r;
}

void namecom_api_dns_record_destroy( namecom_api_dns_record_t* r )
{
	if( r )
	{
		free( (char*) r->fqdn );
		free( (char*) r->type );
		free( (char*) r->content );
		free( (char*) r->create_date );

		free( r );
	}
}

namecom_api_dns_record_t** namecom_api_dns_record_list( namecom_api_t* api, const char* domain )
{
	namecom_api_dns_record_t** records = NULL;
	CURL* curl = curl_easy_init();

	if( curl )
	{
		char url[ 256 ];
		snprintf( url, sizeof(url), "https://%s/api/dns/list/%s", api->api_server, domain );

		curl_easy_setopt( curl, CURLOPT_URL, url );


		char header_content_type[ 256 ];
		snprintf( header_content_type, sizeof(header_content_type), "Content-Type: application/json" );

		char header_user_agent[ 256 ];
		snprintf( header_user_agent, sizeof(header_user_agent), "User-Agent: %s v%s", NAMECOM_API_USERAGENT, NAMECOM_API_VERSION );


		struct curl_slist* headers_list = NULL;
		headers_list = curl_slist_append( headers_list, header_content_type );
		headers_list = curl_slist_append( headers_list, header_user_agent );
		namecom_api_set_authentication_headers( api, headers_list );

		curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers_list );

		response_body_t response_body = { .text = NULL, .len = 0 };

		curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, namecom_api_writefunc );
		curl_easy_setopt( curl, CURLOPT_WRITEDATA, &response_body );

		curl_easy_setopt( curl, CURLOPT_VERBOSE, NAMECOM_API_VERBOSE );


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
			//printf( "DEBUG: %s\n", response_body.text );

			json_error_t error;
			json_t* root = json_loads( response_body.text, 0, &error );

			if( root )
			{
				json_t* result_obj = json_object_get( root, "result" );

				if( json_is_object(result_obj) )
				{
					json_t* code_obj = json_object_get( result_obj, "code" );

					if( json_is_integer(code_obj) )
					{
						json_int_t code = json_integer_value( code_obj );

						if( code == NAMECOM_API_RESPONSE_CODE_COMMAND_SUCCESSFUL )
						{
							json_t* records_obj = json_object_get( root, "records" );

							if( json_is_array(records_obj) )
							{
								lc_vector_create( records, 5 );

								for( size_t i = 0; i < json_array_size(records_obj); i++ )
								{
									json_t* record_obj = json_array_get( records_obj, i );

									if( json_is_object(record_obj) )
									{
										json_t* record_id_obj   = json_object_get( record_obj, "record_id" );
										json_t* name_obj        = json_object_get( record_obj, "name" );
										json_t* type_obj        = json_object_get( record_obj, "type" );
										json_t* content_obj     = json_object_get( record_obj, "content" );
										json_t* ttl_obj         = json_object_get( record_obj, "ttl" );
										json_t* create_date_obj = json_object_get( record_obj, "create_date" );


										namecom_api_dns_record_t* r = namecom_api_dns_record_create(
											atol(json_string_value(record_id_obj)),
											json_string_value(name_obj),
											json_string_value(type_obj),
											json_string_value(content_obj),
											atoi(json_string_value(ttl_obj)),
											json_string_value(create_date_obj)
										);

										lc_vector_push( records, r );
									}
								}
							}
						}
						else if( api->verbose )
						{
							fprintf( stderr, "[ERROR] %s\n", namecom_api_code_string(code) );
						}
					}
				}
			}

			json_decref( root );
		}
		else
		{
			fprintf(stderr, "[ERROR] %s\n", curl_easy_strerror(res));
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}

	return records;
}

bool namecom_api_dns_record_add( namecom_api_t* api, const char* domain, const char* hostname, const char* type, const char* content, int ttl, int priority, long* id )
{
	bool result = true;
	CURL* curl = curl_easy_init();

	if( curl )
	{
		char url[ 256 ];
		snprintf( url, sizeof(url), "https://%s/api/dns/create/%s", api->api_server, domain );

		curl_easy_setopt( curl, CURLOPT_URL, url );


		char header_content_type[ 256 ];
		snprintf( header_content_type, sizeof(header_content_type), "Content-Type: application/json" );

		char header_user_agent[ 256 ];
		snprintf( header_user_agent, sizeof(header_user_agent), "User-Agent: %s v%s", NAMECOM_API_USERAGENT, NAMECOM_API_VERSION );


		struct curl_slist* headers_list = NULL;
		headers_list = curl_slist_append( headers_list, header_content_type );
		headers_list = curl_slist_append( headers_list, header_user_agent );
		namecom_api_set_authentication_headers( api, headers_list );

		curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers_list );

		char post_body[ 1024 ];
		snprintf( post_body, sizeof(post_body), "{\"hostname\": \"%s\", \"type\": \"%s\", \"content\": \"%s\", \"ttl\": %d, \"priority\": %d}", hostname, type, content, ttl, priority );

		//printf( "DEBUG: %s\n", post_body );

		curl_easy_setopt( curl, CURLOPT_POSTFIELDS, post_body );

		response_body_t response_body = { .text = NULL, .len = 0 };

		curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, namecom_api_writefunc );
		curl_easy_setopt( curl, CURLOPT_WRITEDATA, &response_body );

		curl_easy_setopt( curl, CURLOPT_VERBOSE, NAMECOM_API_VERBOSE );


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
			//printf( "DEBUG: %s\n", response_body.text );

			json_error_t error;
			json_t* root = json_loads( response_body.text, 0, &error );

			if( root )
			{
				json_t* result_obj = json_object_get( root, "result" );

				if( json_is_object(result_obj) )
				{
					json_t* code_obj = json_object_get( result_obj, "code" );

					if( json_is_integer(code_obj) )
					{
						json_int_t code = json_integer_value( code_obj );
						result = code == NAMECOM_API_RESPONSE_CODE_COMMAND_SUCCESSFUL;

						if( result && id )
						{
							json_t* record_id_obj = json_object_get( root, "record_id" );

							if( json_is_integer(record_id_obj) )
							{
								*id = json_integer_value(record_id_obj);
							}

						}
						else if( api->verbose )
						{
							fprintf( stderr, "[ERROR] %s\n", namecom_api_code_string(code) );
						}
					}
					else
					{
						result = false;
					}
				}
				else
				{
					result = false;
				}
			}
			else
			{
				result = false;
			}

			json_decref( root );
		}
		else
		{
			fprintf(stderr, "[ERROR] %s\n", curl_easy_strerror(res));

			result = false;
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}

	return result;
}


bool namecom_api_dns_record_remove( namecom_api_t* api, const char* domain, long id )
{
	bool result = true;
	CURL* curl = curl_easy_init();

	if( curl )
	{
		char url[ 256 ];
		snprintf( url, sizeof(url), "https://%s/api/dns/delete/%s", api->api_server, domain );

		curl_easy_setopt( curl, CURLOPT_URL, url );


		char header_content_type[ 256 ];
		snprintf( header_content_type, sizeof(header_content_type), "Content-Type: application/json" );

		char header_user_agent[ 256 ];
		snprintf( header_user_agent, sizeof(header_user_agent), "User-Agent: %s v%s", NAMECOM_API_USERAGENT, NAMECOM_API_VERSION );


		struct curl_slist* headers_list = NULL;
		headers_list = curl_slist_append( headers_list, header_content_type );
		headers_list = curl_slist_append( headers_list, header_user_agent );
		namecom_api_set_authentication_headers( api, headers_list );

		curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers_list );

		char post_body[ 1024 ];
		snprintf( post_body, sizeof(post_body), "{ \"record_id\": %ld}", id );

		//printf( "DEBUG: %s\n", post_body );

		curl_easy_setopt( curl, CURLOPT_POSTFIELDS, post_body );

		response_body_t response_body = { .text = NULL, .len = 0 };

		curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, namecom_api_writefunc );
		curl_easy_setopt( curl, CURLOPT_WRITEDATA, &response_body );

		curl_easy_setopt( curl, CURLOPT_VERBOSE, NAMECOM_API_VERBOSE );


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
			//printf( "DEBUG: %s\n", response_body.text );

			json_error_t error;
			json_t* root = json_loads( response_body.text, 0, &error );

			if( root )
			{
				json_t* result_obj = json_object_get( root, "result" );

				if( json_is_object(result_obj) )
				{
					json_t* code_obj = json_object_get( result_obj, "code" );

					if( json_is_integer(code_obj) )
					{
						json_int_t code = json_integer_value( code_obj );
						result = code == NAMECOM_API_RESPONSE_CODE_COMMAND_SUCCESSFUL;

						if( !result && api->verbose )
						{
							fprintf( stderr, "[ERROR] %s\n", namecom_api_code_string(code) );
						}
					}
					else
					{
						result = false;
					}
				}
				else
				{
					result = false;
				}
			}
			else
			{
				result = false;
			}

			json_decref( root );
		}
		else
		{
			fprintf(stderr, "[ERROR] %s\n", curl_easy_strerror(res));

			result = false;
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}

	return result;
}

const char* namecom_api_code_string( int code )
{
	const char* message = "Unknown";

	switch( code )
	{
		case NAMECOM_API_RESPONSE_CODE_COMMAND_SUCCESSFUL:
			message = "Successful";
			break;
		case NAMECOM_API_RESPONSE_CODE_REQUIRED_PARAM_MISSING:
			message = "Required parameter is missing";
			break;
		case NAMECOM_API_RESPONSE_CODE_PARAM_VALUE_ERROR:
			message = "Parameter value error";
			break;
		case NAMECOM_API_RESPONSE_CODE_INVALID_COMMAND_URL:
			message = "Invalid command URL";
			break;
		case NAMECOM_API_RESPONSE_CODE_AUTHORIZATION_ERROR:
			message = "Authorization error";
			break;
		case NAMECOM_API_RESPONSE_CODE_COMMAND_FAILED:
			message = "Command failed";
			break;
		case NAMECOM_API_RESPONSE_CODE_UNEXPECTED_ERROR:
			message = "Unexpected error";
			break;
		case NAMECOM_API_RESPONSE_CODE_AUTHENTICATION_ERROR:
			message = "Authentication error";
			break;
		case NAMECOM_API_RESPONSE_CODE_INSUFFICIENT_FUNDS:
			message = "Insufficient funds";
			break;
		case NAMECOM_API_RESPONSE_CODE_UNABLE_TO_AUTHORIZE_FUNDS:
			message = "Unable to authorize funds";
			break;
		default:
			break;
	}

	return message;
}
