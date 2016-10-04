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
#include <unistd.h>
#include <curl/curl.h>
#include <libutility/utility.h>
#include <libutility/console.h>
#include <libcollections/vector.h>
#include "ipify.h"
#include "namecom_api.h"

#define VERSION  "1.0"

static void banner( void );
static void about( int argc, char* argv[] );
static bool separate_fqdn( const char* fqdn, char** host, char** domain );


typedef struct {
	char* host;
	char* domain;
	char* ip_address;
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
		.ip_address = NULL,
		.username   = getenv( "NAMECOM_USERNAME" ),
		.token      = getenv( "NAMECOM_API_TOKEN" ),
		.verbose    = false
	};

	const char* fqdn = getenv( "NAMECOM_HOST" );

	if( fqdn && !separate_fqdn(fqdn, &args.host, &args.domain) )
	{
		fprintf( stderr, "[ERROR] Malformed fully qualified domain name in NAMECOM_HOST environment variable.\n" );
		result = -1;
		goto done;
	}

	if( fqdn )
	{
		args.host   = string_substring( fqdn, 0, strchr(fqdn, '.') - fqdn );
		args.domain = string_dup( strchr(fqdn, '.') + 1 );
	}

	if( argc > 0 )
	{
		for( int arg = 1; arg < argc; arg++ )
		{
			if( strcmp( "-h", argv[arg] ) == 0 || strcmp( "--host", argv[arg] ) == 0 )
			{
				if( (arg + 1) < argc )
				{
					fqdn = argv[ arg + 1 ];
					arg++;
				}
				else
				{
					fprintf( stderr, "[ERROR] The domain name argument is missing.\n" );
					result = -1;
					goto done;
				}

				if( args.host )
				{
					free( args.host );
				}

				if( args.domain )
				{
					free( args.domain );
				}

				if( fqdn && !separate_fqdn(fqdn, &args.host, &args.domain) )
				{
					fprintf( stderr, "[ERROR] Malformed domain name in command line parameters.\n" );
					result = -1;
					goto done;
				}
			}
			else if( strcmp( "-a", argv[arg] ) == 0 || strcmp( "--ip-address", argv[arg] ) == 0 )
			{
				if( (arg + 1) < argc )
				{
					args.ip_address = string_dup(argv[ arg + 1 ]);
					arg++;
				}
				else
				{
					fprintf( stderr, "[ERROR] The address argument is missing.\n" );
					result = -1;
					goto done;
				}
			}
			else if( strcmp( "-u", argv[arg] ) == 0 || strcmp( "--username", argv[arg] ) == 0 )
			{
				if( (arg + 1) < argc )
				{
					args.username = argv[ arg + 1 ];
					arg++;
				}
				else
				{
					fprintf( stderr, "[ERROR] The name.com username argument is missing.\n" );
					result = -1;
					goto done;
				}
			}
			else if( strcmp( "-t", argv[arg] ) == 0 || strcmp( "--token", argv[arg] ) == 0 )
			{
				if( (arg + 1) < argc )
				{
					args.token = argv[ arg + 1 ];
					arg++;
				}
				else
				{
					fprintf( stderr, "[ERROR] The name.com API token argument is missing.\n" );
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

	if( !args.host || *args.host == '\0' )
	{
        fprintf( stderr, "[ERROR] The hostname was not set.\n" );
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


	banner();
	printf( "\n" );

	curl_global_init(CURL_GLOBAL_DEFAULT);

	if( !args.ip_address )
	{
		// If no IP address is passed then try to figure out the
		// public IP address.
		args.ip_address = ipify_public_ip();

		if( !args.ip_address )
		{
			fprintf( stderr, "[ERROR] Failed to determine public IP address.\n" );
			result = -3;
			goto done;
		}
	}

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

		long record_id = -1;
		bool record_exists = false;
		bool record_needs_update = false;

		namecom_api_dns_record_t** records = namecom_api_dns_record_list( api, args.domain );

		if( records )
		{
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

					if( strcmp(r->content, args.ip_address) != 0 )
					{
						record_needs_update = true;
					}
				}
			}

			lc_vector_destroy( records );
		}
		else
		{
			fprintf( stderr, "[ERROR] Failed to list records.\n" );
		}

		if( record_needs_update )
		{
			if( namecom_api_dns_record_remove( api, args.domain, record_id ) &&
				namecom_api_dns_record_add( api, args.domain, args.host, "A", args.ip_address, 60, 10, &record_id ) )
			{
				printf( "Record updated.\n" );
			}
			else
			{
				fprintf( stderr, "[ERROR] Failed to update record.\n" );
			}
		}
		else if( !record_exists )
		{
			if( namecom_api_dns_record_add( api, args.domain, args.host, "A", args.ip_address, 60, 10, &record_id ) )
			{
				printf( "Record created.\n" );
			}
			else
			{
				fprintf( stderr, "[ERROR] Failed to create record (%s.%s --> %s).\n", args.host, args.domain, args.ip_address );
			}
		}
		else
		{
			printf( "Record up to date.\n" );
		}

		namecom_api_logout( api );
		namecom_api_destroy( api );
	}

	curl_global_cleanup();

done:
	if( args.host )       free( args.host );
	if( args.domain )     free( args.domain );
	if( args.ip_address ) free( args.ip_address );
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

	console_fg_color_256( stdout, CONSOLE_COLOR256_BRIGHT_YELLOW );
	printf( "               Dynamic DNS Updater\n");
	console_reset( stdout );
}

void about( int argc, char* argv[] )
{
	banner( );
	printf( "----------------------------------------------------\n");
	printf( "        Copyright (c) 2016, Joe Marrero.\n");
	printf( "           http://www.joemarrero.com/\n");
	printf( "\n" );

	printf( "Usage:\n" );
	printf( "    %s -h <host> -u <username> -t <api-token>", argv[0] );
	printf( "\n\n" );

	printf( "Parameters can also be passed via environment variables:\n" );
	printf( "    %-20s  %-50s\n", "NAMECOM_USERNAME", "The name.com username to use." );
	printf( "    %-20s  %-50s\n", "NAMECOM_API_TOKEN", "The name.com API token to use." );
	printf( "\n\n" );

	printf( "Command Line Options:\n" );
	printf( "    %-2s, %-12s   %-50s\n", "-u", "--username", "The name.com username." );
	printf( "    %-2s, %-12s   %-50s\n", "-t", "--token", "The name.com API token." );
	printf( "    %-2s, %-12s   %-50s\n", "-h", "--host", "The DNS record's hostname to update." );
	//printf( "    %-2s, %-12s   %-50s\n", "-d", "--domain", "The domain name." );
	printf( "    %-2s, %-12s   %-50s\n", "-a", "--ip-address", "An optional IP address to use." );
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
