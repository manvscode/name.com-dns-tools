/*
 * Copyright (C) 2016 by Joseph A. Marrero. http://joemarrero.com/
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
#include <unistd.h>
#include <curl/curl.h>
#include <xtd/string.h>
#include <xtd/console.h>
#include <collections/vector.h>
#include "namecom_api.h"

#define VERSION  "1.0"

static void banner( void );
static void about( int argc, char* argv[] );
static bool separate_fqdn( const char* fqdn, char** host, char** domain );

typedef enum {
	COMMAND_NOT_SET = 0,
	COMMAND_LIST,
	COMMAND_SET,
	COMMAND_DELETE,
} command_t;

typedef struct {
	char* host;
	char* domain;
	char* type;
	char* answer;
	int ttl;
	command_t command;
	const char* username;
	const char* token;
	bool verbose;
} app_args_t;

int main( int argc, char* argv[] )
{
	int result = 0;

	app_args_t args = {
		.host       = NULL,
		.domain     = NULL,
		.type       = NULL,
		.answer     = NULL,
		.ttl        = 300,
		.command    = COMMAND_NOT_SET,
		.username   = getenv( "NAMECOM_USERNAME" ),
		.token      = getenv( "NAMECOM_API_TOKEN" ),
		.verbose    = false
	};


	if( argc > 0 )
	{
		for( int arg = 1; arg < argc; )
		{
			if( strcmp( "-l", argv[arg] ) == 0 || strcmp( "--list", argv[arg] ) == 0 )
			{
				if( (arg + 1) < argc )
				{
					args.domain = string_dup( argv[arg + 1] );

					args.command = COMMAND_LIST;
					//printf("HERE %s.%s %s\n", args.host, args.domain, "");
					arg += 2;
				}
				else
				{
					fprintf( stderr, "[ERROR] Missing required parameter for list operation.\n" );
					result = -1;
					goto done;
				}
			}
			else if( strcmp( "-s", argv[arg] ) == 0 || strcmp( "--set", argv[arg] ) == 0 )
			{
				int last_arg = arg + 5;

				if( last_arg <= argc )
				{
					const char* fqdn = argv[arg + 2];

					if( fqdn && !separate_fqdn(fqdn, &args.host, &args.domain) )
					{
						fprintf( stderr, "[ERROR] Delete parameter is not a fully qualified domain name.\n" );
						result = -1;
						goto done;
					}

					if( !args.host || *args.host == '\0' )
					{
						fprintf( stderr, "[ERROR] The hostname was not set.\n" );
						about( argc, argv );
						result = -2;
						goto done;
					}

					args.type = string_dup( argv[arg + 1] );

					if( !args.type || *args.type == '\0' )
					{
						fprintf( stderr, "[ERROR] The record type was not set.\n" );
						about( argc, argv );
						result = -2;
						goto done;
					}

					if( strcmp(args.type, "A") != 0 &&
					    strcmp(args.type, "CNAME") != 0 &&
					    strcmp(args.type, "MX") != 0 &&
					    strcmp(args.type, "NS") != 0 &&
					    strcmp(args.type, "TXT") != 0)
					{
						fprintf( stderr, "[ERROR] The record type must be either a A, CNAME, MX, NS, or TXT.\n" );
						result = -1;
						goto done;
					}

					args.answer = string_dup( argv[arg + 3] );

					if( !args.answer || *args.answer == '\0' )
					{
						fprintf( stderr, "[ERROR] The answer was not set.\n" );
						about( argc, argv );
						result = -2;
						goto done;
					}

					char *ttl_bad_char = NULL;
					args.ttl = strtol(argv[arg + 4], &ttl_bad_char, 10);

					if( *ttl_bad_char || args.ttl <= 0 )
					{
						fprintf( stderr, "[ERROR] Malformed ttl value (it must be a valid positive integer).  %p \n", ttl_bad_char );
						result = -1;
						goto done;
					}

					args.command = COMMAND_SET;
					arg += 5;
				}
				else
				{
					fprintf( stderr, "[ERROR] Missing required parameters for set operation.\n" );
					result = -1;
					goto done;
				}
			}
			else if( strcmp( "-d", argv[arg] ) == 0 || strcmp( "--delete", argv[arg] ) == 0 )
			{
				if( (arg + 1) < argc )
				{
					const char* fqdn = argv[arg + 1];

					if( fqdn && !separate_fqdn(fqdn, &args.host, &args.domain) )
					{
						fprintf( stderr, "[ERROR] Delete parameter is not a fully qualified domain name.\n" );
						result = -1;
						goto done;
					}

					args.command = COMMAND_DELETE;
					arg += 2;
				}
				else
				{
					fprintf( stderr, "[ERROR] Missing required parameter for delete operation.\n" );
					result = -1;
					goto done;
				}
			}
			else if( strcmp( "-u", argv[arg] ) == 0 || strcmp( "--username", argv[arg] ) == 0 )
			{
				if( (arg + 1) < argc )
				{
					args.username = argv[ arg + 1 ];
					arg += 2;
				}
				else
				{
					fprintf( stderr, "[ERROR] Missing required parameter for Name.com username.\n" );
					result = -1;
					goto done;
				}
			}
			else if( strcmp( "-t", argv[arg] ) == 0 || strcmp( "--token", argv[arg] ) == 0 )
			{
				if( (arg + 1) < argc )
				{
					args.token = argv[ arg + 1 ];
					arg += 2;
				}
				else
				{
					fprintf( stderr, "[ERROR] Missing required parameter for Name.com authentication token.\n" );
					result = -1;
					goto done;
				}
			}
			else
			{
				console_fg_color_256( stderr, CONSOLE_COLOR256_RED );
				printf( "\n" );
				fprintf( stderr, "ERROR: " );
				console_reset( stderr );
				fprintf( stderr, "Unrecognized command line option '%s'\n", argv[arg] );
				about( argc, argv );
				result = -2;
				goto done;
			}
		}
	}

	if( args.command == COMMAND_NOT_SET )
	{
        fprintf( stderr, "[ERROR] No command was specified.\n" );
		about( argc, argv );
		result = -2;
		goto done;
	}

	if( !args.username || *args.username == '\0' )
	{
        fprintf( stderr, "[ERROR] The name.com username was not set.\n" );
		about( argc, argv );
		result = -2;
		goto done;
	}

	if( !args.token || *args.token == '\0' )
	{
        fprintf( stderr, "[ERROR] The name.com API token was not set.\n" );
		about( argc, argv );
		result = -2;
		goto done;
	}

	/*
	*/


	banner();
	printf( "\n" );

	curl_global_init(CURL_GLOBAL_DEFAULT);

#if 1
	namecom_api_t* api = namecom_api_create( args.username,  args.token, false, args.verbose );
#else
	namecom_api_t* api = namecom_api_create( args.username, args.token, true, args.verbose );
#endif

	if( api )
	{
		if( !namecom_api_login( api ) )
		{
			fprintf( stderr, "[ERROR] Failed to login to name.com.\n" );
			result = -4;
			goto done;
		}


		namecom_api_dns_record_t** records = namecom_api_dns_record_list( api, args.domain );

		if( !records )
		{
			fprintf( stderr, "[ERROR] Failed to retrieve records for %s.\n", args.domain );
			result = -2;
			goto done;
		}


		switch(args.command)
		{
			case COMMAND_LIST:
			{
				int max_fqdn_len = 8;
				int max_content_len = 8;
				for( int i = 0; i < lc_vector_size(records); i++ )
				{
					namecom_api_dns_record_t* r = records[ i ];
					int fqdn_len = strlen(r->fqdn);
					int content_len = strlen(r->content);

					if( fqdn_len > max_fqdn_len )
					{
						max_fqdn_len = fqdn_len;
					}
					if( content_len > max_content_len )
					{
						max_content_len = content_len;
					}
				}

#if 1
				// header
				printf("\u250c");
				for(int i = 0; i < (39 + max_fqdn_len + max_content_len); i++ )
				{
					printf("\u2500");
				}
				printf("\u2510\n");
				printf("\u2502 %5s  %-*s  %-*s  %5s  %-19s \u2502\n", "Type", max_fqdn_len, "Host", max_content_len, "Answer", "TTL", "Created");

				// divider
				printf("\u251c");
				for(int i = 0; i < (39 + max_fqdn_len + max_content_len); i++ )
				{
					printf("\u2500");
				}
				printf("\u2524\n");

				for( int i = 0; i < lc_vector_size(records); i++ )
				{
					namecom_api_dns_record_t* r = records[ i ];
					printf("\u2502 %5s  %-*s  %-*s  %5d  %-19s \u2502\n", r->type, max_fqdn_len, r->fqdn, max_content_len, r->content, r->ttl, r->create_date);
				}

				// footer
				printf("\u2514");
				for(int i = 0; i < (39 + max_fqdn_len + max_content_len); i++ )
				{
					printf("\u2500");
				}
				printf("\u2518\n");
#else
				char fqdn_border[ max_fqdn_len + 1 ];
				memset(fqdn_border, '-', sizeof(fqdn_border));
				fqdn_border[ sizeof(fqdn_border) - 1 ] = '\0';

				char content_border[ max_content_len + 1 ];
				memset(content_border, '-', sizeof(content_border));
				content_border[ sizeof(content_border) - 1 ] = '\0';

				printf("+-%5s--%-*s--%-*s--%5s--%19s-+\n", "-----", max_fqdn_len, fqdn_border, max_content_len, content_border, "-----", "-------------------");
				printf("| %5s  %-*s  %-*s  %5s  %-19s |\n", "Type", max_fqdn_len, "Host", max_content_len, "Answer", "TTL", "Created");
				printf("+-%5s--%-*s--%-*s--%5s--%19s-+\n", "-----", max_fqdn_len, fqdn_border, max_content_len, content_border, "-----", "-------------------");
				for( int i = 0; i < lc_vector_size(records); i++ )
				{
					namecom_api_dns_record_t* r = records[ i ];
					printf("| %5s  %-*s  %-*s  %5d  %-19s |\n", r->type, max_fqdn_len, r->fqdn, max_content_len, r->content, r->ttl, r->create_date);
				}
				printf("+-%5s--%-*s--%-*s--%5s--%19s-+\n", "-----", max_fqdn_len, fqdn_border, max_content_len, content_border, "-----", "-------------------");
#endif
				break;
			}
			case COMMAND_SET:
			{
				long record_id = -1;
				bool record_exists = false;

				for( int i = 0; i < lc_vector_size(records); i++ )
				{
					namecom_api_dns_record_t* r = records[ i ];
					char record_fqdn[ 256 ];
					snprintf( record_fqdn, sizeof(record_fqdn), "%s.%s", args.host, args.domain );
					record_fqdn[ sizeof(record_fqdn) - 1 ] = '\0';

					if( strcmp(r->fqdn, record_fqdn) == 0 )
					{
						record_id = r->id;
						record_exists = true;
					}
				}

				if( record_exists )
				{
					if( namecom_api_dns_record_remove( api, args.domain, record_id ) &&
						namecom_api_dns_record_add( api, args.domain, args.host, args.type, args.answer, args.ttl, 10, &record_id ) )
					{
						printf( "Record updated.\n" );
					}
					else
					{
						fprintf( stderr, "[ERROR] Failed to update record.\n" );
					}
				}
				else
				{
					if( namecom_api_dns_record_add( api, args.domain, args.host, args.type, args.answer, args.ttl, 10, &record_id ) )
					{
						printf( "Record created.\n" );
					}
					else
					{
						fprintf( stderr, "[ERROR] Failed to create record (%s.%s --> %s).\n", args.host, args.domain, args.answer );
					}
				}

				break;
			}
			case COMMAND_DELETE:
			{
				long record_id = -1;
				bool record_exists = false;

				for( int i = 0; i < lc_vector_size(records); i++ )
				{
					namecom_api_dns_record_t* r = records[ i ];
					char record_fqdn[ 256 ];
					snprintf( record_fqdn, sizeof(record_fqdn), "%s.%s", args.host, args.domain );
					record_fqdn[ sizeof(record_fqdn) - 1 ] = '\0';

					if( strcmp(r->fqdn, record_fqdn) == 0 )
					{
						record_id = r->id;
						record_exists = true;
					}
				}

				if( record_exists )
				{

					bool deleteSuccessful = namecom_api_dns_record_remove( api, args.domain, record_id );

					if( deleteSuccessful )
					{
						printf( "Record deleted.\n" );
					}
					else
					{
						fprintf( stderr, "[ERROR] Failed to delete record (%s.%s).\n", args.host, args.domain );
					}

				}
				else
				{
					printf( "Record does not exist.\n" );
				}


				break;
			}
			default:
				break;
		}

		if( records ) lc_vector_destroy( records );
		namecom_api_logout( api );
		namecom_api_destroy( api );
	}

	curl_global_cleanup();

done:
	if( args.host )   free( args.host );
	if( args.domain ) free( args.domain );
	if( args.type )   free( args.type );
	if( args.answer ) free( args.answer );
	return result;
}

void banner( void )
{
	console_fg_color_256( stdout, CONSOLE_COLOR256_BRIGHT_GREEN );
	printf("  _ __   __ _ _ __ ___   ___ ");
	console_reset( stdout );
	printf("   ___ ___  _ __ ___  \n");

	console_fg_color_256( stdout, CONSOLE_COLOR256_BRIGHT_GREEN );
	printf(" | '_ \\ / _` | '_ ` _ \\ / _ \\");
	console_reset( stdout );
	printf("  / __/ _ \\| '_ ` _ \\ \n");


	console_fg_color_256( stdout, CONSOLE_COLOR256_BRIGHT_GREEN );
	printf(" | | | | (_| | | | | | |  __/");
	console_reset( stdout );
	printf(" | (_| (_) | | | | | |\n");

	console_fg_color_256( stdout, CONSOLE_COLOR256_BRIGHT_GREEN );
	printf(" |_| |_|\\__,_|_| |_| |_|\\___|");
	console_reset( stdout );
	printf("(_)___\\___/|_| |_| |_|\n");

	console_fg_color_256( stdout, CONSOLE_COLOR256_YELLOW );
	printf( "                DNS Record Manager\n");
	console_reset( stdout );
}

void about( int argc, char* argv[] )
{
	banner( );
	printf( "----------------------------------------------------\n");
	printf( "        Copyright (c) 2016, Joe Marrero.\n");
	printf( "             http://joemarrero.com/\n");
	printf( "\n" );

	printf( "Example Usage:\n" );
	printf( "    %s -h <host> -u <username> -t <api-token> -l example.com\n", argv[0] );
	printf( "    %s -h <host> -u <username> -t <api-token> -s CNAME something.other.com example.com 300\n", argv[0] );
	printf( "    %s -h <host> -u <username> -t <api-token> -d something.other.com\n", argv[0] );
	printf( "\n\n" );

	printf( "Parameters can also be passed via environment variables:\n" );
	printf( "    %-20s  %-50s\n", "NAMECOM_USERNAME", "The name.com username to use." );
	printf( "    %-20s  %-50s\n", "NAMECOM_API_TOKEN", "The name.com API token to use." );
	printf( "\n\n" );

	printf( "Command Line Options:\n" );
	printf( "    %-2s, %-12s   %-50s\n", "-u", "--username", "The name.com username." );
	printf( "    %-2s, %-12s   %-50s\n", "-t", "--token", "The name.com API token." );
	printf( "    %-2s, %-12s   %-50s\n", "-l", "--list", "List DNS records for domain." );
	printf( "    %-2s, %-12s   %-50s\n", "-s", "--set", "Set a DNS record." );
	printf( "    %-2s, %-12s   %-50s\n", "-d", "--delete", "Delete a DNS record." );
	printf( "\n\n" );

	printf( "If you don't already have a Name.com API token, then you may apply for\n" );
	printf( "one at: ");
	console_fg_color_256( stdout, CONSOLE_COLOR256_BRIGHT_MAGENTA );
	printf( "http://name.com/reseller" );
	console_reset( stdout );
	printf( "\n\n" );

	printf( "If you found this utility useful, please consider making a donation\n" );
	printf( "of bitcoin to: " );
	console_fg_color_256( stdout, CONSOLE_COLOR256_BRIGHT_MAGENTA );
	printf( "3A5M1m2BNSBgo9V7B8wf6VtWQDMMgp5abZ" );
	console_reset( stdout );
	printf( "\n\n" );
	printf( "All donations help cover maintenance costs and are much appreciated.\n" );

	printf( "\n" );
}

bool separate_fqdn( const char* fqdn, char** host, char** domain )
{
	bool result = true;
	if( fqdn )
	{
		char* dot = strchr( fqdn, '.' );
		if( dot )
		{
			*host = string_substring( fqdn, 0, dot - fqdn );

			if( *host && **host == '\0' )
			{
				result = false;
				free( *host );
				goto done;
			}

			*domain = string_dup( dot + 1 );

			if( *domain && **domain == '\0' )
			{
				result = false;
				free( *host );
				free( *domain );
				goto done;
			}
		}
		else
		{
			result = false;
			goto done;
		}
	}

done:
	return result;
}
